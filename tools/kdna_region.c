#include "kdna.h"
#include "kdna_ksoa.h"
#include "kdna_kreg.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum kdna_region_exit {
    KDNA_REGION_OK = 0,
    KDNA_REGION_USAGE = 2,
    KDNA_REGION_RUNTIME = 3
};

enum split_mode {
    SPLIT_BOTH = 0,
    SPLIT_RAW = 1,
    SPLIT_DOM = 2
};

typedef struct ksoa_file {
    const char *path;
    kdna_ksoa_header h;
    double *payload;
} ksoa_file;

static void usage(FILE *f) {
    fprintf(f,
        "kdna_region --in <scan.ksoa> --out <regions.kreg> [--split both|raw|dom]\n"
        "\n"
        "Extracts a KREG v1 semantic topology from a KSOA v1 resonance field.\n"
        "\n"
        "Default split mode is 'both': each record is a maximal contiguous segment\n"
        "where the pair (RAW, D) is constant. The output record stores index range,\n"
        "x range, RAW, D, L_K min/max and dominanceScore min/max.\n"
        "\n"
        "KREG v1 binary contract:\n"
        "  header: 128 bytes, magic KREG0001, version 1\n"
        "  records: 80 bytes each, ordered by i0, covering the source scan without gaps\n"
        "\n"
        "Examples:\n"
        "  kdna_region --in scan_cpu.ksoa --out regions_cpu.kreg\n"
        "  kdna_region --in scan_cpu.ksoa --out raw_regions.kreg --split raw\n"
    );
}

static int checked_mul_size(size_t a, size_t b, size_t *out) {
    if (a != 0u && b > ((size_t)-1) / a) return 0;
    *out = a * b;
    return 1;
}

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

static void free_ksoa(ksoa_file *kf) {
    if (kf) {
        free(kf->payload);
        kf->payload = NULL;
    }
}

static int read_ksoa_payload(const char *path, ksoa_file *out) {
    if (!path || !out) return KDNA_EINVAL;
    memset(out, 0, sizeof(*out));
    out->path = path;

    int rc = kdna_ksoa_read_header_file(path, &out->h);
    if (rc != KDNA_OK) return rc;

    size_t cells = 0u;
    if (!checked_mul_size((size_t)out->h.fields, (size_t)out->h.n, &cells)) return KDNA_EINVAL;
    if (out->h.payload_bytes != (uint64_t)cells * (uint64_t)sizeof(double)) return KDNA_EINVAL;

    out->payload = (double *)calloc(cells, sizeof(double));
    if (!out->payload) return KDNA_ENOMEM;

    FILE *f = fopen(path, "rb");
    if (!f) {
        free_ksoa(out);
        return KDNA_EIO;
    }

    int ok = 1;
    if (fseek(f, (long)out->h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && !read_exact(f, out->payload, (size_t)out->h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;

    if (!ok) {
        free_ksoa(out);
        return KDNA_EIO;
    }
    return KDNA_OK;
}

static int as_operator_id(double v) {
    const int id = (int)llround(v);
    return (id >= 1 && id <= 5) ? id : 0;
}

static int same_region(int raw_a, int dom_a, int raw_b, int dom_b, enum split_mode mode) {
    switch (mode) {
        case SPLIT_RAW: return raw_a == raw_b;
        case SPLIT_DOM: return dom_a == dom_b;
        case SPLIT_BOTH:
        default: return raw_a == raw_b && dom_a == dom_b;
    }
}

static int append_record(kdna_kreg_record **records,
                         size_t *count,
                         size_t *cap,
                         const ksoa_file *kf,
                         size_t i0,
                         size_t i1) {
    if (!records || !count || !cap || !kf || i1 < i0) return KDNA_EINVAL;

    if (*count == *cap) {
        size_t next = (*cap == 0u) ? 64u : (*cap * 2u);
        if (next < *cap) return KDNA_ENOMEM;
        kdna_kreg_record *tmp = (kdna_kreg_record *)realloc(*records, next * sizeof(kdna_kreg_record));
        if (!tmp) return KDNA_ENOMEM;
        *records = tmp;
        *cap = next;
    }

    const size_t n = (size_t)kf->h.n;
    const double *x = kf->payload + kdna_idx(KDNA_X, n, 0u);
    const double *raw = kf->payload + kdna_idx(KDNA_RAW, n, 0u);
    const double *dom = kf->payload + kdna_idx(KDNA_DOM, n, 0u);
    const double *lk = kf->payload + kdna_idx(KDNA_LK, n, 0u);
    const double *score = kf->payload + kdna_idx(KDNA_DOM_SCORE, n, 0u);

    kdna_kreg_record r;
    memset(&r, 0, sizeof(r));
    r.i0 = (uint64_t)i0;
    r.i1 = (uint64_t)i1;
    r.x0 = x[i0];
    r.x1 = x[i1];
    r.raw = (uint32_t)as_operator_id(raw[i0]);
    r.dom = (uint32_t)as_operator_id(dom[i0]);
    r.flags = 0u;
    r.lock_min = INFINITY;
    r.lock_max = -INFINITY;
    r.score_min = INFINITY;
    r.score_max = -INFINITY;

    for (size_t i = i0; i <= i1; ++i) {
        const double l = lk[i];
        const double s = score[i];
        if (isfinite(l)) {
            if (l < r.lock_min) r.lock_min = l;
            if (l > r.lock_max) r.lock_max = l;
        }
        if (isfinite(s)) {
            if (s < r.score_min) r.score_min = s;
            if (s > r.score_max) r.score_max = s;
        }
        if (i == i1) break; /* avoid wrap if i1 == SIZE_MAX, defensive */
    }

    if (!isfinite(r.lock_min)) r.lock_min = NAN;
    if (!isfinite(r.lock_max)) r.lock_max = NAN;
    if (!isfinite(r.score_min)) r.score_min = NAN;
    if (!isfinite(r.score_max)) r.score_max = NAN;

    (*records)[*count] = r;
    *count += 1u;
    return KDNA_OK;
}

static int extract_regions(const ksoa_file *kf,
                           enum split_mode mode,
                           kdna_kreg_record **records_out,
                           size_t *count_out) {
    if (!kf || !records_out || !count_out) return KDNA_EINVAL;
    const size_t n = (size_t)kf->h.n;
    if (n == 0u) return KDNA_EINVAL;

    kdna_kreg_record *records = NULL;
    size_t count = 0u;
    size_t cap = 0u;

    const double *raw = kf->payload + kdna_idx(KDNA_RAW, n, 0u);
    const double *dom = kf->payload + kdna_idx(KDNA_DOM, n, 0u);

    size_t start = 0u;
    int prev_raw = as_operator_id(raw[0]);
    int prev_dom = as_operator_id(dom[0]);

    for (size_t i = 1u; i < n; ++i) {
        const int cur_raw = as_operator_id(raw[i]);
        const int cur_dom = as_operator_id(dom[i]);
        if (!same_region(prev_raw, prev_dom, cur_raw, cur_dom, mode)) {
            int rc = append_record(&records, &count, &cap, kf, start, i - 1u);
            if (rc != KDNA_OK) {
                free(records);
                return rc;
            }
            start = i;
            prev_raw = cur_raw;
            prev_dom = cur_dom;
        }
    }

    int rc = append_record(&records, &count, &cap, kf, start, n - 1u);
    if (rc != KDNA_OK) {
        free(records);
        return rc;
    }

    *records_out = records;
    *count_out = count;
    return KDNA_OK;
}

static const char *split_mode_name(enum split_mode mode) {
    switch (mode) {
        case SPLIT_RAW: return "raw";
        case SPLIT_DOM: return "dom";
        case SPLIT_BOTH:
        default: return "both";
    }
}

int main(int argc, char **argv) {
    const char *in_path = NULL;
    const char *out_path = NULL;
    enum split_mode mode = SPLIT_BOTH;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
            usage(stdout);
            return KDNA_REGION_OK;
        } else if (strcmp(a, "--in") == 0 && i + 1 < argc) {
            in_path = argv[++i];
        } else if (strcmp(a, "--out") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (strcmp(a, "--split") == 0 && i + 1 < argc) {
            const char *s = argv[++i];
            if (strcmp(s, "both") == 0) {
                mode = SPLIT_BOTH;
            } else if (strcmp(s, "raw") == 0) {
                mode = SPLIT_RAW;
            } else if (strcmp(s, "dom") == 0) {
                mode = SPLIT_DOM;
            } else {
                fprintf(stderr, "kdna_region: invalid --split '%s'\n", s);
                return KDNA_REGION_USAGE;
            }
        } else {
            fprintf(stderr, "kdna_region: unknown or incomplete argument '%s'\n", a);
            usage(stderr);
            return KDNA_REGION_USAGE;
        }
    }

    if (!in_path || !out_path) {
        fprintf(stderr, "kdna_region: --in and --out are required\n");
        usage(stderr);
        return KDNA_REGION_USAGE;
    }

    ksoa_file kf;
    int rc = read_ksoa_payload(in_path, &kf);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_region: cannot read '%s': %s\n", in_path, kdna_status_str(rc));
        return KDNA_REGION_RUNTIME;
    }

    kdna_kreg_record *records = NULL;
    size_t segment_count = 0u;
    rc = extract_regions(&kf, mode, &records, &segment_count);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_region: extraction failed: %s\n", kdna_status_str(rc));
        free_ksoa(&kf);
        return KDNA_REGION_RUNTIME;
    }

    kdna_kreg_header h;
    rc = kdna_kreg_init_header(&h,
                               segment_count,
                               (size_t)kf.h.n,
                               kf.h.x_min,
                               kf.h.x_max,
                               kf.h.dx);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_region: cannot create KREG header: %s\n", kdna_status_str(rc));
        free(records);
        free_ksoa(&kf);
        return KDNA_REGION_RUNTIME;
    }

    rc = kdna_kreg_write_file(out_path, &h, records);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_region: cannot write '%s': %s\n", out_path, kdna_status_str(rc));
        free(records);
        free_ksoa(&kf);
        return KDNA_REGION_RUNTIME;
    }

    printf("kdna_region: wrote %s split=%s source=%s segments=%zu source_n=%" PRIu64
           " record_bytes=%u payload_bytes=%" PRIu64 "\n",
           out_path,
           split_mode_name(mode),
           in_path,
           segment_count,
           kf.h.n,
           (unsigned)KDNA_KREG_RECORD_BYTES,
           h.payload_bytes);

    free(records);
    free_ksoa(&kf);
    return KDNA_REGION_OK;
}
