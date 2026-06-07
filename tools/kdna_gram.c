#include "kdna.h"
#include "kdna_kgram.h"
#include "kdna_klib.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    KDNA_GRAM_OK = 0,
    KDNA_GRAM_USAGE = 2,
    KDNA_GRAM_RUNTIME = 3
};

typedef struct klib_file {
    const char *path;
    kdna_klib_header h;
    kdna_kword_record *words;
} klib_file;

typedef struct kgram_file {
    const char *path;
    kdna_kgram_header h;
    kdna_krule_record *rules;
} kgram_file;

typedef struct gram_opts {
    const char *lib_path;
    const char *out_path;
    const char *gram_path;
    size_t top;
    int only_changes;
    int only_causal;
    int from_effect;
    int to_effect;
    int from_dom;
    int to_dom;
    double min_strength;
} gram_opts;

static void usage(FILE *f) {
    fprintf(f,
        "kdna_gram --lib <library.klib> --out <grammar.kgram> [filters]\n"
        "kdna_gram --gram <grammar.kgram> [filters]\n"
        "\n"
        "Builds or inspects KGRAM v1: adjacent KWORD transition rules.\n"
        "\n"
        "Build mode:\n"
        "  --lib <file.klib>          input resonance vocabulary\n"
        "  --out <file.kgram>         output transition grammar\n"
        "\n"
        "Inspect/query mode:\n"
        "  --gram <file.kgram>        read existing grammar\n"
        "\n"
        "Filters:\n"
        "  --top <n>                  max printed rules, default 32\n"
        "  --changes                  only rules with RAW/DOM/effect change\n"
        "  --causal                   only high-signal causal rules\n"
        "  --from-effect <name|id>    require source effect\n"
        "  --to-effect <name|id>      require target effect\n"
        "  --from-dom K1..K5          require source D\n"
        "  --to-dom K1..K5            require target D\n"
        "  --min-strength <v>         require strength >= v\n"
        "\n"
        "Examples:\n"
        "  kdna_gram --lib library_cpu.klib --out grammar_cpu.kgram\n"
        "  kdna_gram --gram grammar_cpu.kgram --causal --top 16\n"
        "  kdna_gram --gram grammar_cpu.kgram --from-effect null --to-effect compression\n"
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

static void free_kgram(kgram_file *gf) {
    if (gf) {
        free(gf->rules);
        gf->rules = NULL;
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

static int read_kgram_payload(const char *path, kgram_file *out) {
    if (!path || !out) return KDNA_EINVAL;
    memset(out, 0, sizeof(*out));
    out->path = path;

    int rc = kdna_kgram_read_header_file(path, &out->h);
    if (rc != KDNA_OK) return rc;

    if (out->h.rule_count > 0u) {
        out->rules = (kdna_krule_record *)calloc((size_t)out->h.rule_count, sizeof(kdna_krule_record));
        if (!out->rules) return KDNA_ENOMEM;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        free_kgram(out);
        return KDNA_EIO;
    }

    int ok = 1;
    if (fseek(f, (long)out->h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && out->h.rule_count > 0u && !read_exact(f, out->rules, (size_t)out->h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;

    if (!ok) {
        free_kgram(out);
        return KDNA_EIO;
    }
    return KDNA_OK;
}

static int parse_size_arg(const char *s, size_t *out) {
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

static void print_rule_flags(uint32_t flags) {
    int first = 1;
    const uint32_t known[] = {
        KDNA_KRULE_FLAG_RAW_CHANGE,
        KDNA_KRULE_FLAG_DOM_CHANGE,
        KDNA_KRULE_FLAG_EFFECT_CHANGE,
        KDNA_KRULE_FLAG_CROSSES_ZERO,
        KDNA_KRULE_FLAG_NULL_TO_COMPRESSION,
        KDNA_KRULE_FLAG_COMPRESSION_TO_NULL,
        KDNA_KRULE_FLAG_HIGH_LOCK_BRIDGE,
        KDNA_KRULE_FLAG_SCORE_RISE,
        KDNA_KRULE_FLAG_SCORE_FALL,
        KDNA_KRULE_FLAG_RAW_DOM_REWIRE
    };
    printf("[");
    for (size_t i = 0u; i < sizeof(known) / sizeof(known[0]); ++i) {
        if (flags & known[i]) {
            printf("%s%s", first ? "" : ",", kdna_krule_flag_name(known[i]));
            first = 0;
        }
    }
    if (first) printf("none");
    printf("]");
}

static int rule_matches(const kdna_krule_record *r, const gram_opts *o) {
    if (!r || !o) return 0;
    if (o->only_changes && !(r->flags & (KDNA_KRULE_FLAG_RAW_CHANGE | KDNA_KRULE_FLAG_DOM_CHANGE | KDNA_KRULE_FLAG_EFFECT_CHANGE))) return 0;
    if (o->only_causal && !(r->flags & (KDNA_KRULE_FLAG_NULL_TO_COMPRESSION |
                                        KDNA_KRULE_FLAG_COMPRESSION_TO_NULL |
                                        KDNA_KRULE_FLAG_CROSSES_ZERO |
                                        KDNA_KRULE_FLAG_RAW_DOM_REWIRE)) &&
        r->strength < 1.0) return 0;
    if (o->from_effect >= 0 && (int)r->from_effect != o->from_effect) return 0;
    if (o->to_effect >= 0 && (int)r->to_effect != o->to_effect) return 0;
    if (o->from_dom > 0 && (int)r->from_dom != o->from_dom) return 0;
    if (o->to_dom > 0 && (int)r->to_dom != o->to_dom) return 0;
    if (r->strength < o->min_strength) return 0;
    return 1;
}

static void print_rule(const kdna_krule_record *r) {
    printf("  id:%" PRIu64 " seq:%" PRIu64 " word:%" PRIu64 "->%" PRIu64
           " x_boundary:%.17g gap:%.17g strength:%.17g\n",
           r->id, r->sequence_index, r->from_id, r->to_id,
           r->boundary_x, r->gap_x, r->strength);
    printf("    from RAW:K%u D:K%u effect:%s x:[%.17g,%.17g] lock:%.17g score:%.17g\n",
           r->from_raw, r->from_dom, kdna_effect_class_name(r->from_effect),
           r->from_x0, r->from_x1, r->from_lock_mean, r->from_score_mean);
    printf("    to   RAW:K%u D:K%u effect:%s x:[%.17g,%.17g] lock:%.17g score:%.17g\n",
           r->to_raw, r->to_dom, kdna_effect_class_name(r->to_effect),
           r->to_x0, r->to_x1, r->to_lock_mean, r->to_score_mean);
    printf("    delta_lock:% .17e delta_score:% .17e flags:", r->delta_lock_mean, r->delta_score_mean);
    print_rule_flags(r->flags);
    printf("\n");
}

static void print_kgram_summary(const kgram_file *gf) {
    printf("file: %s\n", gf->path);
    printf("  magic: %.8s version:%u header_bytes:%u record_bytes:%u\n",
           gf->h.magic, gf->h.version, gf->h.header_bytes, gf->h.record_bytes);
    printf("  rules:%" PRIu64 " source_words:%" PRIu64 " source_n:%" PRIu64
           " x_min:%.17g x_max:%.17g dx:%.17g payload_bytes:%" PRIu64 " flags:0x%" PRIx64 "\n",
           gf->h.rule_count, gf->h.source_word_count, gf->h.source_n,
           gf->h.x_min, gf->h.x_max, gf->h.dx, gf->h.payload_bytes, gf->h.flags);
}

static int build_grammar(const gram_opts *opts) {
    klib_file kf;
    int rc = read_klib_payload(opts->lib_path, &kf);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_gram: cannot read '%s': error %d\n", opts->lib_path, rc);
        return KDNA_GRAM_RUNTIME;
    }

    const size_t rule_count = kf.h.word_count > 0u ? (size_t)kf.h.word_count - 1u : 0u;
    kdna_krule_record *rules = NULL;
    if (rule_count > 0u) {
        rules = (kdna_krule_record *)calloc(rule_count, sizeof(kdna_krule_record));
        if (!rules) {
            free_klib(&kf);
            fprintf(stderr, "kdna_gram: allocation failed\n");
            return KDNA_GRAM_RUNTIME;
        }
    }

    kdna_kgram_header gh;
    rc = kdna_kgram_init_header(&gh, rule_count, (size_t)kf.h.word_count, (size_t)kf.h.source_n,
                                kf.h.x_min, kf.h.x_max, kf.h.dx);
    if (rc == KDNA_OK) rc = kdna_kgram_build_rules(&kf.h, kf.words, rules, rule_count);
    if (rc == KDNA_OK) rc = kdna_kgram_write_file(opts->out_path, &gh, rules);

    if (rc != KDNA_OK) {
        free(rules);
        free_klib(&kf);
        fprintf(stderr, "kdna_gram: failed to build grammar: error %d\n", rc);
        return KDNA_GRAM_RUNTIME;
    }

    printf("kdna_gram: wrote %s source=%s rules=%zu source_words=%" PRIu64
           " record_bytes=%u payload_bytes=%" PRIu64 "\n",
           opts->out_path, opts->lib_path, rule_count, kf.h.word_count,
           KDNA_KGRAM_RECORD_BYTES, gh.payload_bytes);

    kgram_file gf;
    memset(&gf, 0, sizeof(gf));
    gf.path = opts->out_path;
    gf.h = gh;
    gf.rules = rules;
    print_kgram_summary(&gf);

    size_t shown = 0u;
    for (size_t i = 0u; i < rule_count && shown < opts->top; ++i) {
        if (rule_matches(&rules[i], opts)) {
            print_rule(&rules[i]);
            ++shown;
        }
    }
    printf("query: hits=");
    size_t hits = 0u;
    for (size_t i = 0u; i < rule_count; ++i) if (rule_matches(&rules[i], opts)) ++hits;
    printf("%zu shown=%zu\n", hits, shown);

    free(rules);
    free_klib(&kf);
    return KDNA_GRAM_OK;
}

static int inspect_grammar(const gram_opts *opts) {
    kgram_file gf;
    int rc = read_kgram_payload(opts->gram_path, &gf);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_gram: cannot read '%s': error %d\n", opts->gram_path, rc);
        return KDNA_GRAM_RUNTIME;
    }

    print_kgram_summary(&gf);

    size_t hits = 0u;
    size_t shown = 0u;
    uint64_t raw_changes = 0u;
    uint64_t dom_changes = 0u;
    uint64_t effect_changes = 0u;
    uint64_t causal = 0u;
    double max_strength = 0.0;
    uint64_t max_strength_id = 0u;

    for (uint64_t i = 0u; i < gf.h.rule_count; ++i) {
        const kdna_krule_record *r = &gf.rules[i];
        if (r->flags & KDNA_KRULE_FLAG_RAW_CHANGE) ++raw_changes;
        if (r->flags & KDNA_KRULE_FLAG_DOM_CHANGE) ++dom_changes;
        if (r->flags & KDNA_KRULE_FLAG_EFFECT_CHANGE) ++effect_changes;
        if (r->flags & (KDNA_KRULE_FLAG_NULL_TO_COMPRESSION |
                        KDNA_KRULE_FLAG_COMPRESSION_TO_NULL |
                        KDNA_KRULE_FLAG_CROSSES_ZERO |
                        KDNA_KRULE_FLAG_RAW_DOM_REWIRE)) ++causal;
        if (r->strength > max_strength) {
            max_strength = r->strength;
            max_strength_id = r->id;
        }
    }

    printf("summary: raw_changes:%" PRIu64 " dom_changes:%" PRIu64
           " effect_changes:%" PRIu64 " causal_candidates:%" PRIu64
           " max_strength:%.17g rule:%" PRIu64 "\n",
           raw_changes, dom_changes, effect_changes, causal,
           max_strength, max_strength_id);

    for (uint64_t i = 0u; i < gf.h.rule_count; ++i) {
        const kdna_krule_record *r = &gf.rules[i];
        if (rule_matches(r, opts)) {
            ++hits;
            if (shown < opts->top) {
                print_rule(r);
                ++shown;
            }
        }
    }
    printf("query: hits=%zu shown=%zu\n", hits, shown);

    free_kgram(&gf);
    return KDNA_GRAM_OK;
}

int main(int argc, char **argv) {
    gram_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.top = 32u;
    opts.from_effect = -1;
    opts.to_effect = -1;
    opts.min_strength = -INFINITY;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--lib") == 0 && i + 1 < argc) {
            opts.lib_path = argv[++i];
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            opts.out_path = argv[++i];
        } else if (strcmp(argv[i], "--gram") == 0 && i + 1 < argc) {
            opts.gram_path = argv[++i];
        } else if (strcmp(argv[i], "--top") == 0 && i + 1 < argc) {
            if (!parse_size_arg(argv[++i], &opts.top)) {
                fprintf(stderr, "kdna_gram: invalid --top\n");
                return KDNA_GRAM_USAGE;
            }
        } else if (strcmp(argv[i], "--changes") == 0) {
            opts.only_changes = 1;
        } else if (strcmp(argv[i], "--causal") == 0) {
            opts.only_causal = 1;
        } else if (strcmp(argv[i], "--from-effect") == 0 && i + 1 < argc) {
            uint32_t effect = 0u;
            if (kdna_effect_class_from_name(argv[++i], &effect) != KDNA_OK) {
                fprintf(stderr, "kdna_gram: invalid --from-effect\n");
                return KDNA_GRAM_USAGE;
            }
            opts.from_effect = (int)effect;
        } else if (strcmp(argv[i], "--to-effect") == 0 && i + 1 < argc) {
            uint32_t effect = 0u;
            if (kdna_effect_class_from_name(argv[++i], &effect) != KDNA_OK) {
                fprintf(stderr, "kdna_gram: invalid --to-effect\n");
                return KDNA_GRAM_USAGE;
            }
            opts.to_effect = (int)effect;
        } else if (strcmp(argv[i], "--from-dom") == 0 && i + 1 < argc) {
            if (!parse_operator(argv[++i], &opts.from_dom)) {
                fprintf(stderr, "kdna_gram: invalid --from-dom\n");
                return KDNA_GRAM_USAGE;
            }
        } else if (strcmp(argv[i], "--to-dom") == 0 && i + 1 < argc) {
            if (!parse_operator(argv[++i], &opts.to_dom)) {
                fprintf(stderr, "kdna_gram: invalid --to-dom\n");
                return KDNA_GRAM_USAGE;
            }
        } else if (strcmp(argv[i], "--min-strength") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &opts.min_strength)) {
                fprintf(stderr, "kdna_gram: invalid --min-strength\n");
                return KDNA_GRAM_USAGE;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(stdout);
            return KDNA_GRAM_OK;
        } else {
            fprintf(stderr, "kdna_gram: unknown or incomplete argument '%s'\n", argv[i]);
            usage(stderr);
            return KDNA_GRAM_USAGE;
        }
    }

    const int build_mode = opts.lib_path && opts.out_path;
    const int inspect_mode = opts.gram_path != NULL;

    if (build_mode == inspect_mode) {
        usage(stderr);
        return KDNA_GRAM_USAGE;
    }

    if (build_mode) return build_grammar(&opts);
    return inspect_grammar(&opts);
}
