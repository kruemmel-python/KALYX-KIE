#include "kdna.h"
#include "kdna_ksoa.h"
#include "kdna_kreg.h"
#include "kdna_klib.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum kdna_library_exit {
    KDNA_LIBRARY_OK = 0,
    KDNA_LIBRARY_USAGE = 2,
    KDNA_LIBRARY_RUNTIME = 3
};

typedef struct ksoa_file {
    const char *path;
    kdna_ksoa_header h;
    double *payload;
} ksoa_file;

typedef struct kreg_file {
    const char *path;
    kdna_kreg_header h;
    kdna_kreg_record *records;
} kreg_file;

static void usage(FILE *f) {
    fprintf(f,
        "kdna_library --ksoa <scan.ksoa> --kreg <regions.kreg> --out <library.klib>\n"
        "\n"
        "Compiles KSOA + KREG into KLIB v1 resonance vocabulary records.\n"
        "Each KWORD is one measured causal vocabulary unit for one KREG segment.\n"
        "\n"
        "KLIB v1 binary contract:\n"
        "  header: 128 bytes, magic KLIB0001, version 1\n"
        "  records: 320 bytes each, one record per KREG segment\n"
        "  all statistics are computed from the original KSOA payload using SoA layout\n"
        "\n"
        "Effect classes are derived from numeric evidence, not supplied by the caller.\n"
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

static void free_kreg(kreg_file *kr) {
    if (kr) {
        free(kr->records);
        kr->records = NULL;
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

static int read_kreg_payload(const char *path, kreg_file *out) {
    if (!path || !out) return KDNA_EINVAL;
    memset(out, 0, sizeof(*out));
    out->path = path;

    int rc = kdna_kreg_read_header_file(path, &out->h);
    if (rc != KDNA_OK) return rc;

    out->records = (kdna_kreg_record *)calloc((size_t)out->h.segment_count, sizeof(kdna_kreg_record));
    if (!out->records) return KDNA_ENOMEM;

    FILE *f = fopen(path, "rb");
    if (!f) {
        free_kreg(out);
        return KDNA_EIO;
    }

    int ok = 1;
    if (fseek(f, (long)out->h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && !read_exact(f, out->records, (size_t)out->h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;

    if (!ok) {
        free_kreg(out);
        return KDNA_EIO;
    }
    return KDNA_OK;
}

static int nearly_equal(double a, double b) {
    const double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
    return fabs(a - b) <= 1.0e-12 * scale;
}

static int validate_lineage(const ksoa_file *kf, const kreg_file *kr) {
    if (!kf || !kr) return KDNA_EINVAL;
    if (kf->h.n != kr->h.source_n) return KDNA_EINVAL;
    if (kf->h.fields != kr->h.source_fields) return KDNA_EINVAL;
    if (!nearly_equal(kf->h.x_min, kr->h.x_min)) return KDNA_EINVAL;
    if (!nearly_equal(kf->h.x_max, kr->h.x_max)) return KDNA_EINVAL;
    if (!nearly_equal(kf->h.dx, kr->h.dx)) return KDNA_EINVAL;
    if (kr->h.segment_count == 0u) return KDNA_EINVAL;

    const kdna_kreg_record *records = kr->records;
    if (records[0].i0 != 0u) return KDNA_EINVAL;
    if (records[kr->h.segment_count - 1u].i1 != kr->h.source_n - 1u) return KDNA_EINVAL;

    for (uint64_t i = 0u; i < kr->h.segment_count; ++i) {
        const kdna_kreg_record *r = &records[i];
        if (r->i0 > r->i1 || r->i1 >= kr->h.source_n) return KDNA_EINVAL;
        if (r->raw < 1u || r->raw > 5u || r->dom < 1u || r->dom > 5u) return KDNA_EINVAL;
        if (i > 0u && records[i - 1u].i1 + 1u != r->i0) return KDNA_EINVAL;
    }
    return KDNA_OK;
}

static double finite_or_zero(double x) {
    return isfinite(x) ? x : 0.0;
}

static uint32_t classify_effect(const kdna_kword_record *w, double dx_abs) {
    if (!w) return KDNA_EFFECT_UNKNOWN;

    const double near_eps = fmax(dx_abs * 4.0, 1.0e-12);
    const int near_zero = (fabs(w->x0) <= near_eps) || (fabs(w->x1) <= near_eps) || (w->x0 <= 0.0 && w->x1 >= 0.0);
    if (near_zero && (w->k_absmax[0] > 1.0e6 || w->score_max > 10.0 || w->raw == 1u || w->dom == 1u || w->dom == 4u)) {
        return KDNA_EFFECT_NULL_MEMBRANE_JUMP;
    }

    if (w->sample_count <= 3u || fabs(w->width) <= dx_abs * 3.0) {
        return KDNA_EFFECT_TRANSITION_BRIDGE;
    }

    if (w->dom == 4u || w->s_absmax[3] >= 1.0) {
        return KDNA_EFFECT_COMPRESSION_GATE;
    }

    if (w->dom == 5u || w->s_absmax[4] >= 1.0) {
        return KDNA_EFFECT_CASCADE_BAND;
    }

    if ((w->dom == 1u || w->raw == 1u) && w->lock_mean >= 0.99) {
        return KDNA_EFFECT_STABLE_ATTRACTOR;
    }

    if (w->dom == 2u) {
        return KDNA_EFFECT_ENVELOPE_FORM;
    }

    if (w->dom == 3u) {
        return KDNA_EFFECT_PHASE_OFFSET;
    }

    if (w->raw != w->dom) {
        return KDNA_EFFECT_RAW_DOM_MISMATCH_ZONE;
    }

    return KDNA_EFFECT_UNKNOWN;
}

static uint32_t derive_flags(const kdna_kword_record *w, double dx_abs) {
    uint32_t flags = 0u;
    const double near_eps = fmax(dx_abs * 4.0, 1.0e-12);
    if ((fabs(w->x0) <= near_eps) || (fabs(w->x1) <= near_eps) || (w->x0 <= 0.0 && w->x1 >= 0.0)) {
        flags |= KDNA_KWORD_FLAG_NULL_NEAR;
    }
    if (w->raw != w->dom) flags |= KDNA_KWORD_FLAG_RAW_DOM_MISMATCH;
    if (w->lock_mean >= 0.99) flags |= KDNA_KWORD_FLAG_HIGH_LOCK;
    if (w->sample_count <= 3u || fabs(w->width) <= dx_abs * 3.0) flags |= KDNA_KWORD_FLAG_NARROW;
    if (w->score_max >= 10.0) flags |= KDNA_KWORD_FLAG_HIGH_SCORE;
    return flags;
}

static int compile_words(const ksoa_file *kf, const kreg_file *kr, kdna_kword_record **words_out) {
    if (!kf || !kr || !words_out) return KDNA_EINVAL;

    const size_t n = (size_t)kf->h.n;
    const size_t count = (size_t)kr->h.segment_count;
    kdna_kword_record *words = (kdna_kword_record *)calloc(count, sizeof(kdna_kword_record));
    if (!words) return KDNA_ENOMEM;

    const double *x = kf->payload + kdna_idx(KDNA_X, n, 0u);
    const double *lk = kf->payload + kdna_idx(KDNA_LK, n, 0u);
    const double *ek = kf->payload + kdna_idx(KDNA_EK, n, 0u);
    const double *ak = kf->payload + kdna_idx(KDNA_AK, n, 0u);
    const double *score = kf->payload + kdna_idx(KDNA_DOM_SCORE, n, 0u);

    const double *k_planes[5] = {
        kf->payload + kdna_idx(KDNA_K01, n, 0u),
        kf->payload + kdna_idx(KDNA_K02, n, 0u),
        kf->payload + kdna_idx(KDNA_K03, n, 0u),
        kf->payload + kdna_idx(KDNA_K04, n, 0u),
        kf->payload + kdna_idx(KDNA_K05, n, 0u)
    };
    const double *s_planes[5] = {
        kf->payload + kdna_idx(KDNA_S01, n, 0u),
        kf->payload + kdna_idx(KDNA_S02, n, 0u),
        kf->payload + kdna_idx(KDNA_S03, n, 0u),
        kf->payload + kdna_idx(KDNA_S04, n, 0u),
        kf->payload + kdna_idx(KDNA_S05, n, 0u)
    };

    const double dx_abs = fabs(kf->h.dx);

    for (size_t rix = 0u; rix < count; ++rix) {
        const kdna_kreg_record *r = &kr->records[rix];
        kdna_kword_record *w = &words[rix];
        memset(w, 0, sizeof(*w));

        w->id = (uint64_t)(rix + 1u);
        w->source_segment_index = (uint64_t)rix;
        w->i0 = r->i0;
        w->i1 = r->i1;
        w->x0 = r->x0;
        w->x1 = r->x1;
        w->width = r->x1 - r->x0;
        w->raw = r->raw;
        w->dom = r->dom;
        w->lock_min = INFINITY;
        w->lock_max = -INFINITY;
        w->score_min = INFINITY;
        w->score_max = -INFINITY;
        for (size_t op = 0u; op < 5u; ++op) {
            w->k_absmax[op] = 0.0;
            w->s_absmax[op] = 0.0;
        }

        uint64_t valid_count = 0u;
        for (uint64_t ii = r->i0; ii <= r->i1; ++ii) {
            const size_t i = (size_t)ii;
            const double l = finite_or_zero(lk[i]);
            const double sc = finite_or_zero(score[i]);
            const double e = finite_or_zero(ek[i]);
            const double a = finite_or_zero(ak[i]);

            if (l < w->lock_min) w->lock_min = l;
            if (l > w->lock_max) w->lock_max = l;
            if (sc < w->score_min) w->score_min = sc;
            if (sc > w->score_max) w->score_max = sc;

            w->lock_mean += l;
            w->score_mean += sc;
            w->ek_mean += e;
            w->ak_mean += a;

            for (size_t op = 0u; op < 5u; ++op) {
                const double kv = finite_or_zero(k_planes[op][i]);
                const double sv = finite_or_zero(s_planes[op][i]);
                w->k_mean[op] += kv;
                w->s_mean[op] += sv;
                if (fabs(kv) > w->k_absmax[op]) w->k_absmax[op] = fabs(kv);
                if (fabs(sv) > w->s_absmax[op]) w->s_absmax[op] = fabs(sv);
            }

            valid_count += 1u;
            if (ii == r->i1) break;
        }

        if (valid_count == 0u) {
            free(words);
            return KDNA_EINVAL;
        }

        const double inv = 1.0 / (double)valid_count;
        w->sample_count = valid_count;
        w->lock_mean *= inv;
        w->score_mean *= inv;
        w->ek_mean *= inv;
        w->ak_mean *= inv;
        for (size_t op = 0u; op < 5u; ++op) {
            w->k_mean[op] *= inv;
            w->s_mean[op] *= inv;
        }

        if (!isfinite(w->lock_min)) w->lock_min = 0.0;
        if (!isfinite(w->lock_max)) w->lock_max = 0.0;
        if (!isfinite(w->score_min)) w->score_min = 0.0;
        if (!isfinite(w->score_max)) w->score_max = 0.0;

        w->flags = derive_flags(w, dx_abs);
        w->effect_class = classify_effect(w, dx_abs);

        (void)x; /* retained for explicit lineage: x coordinates come from KREG records */
    }

    *words_out = words;
    return KDNA_OK;
}

int main(int argc, char **argv) {
    const char *ksoa_path = NULL;
    const char *kreg_path = NULL;
    const char *out_path = NULL;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
            usage(stdout);
            return KDNA_LIBRARY_OK;
        } else if (strcmp(a, "--ksoa") == 0 && i + 1 < argc) {
            ksoa_path = argv[++i];
        } else if (strcmp(a, "--kreg") == 0 && i + 1 < argc) {
            kreg_path = argv[++i];
        } else if (strcmp(a, "--out") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else {
            fprintf(stderr, "kdna_library: unknown or incomplete argument '%s'\n", a);
            usage(stderr);
            return KDNA_LIBRARY_USAGE;
        }
    }

    if (!ksoa_path || !kreg_path || !out_path) {
        fprintf(stderr, "kdna_library: --ksoa, --kreg and --out are required\n");
        usage(stderr);
        return KDNA_LIBRARY_USAGE;
    }

    ksoa_file kf;
    kreg_file kr;
    int rc = read_ksoa_payload(ksoa_path, &kf);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_library: cannot read KSOA '%s': %s\n", ksoa_path, kdna_status_str(rc));
        return KDNA_LIBRARY_RUNTIME;
    }

    rc = read_kreg_payload(kreg_path, &kr);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_library: cannot read KREG '%s': %s\n", kreg_path, kdna_status_str(rc));
        free_ksoa(&kf);
        return KDNA_LIBRARY_RUNTIME;
    }

    rc = validate_lineage(&kf, &kr);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_library: KSOA/KREG lineage mismatch\n");
        free_kreg(&kr);
        free_ksoa(&kf);
        return KDNA_LIBRARY_RUNTIME;
    }

    kdna_kword_record *words = NULL;
    rc = compile_words(&kf, &kr, &words);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_library: vocabulary compilation failed: %s\n", kdna_status_str(rc));
        free_kreg(&kr);
        free_ksoa(&kf);
        return KDNA_LIBRARY_RUNTIME;
    }

    kdna_klib_header h;
    rc = kdna_klib_init_header(&h,
                               (size_t)kr.h.segment_count,
                               (size_t)kf.h.n,
                               (size_t)kr.h.segment_count,
                               kf.h.x_min,
                               kf.h.x_max,
                               kf.h.dx);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_library: cannot create KLIB header: %s\n", kdna_status_str(rc));
        free(words);
        free_kreg(&kr);
        free_ksoa(&kf);
        return KDNA_LIBRARY_RUNTIME;
    }

    rc = kdna_klib_write_file(out_path, &h, words);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_library: cannot write '%s': %s\n", out_path, kdna_status_str(rc));
        free(words);
        free_kreg(&kr);
        free_ksoa(&kf);
        return KDNA_LIBRARY_RUNTIME;
    }

    printf("kdna_library: wrote %s words=%" PRIu64 " source_n=%" PRIu64
           " source_segments=%" PRIu64 " record_bytes=%u payload_bytes=%" PRIu64 "\n",
           out_path,
           h.word_count,
           h.source_n,
           h.source_segment_count,
           (unsigned)KDNA_KLIB_RECORD_BYTES,
           h.payload_bytes);

    free(words);
    free_kreg(&kr);
    free_ksoa(&kf);
    return KDNA_LIBRARY_OK;
}
