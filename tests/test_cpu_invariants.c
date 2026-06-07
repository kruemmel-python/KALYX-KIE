#include "kdna.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int close_rel(double a, double b, double tol) {
    double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
    return fabs(a - b) <= tol * scale;
}

int main(void) {
    kdna_constants c;
    kdna_default_constants(&c);

    const size_t n = 13u;
    double x[13] = { -1000.0, -60.0, -3.0, -1.0, -0.0, 0.0, 1.0e-14, 0.25, 0.75, 1.0, 3.0, 60.0, 1000.0 };
    double *out = (double *)calloc(KDNA_FIELDS * n, sizeof(double));
    if (!out) return 2;

    int rc = kdna_eval_cpu(x, out, n, &c);
    if (rc != KDNA_OK) {
        fprintf(stderr, "cpu eval failed: %s\n", kdna_status_str(rc));
        free(out);
        return 2;
    }

    for (size_t i = 0u; i < n; ++i) {
        double k1 = out[kdna_idx(KDNA_K01, n, i)];
        double k2 = out[kdna_idx(KDNA_K02, n, i)];
        double k3 = out[kdna_idx(KDNA_K03, n, i)];
        double k4 = out[kdna_idx(KDNA_K04, n, i)];
        double k5 = out[kdna_idx(KDNA_K05, n, i)];
        double e = out[kdna_idx(KDNA_EK, n, i)];
        double a = out[kdna_idx(KDNA_AK, n, i)];
        double l = out[kdna_idx(KDNA_LK, n, i)];
        double s1 = out[kdna_idx(KDNA_S01, n, i)];
        double s2 = out[kdna_idx(KDNA_S02, n, i)];
        double s3 = out[kdna_idx(KDNA_S03, n, i)];
        double s4 = out[kdna_idx(KDNA_S04, n, i)];
        double s5 = out[kdna_idx(KDNA_S05, n, i)];
        double score = out[kdna_idx(KDNA_DOM_SCORE, n, i)];
        int raw = (int)out[kdna_idx(KDNA_RAW, n, i)];
        int dom = (int)out[kdna_idx(KDNA_DOM, n, i)];

        double vals[17] = { k1,k2,k3,k4,k5,e,a,l,s1,s2,s3,s4,s5,score,
                            out[kdna_idx(KDNA_X,n,i)], out[kdna_idx(KDNA_RAW,n,i)], out[kdna_idx(KDNA_DOM,n,i)] };
        for (size_t j = 0u; j < 17u; ++j) {
            if (!isfinite(vals[j])) {
                fprintf(stderr, "non-finite at sample=%zu field=%zu\n", i, j);
                free(out);
                return 1;
            }
        }

        double eref = sqrt(k1*k1 + k2*k2 + k3*k3 + k4*k4 + k5*k5);
        double aref = (k1 + 2.0*k2 + 3.0*k3 + 4.0*k4 + 5.0*k5) / 5.0;
        double lref = tanh(eref + fabs(aref));
        if (!close_rel(e, eref, 1.0e-13) || !close_rel(a, aref, 1.0e-13) || !close_rel(l, lref, 1.0e-13)) {
            fprintf(stderr, "derived invariant failed at sample=%zu\n", i);
            free(out);
            return 1;
        }
        if (!(l >= -1.0 && l <= 1.0)) {
            fprintf(stderr, "lock out of representable range at sample=%zu\n", i);
            free(out);
            return 1;
        }

        if (raw < 1 || raw > 5 || dom < 1 || dom > 5) {
            fprintf(stderr, "bad index at sample=%zu raw=%d dom=%d\n", i, raw, dom);
            free(out);
            return 1;
        }

        double max_s = s1;
        if (s2 > max_s) max_s = s2;
        if (s3 > max_s) max_s = s3;
        if (s4 > max_s) max_s = s4;
        if (s5 > max_s) max_s = s5;
        if (!close_rel(score, max_s, 1.0e-13)) {
            fprintf(stderr, "dominance score mismatch at sample=%zu\n", i);
            free(out);
            return 1;
        }
    }

    free(out);
    return 0;
}
