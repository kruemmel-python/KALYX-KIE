#include "kdna.h"
#include "kdna_kreg.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

static int fail(const char *msg) {
    fprintf(stderr, "test_kreg: %s\n", msg);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_kreg <regions.kreg> <expected_source_n>\n");
        return 2;
    }

    char *end = NULL;
    unsigned long long expected_n_ull = strtoull(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0' || expected_n_ull == 0ull) return fail("invalid expected source n");
    const uint64_t expected_n = (uint64_t)expected_n_ull;

    kdna_kreg_header h;
    int rc = kdna_kreg_read_header_file(argv[1], &h);
    if (rc != KDNA_OK) return fail("header validation failed");

    if (h.source_n != expected_n) return fail("source_n mismatch");
    if (h.segment_count == 0u) return fail("segment_count zero");
    if (h.record_bytes != KDNA_KREG_RECORD_BYTES) return fail("record_bytes mismatch");
    if (h.payload_bytes != h.segment_count * (uint64_t)sizeof(kdna_kreg_record)) return fail("payload_bytes mismatch");

    kdna_kreg_record *records = (kdna_kreg_record *)calloc((size_t)h.segment_count, sizeof(kdna_kreg_record));
    if (!records) return fail("allocation failed");

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        free(records);
        return fail("cannot open kreg");
    }
    int ok = 1;
    if (fseek(f, (long)h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && !read_exact(f, records, (size_t)h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        free(records);
        return fail("cannot read records");
    }

    if (records[0].i0 != 0u) {
        free(records);
        return fail("first segment does not start at 0");
    }
    if (records[h.segment_count - 1u].i1 != expected_n - 1u) {
        free(records);
        return fail("last segment does not end at source_n - 1");
    }

    for (uint64_t i = 0u; i < h.segment_count; ++i) {
        const kdna_kreg_record *r = &records[i];
        if (r->i0 > r->i1) {
            free(records);
            return fail("invalid range ordering");
        }
        if (r->raw < 1u || r->raw > 5u || r->dom < 1u || r->dom > 5u) {
            free(records);
            return fail("operator id out of range");
        }
        if (!isfinite(r->x0) || !isfinite(r->x1)) {
            free(records);
            return fail("nonfinite x range");
        }
        if (!isfinite(r->lock_min) || !isfinite(r->lock_max) || r->lock_min > r->lock_max) {
            free(records);
            return fail("invalid lock bounds");
        }
        if (!isfinite(r->score_min) || !isfinite(r->score_max) || r->score_min > r->score_max) {
            free(records);
            return fail("invalid score bounds");
        }
        if (i > 0u && records[i - 1u].i1 + 1u != r->i0) {
            free(records);
            return fail("gap or overlap between segments");
        }
    }

    printf("test_kreg: ok segments=%" PRIu64 " source_n=%" PRIu64 "\n", h.segment_count, h.source_n);
    free(records);
    return 0;
}
