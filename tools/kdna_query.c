#include "kdna.h"
#include "kdna_klib.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum kdna_query_exit {
    KDNA_QUERY_OK = 0,
    KDNA_QUERY_USAGE = 2,
    KDNA_QUERY_RUNTIME = 3
};

typedef struct klib_file {
    const char *path;
    kdna_klib_header h;
    kdna_kword_record *words;
} klib_file;

typedef struct query_filter {
    int raw;              /* 0 means any */
    int dom;              /* 0 means any */
    int effect;           /* -1 means any */
    double lock_min;
    double score_min;
    double width_min;
    double width_max;
    uint32_t require_flags;
    size_t top;
} query_filter;

typedef struct ranked_word {
    size_t index;
    double rank;
} ranked_word;

static void usage(FILE *f) {
    fprintf(f,
        "kdna_query --lib <library.klib> [filters]\n"
        "\n"
        "Filters KLIB v1 resonance vocabulary records without re-running simulation.\n"
        "\n"
        "Filters:\n"
        "  --raw K1|1                 require RAW operator\n"
        "  --dom K4|4                 require activation-normalized dominance operator\n"
        "  --effect <name|id>         stable_attractor|envelope_form|phase_offset|\n"
        "                             compression_gate|cascade_band|null_membrane_jump|\n"
        "                             raw_dom_mismatch_zone|transition_bridge\n"
        "  --lock-min <v>             require lock_mean >= v\n"
        "  --score-min <v>            require score_max >= v\n"
        "  --width-min <v>            require abs(width) >= v\n"
        "  --width-max <v>            require abs(width) <= v\n"
        "  --flag null|mismatch|high_lock|narrow|high_score\n"
        "  --top <n>                  max output rows, default 16\n"
        "\n"
        "Examples:\n"
        "  kdna_query --lib library.klib --dom K4 --effect compression --top 8\n"
        "  kdna_query --lib library.klib --effect null --top 16\n"
    );
}

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

static void free_klib(klib_file *kf) {
    if (kf) {
        free(kf->words);
        kf->words = NULL;
    }
}

static int read_klib_payload(const char *path, klib_file *out) {
    if (!path || !out) return KDNA_EINVAL;
    memset(out, 0, sizeof(*out));
    out->path = path;

    int rc = kdna_klib_read_header_file(path, &out->h);
    if (rc != KDNA_OK) return rc;

    out->words = (kdna_kword_record *)calloc((size_t)out->h.word_count, sizeof(kdna_kword_record));
    if (!out->words) return KDNA_ENOMEM;

    FILE *f = fopen(path, "rb");
    if (!f) {
        free_klib(out);
        return KDNA_EIO;
    }

    int ok = 1;
    if (fseek(f, (long)out->h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && !read_exact(f, out->words, (size_t)out->h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;

    if (!ok) {
        free_klib(out);
        return KDNA_EIO;
    }

    return KDNA_OK;
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

static int parse_size(const char *s, size_t *out) {
    if (!s || !out) return 0;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (end == s || *end != '\0' || v == 0ull) return 0;
    *out = (size_t)v;
    return 1;
}

static int parse_double_arg(const char *s, double *out) {
    if (!s || !out) return 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0' || !isfinite(v)) return 0;
    *out = v;
    return 1;
}

static int parse_flag(const char *s, uint32_t *flag_out) {
    if (!s || !flag_out) return 0;
    if (strcmp(s, "null") == 0 || strcmp(s, "null_near") == 0) {
        *flag_out = KDNA_KWORD_FLAG_NULL_NEAR;
    } else if (strcmp(s, "mismatch") == 0 || strcmp(s, "raw_dom_mismatch") == 0) {
        *flag_out = KDNA_KWORD_FLAG_RAW_DOM_MISMATCH;
    } else if (strcmp(s, "high_lock") == 0 || strcmp(s, "lock") == 0) {
        *flag_out = KDNA_KWORD_FLAG_HIGH_LOCK;
    } else if (strcmp(s, "narrow") == 0) {
        *flag_out = KDNA_KWORD_FLAG_NARROW;
    } else if (strcmp(s, "high_score") == 0 || strcmp(s, "score") == 0) {
        *flag_out = KDNA_KWORD_FLAG_HIGH_SCORE;
    } else {
        return 0;
    }
    return 1;
}

static int match_word(const kdna_kword_record *w, const query_filter *q) {
    if (!w || !q) return 0;
    if (q->raw != 0 && w->raw != (uint32_t)q->raw) return 0;
    if (q->dom != 0 && w->dom != (uint32_t)q->dom) return 0;
    if (q->effect >= 0 && w->effect_class != (uint32_t)q->effect) return 0;
    if (w->lock_mean < q->lock_min) return 0;
    if (w->score_max < q->score_min) return 0;
    if (fabs(w->width) < q->width_min) return 0;
    if (q->width_max >= 0.0 && fabs(w->width) > q->width_max) return 0;
    if ((w->flags & q->require_flags) != q->require_flags) return 0;
    return 1;
}

static int cmp_rank_desc(const void *a, const void *b) {
    const ranked_word *ra = (const ranked_word *)a;
    const ranked_word *rb = (const ranked_word *)b;
    if (ra->rank < rb->rank) return 1;
    if (ra->rank > rb->rank) return -1;
    if (ra->index > rb->index) return 1;
    if (ra->index < rb->index) return -1;
    return 0;
}

static void print_flag_names(uint32_t flags) {
    int any = 0;
    printf("[");
    if ((flags & KDNA_KWORD_FLAG_NULL_NEAR) != 0u) { printf("%snull", any ? "," : ""); any = 1; }
    if ((flags & KDNA_KWORD_FLAG_RAW_DOM_MISMATCH) != 0u) { printf("%smismatch", any ? "," : ""); any = 1; }
    if ((flags & KDNA_KWORD_FLAG_HIGH_LOCK) != 0u) { printf("%shigh_lock", any ? "," : ""); any = 1; }
    if ((flags & KDNA_KWORD_FLAG_NARROW) != 0u) { printf("%snarrow", any ? "," : ""); any = 1; }
    if ((flags & KDNA_KWORD_FLAG_HIGH_SCORE) != 0u) { printf("%shigh_score", any ? "," : ""); any = 1; }
    printf("]");
}

static void print_header(const klib_file *kf) {
    printf("file: %s\n", kf->path);
    printf("  magic: %.8s version:%u header_bytes:%u record_bytes:%u\n",
           kf->h.magic, kf->h.version, kf->h.header_bytes, kf->h.record_bytes);
    printf("  words:%" PRIu64 " source_n:%" PRIu64 " source_segments:%" PRIu64
           " x_min:%.17g x_max:%.17g dx:%.17g payload_bytes:%" PRIu64 " flags:0x%" PRIx64 "\n",
           kf->h.word_count,
           kf->h.source_n,
           kf->h.source_segment_count,
           kf->h.x_min,
           kf->h.x_max,
           kf->h.dx,
           kf->h.payload_bytes,
           kf->h.flags);
}

int main(int argc, char **argv) {
    const char *lib_path = NULL;
    query_filter q;
    q.raw = 0;
    q.dom = 0;
    q.effect = -1;
    q.lock_min = -INFINITY;
    q.score_min = -INFINITY;
    q.width_min = 0.0;
    q.width_max = -1.0;
    q.require_flags = 0u;
    q.top = 16u;

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
            usage(stdout);
            return KDNA_QUERY_OK;
        } else if (strcmp(a, "--lib") == 0 && i + 1 < argc) {
            lib_path = argv[++i];
        } else if (strcmp(a, "--raw") == 0 && i + 1 < argc) {
            if (!parse_operator(argv[++i], &q.raw)) {
                fprintf(stderr, "kdna_query: invalid --raw\n");
                return KDNA_QUERY_USAGE;
            }
        } else if (strcmp(a, "--dom") == 0 && i + 1 < argc) {
            if (!parse_operator(argv[++i], &q.dom)) {
                fprintf(stderr, "kdna_query: invalid --dom\n");
                return KDNA_QUERY_USAGE;
            }
        } else if (strcmp(a, "--effect") == 0 && i + 1 < argc) {
            uint32_t e = 0u;
            if (kdna_effect_class_from_name(argv[++i], &e) != KDNA_OK) {
                fprintf(stderr, "kdna_query: invalid --effect\n");
                return KDNA_QUERY_USAGE;
            }
            q.effect = (int)e;
        } else if (strcmp(a, "--lock-min") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &q.lock_min)) {
                fprintf(stderr, "kdna_query: invalid --lock-min\n");
                return KDNA_QUERY_USAGE;
            }
        } else if (strcmp(a, "--score-min") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &q.score_min)) {
                fprintf(stderr, "kdna_query: invalid --score-min\n");
                return KDNA_QUERY_USAGE;
            }
        } else if (strcmp(a, "--width-min") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &q.width_min) || q.width_min < 0.0) {
                fprintf(stderr, "kdna_query: invalid --width-min\n");
                return KDNA_QUERY_USAGE;
            }
        } else if (strcmp(a, "--width-max") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &q.width_max) || q.width_max < 0.0) {
                fprintf(stderr, "kdna_query: invalid --width-max\n");
                return KDNA_QUERY_USAGE;
            }
        } else if (strcmp(a, "--flag") == 0 && i + 1 < argc) {
            uint32_t fl = 0u;
            if (!parse_flag(argv[++i], &fl)) {
                fprintf(stderr, "kdna_query: invalid --flag\n");
                return KDNA_QUERY_USAGE;
            }
            q.require_flags |= fl;
        } else if (strcmp(a, "--top") == 0 && i + 1 < argc) {
            if (!parse_size(argv[++i], &q.top)) {
                fprintf(stderr, "kdna_query: invalid --top\n");
                return KDNA_QUERY_USAGE;
            }
        } else {
            fprintf(stderr, "kdna_query: unknown or incomplete argument '%s'\n", a);
            usage(stderr);
            return KDNA_QUERY_USAGE;
        }
    }

    if (!lib_path) {
        fprintf(stderr, "kdna_query: --lib is required\n");
        usage(stderr);
        return KDNA_QUERY_USAGE;
    }

    klib_file kf;
    int rc = read_klib_payload(lib_path, &kf);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_query: cannot read '%s': %s\n", lib_path, kdna_status_str(rc));
        return KDNA_QUERY_RUNTIME;
    }

    ranked_word *hits = (ranked_word *)calloc((size_t)kf.h.word_count, sizeof(ranked_word));
    if (!hits) {
        free_klib(&kf);
        fprintf(stderr, "kdna_query: allocation failed\n");
        return KDNA_QUERY_RUNTIME;
    }

    size_t hit_count = 0u;
    for (size_t i = 0u; i < (size_t)kf.h.word_count; ++i) {
        const kdna_kword_record *w = &kf.words[i];
        if (match_word(w, &q)) {
            hits[hit_count].index = i;
            hits[hit_count].rank = w->score_mean;
            hit_count += 1u;
        }
    }

    qsort(hits, hit_count, sizeof(ranked_word), cmp_rank_desc);

    print_header(&kf);
    printf("query: hits=%zu shown=%zu\n", hit_count, hit_count < q.top ? hit_count : q.top);

    const size_t shown = hit_count < q.top ? hit_count : q.top;
    for (size_t hix = 0u; hix < shown; ++hix) {
        const kdna_kword_record *w = &kf.words[hits[hix].index];
        printf("  id:%" PRIu64 " seg:%" PRIu64 " i:[%" PRIu64 ",%" PRIu64 "]"
               " x:[%.17g,%.17g] width:%.17g RAW:K%u D:K%u effect:%s"
               " lock:[%.17g,%.17g] mean:%.17g score:[%.17g,%.17g] mean:%.17g flags:",
               w->id,
               w->source_segment_index,
               w->i0,
               w->i1,
               w->x0,
               w->x1,
               w->width,
               w->raw,
               w->dom,
               kdna_effect_class_name(w->effect_class),
               w->lock_min,
               w->lock_max,
               w->lock_mean,
               w->score_min,
               w->score_max,
               w->score_mean);
        print_flag_names(w->flags);
        printf("\n");
    }

    free(hits);
    free_klib(&kf);
    return KDNA_QUERY_OK;
}
