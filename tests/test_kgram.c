#include "kdna.h"
#include "kdna_kgram.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

static int fail(const char *msg) {
    fprintf(stderr, "test_kgram: %s\n", msg);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_kgram <grammar.kgram> <expected_source_n>\n");
        return 2;
    }

    char *end = NULL;
    unsigned long long expected_n_ull = strtoull(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0' || expected_n_ull == 0ull) return fail("invalid expected source n");
    const uint64_t expected_n = (uint64_t)expected_n_ull;

    kdna_kgram_header h;
    int rc = kdna_kgram_read_header_file(argv[1], &h);
    if (rc != KDNA_OK) return fail("header validation failed");

    if (h.source_n != expected_n) return fail("source_n mismatch");
    if (h.source_word_count < 2u) return fail("expected at least two source words");
    if (h.rule_count != h.source_word_count - 1u) return fail("rule_count mismatch");
    if (h.record_bytes != KDNA_KGRAM_RECORD_BYTES) return fail("record_bytes mismatch");
    if (h.payload_bytes != h.rule_count * (uint64_t)sizeof(kdna_krule_record)) return fail("payload_bytes mismatch");
    if ((h.flags & KDNA_KGRAM_FLAG_ADJACENT_ONLY) == 0u) return fail("missing adjacent flag");

    kdna_krule_record *rules = NULL;
    if (h.rule_count > 0u) {
        rules = (kdna_krule_record *)calloc((size_t)h.rule_count, sizeof(kdna_krule_record));
        if (!rules) return fail("allocation failed");
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        free(rules);
        return fail("cannot open grammar");
    }
    int ok = 1;
    if (fseek(f, (long)h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && h.rule_count > 0u && !read_exact(f, rules, (size_t)h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        free(rules);
        return fail("cannot read rules");
    }

    uint64_t change_count = 0u;
    uint64_t causal_count = 0u;

    for (uint64_t i = 0u; i < h.rule_count; ++i) {
        const kdna_krule_record *r = &rules[i];

        if (r->id != i + 1u) {
            free(rules);
            return fail("id sequence mismatch");
        }
        if (r->sequence_index != i) {
            free(rules);
            return fail("sequence_index mismatch");
        }
        if (r->from_id + 1u != r->to_id) {
            free(rules);
            return fail("non-adjacent word ids");
        }
        if (r->from_i1 + 1u != r->to_i0) {
            free(rules);
            return fail("source ranges are not contiguous");
        }
        if (r->from_raw < 1u || r->from_raw > 5u || r->to_raw < 1u || r->to_raw > 5u ||
            r->from_dom < 1u || r->from_dom > 5u || r->to_dom < 1u || r->to_dom > 5u) {
            free(rules);
            return fail("operator id out of range");
        }
        if (r->from_effect > KDNA_EFFECT_TRANSITION_BRIDGE || r->to_effect > KDNA_EFFECT_TRANSITION_BRIDGE) {
            free(rules);
            return fail("effect out of range");
        }
        if (!isfinite(r->boundary_x) || !isfinite(r->gap_x) ||
            !isfinite(r->delta_lock_mean) || !isfinite(r->delta_score_mean) || !isfinite(r->strength)) {
            free(rules);
            return fail("nonfinite numeric field");
        }
        if (r->strength < 0.0) {
            free(rules);
            return fail("negative strength");
        }
        if (r->flags & (KDNA_KRULE_FLAG_RAW_CHANGE | KDNA_KRULE_FLAG_DOM_CHANGE | KDNA_KRULE_FLAG_EFFECT_CHANGE)) {
            ++change_count;
        }
        if (r->flags & (KDNA_KRULE_FLAG_NULL_TO_COMPRESSION |
                        KDNA_KRULE_FLAG_COMPRESSION_TO_NULL |
                        KDNA_KRULE_FLAG_CROSSES_ZERO |
                        KDNA_KRULE_FLAG_RAW_DOM_REWIRE)) {
            ++causal_count;
        }
    }

    if (h.rule_count > 0u && change_count == 0u) {
        free(rules);
        return fail("expected at least one changing rule");
    }
    if (h.rule_count > 0u && causal_count == 0u) {
        free(rules);
        return fail("expected at least one causal candidate");
    }

    free(rules);
    return 0;
}
