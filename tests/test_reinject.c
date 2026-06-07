
#include "kdna.h"
#include "kdna_ksoa.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

static int nearly_equal(double a, double b, double tol) {
    const double d = fabs(a - b);
    const double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
    return d <= tol * scale;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_reinject <file.ksoa> <expected_n>\n");
        return 2;
    }

    const char *path = argv[1];
    const uint64_t expected_n = (uint64_t)strtoull(argv[2], NULL, 10);

    kdna_ksoa_header h;
    int rc = kdna_ksoa_read_header_file(path, &h);
    if (rc != KDNA_OK) {
        fprintf(stderr, "KSOA header invalid: %s\n", kdna_status_str(rc));
        return 1;
    }
    if (h.n != expected_n) {
        fprintf(stderr, "n mismatch got=%llu expected=%llu\n",
                (unsigned long long)h.n, (unsigned long long)expected_n);
        return 1;
    }
    if (h.backend != KDNA_KSOA_BACKEND_CPU) {
        fprintf(stderr, "expected CPU backend, got %u\n", (unsigned)h.backend);
        return 1;
    }
    if (h.dx != 0.0) {
        fprintf(stderr, "expected nonuniform dx=0, got %.17g\n", h.dx);
        return 1;
    }
    if (!isfinite(h.x_min) || !isfinite(h.x_max) || h.x_min > h.x_max) {
        fprintf(stderr, "bad x range\n");
        return 1;
    }

    const size_t n = (size_t)h.n;
    const size_t cells = (size_t)KDNA_FIELDS * n;
    double *payload = (double *)calloc(cells, sizeof(double));
    double *expected = (double *)calloc(cells, sizeof(double));
    double *x = (double *)calloc(n, sizeof(double));
    if (!payload || !expected || !x) {
        free(payload); free(expected); free(x);
        return 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        free(payload); free(expected); free(x);
        return 1;
    }
    if (fseek(f, (long)h.header_bytes, SEEK_SET) != 0 ||
        !read_exact(f, payload, (size_t)h.payload_bytes)) {
        fclose(f);
        free(payload); free(expected); free(x);
        fprintf(stderr, "cannot read payload\n");
        return 1;
    }
    fclose(f);

    double actual_min = payload[kdna_idx(KDNA_X, n, 0u)];
    double actual_max = actual_min;
    for (size_t i = 0u; i < n; ++i) {
        x[i] = payload[kdna_idx(KDNA_X, n, i)];
        if (!isfinite(x[i])) {
            fprintf(stderr, "nonfinite X at %zu\n", i);
            free(payload); free(expected); free(x);
            return 1;
        }
        if (x[i] < actual_min) actual_min = x[i];
        if (x[i] > actual_max) actual_max = x[i];
    }

    if (!nearly_equal(actual_min, h.x_min, 1e-12) || !nearly_equal(actual_max, h.x_max, 1e-12)) {
        fprintf(stderr, "header range does not match X payload\n");
        free(payload); free(expected); free(x);
        return 1;
    }

    kdna_constants c;
    kdna_default_constants(&c);
    rc = kdna_eval_cpu(x, expected, n, &c);
    if (rc != KDNA_OK) {
        fprintf(stderr, "CPU reference failed\n");
        free(payload); free(expected); free(x);
        return 1;
    }

    double max_abs = 0.0;
    for (size_t i = 0u; i < cells; ++i) {
        const double d = fabs(payload[i] - expected[i]);
        if (d > max_abs) max_abs = d;
        if (!nearly_equal(payload[i], expected[i], 1e-12)) {
            fprintf(stderr, "payload mismatch cell=%zu got=%.17g expected=%.17g diff=%.17g\n",
                    i, payload[i], expected[i], d);
            free(payload); free(expected); free(x);
            return 1;
        }
    }

    printf("test_reinject: KSOA trace OK n=%llu max_abs=%.17g\n",
           (unsigned long long)h.n, max_abs);

    free(payload);
    free(expected);
    free(x);
    return 0;
}
