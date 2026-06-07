#include "kdna.h"
#include "kdna_kgram.h"
#include "kdna_kgenome.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct entry {
    char name[32];
    const char *symbols_path;
    const char *grammar_path;
    uint64_t n;
} entry;

typedef struct edge {
    uint64_t from;
    uint64_t to;
} edge;

typedef struct count_pair {
    uint64_t value;
    uint64_t count;
} count_pair;

typedef struct grammar_cache {
    edge *edges;
    size_t edge_count;
} grammar_cache;

static void usage(void) {
    fprintf(stderr,
        "kdna_genome_matrix --entry NAME SYMBOLS.u64 GRAMMAR.kgram [--entry ...]\n"
        "                   --out matrix.kgenome --csv matrix.csv [--train 0.70] [--bins 32]\n"
        "\n"
        "Builds a KGENOME0001 cross-chromosome KDNA/KGRAM matrix.\n"
        "Each row is a KDNA-projected uint64 variant stream, each column is a KGRAM01 grammar.\n"
        "The tool performs no prediction and no grammar learning; it only correlates row transitions\n"
        "against existing column grammars and writes entropy_raw, baseline_accuracy, kgram_accuracy,\n"
        "lift and out_of_grammar/surprise_rate.\n");
}

static int cmp_u64(const void *pa, const void *pb) {
    const uint64_t a = *(const uint64_t *)pa;
    const uint64_t b = *(const uint64_t *)pb;
    return (a > b) - (a < b);
}

static int cmp_edge(const void *pa, const void *pb) {
    const edge *a = (const edge *)pa;
    const edge *b = (const edge *)pb;
    if (a->from != b->from) return (a->from > b->from) - (a->from < b->from);
    return (a->to > b->to) - (a->to < b->to);
}

static int edge_exists(const edge *edges, size_t n, uint64_t from, uint64_t to) {
    size_t lo = 0u, hi = n;
    while (lo < hi) {
        const size_t mid = lo + ((hi - lo) >> 1);
        const edge *e = &edges[mid];
        if (e->from < from || (e->from == from && e->to < to)) lo = mid + 1u;
        else hi = mid;
    }
    return (lo < n && edges[lo].from == from && edges[lo].to == to);
}

static int read_symbols(const char *path, uint64_t n, uint64_t **out) {
    if (!path || !out || n < 8u) return KDNA_EINVAL;
    *out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    uint64_t *s = (uint64_t *)malloc((size_t)n * sizeof(uint64_t));
    if (!s) { fclose(f); return KDNA_ENOMEM; }
    if (fread(s, sizeof(uint64_t), (size_t)n, f) != (size_t)n) {
        free(s); fclose(f); return KDNA_EIO;
    }
    fclose(f);
    *out = s;
    return KDNA_OK;
}

static uint64_t infer_symbol_n(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0u;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0u; }
    long sz = ftell(f);
    fclose(f);
    if (sz <= 0 || (sz % 8) != 0) return 0u;
    return (uint64_t)((uint64_t)sz / 8u);
}

static int read_grammar_edges(const char *path, grammar_cache *g) {
    if (!path || !g) return KDNA_EINVAL;
    memset(g, 0, sizeof(*g));
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    kdna_kgram_header h;
    if (fread(&h, 1u, sizeof(h), f) != sizeof(h)) { fclose(f); return KDNA_EIO; }
    int rc = kdna_kgram_validate_header(&h);
    if (rc != KDNA_OK) { fclose(f); return rc; }
    kdna_krule_record *rules = NULL;
    if (h.rule_count) {
        rules = (kdna_krule_record *)malloc((size_t)h.payload_bytes);
        if (!rules) { fclose(f); return KDNA_ENOMEM; }
        if (fread(rules, 1u, (size_t)h.payload_bytes, f) != (size_t)h.payload_bytes) {
            free(rules); fclose(f); return KDNA_EIO;
        }
    }
    fclose(f);
    edge *edges = NULL;
    if (h.rule_count) {
        edges = (edge *)calloc((size_t)h.rule_count, sizeof(edge));
        if (!edges) { free(rules); return KDNA_ENOMEM; }
        for (size_t i = 0u; i < (size_t)h.rule_count; ++i) {
            edges[i].from = rules[i].from_id;
            edges[i].to = rules[i].to_id;
        }
        qsort(edges, (size_t)h.rule_count, sizeof(edge), cmp_edge);
    }
    free(rules);
    g->edges = edges;
    g->edge_count = (size_t)h.rule_count;
    return KDNA_OK;
}

static int compute_symbol_stats(const uint64_t *symbols,
                                size_t n,
                                size_t train_n,
                                size_t *unique_out,
                                uint64_t *baseline_out,
                                double *entropy_out,
                                double *compression_out) {
    if (!symbols || n == 0u || !unique_out || !baseline_out || !entropy_out || !compression_out) return KDNA_EINVAL;
    uint64_t *copy = (uint64_t *)malloc(n * sizeof(uint64_t));
    if (!copy) return KDNA_ENOMEM;
    memcpy(copy, symbols, n * sizeof(uint64_t));
    qsort(copy, n, sizeof(uint64_t), cmp_u64);

    size_t unique = 0u;
    double h = 0.0;
    for (size_t i = 0u; i < n;) {
        size_t j = i + 1u;
        while (j < n && copy[j] == copy[i]) ++j;
        const double p = (double)(j - i) / (double)n;
        if (p > 0.0) h -= p * (log(p) / log(2.0));
        ++unique;
        i = j;
    }
    free(copy);

    if (train_n < 2u) return KDNA_EINVAL;
    const size_t targets = train_n - 1u;
    copy = (uint64_t *)malloc(targets * sizeof(uint64_t));
    if (!copy) return KDNA_ENOMEM;
    memcpy(copy, symbols + 1u, targets * sizeof(uint64_t));
    qsort(copy, targets, sizeof(uint64_t), cmp_u64);

    uint64_t best = copy[0];
    size_t best_count = 0u;
    for (size_t i = 0u; i < targets;) {
        size_t j = i + 1u;
        while (j < targets && copy[j] == copy[i]) ++j;
        if ((j - i) > best_count) { best = copy[i]; best_count = j - i; }
        i = j;
    }
    free(copy);

    *unique_out = unique;
    *baseline_out = best;
    *entropy_out = h;
    *compression_out = unique ? (double)n / (double)unique : 0.0;
    return KDNA_OK;
}

static void copy_name(char dst[32], const char *src) {
    memset(dst, 0, 32u);
    if (!src) return;
    strncpy(dst, src, 31u);
}

static int write_kgenome(const char *path, const kdna_kgenome_record *records, size_t count, size_t source_count, double train, uint32_t bins) {
    if (!path || !records) return KDNA_EINVAL;
    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;
    kdna_kgenome_header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, KDNA_KGENOME_MAGIC, 8u);
    h.version = KDNA_KGENOME_VERSION;
    h.header_bytes = KDNA_KGENOME_HEADER_BYTES;
    h.record_bytes = KDNA_KGENOME_RECORD_BYTES;
    h.source_count = (uint64_t)source_count;
    h.matrix_count = (uint64_t)count;
    h.payload_bytes = (uint64_t)count * (uint64_t)sizeof(kdna_kgenome_record);
    h.flags = KDNA_KGENOME_FLAG_LE_IEEE754_DOUBLE | KDNA_KGENOME_FLAG_KDNA_VARIANT_STREAM |
              KDNA_KGENOME_FLAG_MATRIX_COMPLETE | KDNA_KGENOME_FLAG_DETERMINISTIC;
    h.train_ratio = train;
    h.bins = bins;
    if (fwrite(&h, 1u, sizeof(h), f) != sizeof(h)) { fclose(f); return KDNA_EIO; }
    if (count && fwrite(records, sizeof(kdna_kgenome_record), count, f) != count) { fclose(f); return KDNA_EIO; }
    if (fclose(f) != 0) return KDNA_EIO;
    return KDNA_OK;
}

static int write_csv(const char *path, const kdna_kgenome_record *r, size_t count) {
    if (!path || !r) return KDNA_EINVAL;
    FILE *f = fopen(path, "w");
    if (!f) return KDNA_EIO;
    fprintf(f, "row,col,n,train_n,unique_variants,grammar_edges,test_transitions,entropy_raw,baseline_accuracy,kgram_accuracy,lift,out_of_grammar,surprise_rate,compression_ratio\n");
    for (size_t i = 0u; i < count; ++i) {
        fprintf(f, "%s,%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.17g,%.17g,%.17g,%.17g,%" PRIu64 ",%.17g,%.17g\n",
                r[i].row_name, r[i].col_name, r[i].n, r[i].train_n, r[i].unique_variants, r[i].grammar_edges,
                r[i].test_transitions, r[i].entropy_raw, r[i].baseline_accuracy, r[i].kgram_accuracy,
                r[i].lift, r[i].out_of_grammar, r[i].surprise_rate, r[i].compression_ratio);
    }
    if (fclose(f) != 0) return KDNA_EIO;
    return KDNA_OK;
}

static int inspect_kgenome(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot read KGENOME '%s': I/O error\n", path); return 2; }
    kdna_kgenome_header h;
    if (fread(&h, 1u, sizeof(h), f) != sizeof(h)) { fclose(f); fprintf(stderr, "bad KGENOME header\n"); return 2; }
    if (memcmp(h.magic, KDNA_KGENOME_MAGIC, 8u) != 0 || h.header_bytes != KDNA_KGENOME_HEADER_BYTES || h.record_bytes != KDNA_KGENOME_RECORD_BYTES) {
        fclose(f); fprintf(stderr, "invalid KGENOME header\n"); return 2;
    }
    printf("file: KGENOME sources:%" PRIu64 " matrix:%" PRIu64 " record_bytes:%u payload:%" PRIu64 " train:%.17g bins:%u\n",
           h.source_count, h.matrix_count, h.record_bytes, h.payload_bytes, h.train_ratio, h.bins);
    kdna_kgenome_record *recs = NULL;
    if (h.matrix_count) {
        recs = (kdna_kgenome_record *)malloc((size_t)h.payload_bytes);
        if (!recs) { fclose(f); return 2; }
        if (fread(recs, 1u, (size_t)h.payload_bytes, f) != (size_t)h.payload_bytes) {
            free(recs); fclose(f); return 2;
        }
    }
    fclose(f);
    for (size_t i = 0u; i < (size_t)h.matrix_count && i < 32u; ++i) {
        printf("cell:%s->%s acc:%.17g lift:%+.17g surprise:%.17g oog:%" PRIu64 " entropy:%.17g edges:%" PRIu64 " unique:%" PRIu64 "\n",
               recs[i].row_name, recs[i].col_name, recs[i].kgram_accuracy, recs[i].lift, recs[i].surprise_rate,
               recs[i].out_of_grammar, recs[i].entropy_raw, recs[i].grammar_edges, recs[i].unique_variants);
    }
    free(recs);
    return 0;
}

int main(int argc, char **argv) {
    entry entries[64];
    size_t entry_count = 0u;
    const char *out_path = NULL;
    const char *csv_path = NULL;
    double train = 0.70;
    uint32_t bins = 32u;

    if (argc == 3 && strcmp(argv[1], "--a") == 0) return inspect_kgenome(argv[2]);
    if (argc < 2) { usage(); return 2; }

    memset(entries, 0, sizeof(entries));
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--entry") == 0 && i + 3 < argc) {
            if (entry_count >= 64u) { fprintf(stderr, "too many entries\n"); return 2; }
            copy_name(entries[entry_count].name, argv[++i]);
            entries[entry_count].symbols_path = argv[++i];
            entries[entry_count].grammar_path = argv[++i];
            entries[entry_count].n = infer_symbol_n(entries[entry_count].symbols_path);
            if (entries[entry_count].n < 8u) {
                fprintf(stderr, "entry %s has invalid symbol stream '%s'\n", entries[entry_count].name, entries[entry_count].symbols_path);
                return 2;
            }
            ++entry_count;
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) csv_path = argv[++i];
        else if (strcmp(argv[i], "--train") == 0 && i + 1 < argc) train = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--bins") == 0 && i + 1 < argc) bins = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (entry_count == 0u || !out_path || !csv_path) { usage(); return 2; }
    if (!(train > 0.05 && train < 0.95)) train = 0.70;
    if (bins < 4u) bins = 32u;

    grammar_cache *grams = (grammar_cache *)calloc(entry_count, sizeof(grammar_cache));
    if (!grams) return 2;
    for (size_t j = 0u; j < entry_count; ++j) {
        int rc = read_grammar_edges(entries[j].grammar_path, &grams[j]);
        if (rc != KDNA_OK) {
            fprintf(stderr, "cannot read grammar '%s': %s\n", entries[j].grammar_path, kdna_status_str(rc));
            for (size_t k = 0u; k < j; ++k) free(grams[k].edges);
            free(grams);
            return 2;
        }
    }

    const size_t matrix_count = entry_count * entry_count;
    kdna_kgenome_record *records = (kdna_kgenome_record *)calloc(matrix_count, sizeof(kdna_kgenome_record));
    if (!records) {
        for (size_t k = 0u; k < entry_count; ++k) free(grams[k].edges);
        free(grams);
        return 2;
    }

    size_t rec_i = 0u;
    for (size_t row = 0u; row < entry_count; ++row) {
        uint64_t *symbols = NULL;
        int rc = read_symbols(entries[row].symbols_path, entries[row].n, &symbols);
        if (rc != KDNA_OK) {
            fprintf(stderr, "cannot read symbols '%s': %s\n", entries[row].symbols_path, kdna_status_str(rc));
            free(records);
            for (size_t k = 0u; k < entry_count; ++k) free(grams[k].edges);
            free(grams);
            return 2;
        }
        const size_t n = (size_t)entries[row].n;
        size_t train_n = (size_t)floor((double)n * train);
        if (train_n < 4u) train_n = 4u;
        if (train_n >= n - 2u) train_n = n - 2u;

        size_t unique = 0u;
        uint64_t baseline = 0u;
        double entropy = 0.0, compression = 0.0;
        rc = compute_symbol_stats(symbols, n, train_n, &unique, &baseline, &entropy, &compression);
        if (rc != KDNA_OK) {
            fprintf(stderr, "symbol stats failed: %s\n", kdna_status_str(rc));
            free(symbols); free(records);
            for (size_t k = 0u; k < entry_count; ++k) free(grams[k].edges);
            free(grams);
            return 2;
        }
        const uint64_t test_trans = (uint64_t)(n - train_n - 1u);
        for (size_t col = 0u; col < entry_count; ++col) {
            uint64_t baseline_ok = 0u, grammar_ok = 0u, oog = 0u;
            for (size_t i = train_n; i + 1u < n; ++i) {
                const uint64_t actual_to = symbols[i + 1u];
                if (actual_to == baseline) baseline_ok++;
                if (edge_exists(grams[col].edges, grams[col].edge_count, symbols[i], actual_to)) grammar_ok++;
                else oog++;
            }
            kdna_kgenome_record *r = &records[rec_i];
            memset(r, 0, sizeof(*r));
            r->id = (uint64_t)(rec_i + 1u);
            r->row_index = (uint64_t)row;
            r->col_index = (uint64_t)col;
            r->n = (uint64_t)n;
            r->train_n = (uint64_t)train_n;
            r->unique_variants = (uint64_t)unique;
            r->grammar_edges = (uint64_t)grams[col].edge_count;
            r->test_transitions = test_trans;
            r->out_of_grammar = oog;
            r->entropy_raw = entropy;
            r->baseline_accuracy = test_trans ? (double)baseline_ok / (double)test_trans : 0.0;
            r->kgram_accuracy = test_trans ? (double)grammar_ok / (double)test_trans : 0.0;
            r->lift = r->kgram_accuracy - r->baseline_accuracy;
            r->surprise_rate = test_trans ? (double)oog / (double)test_trans : 0.0;
            r->compression_ratio = compression;
            copy_name(r->row_name, entries[row].name);
            copy_name(r->col_name, entries[col].name);
            ++rec_i;
        }
        free(symbols);
    }

    int rc = write_kgenome(out_path, records, matrix_count, entry_count, train, bins);
    if (rc == KDNA_OK) rc = write_csv(csv_path, records, matrix_count);
    if (rc != KDNA_OK) {
        fprintf(stderr, "cannot write KGENOME/CSV: %s\n", kdna_status_str(rc));
        free(records);
        for (size_t k = 0u; k < entry_count; ++k) free(grams[k].edges);
        free(grams);
        return 2;
    }

    printf("kdna_genome_matrix: wrote %s and %s entries=%zu cells=%zu train=%.17g\n",
           out_path, csv_path, entry_count, matrix_count, train);
    for (size_t i = 0u; i < matrix_count; ++i) {
        printf("  %s -> %s acc:%.17g lift:%+.17g surprise:%.17g oog:%" PRIu64 "\n",
               records[i].row_name, records[i].col_name, records[i].kgram_accuracy,
               records[i].lift, records[i].surprise_rate, records[i].out_of_grammar);
    }

    free(records);
    for (size_t k = 0u; k < entry_count; ++k) free(grams[k].edges);
    free(grams);
    return 0;
}
