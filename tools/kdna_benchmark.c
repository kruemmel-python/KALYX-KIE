#include "kdna_kexp.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
    printf("kdna_benchmark --out-prefix prefix [--n N] [--seed S]\n");
}

static int write_one(const char *prefix, uint32_t kind, size_t n, uint64_t seed, uint32_t idx) {
    char data_path[512];
    char report_path[512];
    snprintf(data_path, sizeof(data_path), "%s_%s.kdat", prefix, kdna_kexp_kind_name(kind));
    snprintf(report_path, sizeof(report_path), "%s_%s.krep", prefix, kdna_kexp_kind_name(kind));

    double *x = (double *)calloc(n, sizeof(double));
    if (!x) return KDNA_ENOMEM;
    int rc = kdna_kexp_generate(kind, n, seed + (uint64_t)idx * 101u, x);
    if (rc != KDNA_OK) { free(x); return rc; }

    kdna_kdat_header dh;
    rc = kdna_kdat_init_header(&dh, (uint64_t)n, seed + (uint64_t)idx * 101u, kind, x);
    if (rc == KDNA_OK) rc = kdna_kdat_write_file(data_path, &dh, x);
    if (rc != KDNA_OK) { free(x); return rc; }

    kdna_constants c; kdna_default_constants(&c);
    kdna_kexp_result_record rec;
    rc = kdna_kexp_analyze(x, n, kind, seed + (uint64_t)idx * 101u, 0.70, 32u, &c, &rec);
    free(x);
    if (rc != KDNA_OK) return rc;

    kdna_krep_header rh;
    rc = kdna_krep_init_header(&rh, 1u);
    if (rc == KDNA_OK) rc = kdna_krep_write_file(report_path, &rh, &rec);
    if (rc != KDNA_OK) return rc;

    printf("%-15s variants:%u edges:%" PRIu64 " baseline:%.6f kgram:%.6f lift:%+.6f compression:%.3f surprise:%.6f\n",
           kdna_kexp_kind_name(kind), rec.unique_variants, rec.grammar_edges,
           rec.baseline_accuracy, rec.kgram_accuracy, rec.kgram_lift,
           rec.compression_ratio, rec.surprise_rate);
    return KDNA_OK;
}

int main(int argc, char **argv) {
    const char *prefix = NULL;
    size_t n = 4096u;
    uint64_t seed = 0x4b455850ULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--out-prefix") == 0 && i + 1 < argc) prefix = argv[++i];
        else if (strcmp(argv[i], "--n") == 0 && i + 1 < argc) n = (size_t)strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }
    if (!prefix || n < 64u) { usage(); return 2; }

    printf("KEXP benchmark n=%zu seed=%" PRIu64 "\n", n, seed);
    int rc = KDNA_OK;
    rc = write_one(prefix, KDNA_KEXP_PRNG, n, seed, 0u); if (rc != KDNA_OK) return 2;
    rc = write_one(prefix, KDNA_KEXP_LOGISTIC, n, seed, 1u); if (rc != KDNA_OK) return 2;
    rc = write_one(prefix, KDNA_KEXP_MARKOV, n, seed, 2u); if (rc != KDNA_OK) return 2;
    rc = write_one(prefix, KDNA_KEXP_HIDDEN_GRAMMAR, n, seed, 3u); if (rc != KDNA_OK) return 2;
    rc = write_one(prefix, KDNA_KEXP_BROWNIAN, n, seed, 4u); if (rc != KDNA_OK) return 2;
    rc = write_one(prefix, KDNA_KEXP_QUASI_PERIODIC, n, seed, 5u); if (rc != KDNA_OK) return 2;
    return 0;
}
