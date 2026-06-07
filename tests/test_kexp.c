#include "kdna_kexp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_kexp <data.kdat> <report.krep>\n");
        return 2;
    }

    kdna_kdat_header dh;
    double *x = NULL;
    int rc = kdna_kdat_read_file(argv[1], &dh, &x);
    if (rc != KDNA_OK) {
        fprintf(stderr, "KDAT read failed: %s\n", kdna_status_str(rc));
        return 1;
    }
    if (dh.n < 128u || dh.kind == 0u || dh.payload_bytes != dh.n * sizeof(double)) {
        fprintf(stderr, "bad KDAT header\n");
        free(x);
        return 1;
    }
    for (size_t i = 0u; i < (size_t)dh.n; ++i) {
        if (!isfinite(x[i])) {
            fprintf(stderr, "nonfinite KDAT value\n");
            free(x);
            return 1;
        }
    }
    free(x);

    kdna_krep_header rh;
    kdna_kexp_result_record *records = NULL;
    rc = kdna_krep_read_file(argv[2], &rh, &records);
    if (rc != KDNA_OK) {
        fprintf(stderr, "KREP read failed: %s\n", kdna_status_str(rc));
        return 1;
    }
    if (rh.experiment_count != 1u || rh.payload_bytes != sizeof(kdna_kexp_result_record)) {
        fprintf(stderr, "bad KREP header\n");
        free(records);
        return 1;
    }
    const kdna_kexp_result_record *r = &records[0];
    if (r->n != dh.n || r->kind != dh.kind || r->unique_variants == 0u || r->grammar_edges == 0u) {
        fprintf(stderr, "bad KEXP metrics\n");
        free(records);
        return 1;
    }
    if (!(r->baseline_accuracy >= 0.0 && r->baseline_accuracy <= 1.0 &&
          r->kgram_accuracy >= 0.0 && r->kgram_accuracy <= 1.0 &&
          r->surprise_rate >= 0.0 && r->surprise_rate <= 1.0)) {
        fprintf(stderr, "probability metrics out of range\n");
        free(records);
        return 1;
    }
    if (r->kind == KDNA_KEXP_HIDDEN_GRAMMAR && r->kgram_accuracy + 1.0e-12 < r->baseline_accuracy) {
        fprintf(stderr, "hidden grammar should not underperform baseline\n");
        free(records);
        return 1;
    }
    printf("KEXP ok kind=%s n=%llu variants=%u baseline=%.9f kgram=%.9f lift=%+.9f\n",
           kdna_kexp_kind_name(r->kind), (unsigned long long)r->n, r->unique_variants,
           r->baseline_accuracy, r->kgram_accuracy, r->kgram_lift);
    free(records);
    return 0;
}
