#include "kdna.h"
#include "kdna_klib.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

static int fail(const char *msg) {
    fprintf(stderr, "test_klib: %s\n", msg);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_klib <library.klib> <expected_source_n>\n");
        return 2;
    }

    char *end = NULL;
    unsigned long long expected_n_ull = strtoull(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0' || expected_n_ull == 0ull) return fail("invalid expected source n");
    const uint64_t expected_n = (uint64_t)expected_n_ull;

    kdna_klib_header h;
    int rc = kdna_klib_read_header_file(argv[1], &h);
    if (rc != KDNA_OK) return fail("header validation failed");

    if (h.source_n != expected_n) return fail("source_n mismatch");
    if (h.word_count == 0u) return fail("word_count zero");
    if (h.word_count != h.source_segment_count) return fail("word_count/source_segment_count mismatch");
    if (h.record_bytes != KDNA_KLIB_RECORD_BYTES) return fail("record_bytes mismatch");
    if (h.payload_bytes != h.word_count * (uint64_t)sizeof(kdna_kword_record)) return fail("payload_bytes mismatch");

    kdna_kword_record *words = (kdna_kword_record *)calloc((size_t)h.word_count, sizeof(kdna_kword_record));
    if (!words) return fail("allocation failed");

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        free(words);
        return fail("cannot open klib");
    }
    int ok = 1;
    if (fseek(f, (long)h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && !read_exact(f, words, (size_t)h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;
    if (!ok) {
        free(words);
        return fail("cannot read words");
    }

    if (words[0].i0 != 0u) {
        free(words);
        return fail("first word does not start at 0");
    }
    if (words[h.word_count - 1u].i1 != expected_n - 1u) {
        free(words);
        return fail("last word does not end at source_n - 1");
    }

    uint64_t null_near_count = 0u;
    uint64_t mismatch_count = 0u;
    uint64_t known_effect_count = 0u;

    for (uint64_t i = 0u; i < h.word_count; ++i) {
        const kdna_kword_record *w = &words[i];

        if (w->id != i + 1u) {
            free(words);
            return fail("id sequence mismatch");
        }
        if (w->source_segment_index != i) {
            free(words);
            return fail("source_segment_index mismatch");
        }
        if (w->i0 > w->i1 || w->i1 >= expected_n) {
            free(words);
            return fail("invalid range");
        }
        if (i > 0u && words[i - 1u].i1 + 1u != w->i0) {
            free(words);
            return fail("gap or overlap");
        }
        if (w->sample_count != w->i1 - w->i0 + 1u) {
            free(words);
            return fail("sample_count mismatch");
        }
        if (w->raw < 1u || w->raw > 5u || w->dom < 1u || w->dom > 5u) {
            free(words);
            return fail("operator id out of range");
        }
        if (!isfinite(w->x0) || !isfinite(w->x1) || !isfinite(w->width)) {
            free(words);
            return fail("nonfinite x/width");
        }
        if (!isfinite(w->lock_min) || !isfinite(w->lock_max) || !isfinite(w->lock_mean) ||
            w->lock_min > w->lock_max || w->lock_mean < w->lock_min - 1e-12 || w->lock_mean > w->lock_max + 1e-12) {
            free(words);
            return fail("invalid lock stats");
        }
        if (!isfinite(w->score_min) || !isfinite(w->score_max) || !isfinite(w->score_mean) ||
            w->score_min > w->score_max || w->score_mean < w->score_min - 1e-9 || w->score_mean > w->score_max + 1e-9) {
            free(words);
            return fail("invalid score stats");
        }
        if (w->effect_class > KDNA_EFFECT_TRANSITION_BRIDGE) {
            free(words);
            return fail("effect_class out of range");
        }
        if ((w->flags & KDNA_KWORD_FLAG_NULL_NEAR) != 0u) null_near_count++;
        if ((w->flags & KDNA_KWORD_FLAG_RAW_DOM_MISMATCH) != 0u) mismatch_count++;
        if (w->effect_class != KDNA_EFFECT_UNKNOWN) known_effect_count++;

        for (size_t op = 0u; op < 5u; ++op) {
            if (!isfinite(w->k_mean[op]) || !isfinite(w->k_absmax[op]) ||
                !isfinite(w->s_mean[op]) || !isfinite(w->s_absmax[op])) {
                free(words);
                return fail("nonfinite operator stats");
            }
            if (w->k_absmax[op] < 0.0 || w->s_absmax[op] < 0.0) {
                free(words);
                return fail("negative absmax");
            }
        }
    }

    if (known_effect_count == 0u) {
        free(words);
        return fail("no classified effects");
    }

    printf("test_klib: ok words=%" PRIu64 " source_n=%" PRIu64
           " null_near=%" PRIu64 " mismatch=%" PRIu64 " known_effect=%" PRIu64 "\n",
           h.word_count, h.source_n, null_near_count, mismatch_count, known_effect_count);

    free(words);
    return 0;
}
