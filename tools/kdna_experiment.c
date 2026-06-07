#include "kdna_kexp.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE *f) {
    fprintf(f,
        "kdna_experiment --kind prng|logistic|markov|hidden|brownian|quasi --n N --seed S --out-data file.kdat --out-report file.krep [--train 0.70] [--bins 32]\n"
        "kdna_experiment --a report.krep\n"
        "kdna_experiment --data file.kdat\n");
}

static int parse_u64(const char *s, uint64_t *out) {
    if (!s || !out) return 0;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 0);
    if (end == s || *end != '\0') return 0;
    *out = (uint64_t)v;
    return 1;
}

static int parse_size(const char *s, size_t *out) {
    uint64_t v = 0u;
    if (!parse_u64(s, &v) || v == 0u) return 0;
    *out = (size_t)v;
    return 1;
}

static void print_record(const kdna_kexp_result_record *r) {
    printf("KEXP result id:%" PRIu64 " kind:%s n:%" PRIu64 " train:%" PRIu64 " seed:%" PRIu64 "\n",
           r->id, kdna_kexp_kind_name(r->kind), r->n, r->train_n, r->seed);
    printf("  x:[%.17g, %.17g] mean:%.17g variance:%.17g\n", r->x_min, r->x_max, r->x_mean, r->x_variance);
    printf("  raw_bins:%u unique_raw:%u unique_variants:%u grammar_edges:%" PRIu64 "\n",
           r->raw_bins, r->unique_raw_bins, r->unique_variants, r->grammar_edges);
    printf("  entropy_raw:%.17g entropy_variant:%.17g compression_ratio:%.17g\n",
           r->entropy_raw, r->entropy_variant, r->compression_ratio);
    printf("  baseline_accuracy:%.17g kgram_accuracy:%.17g lift:%+.17g surprise_rate:%.17g out_of_grammar:%" PRIu64 "\n",
           r->baseline_accuracy, r->kgram_accuracy, r->kgram_lift, r->surprise_rate, r->out_of_grammar);
    printf("  mean_lock:%.17g mean_dom_score:%.17g max_dom_score:%.17g null_hits:%.17g\n",
           r->mean_lock, r->mean_dom_score, r->max_dom_score, r->null_membrane_hits);
    printf("  top_variant:%" PRIu64 " count:%" PRIu64 " top_edge:%" PRIu64 "->%" PRIu64 " count:%" PRIu64 "\n",
           r->top_variant_id, r->top_variant_count, r->top_edge_from, r->top_edge_to, r->top_edge_count);
}

int main(int argc, char **argv) {
    uint32_t kind = KDNA_KEXP_HIDDEN_GRAMMAR;
    size_t n = 4096u;
    uint64_t seed = 0x4b455850ULL;
    const char *out_data = NULL;
    const char *out_report = NULL;
    const char *report_in = NULL;
    const char *data_in = NULL;
    double train = 0.70;
    uint32_t bins = 32u;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kind") == 0 && i + 1 < argc) {
            if (kdna_kexp_kind_from_name(argv[++i], &kind) != KDNA_OK) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--n") == 0 && i + 1 < argc) {
            if (!parse_size(argv[++i], &n)) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &seed)) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--out-data") == 0 && i + 1 < argc) out_data = argv[++i];
        else if (strcmp(argv[i], "--out-report") == 0 && i + 1 < argc) out_report = argv[++i];
        else if (strcmp(argv[i], "--a") == 0 && i + 1 < argc) report_in = argv[++i];
        else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) data_in = argv[++i];
        else if (strcmp(argv[i], "--train") == 0 && i + 1 < argc) train = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--bins") == 0 && i + 1 < argc) bins = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--help") == 0) { usage(stdout); return 0; }
        else { usage(stderr); return 2; }
    }

    if (report_in) {
        kdna_krep_header h;
        kdna_kexp_result_record *records = NULL;
        int rc = kdna_krep_read_file(report_in, &h, &records);
        if (rc != KDNA_OK) { fprintf(stderr, "cannot read report '%s': %s\n", report_in, kdna_status_str(rc)); return 2; }
        printf("KREP %.8s version:%u experiments:%" PRIu64 " record_bytes:%u payload_bytes:%" PRIu64 "\n",
               h.magic, h.version, h.experiment_count, h.record_bytes, h.payload_bytes);
        for (uint64_t i = 0u; i < h.experiment_count; ++i) print_record(&records[i]);
        free(records);
        return 0;
    }

    if (data_in) {
        kdna_kdat_header h;
        double *x = NULL;
        int rc = kdna_kdat_read_file(data_in, &h, &x);
        if (rc != KDNA_OK) { fprintf(stderr, "cannot read data '%s': %s\n", data_in, kdna_status_str(rc)); return 2; }
        printf("KDAT %.8s version:%u kind:%s n:%" PRIu64 " seed:%" PRIu64 " x:[%.17g,%.17g] mean:%.17g variance:%.17g payload:%" PRIu64 "\n",
               h.magic, h.version, kdna_kexp_kind_name(h.kind), h.n, h.seed, h.x_min, h.x_max, h.mean, h.variance, h.payload_bytes);
        size_t show = h.n < 8u ? (size_t)h.n : 8u;
        printf("samples:");
        for (size_t i = 0; i < show; ++i) printf(" %.9g", x[i]);
        printf("\n");
        free(x);
        return 0;
    }

    if (!out_data || !out_report) { usage(stderr); return 2; }

    double *x = (double *)calloc(n, sizeof(double));
    if (!x) return 2;

    int rc = kdna_kexp_generate(kind, n, seed, x);
    if (rc != KDNA_OK) { fprintf(stderr, "generate failed: %s\n", kdna_status_str(rc)); free(x); return 2; }

    kdna_kdat_header dh;
    rc = kdna_kdat_init_header(&dh, (uint64_t)n, seed, kind, x);
    if (rc == KDNA_OK) rc = kdna_kdat_write_file(out_data, &dh, x);
    if (rc != KDNA_OK) { fprintf(stderr, "write KDAT failed: %s\n", kdna_status_str(rc)); free(x); return 2; }

    kdna_constants c;
    kdna_default_constants(&c);
    kdna_kexp_result_record rec;
    rc = kdna_kexp_analyze(x, n, kind, seed, train, bins, &c, &rec);
    if (rc != KDNA_OK) { fprintf(stderr, "analyze failed: %s\n", kdna_status_str(rc)); free(x); return 2; }

    kdna_krep_header rh;
    rc = kdna_krep_init_header(&rh, 1u);
    if (rc == KDNA_OK) rc = kdna_krep_write_file(out_report, &rh, &rec);
    if (rc != KDNA_OK) { fprintf(stderr, "write KREP failed: %s\n", kdna_status_str(rc)); free(x); return 2; }

    printf("kdna_experiment: wrote data=%s report=%s kind=%s n=%zu seed=%" PRIu64 "\n",
           out_data, out_report, kdna_kexp_kind_name(kind), n, seed);
    print_record(&rec);
    free(x);
    return 0;
}
