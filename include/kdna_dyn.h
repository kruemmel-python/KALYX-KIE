#ifndef KDNA_DYN_H
#define KDNA_DYN_H

#include "kdna.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_DYN_MAGIC "KDYN0001"
#define KDNA_DYN_VERSION 1u
#define KDNA_DYN_HEADER_BYTES 128u
#define KDNA_DYN_FIELDS 25u

#define KDNA_DYN_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_DYN_FLAG_SOURCE_KRUN_V1    2ull
#define KDNA_DYN_FLAG_DETERMINISTIC     4ull
#define KDNA_DYN_FLAG_RESIDENT_UPDATE   8ull
#define KDNA_DYN_FLAG_OPENCL_KDNA_PACK  16ull

enum kdna_dyn_field {
    KDNA_DYN_E = 0,
    KDNA_DYN_PHI = 1,
    KDNA_DYN_X = 2,
    KDNA_DYN_K01 = 3,
    KDNA_DYN_K02 = 4,
    KDNA_DYN_K03 = 5,
    KDNA_DYN_K04 = 6,
    KDNA_DYN_K05 = 7,
    KDNA_DYN_EK = 8,
    KDNA_DYN_AK = 9,
    KDNA_DYN_LOCK = 10,
    KDNA_DYN_RAW = 11,
    KDNA_DYN_DOM = 12,
    KDNA_DYN_S01 = 13,
    KDNA_DYN_S02 = 14,
    KDNA_DYN_S03 = 15,
    KDNA_DYN_S04 = 16,
    KDNA_DYN_S05 = 17,
    KDNA_DYN_DOM_SCORE = 18,
    KDNA_DYN_GATE = 19,
    KDNA_DYN_GAIN = 20,
    KDNA_DYN_BIAS = 21,
    KDNA_DYN_DRIVE = 22,
    KDNA_DYN_STEP_ID = 23,
    KDNA_DYN_TIME = 24
};

typedef struct kdna_dyn_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t field_count;
    uint32_t reserved0;

    uint64_t n;
    uint64_t steps;
    uint64_t source_step_count;
    uint64_t seed;

    double dt;
    double x_min;
    double x_max;
    double dx;

    double coupling;
    double drive_pull;

    uint64_t payload_bytes;
    uint64_t flags;
    uint64_t reserved1;
} kdna_dyn_header;

typedef char kdna_dyn_header_size_must_be_128[(sizeof(kdna_dyn_header) == KDNA_DYN_HEADER_BYTES) ? 1 : -1];

static inline size_t kdna_dyn_idx(uint32_t field, size_t n, size_t i) {
    return ((size_t)field * n) + i;
}

static inline uint64_t kdna_dyn_payload_bytes_inline(uint64_t n) {
    return n * (uint64_t)KDNA_DYN_FIELDS * (uint64_t)sizeof(double);
}

const char *kdna_dyn_field_name_local(uint32_t field);

#ifdef __cplusplus
}
#endif

#endif
