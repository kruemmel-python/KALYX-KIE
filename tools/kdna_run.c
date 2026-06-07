#include "kdna.h"
#include "kdna_kgram.h"
#include "kdna_klib.h"
#include "kdna_krun.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    KDNA_RUN_OK = 0,
    KDNA_RUN_USAGE = 2,
    KDNA_RUN_RUNTIME = 3
};

#define RUN_SELECTOR_CAUSAL       0x00000001u
#define RUN_SELECTOR_CHANGES      0x00000002u
#define RUN_SELECTOR_FROM_EFFECT  0x00000004u
#define RUN_SELECTOR_TO_EFFECT    0x00000008u
#define RUN_SELECTOR_FROM_DOM     0x00000010u
#define RUN_SELECTOR_TO_DOM       0x00000020u
#define RUN_SELECTOR_MIN_STRENGTH 0x00000040u

typedef struct kgram_file {
    const char *path;
    kdna_kgram_header h;
    kdna_krule_record *rules;
} kgram_file;

typedef struct krun_file {
    const char *path;
    kdna_krun_header h;
    kdna_krun_step_record *steps;
} krun_file;

typedef struct run_opts {
    const char *gram_path;
    const char *out_path;
    const char *run_path;
    size_t top;
    size_t max_steps;
    int only_changes;
    int only_causal;
    int from_effect;
    int to_effect;
    int from_dom;
    int to_dom;
    double min_strength;
} run_opts;

static void usage(FILE *f) {
    fprintf(f,
        "kdna_run --gram <grammar.kgram> --out <plan.krun> [selectors]\n"
        "kdna_run --run <plan.krun> [--top n]\n"
        "\n"
        "Builds or inspects KRUN v1: executable deterministic resonance plans.\n"
        "\n"
        "Build mode:\n"
        "  --gram <file.kgram>        input transition grammar\n"
        "  --out <file.krun>          output executable run plan\n"
        "\n"
        "Inspect mode:\n"
        "  --run <file.krun>          read existing run plan\n"
        "\n"
        "Selectors:\n"
        "  --top <n>                  max printed steps, default 32\n"
        "  --max-steps <n>            max selected steps when writing, default all matches\n"
        "  --changes                  only RAW/DOM/effect changing rules\n"
        "  --causal                   only causal-candidate rules\n"
        "  --from-effect <name|id>    require source effect\n"
        "  --to-effect <name|id>      require target effect\n"
        "  --from-dom K1..K5          require source D\n"
        "  --to-dom K1..K5            require target D\n"
        "  --min-strength <v>         require KRULE strength >= v\n"
        "\n"
        "Examples:\n"
        "  kdna_run --gram grammar_cpu.kgram --out run_causal.krun --causal\n"
        "  kdna_run --gram grammar_cpu.kgram --out run_null_to_compression.krun --from-effect null --to-effect compression\n"
        "  kdna_run --run run_causal.krun --top 16\n"
    );
}

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

static void free_kgram(kgram_file *gf) {
    if (gf) {
        free(gf->rules);
        gf->rules = NULL;
    }
}

static void free_krun(krun_file *rf) {
    if (rf) {
        free(rf->steps);
        rf->steps = NULL;
    }
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

static int read_krun_payload(const char *path, krun_file *out) {
    if (!path || !out) return KDNA_EINVAL;
    memset(out, 0, sizeof(*out));
    out->path = path;
    int rc = kdna_krun_read_header_file(path, &out->h);
    if (rc != KDNA_OK) return rc;

    if (out->h.step_count > 0u) {
        out->steps = (kdna_krun_step_record *)calloc((size_t)out->h.step_count, sizeof(kdna_krun_step_record));
        if (!out->steps) return KDNA_ENOMEM;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        free_krun(out);
        return KDNA_EIO;
    }
    int ok = 1;
    if (fseek(f, (long)out->h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && out->h.step_count > 0u && !read_exact(f, out->steps, (size_t)out->h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        free_krun(out);
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

static int rule_is_causal(const kdna_krule_record *r) {
    return (r->flags & (KDNA_KRULE_FLAG_NULL_TO_COMPRESSION |
                        KDNA_KRULE_FLAG_COMPRESSION_TO_NULL |
                        KDNA_KRULE_FLAG_CROSSES_ZERO |
                        KDNA_KRULE_FLAG_RAW_DOM_REWIRE)) != 0u;
}

static int rule_matches(const kdna_krule_record *r, const run_opts *o) {
    if (!r || !o) return 0;
    if (o->only_changes && !(r->flags & (KDNA_KRULE_FLAG_RAW_CHANGE | KDNA_KRULE_FLAG_DOM_CHANGE | KDNA_KRULE_FLAG_EFFECT_CHANGE))) return 0;
    if (o->only_causal && !rule_is_causal(r)) return 0;
    if (o->from_effect >= 0 && (int)r->from_effect != o->from_effect) return 0;
    if (o->to_effect >= 0 && (int)r->to_effect != o->to_effect) return 0;
    if (o->from_dom > 0 && (int)r->from_dom != o->from_dom) return 0;
    if (o->to_dom > 0 && (int)r->to_dom != o->to_dom) return 0;
    if (r->strength < o->min_strength) return 0;
    return 1;
}

static uint32_t selector_flags_from_opts(const run_opts *o) {
    uint32_t flags = 0u;
    if (o->only_causal) flags |= RUN_SELECTOR_CAUSAL;
    if (o->only_changes) flags |= RUN_SELECTOR_CHANGES;
    if (o->from_effect >= 0) flags |= RUN_SELECTOR_FROM_EFFECT;
    if (o->to_effect >= 0) flags |= RUN_SELECTOR_TO_EFFECT;
    if (o->from_dom > 0) flags |= RUN_SELECTOR_FROM_DOM;
    if (o->to_dom > 0) flags |= RUN_SELECTOR_TO_DOM;
    if (o->min_strength > 0.0) flags |= RUN_SELECTOR_MIN_STRENGTH;
    return flags;
}

static void print_step_flags(uint32_t flags) {
    int first = 1;
    printf("[");
    if (flags & KDNA_KRUN_STEP_FLAG_RAW_CHANGE) { printf("%sraw_change", first ? "" : ","); first = 0; }
    if (flags & KDNA_KRUN_STEP_FLAG_DOM_CHANGE) { printf("%sdom_change", first ? "" : ","); first = 0; }
    if (flags & KDNA_KRUN_STEP_FLAG_EFFECT_CHANGE) { printf("%seffect_change", first ? "" : ","); first = 0; }
    if (flags & KDNA_KRUN_STEP_FLAG_CROSSES_ZERO) { printf("%scrosses_zero", first ? "" : ","); first = 0; }
    if (flags & KDNA_KRUN_STEP_FLAG_HIGH_LOCK) { printf("%shigh_lock", first ? "" : ","); first = 0; }
    if (flags & KDNA_KRUN_STEP_FLAG_SCORE_RISE) { printf("%sscore_rise", first ? "" : ","); first = 0; }
    if (flags & KDNA_KRUN_STEP_FLAG_SCORE_FALL) { printf("%sscore_fall", first ? "" : ","); first = 0; }
    if (flags & KDNA_KRUN_STEP_FLAG_CAUSAL_CANDIDATE) { printf("%scausal", first ? "" : ","); first = 0; }
    if (first) printf("none");
    printf("]");
}

static void print_step(const kdna_krun_step_record *s) {
    printf("  step:%" PRIu64 " rule:%" PRIu64 " word:%" PRIu64 "->%" PRIu64
           " action:%s drive_x:%.17g gain:%.17g bias:%.17g duration:%.17g\n",
           s->id, s->source_rule_id, s->from_word_id, s->to_word_id,
           kdna_krun_action_name(s->action), s->drive_x, s->gain, s->bias, s->duration);
    printf("    x:[%.17g -> %.17g -> %.17g] RAW:K%u->K%u D:K%u->K%u effect:%s->%s\n",
           s->x_start, s->x_boundary, s->x_end,
           s->from_raw, s->to_raw, s->from_dom, s->to_dom,
           kdna_effect_class_name(s->from_effect), kdna_effect_class_name(s->to_effect));
    printf("    lock:%.17g->%.17g score:%.17g->%.17g delta_lock:% .17e delta_score:% .17e strength:%.17g flags:",
           s->lock_start, s->lock_end, s->score_start, s->score_end,
           s->delta_lock, s->delta_score, s->strength);
    print_step_flags(s->flags);
    printf("\n");
}

static void print_krun_summary(const krun_file *rf) {
    printf("file: %s\n", rf->path);
    printf("  magic: %.8s version:%u header_bytes:%u record_bytes:%u\n",
           rf->h.magic, rf->h.version, rf->h.header_bytes, rf->h.record_bytes);
    printf("  steps:%" PRIu64 " source_rules:%" PRIu64 " source_words:%" PRIu64
           " source_n:%" PRIu64 " x_min:%.17g x_max:%.17g dx:%.17g payload_bytes:%" PRIu64
           " flags:0x%" PRIx64 " selector_flags:0x%x\n",
           rf->h.step_count, rf->h.source_rule_count, rf->h.source_word_count,
           rf->h.source_n, rf->h.x_min, rf->h.x_max, rf->h.dx,
           rf->h.payload_bytes, rf->h.flags, rf->h.selector_flags);
}

static int build_run(const run_opts *opts) {
    kgram_file gf;
    int rc = read_kgram_payload(opts->gram_path, &gf);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_run: cannot read '%s': error %d\n", opts->gram_path, rc);
        return KDNA_RUN_RUNTIME;
    }

    size_t matches = 0u;
    for (uint64_t i = 0u; i < gf.h.rule_count; ++i) {
        if (rule_matches(&gf.rules[i], opts)) ++matches;
    }
    if (opts->max_steps > 0u && matches > opts->max_steps) matches = opts->max_steps;

    kdna_krun_step_record *steps = NULL;
    if (matches > 0u) {
        steps = (kdna_krun_step_record *)calloc(matches, sizeof(kdna_krun_step_record));
        if (!steps) {
            free_kgram(&gf);
            fprintf(stderr, "kdna_run: allocation failed\n");
            return KDNA_RUN_RUNTIME;
        }
    }

    size_t w = 0u;
    for (uint64_t i = 0u; i < gf.h.rule_count && w < matches; ++i) {
        if (rule_matches(&gf.rules[i], opts)) {
            rc = kdna_krun_build_step_from_rule(&gf.rules[i], (uint64_t)w, &steps[w]);
            if (rc != KDNA_OK) break;
            ++w;
        }
    }

    kdna_krun_header rh;
    if (rc == KDNA_OK) {
        rc = kdna_krun_init_header(&rh, matches, (size_t)gf.h.rule_count, (size_t)gf.h.source_word_count,
                                   (size_t)gf.h.source_n, gf.h.x_min, gf.h.x_max, gf.h.dx,
                                   selector_flags_from_opts(opts));
    }
    if (rc == KDNA_OK) rc = kdna_krun_write_file(opts->out_path, &rh, steps);

    if (rc != KDNA_OK) {
        free(steps);
        free_kgram(&gf);
        fprintf(stderr, "kdna_run: failed to build run: error %d\n", rc);
        return KDNA_RUN_RUNTIME;
    }

    printf("kdna_run: wrote %s source=%s steps=%zu source_rules=%" PRIu64
           " record_bytes=%u payload_bytes=%" PRIu64 "\n",
           opts->out_path, opts->gram_path, matches, gf.h.rule_count,
           KDNA_KRUN_RECORD_BYTES, rh.payload_bytes);

    krun_file rf;
    memset(&rf, 0, sizeof(rf));
    rf.path = opts->out_path;
    rf.h = rh;
    rf.steps = steps;
    print_krun_summary(&rf);

    size_t shown = 0u;
    for (size_t i = 0u; i < matches && shown < opts->top; ++i) {
        print_step(&steps[i]);
        ++shown;
    }
    printf("plan: steps=%zu shown=%zu\n", matches, shown);

    free(steps);
    free_kgram(&gf);
    return KDNA_RUN_OK;
}

static int inspect_run(const run_opts *opts) {
    krun_file rf;
    int rc = read_krun_payload(opts->run_path, &rf);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_run: cannot read '%s': error %d\n", opts->run_path, rc);
        return KDNA_RUN_RUNTIME;
    }

    print_krun_summary(&rf);

    uint64_t causal = 0u;
    uint64_t membrane = 0u;
    uint64_t compression = 0u;
    uint64_t cascade = 0u;
    double total_gain = 0.0;
    double total_duration = 0.0;
    double max_strength = 0.0;
    uint64_t max_strength_step = 0u;

    for (uint64_t i = 0u; i < rf.h.step_count; ++i) {
        const kdna_krun_step_record *s = &rf.steps[i];
        if (s->flags & KDNA_KRUN_STEP_FLAG_CAUSAL_CANDIDATE) ++causal;
        if (s->action == KDNA_KRUN_ACTION_MEMBRANE_CROSS) ++membrane;
        if (s->action == KDNA_KRUN_ACTION_COMPRESSION_GATE) ++compression;
        if (s->action == KDNA_KRUN_ACTION_CASCADE) ++cascade;
        total_gain += s->gain;
        total_duration += s->duration;
        if (s->strength > max_strength) {
            max_strength = s->strength;
            max_strength_step = s->id;
        }
    }

    printf("summary: causal_steps:%" PRIu64 " membrane:%" PRIu64
           " compression:%" PRIu64 " cascade:%" PRIu64
           " total_gain:%.17g total_duration:%.17g max_strength:%.17g step:%" PRIu64 "\n",
           causal, membrane, compression, cascade,
           total_gain, total_duration, max_strength, max_strength_step);

    size_t shown = 0u;
    for (uint64_t i = 0u; i < rf.h.step_count && shown < opts->top; ++i) {
        print_step(&rf.steps[i]);
        ++shown;
    }
    printf("plan: steps=%" PRIu64 " shown=%zu\n", rf.h.step_count, shown);

    free_krun(&rf);
    return KDNA_RUN_OK;
}

static int parse_effect_arg(const char *s, int *out) {
    uint32_t e = 0u;
    if (kdna_effect_class_from_name(s, &e) == KDNA_OK) {
        *out = (int)e;
        return 1;
    }
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end != s && *end == '\0' && v >= 0 && v <= 8) {
        *out = (int)v;
        return 1;
    }
    return 0;
}

static int parse_args(int argc, char **argv, run_opts *opts) {
    memset(opts, 0, sizeof(*opts));
    opts->top = 32u;
    opts->max_steps = 0u;
    opts->from_effect = -1;
    opts->to_effect = -1;
    opts->from_dom = 0;
    opts->to_dom = 0;
    opts->min_strength = 0.0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--gram") == 0 && i + 1 < argc) opts->gram_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) opts->out_path = argv[++i];
        else if (strcmp(argv[i], "--run") == 0 && i + 1 < argc) opts->run_path = argv[++i];
        else if (strcmp(argv[i], "--top") == 0 && i + 1 < argc) {
            if (!parse_size_arg(argv[++i], &opts->top)) return 0;
        } else if (strcmp(argv[i], "--max-steps") == 0 && i + 1 < argc) {
            if (!parse_size_arg(argv[++i], &opts->max_steps)) return 0;
        } else if (strcmp(argv[i], "--changes") == 0) opts->only_changes = 1;
        else if (strcmp(argv[i], "--causal") == 0) opts->only_causal = 1;
        else if (strcmp(argv[i], "--from-effect") == 0 && i + 1 < argc) {
            if (!parse_effect_arg(argv[++i], &opts->from_effect)) return 0;
        } else if (strcmp(argv[i], "--to-effect") == 0 && i + 1 < argc) {
            if (!parse_effect_arg(argv[++i], &opts->to_effect)) return 0;
        } else if (strcmp(argv[i], "--from-dom") == 0 && i + 1 < argc) {
            if (!parse_operator(argv[++i], &opts->from_dom)) return 0;
        } else if (strcmp(argv[i], "--to-dom") == 0 && i + 1 < argc) {
            if (!parse_operator(argv[++i], &opts->to_dom)) return 0;
        } else if (strcmp(argv[i], "--min-strength") == 0 && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &opts->min_strength)) return 0;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(stdout);
            exit(KDNA_RUN_OK);
        } else {
            return 0;
        }
    }

    const int build_mode = opts->gram_path && opts->out_path && !opts->run_path;
    const int inspect_mode = opts->run_path && !opts->gram_path && !opts->out_path;
    return build_mode || inspect_mode;
}

int main(int argc, char **argv) {
    run_opts opts;
    if (!parse_args(argc, argv, &opts)) {
        usage(stderr);
        return KDNA_RUN_USAGE;
    }

    if (opts.run_path) return inspect_run(&opts);
    return build_run(&opts);
}
