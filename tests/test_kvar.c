#include "kdna_kvar.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int fail(const char *msg) {
    fprintf(stderr, "test_kvar: %s\n", msg);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_kvar <file.kvar> <expected_source_n>\n");
        return 2;
    }

    const uint64_t expected_n = (uint64_t)strtoull(argv[2], NULL, 10);

    kdna_kvar_header h;
    kdna_kvar_record *records = NULL;
    int rc = kdna_kvar_read_file(argv[1], &h, &records);
    if (rc != KDNA_OK) {
        fprintf(stderr, "read failed: %s\n", kdna_status_str(rc));
        return 1;
    }

    if (h.source_n != expected_n) {
        free(records);
        return fail("source_n mismatch");
    }
    if (h.variant_count == 0u) {
        free(records);
        return fail("variant_count zero");
    }
    if (h.payload_bytes != h.variant_count * (uint64_t)sizeof(kdna_kvar_record)) {
        free(records);
        return fail("payload mismatch");
    }
    if (h.record_bytes != KDNA_KVAR_RECORD_BYTES) {
        free(records);
        return fail("record bytes mismatch");
    }

    uint64_t count_sum = 0u;
    uint64_t resonant = 0u;
    uint64_t repeated = 0u;

    for (uint64_t i = 0u; i < h.variant_count; ++i) {
        const kdna_kvar_record *r = &records[i];
        if (r->variant_id == 0u) {
            free(records);
            return fail("zero variant id");
        }
        if (i > 0u && records[i - 1u].variant_id >= r->variant_id) {
            free(records);
            return fail("variant ids not strictly sorted");
        }
        if (r->sample_count == 0u) {
            free(records);
            return fail("zero sample count");
        }
        count_sum += r->sample_count;

        if (r->raw < 1u || r->raw > 5u || r->dom < 1u || r->dom > 5u) {
            free(records);
            return fail("bad raw/dom");
        }
        if (r->first_i > r->last_i || r->last_i >= h.source_n) {
            free(records);
            return fail("bad index span");
        }
        if (!isfinite(r->x_min) || !isfinite(r->x_max) || !isfinite(r->x_mean) || r->x_min > r->x_max) {
            free(records);
            return fail("bad x stats");
        }
        if (!isfinite(r->resonance_min) || !isfinite(r->resonance_max) || !isfinite(r->resonance_mean) ||
            r->resonance_min > r->resonance_max) {
            free(records);
            return fail("bad resonance stats");
        }
        if (!isfinite(r->injection_mean) || !isfinite(r->injection_max) ||
            !isfinite(r->stability_min) || !isfinite(r->stability_max) || !isfinite(r->stability_mean) ||
            !isfinite(r->alignment_mean) || !isfinite(r->lock_mean) || !isfinite(r->score_mean)) {
            free(records);
            return fail("nonfinite aggregate field");
        }
        for (size_t op = 0u; op < 5u; ++op) {
            if (!isfinite(r->k_mean[op]) || !isfinite(r->k_absmax[op]) ||
                !isfinite(r->s_mean[op]) || !isfinite(r->s_absmax[op])) {
                free(records);
                return fail("nonfinite K/S aggregate");
            }
        }
        if (r->flags & KDNA_KVAR_REC_FLAG_RESONANT) resonant++;
        if (r->flags & KDNA_KVAR_REC_FLAG_REPEATED) repeated++;
    }

    if (count_sum != h.source_n) {
        free(records);
        return fail("sample count sum mismatch");
    }
    if (resonant == 0u) {
        free(records);
        return fail("expected at least one resonant variant");
    }

    printf("test_kvar: ok variants=%" PRIu64 " source_n=%" PRIu64 " resonant=%" PRIu64 " repeated=%" PRIu64 "\n",
           h.variant_count, h.source_n, resonant, repeated);

    free(records);
    return 0;
}
