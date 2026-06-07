#include "kdna.h"
#include "kdna_ksoa.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int nearly_equal(double a, double b, double tol) {
    const double d = fabs(a - b);
    const double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
    return d <= tol * scale;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: test_scan_ksoa <file.ksoa>\n");
        return 2;
    }

    const char *path = argv[1];

    kdna_ksoa_header h;
    int rc = kdna_ksoa_read_header_file(path, &h);
    if (rc != KDNA_OK) {
        fprintf(stderr, "header validation failed: %s\n", kdna_status_str(rc));
        return 1;
    }

    if (h.n != 17u) {
        fprintf(stderr, "unexpected n: %llu\n", (unsigned long long)h.n);
        return 1;
    }
    if (h.backend != KDNA_KSOA_BACKEND_CPU) {
        fprintf(stderr, "unexpected backend: %u\n", (unsigned)h.backend);
        return 1;
    }
    if (!nearly_equal(h.x_min, -1.0, 0.0) ||
        !nearly_equal(h.x_max,  1.0, 0.0) ||
        !nearly_equal(h.dx, 0.125, 0.0)) {
        fprintf(stderr, "unexpected scan grid: min=%.17g max=%.17g dx=%.17g\n", h.x_min, h.x_max, h.dx);
        return 1;
    }

    const size_t n = (size_t)h.n;
    const size_t cells = (size_t)KDNA_FIELDS * n;
    double *payload = (double *)calloc(cells, sizeof(double));
    double *expected = (double *)calloc(cells, sizeof(double));
    double *x = (double *)calloc(n, sizeof(double));
    if (!payload || !expected || !x) {
        free(payload);
        free(expected);
        free(x);
        return 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open payload\n");
        free(payload); free(expected); free(x);
        return 1;
    }
    if (fseek(f, (long)KDNA_KSOA_HEADER_BYTES, SEEK_SET) != 0) {
        fprintf(stderr, "cannot seek payload\n");
        fclose(f); free(payload); free(expected); free(x);
        return 1;
    }
    if (fread(payload, sizeof(double), cells, f) != cells) {
        fprintf(stderr, "cannot read payload\n");
        fclose(f); free(payload); free(expected); free(x);
        return 1;
    }
    fclose(f);

    for (size_t i = 0u; i < n; ++i) {
        x[i] = h.x_min + h.dx * (double)i;
    }

    kdna_constants c;
    kdna_default_constants(&c);
    rc = kdna_eval_cpu(x, expected, n, &c);
    if (rc != KDNA_OK) {
        fprintf(stderr, "cpu reference failed: %s\n", kdna_status_str(rc));
        free(payload); free(expected); free(x);
        return 1;
    }

    double max_abs = 0.0;
    for (size_t i = 0u; i < cells; ++i) {
        const double d = fabs(payload[i] - expected[i]);
        if (d > max_abs) max_abs = d;
        if (!nearly_equal(payload[i], expected[i], 1e-12)) {
            fprintf(stderr, "payload mismatch at cell %zu got=%.17g expected=%.17g diff=%.17g\n",
                    i, payload[i], expected[i], d);
            free(payload); free(expected); free(x);
            return 1;
        }
    }

    if (payload[kdna_idx(KDNA_RAW, n, 0u)] < 1.0 || payload[kdna_idx(KDNA_RAW, n, 0u)] > 5.0) {
        fprintf(stderr, "RAW encoding out of range\n");
        free(payload); free(expected); free(x);
        return 1;
    }
    if (payload[kdna_idx(KDNA_DOM, n, 0u)] < 1.0 || payload[kdna_idx(KDNA_DOM, n, 0u)] > 5.0) {
        fprintf(stderr, "DOM encoding out of range\n");
        free(payload); free(expected); free(x);
        return 1;
    }

    printf("test_scan_ksoa: header+payload OK max_abs=%.17g\n", max_abs);

    free(payload);
    free(expected);
    free(x);
    return 0;
}
