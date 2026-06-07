#include "kdna.h"
#include "kdna_ksoa.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum kdna_inspect_exit {
    KDNA_INSPECT_OK = 0,
    KDNA_INSPECT_USAGE = 2,
    KDNA_INSPECT_RUNTIME = 3,
    KDNA_INSPECT_MISMATCH = 4
};

typedef struct ksoa_file {
    const char *path;
    kdna_ksoa_header h;
    double *payload;
} ksoa_file;

typedef struct field_stats {
    double min_v;
    double max_v;
    double max_abs;
    size_t nonfinite;
} field_stats;

static const char *field_name(uint32_t f) {
    switch (f) {
        case KDNA_X: return "X";
        case KDNA_K01: return "K01";
        case KDNA_K02: return "K02";
        case KDNA_K03: return "K03";
        case KDNA_K04: return "K04";
        case KDNA_K05: return "K05";
        case KDNA_EK: return "E_K";
        case KDNA_AK: return "A_K";
        case KDNA_LK: return "L_K";
        case KDNA_RAW: return "RAW";
        case KDNA_DOM: return "D";
        case KDNA_S01: return "S01";
        case KDNA_S02: return "S02";
        case KDNA_S03: return "S03";
        case KDNA_S04: return "S04";
        case KDNA_S05: return "S05";
        case KDNA_DOM_SCORE: return "dominanceScore";
        default: return "?";
    }
}

static void usage(FILE *f) {
    fprintf(f,
        "kdna_inspect --a <scan.ksoa> [--b <other.ksoa>] [--zero-eps <eps>] [--max-events <n>]\n"
        "\n"
        "Validates KSOA v1 files and extracts substrate diagnostics without re-evaluating K01-K05.\n"
        "\n"
        "Single-file mode:\n"
        "  - validates header/magic/version/layout\n"
        "  - prints field min/max/maxAbs/nonfinite statistics\n"
        "  - extracts RAW and D dominance transitions\n"
        "  - extracts zero-membrane candidates around x=0/sign crossing\n"
        "\n"
        "Two-file mode:\n"
        "  - validates both headers\n"
        "  - checks grid/layout compatibility\n"
        "  - computes max absolute and relative deviation per field\n"
        "  - reports RAW/D encoding mismatches\n"
        "\n"
        "Examples:\n"
        "  kdna_inspect --a scan_cpu.ksoa\n"
        "  kdna_inspect --a scan_cpu.ksoa --b scan_opencl.ksoa --max-events 32\n"
    );
}

static int parse_double_arg(const char *s, double *out) {
    char *end = NULL;
    errno = 0;
    const double v = strtod(s, &end);
    if (errno != 0 || end == s || *end != '\0' || !isfinite(v) || v < 0.0) return 0;
    *out = v;
    return 1;
}

static int parse_size_arg(const char *s, size_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return 0;
#if SIZE_MAX < UINT64_MAX
    if (v > (unsigned long long)SIZE_MAX) return 0;
#endif
    *out = (size_t)v;
    return 1;
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

static void print_header(const ksoa_file *kf) {
    printf("file: %s\n", kf->path);
    printf("  magic: %.8s version:%u header_bytes:%u fields:%u backend:%s(%u)\n",
           kf->h.magic,
           (unsigned)kf->h.version,
           (unsigned)kf->h.header_bytes,
           (unsigned)kf->h.fields,
           kdna_ksoa_backend_name(kf->h.backend),
           (unsigned)kf->h.backend);
    printf("  n:%" PRIu64 " x_min:%.17g x_max:%.17g dx:%.17g payload_bytes:%" PRIu64 " flags:0x%" PRIx64 "\n",
           kf->h.n,
           kf->h.x_min,
           kf->h.x_max,
           kf->h.dx,
           kf->h.payload_bytes,
           kf->h.flags);
}

static field_stats calc_field_stats(const ksoa_file *kf, uint32_t field) {
    const size_t n = (size_t)kf->h.n;
    const double *p = kf->payload + kdna_idx(field, n, 0u);
    field_stats s;
    s.min_v = INFINITY;
    s.max_v = -INFINITY;
    s.max_abs = 0.0;
    s.nonfinite = 0u;

    for (size_t i = 0u; i < n; ++i) {
        const double v = p[i];
        if (!isfinite(v)) {
            ++s.nonfinite;
            continue;
        }
        if (v < s.min_v) s.min_v = v;
        if (v > s.max_v) s.max_v = v;
        const double av = fabs(v);
        if (av > s.max_abs) s.max_abs = av;
    }

    if (s.nonfinite == n) {
        s.min_v = NAN;
        s.max_v = NAN;
        s.max_abs = NAN;
    }
    return s;
}

static void print_stats(const ksoa_file *kf) {
    printf("field_stats:\n");
    for (uint32_t f = 0u; f < kf->h.fields; ++f) {
        const field_stats s = calc_field_stats(kf, f);
        printf("  %-15s min:% .17e max:% .17e maxAbs:% .17e nonfinite:%zu\n",
               field_name(f), s.min_v, s.max_v, s.max_abs, s.nonfinite);
    }
}

static int as_operator_id(double v) {
    const int id = (int)llround(v);
    return (id >= 1 && id <= 5) ? id : 0;
}

static void print_transition_sample(const ksoa_file *kf, uint32_t field, size_t max_events) {
    const size_t n = (size_t)kf->h.n;
    const double *x = kf->payload + kdna_idx(KDNA_X, n, 0u);
    const double *p = kf->payload + kdna_idx(field, n, 0u);

    size_t count = 0u;
    int prev = (n > 0u) ? as_operator_id(p[0]) : 0;

    printf("%s_transitions:\n", field_name(field));
    for (size_t i = 1u; i < n; ++i) {
        const int cur = as_operator_id(p[i]);
        if (cur != prev) {
            ++count;
            if (count <= max_events) {
                printf("  i:%zu x_prev:%.17g x:%.17g %s:K%d->K%d\n",
                       i, x[i - 1u], x[i], field_name(field), prev, cur);
            }
            prev = cur;
        }
    }
    printf("  total:%zu shown:%zu\n", count, (count < max_events) ? count : max_events);
}

static int sign_class(double x) {
    if (x == 0.0) return signbit(x) ? -1 : 1;
    return x < 0.0 ? -1 : 1;
}

static void print_zero_membranes(const ksoa_file *kf, double zero_eps, size_t max_events) {
    const size_t n = (size_t)kf->h.n;
    const double *x = kf->payload + kdna_idx(KDNA_X, n, 0u);
    const double *k01 = kf->payload + kdna_idx(KDNA_K01, n, 0u);
    const double *dom = kf->payload + kdna_idx(KDNA_DOM, n, 0u);
    const double *raw = kf->payload + kdna_idx(KDNA_RAW, n, 0u);

    double eps = zero_eps;
    if (eps == 0.0) {
        eps = fabs(kf->h.dx) * 2.0;
        if (eps < 1e-12) eps = 1e-12;
    }

    size_t count = 0u;
    printf("zero_membrane_candidates eps:%.17g\n", eps);

    for (size_t i = 0u; i < n; ++i) {
        int candidate = fabs(x[i]) <= eps;
        if (!candidate && i > 0u) {
            const int s0 = sign_class(x[i - 1u]);
            const int s1 = sign_class(x[i]);
            candidate = (s0 != s1);
        }

        if (candidate) {
            ++count;
            if (count <= max_events) {
                printf("  i:%zu x:%.17g sign:%s K01:% .17e RAW:K%d D:K%d\n",
                       i,
                       x[i],
                       signbit(x[i]) ? "-" : "+",
                       k01[i],
                       as_operator_id(raw[i]),
                       as_operator_id(dom[i]));
                if (i > 0u && count <= max_events) {
                    printf("    prev i:%zu x:%.17g sign:%s K01:% .17e RAW:K%d D:K%d dK01:% .17e\n",
                           i - 1u,
                           x[i - 1u],
                           signbit(x[i - 1u]) ? "-" : "+",
                           k01[i - 1u],
                           as_operator_id(raw[i - 1u]),
                           as_operator_id(dom[i - 1u]),
                           k01[i] - k01[i - 1u]);
                }
            }
        }
    }

    printf("  total:%zu shown:%zu\n", count, (count < max_events) ? count : max_events);
}

static int compatible_grid(const ksoa_file *a, const ksoa_file *b) {
    if (a->h.fields != b->h.fields) return 0;
    if (a->h.n != b->h.n) return 0;
    if (a->h.payload_bytes != b->h.payload_bytes) return 0;
    if (a->h.header_bytes != b->h.header_bytes) return 0;
    if (a->h.version != b->h.version) return 0;
    if (a->h.x_min != b->h.x_min) return 0;
    if (a->h.x_max != b->h.x_max) return 0;
    if (a->h.dx != b->h.dx) return 0;
    return 1;
}

static void compare_files(const ksoa_file *a, const ksoa_file *b) {
    const size_t n = (size_t)a->h.n;
    double global_abs = 0.0;
    double global_rel = 0.0;
    uint32_t global_field = 0u;
    size_t global_i = 0u;
    size_t raw_mismatch = 0u;
    size_t dom_mismatch = 0u;

    printf("compare:\n");
    printf("  a_backend:%s b_backend:%s n:%zu fields:%u\n",
           kdna_ksoa_backend_name(a->h.backend),
           kdna_ksoa_backend_name(b->h.backend),
           n,
           (unsigned)a->h.fields);
    printf("  per_field_diff:\n");

    for (uint32_t f = 0u; f < a->h.fields; ++f) {
        double max_abs = 0.0;
        double max_rel = 0.0;
        size_t max_i = 0u;
        size_t nonfinite_mismatch = 0u;

        for (size_t i = 0u; i < n; ++i) {
            const double av = a->payload[kdna_idx(f, n, i)];
            const double bv = b->payload[kdna_idx(f, n, i)];

            if (isfinite(av) != isfinite(bv)) {
                ++nonfinite_mismatch;
                continue;
            }
            if (!isfinite(av) && !isfinite(bv)) continue;

            const double d = fabs(av - bv);
            const double scale = fmax(1.0, fmax(fabs(av), fabs(bv)));
            const double r = d / scale;

            if (d > max_abs) {
                max_abs = d;
                max_rel = r;
                max_i = i;
            }
            if (r > global_rel) global_rel = r;
        }

        if (max_abs > global_abs) {
            global_abs = max_abs;
            global_field = f;
            global_i = max_i;
        }

        printf("    %-15s max_abs:% .17e max_rel:% .17e at_i:%zu x:%.17g nonfinite_mismatch:%zu\n",
               field_name(f),
               max_abs,
               max_rel,
               max_i,
               a->payload[kdna_idx(KDNA_X, n, max_i)],
               nonfinite_mismatch);
    }

    for (size_t i = 0u; i < n; ++i) {
        if (as_operator_id(a->payload[kdna_idx(KDNA_RAW, n, i)]) !=
            as_operator_id(b->payload[kdna_idx(KDNA_RAW, n, i)])) {
            ++raw_mismatch;
        }
        if (as_operator_id(a->payload[kdna_idx(KDNA_DOM, n, i)]) !=
            as_operator_id(b->payload[kdna_idx(KDNA_DOM, n, i)])) {
            ++dom_mismatch;
        }
    }

    printf("  global_max_abs:% .17e field:%s index:%zu x:%.17g\n",
           global_abs,
           field_name(global_field),
           global_i,
           a->payload[kdna_idx(KDNA_X, n, global_i)]);
    printf("  global_max_rel:% .17e\n", global_rel);
    printf("  RAW_mismatches:%zu D_mismatches:%zu\n", raw_mismatch, dom_mismatch);
}

int main(int argc, char **argv) {
    const char *a_path = NULL;
    const char *b_path = NULL;
    double zero_eps = 0.0;
    size_t max_events = 64u;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            usage(stdout);
            return KDNA_INSPECT_OK;
        } else if (strcmp(arg, "--a") == 0 && i + 1 < argc) {
            a_path = argv[++i];
        } else if (strcmp(arg, "--b") == 0 && i + 1 < argc) {
            b_path = argv[++i];
        } else if (strcmp(arg, "--zero-eps") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &zero_eps)) {
                fprintf(stderr, "kdna_inspect: invalid --zero-eps\n");
                return KDNA_INSPECT_USAGE;
            }
        } else if (strcmp(arg, "--max-events") == 0 && i + 1 < argc) {
            if (!parse_size_arg(argv[++i], &max_events)) {
                fprintf(stderr, "kdna_inspect: invalid --max-events\n");
                return KDNA_INSPECT_USAGE;
            }
        } else {
            fprintf(stderr, "kdna_inspect: unknown or incomplete argument '%s'\n", arg);
            usage(stderr);
            return KDNA_INSPECT_USAGE;
        }
    }

    if (!a_path) {
        usage(stderr);
        return KDNA_INSPECT_USAGE;
    }

    ksoa_file a;
    int rc = read_ksoa_payload(a_path, &a);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_inspect: cannot read '%s': %s\n", a_path, kdna_status_str(rc));
        return KDNA_INSPECT_RUNTIME;
    }

    print_header(&a);
    print_stats(&a);
    print_transition_sample(&a, KDNA_RAW, max_events);
    print_transition_sample(&a, KDNA_DOM, max_events);
    print_zero_membranes(&a, zero_eps, max_events);

    if (b_path) {
        ksoa_file b;
        rc = read_ksoa_payload(b_path, &b);
        if (rc != KDNA_OK) {
            fprintf(stderr, "kdna_inspect: cannot read '%s': %s\n", b_path, kdna_status_str(rc));
            free_ksoa(&a);
            return KDNA_INSPECT_RUNTIME;
        }

        print_header(&b);

        if (!compatible_grid(&a, &b)) {
            fprintf(stderr, "kdna_inspect: KSOA files are not grid/layout compatible\n");
            free_ksoa(&b);
            free_ksoa(&a);
            return KDNA_INSPECT_MISMATCH;
        }

        compare_files(&a, &b);
        free_ksoa(&b);
    }

    free_ksoa(&a);
    return KDNA_INSPECT_OK;
}
