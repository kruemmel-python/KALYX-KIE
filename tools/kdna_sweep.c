#include "kdna_kexp.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t KEXP_V2_KINDS[] = {
    KDNA_KEXP_PRNG,
    KDNA_KEXP_LOGISTIC,
    KDNA_KEXP_MARKOV,
    KDNA_KEXP_HIDDEN_GRAMMAR,
    KDNA_KEXP_BROWNIAN,
    KDNA_KEXP_QUASI_PERIODIC
};

static void usage(FILE *f) {
    fprintf(f,
        "kdna_sweep --out-report suite.krep [--out-csv suite.csv] [--out-md suite.md]\n"
        "           [--n N] [--seed S] [--seeds COUNT] [--train F] [--bins B]\n"
        "\n"
        "KEXP v2 multi-seed experiment sweep. It runs all supported process classes\n"
        "through the KDNA variant grammar metric and writes one multi-record KREP.\n");
}

static int write_csv(const char *path, const kdna_kexp_result_record *records, size_t count) {
    FILE *f = fopen(path, "w");
    if (!f) return KDNA_EIO;
    fprintf(f, "kind,seed,n,train_n,variants,edges,entropy_raw,entropy_variant,compression,baseline,kgram,lift,surprise,out_of_grammar,mean_lock,mean_dom_score,max_dom_score,top_variant,top_edge_from,top_edge_to\n");
    for (size_t i = 0; i < count; ++i) {
        const kdna_kexp_result_record *r = &records[i];
        fprintf(f, "%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%" PRIu64 ",%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%" PRIu64 ",%.17g,%.17g,%.17g,%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
                kdna_kexp_kind_name(r->kind), r->seed, r->n, r->train_n, r->unique_variants,
                r->grammar_edges, r->entropy_raw, r->entropy_variant, r->compression_ratio,
                r->baseline_accuracy, r->kgram_accuracy, r->kgram_lift, r->surprise_rate,
                r->out_of_grammar, r->mean_lock, r->mean_dom_score, r->max_dom_score,
                r->top_variant_id, r->top_edge_from, r->top_edge_to);
    }
    return fclose(f) == 0 ? KDNA_OK : KDNA_EIO;
}

static int write_md(const char *path, const kdna_kexp_result_record *records, size_t count) {
    FILE *f = fopen(path, "w");
    if (!f) return KDNA_EIO;
    fprintf(f, "# KEXP v2 Sweep Report\n\n");
    fprintf(f, "This report tests whether KDNA variant grammars expose transition structure below probability-level event frequencies. Positive lift is not a physical proof of SUBQG; it is an operational signal that the KDNA/KGRAM projection captured transition structure beyond the baseline.\n\n");
    fprintf(f, "| kind | runs | mean lift | positive lift rate | mean kgram | mean baseline | mean compression | mean surprise |\n");
    fprintf(f, "|---|---:|---:|---:|---:|---:|---:|---:|\n");
    for (size_t k = 0; k < sizeof(KEXP_V2_KINDS)/sizeof(KEXP_V2_KINDS[0]); ++k) {
        const uint32_t kind = KEXP_V2_KINDS[k];
        double lift=0.0, kg=0.0, bl=0.0, comp=0.0, surprise=0.0, pos=0.0;
        size_t n=0;
        for (size_t i=0;i<count;++i) if (records[i].kind == kind) {
            lift += records[i].kgram_lift;
            kg += records[i].kgram_accuracy;
            bl += records[i].baseline_accuracy;
            comp += records[i].compression_ratio;
            surprise += records[i].surprise_rate;
            if (records[i].kgram_lift > 0.0) pos += 1.0;
            n++;
        }
        if (n) {
            fprintf(f, "| %s | %zu | %.6f | %.3f | %.6f | %.6f | %.3f | %.6f |\n",
                    kdna_kexp_kind_name(kind), n, lift/(double)n, pos/(double)n,
                    kg/(double)n, bl/(double)n, comp/(double)n, surprise/(double)n);
        }
    }
    fprintf(f, "\n## Per-run records\n\n");
    fprintf(f, "| kind | seed | variants | edges | baseline | kgram | lift | surprise |\n");
    fprintf(f, "|---|---:|---:|---:|---:|---:|---:|---:|\n");
    for (size_t i=0;i<count;++i) {
        const kdna_kexp_result_record *r=&records[i];
        fprintf(f, "| %s | %" PRIu64 " | %u | %" PRIu64 " | %.6f | %.6f | %.6f | %.6f |\n",
                kdna_kexp_kind_name(r->kind), r->seed, r->unique_variants, r->grammar_edges,
                r->baseline_accuracy, r->kgram_accuracy, r->kgram_lift, r->surprise_rate);
    }
    return fclose(f) == 0 ? KDNA_OK : KDNA_EIO;
}

int main(int argc, char **argv) {
    const char *out_report = NULL;
    const char *out_csv = NULL;
    const char *out_md = NULL;
    size_t n = 4096u;
    uint64_t seed = 0x4b455850ULL;
    uint32_t seed_count = 8u;
    double train = 0.70;
    uint32_t bins = 32u;

    for (int i=1;i<argc;++i) {
        if (strcmp(argv[i], "--out-report") == 0 && i+1<argc) out_report = argv[++i];
        else if (strcmp(argv[i], "--out-csv") == 0 && i+1<argc) out_csv = argv[++i];
        else if (strcmp(argv[i], "--out-md") == 0 && i+1<argc) out_md = argv[++i];
        else if (strcmp(argv[i], "--n") == 0 && i+1<argc) n = (size_t)strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--seed") == 0 && i+1<argc) seed = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--seeds") == 0 && i+1<argc) seed_count = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--train") == 0 && i+1<argc) train = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--bins") == 0 && i+1<argc) bins = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--help") == 0) { usage(stdout); return 0; }
        else { usage(stderr); return 2; }
    }
    if (!out_report || n < 64u || seed_count == 0u) { usage(stderr); return 2; }

    const size_t kind_count = sizeof(KEXP_V2_KINDS)/sizeof(KEXP_V2_KINDS[0]);
    const size_t total = kind_count * (size_t)seed_count;
    kdna_kexp_result_record *records = (kdna_kexp_result_record *)calloc(total, sizeof(*records));
    double *x = (double *)calloc(n, sizeof(double));
    if (!records || !x) { free(records); free(x); return 2; }

    kdna_constants c; kdna_default_constants(&c);
    size_t ri = 0u;
    printf("KEXP v2 sweep n=%zu seeds=%u base_seed=%" PRIu64 " train=%.3f bins=%u\n", n, seed_count, seed, train, bins);
    for (uint32_t si=0; si<seed_count; ++si) {
        for (size_t ki=0; ki<kind_count; ++ki) {
            const uint32_t kind = KEXP_V2_KINDS[ki];
            const uint64_t run_seed = seed + (uint64_t)si * 1000003ULL + (uint64_t)ki * 1009ULL;
            int rc = kdna_kexp_generate(kind, n, run_seed, x);
            if (rc != KDNA_OK) { fprintf(stderr, "generate failed kind=%s\n", kdna_kexp_kind_name(kind)); free(records); free(x); return 2; }
            rc = kdna_kexp_analyze(x, n, kind, run_seed, train, bins, &c, &records[ri]);
            if (rc != KDNA_OK) { fprintf(stderr, "analyze failed kind=%s\n", kdna_kexp_kind_name(kind)); free(records); free(x); return 2; }
            records[ri].id = (uint64_t)ri + 1u;
            printf("%-15s seed:%" PRIu64 " variants:%u edges:%" PRIu64 " baseline:%.6f kgram:%.6f lift:%+.6f surprise:%.6f\n",
                   kdna_kexp_kind_name(kind), run_seed, records[ri].unique_variants, records[ri].grammar_edges,
                   records[ri].baseline_accuracy, records[ri].kgram_accuracy, records[ri].kgram_lift, records[ri].surprise_rate);
            ri++;
        }
    }

    kdna_krep_header h;
    int rc = kdna_krep_init_header(&h, (uint64_t)total);
    if (rc == KDNA_OK) rc = kdna_krep_write_file(out_report, &h, records);
    if (rc != KDNA_OK) { fprintf(stderr, "write KREP failed: %s\n", kdna_status_str(rc)); free(records); free(x); return 2; }
    if (out_csv && (rc = write_csv(out_csv, records, total)) != KDNA_OK) { fprintf(stderr, "write CSV failed\n"); free(records); free(x); return 2; }
    if (out_md && (rc = write_md(out_md, records, total)) != KDNA_OK) { fprintf(stderr, "write Markdown failed\n"); free(records); free(x); return 2; }

    printf("KEXP v2 wrote %s records=%zu\n", out_report, total);
    if (out_csv) printf("KEXP v2 wrote %s\n", out_csv);
    if (out_md) printf("KEXP v2 wrote %s\n", out_md);
    free(records);
    free(x);
    return 0;
}
