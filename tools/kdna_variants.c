#include "kdna_kvar.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct kvar_opts {
    const char *kgen_path;
    const char *out_path;
    const char *a_path;
    size_t top;
    int raw;
    int dom;
    uint64_t variant_id;
    double min_resonance;
    double min_injection;
    uint64_t min_count;
    int only_resonant;
} kvar_opts;

static void usage(FILE *f) {
    fprintf(f,
        "kdna_variants --kgen <genesis.kgen> --out <variants.kvar> [query]\n"
        "kdna_variants --a <variants.kvar> [query]\n"
        "\n"
        "Builds or inspects KVAR0001: Ralf's KDNA/SUBQG variant-DNA archive.\n"
        "\n"
        "Build mode:\n"
        "  --kgen <file.kgen>         input SUBQG Genesis field\n"
        "  --out <file.kvar>          output variant archive\n"
        "\n"
        "Inspect/query mode:\n"
        "  --a <file.kvar>            read existing KVAR archive\n"
        "\n"
        "Filters:\n"
        "  --top <n>                  max printed variants, default 32\n"
        "  --raw K1..K5               require RAW\n"
        "  --dom K1..K5               require D\n"
        "  --variant <id>             exact variant_id, decimal or 0x hex\n"
        "  --min-resonance <v>        resonance_mean >= v or resonance_max >= v\n"
        "  --min-injection <v>        injection_mean >= v or injection_max >= v\n"
        "  --min-count <n>            sample_count >= n\n"
        "  --resonant                 require resonant flag\n"
    );
}

static int parse_operator(const char *s, int *out) {
    if (!s || !out) return 0;
    if ((s[0] == 'K' || s[0] == 'k') && s[1] >= '1' && s[1] <= '5' && s[2] == '\0') {
        *out = s[1] - '0';
        return 1;
    }
    if (s[0] >= '1' && s[0] <= '5' && s[1] == '\0') {
        *out = s[0] - '0';
        return 1;
    }
    return 0;
}

static int parse_double(const char *s, double *out) {
    if (!s || !out) return 0;
    char *end = NULL;
    errno = 0;
    double v = strtod(s, &end);
    if (errno || end == s || *end != '\0' || !isfinite(v)) return 0;
    *out = v;
    return 1;
}

static int parse_u64(const char *s, uint64_t *out) {
    if (!s || !out) return 0;
    char *end = NULL;
    errno = 0;
    unsigned long long v = strtoull(s, &end, 0);
    if (errno || end == s || *end != '\0') return 0;
    *out = (uint64_t)v;
    return 1;
}

static int read_kgen(const char *path, kdna_kgen_header *h, double **payload) {
    return kdna_kgen_read_file(path, h, payload);
}

static int matches(const kdna_kvar_record *r, const kvar_opts *o) {
    if (!r || !o) return 0;
    if (o->raw > 0 && (int)r->raw != o->raw) return 0;
    if (o->dom > 0 && (int)r->dom != o->dom) return 0;
    if (o->variant_id != 0u && r->variant_id != o->variant_id) return 0;
    if (o->min_count > 0u && r->sample_count < o->min_count) return 0;
    if (o->min_resonance >= 0.0 && r->resonance_mean < o->min_resonance && r->resonance_max < o->min_resonance) return 0;
    if (o->min_injection >= 0.0 && r->injection_mean < o->min_injection && r->injection_max < o->min_injection) return 0;
    if (o->only_resonant && (r->flags & KDNA_KVAR_REC_FLAG_RESONANT) == 0u) return 0;
    return 1;
}

static double rank_score(const kdna_kvar_record *r) {
    return r->resonance_mean * log1p((double)r->sample_count) +
           r->injection_mean +
           0.25 * r->stability_mean +
           0.1 * log1p(r->score_mean);
}

typedef struct rank_entry {
    size_t index;
    double score;
} rank_entry;

static const kdna_kvar_record *g_records = NULL;
static int cmp_rank_desc(const void *a, const void *b) {
    const rank_entry *x = (const rank_entry *)a;
    const rank_entry *y = (const rank_entry *)b;
    if (x->score > y->score) return -1;
    if (x->score < y->score) return 1;
    if (g_records[x->index].variant_id < g_records[y->index].variant_id) return -1;
    if (g_records[x->index].variant_id > g_records[y->index].variant_id) return 1;
    return 0;
}

static void print_flags(uint32_t flags) {
    const uint32_t all[] = {
        KDNA_KVAR_REC_FLAG_RESONANT,
        KDNA_KVAR_REC_FLAG_HIGH_INJECTION,
        KDNA_KVAR_REC_FLAG_HIGH_STABILITY,
        KDNA_KVAR_REC_FLAG_RAW_DOM_MISMATCH,
        KDNA_KVAR_REC_FLAG_REPEATED,
        KDNA_KVAR_REC_FLAG_HAS_PREDECESSOR,
        KDNA_KVAR_REC_FLAG_HAS_SUCCESSOR,
        KDNA_KVAR_REC_FLAG_NULL_NEAR
    };
    int first = 1;
    printf("[");
    for (size_t i = 0u; i < sizeof(all)/sizeof(all[0]); ++i) {
        if (flags & all[i]) {
            printf("%s%s", first ? "" : ",", kdna_kvar_record_flag_name(all[i]));
            first = 0;
        }
    }
    if (first) printf("none");
    printf("]");
}

static void print_record(const kdna_kvar_record *r) {
    printf("variant:%" PRIu64 " raw:K%u D:K%u count:%" PRIu64
           " x:[%.17g,%.17g] x_mean:%.17g rank:%.17g\n",
           r->variant_id, r->raw, r->dom, r->sample_count,
           r->x_min, r->x_max, r->x_mean, rank_score(r));
    printf("  resonance mean/max:%.17g/%.17g injection mean/max:%.17g/%.17g stability mean:[%.17g] alignment:%.17g\n",
           r->resonance_mean, r->resonance_max, r->injection_mean, r->injection_max,
           r->stability_mean, r->alignment_mean);
    printf("  Kmean:[%.6g %.6g %.6g %.6g %.6g] Smean:[%.6g %.6g %.6g %.6g %.6g] lock mean/max:%.17g/%.17g score mean/max:%.17g/%.17g\n",
           r->k_mean[0], r->k_mean[1], r->k_mean[2], r->k_mean[3], r->k_mean[4],
           r->s_mean[0], r->s_mean[1], r->s_mean[2], r->s_mean[3], r->s_mean[4],
           r->lock_mean, r->lock_max, r->score_mean, r->score_max);
    printf("  first_i:%" PRIu64 " last_i:%" PRIu64 " centroid_i:%.3f span_i:%.3f predecessor:%" PRIu64
           "(%" PRIu64 ") successor:%" PRIu64 "(%" PRIu64 ") flags:",
           r->first_i, r->last_i, r->centroid_i, r->span_i,
           r->predecessor_id, r->predecessor_count, r->successor_id, r->successor_count);
    print_flags(r->flags);
    printf("\n");
}

static void print_summary(const kdna_kvar_header *h, const kdna_kvar_record *r) {
    uint64_t resonant = 0u, high_inj = 0u, repeated = 0u, mismatch = 0u;
    uint64_t count_sum = 0u;
    double max_res = 0.0, max_inj = 0.0, max_rank = 0.0;
    uint64_t best_variant = 0u;
    for (uint64_t i = 0u; i < h->variant_count; ++i) {
        count_sum += r[i].sample_count;
        if (r[i].flags & KDNA_KVAR_REC_FLAG_RESONANT) resonant++;
        if (r[i].flags & KDNA_KVAR_REC_FLAG_HIGH_INJECTION) high_inj++;
        if (r[i].flags & KDNA_KVAR_REC_FLAG_REPEATED) repeated++;
        if (r[i].flags & KDNA_KVAR_REC_FLAG_RAW_DOM_MISMATCH) mismatch++;
        if (r[i].resonance_max > max_res) max_res = r[i].resonance_max;
        if (r[i].injection_max > max_inj) max_inj = r[i].injection_max;
        const double rs = rank_score(&r[i]);
        if (rs > max_rank) { max_rank = rs; best_variant = r[i].variant_id; }
    }

    printf("file: KVAR variants:%" PRIu64 " source_n:%" PRIu64 " source_steps:%" PRIu64
           " seed:%" PRIu64 " payload:%" PRIu64 " x:[%.17g,%.17g] time:%.17g threshold:%.17g\n",
           h->variant_count, h->source_n, h->source_steps, h->source_seed,
           h->payload_bytes, h->x_min, h->x_max, h->time_value, h->resonance_threshold);
    printf("summary: count_sum:%" PRIu64 " resonant:%" PRIu64 " high_injection:%" PRIu64
           " repeated:%" PRIu64 " raw_dom_mismatch:%" PRIu64
           " max_resonance:%.17g max_injection:%.17g best_variant:%" PRIu64 " best_rank:%.17g\n",
           count_sum, resonant, high_inj, repeated, mismatch,
           max_res, max_inj, best_variant, max_rank);
}

static int emit_query(const kdna_kvar_header *h, const kdna_kvar_record *records, const kvar_opts *opts) {
    rank_entry *ranked = (rank_entry *)calloc((size_t)h->variant_count, sizeof(rank_entry));
    if (!ranked) return KDNA_ENOMEM;

    size_t hits = 0u;
    for (uint64_t i = 0u; i < h->variant_count; ++i) {
        if (matches(&records[i], opts)) {
            ranked[hits].index = (size_t)i;
            ranked[hits].score = rank_score(&records[i]);
            hits++;
        }
    }
    g_records = records;
    qsort(ranked, hits, sizeof(rank_entry), cmp_rank_desc);

    size_t shown = 0u;
    for (size_t i = 0u; i < hits && shown < opts->top; ++i) {
        print_record(&records[ranked[i].index]);
        shown++;
    }
    printf("query: hits=%zu shown=%zu\n", hits, shown);
    free(ranked);
    return KDNA_OK;
}

static int build_kvar(const kvar_opts *opts) {
    kdna_kgen_header gh;
    double *payload = NULL;
    int rc = read_kgen(opts->kgen_path, &gh, &payload);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_variants: cannot read KGEN '%s': %s\n", opts->kgen_path, kdna_status_str(rc));
        return 3;
    }

    kdna_kvar_header vh;
    kdna_kvar_record *records = NULL;
    rc = kdna_kvar_build_from_kgen(&gh, payload, &vh, &records);
    free(payload);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_variants: build failed: %s\n", kdna_status_str(rc));
        return 3;
    }

    rc = kdna_kvar_write_file(opts->out_path, &vh, records);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_variants: cannot write '%s': %s\n", opts->out_path, kdna_status_str(rc));
        free(records);
        return 3;
    }

    printf("kdna_variants: wrote %s source=%s variants=%" PRIu64 " source_n=%" PRIu64
           " record_bytes=%u payload_bytes=%" PRIu64 "\n",
           opts->out_path, opts->kgen_path, vh.variant_count, vh.source_n,
           KDNA_KVAR_RECORD_BYTES, vh.payload_bytes);
    print_summary(&vh, records);
    int erc = emit_query(&vh, records, opts);
    free(records);
    return erc == KDNA_OK ? 0 : 3;
}

static int inspect_kvar(const kvar_opts *opts) {
    kdna_kvar_header h;
    kdna_kvar_record *records = NULL;
    int rc = kdna_kvar_read_file(opts->a_path, &h, &records);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_variants: cannot read KVAR '%s': %s\n", opts->a_path, kdna_status_str(rc));
        return 3;
    }
    print_summary(&h, records);
    rc = emit_query(&h, records, opts);
    free(records);
    return rc == KDNA_OK ? 0 : 3;
}

int main(int argc, char **argv) {
    kvar_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.top = 32u;
    opts.raw = 0;
    opts.dom = 0;
    opts.min_resonance = -1.0;
    opts.min_injection = -1.0;
    opts.min_count = 0u;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kgen") == 0 && i + 1 < argc) opts.kgen_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) opts.out_path = argv[++i];
        else if (strcmp(argv[i], "--a") == 0 && i + 1 < argc) opts.a_path = argv[++i];
        else if (strcmp(argv[i], "--top") == 0 && i + 1 < argc) {
            uint64_t v = 0u;
            if (!parse_u64(argv[++i], &v) || v == 0u) { usage(stderr); return 2; }
            opts.top = (size_t)v;
        } else if (strcmp(argv[i], "--raw") == 0 && i + 1 < argc) {
            if (!parse_operator(argv[++i], &opts.raw)) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--dom") == 0 && i + 1 < argc) {
            if (!parse_operator(argv[++i], &opts.dom)) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--variant") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &opts.variant_id)) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--min-resonance") == 0 && i + 1 < argc) {
            if (!parse_double(argv[++i], &opts.min_resonance)) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--min-injection") == 0 && i + 1 < argc) {
            if (!parse_double(argv[++i], &opts.min_injection)) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--min-count") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &opts.min_count)) { usage(stderr); return 2; }
        } else if (strcmp(argv[i], "--resonant") == 0) {
            opts.only_resonant = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(stdout);
            return 0;
        } else {
            usage(stderr);
            return 2;
        }
    }

    const int build_mode = opts.kgen_path && opts.out_path && !opts.a_path;
    const int inspect_mode = opts.a_path && !opts.kgen_path && !opts.out_path;
    if (build_mode == inspect_mode) {
        usage(stderr);
        return 2;
    }
    return build_mode ? build_kvar(&opts) : inspect_kvar(&opts);
}
