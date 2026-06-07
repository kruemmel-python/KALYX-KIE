#include "kdna_kexp.h"
#include "kdna_ksoa.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double absd(double x) { return x < 0.0 ? -x : x; }

const char *kdna_kexp_kind_name(uint32_t kind) {
    switch (kind) {
        case KDNA_KEXP_PRNG: return "prng";
        case KDNA_KEXP_LOGISTIC: return "logistic";
        case KDNA_KEXP_MARKOV: return "markov";
        case KDNA_KEXP_HIDDEN_GRAMMAR: return "hidden_grammar";
        case KDNA_KEXP_BROWNIAN: return "brownian";
        case KDNA_KEXP_QUASI_PERIODIC: return "quasi_periodic";
        case KDNA_KEXP_KSIEVE: return "k_sieve";
        default: return "unknown";
    }
}

int kdna_kexp_kind_from_name(const char *name, uint32_t *kind_out) {
    if (!name || !kind_out) return KDNA_EINVAL;
    if (strcmp(name, "prng") == 0) *kind_out = KDNA_KEXP_PRNG;
    else if (strcmp(name, "logistic") == 0) *kind_out = KDNA_KEXP_LOGISTIC;
    else if (strcmp(name, "markov") == 0) *kind_out = KDNA_KEXP_MARKOV;
    else if (strcmp(name, "hidden") == 0 || strcmp(name, "hidden_grammar") == 0) *kind_out = KDNA_KEXP_HIDDEN_GRAMMAR;
    else if (strcmp(name, "brownian") == 0 || strcmp(name, "random_walk") == 0) *kind_out = KDNA_KEXP_BROWNIAN;
    else if (strcmp(name, "quasi") == 0 || strcmp(name, "quasi_periodic") == 0) *kind_out = KDNA_KEXP_QUASI_PERIODIC;
    else if (strcmp(name, "ksieve") == 0 || strcmp(name, "k_sieve") == 0) *kind_out = KDNA_KEXP_KSIEVE;
    else return KDNA_EINVAL;
    return KDNA_OK;
}

static uint64_t splitmix64(uint64_t *s) {
    uint64_t z = (*s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static double u01(uint64_t *s) {
    return (double)(splitmix64(s) >> 11) * (1.0 / 9007199254740992.0);
}

static uint64_t hash64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static uint64_t variant_hash_from_k(const double *out, size_t n, size_t i) {
    /*
      KEXP deliberately uses a coarser variant projection than KGEN/KVAR.
      The experiment asks whether a hidden transition grammar becomes visible
      below probability-level event frequencies. Overly fine quantization would
      fragment the same causal state into thousands of singleton variants.
    */
    const uint64_t raw = (uint64_t)(out[kdna_idx(KDNA_RAW, n, i)] + 0.5);
    const uint64_t dom = (uint64_t)(out[kdna_idx(KDNA_DOM, n, i)] + 0.5);
    const int64_t lock_q = (int64_t)floor(out[kdna_idx(KDNA_LK, n, i)] * 32.0);
    const int64_t score_q = (int64_t)floor(log1p(absd(out[kdna_idx(KDNA_DOM_SCORE, n, i)])) * 16.0);
    const int64_t k1 = (int64_t)floor(log1p(absd(out[kdna_idx(KDNA_K01, n, i)])) * 8.0);
    const int64_t k2 = (int64_t)floor(out[kdna_idx(KDNA_K02, n, i)] * 16.0);
    const int64_t k3 = (int64_t)floor(out[kdna_idx(KDNA_K03, n, i)] * 16.0);
    const int64_t k4 = (int64_t)floor(out[kdna_idx(KDNA_K04, n, i)] * 24.0);
    const int64_t k5 = (int64_t)floor(out[kdna_idx(KDNA_K05, n, i)] * 16.0);
    uint64_t h = 0x4b444e415f4b4558ULL;
    h ^= hash64(raw + 0x11ULL);
    h ^= hash64((dom << 8) + 0x22ULL);
    h ^= hash64((uint64_t)lock_q);
    h ^= hash64((uint64_t)score_q);
    h ^= hash64((uint64_t)k1);
    h ^= hash64((uint64_t)k2);
    h ^= hash64((uint64_t)k3);
    h ^= hash64((uint64_t)k4);
    h ^= hash64((uint64_t)k5);
    return h ? h : 1ULL;
}

static uint32_t raw_bin(double x, uint32_t bins) {
    if (bins < 2u) bins = 2u;
    double u = (x + 1.0) * 0.5;
    if (u < 0.0) u = 0.0;
    if (u > 1.0) u = 1.0;
    uint32_t b = (uint32_t)floor(u * (double)bins);
    return b >= bins ? bins - 1u : b;
}

static double entropy_counts_u64(const uint64_t *counts, size_t n, uint64_t total) {
    if (!counts || total == 0u) return 0.0;
    double h = 0.0;
    for (size_t i = 0u; i < n; ++i) {
        if (counts[i]) {
            double p = (double)counts[i] / (double)total;
            h -= p * log(p);
        }
    }
    return h;
}

typedef struct edge_count {
    uint64_t from;
    uint64_t to;
    uint64_t count;
} edge_count;

static int add_edge(edge_count *edges, size_t *edge_n, size_t edge_cap, uint64_t from, uint64_t to) {
    for (size_t i = 0u; i < *edge_n; ++i) {
        if (edges[i].from == from && edges[i].to == to) {
            edges[i].count++;
            return KDNA_OK;
        }
    }
    if (*edge_n >= edge_cap) return KDNA_EINVAL;
    edges[*edge_n].from = from;
    edges[*edge_n].to = to;
    edges[*edge_n].count = 1u;
    (*edge_n)++;
    return KDNA_OK;
}

static uint64_t best_successor(const edge_count *edges, size_t edge_n, uint64_t from, uint64_t fallback) {
    uint64_t best = fallback;
    uint64_t best_count = 0u;
    for (size_t i = 0u; i < edge_n; ++i) {
        if (edges[i].from == from && edges[i].count > best_count) {
            best_count = edges[i].count;
            best = edges[i].to;
        }
    }
    return best;
}

static int add_value_count(uint64_t *vals, uint64_t *counts, size_t *n, size_t cap, uint64_t v) {
    for (size_t i = 0u; i < *n; ++i) {
        if (vals[i] == v) { counts[i]++; return KDNA_OK; }
    }
    if (*n >= cap) return KDNA_EINVAL;
    vals[*n] = v;
    counts[*n] = 1u;
    (*n)++;
    return KDNA_OK;
}

static uint64_t most_common(const uint64_t *vals, const uint64_t *counts, size_t n, uint64_t fallback, uint64_t *cnt_out) {
    uint64_t best = fallback, bestc = 0u;
    for (size_t i = 0u; i < n; ++i) {
        if (counts[i] > bestc) { bestc = counts[i]; best = vals[i]; }
    }
    if (cnt_out) *cnt_out = bestc;
    return best;
}

int kdna_kexp_generate(uint32_t kind, size_t n, uint64_t seed, double *x) {
    if (!x || n < 4u) return KDNA_EINVAL;
    uint64_t s = seed ? seed : 0x4b4558505f534545ULL;
    if (kind == KDNA_KEXP_PRNG) {
        for (size_t i = 0u; i < n; ++i) x[i] = 2.0 * u01(&s) - 1.0;
        return KDNA_OK;
    }
    if (kind == KDNA_KEXP_LOGISTIC) {
        double y = 0.123456789 + 0.1 * u01(&s);
        const double r = 3.91;
        for (size_t i = 0u; i < n; ++i) {
            y = r * y * (1.0 - y);
            x[i] = 2.0 * y - 1.0;
        }
        return KDNA_OK;
    }
    if (kind == KDNA_KEXP_MARKOV) {
        static const double centers[4] = {-0.82, -0.22, 0.31, 0.79};
        int state = (int)(splitmix64(&s) & 3u);
        for (size_t i = 0u; i < n; ++i) {
            double u = u01(&s);
            switch (state) {
                case 0: state = (u < 0.70) ? 0 : ((u < 0.90) ? 1 : 2); break;
                case 1: state = (u < 0.15) ? 0 : ((u < 0.75) ? 2 : 3); break;
                case 2: state = (u < 0.20) ? 1 : ((u < 0.70) ? 2 : 3); break;
                default: state = (u < 0.45) ? 0 : ((u < 0.65) ? 2 : 3); break;
            }
            x[i] = centers[state] + 0.06 * (2.0 * u01(&s) - 1.0);
        }
        return KDNA_OK;
    }
    if (kind == KDNA_KEXP_HIDDEN_GRAMMAR) {
        static const double centers[6] = {-0.91, -0.55, -0.08, 0.24, 0.63, 0.93};
        static const int next_main[6] = {1, 3, 0, 4, 5, 2};
        static const int next_alt[6] = {2, 0, 3, 1, 2, 4};
        int state = (int)(splitmix64(&s) % 6u);
        for (size_t i = 0u; i < n; ++i) {
            const double u = u01(&s);
            if (u < 0.82) state = next_main[state];
            else if (u < 0.95) state = next_alt[state];
            else state = (int)(splitmix64(&s) % 6u);
            double motif = 0.035 * sin((double)i * 0.173 + (double)state);
            x[i] = centers[state] + motif + 0.025 * (2.0 * u01(&s) - 1.0);
        }
        return KDNA_OK;
    }
    if (kind == KDNA_KEXP_BROWNIAN) {
        double y = 0.0;
        for (size_t i = 0u; i < n; ++i) {
            y += 0.075 * (2.0 * u01(&s) - 1.0);
            if (y > 1.0) y = 2.0 - y;
            if (y < -1.0) y = -2.0 - y;
            x[i] = y + 0.01 * (2.0 * u01(&s) - 1.0);
            if (x[i] > 1.0) x[i] = 1.0;
            if (x[i] < -1.0) x[i] = -1.0;
        }
        return KDNA_OK;
    }
    if (kind == KDNA_KEXP_QUASI_PERIODIC) {
        const double p1 = 0.017 + 0.006 * u01(&s);
        const double p2 = 0.041 + 0.009 * u01(&s);
        const double p3 = 0.113 + 0.015 * u01(&s);
        const double ph1 = 6.283185307179586 * u01(&s);
        const double ph2 = 6.283185307179586 * u01(&s);
        const double ph3 = 6.283185307179586 * u01(&s);
        for (size_t i = 0u; i < n; ++i) {
            const double t = (double)i;
            double y = 0.52 * sin(t * p1 + ph1) + 0.31 * sin(t * p2 + ph2) + 0.17 * sin(t * p3 + ph3);
            y += 0.025 * (2.0 * u01(&s) - 1.0);
            if (y > 1.0) y = 1.0;
            if (y < -1.0) y = -1.0;
            x[i] = y;
        }
        return KDNA_OK;
    }
    return KDNA_EINVAL;
}

int kdna_kexp_analyze(const double *x,
                      size_t n,
                      uint32_t kind,
                      uint64_t seed,
                      double train_fraction,
                      uint32_t raw_bins,
                      const kdna_constants *constants,
                      kdna_kexp_result_record *r) {
    if (!x || n < 8u || !constants || !r) return KDNA_EINVAL;
    if (!(train_fraction > 0.05 && train_fraction < 0.95)) train_fraction = 0.70;
    if (raw_bins < 4u) raw_bins = 32u;

    double *out = (double *)calloc((size_t)KDNA_FIELDS * n, sizeof(double));
    uint64_t *variants = (uint64_t *)calloc(n, sizeof(uint64_t));
    uint64_t *variant_vals = (uint64_t *)calloc(n, sizeof(uint64_t));
    uint64_t *variant_counts = (uint64_t *)calloc(n, sizeof(uint64_t));
    uint64_t *raw_counts = (uint64_t *)calloc(raw_bins, sizeof(uint64_t));
    edge_count *edges = (edge_count *)calloc(n, sizeof(edge_count));
    uint64_t *next_vals = (uint64_t *)calloc(n, sizeof(uint64_t));
    uint64_t *next_counts = (uint64_t *)calloc(n, sizeof(uint64_t));
    if (!out || !variants || !variant_vals || !variant_counts || !raw_counts || !edges || !next_vals || !next_counts) {
        free(out); free(variants); free(variant_vals); free(variant_counts); free(raw_counts); free(edges); free(next_vals); free(next_counts);
        return KDNA_ENOMEM;
    }

    int rc = kdna_eval_cpu(x, out, n, constants);
    if (rc != KDNA_OK) {
        free(out); free(variants); free(variant_vals); free(variant_counts); free(raw_counts); free(edges); free(next_vals); free(next_counts);
        return rc;
    }

    size_t unique_variants = 0u;
    double xmin = x[0], xmax = x[0], sum = 0.0, sum2 = 0.0;
    double mean_lock = 0.0, mean_score = 0.0, max_score = 0.0, null_hits = 0.0;
    for (size_t i = 0u; i < n; ++i) {
        if (x[i] < xmin) xmin = x[i];
        if (x[i] > xmax) xmax = x[i];
        sum += x[i]; sum2 += x[i] * x[i];
        raw_counts[raw_bin(x[i], raw_bins)]++;
        variants[i] = variant_hash_from_k(out, n, i);
        add_value_count(variant_vals, variant_counts, &unique_variants, n, variants[i]);
        mean_lock += out[kdna_idx(KDNA_LK, n, i)];
        const double score = out[kdna_idx(KDNA_DOM_SCORE, n, i)];
        mean_score += score;
        if (score > max_score) max_score = score;
        if (fabs(x[i]) < 1.0e-4) null_hits += 1.0;
    }
    const double mean = sum / (double)n;
    const double var = fmax(0.0, sum2 / (double)n - mean * mean);
    mean_lock /= (double)n;
    mean_score /= (double)n;

    uint32_t unique_raw = 0u;
    for (uint32_t b = 0u; b < raw_bins; ++b) if (raw_counts[b]) unique_raw++;
    const double h_raw = entropy_counts_u64(raw_counts, raw_bins, (uint64_t)n);
    const double h_variant = entropy_counts_u64(variant_counts, unique_variants, (uint64_t)n);

    size_t train_n = (size_t)floor((double)n * train_fraction);
    if (train_n < 4u) train_n = 4u;
    if (train_n >= n - 2u) train_n = n - 2u;

    size_t edge_n = 0u, next_n = 0u;
    for (size_t i = 0u; i + 1u < train_n; ++i) {
        add_edge(edges, &edge_n, n, variants[i], variants[i + 1u]);
        add_value_count(next_vals, next_counts, &next_n, n, variants[i + 1u]);
    }
    uint64_t baseline_count = 0u;
    const uint64_t baseline = most_common(next_vals, next_counts, next_n, variants[train_n], &baseline_count);

    uint64_t baseline_ok = 0u, kgram_ok = 0u, test_trans = 0u, oog = 0u;
    for (size_t i = train_n; i + 1u < n; ++i) {
        const uint64_t actual = variants[i + 1u];
        if (baseline == actual) baseline_ok++;
        const uint64_t pred = best_successor(edges, edge_n, variants[i], baseline);
        if (pred == baseline) {
            int seen = 0;
            for (size_t e = 0u; e < edge_n; ++e) if (edges[e].from == variants[i]) { seen = 1; break; }
            if (!seen) oog++;
        }
        if (pred == actual) kgram_ok++;
        test_trans++;
    }

    uint64_t top_var_count = 0u;
    const uint64_t top_var = most_common(variant_vals, variant_counts, unique_variants, 0u, &top_var_count);
    uint64_t top_from = 0u, top_to = 0u, top_edge_count = 0u;
    for (size_t e = 0u; e < edge_n; ++e) {
        if (edges[e].count > top_edge_count) {
            top_edge_count = edges[e].count; top_from = edges[e].from; top_to = edges[e].to;
        }
    }

    memset(r, 0, sizeof(*r));
    r->id = 1u; r->n = (uint64_t)n; r->train_n = (uint64_t)train_n; r->seed = seed;
    r->kind = kind; r->raw_bins = raw_bins; r->unique_raw_bins = unique_raw; r->unique_variants = (uint32_t)unique_variants;
    r->train_transitions = (uint64_t)(train_n - 1u);
    r->test_transitions = test_trans;
    r->grammar_edges = (uint64_t)edge_n;
    r->out_of_grammar = oog;
    r->x_min = xmin; r->x_max = xmax; r->x_mean = mean; r->x_variance = var;
    r->entropy_raw = h_raw;
    r->entropy_variant = h_variant;
    r->compression_ratio = unique_variants ? (double)n / (double)unique_variants : 0.0;
    r->baseline_accuracy = test_trans ? (double)baseline_ok / (double)test_trans : 0.0;
    r->kgram_accuracy = test_trans ? (double)kgram_ok / (double)test_trans : 0.0;
    r->kgram_lift = r->kgram_accuracy - r->baseline_accuracy;
    r->surprise_rate = test_trans ? (double)oog / (double)test_trans : 0.0;
    r->mean_lock = mean_lock; r->mean_dom_score = mean_score; r->max_dom_score = max_score;
    r->null_membrane_hits = null_hits;
    r->top_variant_id = top_var; r->top_variant_count = top_var_count;
    r->top_edge_from = top_from; r->top_edge_to = top_to; r->top_edge_count = top_edge_count;

    free(out); free(variants); free(variant_vals); free(variant_counts); free(raw_counts); free(edges); free(next_vals); free(next_counts);
    return KDNA_OK;
}

int kdna_kdat_init_header(kdna_kdat_header *h, uint64_t n, uint64_t seed, uint32_t kind, const double *x) {
    if (!h || !x || n == 0u) return KDNA_EINVAL;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;
    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KDAT_MAGIC, 8u);
    h->version = KDNA_KEXP_VERSION;
    h->header_bytes = KDNA_KDAT_HEADER_BYTES;
    h->record_bytes = 8u;
    h->n = n;
    h->seed = seed;
    h->kind = kind;
    double mn = x[0], mx = x[0], sum = 0.0, sum2 = 0.0;
    for (uint64_t i = 0u; i < n; ++i) {
        if (x[i] < mn) mn = x[i]; if (x[i] > mx) mx = x[i];
        sum += x[i]; sum2 += x[i] * x[i];
    }
    h->x_min = mn; h->x_max = mx; h->mean = sum / (double)n;
    h->variance = fmax(0.0, sum2 / (double)n - h->mean * h->mean);
    h->payload_bytes = n * (uint64_t)sizeof(double);
    h->flags = KDNA_KEXP_FLAG_LE_IEEE754_DOUBLE | KDNA_KEXP_FLAG_DETERMINISTIC;
    return KDNA_OK;
}

int kdna_kdat_validate_header(const kdna_kdat_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KDAT_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KEXP_VERSION || h->header_bytes != KDNA_KDAT_HEADER_BYTES || h->record_bytes != 8u) return KDNA_EINVAL;
    if (h->n == 0u || h->payload_bytes != h->n * (uint64_t)sizeof(double)) return KDNA_EINVAL;
    if ((h->flags & KDNA_KEXP_FLAG_LE_IEEE754_DOUBLE) == 0u) return KDNA_EINVAL;
    return KDNA_OK;
}

int kdna_krep_init_header(kdna_krep_header *h, uint64_t experiment_count) {
    if (!h || experiment_count == 0u) return KDNA_EINVAL;
    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KREP_MAGIC, 8u);
    h->version = KDNA_KEXP_VERSION;
    h->header_bytes = KDNA_KREP_HEADER_BYTES;
    h->record_bytes = KDNA_KREP_RECORD_BYTES;
    h->experiment_count = experiment_count;
    h->payload_bytes = experiment_count * (uint64_t)sizeof(kdna_kexp_result_record);
    h->created_unix = (uint64_t)time(NULL);
    h->flags = KDNA_KEXP_FLAG_LE_IEEE754_DOUBLE | KDNA_KEXP_FLAG_KDNA_VARIANTS |
               KDNA_KEXP_FLAG_GRAMMAR_TEST | KDNA_KEXP_FLAG_DETERMINISTIC;
    return KDNA_OK;
}

int kdna_krep_validate_header(const kdna_krep_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KREP_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KEXP_VERSION || h->header_bytes != KDNA_KREP_HEADER_BYTES || h->record_bytes != KDNA_KREP_RECORD_BYTES) return KDNA_EINVAL;
    if (h->experiment_count == 0u || h->payload_bytes != h->experiment_count * (uint64_t)sizeof(kdna_kexp_result_record)) return KDNA_EINVAL;
    if ((h->flags & KDNA_KEXP_FLAG_KDNA_VARIANTS) == 0u) return KDNA_EINVAL;
    return KDNA_OK;
}

int kdna_kdat_write_file(const char *path, const kdna_kdat_header *h, const double *x) {
    if (!path || !h || !x) return KDNA_EINVAL;
    int rc = kdna_kdat_validate_header(h);
    if (rc != KDNA_OK) return rc;
    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;
    int ok = 1;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) ok = 0;
    if (ok && fwrite(x, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) ok = 0;
    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

int kdna_kdat_read_file(const char *path, kdna_kdat_header *h, double **x_out) {
    if (!path || !h || !x_out) return KDNA_EINVAL;
    *x_out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return KDNA_EIO; }
    int rc = kdna_kdat_validate_header(h);
    if (rc != KDNA_OK) { fclose(f); return rc; }
    double *x = (double *)malloc((size_t)h->payload_bytes);
    if (!x) { fclose(f); return KDNA_ENOMEM; }
    if (fread(x, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) { free(x); fclose(f); return KDNA_EIO; }
    if (fclose(f) != 0) { free(x); return KDNA_EIO; }
    *x_out = x;
    return KDNA_OK;
}

int kdna_krep_write_file(const char *path, const kdna_krep_header *h, const kdna_kexp_result_record *records) {
    if (!path || !h || !records) return KDNA_EINVAL;
    int rc = kdna_krep_validate_header(h);
    if (rc != KDNA_OK) return rc;
    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;
    int ok = 1;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) ok = 0;
    if (ok && fwrite(records, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) ok = 0;
    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

int kdna_krep_read_file(const char *path, kdna_krep_header *h, kdna_kexp_result_record **records_out) {
    if (!path || !h || !records_out) return KDNA_EINVAL;
    *records_out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return KDNA_EIO; }
    int rc = kdna_krep_validate_header(h);
    if (rc != KDNA_OK) { fclose(f); return rc; }
    kdna_kexp_result_record *records = (kdna_kexp_result_record *)malloc((size_t)h->payload_bytes);
    if (!records) { fclose(f); return KDNA_ENOMEM; }
    if (fread(records, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) { free(records); fclose(f); return KDNA_EIO; }
    if (fclose(f) != 0) { free(records); return KDNA_EIO; }
    *records_out = records;
    return KDNA_OK;
}
