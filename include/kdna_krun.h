#ifndef KDNA_KRUN_H
#define KDNA_KRUN_H

#include "kdna.h"
#include "kdna_kgram.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KRUN_MAGIC "KRUN0001"
#define KDNA_KRUN_VERSION 1u
#define KDNA_KRUN_HEADER_BYTES 128u
#define KDNA_KRUN_RECORD_BYTES 256u

#define KDNA_KRUN_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KRUN_FLAG_SOURCE_KGRAM_V1   2ull
#define KDNA_KRUN_FLAG_DETERMINISTIC     4ull
#define KDNA_KRUN_FLAG_RULE_ORDER        8ull

#define KDNA_KRUN_STEP_FLAG_RAW_CHANGE        0x00000001u
#define KDNA_KRUN_STEP_FLAG_DOM_CHANGE        0x00000002u
#define KDNA_KRUN_STEP_FLAG_EFFECT_CHANGE     0x00000004u
#define KDNA_KRUN_STEP_FLAG_CROSSES_ZERO      0x00000008u
#define KDNA_KRUN_STEP_FLAG_HIGH_LOCK         0x00000010u
#define KDNA_KRUN_STEP_FLAG_SCORE_RISE        0x00000020u
#define KDNA_KRUN_STEP_FLAG_SCORE_FALL        0x00000040u
#define KDNA_KRUN_STEP_FLAG_CAUSAL_CANDIDATE  0x00000080u

enum kdna_krun_action {
    KDNA_KRUN_ACTION_HOLD = 0,
    KDNA_KRUN_ACTION_TRANSITION = 1,
    KDNA_KRUN_ACTION_MEMBRANE_CROSS = 2,
    KDNA_KRUN_ACTION_COMPRESSION_GATE = 3,
    KDNA_KRUN_ACTION_CASCADE = 4,
    KDNA_KRUN_ACTION_STABILIZE = 5
};

/*
  KRUN v1: deterministic executable resonance plan selected from KGRAM rules.

  Contract:
    - exactly 128-byte header
    - exactly 256-byte records
    - one step per selected KRULE
    - records preserve source rule order
    - little-endian fixed-width integers and IEEE-754 doubles

  Semantics:
    KGRAM = observed transition grammar
    KRUN  = executable/replayable control plan over selected grammar transitions

  The runtime step does not re-simulate K01-K05. It carries measured transition
  boundaries and derived control parameters that a later SubQG runtime can use as
  gates, anchors, coupling windows or initialization slices.
*/
typedef struct kdna_krun_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t reserved0;

    uint64_t step_count;
    uint64_t source_rule_count;
    uint64_t source_word_count;
    uint64_t source_n;

    double x_min;
    double x_max;
    double dx;

    uint64_t payload_bytes;
    uint64_t flags;

    uint32_t selector_flags;
    uint32_t reserved1;
    uint8_t reserved[24];
} kdna_krun_header;

typedef struct kdna_krun_step_record {
    uint64_t id;
    uint64_t step_index;
    uint64_t source_rule_id;
    uint64_t from_word_id;
    uint64_t to_word_id;

    uint64_t from_i0;
    uint64_t from_i1;
    uint64_t to_i0;
    uint64_t to_i1;

    double x_start;
    double x_boundary;
    double x_end;
    double gap_x;

    uint32_t from_raw;
    uint32_t from_dom;
    uint32_t from_effect;
    uint32_t to_raw;
    uint32_t to_dom;
    uint32_t to_effect;
    uint32_t flags;
    uint32_t action;

    double lock_start;
    double lock_end;
    double score_start;
    double score_end;
    double delta_lock;
    double delta_score;
    double strength;

    double drive_x;
    double gain;
    double bias;
    double duration;

    uint8_t reserved[32];
} kdna_krun_step_record;

typedef char kdna_krun_header_size_must_be_128[(sizeof(kdna_krun_header) == KDNA_KRUN_HEADER_BYTES) ? 1 : -1];
typedef char kdna_krun_step_record_size_must_be_256[(sizeof(kdna_krun_step_record) == KDNA_KRUN_RECORD_BYTES) ? 1 : -1];

const char *kdna_krun_action_name(uint32_t action);

int kdna_krun_payload_bytes(size_t step_count, uint64_t *bytes_out);
int kdna_krun_init_header(kdna_krun_header *h,
                          size_t step_count,
                          size_t source_rule_count,
                          size_t source_word_count,
                          size_t source_n,
                          double x_min,
                          double x_max,
                          double dx,
                          uint32_t selector_flags);
int kdna_krun_validate_header(const kdna_krun_header *h);
int kdna_krun_write_file(const char *path,
                         const kdna_krun_header *h,
                         const kdna_krun_step_record *steps);
int kdna_krun_read_header_file(const char *path, kdna_krun_header *h);
int kdna_krun_build_step_from_rule(const kdna_krule_record *rule,
                                   uint64_t step_index,
                                   kdna_krun_step_record *step);

#ifdef __cplusplus
}
#endif

#endif
