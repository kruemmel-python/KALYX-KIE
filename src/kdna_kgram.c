#include "kdna_kgram.h"
#include "kdna_ksoa.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static double dabs_local(double x) {
    return x < 0.0 ? -x : x;
}

int kdna_kgram_payload_bytes(size_t rule_count, uint64_t *bytes_out) {
    if (!bytes_out) return KDNA_EINVAL;
    if (rule_count == 0u) {
        *bytes_out = 0u;
        return KDNA_OK;
    }
    if (rule_count > ((size_t)-1) / sizeof(kdna_krule_record)) return KDNA_EINVAL;
    const size_t bytes = rule_count * sizeof(kdna_krule_record);
    if ((uint64_t)bytes != (uint64_t)rule_count * (uint64_t)sizeof(kdna_krule_record)) return KDNA_EINVAL;
    *bytes_out = (uint64_t)bytes;
    return KDNA_OK;
}

int kdna_kgram_init_header(kdna_kgram_header *h,
                           size_t rule_count,
                           size_t source_word_count,
                           size_t source_n,
                           double x_min,
                           double x_max,
                           double dx) {
    if (!h || source_word_count == 0u || source_n == 0u) return KDNA_EINVAL;
    if (source_word_count > 1u && rule_count != source_word_count - 1u) return KDNA_EINVAL;
    if (source_word_count == 1u && rule_count != 0u) return KDNA_EINVAL;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    uint64_t payload_bytes = 0u;
    int rc = kdna_kgram_payload_bytes(rule_count, &payload_bytes);
    if (rc != KDNA_OK) return rc;

    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KGRAM_MAGIC, 8u);
    h->version = KDNA_KGRAM_VERSION;
    h->header_bytes = KDNA_KGRAM_HEADER_BYTES;
    h->record_bytes = KDNA_KGRAM_RECORD_BYTES;
    h->rule_count = (uint64_t)rule_count;
    h->source_word_count = (uint64_t)source_word_count;
    h->source_n = (uint64_t)source_n;
    h->x_min = x_min;
    h->x_max = x_max;
    h->dx = dx;
    h->payload_bytes = payload_bytes;
    h->flags = KDNA_KGRAM_FLAG_LE_IEEE754_DOUBLE |
               KDNA_KGRAM_FLAG_SOURCE_KLIB_V1 |
               KDNA_KGRAM_FLAG_ADJACENT_ONLY;
    return KDNA_OK;
}

int kdna_kgram_validate_header(const kdna_kgram_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KGRAM_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KGRAM_VERSION) return KDNA_EINVAL;
    if (h->header_bytes != KDNA_KGRAM_HEADER_BYTES) return KDNA_EINVAL;
    if (h->record_bytes != KDNA_KGRAM_RECORD_BYTES) return KDNA_EINVAL;
    if (h->source_word_count == 0u || h->source_n == 0u) return KDNA_EINVAL;
    if (h->source_word_count > 1u && h->rule_count != h->source_word_count - 1u) return KDNA_EINVAL;
    if (h->source_word_count == 1u && h->rule_count != 0u) return KDNA_EINVAL;
    if (h->payload_bytes != h->rule_count * (uint64_t)sizeof(kdna_krule_record)) return KDNA_EINVAL;
    if ((h->flags & KDNA_KGRAM_FLAG_LE_IEEE754_DOUBLE) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KGRAM_FLAG_SOURCE_KLIB_V1) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KGRAM_FLAG_ADJACENT_ONLY) == 0u) return KDNA_EINVAL;
    if (!isfinite(h->x_min) || !isfinite(h->x_max) || !isfinite(h->dx)) return KDNA_EINVAL;
    return KDNA_OK;
}

int kdna_kgram_write_file(const char *path,
                          const kdna_kgram_header *h,
                          const kdna_krule_record *records) {
    if (!path || !h) return KDNA_EINVAL;
    int rc = kdna_kgram_validate_header(h);
    if (rc != KDNA_OK) return rc;
    if (h->rule_count > 0u && !records) return KDNA_EINVAL;

    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;

    int ok = 1;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) ok = 0;
    if (ok && h->rule_count > 0u &&
        fwrite(records, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) {
        ok = 0;
    }
    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

int kdna_kgram_read_header_file(const char *path, kdna_kgram_header *h) {
    if (!path || !h) return KDNA_EINVAL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    const size_t got = fread(h, 1u, sizeof(*h), f);
    const int close_ok = fclose(f) == 0;
    if (got != sizeof(*h) || !close_ok) return KDNA_EIO;
    return kdna_kgram_validate_header(h);
}

static uint32_t rule_flags_from_words(const kdna_kword_record *a, const kdna_kword_record *b) {
    uint32_t flags = 0u;
    if (a->raw != b->raw) flags |= KDNA_KRULE_FLAG_RAW_CHANGE;
    if (a->dom != b->dom) flags |= KDNA_KRULE_FLAG_DOM_CHANGE;
    if (a->effect_class != b->effect_class) flags |= KDNA_KRULE_FLAG_EFFECT_CHANGE;
    if ((a->x0 <= 0.0 && b->x1 >= 0.0) || (a->x1 < 0.0 && b->x0 > 0.0)) flags |= KDNA_KRULE_FLAG_CROSSES_ZERO;
    if (a->effect_class == KDNA_EFFECT_NULL_MEMBRANE_JUMP && b->effect_class == KDNA_EFFECT_COMPRESSION_GATE) flags |= KDNA_KRULE_FLAG_NULL_TO_COMPRESSION;
    if (a->effect_class == KDNA_EFFECT_COMPRESSION_GATE && b->effect_class == KDNA_EFFECT_NULL_MEMBRANE_JUMP) flags |= KDNA_KRULE_FLAG_COMPRESSION_TO_NULL;
    if (a->lock_mean >= 0.99 && b->lock_mean >= 0.99) flags |= KDNA_KRULE_FLAG_HIGH_LOCK_BRIDGE;
    if (b->score_mean > a->score_mean) flags |= KDNA_KRULE_FLAG_SCORE_RISE;
    if (b->score_mean < a->score_mean) flags |= KDNA_KRULE_FLAG_SCORE_FALL;
    if ((a->raw != a->dom) != (b->raw != b->dom)) flags |= KDNA_KRULE_FLAG_RAW_DOM_REWIRE;
    return flags;
}

static double rule_strength(const kdna_kword_record *a, const kdna_kword_record *b, uint32_t flags) {
    double s = dabs_local(b->lock_mean - a->lock_mean);
    s += log1p(dabs_local(b->score_mean - a->score_mean));
    if (flags & KDNA_KRULE_FLAG_RAW_CHANGE) s += 0.5;
    if (flags & KDNA_KRULE_FLAG_DOM_CHANGE) s += 0.75;
    if (flags & KDNA_KRULE_FLAG_EFFECT_CHANGE) s += 1.0;
    if (flags & KDNA_KRULE_FLAG_CROSSES_ZERO) s += 2.0;
    if (flags & KDNA_KRULE_FLAG_NULL_TO_COMPRESSION) s += 1.5;
    if (flags & KDNA_KRULE_FLAG_COMPRESSION_TO_NULL) s += 1.5;
    if (flags & KDNA_KRULE_FLAG_RAW_DOM_REWIRE) s += 0.5;
    return isfinite(s) ? s : 0.0;
}

int kdna_kgram_build_rules(const kdna_klib_header *lib_header,
                           const kdna_kword_record *words,
                           kdna_krule_record *rules,
                           size_t rule_capacity) {
    if (!lib_header || !words) return KDNA_EINVAL;
    int rc = kdna_klib_validate_header(lib_header);
    if (rc != KDNA_OK) return rc;
    const size_t word_count = (size_t)lib_header->word_count;
    const size_t rule_count = word_count > 0u ? word_count - 1u : 0u;
    if (rule_count > 0u && !rules) return KDNA_EINVAL;
    if (rule_capacity < rule_count) return KDNA_EINVAL;

    for (size_t i = 0u; i < rule_count; ++i) {
        const kdna_kword_record *a = &words[i];
        const kdna_kword_record *b = &words[i + 1u];
        kdna_krule_record *r = &rules[i];
        memset(r, 0, sizeof(*r));

        const uint32_t flags = rule_flags_from_words(a, b);

        r->id = (uint64_t)i + 1u;
        r->sequence_index = (uint64_t)i;
        r->from_id = a->id;
        r->to_id = b->id;
        r->from_segment_index = a->source_segment_index;
        r->to_segment_index = b->source_segment_index;
        r->from_i0 = a->i0;
        r->from_i1 = a->i1;
        r->to_i0 = b->i0;
        r->to_i1 = b->i1;
        r->from_x0 = a->x0;
        r->from_x1 = a->x1;
        r->to_x0 = b->x0;
        r->to_x1 = b->x1;
        r->from_raw = a->raw;
        r->from_dom = a->dom;
        r->from_effect = a->effect_class;
        r->to_raw = b->raw;
        r->to_dom = b->dom;
        r->to_effect = b->effect_class;
        r->flags = flags;
        r->boundary_x = 0.5 * (a->x1 + b->x0);
        r->gap_x = b->x0 - a->x1;
        r->delta_lock_mean = b->lock_mean - a->lock_mean;
        r->delta_score_mean = b->score_mean - a->score_mean;
        r->strength = rule_strength(a, b, flags);
        r->from_lock_mean = a->lock_mean;
        r->to_lock_mean = b->lock_mean;
        r->from_score_mean = a->score_mean;
        r->to_score_mean = b->score_mean;
        r->from_width = a->width;
        r->to_width = b->width;
    }

    return KDNA_OK;
}

const char *kdna_krule_flag_name(uint32_t flag) {
    switch (flag) {
        case KDNA_KRULE_FLAG_RAW_CHANGE: return "raw_change";
        case KDNA_KRULE_FLAG_DOM_CHANGE: return "dom_change";
        case KDNA_KRULE_FLAG_EFFECT_CHANGE: return "effect_change";
        case KDNA_KRULE_FLAG_CROSSES_ZERO: return "crosses_zero";
        case KDNA_KRULE_FLAG_NULL_TO_COMPRESSION: return "null_to_compression";
        case KDNA_KRULE_FLAG_COMPRESSION_TO_NULL: return "compression_to_null";
        case KDNA_KRULE_FLAG_HIGH_LOCK_BRIDGE: return "high_lock_bridge";
        case KDNA_KRULE_FLAG_SCORE_RISE: return "score_rise";
        case KDNA_KRULE_FLAG_SCORE_FALL: return "score_fall";
        case KDNA_KRULE_FLAG_RAW_DOM_REWIRE: return "raw_dom_rewire";
        default: return "unknown";
    }
}
