#ifndef KDNA_KMAP_H
#define KDNA_KMAP_H

#include "kdna.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KMAP_MAGIC "KMAP0001"
#define KDNA_KMAP_VERSION 1u
#define KDNA_KMAP_HEADER_BYTES 128u
#define KDNA_KMAP_RECORD_BYTES 128u

#define KDNA_KMAP_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KMAP_FLAG_SYMBOL_U64        2ull
#define KDNA_KMAP_FLAG_KDNA_VARIANT_ID   4ull
#define KDNA_KMAP_FLAG_DETERMINISTIC     8ull

enum kdna_kmap_mode {
    KDNA_KMAP_MODE_AFFINE = 1u,
    KDNA_KMAP_MODE_HASH = 2u
};

typedef struct kdna_kmap_params {
    uint32_t mode;
    uint32_t kmer_k;
    double x_min;
    double x_max;
    uint64_t source_min;
    uint64_t source_max;
    size_t chunk_n;
} kdna_kmap_params;

typedef struct kdna_kmap_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t mode;

    uint64_t n;
    uint64_t source_bytes;
    uint64_t payload_bytes;
    uint64_t flags;

    double x_min;
    double x_max;
    double source_min;
    double source_max;

    uint64_t kmer_k;
    uint8_t reserved[32];
} kdna_kmap_header;

typedef struct kdna_kmap_record {
    uint64_t source_symbol;
    uint64_t variant_id;

    double x;
    double k1;
    double k2;
    double k3;
    double k4;
    double k5;
    double lock;
    double dominance_score;

    uint32_t raw;
    uint32_t dom;
    uint32_t flags;
    uint32_t reserved0;

    uint64_t reserved[4];
} kdna_kmap_record;

typedef char kdna_kmap_header_size_must_be_128[(sizeof(kdna_kmap_header) == KDNA_KMAP_HEADER_BYTES) ? 1 : -1];
typedef char kdna_kmap_record_size_must_be_128[(sizeof(kdna_kmap_record) == KDNA_KMAP_RECORD_BYTES) ? 1 : -1];

void kdna_kmap_default_params(kdna_kmap_params *p);
const char *kdna_kmap_mode_name(uint32_t mode);

int kdna_kmap_init_header(kdna_kmap_header *h, const kdna_kmap_params *p, size_t n);
int kdna_kmap_validate_header(const kdna_kmap_header *h);

int kdna_kmap_read_header_file(const char *path, kdna_kmap_header *h);
int kdna_kmap_write_header_file(FILE *f, const kdna_kmap_header *h);

int kdna_kmap_project_symbols_file(const char *symbols_path,
                                   size_t n,
                                   const char *out_symbols_path,
                                   const char *out_kmap_path,
                                   const kdna_kmap_params *params,
                                   const kdna_constants *constants,
                                   const char *backend,
                                   const char *kernel_path);

#ifdef __cplusplus
}
#endif

#endif
