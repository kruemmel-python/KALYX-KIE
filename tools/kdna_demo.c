#include "kdna.h"

#include <stdio.h>
#include <stdlib.h>

static const char *op_name(int v) {
    switch (v) {
        case 1: return "K01";
        case 2: return "K02";
        case 3: return "K03";
        case 4: return "K04";
        case 5: return "K05";
        default: return "K??";
    }
}

static void print_one(const double *out, size_t n, size_t i) {
    int raw = (int)out[kdna_idx(KDNA_RAW, n, i)];
    int dom = (int)out[kdna_idx(KDNA_DOM, n, i)];
    printf("x:% .9f ", out[kdna_idx(KDNA_X, n, i)]);
    printf("K01:% .6f K02:% .6f K03:% .6f K04:% .6f K05:% .6f ",
        out[kdna_idx(KDNA_K01, n, i)], out[kdna_idx(KDNA_K02, n, i)],
        out[kdna_idx(KDNA_K03, n, i)], out[kdna_idx(KDNA_K04, n, i)],
        out[kdna_idx(KDNA_K05, n, i)]);
    printf("| E:% .6f A:% .6f L:% .6f ",
        out[kdna_idx(KDNA_EK, n, i)], out[kdna_idx(KDNA_AK, n, i)],
        out[kdna_idx(KDNA_LK, n, i)]);
    printf("| D:%s RAW:%s ", op_name(dom), op_name(raw));
    printf("| ACT:[K01:% .3f K02:% .3f K03:% .3f K04:% .3f K05:% .3f] score:% .3f\n",
        out[kdna_idx(KDNA_S01, n, i)], out[kdna_idx(KDNA_S02, n, i)],
        out[kdna_idx(KDNA_S03, n, i)], out[kdna_idx(KDNA_S04, n, i)],
        out[kdna_idx(KDNA_S05, n, i)], out[kdna_idx(KDNA_DOM_SCORE, n, i)]);
}

int main(int argc, char **argv) {
    const int use_opencl = argc > 1;
    const char *kernel_path = use_opencl ? argv[1] : "kernels/kdna_eval.cl";

    const size_t n = 9u;
    double x[9] = { -3.0, -1.0, -0.0, 0.0, 1.0e-14, 0.25, 0.75, 1.0, 3.0 };
    double *out = (double *)calloc(KDNA_FIELDS * n, sizeof(double));
    if (!out) return 2;

    kdna_constants c;
    kdna_default_constants(&c);

    int rc;
    if (use_opencl) {
        rc = kdna_eval_opencl(x, out, n, &c, kernel_path);
        if (rc != KDNA_OK) {
            fprintf(stderr, "OpenCL failed: %s; falling back to CPU\n", kdna_status_str(rc));
            rc = kdna_eval_cpu(x, out, n, &c);
        }
    } else {
        rc = kdna_eval_cpu(x, out, n, &c);
    }

    if (rc != KDNA_OK) {
        fprintf(stderr, "eval failed: %s\n", kdna_status_str(rc));
        free(out);
        return 2;
    }

    for (size_t i = 0u; i < n; ++i) print_one(out, n, i);
    free(out);
    return 0;
}
