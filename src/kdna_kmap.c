#include "kdna_kmap.h"
#include "kdna_kgen.h"
#include "kdna_ksoa.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t hash64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static double unit_from_u64(uint64_t x) {
    return (double)(x >> 11) * (1.0 / 9007199254740992.0);
}

static uint64_t kmap_source_max_from_k(uint32_t k) {
    if (k == 0u) return 0ull;
    if (k >= 32u) return UINT64_MAX;
    return (1ull << (2u * k)) - 1ull;
}

void kdna_kmap_default_params(kdna_kmap_params *p) {
    if (!p) return;
    p->mode = KDNA_KMAP_MODE_AFFINE;
    p->kmer_k = 16u;
    p->x_min = -8.0;
    p->x_max = 8.0;
    p->source_min = 0u;
    p->source_max = kmap_source_max_from_k(16u);
    p->chunk_n = 262144u;
}

const char *kdna_kmap_mode_name(uint32_t mode) {
    switch (mode) {
        case KDNA_KMAP_MODE_AFFINE: return "affine";
        case KDNA_KMAP_MODE_HASH: return "hash";
        default: return "unknown";
    }
}

int kdna_kmap_init_header(kdna_kmap_header *h, const kdna_kmap_params *p, size_t n) {
    if (!h || !p || n == 0u || !(p->x_max > p->x_min)) return KDNA_EINVAL;
    if (p->mode != KDNA_KMAP_MODE_AFFINE && p->mode != KDNA_KMAP_MODE_HASH) return KDNA_EINVAL;
    if (p->mode == KDNA_KMAP_MODE_AFFINE && !(p->source_max > p->source_min)) return KDNA_EINVAL;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;
    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KMAP_MAGIC, 8u);
    h->version = KDNA_KMAP_VERSION;
    h->header_bytes = KDNA_KMAP_HEADER_BYTES;
    h->record_bytes = KDNA_KMAP_RECORD_BYTES;
    h->mode = p->mode;
    h->n = (uint64_t)n;
    h->source_bytes = (uint64_t)n * (uint64_t)sizeof(uint64_t);
    h->payload_bytes = (uint64_t)n * (uint64_t)KDNA_KMAP_RECORD_BYTES;
    h->flags = KDNA_KMAP_FLAG_LE_IEEE754_DOUBLE | KDNA_KMAP_FLAG_SYMBOL_U64 |
               KDNA_KMAP_FLAG_KDNA_VARIANT_ID | KDNA_KMAP_FLAG_DETERMINISTIC;
    h->x_min = p->x_min;
    h->x_max = p->x_max;
    h->source_min = (double)p->source_min;
    h->source_max = (double)p->source_max;
    h->kmer_k = p->kmer_k;
    return KDNA_OK;
}

int kdna_kmap_validate_header(const kdna_kmap_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KMAP_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KMAP_VERSION || h->header_bytes != KDNA_KMAP_HEADER_BYTES) return KDNA_EINVAL;
    if (h->record_bytes != KDNA_KMAP_RECORD_BYTES || h->n == 0u) return KDNA_EINVAL;
    if (!(h->x_max > h->x_min)) return KDNA_EINVAL;
    if ((h->flags & KDNA_KMAP_FLAG_LE_IEEE754_DOUBLE) == 0u) return KDNA_EINVAL;
    if (h->payload_bytes != h->n * (uint64_t)KDNA_KMAP_RECORD_BYTES) return KDNA_EINVAL;
    if (h->source_bytes != h->n * (uint64_t)sizeof(uint64_t)) return KDNA_EINVAL;
    if (h->mode != KDNA_KMAP_MODE_AFFINE && h->mode != KDNA_KMAP_MODE_HASH) return KDNA_EINVAL;
    return KDNA_OK;
}

int kdna_kmap_read_header_file(const char *path, kdna_kmap_header *h) {
    if (!path || !h) return KDNA_EINVAL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    size_t got = fread(h, 1u, sizeof(*h), f);
    int ok = fclose(f) == 0;
    if (got != sizeof(*h) || !ok) return KDNA_EIO;
    return kdna_kmap_validate_header(h);
}

int kdna_kmap_write_header_file(FILE *f, const kdna_kmap_header *h) {
    if (!f || !h) return KDNA_EINVAL;
    int rc = kdna_kmap_validate_header(h);
    if (rc != KDNA_OK) return rc;
    return fwrite(h, 1u, sizeof(*h), f) == sizeof(*h) ? KDNA_OK : KDNA_EIO;
}

static double project_symbol(uint64_t sym, const kdna_kmap_params *p) {
    double u = 0.0;
    if (p->mode == KDNA_KMAP_MODE_HASH) {
        u = unit_from_u64(hash64(sym));
    } else {
        const double lo = (double)p->source_min;
        const double hi = (double)p->source_max;
        u = ((double)sym - lo) / (hi - lo);
        if (u < 0.0) u = 0.0;
        if (u > 1.0) u = 1.0;
    }
    return p->x_min + u * (p->x_max - p->x_min);
}

static int process_chunk(const uint64_t *symbols,
                         size_t count,
                         const kdna_kmap_params *p,
                         const kdna_constants *c,
                         const char *backend,
                         const char *kernel_path,
                         uint64_t *out_symbols,
                         kdna_kmap_record *records) {
    double *x = (double *)malloc(count * sizeof(double));
    double *k = (double *)calloc((size_t)KDNA_FIELDS * count, sizeof(double));
    if (!x || !k) { free(x); free(k); return KDNA_ENOMEM; }

    for (size_t i = 0u; i < count; ++i) x[i] = project_symbol(symbols[i], p);

    int rc;
    if (backend && strcmp(backend, "opencl") == 0) {
        rc = kdna_eval_opencl(x, k, count, c, kernel_path ? kernel_path : "kernels/kdna_eval.cl");
        if (rc == KDNA_EOPENCL || rc == KDNA_ENO_DEVICE || rc == KDNA_EBUILD) {
            free(x); free(k); return rc;
        }
    } else {
        rc = kdna_eval_cpu(x, k, count, c);
    }
    if (rc != KDNA_OK) { free(x); free(k); return rc; }

    for (size_t i = 0u; i < count; ++i) {
        const double k1 = k[kdna_idx(KDNA_K01, count, i)];
        const double k2 = k[kdna_idx(KDNA_K02, count, i)];
        const double k3 = k[kdna_idx(KDNA_K03, count, i)];
        const double k4 = k[kdna_idx(KDNA_K04, count, i)];
        const double k5 = k[kdna_idx(KDNA_K05, count, i)];
        const double lock = k[kdna_idx(KDNA_LK, count, i)];
        const double score = k[kdna_idx(KDNA_DOM_SCORE, count, i)];
        const uint32_t raw = (uint32_t)llround(k[kdna_idx(KDNA_RAW, count, i)]);
        const uint32_t dom = (uint32_t)llround(k[kdna_idx(KDNA_DOM, count, i)]);
        const uint64_t vid = kdna_kgen_variant_id(raw, dom, lock, score, k1, k2, k3, k4, k5);
        out_symbols[i] = vid;
        if (records) {
            memset(&records[i], 0, sizeof(records[i]));
            records[i].source_symbol = symbols[i];
            records[i].variant_id = vid;
            records[i].x = x[i];
            records[i].k1 = k1; records[i].k2 = k2; records[i].k3 = k3; records[i].k4 = k4; records[i].k5 = k5;
            records[i].lock = lock;
            records[i].dominance_score = score;
            records[i].raw = raw;
            records[i].dom = dom;
            records[i].flags = KDNA_KMAP_FLAG_SYMBOL_U64 | KDNA_KMAP_FLAG_KDNA_VARIANT_ID;
        }
    }

    free(x); free(k);
    return KDNA_OK;
}

int kdna_kmap_project_symbols_file(const char *symbols_path,
                                   size_t n,
                                   const char *out_symbols_path,
                                   const char *out_kmap_path,
                                   const kdna_kmap_params *params,
                                   const kdna_constants *constants,
                                   const char *backend,
                                   const char *kernel_path) {
    if (!symbols_path || n == 0u || !out_symbols_path || !params || !constants) return KDNA_EINVAL;
    if (params->mode == KDNA_KMAP_MODE_AFFINE && !(params->source_max > params->source_min)) return KDNA_EINVAL;
    if (!(params->x_max > params->x_min)) return KDNA_EINVAL;
    const size_t chunk_n = params->chunk_n ? params->chunk_n : 262144u;

    FILE *in = fopen(symbols_path, "rb");
    if (!in) return KDNA_EIO;
    FILE *outs = fopen(out_symbols_path, "wb");
    if (!outs) { fclose(in); return KDNA_EIO; }
    FILE *outm = NULL;
    kdna_kmap_header h;
    if (out_kmap_path) {
        int rc = kdna_kmap_init_header(&h, params, n);
        if (rc != KDNA_OK) { fclose(in); fclose(outs); return rc; }
        outm = fopen(out_kmap_path, "wb");
        if (!outm) { fclose(in); fclose(outs); return KDNA_EIO; }
        rc = kdna_kmap_write_header_file(outm, &h);
        if (rc != KDNA_OK) { fclose(in); fclose(outs); fclose(outm); return rc; }
    }

    uint64_t *symbols = (uint64_t *)malloc(chunk_n * sizeof(uint64_t));
    uint64_t *out_symbols = (uint64_t *)malloc(chunk_n * sizeof(uint64_t));
    kdna_kmap_record *records = outm ? (kdna_kmap_record *)malloc(chunk_n * sizeof(kdna_kmap_record)) : NULL;
    if (!symbols || !out_symbols || (outm && !records)) {
        free(symbols); free(out_symbols); free(records); fclose(in); fclose(outs); if (outm) fclose(outm);
        return KDNA_ENOMEM;
    }

    size_t done = 0u;
    int rc = KDNA_OK;
    while (done < n) {
        const size_t want = (n - done) < chunk_n ? (n - done) : chunk_n;
        if (fread(symbols, sizeof(uint64_t), want, in) != want) { rc = KDNA_EIO; break; }
        rc = process_chunk(symbols, want, params, constants, backend, kernel_path, out_symbols, records);
        if (rc != KDNA_OK) break;
        if (fwrite(out_symbols, sizeof(uint64_t), want, outs) != want) { rc = KDNA_EIO; break; }
        if (outm && fwrite(records, sizeof(kdna_kmap_record), want, outm) != want) { rc = KDNA_EIO; break; }
        done += want;
    }

    if (fclose(in) != 0) rc = KDNA_EIO;
    if (fclose(outs) != 0) rc = KDNA_EIO;
    if (outm && fclose(outm) != 0) rc = KDNA_EIO;

    free(symbols); free(out_symbols); free(records);
    return rc;
}
