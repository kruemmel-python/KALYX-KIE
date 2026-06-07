#include "kdna_krun.h"
#include "kdna_ksoa.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static double absd(double x) {
    return x < 0.0 ? -x : x;
}

const char *kdna_krun_action_name(uint32_t action) {
    switch (action) {
        case KDNA_KRUN_ACTION_HOLD: return "hold";
        case KDNA_KRUN_ACTION_TRANSITION: return "transition";
        case KDNA_KRUN_ACTION_MEMBRANE_CROSS: return "membrane_cross";
        case KDNA_KRUN_ACTION_COMPRESSION_GATE: return "compression_gate";
        case KDNA_KRUN_ACTION_CASCADE: return "cascade";
        case KDNA_KRUN_ACTION_STABILIZE: return "stabilize";
        default: return "unknown";
    }
}

int kdna_krun_payload_bytes(size_t step_count, uint64_t *bytes_out) {
    if (!bytes_out) return KDNA_EINVAL;
    if (step_count == 0u) {
        *bytes_out = 0u;
        return KDNA_OK;
    }
    if (step_count > ((size_t)-1) / sizeof(kdna_krun_step_record)) return KDNA_EINVAL;
    const size_t bytes = step_count * sizeof(kdna_krun_step_record);
    if ((uint64_t)bytes != (uint64_t)step_count * (uint64_t)sizeof(kdna_krun_step_record)) return KDNA_EINVAL;
    *bytes_out = (uint64_t)bytes;
    return KDNA_OK;
}

int kdna_krun_init_header(kdna_krun_header *h,
                          size_t step_count,
                          size_t source_rule_count,
                          size_t source_word_count,
                          size_t source_n,
                          double x_min,
                          double x_max,
                          double dx,
                          uint32_t selector_flags) {
    if (!h || source_word_count == 0u || source_n == 0u) return KDNA_EINVAL;
    if (source_rule_count > 0u && source_rule_count != source_word_count - 1u) return KDNA_EINVAL;
    if (step_count > source_rule_count) return KDNA_EINVAL;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    uint64_t payload_bytes = 0u;
    int rc = kdna_krun_payload_bytes(step_count, &payload_bytes);
    if (rc != KDNA_OK) return rc;

    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KRUN_MAGIC, 8u);
    h->version = KDNA_KRUN_VERSION;
    h->header_bytes = KDNA_KRUN_HEADER_BYTES;
    h->record_bytes = KDNA_KRUN_RECORD_BYTES;
    h->step_count = (uint64_t)step_count;
    h->source_rule_count = (uint64_t)source_rule_count;
    h->source_word_count = (uint64_t)source_word_count;
    h->source_n = (uint64_t)source_n;
    h->x_min = x_min;
    h->x_max = x_max;
    h->dx = dx;
    h->payload_bytes = payload_bytes;
    h->flags = KDNA_KRUN_FLAG_LE_IEEE754_DOUBLE |
               KDNA_KRUN_FLAG_SOURCE_KGRAM_V1 |
               KDNA_KRUN_FLAG_DETERMINISTIC |
               KDNA_KRUN_FLAG_RULE_ORDER;
    h->selector_flags = selector_flags;
    return KDNA_OK;
}

int kdna_krun_validate_header(const kdna_krun_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KRUN_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KRUN_VERSION) return KDNA_EINVAL;
    if (h->header_bytes != KDNA_KRUN_HEADER_BYTES) return KDNA_EINVAL;
    if (h->record_bytes != KDNA_KRUN_RECORD_BYTES) return KDNA_EINVAL;
    if (h->source_word_count == 0u || h->source_n == 0u) return KDNA_EINVAL;
    if (h->source_rule_count > 0u && h->source_rule_count != h->source_word_count - 1u) return KDNA_EINVAL;
    if (h->step_count > h->source_rule_count) return KDNA_EINVAL;
    if (h->payload_bytes != h->step_count * (uint64_t)sizeof(kdna_krun_step_record)) return KDNA_EINVAL;
    if ((h->flags & KDNA_KRUN_FLAG_LE_IEEE754_DOUBLE) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KRUN_FLAG_SOURCE_KGRAM_V1) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KRUN_FLAG_DETERMINISTIC) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KRUN_FLAG_RULE_ORDER) == 0u) return KDNA_EINVAL;
    if (!isfinite(h->x_min) || !isfinite(h->x_max) || !isfinite(h->dx)) return KDNA_EINVAL;
    return KDNA_OK;
}

int kdna_krun_write_file(const char *path,
                         const kdna_krun_header *h,
                         const kdna_krun_step_record *steps) {
    if (!path || !h) return KDNA_EINVAL;
    int rc = kdna_krun_validate_header(h);
    if (rc != KDNA_OK) return rc;
    if (h->step_count > 0u && !steps) return KDNA_EINVAL;

    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;

    int ok = 1;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) ok = 0;
    if (ok && h->step_count > 0u &&
        fwrite(steps, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) {
        ok = 0;
    }
    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

int kdna_krun_read_header_file(const char *path, kdna_krun_header *h) {
    if (!path || !h) return KDNA_EINVAL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    const size_t got = fread(h, 1u, sizeof(*h), f);
    const int close_ok = fclose(f) == 0;
    if (got != sizeof(*h) || !close_ok) return KDNA_EIO;
    return kdna_krun_validate_header(h);
}

static uint32_t step_flags_from_rule(const kdna_krule_record *r) {
    uint32_t flags = 0u;
    if (r->flags & KDNA_KRULE_FLAG_RAW_CHANGE) flags |= KDNA_KRUN_STEP_FLAG_RAW_CHANGE;
    if (r->flags & KDNA_KRULE_FLAG_DOM_CHANGE) flags |= KDNA_KRUN_STEP_FLAG_DOM_CHANGE;
    if (r->flags & KDNA_KRULE_FLAG_EFFECT_CHANGE) flags |= KDNA_KRUN_STEP_FLAG_EFFECT_CHANGE;
    if (r->flags & KDNA_KRULE_FLAG_CROSSES_ZERO) flags |= KDNA_KRUN_STEP_FLAG_CROSSES_ZERO;
    if (r->flags & KDNA_KRULE_FLAG_HIGH_LOCK_BRIDGE) flags |= KDNA_KRUN_STEP_FLAG_HIGH_LOCK;
    if (r->flags & KDNA_KRULE_FLAG_SCORE_RISE) flags |= KDNA_KRUN_STEP_FLAG_SCORE_RISE;
    if (r->flags & KDNA_KRULE_FLAG_SCORE_FALL) flags |= KDNA_KRUN_STEP_FLAG_SCORE_FALL;
    if (r->flags & (KDNA_KRULE_FLAG_NULL_TO_COMPRESSION |
                    KDNA_KRULE_FLAG_COMPRESSION_TO_NULL |
                    KDNA_KRULE_FLAG_CROSSES_ZERO |
                    KDNA_KRULE_FLAG_RAW_DOM_REWIRE)) {
        flags |= KDNA_KRUN_STEP_FLAG_CAUSAL_CANDIDATE;
    }
    return flags;
}

static uint32_t action_from_rule(const kdna_krule_record *r) {
    if (r->flags & KDNA_KRULE_FLAG_CROSSES_ZERO) return KDNA_KRUN_ACTION_MEMBRANE_CROSS;
    if (r->to_effect == KDNA_EFFECT_COMPRESSION_GATE) return KDNA_KRUN_ACTION_COMPRESSION_GATE;
    if (r->to_effect == KDNA_EFFECT_CASCADE_BAND) return KDNA_KRUN_ACTION_CASCADE;
    if (r->to_effect == KDNA_EFFECT_STABLE_ATTRACTOR) return KDNA_KRUN_ACTION_STABILIZE;
    if (r->flags & (KDNA_KRULE_FLAG_RAW_CHANGE | KDNA_KRULE_FLAG_DOM_CHANGE | KDNA_KRULE_FLAG_EFFECT_CHANGE)) {
        return KDNA_KRUN_ACTION_TRANSITION;
    }
    return KDNA_KRUN_ACTION_HOLD;
}

int kdna_krun_build_step_from_rule(const kdna_krule_record *rule,
                                   uint64_t step_index,
                                   kdna_krun_step_record *step) {
    if (!rule || !step) return KDNA_EINVAL;
    memset(step, 0, sizeof(*step));

    step->id = step_index + 1u;
    step->step_index = step_index;
    step->source_rule_id = rule->id;
    step->from_word_id = rule->from_id;
    step->to_word_id = rule->to_id;

    step->from_i0 = rule->from_i0;
    step->from_i1 = rule->from_i1;
    step->to_i0 = rule->to_i0;
    step->to_i1 = rule->to_i1;

    step->x_start = rule->from_x0;
    step->x_boundary = rule->boundary_x;
    step->x_end = rule->to_x1;
    step->gap_x = rule->gap_x;

    step->from_raw = rule->from_raw;
    step->from_dom = rule->from_dom;
    step->from_effect = rule->from_effect;
    step->to_raw = rule->to_raw;
    step->to_dom = rule->to_dom;
    step->to_effect = rule->to_effect;
    step->flags = step_flags_from_rule(rule);
    step->action = action_from_rule(rule);

    step->lock_start = rule->from_lock_mean;
    step->lock_end = rule->to_lock_mean;
    step->score_start = rule->from_score_mean;
    step->score_end = rule->to_score_mean;
    step->delta_lock = rule->delta_lock_mean;
    step->delta_score = rule->delta_score_mean;
    step->strength = rule->strength;

    step->drive_x = rule->boundary_x;
    step->gain = isfinite(rule->strength) ? log1p(absd(rule->strength)) : 0.0;
    step->bias = 0.5 * (rule->from_lock_mean + rule->to_lock_mean);
    step->duration = absd(rule->to_x1 - rule->from_x0);
    if (!isfinite(step->gain)) step->gain = 0.0;
    if (!isfinite(step->bias)) step->bias = 0.0;
    if (!isfinite(step->duration)) step->duration = 0.0;
    return KDNA_OK;
}
