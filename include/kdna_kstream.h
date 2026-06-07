#ifndef KDNA_KSTREAM_H
#define KDNA_KSTREAM_H

#include <stdint.h>

#define KDNA_KSTREAM_MAGIC "KSTREAM1"
#define KDNA_KSTREAM_VERSION 1u
#define KDNA_KSTREAM_HEADER_BYTES 256u

#define KDNA_KSTREAM_KIND_BYTES  1u
#define KDNA_KSTREAM_KIND_TOKENS 2u
#define KDNA_KSTREAM_KIND_CSV    3u
#define KDNA_KSTREAM_KIND_CONLLU 4u

#define KDNA_KSTREAM_FLAG_LE_U64      0x00000001u
#define KDNA_KSTREAM_FLAG_SUMMARY     0x00000002u
#define KDNA_KSTREAM_FLAG_UNLIMITED   0x00000004u
#define KDNA_KSTREAM_FLAG_AUTO_RANGE  0x00000008u
#define KDNA_KSTREAM_FLAG_HASHED      0x00000010u
#define KDNA_KSTREAM_FLAG_QUANTIZED   0x00000020u

#pragma pack(push, 1)
typedef struct kdna_kstream_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t kind;
    uint32_t flags;
    uint32_t symbol_bits;
    uint32_t window;
    uint32_t stride;
    uint32_t bins;

    uint64_t input_bytes;
    uint64_t input_records;
    uint64_t symbols_written;
    uint64_t skipped_records;
    uint64_t invalid_records;
    uint64_t max_symbols;
    uint64_t payload_bytes;
    uint64_t seed;
    uint64_t input_hash;
    uint64_t symbol_hash;

    double min_value;
    double max_value;
    double scale;
    double offset;

    char source_name[64];
    uint8_t reserved[40];
} kdna_kstream_header;
#pragma pack(pop)

typedef char kdna_kstream_header_must_be_256[(sizeof(kdna_kstream_header) == KDNA_KSTREAM_HEADER_BYTES) ? 1 : -1];

#endif

