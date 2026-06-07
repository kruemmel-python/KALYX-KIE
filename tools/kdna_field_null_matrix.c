
/*
  kdna_field_null_matrix.c — generic KFIELD true null sampler

  Reads a manifest CSV with columns:
    name,symbols,grammar

  It does NOT rebuild streams, KDNA or KGRAM. It samples existing KDNA uint64
  streams against existing KGRAM01 grammars under destroyed sequence order.

  Output:
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
#define MAX_ENTRY 128

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

typedef struct field_entry {
    char name[64];
    char symbol_path[512];
    char grammar_path[512];
    uint64_t n;
} field_entry;

typedef struct grammar {
    char name[64];
    edge *edges;
    uint64_t edge_count;
} grammar;

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
    sz = ftell(f);
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
    if (!name || !path || !g) return 0;
    memset(g, 0, sizeof(*g));
    snprintf(g->name, sizeof(g->name), "%s", name);
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
    for (uint64_t i = 0; i < g->edge_count; ++i) {
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

static void trim(char *s) {
    size_t n;
    if (!s) return;
    while (*s == ' ' || *s == '\t') memmove(s, s + 1, strlen(s));
    n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r' || s[n-1] == ' ' || s[n-1] == '\t')) s[--n] = 0;
}

static int parse_manifest_line(char *line, field_entry *e) {
    char *a, *b, *c;
    if (!line || !e) return 0;
    trim(line);
    if (!*line || strncmp(line, "name,", 5) == 0) return 0;
    a = strtok(line, ",");
    b = strtok(NULL, ",");
    c = strtok(NULL, ",");
    if (!a || !b || !c) return 0;
    trim(a); trim(b); trim(c);
    snprintf(e->name, sizeof(e->name), "%s", a);
    snprintf(e->symbol_path, sizeof(e->symbol_path), "%s", b);
    snprintf(e->grammar_path, sizeof(e->grammar_path), "%s", c);
    for (size_t i = 0; e->symbol_path[i]; ++i) if (e->symbol_path[i] == '\\') e->symbol_path[i] = '/';
    for (size_t i = 0; e->grammar_path[i]; ++i) if (e->grammar_path[i] == '\\') e->grammar_path[i] = '/';
    return 1;
}

static int read_manifest(const char *path, field_entry *entries, int *count) {
    FILE *f;
    char line[2048];
    int n = 0;
    if (!path || !entries || !count) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        field_entry e;
        memset(&e, 0, sizeof(e));
        if (parse_manifest_line(line, &e)) {
            if (n >= MAX_ENTRY) { fclose(f); return 0; }
            if (!file_size_u64(e.symbol_path, &e.n) || e.n < 8u) { fclose(f); return 0; }
            entries[n++] = e;
        }
    }
    fclose(f);
    *count = n;
    return n > 0;
}

static int compute_mode(FILE *out,
                        const char *mode,
                        const field_entry *row,
                        const uint64_t *symbols,
                        const grammar *grams,
                        int count,
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

    for (int c = 0; c < count; ++c) {
        uint64_t ok = 0u;
        uint64_t name_mix = 0u;
        for (size_t j = 0; row->name[j]; ++j) name_mix = name_mix * 131u + (unsigned char)row->name[j];
        uint64_t rng = seed ^ (name_mix << 1) ^ (uint64_t)c * UINT64_C(0x9e3779b97f4a7c15);
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
        fprintf(out, "%s,%s,%s,%" PRIu64 ",%.17g,%.17g,0x%" PRIx64 ",%.17g,%" PRIu64 ",%" PRIu64 "\n",
                mode, row->name, grams[c].name, samples, acc, 1.0 - acc, seed, train_ratio, block_size, rotation_offset);
        fflush(out);
    }
    return 1;
}

static void usage(void) {
    fprintf(stderr,
        "kdna_field_null_matrix --manifest manifest.csv --out null.csv [options]\n"
        "Options:\n"
        "  --mode all|symbol_shuffle|rotation|block_boundary  default: all\n"
        "  --sample-per-cell N      default: 200000\n"
        "  --train R                default: 0.70\n"
        "  --seed N                 default: 0x4b4649454c444e55\n"
        "  --block-size N           default: 4096\n"
        "  --rotation-offset N      default: 4096\n");
}

int main(int argc, char **argv) {
    const char *manifest_path = NULL;
    const char *out_path = NULL;
    const char *mode = "all";
    uint64_t sample_per_cell = 200000u;
    uint64_t seed = UINT64_C(0x4b4649454c444e55);
    uint64_t block_size = 4096u;
    uint64_t rotation_offset = 4096u;
    double train_ratio = 0.70;
    field_entry entries[MAX_ENTRY];
    grammar grams[MAX_ENTRY];
    int count = 0;
    FILE *out = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--manifest") == 0 && i + 1 < argc) manifest_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) mode = argv[++i];
        else if (strcmp(argv[i], "--sample-per-cell") == 0 && i + 1 < argc) sample_per_cell = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--block-size") == 0 && i + 1 < argc) block_size = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--rotation-offset") == 0 && i + 1 < argc) rotation_offset = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--train") == 0 && i + 1 < argc) train_ratio = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (!manifest_path || !out_path || !(train_ratio > 0.0 && train_ratio < 1.0)) { usage(); return 2; }
    memset(entries, 0, sizeof(entries));
    memset(grams, 0, sizeof(grams));
    if (!read_manifest(manifest_path, entries, &count)) {
        fprintf(stderr, "cannot read manifest %s\n", manifest_path);
        return 2;
    }
    for (int i = 0; i < count; ++i) {
        if (!load_grammar(entries[i].name, entries[i].grammar_path, &grams[i])) {
            fprintf(stderr, "cannot load grammar %s (%s)\n", entries[i].name, entries[i].grammar_path);
            return 2;
        }
    }

    out = fopen(out_path, "w");
    if (!out) { fprintf(stderr, "cannot open output %s\n", out_path); return 2; }
    fprintf(out, "mode,row,col,samples,null_kgram_accuracy,null_surprise,seed,train,block_size,rotation_offset\n");

    for (int r = 0; r < count; ++r) {
        uint64_t *symbols = NULL;
        fprintf(stderr, "kdna_field_null_matrix: row=%s n=%" PRIu64 "\n", entries[r].name, entries[r].n);
        if (!read_symbols(entries[r].symbol_path, entries[r].n, &symbols)) {
            fprintf(stderr, "cannot read symbols for %s\n", entries[r].name);
            fclose(out);
            return 2;
        }
        if (strcmp(mode, "all") == 0 || strcmp(mode, "symbol_shuffle") == 0) {
            if (!compute_mode(out, "symbol_shuffle", &entries[r], symbols, grams, count, train_ratio, sample_per_cell, seed, block_size, rotation_offset)) {
                free(symbols); fclose(out); return 2;
            }
        }
        if (strcmp(mode, "all") == 0 || strcmp(mode, "rotation") == 0) {
            if (!compute_mode(out, "rotation", &entries[r], symbols, grams, count, train_ratio, sample_per_cell, seed ^ UINT64_C(0x52544154), block_size, rotation_offset)) {
                free(symbols); fclose(out); return 2;
            }
        }
        if (strcmp(mode, "all") == 0 || strcmp(mode, "block_boundary") == 0) {
            if (!compute_mode(out, "block_boundary", &entries[r], symbols, grams, count, train_ratio, sample_per_cell, seed ^ UINT64_C(0x424c4f43), block_size, rotation_offset)) {
                free(symbols); fclose(out); return 2;
            }
        }
        free(symbols);
    }

    fclose(out);
    for (int i = 0; i < count; ++i) free(grams[i].edges);
    printf("kdna_field_null_matrix: wrote %s entries=%d modes=%s sample_per_cell=%" PRIu64 " train=%.17g\n",
           out_path, count, mode, sample_per_cell, train_ratio);
    return 0;
}
