#ifndef KDNA_KGLIB_H
#define KDNA_KGLIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KGLIB_MAGIC "KGLIB001"
#define KDNA_KGLIB_VERSION 1u
#define KDNA_KGLIB_HEADER_BYTES 128u
#define KDNA_KGLIB_RECORD_BYTES 1024u

#define KDNA_KGLIB_FLAG_LE 0x1ull
#define KDNA_KGLIB_FLAG_HUMAN24 0x2ull
#define KDNA_KGLIB_FLAG_MATRIX_PRESENT 0x4ull
#define KDNA_KGLIB_FLAG_MATRIX_CSV_PRESENT 0x8ull
#define KDNA_KGLIB_FLAG_COMPLETE 0x10ull

#define KDNA_KGLIB_REC_RAW_PRESENT     0x0001ull
#define KDNA_KGLIB_REC_KFSUM_PRESENT   0x0002ull
#define KDNA_KGLIB_REC_KDNA_PRESENT    0x0004ull
#define KDNA_KGLIB_REC_KGRAM_PRESENT   0x0008ull
#define KDNA_KGLIB_REC_RAW_KFSUM_MATCH 0x0010ull
#define KDNA_KGLIB_REC_RAW_KDNA_MATCH  0x0020ull
#define KDNA_KGLIB_REC_KGRAM_VALID     0x0040ull
#define KDNA_KGLIB_REC_COMPLETE        0x0080ull

typedef struct kdna_kglib_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t chrom_count;

    uint64_t payload_bytes;
    uint64_t flags;
    uint64_t total_raw_symbols;
    uint64_t total_kdna_symbols;
    uint64_t total_kgram_rules;
    uint64_t total_artifact_bytes;

    uint64_t matrix_hash;
    uint64_t matrix_csv_hash;

    uint8_t reserved[40];
} kdna_kglib_header;

typedef struct kdna_kglib_record {
    char name[32];

    uint64_t status_flags;
    uint64_t raw_bytes;
    uint64_t kfsum_bytes;
    uint64_t kdna_bytes;
    uint64_t kgram_bytes;

    uint64_t raw_hash;
    uint64_t kfsum_hash;
    uint64_t kdna_hash;
    uint64_t kgram_hash;

    uint64_t kfsum_symbols;
    uint64_t kfsum_bases_total;
    uint64_t kfsum_bases_valid;
    uint64_t kfsum_bases_invalid;
    uint64_t kfsum_resets;
    uint64_t kfsum_contigs;

    uint64_t raw_symbols;
    uint64_t kdna_symbols;

    uint64_t kgram_rules;
    uint64_t kgram_source_n;
    uint64_t kgram_payload_bytes;

    char raw_path[128];
    char kfsum_path[128];
    char kdna_path[128];
    char kgram_path[128];

    uint8_t reserved[320];
} kdna_kglib_record;

typedef char kdna_kglib_header_size_must_be_128[(sizeof(kdna_kglib_header) == KDNA_KGLIB_HEADER_BYTES) ? 1 : -1];
typedef char kdna_kglib_record_size_must_be_1024[(sizeof(kdna_kglib_record) == KDNA_KGLIB_RECORD_BYTES) ? 1 : -1];

#ifdef __cplusplus
}
#endif

#endif
