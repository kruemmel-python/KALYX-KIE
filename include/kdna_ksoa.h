#ifndef KDNA_KSOA_H
#define KDNA_KSOA_H

#include "kdna.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KSOA_MAGIC "KSOA0001"
#define KDNA_KSOA_VERSION 1u
#define KDNA_KSOA_HEADER_BYTES 128u
#define KDNA_KSOA_FLAG_LE_IEEE754_DOUBLE 1ull

enum kdna_ksoa_backend {
    KDNA_KSOA_BACKEND_CPU = 0,
    KDNA_KSOA_BACKEND_OPENCL = 1
};

/*
  On-disk KSOA v1 header.

  Binary contract:
    - exactly 128 bytes
    - little-endian scalar fields
    - payload starts immediately after header
    - payload is double out[field * n + i]
    - fields count must be KDNA_FIELDS
    - flags bit0 means little-endian IEEE-754 double payload

  This struct is intentionally fixed-width. Do not append fields without
  bumping KDNA_KSOA_VERSION and preserving the first 128 bytes contract.
*/
typedef struct kdna_ksoa_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t fields;
    uint32_t backend;
    uint64_t n;
    double x_min;
    double x_max;
    double dx;
    uint64_t payload_bytes;
    uint64_t flags;
    uint8_t reserved[56];
} kdna_ksoa_header;

int kdna_ksoa_host_is_little_endian(void);
const char *kdna_ksoa_backend_name(uint32_t backend);

int kdna_ksoa_payload_bytes(size_t n, uint64_t *bytes_out);
int kdna_ksoa_init_header(kdna_ksoa_header *h,
                          size_t n,
                          double x_min,
                          double x_max,
                          double dx,
                          uint32_t backend);

int kdna_ksoa_validate_header(const kdna_ksoa_header *h);
int kdna_ksoa_write_file(const char *path,
                         const kdna_ksoa_header *h,
                         const double *payload);
int kdna_ksoa_read_header_file(const char *path, kdna_ksoa_header *h);

#ifdef __cplusplus
}
#endif

#endif
