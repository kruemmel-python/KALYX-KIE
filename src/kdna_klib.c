#include "kdna_klib.h"

#include <ctype.h>
#include <string.h>

static int streq_ci(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

const char *kdna_effect_class_name(uint32_t effect_class) {
    switch (effect_class) {
        case KDNA_EFFECT_STABLE_ATTRACTOR: return "stable_attractor";
        case KDNA_EFFECT_ENVELOPE_FORM: return "envelope_form";
        case KDNA_EFFECT_PHASE_OFFSET: return "phase_offset";
        case KDNA_EFFECT_COMPRESSION_GATE: return "compression_gate";
        case KDNA_EFFECT_CASCADE_BAND: return "cascade_band";
        case KDNA_EFFECT_NULL_MEMBRANE_JUMP: return "null_membrane_jump";
        case KDNA_EFFECT_RAW_DOM_MISMATCH_ZONE: return "raw_dom_mismatch_zone";
        case KDNA_EFFECT_TRANSITION_BRIDGE: return "transition_bridge";
        case KDNA_EFFECT_UNKNOWN:
        default: return "unknown";
    }
}

int kdna_effect_class_from_name(const char *name, uint32_t *effect_out) {
    if (!name || !effect_out) return KDNA_EINVAL;

    if (streq_ci(name, "unknown") || strcmp(name, "0") == 0) {
        *effect_out = KDNA_EFFECT_UNKNOWN;
    } else if (streq_ci(name, "stable_attractor") || streq_ci(name, "attractor") || strcmp(name, "1") == 0) {
        *effect_out = KDNA_EFFECT_STABLE_ATTRACTOR;
    } else if (streq_ci(name, "envelope_form") || streq_ci(name, "envelope") || strcmp(name, "2") == 0) {
        *effect_out = KDNA_EFFECT_ENVELOPE_FORM;
    } else if (streq_ci(name, "phase_offset") || streq_ci(name, "phase") || strcmp(name, "3") == 0) {
        *effect_out = KDNA_EFFECT_PHASE_OFFSET;
    } else if (streq_ci(name, "compression_gate") || streq_ci(name, "compression") || strcmp(name, "4") == 0) {
        *effect_out = KDNA_EFFECT_COMPRESSION_GATE;
    } else if (streq_ci(name, "cascade_band") || streq_ci(name, "cascade") || strcmp(name, "5") == 0) {
        *effect_out = KDNA_EFFECT_CASCADE_BAND;
    } else if (streq_ci(name, "null_membrane_jump") || streq_ci(name, "null") || streq_ci(name, "membrane") || strcmp(name, "6") == 0) {
        *effect_out = KDNA_EFFECT_NULL_MEMBRANE_JUMP;
    } else if (streq_ci(name, "raw_dom_mismatch_zone") || streq_ci(name, "mismatch") || strcmp(name, "7") == 0) {
        *effect_out = KDNA_EFFECT_RAW_DOM_MISMATCH_ZONE;
    } else if (streq_ci(name, "transition_bridge") || streq_ci(name, "transition") || strcmp(name, "8") == 0) {
        *effect_out = KDNA_EFFECT_TRANSITION_BRIDGE;
    } else {
        return KDNA_EINVAL;
    }

    return KDNA_OK;
}

int kdna_klib_payload_bytes(size_t word_count, uint64_t *bytes_out) {
    if (!bytes_out || word_count == 0u) return KDNA_EINVAL;
    if (word_count > ((size_t)-1) / sizeof(kdna_kword_record)) return KDNA_EINVAL;
    const size_t bytes = word_count * sizeof(kdna_kword_record);
    if ((uint64_t)bytes != (uint64_t)word_count * (uint64_t)sizeof(kdna_kword_record)) return KDNA_EINVAL;
    *bytes_out = (uint64_t)bytes;
    return KDNA_OK;
}

int kdna_klib_init_header(kdna_klib_header *h,
                          size_t word_count,
                          size_t source_n,
                          size_t source_segment_count,
                          double x_min,
                          double x_max,
                          double dx) {
    if (!h || word_count == 0u || source_n == 0u || source_segment_count == 0u) return KDNA_EINVAL;
    if (word_count != source_segment_count) return KDNA_EINVAL;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    uint64_t payload_bytes = 0u;
    int rc = kdna_klib_payload_bytes(word_count, &payload_bytes);
    if (rc != KDNA_OK) return rc;

    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KLIB_MAGIC, 8u);
    h->version = KDNA_KLIB_VERSION;
    h->header_bytes = KDNA_KLIB_HEADER_BYTES;
    h->record_bytes = KDNA_KLIB_RECORD_BYTES;
    h->source_fields = KDNA_FIELDS;
    h->word_count = (uint64_t)word_count;
    h->source_n = (uint64_t)source_n;
    h->source_segment_count = (uint64_t)source_segment_count;
    h->x_min = x_min;
    h->x_max = x_max;
    h->dx = dx;
    h->payload_bytes = payload_bytes;
    h->flags = KDNA_KLIB_FLAG_LE_IEEE754_DOUBLE | KDNA_KLIB_FLAG_SOURCE_KSOA_V1 | KDNA_KLIB_FLAG_SOURCE_KREG_V1;
    return KDNA_OK;
}

int kdna_klib_validate_header(const kdna_klib_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KLIB_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KLIB_VERSION) return KDNA_EINVAL;
    if (h->header_bytes != KDNA_KLIB_HEADER_BYTES) return KDNA_EINVAL;
    if (h->record_bytes != KDNA_KLIB_RECORD_BYTES) return KDNA_EINVAL;
    if (h->source_fields != KDNA_FIELDS) return KDNA_EINVAL;
    if (h->word_count == 0u || h->source_n == 0u || h->source_segment_count == 0u) return KDNA_EINVAL;
    if (h->word_count != h->source_segment_count) return KDNA_EINVAL;
    if ((h->flags & KDNA_KLIB_FLAG_LE_IEEE754_DOUBLE) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KLIB_FLAG_SOURCE_KSOA_V1) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KLIB_FLAG_SOURCE_KREG_V1) == 0u) return KDNA_EINVAL;

    uint64_t expected_payload = 0u;
    int rc = kdna_klib_payload_bytes((size_t)h->word_count, &expected_payload);
    if (rc != KDNA_OK) return rc;
    if (h->payload_bytes != expected_payload) return KDNA_EINVAL;
    return KDNA_OK;
}

static int write_exact(FILE *f, const void *ptr, size_t bytes) {
    return fwrite(ptr, 1u, bytes, f) == bytes;
}

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

int kdna_klib_write_file(const char *path,
                         const kdna_klib_header *h,
                         const kdna_kword_record *records) {
    if (!path || !h || !records) return KDNA_EINVAL;

    int rc = kdna_klib_validate_header(h);
    if (rc != KDNA_OK) return rc;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;

    int ok = write_exact(f, h, sizeof(*h)) &&
             write_exact(f, records, (size_t)h->payload_bytes);
    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

int kdna_klib_read_header_file(const char *path, kdna_klib_header *h) {
    if (!path || !h) return KDNA_EINVAL;

    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;

    const int ok = read_exact(f, h, sizeof(*h));
    const int close_ok = (fclose(f) == 0);
    if (!ok || !close_ok) return KDNA_EIO;

    return kdna_klib_validate_header(h);
}
