#include "kdna.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static double max_abs_diff(const double *a, const double *b, size_t n) {
    double m = 0.0;
    for (size_t i = 0u; i < n; ++i) {
        double d = fabs(a[i] - b[i]);
        if (d > m) m = d;
    }
    return m;
}

int main(int argc, char **argv) {
    const char *kernel_path = argc > 1 ? argv[1] : "kernels/kdna_eval.cl";
    const size_t n = 4096u;

    double *x = (double *)calloc(n, sizeof(double));
    double *cpu = (double *)calloc(KDNA_FIELDS * n, sizeof(double));
    double *gpu = (double *)calloc(KDNA_FIELDS * n, sizeof(double));
    if (!x || !cpu || !gpu) return 2;

    for (size_t i = 0u; i < n; ++i) {
        double u = ((double)i / (double)(n - 1u));
        x[i] = -8.0 + 16.0 * u;
    }
    x[0] = -0.0;
    x[1] = 0.0;
    x[2] = 1.0e-14;

    kdna_constants c;
    kdna_default_constants(&c);

    int rc = kdna_eval_cpu(x, cpu, n, &c);
    if (rc != KDNA_OK) {
        fprintf(stderr, "cpu failed: %s\n", kdna_status_str(rc));
        return 2;
    }

    rc = kdna_eval_opencl(x, gpu, n, &c, kernel_path);
    if (rc == KDNA_EOPENCL || rc == KDNA_ENO_DEVICE || rc == KDNA_EBUILD) {
        printf("SKIP OpenCL unavailable or fp64 unsupported: %s\n", kdna_status_str(rc));
        free(x); free(cpu); free(gpu);
        return 0;
    }
    if (rc != KDNA_OK) {
        fprintf(stderr, "opencl failed: %s\n", kdna_status_str(rc));
        return 2;
    }

    double d = max_abs_diff(cpu, gpu, KDNA_FIELDS * n);
    printf("max_abs_diff=%0.17g\n", d);
    if (d > 5.0e-9) {
        fprintf(stderr, "DIFF FAILED\n");
        return 1;
    }

    free(x); free(cpu); free(gpu);
    return 0;
}
