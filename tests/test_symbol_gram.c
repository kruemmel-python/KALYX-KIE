
#include "kdna.h"
#include "kdna_kgram.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_symbol_gram <grammar.kgram> <expected_source_n>\n");
        return 2;
    }

    const uint64_t expected_n = (uint64_t)strtoull(argv[2], NULL, 10);
    kdna_kgram_header h;
    int rc = kdna_kgram_read_header_file(argv[1], &h);
    if (rc != KDNA_OK) {
        fprintf(stderr, "symbol grammar header invalid: %s\n", kdna_status_str(rc));
        return 1;
    }
    if (h.source_n != expected_n) {
        fprintf(stderr, "source_n mismatch: got %llu expected %llu\n",
                (unsigned long long)h.source_n, (unsigned long long)expected_n);
        return 1;
    }
    if (h.rule_count == 0u || h.source_word_count != h.rule_count + 1u) {
        fprintf(stderr, "bad rule/source word relation\n");
        return 1;
    }
    if (h.record_bytes != KDNA_KGRAM_RECORD_BYTES ||
        h.payload_bytes != h.rule_count * (uint64_t)KDNA_KGRAM_RECORD_BYTES) {
        fprintf(stderr, "bad record/payload contract\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;
    if (fseek(f, (long)h.header_bytes, SEEK_SET) != 0) { fclose(f); return 1; }

    kdna_krule_record prev;
    int have_prev = 0;
    uint64_t nonzero_strength = 0u;
    for (uint64_t i = 0u; i < h.rule_count; ++i) {
        kdna_krule_record r;
        if (fread(&r, 1u, sizeof(r), f) != sizeof(r)) { fclose(f); return 1; }
        if (r.id != i + 1u || r.sequence_index != i) { fclose(f); return 1; }
        if (r.from_id == 0u || r.to_id == 0u) { fclose(f); return 1; }
        if (r.from_raw < 1u || r.from_raw > 5u || r.to_raw < 1u || r.to_raw > 5u ||
            r.from_dom < 1u || r.from_dom > 5u || r.to_dom < 1u || r.to_dom > 5u) {
            fclose(f); return 1;
        }
        if (r.strength <= 0.0) { fclose(f); return 1; }
        if (r.strength > 0.0) nonzero_strength++;
        if (have_prev) {
            if (prev.from_id > r.from_id || (prev.from_id == r.from_id && prev.to_id > r.to_id)) {
                fclose(f); return 1;
            }
        }
        prev = r;
        have_prev = 1;
    }
    fclose(f);
    if (nonzero_strength == 0u) return 1;
    printf("symbol_gram_ok rules=%llu source_n=%llu\n",
           (unsigned long long)h.rule_count, (unsigned long long)h.source_n);
    return 0;
}
