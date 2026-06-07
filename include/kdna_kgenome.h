#ifndef KDNA_KGENOME_H
#define KDNA_KGENOME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KGENOME_MAGIC "KGENOME1"
#define KDNA_KGENOME_VERSION 1u
#define KDNA_KGENOME_HEADER_BYTES 128u
#define KDNA_KGENOME_RECORD_BYTES 256u

#define KDNA_KGENOME_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KGENOME_FLAG_KDNA_VARIANT_STREAM 2ull
#define KDNA_KGENOME_FLAG_MATRIX_COMPLETE 4ull
#define KDNA_KGENOME_FLAG_DETERMINISTIC 8ull

typedef struct kdna_kgenome_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t reserved0;

    uint64_t source_count;
    uint64_t matrix_count;
    uint64_t payload_bytes;
    uint64_t flags;

    double train_ratio;
    uint32_t bins;
    uint32_t reserved1;

    uint8_t reserved[56];
} kdna_kgenome_header;

typedef struct kdna_kgenome_record {
    uint64_t id;
    uint64_t row_index;
    uint64_t col_index;
    uint64_t n;

    uint64_t train_n;
    uint64_t unique_variants;
    uint64_t grammar_edges;
    uint64_t test_transitions;
    uint64_t out_of_grammar;

    double entropy_raw;
    double baseline_accuracy;
    double kgram_accuracy;
    double lift;
    double surprise_rate;
    double compression_ratio;

    char row_name[32];
    char col_name[32];

    uint8_t reserved[72];
} kdna_kgenome_record;

typedef char kdna_kgenome_header_size_must_be_128[(sizeof(kdna_kgenome_header) == KDNA_KGENOME_HEADER_BYTES) ? 1 : -1];
typedef char kdna_kgenome_record_size_must_be_256[(sizeof(kdna_kgenome_record) == KDNA_KGENOME_RECORD_BYTES) ? 1 : -1];

#ifdef __cplusplus
}
#endif

#endif
