#include "kdna_kmap.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
    printf("kdna_project — KMAP external-symbol projection into Krümmel-DNA variant language\n");
    printf("project:\n");
    printf("  kdna_project --symbols in.u64 --n N --out-symbols out.u64 [--out-kmap out.kmap]\n");
    printf("               [--mode affine|hash] [--k K] [--xmin -8] [--xmax 8]\n");
    printf("               [--backend cpu|opencl] [--kernel kernels/kdna_eval.cl] [--chunk N]\n");
    printf("inspect:\n");
    printf("  kdna_project --a file.kmap\n");
}

static int parse_u64(const char *s, uint64_t *v) {
    if (!s || !v) return 0;
    char *end = NULL;
    unsigned long long x = strtoull(s, &end, 0);
    if (!end || *end != 0) return 0;
    *v = (uint64_t)x;
    return 1;
}

static uint64_t source_max_from_k(uint32_t k) {
    if (k == 0u) return 0u;
    if (k >= 32u) return UINT64_MAX;
    return (1ull << (2u * k)) - 1ull;
}

static void inspect_kmap(const char *path, size_t top) {
    kdna_kmap_header h;
    int rc = kdna_kmap_read_header_file(path, &h);
    if (rc != KDNA_OK) {
        fprintf(stderr, "cannot read KMAP '%s': %s\n", path, kdna_status_str(rc));
        exit(2);
    }
    printf("file: KMAP n:%" PRIu64 " mode:%s record_bytes:%u payload:%" PRIu64 " x:[%0.17g,%0.17g] source:[%0.17g,%0.17g] k:%" PRIu64 "\n",
           h.n, kdna_kmap_mode_name(h.mode), h.record_bytes, h.payload_bytes,
           h.x_min, h.x_max, h.source_min, h.source_max, h.kmer_k);

    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "open failed\n"); exit(2); }
    fseek(f, (long)sizeof(h), SEEK_SET);
    kdna_kmap_record r;
    size_t shown = 0u;
    while (shown < top && fread(&r, 1u, sizeof(r), f) == sizeof(r)) {
        printf("record:%zu source:%" PRIu64 " variant:%" PRIu64 " x:% .17e raw:K%u D:K%u lock:% .17e score:% .17e K:[%.6g %.6g %.6g %.6g %.6g]\n",
               shown, r.source_symbol, r.variant_id, r.x, r.raw, r.dom, r.lock, r.dominance_score,
               r.k1, r.k2, r.k3, r.k4, r.k5);
        shown++;
    }
    fclose(f);
}

int main(int argc, char **argv) {
    const char *symbols = NULL, *out_symbols = NULL, *out_kmap = NULL;
    const char *backend = "cpu", *kernel = "kernels/kdna_eval.cl";
    const char *inspect = NULL;
    uint64_t n64 = 0u;
    size_t top = 8u;
    kdna_kmap_params p;
    kdna_kmap_default_params(&p);

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--symbols") == 0 && i + 1 < argc) symbols = argv[++i];
        else if (strcmp(argv[i], "--n") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &n64)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--out-symbols") == 0 && i + 1 < argc) out_symbols = argv[++i];
        else if (strcmp(argv[i], "--out-kmap") == 0 && i + 1 < argc) out_kmap = argv[++i];
        else if (strcmp(argv[i], "--backend") == 0 && i + 1 < argc) backend = argv[++i];
        else if (strcmp(argv[i], "--kernel") == 0 && i + 1 < argc) kernel = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            const char *m = argv[++i];
            if (strcmp(m, "affine") == 0) p.mode = KDNA_KMAP_MODE_AFFINE;
            else if (strcmp(m, "hash") == 0) p.mode = KDNA_KMAP_MODE_HASH;
            else { usage(); return 2; }
        } else if (strcmp(argv[i], "--k") == 0 && i + 1 < argc) {
            uint64_t k = 0u;
            if (!parse_u64(argv[++i], &k) || k > 63u) { usage(); return 2; }
            p.kmer_k = (uint32_t)k;
            p.source_min = 0u;
            p.source_max = source_max_from_k((uint32_t)k);
        } else if (strcmp(argv[i], "--xmin") == 0 && i + 1 < argc) p.x_min = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--xmax") == 0 && i + 1 < argc) p.x_max = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--source-min") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &p.source_min)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--source-max") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &p.source_max)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--chunk") == 0 && i + 1 < argc) { uint64_t v=0; if (!parse_u64(argv[++i], &v)) return 2; p.chunk_n = (size_t)v; }
        else if (strcmp(argv[i], "--a") == 0 && i + 1 < argc) inspect = argv[++i];
        else if (strcmp(argv[i], "--top") == 0 && i + 1 < argc) { uint64_t v=0; if (!parse_u64(argv[++i], &v)) return 2; top = (size_t)v; }
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (inspect) {
        inspect_kmap(inspect, top);
        return 0;
    }

    if (!symbols || n64 == 0u || !out_symbols) {
        usage();
        return 2;
    }
    if (p.mode == KDNA_KMAP_MODE_AFFINE && !(p.source_max > p.source_min)) {
        fprintf(stderr, "invalid affine source range; pass --k K or --source-min/--source-max\n");
        return 2;
    }

    kdna_constants c;
    kdna_default_constants(&c);

    int rc = kdna_kmap_project_symbols_file(symbols,
                                            (size_t)n64,
                                            out_symbols,
                                            out_kmap,
                                            &p,
                                            &c,
                                            backend,
                                            kernel);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_project failed: %s\n", kdna_status_str(rc));
        return rc == KDNA_ENO_DEVICE || rc == KDNA_EOPENCL || rc == KDNA_EBUILD ? 3 : 2;
    }

    printf("kdna_project: source=%s n=%" PRIu64 " mode=%s backend=%s out_symbols=%s",
           symbols, n64, kdna_kmap_mode_name(p.mode), backend, out_symbols);
    if (out_kmap) printf(" out_kmap=%s", out_kmap);
    printf(" x:[%0.17g,%0.17g] source:[%" PRIu64 ",%" PRIu64 "] k=%u\n",
           p.x_min, p.x_max, p.source_min, p.source_max, p.kmer_k);
    return 0;
}
