#include "kdna_kexp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int file_exists_nonempty(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long n = ftell(f);
    fclose(f);
    return n > 0;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: test_kexp_sweep <suite.krep> <suite.csv> <suite.md> <expected_records>\n");
        return 2;
    }
    const char *report = argv[1];
    const char *csv = argv[2];
    const char *md = argv[3];
    const unsigned long long expected = strtoull(argv[4], NULL, 10);

    kdna_krep_header h;
    kdna_kexp_result_record *records = NULL;
    int rc = kdna_krep_read_file(report, &h, &records);
    if (rc != KDNA_OK) {
        fprintf(stderr, "read suite failed: %s\n", kdna_status_str(rc));
        return 1;
    }
    if (h.experiment_count != expected) {
        fprintf(stderr, "record count mismatch got=%llu expected=%llu\n",
                (unsigned long long)h.experiment_count, expected);
        free(records);
        return 1;
    }
    if (!file_exists_nonempty(csv) || !file_exists_nonempty(md)) {
        fprintf(stderr, "csv/md missing or empty\n");
        free(records);
        return 1;
    }

    unsigned kind_seen[7] = {0,0,0,0,0,0,0};
    double positive_lifts = 0.0;
    for (unsigned long long i = 0; i < expected; ++i) {
        const kdna_kexp_result_record *r = &records[i];
        if (r->kind > 0 && r->kind < 7) kind_seen[r->kind]++;
        if (r->n < 64u || r->train_n == 0u || r->test_transitions == 0u) {
            fprintf(stderr, "bad dimensions at %llu\n", i);
            free(records);
            return 1;
        }
        if (r->unique_variants == 0u || r->grammar_edges == 0u) {
            fprintf(stderr, "empty grammar at %llu\n", i);
            free(records);
            return 1;
        }
        if (r->baseline_accuracy < 0.0 || r->baseline_accuracy > 1.0 ||
            r->kgram_accuracy < 0.0 || r->kgram_accuracy > 1.0 ||
            r->surprise_rate < 0.0 || r->surprise_rate > 1.0) {
            fprintf(stderr, "bad metric range at %llu\n", i);
            free(records);
            return 1;
        }
        if (r->kgram_lift > 0.0) positive_lifts += 1.0;
    }
    for (unsigned k = 1; k <= 6; ++k) {
        if (kind_seen[k] == 0u) {
            fprintf(stderr, "missing kind %u\n", k);
            free(records);
            return 1;
        }
    }
    printf("kexp_sweep_ok records=%llu positive_lift_rate=%.6f\n",
           expected, positive_lifts / (double)expected);
    free(records);
    return 0;
}
