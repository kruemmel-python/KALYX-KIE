#ifndef KDNA_KLIB_H
#define KDNA_KLIB_H

#include "kdna.h"
#include "kdna_ksoa.h"
#include "kdna_kreg.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KLIB_MAGIC "KLIB0001"
#define KDNA_KLIB_VERSION 1u
#define KDNA_KLIB_HEADER_BYTES 128u
#define KDNA_KLIB_RECORD_BYTES 320u

#define KDNA_KLIB_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KLIB_FLAG_SOURCE_KSOA_V1    2ull
#define KDNA_KLIB_FLAG_SOURCE_KREG_V1    4ull

#define KDNA_KWORD_FLAG_NULL_NEAR        0x00000001u
#define KDNA_KWORD_FLAG_RAW_DOM_MISMATCH 0x00000002u
#define KDNA_KWORD_FLAG_HIGH_LOCK        0x00000004u
#define KDNA_KWORD_FLAG_NARROW           0x00000008u
#define KDNA_KWORD_FLAG_HIGH_SCORE       0x00000010u

enum kdna_effect_class {
    KDNA_EFFECT_UNKNOWN = 0,
    KDNA_EFFECT_STABLE_ATTRACTOR = 1,
    KDNA_EFFECT_ENVELOPE_FORM = 2,
    KDNA_EFFECT_PHASE_OFFSET = 3,
    KDNA_EFFECT_COMPRESSION_GATE = 4,
    KDNA_EFFECT_CASCADE_BAND = 5,
    KDNA_EFFECT_NULL_MEMBRANE_JUMP = 6,
    KDNA_EFFECT_RAW_DOM_MISMATCH_ZONE = 7,
    KDNA_EFFECT_TRANSITION_BRIDGE = 8
};

/*
  KLIB v1: fixed-size resonance vocabulary extracted from KSOA + KREG.

  Binary contract:
    - exactly 128-byte header
    - exactly 320-byte records
    - little-endian fixed-width integers and IEEE-754 doubles
    - one KWORD record per KREG segment
    - records are ordered by source_segment_index ascending

  Semantics:
    KREG = contiguous discrete region
    KWORD = measured resonance vocabulary unit for that region
    KLIB = archive of KWORD records, queryable without re-running OpenCL/CPU evaluation
*/
typedef struct kdna_klib_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t source_fields;
    uint64_t word_count;
    uint64_t source_n;
    uint64_t source_segment_count;
    double x_min;
    double x_max;
    double dx;
    uint64_t payload_bytes;
    uint64_t flags;
    uint8_t reserved[40];
} kdna_klib_header;

typedef struct kdna_kword_record {
    uint64_t id;
    uint64_t source_segment_index;
    uint64_t i0;
    uint64_t i1;

    double x0;
    double x1;
    double width;

    uint32_t raw;
    uint32_t dom;
    uint32_t flags;
    uint32_t effect_class;

    double lock_min;
    double lock_max;
    double lock_mean;

    double score_min;
    double score_max;
    double score_mean;

    double k_mean[5];
    double k_absmax[5];

    double s_mean[5];
    double s_absmax[5];

    double ek_mean;
    double ak_mean;

    uint64_t sample_count;
    uint8_t reserved[16];
} kdna_kword_record;

typedef char kdna_klib_header_size_must_be_128[(sizeof(kdna_klib_header) == KDNA_KLIB_HEADER_BYTES) ? 1 : -1];
typedef char kdna_kword_record_size_must_be_320[(sizeof(kdna_kword_record) == KDNA_KLIB_RECORD_BYTES) ? 1 : -1];

const char *kdna_effect_class_name(uint32_t effect_class);
int kdna_effect_class_from_name(const char *name, uint32_t *effect_out);

int kdna_klib_payload_bytes(size_t word_count, uint64_t *bytes_out);
int kdna_klib_init_header(kdna_klib_header *h,
                          size_t word_count,
                          size_t source_n,
                          size_t source_segment_count,
                          double x_min,
                          double x_max,
                          double dx);
int kdna_klib_validate_header(const kdna_klib_header *h);
int kdna_klib_write_file(const char *path,
                         const kdna_klib_header *h,
                         const kdna_kword_record *records);
int kdna_klib_read_header_file(const char *path, kdna_klib_header *h);

#ifdef __cplusplus
}
#endif

#endif
