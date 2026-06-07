
#include "kdna.h"
#include "kdna_kgram.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct edge_tmp {
    uint64_t from;
    uint64_t to;
    uint64_t first_index;
    uint64_t count;
} edge_tmp;

static void usage(void) {
    fprintf(stderr,
        "kdna_symbol_gram --symbols input.u64 --n N --out grammar.kgram [--train 0.70] [--top N]\n"
        "  Builds a KGRAM01 from an already KDNA-projected uint64 variant-symbol stream.\n"
        "  No learning beyond observed adjacent transition materialization; no prediction.\n");
}

static int cmp_edge(const void *pa, const void *pb) {
    const edge_tmp *a = (const edge_tmp *)pa;
    const edge_tmp *b = (const edge_tmp *)pb;
    if (a->from != b->from) return (a->from > b->from) - (a->from < b->from);
    return (a->to > b->to) - (a->to < b->to);
}

static int cmp_edge_count_desc(const void *pa, const void *pb) {
    const edge_tmp *a = (const edge_tmp *)pa;
    const edge_tmp *b = (const edge_tmp *)pb;
    if (a->count != b->count) return (a->count < b->count) - (a->count > b->count);
    if (a->from != b->from) return (a->from > b->from) - (a->from < b->from);
    return (a->to > b->to) - (a->to < b->to);
}

static int read_symbols(const char *path, uint64_t n, uint64_t **out) {
    if (!path || !out || n < 2u) return KDNA_EINVAL;
    *out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    uint64_t *s = (uint64_t *)malloc((size_t)n * sizeof(uint64_t));
    if (!s) { fclose(f); return KDNA_ENOMEM; }
    if (fread(s, sizeof(uint64_t), (size_t)n, f) != (size_t)n) {
        free(s);
        fclose(f);
        return KDNA_EIO;
    }
    if (fclose(f) != 0) {
        free(s);
        return KDNA_EIO;
    }
    *out = s;
    return KDNA_OK;
}

static int write_kgram_from_symbols(const char *symbols_path,
                                    uint64_t n,
                                    double train_ratio,
                                    const char *out_path,
                                    uint64_t top_limit) {
    if (!symbols_path || !out_path || n < 3u) return KDNA_EINVAL;
    if (!(train_ratio > 0.0 && train_ratio < 1.0)) return KDNA_EINVAL;

    uint64_t *s = NULL;
    int rc = read_symbols(symbols_path, n, &s);
    if (rc != KDNA_OK) return rc;

    uint64_t train_n = (uint64_t)((double)n * train_ratio);
    if (train_n < 2u) train_n = 2u;
    if (train_n > n) train_n = n;
    const uint64_t transition_n = train_n - 1u;

    edge_tmp *edges = (edge_tmp *)calloc((size_t)transition_n, sizeof(edge_tmp));
    if (!edges) {
        free(s);
        return KDNA_ENOMEM;
    }

    for (uint64_t i = 0u; i < transition_n; ++i) {
        edges[i].from = s[i];
        edges[i].to = s[i + 1u];
        edges[i].first_index = i;
        edges[i].count = 1u;
    }

    qsort(edges, (size_t)transition_n, sizeof(edge_tmp), cmp_edge);

    uint64_t unique_n = 0u;
    for (uint64_t i = 0u; i < transition_n;) {
        edge_tmp e = edges[i];
        uint64_t j = i + 1u;
        while (j < transition_n && edges[j].from == e.from && edges[j].to == e.to) {
            e.count++;
            if (edges[j].first_index < e.first_index) e.first_index = edges[j].first_index;
            ++j;
        }
        edges[unique_n++] = e;
        i = j;
    }

    if (top_limit > 0u && top_limit < unique_n) {
        qsort(edges, (size_t)unique_n, sizeof(edge_tmp), cmp_edge_count_desc);
        unique_n = top_limit;
        qsort(edges, (size_t)unique_n, sizeof(edge_tmp), cmp_edge);
    }

    kdna_kgram_header h;
    rc = kdna_kgram_init_header(&h,
                                (size_t)unique_n,
                                (size_t)unique_n + 1u,
                                (size_t)n,
                                0.0,
                                0.0,
                                0.0);
    if (rc != KDNA_OK) {
        free(edges);
        free(s);
        return rc;
    }

    kdna_krule_record *rules = NULL;
    if (unique_n > 0u) {
        rules = (kdna_krule_record *)calloc((size_t)unique_n, sizeof(kdna_krule_record));
        if (!rules) {
            free(edges);
            free(s);
            return KDNA_ENOMEM;
        }
    }

    for (uint64_t i = 0u; i < unique_n; ++i) {
        kdna_krule_record *r = &rules[i];
        const edge_tmp *e = &edges[i];
        memset(r, 0, sizeof(*r));
        r->id = i + 1u;
        r->sequence_index = i;
        r->from_id = e->from;
        r->to_id = e->to;
        r->from_segment_index = e->first_index;
        r->to_segment_index = e->first_index + 1u;
        r->from_i0 = e->first_index;
        r->from_i1 = e->first_index;
        r->to_i0 = e->first_index + 1u;
        r->to_i1 = e->first_index + 1u;
        r->from_raw = (uint32_t)((e->from % 5u) + 1u);
        r->from_dom = (uint32_t)(((e->from >> 8) % 5u) + 1u);
        r->to_raw = (uint32_t)((e->to % 5u) + 1u);
        r->to_dom = (uint32_t)(((e->to >> 8) % 5u) + 1u);
        r->from_effect = 0u;
        r->to_effect = 0u;
        if (r->from_raw != r->to_raw) r->flags |= KDNA_KRULE_FLAG_RAW_CHANGE;
        if (r->from_dom != r->to_dom) r->flags |= KDNA_KRULE_FLAG_DOM_CHANGE;
        if ((e->from & 0xffu) != (e->to & 0xffu)) r->flags |= KDNA_KRULE_FLAG_SCORE_RISE;
        r->boundary_x = 0.0;
        r->gap_x = 0.0;
        r->delta_lock_mean = 0.0;
        r->delta_score_mean = 0.0;
        r->strength = (double)e->count;
        r->from_lock_mean = 0.0;
        r->to_lock_mean = 0.0;
        r->from_score_mean = 0.0;
        r->to_score_mean = 0.0;
        r->from_width = 1.0;
        r->to_width = 1.0;
    }

    rc = kdna_kgram_write_file(out_path, &h, rules);
    if (rc == KDNA_OK) {
        printf("kdna_symbol_gram: source=%s out=%s n=%" PRIu64 " train_n=%" PRIu64 " transitions=%" PRIu64 " unique_edges=%" PRIu64 " payload_bytes=%" PRIu64 "\n",
               symbols_path, out_path, n, train_n, transition_n, unique_n, h.payload_bytes);
    }

    free(rules);
    free(edges);
    free(s);
    return rc;
}

int main(int argc, char **argv) {
    const char *symbols = NULL;
    const char *out = NULL;
    uint64_t n = 0u;
    uint64_t top = 0u;
    double train = 0.70;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--symbols") == 0 && i + 1 < argc) symbols = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out = argv[++i];
        else if (strcmp(argv[i], "--n") == 0 && i + 1 < argc) n = (uint64_t)strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--train") == 0 && i + 1 < argc) train = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--top") == 0 && i + 1 < argc) top = (uint64_t)strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (!symbols || !out || n < 3u) {
        usage();
        return 2;
    }

    int rc = write_kgram_from_symbols(symbols, n, train, out, top);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_symbol_gram: failed: %s\n", kdna_status_str(rc));
        return 1;
    }
    return 0;
}
