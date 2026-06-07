#include "kdna_kmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

static int write_symbols(const char *path, size_t n) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    for (size_t i = 0; i < n; ++i) {
        uint64_t v = (uint64_t)((i * 65537u) ^ (i << 7) ^ 0x12345u);
        if (fwrite(&v, sizeof(v), 1u, f) != 1u) { fclose(f); return 0; }
    }
    return fclose(f) == 0;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: test_kmap symbols.u64 out.u64 out.kmap\n");
        return 2;
    }
    const size_t n = 1024u;
    if (!write_symbols(argv[1], n)) {
        fprintf(stderr, "write input failed\n");
        return 1;
    }

    kdna_kmap_params p;
    kdna_kmap_default_params(&p);
    p.kmer_k = 16u;
    p.source_min = 0u;
    p.source_max = (1ull << 32u) - 1ull;
    p.chunk_n = 128u;

    kdna_constants c;
    kdna_default_constants(&c);
    int rc = kdna_kmap_project_symbols_file(argv[1], n, argv[2], argv[3], &p, &c, "cpu", "kernels/kdna_eval.cl");
    if (rc != KDNA_OK) {
        fprintf(stderr, "project failed: %s\n", kdna_status_str(rc));
        return 1;
    }

    kdna_kmap_header h;
    rc = kdna_kmap_read_header_file(argv[3], &h);
    if (rc != KDNA_OK) {
        fprintf(stderr, "header failed: %s\n", kdna_status_str(rc));
        return 1;
    }
    if (h.n != n || h.record_bytes != KDNA_KMAP_RECORD_BYTES || h.payload_bytes != n * (uint64_t)KDNA_KMAP_RECORD_BYTES) {
        fprintf(stderr, "bad header values\n");
        return 1;
    }

    FILE *f = fopen(argv[2], "rb");
    if (!f) return 1;
    uint64_t first = 0u;
    if (fread(&first, sizeof(first), 1u, f) != 1u) { fclose(f); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    if (sz != (long)(n * sizeof(uint64_t)) || first == 0u) {
        fprintf(stderr, "bad output symbols size/first=%" PRIu64 "\n", first);
        return 1;
    }

    printf("kmap_ok n=%zu first_variant=%" PRIu64 "\n", n, first);
    return 0;
}
