#include "kdna_krun.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_krun <file.krun> <expected_source_n>\n");
        return 2;
    }

    const char *path = argv[1];
    const uint64_t expected_source_n = (uint64_t)strtoull(argv[2], NULL, 10);

    kdna_krun_header h;
    int rc = kdna_krun_read_header_file(path, &h);
    if (rc != KDNA_OK) {
        fprintf(stderr, "header invalid: %d\n", rc);
        return 1;
    }

    if (h.source_n != expected_source_n) {
        fprintf(stderr, "source_n mismatch: got %" PRIu64 " expected %" PRIu64 "\n",
                h.source_n, expected_source_n);
        return 1;
    }
    if (h.source_rule_count == 0u || h.source_rule_count != h.source_word_count - 1u) {
        fprintf(stderr, "invalid source_rule_count: %" PRIu64 " words:%" PRIu64 "\n",
                h.source_rule_count, h.source_word_count);
        return 1;
    }
    if (h.step_count == 0u || h.step_count > h.source_rule_count) {
        fprintf(stderr, "invalid step_count: %" PRIu64 "\n", h.step_count);
        return 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open file\n");
        return 1;
    }
    if (fseek(f, (long)h.header_bytes, SEEK_SET) != 0) {
        fclose(f);
        fprintf(stderr, "seek failed\n");
        return 1;
    }

    kdna_krun_step_record *steps = (kdna_krun_step_record *)calloc((size_t)h.step_count, sizeof(kdna_krun_step_record));
    if (!steps) {
        fclose(f);
        fprintf(stderr, "alloc failed\n");
        return 1;
    }
    if (!read_exact(f, steps, (size_t)h.payload_bytes)) {
        free(steps);
        fclose(f);
        fprintf(stderr, "payload read failed\n");
        return 1;
    }
    fclose(f);

    uint64_t causal = 0u;
    uint64_t prev_rule = 0u;
    for (uint64_t i = 0u; i < h.step_count; ++i) {
        const kdna_krun_step_record *s = &steps[i];
        if (s->id != i + 1u || s->step_index != i) {
            fprintf(stderr, "step identity mismatch at %" PRIu64 "\n", i);
            free(steps);
            return 1;
        }
        if (s->source_rule_id == 0u || s->source_rule_id > h.source_rule_count) {
            fprintf(stderr, "bad source_rule_id at %" PRIu64 "\n", i);
            free(steps);
            return 1;
        }
        if (s->source_rule_id <= prev_rule && i > 0u) {
            fprintf(stderr, "non-monotonic source_rule_id at %" PRIu64 "\n", i);
            free(steps);
            return 1;
        }
        prev_rule = s->source_rule_id;
        if (s->from_word_id == 0u || s->to_word_id == 0u) {
            fprintf(stderr, "bad word ids at %" PRIu64 "\n", i);
            free(steps);
            return 1;
        }
        if (s->duration < 0.0 || s->gain < 0.0) {
            fprintf(stderr, "bad derived controls at %" PRIu64 "\n", i);
            free(steps);
            return 1;
        }
        if (s->flags & KDNA_KRUN_STEP_FLAG_CAUSAL_CANDIDATE) ++causal;
    }

    if (causal == 0u) {
        fprintf(stderr, "expected at least one causal step\n");
        free(steps);
        return 1;
    }

    free(steps);
    return 0;
}
