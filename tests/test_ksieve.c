#include "kdna_kexp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: test_ksieve <report.krep>\n");
        return 2;
    }

    kdna_krep_header h;
    kdna_kexp_result_record *r = NULL;
    int rc = kdna_krep_read_file(argv[1], &h, &r);
    if (rc != KDNA_OK) {
        fprintf(stderr, "cannot read KREP: %s\n", kdna_status_str(rc));
        return 1;
    }
    if (h.experiment_count != 1u || h.record_bytes != KDNA_KREP_RECORD_BYTES) {
        fprintf(stderr, "bad KREP header\n");
        free(r);
        return 1;
    }
    const kdna_kexp_result_record *x = &r[0];
    if (x->n < 128u || x->test_transitions == 0u || x->grammar_edges == 0u) {
        fprintf(stderr, "bad sieve dimensions\n");
        free(r);
        return 1;
    }
    if (!isfinite(x->entropy_raw) || !isfinite(x->baseline_accuracy) ||
        !isfinite(x->kgram_accuracy) || !isfinite(x->kgram_lift) ||
        !isfinite(x->surprise_rate)) {
        fprintf(stderr, "nonfinite sieve metric\n");
        free(r);
        return 1;
    }
    if (x->baseline_accuracy < 0.0 || x->baseline_accuracy > 1.0 ||
        x->kgram_accuracy < 0.0 || x->kgram_accuracy > 1.0 ||
        x->surprise_rate < 0.0 || x->surprise_rate > 1.0 ||
        x->out_of_grammar > x->test_transitions) {
        fprintf(stderr, "sieve metric out of range\n");
        free(r);
        return 1;
    }
    const double expected_surprise = (double)x->out_of_grammar / (double)x->test_transitions;
    if (fabs(expected_surprise - x->surprise_rate) > 1e-12) {
        fprintf(stderr, "surprise/out_of_grammar mismatch\n");
        free(r);
        return 1;
    }
    const double expected_lift = x->kgram_accuracy - x->baseline_accuracy;
    if (fabs(expected_lift - x->kgram_lift) > 1e-12) {
        fprintf(stderr, "lift definition mismatch\n");
        free(r);
        return 1;
    }
    printf("K-Sieve ok n=%llu entropy_raw=%.9f kgram=%.9f lift=%+.9f out_of_grammar=%llu\n",
           (unsigned long long)x->n, x->entropy_raw, x->kgram_accuracy, x->kgram_lift,
           (unsigned long long)x->out_of_grammar);
    free(r);
    return 0;
}
