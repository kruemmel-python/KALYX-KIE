#include "kdna_kvar.h"
#include "kdna_kgram.h"
#include "kdna_klib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_kvar(const char *path, kdna_kvar_header *h, kdna_kvar_record **records) {
    return kdna_kvar_read_file(path, h, records);
}

static const kdna_kvar_record *find_variant(const kdna_kvar_record *r, size_t n, uint64_t id) {
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t m = lo + (hi - lo) / 2u;
        if (r[m].variant_id < id) lo = m + 1u;
        else hi = m;
    }
    if (lo < n && r[lo].variant_id == id) return &r[lo];
    return NULL;
}

static uint32_t effect_from_variant(const kdna_kvar_record *v) {
    if (v->flags & KDNA_KVAR_REC_FLAG_NULL_NEAR) return KDNA_EFFECT_NULL_MEMBRANE_JUMP;
    if (v->flags & KDNA_KVAR_REC_FLAG_RAW_DOM_MISMATCH) return KDNA_EFFECT_RAW_DOM_MISMATCH_ZONE;
    if (v->dom == 4u) return KDNA_EFFECT_COMPRESSION_GATE;
    if (v->dom == 5u) return KDNA_EFFECT_CASCADE_BAND;
    if (v->dom == 2u) return KDNA_EFFECT_ENVELOPE_FORM;
    if (v->dom == 3u) return KDNA_EFFECT_PHASE_OFFSET;
    if (v->flags & KDNA_KVAR_REC_FLAG_HIGH_STABILITY) return KDNA_EFFECT_STABLE_ATTRACTOR;
    return KDNA_EFFECT_TRANSITION_BRIDGE;
}

static uint32_t flags_from_pair(const kdna_kvar_record *a, const kdna_kvar_record *b) {
    uint32_t f = 0u;
    if (a->raw != b->raw) f |= KDNA_KRULE_FLAG_RAW_CHANGE;
    if (a->dom != b->dom) f |= KDNA_KRULE_FLAG_DOM_CHANGE;
    if (effect_from_variant(a) != effect_from_variant(b)) f |= KDNA_KRULE_FLAG_EFFECT_CHANGE;
    if ((a->x_min <= 0.0 && b->x_max >= 0.0) || (a->x_max < 0.0 && b->x_min > 0.0)) f |= KDNA_KRULE_FLAG_CROSSES_ZERO;
    if (effect_from_variant(a) == KDNA_EFFECT_NULL_MEMBRANE_JUMP && effect_from_variant(b) == KDNA_EFFECT_COMPRESSION_GATE) f |= KDNA_KRULE_FLAG_NULL_TO_COMPRESSION;
    if (effect_from_variant(a) == KDNA_EFFECT_COMPRESSION_GATE && effect_from_variant(b) == KDNA_EFFECT_NULL_MEMBRANE_JUMP) f |= KDNA_KRULE_FLAG_COMPRESSION_TO_NULL;
    if (a->lock_mean >= 0.99 && b->lock_mean >= 0.99) f |= KDNA_KRULE_FLAG_HIGH_LOCK_BRIDGE;
    if (b->score_mean > a->score_mean) f |= KDNA_KRULE_FLAG_SCORE_RISE;
    if (b->score_mean < a->score_mean) f |= KDNA_KRULE_FLAG_SCORE_FALL;
    if ((a->raw != a->dom) != (b->raw != b->dom)) f |= KDNA_KRULE_FLAG_RAW_DOM_REWIRE;
    return f;
}

static double strength_from_pair(const kdna_kvar_record *a, const kdna_kvar_record *b, uint32_t flags) {
    double s = fabs(b->lock_mean - a->lock_mean) + log1p(fabs(b->score_mean - a->score_mean));
    s += log1p((double)a->successor_count);
    if (flags & KDNA_KRULE_FLAG_RAW_CHANGE) s += 0.5;
    if (flags & KDNA_KRULE_FLAG_DOM_CHANGE) s += 0.75;
    if (flags & KDNA_KRULE_FLAG_EFFECT_CHANGE) s += 1.0;
    if (flags & KDNA_KRULE_FLAG_CROSSES_ZERO) s += 2.0;
    if (flags & (KDNA_KRULE_FLAG_NULL_TO_COMPRESSION | KDNA_KRULE_FLAG_COMPRESSION_TO_NULL)) s += 1.5;
    if (!isfinite(s)) s = 0.0;
    return s;
}

static void usage(void) {
    fprintf(stderr, "kdna_variant_gram --kvar variants.kvar --out variant_grammar.kgram [--top n]\n");
}

int main(int argc, char **argv) {
    const char *kvar_path = NULL, *out_path = NULL;
    size_t top = 16u;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kvar") == 0 && i + 1 < argc) kvar_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--top") == 0 && i + 1 < argc) top = (size_t)strtoull(argv[++i], NULL, 10);
        else { usage(); return 2; }
    }
    if (!kvar_path || !out_path) { usage(); return 2; }

    kdna_kvar_header kh;
    kdna_kvar_record *vr = NULL;
    int rc = read_kvar(kvar_path, &kh, &vr);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_variant_gram: cannot read '%s': %s\n", kvar_path, kdna_status_str(rc));
        return 3;
    }

    size_t rule_count = 0u;
    for (size_t i = 0; i < (size_t)kh.variant_count; ++i) {
        if ((vr[i].flags & KDNA_KVAR_REC_FLAG_HAS_SUCCESSOR) && find_variant(vr, (size_t)kh.variant_count, vr[i].successor_id)) rule_count++;
    }
    if (rule_count == 0u) {
        free(vr);
        fprintf(stderr, "kdna_variant_gram: no successor-linked variants\n");
        return 3;
    }

    kdna_krule_record *rules = (kdna_krule_record *)calloc(rule_count, sizeof(kdna_krule_record));
    if (!rules) { free(vr); return 3; }

    size_t w = 0u;
    for (size_t i = 0; i < (size_t)kh.variant_count; ++i) {
        if (!(vr[i].flags & KDNA_KVAR_REC_FLAG_HAS_SUCCESSOR)) continue;
        const kdna_kvar_record *b = find_variant(vr, (size_t)kh.variant_count, vr[i].successor_id);
        if (!b) continue;
        kdna_krule_record *r = &rules[w];
        uint32_t flags = flags_from_pair(&vr[i], b);
        r->id = (uint64_t)w + 1u;
        r->sequence_index = (uint64_t)w;
        r->from_id = vr[i].variant_id;
        r->to_id = b->variant_id;
        r->from_segment_index = (uint64_t)i;
        r->to_segment_index = (uint64_t)(b - vr);
        r->from_i0 = vr[i].first_i;
        r->from_i1 = vr[i].last_i;
        r->to_i0 = b->first_i;
        r->to_i1 = b->last_i;
        r->from_x0 = vr[i].x_min;
        r->from_x1 = vr[i].x_max;
        r->to_x0 = b->x_min;
        r->to_x1 = b->x_max;
        r->from_raw = vr[i].raw;
        r->from_dom = vr[i].dom;
        r->from_effect = effect_from_variant(&vr[i]);
        r->to_raw = b->raw;
        r->to_dom = b->dom;
        r->to_effect = effect_from_variant(b);
        r->flags = flags;
        r->boundary_x = 0.5 * (vr[i].x_mean + b->x_mean);
        r->gap_x = b->x_mean - vr[i].x_mean;
        r->delta_lock_mean = b->lock_mean - vr[i].lock_mean;
        r->delta_score_mean = b->score_mean - vr[i].score_mean;
        r->strength = strength_from_pair(&vr[i], b, flags);
        r->from_lock_mean = vr[i].lock_mean;
        r->to_lock_mean = b->lock_mean;
        r->from_score_mean = vr[i].score_mean;
        r->to_score_mean = b->score_mean;
        r->from_width = vr[i].span_i;
        r->to_width = b->span_i;
        w++;
    }

    kdna_kgram_header gh;
    rc = kdna_kgram_init_header(&gh, rule_count, rule_count + 1u, (size_t)kh.source_n, kh.x_min, kh.x_max,
                                kh.source_n > 1u ? (kh.x_max - kh.x_min) / (double)(kh.source_n - 1u) : 0.0);
    if (rc == KDNA_OK) rc = kdna_kgram_write_file(out_path, &gh, rules);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_variant_gram: cannot write '%s': %s\n", out_path, kdna_status_str(rc));
        free(vr); free(rules); return 3;
    }

    printf("kdna_variant_gram: wrote %s source=%s rules=%zu variant_count=%llu\n",
           out_path, kvar_path, rule_count, (unsigned long long)kh.variant_count);
    for (size_t i = 0; i < rule_count && i < top; ++i) {
        printf("  rule:%llu variant:%llu->%llu RAW:K%u->K%u D:K%u->K%u strength:%.17g flags:0x%x\n",
               (unsigned long long)rules[i].id,
               (unsigned long long)rules[i].from_id, (unsigned long long)rules[i].to_id,
               rules[i].from_raw, rules[i].to_raw, rules[i].from_dom, rules[i].to_dom,
               rules[i].strength, rules[i].flags);
    }
    free(vr); free(rules);
    return 0;
}
