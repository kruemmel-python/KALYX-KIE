
/*
  kdna_null_matrix.c — KGENOME_REPORT v3 true null sampler

  This tool does NOT rebuild FASTA, KMAP, KDNA or KGRAM artifacts.
  It reads existing KDNA uint64 streams and existing KGRAM01 grammars,
  then computes sequence-order null accuracies by destroying adjacency
  in controlled ways.

  Modes:
    symbol_shuffle : from/to are sampled independently from test symbols.
    rotation       : from is sampled from test region, to is sampled at fixed offset.
    block_boundary : from comes from the end of one random block, to from start of another.

  Output CSV:
    mode,row,col,samples,null_kgram_accuracy,null_surprise,seed,train,block_size,rotation_offset
*/

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define KGRAM_MAGIC "KGRAM01"
#define KGRAM_HEADER_BYTES 128u
#define KGRAM_RECORD_BYTES 256u

typedef struct kgram_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t reserved0;
    uint64_t rule_count;
    uint64_t source_word_count;
    uint64_t source_n;
    double x_min;
    double x_max;
    double dx;
    uint64_t payload_bytes;
    uint64_t flags;
    uint8_t reserved[40];
} kgram_header;

typedef struct krule_record {
    uint64_t id;
    uint64_t sequence_index;
    uint64_t from_id;
    uint64_t to_id;
    uint8_t rest[224];
} krule_record;

typedef struct edge {
    uint64_t from;
    uint64_t to;
} edge;

typedef struct grammar {
    char name[16];
    char path[512];
    edge *edges;
    uint64_t edge_count;
} grammar;

typedef struct chrom {
    char name[16];
    char symbol_path[512];
    char grammar_path[512];
    uint64_t n;
} chrom;

static const char *HUMAN24[] = {
    "chr1","chr2","chr3","chr4","chr5","chr6",
    "chr7","chr8","chr9","chr10","chr11","chr12",
    "chr13","chr14","chr15","chr16","chr17","chr18",
    "chr19","chr20","chr21","chr22","chrX","chrY"
};

static uint64_t splitmix64(uint64_t *x) {
    uint64_t z;
    *x += UINT64_C(0x9e3779b97f4a7c15);
    z = *x;
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

static int cmp_edge(const void *pa, const void *pb) {
    const edge *a = (const edge *)pa;
    const edge *b = (const edge *)pb;
    if (a->from != b->from) return (a->from > b->from) - (a->from < b->from);
    return (a->to > b->to) - (a->to < b->to);
}

static int edge_exists(const edge *edges, uint64_t n, uint64_t from, uint64_t to) {
    uint64_t lo = 0, hi = n;
    while (lo < hi) {
        uint64_t mid = lo + ((hi - lo) >> 1);
        const edge *e = &edges[mid];
        if (e->from < from || (e->from == from && e->to < to)) lo = mid + 1;
        else hi = mid;
    }
    return lo < n && edges[lo].from == from && edges[lo].to == to;
}

static int file_size_u64(const char *path, uint64_t *n_out) {
    FILE *f;
    long long sz;
    if (!path || !n_out) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
#if defined(_WIN32)
    sz = _ftelli64(f);
#else
    sz = (long long)ftell(f);
#endif
    fclose(f);
    if (sz < 0 || (sz % 8) != 0) return 0;
    *n_out = (uint64_t)sz / 8u;
    return 1;
}

static int read_symbols(const char *path, uint64_t n, uint64_t **out) {
    FILE *f;
    uint64_t *buf;
    if (!path || !out || n < 8u) return 0;
    *out = NULL;
    f = fopen(path, "rb");
    if (!f) return 0;
    buf = (uint64_t *)malloc((size_t)n * sizeof(uint64_t));
    if (!buf) { fclose(f); return 0; }
    if (fread(buf, sizeof(uint64_t), (size_t)n, f) != (size_t)n) {
        free(buf); fclose(f); return 0;
    }
    fclose(f);
    *out = buf;
    return 1;
}

static int load_grammar(const char *name, const char *path, grammar *g) {
    FILE *f;
    kgram_header h;
    krule_record r;
    uint64_t i;
    if (!name || !path || !g) return 0;
    memset(g, 0, sizeof(*g));
    strncpy(g->name, name, sizeof(g->name)-1);
    strncpy(g->path, path, sizeof(g->path)-1);
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fread(&h, 1, sizeof(h), f) != sizeof(h)) { fclose(f); return 0; }
    if (memcmp(h.magic, KGRAM_MAGIC, 7) != 0 || h.header_bytes != KGRAM_HEADER_BYTES ||
        h.record_bytes != KGRAM_RECORD_BYTES || h.payload_bytes != h.rule_count * (uint64_t)KGRAM_RECORD_BYTES) {
        fclose(f); return 0;
    }
    g->edge_count = h.rule_count;
    g->edges = (edge *)malloc((size_t)g->edge_count * sizeof(edge));
    if (!g->edges) { fclose(f); return 0; }
    for (i = 0; i < g->edge_count; ++i) {
        if (fread(&r, 1, sizeof(r), f) != sizeof(r)) {
            free(g->edges); g->edges = NULL; fclose(f); return 0;
        }
        g->edges[i].from = r.from_id;
        g->edges[i].to = r.to_id;
    }
    fclose(f);
    qsort(g->edges, (size_t)g->edge_count, sizeof(edge), cmp_edge);
    return 1;
}

static void usage(void) {
    fprintf(stderr,
        "kdna_null_matrix --dir GenomeOutFull --human24 --out null.csv [options]\n"
        "Options:\n"
        "  --mode all|symbol_shuffle|rotation|block_boundary  default: all\n"
        "  --sample-per-cell N      default: 200000\n"
        "  --train R                default: 0.70\n"
        "  --seed N                 default: 0x4b444e414e554c4c\n"
        "  --block-size N           default: 4096\n"
        "  --rotation-offset N      default: 4096\n"
        "\n"
        "Reads existing *_k16_kdna.u64 and *_k16_self.kgram. No FASTA/KMAP/KGRAM rebuild.\n");
}

static void join_path(char *dst, size_t cap, const char *dir, const char *name, const char *suffix) {
    size_t n;
    snprintf(dst, cap, "%s/%s%s", dir, name, suffix);
    n = strlen(dst);
    for (size_t i = 0; i < n; ++i) if (dst[i] == '\\') dst[i] = '/';
}

static int compute_mode(FILE *out,
                        const char *mode,
                        const chrom *row,
                        const uint64_t *symbols,
                        const grammar *grams,
                        int chrom_count,
                        double train_ratio,
                        uint64_t sample_per_cell,
                        uint64_t seed,
                        uint64_t block_size,
                        uint64_t rotation_offset) {
    uint64_t train_n = (uint64_t)floor((double)row->n * train_ratio);
    if (train_n < 2u || train_n + 2u >= row->n) return 0;
    uint64_t test_len = row->n - train_n;
    uint64_t test_trans = test_len > 1u ? test_len - 1u : 0u;
    uint64_t samples = sample_per_cell;
    if (samples == 0u || samples > test_trans) samples = test_trans;
    if (samples == 0u) return 0;

    for (int c = 0; c < chrom_count; ++c) {
        uint64_t ok = 0u;
        uint64_t rng = seed ^ ((uint64_t)(row->name[3] ? row->name[3] : row->name[0]) << 32) ^ (uint64_t)c * UINT64_C(0x9e3779b97f4a7c15);
        for (uint64_t s = 0u; s < samples; ++s) {
            uint64_t from = 0u, to = 0u;
            if (strcmp(mode, "symbol_shuffle") == 0) {
                uint64_t a = splitmix64(&rng) % test_len;
                uint64_t b = splitmix64(&rng) % test_len;
                from = symbols[train_n + a];
                to = symbols[train_n + b];
            } else if (strcmp(mode, "rotation") == 0) {
                uint64_t off = rotation_offset % test_len;
                if (off == 0u) off = 1u;
                uint64_t a = splitmix64(&rng) % test_len;
                uint64_t b = (a + off) % test_len;
                from = symbols[train_n + a];
                to = symbols[train_n + b];
            } else if (strcmp(mode, "block_boundary") == 0) {
                uint64_t bs = block_size ? block_size : 4096u;
                uint64_t blocks = test_len / bs;
                if (blocks < 2u) {
                    uint64_t a = splitmix64(&rng) % test_len;
                    uint64_t b = splitmix64(&rng) % test_len;
                    from = symbols[train_n + a];
                    to = symbols[train_n + b];
                } else {
                    uint64_t a = splitmix64(&rng) % blocks;
                    uint64_t b = splitmix64(&rng) % blocks;
                    from = symbols[train_n + a * bs + (bs - 1u)];
                    to = symbols[train_n + b * bs];
                }
            } else {
                return 0;
            }
            if (edge_exists(grams[c].edges, grams[c].edge_count, from, to)) ok++;
        }
        double acc = (double)ok / (double)samples;
        double surprise = 1.0 - acc;
        fprintf(out, "%s,%s,%s,%" PRIu64 ",%.17g,%.17g,0x%" PRIx64 ",%.17g,%" PRIu64 ",%" PRIu64 "\n",
                mode, row->name, grams[c].name, samples, acc, surprise, seed, train_ratio, block_size, rotation_offset);
        fflush(out);
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *dir = NULL;
    const char *out_path = NULL;
    const char *mode = "all";
    uint64_t sample_per_cell = 200000u;
    uint64_t seed = UINT64_C(0x4b444e414e554c4c);
    uint64_t block_size = 4096u;
    uint64_t rotation_offset = 4096u;
    double train_ratio = 0.70;
    int human24 = 0;
    chrom chroms[24];
    grammar grams[24];
    int chrom_count = 24;
    FILE *out = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) dir = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) mode = argv[++i];
        else if (strcmp(argv[i], "--sample-per-cell") == 0 && i + 1 < argc) sample_per_cell = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--block-size") == 0 && i + 1 < argc) block_size = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--rotation-offset") == 0 && i + 1 < argc) rotation_offset = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--train") == 0 && i + 1 < argc) train_ratio = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--human24") == 0) human24 = 1;
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (!dir || !out_path || !human24 || !(train_ratio > 0.0 && train_ratio < 1.0)) {
        usage(); return 2;
    }

    memset(chroms, 0, sizeof(chroms));
    memset(grams, 0, sizeof(grams));
    for (int i = 0; i < chrom_count; ++i) {
        strncpy(chroms[i].name, HUMAN24[i], sizeof(chroms[i].name)-1);
        join_path(chroms[i].symbol_path, sizeof(chroms[i].symbol_path), dir, chroms[i].name, "_k16_kdna.u64");
        join_path(chroms[i].grammar_path, sizeof(chroms[i].grammar_path), dir, chroms[i].name, "_k16_self.kgram");
        if (!file_size_u64(chroms[i].symbol_path, &chroms[i].n) || chroms[i].n < 8u) {
            fprintf(stderr, "cannot infer symbol count for %s (%s)\n", chroms[i].name, chroms[i].symbol_path);
            return 2;
        }
        if (!load_grammar(chroms[i].name, chroms[i].grammar_path, &grams[i])) {
            fprintf(stderr, "cannot load grammar for %s (%s)\n", chroms[i].name, chroms[i].grammar_path);
            return 2;
        }
    }

    out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "cannot open output %s\n", out_path);
        return 2;
    }
    fprintf(out, "mode,row,col,samples,null_kgram_accuracy,null_surprise,seed,train,block_size,rotation_offset\n");

    for (int r = 0; r < chrom_count; ++r) {
        uint64_t *symbols = NULL;
        fprintf(stderr, "kdna_null_matrix: row=%s n=%" PRIu64 "\n", chroms[r].name, chroms[r].n);
        if (!read_symbols(chroms[r].symbol_path, chroms[r].n, &symbols)) {
            fprintf(stderr, "cannot read symbols for %s\n", chroms[r].name);
            fclose(out);
            return 2;
        }
        if (strcmp(mode, "all") == 0 || strcmp(mode, "symbol_shuffle") == 0) {
            if (!compute_mode(out, "symbol_shuffle", &chroms[r], symbols, grams, chrom_count, train_ratio, sample_per_cell, seed, block_size, rotation_offset)) {
                free(symbols); fclose(out); return 2;
            }
        }
        if (strcmp(mode, "all") == 0 || strcmp(mode, "rotation") == 0) {
            if (!compute_mode(out, "rotation", &chroms[r], symbols, grams, chrom_count, train_ratio, sample_per_cell, seed ^ UINT64_C(0x52544154), block_size, rotation_offset)) {
                free(symbols); fclose(out); return 2;
            }
        }
        if (strcmp(mode, "all") == 0 || strcmp(mode, "block_boundary") == 0) {
            if (!compute_mode(out, "block_boundary", &chroms[r], symbols, grams, chrom_count, train_ratio, sample_per_cell, seed ^ UINT64_C(0x424c4f43), block_size, rotation_offset)) {
                free(symbols); fclose(out); return 2;
            }
        }
        free(symbols);
    }

    fclose(out);
    for (int i = 0; i < chrom_count; ++i) free(grams[i].edges);
    printf("kdna_null_matrix: wrote %s modes=%s sample_per_cell=%" PRIu64 " train=%.17g\n", out_path, mode, sample_per_cell, train_ratio);
    return 0;
}
