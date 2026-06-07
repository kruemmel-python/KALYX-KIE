#ifndef KDNA_KLLIB_H
#define KDNA_KLLIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KLLIB_MAGIC "KLLIB001"
#define KDNA_KLLIB_VERSION 1u
#define KDNA_KLLIB_HEADER_BYTES 256u
#define KDNA_KLLIB_CONFIG_RECORD_BYTES 1024u
#define KDNA_KLLIB_CELL_RECORD_BYTES 512u

#define KDNA_KLLIB_FLAG_LE 0x0000000000000001ull
#define KDNA_KLLIB_FLAG_COMPLETE 0x0000000000000002ull
#define KDNA_KLLIB_FLAG_HAS_NULLS 0x0000000000000004ull
#define KDNA_KLLIB_FLAG_HAS_REPORTS 0x0000000000000008ull

/*
    KLLIB001 ABI
    Fixed packed layout:
      header        = 256 bytes
      config record = 1024 bytes
      cell record   = 512 bytes

    v1.0.2 correction:
      - header reserved padding is 120 bytes, not 128.
      - config record has no trailing reserved padding.
      - cell record keeps 264 bytes reserved padding.

    The static size checks below are intentional ABI guards.
*/

#pragma pack(push, 1)

typedef struct kdna_kllib_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t config_record_bytes;
    uint32_t cell_record_bytes;

    uint32_t config_count;
    uint32_t cell_count;
    uint64_t flags;

    uint64_t total_de_symbols;
    uint64_t total_en_symbols;
    uint64_t total_de_kgram_rules;
    uint64_t total_en_kgram_rules;
    uint64_t total_artifact_bytes;

    uint64_t config_payload_bytes;
    uint64_t cell_payload_bytes;
    uint64_t library_hash;

    double avg_self_observed_accuracy;
    double avg_cross_observed_accuracy;
    double avg_self_observed_minus_null;
    double avg_cross_observed_minus_null;

    uint8_t reserved[120];
} kdna_kllib_header;

typedef struct kdna_kllib_config_record {
    char name[64];
    char field[32];

    uint32_t window;
    uint32_t symbol_bits;
    uint32_t flags;
    uint32_t reserved0;

    uint64_t de_symbols;
    uint64_t en_symbols;
    uint64_t de_kstream_symbols;
    uint64_t en_kstream_symbols;

    uint64_t de_kgram_rules;
    uint64_t en_kgram_rules;
    uint64_t de_kgram_source_n;
    uint64_t en_kgram_source_n;

    uint64_t de_symbols_hash;
    uint64_t en_symbols_hash;
    uint64_t de_kstream_hash;
    uint64_t en_kstream_hash;
    uint64_t de_kdna_hash;
    uint64_t en_kdna_hash;
    uint64_t de_kgram_hash;
    uint64_t en_kgram_hash;

    uint64_t matrix_csv_hash;
    uint64_t null_csv_hash;
    uint64_t report_html_hash;

    uint64_t de_symbols_bytes;
    uint64_t en_symbols_bytes;
    uint64_t de_kgram_bytes;
    uint64_t en_kgram_bytes;
    uint64_t matrix_csv_bytes;
    uint64_t null_csv_bytes;
    uint64_t report_html_bytes;

    double avg_self_observed_accuracy;
    double avg_cross_observed_accuracy;
    double avg_self_observed_minus_null;
    double avg_cross_observed_minus_null;

    char de_symbols_path[96];
    char en_symbols_path[96];
    char de_kgram_path[96];
    char en_kgram_path[96];
    char matrix_csv_path[96];
    char null_csv_path[96];
    char report_html_path[96];
} kdna_kllib_config_record;

typedef struct kdna_kllib_cell_record {
    char config[64];
    char row[16];
    char col[16];
    char cell_class[16];

    double observed_accuracy;
    double baseline_accuracy;
    double lift;
    double surprise_rate;
    double avg_null_accuracy;
    double observed_minus_null;

    uint64_t out_of_grammar;
    uint64_t grammar_edges;
    uint64_t flags;

    char null_modes[64];

    uint8_t reserved[264];
} kdna_kllib_cell_record;

#pragma pack(pop)

typedef char kdna_kllib_header_must_be_256[(sizeof(kdna_kllib_header) == KDNA_KLLIB_HEADER_BYTES) ? 1 : -1];
typedef char kdna_kllib_config_record_must_be_1024[(sizeof(kdna_kllib_config_record) == KDNA_KLLIB_CONFIG_RECORD_BYTES) ? 1 : -1];
typedef char kdna_kllib_cell_record_must_be_512[(sizeof(kdna_kllib_cell_record) == KDNA_KLLIB_CELL_RECORD_BYTES) ? 1 : -1];

#ifdef __cplusplus
}
#endif

#endif

