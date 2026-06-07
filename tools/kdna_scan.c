
#include "kdna.h"
#include "kdna_ksoa.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum kdna_scan_exit {
    KDNA_SCAN_EXIT_OK = 0,
    KDNA_SCAN_EXIT_USAGE = 2,
    KDNA_SCAN_EXIT_RUNTIME = 3
};

enum kdna_scan_backend {
    KDNA_SCAN_BACKEND_CPU = KDNA_KSOA_BACKEND_CPU,
    KDNA_SCAN_BACKEND_OPENCL = KDNA_KSOA_BACKEND_OPENCL
};

static void usage(FILE *f) {
    fprintf(f,
        "kdna_scan --min <x0> --max <x1> --n <count> --backend cpu|opencl --out <file> [--kernel <path>]\n"
        "\n"
        "Writes KSOA v1 binary substrate snapshots:\n"
        "  header: 128 bytes, magic KSOA0001, version 1\n"
        "  payload: little-endian IEEE-754 double, plane-major out[field * n + i]\n"
        "  fields: 0 X, 1 K01, 2 K02, 3 K03, 4 K04, 5 K05, 6 E_K, 7 A_K,\n"
        "          8 L_K, 9 RAW, 10 D, 11 S01, 12 S02, 13 S03, 14 S04, 15 S05, 16 dominanceScore\n"
        "\n"
        "Examples:\n"
        "  kdna_scan --min -8 --max 8 --n 1000000 --backend opencl --kernel kernels/kdna_eval.cl --out scan.ksoa\n"
        "  kdna_scan --min -3 --max 3 --n 4096 --backend cpu --out scan_cpu.ksoa\n"
    );
}

static int parse_double_arg(const char *s, double *out) {
    char *end = NULL;
    errno = 0;
    const double v = strtod(s, &end);
    if (errno != 0 || end == s || *end != '\0' || !isfinite(v)) return 0;
    *out = v;
    return 1;
}

static int parse_size_arg(const char *s, size_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v == 0ull) return 0;
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

static const char *scan_backend_name(enum kdna_scan_backend b) {
    return kdna_ksoa_backend_name((uint32_t)b);
}

int main(int argc, char **argv) {
    double x_min = 0.0;
    double x_max = 0.0;
    size_t n = 0u;
    const char *out_path = NULL;
    const char *kernel_path = "kernels/kdna_eval.cl";
    enum kdna_scan_backend backend = KDNA_SCAN_BACKEND_CPU;
    int have_min = 0, have_max = 0, have_backend = 0;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
            usage(stdout);
            return KDNA_SCAN_EXIT_OK;
        } else if (strcmp(a, "--min") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &x_min)) {
                fprintf(stderr, "kdna_scan: invalid --min\n");
                return KDNA_SCAN_EXIT_USAGE;
            }
            have_min = 1;
        } else if (strcmp(a, "--max") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &x_max)) {
                fprintf(stderr, "kdna_scan: invalid --max\n");
                return KDNA_SCAN_EXIT_USAGE;
            }
            have_max = 1;
        } else if (strcmp(a, "--n") == 0 && i + 1 < argc) {
            if (!parse_size_arg(argv[++i], &n)) {
                fprintf(stderr, "kdna_scan: invalid --n\n");
                return KDNA_SCAN_EXIT_USAGE;
            }
        } else if (strcmp(a, "--backend") == 0 && i + 1 < argc) {
            const char *b = argv[++i];
            if (strcmp(b, "cpu") == 0) {
                backend = KDNA_SCAN_BACKEND_CPU;
            } else if (strcmp(b, "opencl") == 0) {
                backend = KDNA_SCAN_BACKEND_OPENCL;
            } else {
                fprintf(stderr, "kdna_scan: invalid --backend '%s'\n", b);
                return KDNA_SCAN_EXIT_USAGE;
            }
            have_backend = 1;
        } else if (strcmp(a, "--out") == 0 && i + 1 < argc) {
            out_path = argv[++i];
        } else if (strcmp(a, "--kernel") == 0 && i + 1 < argc) {
            kernel_path = argv[++i];
        } else {
            fprintf(stderr, "kdna_scan: unknown or incomplete argument '%s'\n", a);
            usage(stderr);
            return KDNA_SCAN_EXIT_USAGE;
        }
    }

    if (!have_min || !have_max || n == 0u || !out_path || !have_backend) {
        usage(stderr);
        return KDNA_SCAN_EXIT_USAGE;
    }

    size_t out_cells = 0u;
    if (!checked_mul_size((size_t)KDNA_FIELDS, n, &out_cells)) {
        fprintf(stderr, "kdna_scan: output size overflow\n");
        return KDNA_SCAN_EXIT_USAGE;
    }

    uint64_t payload_bytes = 0u;
    int rc = kdna_ksoa_payload_bytes(n, &payload_bytes);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_scan: payload size overflow\n");
        return KDNA_SCAN_EXIT_USAGE;
    }

    double *x = (double *)calloc(n, sizeof(double));
    double *out = (double *)calloc(out_cells, sizeof(double));
    if (!x || !out) {
        free(x);
        free(out);
        fprintf(stderr, "kdna_scan: allocation failed for n=%zu (%zu output cells)\n", n, out_cells);
        return KDNA_SCAN_EXIT_RUNTIME;
    }

    const double dx = (n > 1u) ? ((x_max - x_min) / (double)(n - 1u)) : 0.0;
    for (size_t i = 0u; i < n; ++i) {
        x[i] = (n > 1u) ? (x_min + dx * (double)i) : x_min;
    }

    kdna_constants c;
    kdna_default_constants(&c);

    if (backend == KDNA_SCAN_BACKEND_OPENCL) {
        rc = kdna_eval_opencl(x, out, n, &c, kernel_path);
    } else {
        rc = kdna_eval_cpu(x, out, n, &c);
    }

    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_scan: evaluation failed on backend=%s: %s\n",
                scan_backend_name(backend), kdna_status_str(rc));
        free(x);
        free(out);
        return KDNA_SCAN_EXIT_RUNTIME;
    }

    kdna_ksoa_header h;
    rc = kdna_ksoa_init_header(&h, n, x_min, x_max, dx, (uint32_t)backend);
    if (rc == KDNA_OK) {
        rc = kdna_ksoa_write_file(out_path, &h, out);
    }

    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_scan: failed writing KSOA '%s': %s\n", out_path, kdna_status_str(rc));
        free(x);
        free(out);
        return KDNA_SCAN_EXIT_RUNTIME;
    }

    printf("kdna_scan: wrote %s backend=%s n=%zu fields=%u header_bytes=%u payload_bytes=%" PRIu64 " dx=%.17g\n",
           out_path,
           scan_backend_name(backend),
           n,
           (unsigned)KDNA_FIELDS,
           (unsigned)KDNA_KSOA_HEADER_BYTES,
           payload_bytes,
           dx);

    free(x);
    free(out);
    return KDNA_SCAN_EXIT_OK;
}
