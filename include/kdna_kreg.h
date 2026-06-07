#ifndef KDNA_KREG_H
#define KDNA_KREG_H

#include "kdna.h"
#include "kdna_ksoa.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KREG_MAGIC "KREG0001"
#define KDNA_KREG_VERSION 1u
#define KDNA_KREG_HEADER_BYTES 128u
#define KDNA_KREG_RECORD_BYTES 80u
#define KDNA_KREG_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KREG_FLAG_SOURCE_KSOA_V1 2ull

/*
  KREG v1: discrete semantic topology extracted from a KSOA scan.

  A segment is a maximal contiguous interval in scan index-space where the
  selected split criterion is stable. The default extractor criterion is the
  pair (RAW, D), therefore records encode connected regions of identical
  raw and activation-normalized dominance.

  Binary contract:
    - exactly 128-byte header
    - exactly 80-byte records
    - little-endian fixed-width integers and IEEE-754 doubles
    - records are ordered by i0 ascending
    - records must cover the original scan without gaps:
        first.i0 == 0
        last.i1 == source_n - 1
        record[k].i1 + 1 == record[k+1].i0
*/
typedef struct kdna_kreg_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t source_fields;
    uint64_t segment_count;
    uint64_t source_n;
    double x_min;
    double x_max;
    double dx;
    uint64_t payload_bytes;
    uint64_t flags;
    uint8_t reserved[48];
} kdna_kreg_header;

typedef struct kdna_kreg_record {
    uint64_t i0;
    uint64_t i1;
    double x0;
    double x1;
    uint32_t raw;
    uint32_t dom;
    uint32_t flags;
    uint32_t reserved0;
    double lock_min;
    double lock_max;
    double score_min;
    double score_max;
} kdna_kreg_record;

typedef char kdna_kreg_header_size_must_be_128[(sizeof(kdna_kreg_header) == KDNA_KREG_HEADER_BYTES) ? 1 : -1];
typedef char kdna_kreg_record_size_must_be_80[(sizeof(kdna_kreg_record) == KDNA_KREG_RECORD_BYTES) ? 1 : -1];

int kdna_kreg_payload_bytes(size_t segment_count, uint64_t *bytes_out);
int kdna_kreg_init_header(kdna_kreg_header *h,
                          size_t segment_count,
                          size_t source_n,
                          double x_min,
                          double x_max,
                          double dx);
int kdna_kreg_validate_header(const kdna_kreg_header *h);
int kdna_kreg_write_file(const char *path,
                         const kdna_kreg_header *h,
                         const kdna_kreg_record *records);
int kdna_kreg_read_header_file(const char *path, kdna_kreg_header *h);

#ifdef __cplusplus
}
#endif

#endif
