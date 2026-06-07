#ifndef KDNA_KVAR_H
#define KDNA_KVAR_H

#include "kdna.h"
#include "kdna_kgen.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KVAR_MAGIC "KVAR0001"
#define KDNA_KVAR_VERSION 1u
#define KDNA_KVAR_HEADER_BYTES 128u
#define KDNA_KVAR_RECORD_BYTES 512u

#define KDNA_KVAR_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KVAR_FLAG_SOURCE_KGEN_V1    2ull
#define KDNA_KVAR_FLAG_VARIANT_DNA       4ull
#define KDNA_KVAR_FLAG_ADJACENCY_INDEX   8ull

#define KDNA_KVAR_REC_FLAG_RESONANT              0x00000001u
#define KDNA_KVAR_REC_FLAG_HIGH_INJECTION        0x00000002u
#define KDNA_KVAR_REC_FLAG_HIGH_STABILITY        0x00000004u
#define KDNA_KVAR_REC_FLAG_RAW_DOM_MISMATCH      0x00000008u
#define KDNA_KVAR_REC_FLAG_REPEATED              0x00000010u
#define KDNA_KVAR_REC_FLAG_HAS_PREDECESSOR       0x00000020u
#define KDNA_KVAR_REC_FLAG_HAS_SUCCESSOR         0x00000040u
#define KDNA_KVAR_REC_FLAG_NULL_NEAR             0x00000080u

/*
  KVAR v1: variant-DNA archive compiled from KGEN Genesis fields.

  KGEN stores resonance points. KVAR groups identical variant_id values into
  stable variant records. Each record aggregates the KDNA statistics, SUBQG
  resonance metrics, first/last occurrence, and local adjacency dominance.

  Contract:
    - 128-byte header
    - 512-byte record
    - source must be KGEN v1
    - records sorted by variant_id ascending
    - little-endian fixed-width integers and IEEE-754 doubles
*/
typedef struct kdna_kvar_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t reserved0;

    uint64_t variant_count;
    uint64_t source_n;
    uint64_t source_steps;
    uint64_t source_seed;
    uint64_t sample_count;
    uint64_t payload_bytes;
    uint64_t flags;

    double x_min;
    double x_max;
    double time_value;
    double resonance_threshold;

    uint8_t reserved[16];
} kdna_kvar_header;

typedef struct kdna_kvar_record {
    uint64_t variant_id;
    uint64_t resonance_id_min;
    uint64_t resonance_id_max;
    uint64_t first_i;
    uint64_t last_i;
    uint64_t sample_count;
    uint64_t predecessor_id;
    uint64_t successor_id;
    uint64_t predecessor_count;
    uint64_t successor_count;

    uint32_t raw;
    uint32_t dom;
    uint32_t flags;
    uint32_t reserved0;

    double x_min;
    double x_max;
    double x_mean;
    double e_mean;
    double phi_mean;
    double resonance_min;
    double resonance_max;
    double resonance_mean;
    double injection_mean;
    double injection_max;
    double stability_min;
    double stability_max;
    double stability_mean;
    double alignment_mean;
    double lock_min;
    double lock_max;
    double lock_mean;
    double score_mean;
    double score_max;
    double time_mean;
    double centroid_i;
    double span_i;

    double k_mean[5];
    double k_absmax[5];
    double s_mean[5];
    double s_absmax[5];

    uint8_t reserved[80];
} kdna_kvar_record;

typedef char kdna_kvar_header_size_must_be_128[(sizeof(kdna_kvar_header) == KDNA_KVAR_HEADER_BYTES) ? 1 : -1];
typedef char kdna_kvar_record_size_must_be_512[(sizeof(kdna_kvar_record) == KDNA_KVAR_RECORD_BYTES) ? 1 : -1];

int kdna_kvar_payload_bytes(size_t variant_count, uint64_t *bytes_out);
int kdna_kvar_init_header(kdna_kvar_header *h,
                          size_t variant_count,
                          const kdna_kgen_header *source);
int kdna_kvar_validate_header(const kdna_kvar_header *h);
int kdna_kvar_write_file(const char *path,
                         const kdna_kvar_header *h,
                         const kdna_kvar_record *records);
int kdna_kvar_read_header_file(const char *path, kdna_kvar_header *h);
int kdna_kvar_read_file(const char *path,
                        kdna_kvar_header *h,
                        kdna_kvar_record **records_out);

int kdna_kvar_build_from_kgen(const kdna_kgen_header *source_header,
                              const double *source_payload,
                              kdna_kvar_header *out_header,
                              kdna_kvar_record **records_out);

const char *kdna_kvar_record_flag_name(uint32_t flag);

#ifdef __cplusplus
}
#endif

#endif
