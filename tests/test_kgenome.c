#include "kdna_kgenome.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: test_kgenome matrix.kgenome csv min_cells\n");
        return 2;
    }
    const char *path = argv[1];
    const char *csv = argv[2];
    const unsigned long long min_cells = strtoull(argv[3], NULL, 10);

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open KGENOME\n");
        return 1;
    }
    kdna_kgenome_header h;
    if (fread(&h, 1u, sizeof(h), f) != sizeof(h)) {
        fclose(f);
        fprintf(stderr, "short header\n");
        return 1;
    }
    if (memcmp(h.magic, KDNA_KGENOME_MAGIC, 8u) != 0 ||
        h.version != KDNA_KGENOME_VERSION ||
        h.header_bytes != KDNA_KGENOME_HEADER_BYTES ||
        h.record_bytes != KDNA_KGENOME_RECORD_BYTES ||
        h.matrix_count < min_cells ||
        h.payload_bytes != h.matrix_count * KDNA_KGENOME_RECORD_BYTES) {
        fclose(f);
        fprintf(stderr, "bad KGENOME header\n");
        return 1;
    }
    kdna_kgenome_record *records = (kdna_kgenome_record *)malloc((size_t)h.payload_bytes);
    if (!records) { fclose(f); return 1; }
    if (fread(records, 1u, (size_t)h.payload_bytes, f) != (size_t)h.payload_bytes) {
        free(records); fclose(f); fprintf(stderr, "short payload\n"); return 1;
    }
    fclose(f);

    for (uint64_t i = 0u; i < h.matrix_count; ++i) {
        const kdna_kgenome_record *r = &records[i];
        if (r->n < 8u || r->test_transitions == 0u || r->unique_variants == 0u) {
            fprintf(stderr, "bad record counts\n");
            free(records); return 1;
        }
        if (!isfinite(r->entropy_raw) || !isfinite(r->baseline_accuracy) ||
            !isfinite(r->kgram_accuracy) || !isfinite(r->lift) ||
            !isfinite(r->surprise_rate) || !isfinite(r->compression_ratio)) {
            fprintf(stderr, "nonfinite record\n");
            free(records); return 1;
        }
        if (r->baseline_accuracy < 0.0 || r->baseline_accuracy > 1.0 ||
            r->kgram_accuracy < 0.0 || r->kgram_accuracy > 1.0 ||
            r->surprise_rate < 0.0 || r->surprise_rate > 1.0) {
            fprintf(stderr, "out of range record\n");
            free(records); return 1;
        }
    }
    free(records);

    f = fopen(csv, "rb");
    if (!f) {
        fprintf(stderr, "cannot open csv\n");
        return 1;
    }
    char line[256];
    if (!fgets(line, sizeof(line), f) || strstr(line, "row,col,n") == NULL) {
        fclose(f);
        fprintf(stderr, "bad csv header\n");
        return 1;
    }
    fclose(f);
    printf("kgenome_ok cells=%llu sources=%llu\n",
           (unsigned long long)h.matrix_count,
           (unsigned long long)h.source_count);
    return 0;
}
