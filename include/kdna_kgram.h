#ifndef KDNA_KGRAM_H
#define KDNA_KGRAM_H

#include "kdna.h"
#include "kdna_klib.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KGRAM_MAGIC "KGRAM01"
#define KDNA_KGRAM_VERSION 1u
#define KDNA_KGRAM_HEADER_BYTES 128u
#define KDNA_KGRAM_RECORD_BYTES 256u

#define KDNA_KGRAM_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KGRAM_FLAG_SOURCE_KLIB_V1    2ull
#define KDNA_KGRAM_FLAG_ADJACENT_ONLY     4ull

#define KDNA_KRULE_FLAG_RAW_CHANGE             0x00000001u
#define KDNA_KRULE_FLAG_DOM_CHANGE             0x00000002u
#define KDNA_KRULE_FLAG_EFFECT_CHANGE          0x00000004u
#define KDNA_KRULE_FLAG_CROSSES_ZERO           0x00000008u
#define KDNA_KRULE_FLAG_NULL_TO_COMPRESSION    0x00000010u
#define KDNA_KRULE_FLAG_COMPRESSION_TO_NULL    0x00000020u
#define KDNA_KRULE_FLAG_HIGH_LOCK_BRIDGE       0x00000040u
#define KDNA_KRULE_FLAG_SCORE_RISE             0x00000080u
#define KDNA_KRULE_FLAG_SCORE_FALL             0x00000100u
#define KDNA_KRULE_FLAG_RAW_DOM_REWIRE         0x00000200u

/*
  KGRAM v1: fixed-size directed transition grammar extracted from KLIB.

  Contract:
    - exactly 128-byte header
    - exactly 256-byte records
    - one rule per adjacent KWORD pair in source order
    - rules[i] maps words[i] -> words[i + 1]
    - little-endian fixed-width integers and IEEE-754 doubles

  Semantics:
    KWORD = resonance vocabulary atom
    KRULE = observed adjacent causal transition between two KWORDs
    KGRAM = ordered transition grammar over measured resonance vocabulary
*/
typedef struct kdna_kgram_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t reserved0;

    uint64_t rule_count;
    uint64_t source_word_count;
    uint64_t source_n;

    double x_min;
    double x_max;
    double dx;

    uint64_t payload_bytes;
    uint64_t flags;
    uint8_t reserved[40];
} kdna_kgram_header;

typedef struct kdna_krule_record {
    uint64_t id;
    uint64_t sequence_index;
    uint64_t from_id;
    uint64_t to_id;

    uint64_t from_segment_index;
    uint64_t to_segment_index;

    uint64_t from_i0;
    uint64_t from_i1;
    uint64_t to_i0;
    uint64_t to_i1;

    double from_x0;
    double from_x1;
    double to_x0;
    double to_x1;

    uint32_t from_raw;
    uint32_t from_dom;
    uint32_t from_effect;
    uint32_t to_raw;
    uint32_t to_dom;
    uint32_t to_effect;
    uint32_t flags;
    uint32_t reserved0;

    double boundary_x;
    double gap_x;
    double delta_lock_mean;
    double delta_score_mean;
    double strength;

    double from_lock_mean;
    double to_lock_mean;
    double from_score_mean;
    double to_score_mean;

    double from_width;
    double to_width;

    uint8_t reserved[24];
} kdna_krule_record;

typedef char kdna_kgram_header_size_must_be_128[(sizeof(kdna_kgram_header) == KDNA_KGRAM_HEADER_BYTES) ? 1 : -1];
typedef char kdna_krule_record_size_must_be_256[(sizeof(kdna_krule_record) == KDNA_KGRAM_RECORD_BYTES) ? 1 : -1];

int kdna_kgram_payload_bytes(size_t rule_count, uint64_t *bytes_out);
int kdna_kgram_init_header(kdna_kgram_header *h,
                           size_t rule_count,
                           size_t source_word_count,
                           size_t source_n,
                           double x_min,
                           double x_max,
                           double dx);
int kdna_kgram_validate_header(const kdna_kgram_header *h);
int kdna_kgram_write_file(const char *path,
                          const kdna_kgram_header *h,
                          const kdna_krule_record *records);
int kdna_kgram_read_header_file(const char *path, kdna_kgram_header *h);
int kdna_kgram_build_rules(const kdna_klib_header *lib_header,
                           const kdna_kword_record *words,
                           kdna_krule_record *rules,
                           size_t rule_capacity);

const char *kdna_krule_flag_name(uint32_t flag);

#ifdef __cplusplus
}
#endif

#endif
