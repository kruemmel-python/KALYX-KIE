#ifndef KDNA_KGEN_H
#define KDNA_KGEN_H

#include "kdna.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KGEN_MAGIC "KGEN0001"
#define KDNA_KGEN_VERSION 1u
#define KDNA_KGEN_HEADER_BYTES 128u
#define KDNA_KGEN_FIELDS 32u

#define KDNA_KGEN_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KGEN_FLAG_SUBQG_GENESIS     2ull
#define KDNA_KGEN_FLAG_DETERMINISTIC     4ull
#define KDNA_KGEN_FLAG_RESIDENT_WAVES    8ull

enum kdna_kgen_field {
    KDNA_KGEN_E = 0,
    KDNA_KGEN_PHI = 1,
    KDNA_KGEN_X = 2,
    KDNA_KGEN_DE = 3,
    KDNA_KGEN_DPHI = 4,
    KDNA_KGEN_PHASE_VELOCITY = 5,
    KDNA_KGEN_ENERGY_VELOCITY = 6,
    KDNA_KGEN_COUPLING = 7,
    KDNA_KGEN_RESONANCE = 8,
    KDNA_KGEN_K01 = 9,
    KDNA_KGEN_K02 = 10,
    KDNA_KGEN_K03 = 11,
    KDNA_KGEN_K04 = 12,
    KDNA_KGEN_K05 = 13,
    KDNA_KGEN_EK = 14,
    KDNA_KGEN_AK = 15,
    KDNA_KGEN_LOCK = 16,
    KDNA_KGEN_RAW = 17,
    KDNA_KGEN_DOM = 18,
    KDNA_KGEN_S01 = 19,
    KDNA_KGEN_S02 = 20,
    KDNA_KGEN_S03 = 21,
    KDNA_KGEN_S04 = 22,
    KDNA_KGEN_S05 = 23,
    KDNA_KGEN_DOM_SCORE = 24,
    KDNA_KGEN_VARIANT_ID = 25,
    KDNA_KGEN_RESONANCE_ID = 26,
    KDNA_KGEN_TIME = 27,
    KDNA_KGEN_GATE = 28,
    KDNA_KGEN_INJECTION_POTENTIAL = 29,
    KDNA_KGEN_STABILITY = 30,
    KDNA_KGEN_ALIGNMENT = 31
};

typedef struct kdna_kgen_params {
    size_t n;
    size_t steps;
    double dt;
    double x_min;
    double x_max;
    uint64_t seed;
    double sigma;
    double energy_coupling;
    double phase_coupling;
    double damping;
    double drive;
    double resonance_threshold;
} kdna_kgen_params;

typedef struct kdna_kgen_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t field_count;
    uint32_t reserved0;
    uint64_t n;
    uint64_t steps;
    uint64_t seed;
    double dt;
    double x_min;
    double x_max;
    double dx;
    double sigma;
    double energy_coupling;
    double phase_coupling;
    double resonance_threshold;
    uint64_t payload_bytes;
    uint64_t flags;
} kdna_kgen_header;

typedef char kdna_kgen_header_size_must_be_128[(sizeof(kdna_kgen_header) == KDNA_KGEN_HEADER_BYTES) ? 1 : -1];

static inline size_t kdna_kgen_idx(uint32_t field, size_t n, size_t i) {
    return ((size_t)field * n) + i;
}

void kdna_kgen_default_params(kdna_kgen_params *p);
const char *kdna_kgen_field_name(uint32_t field);

int kdna_kgen_payload_bytes(size_t n, uint64_t *bytes_out);
int kdna_kgen_init_header(kdna_kgen_header *h, const kdna_kgen_params *p);
int kdna_kgen_validate_header(const kdna_kgen_header *h);
int kdna_kgen_write_file(const char *path, const kdna_kgen_header *h, const double *payload);
int kdna_kgen_read_header_file(const char *path, kdna_kgen_header *h);
int kdna_kgen_read_file(const char *path, kdna_kgen_header *h, double **payload_out);

int kdna_kgen_eval_cpu(const kdna_kgen_params *p, const kdna_constants *c, double *out);
int kdna_kgen_eval_opencl(const kdna_kgen_params *p, const kdna_constants *c, const char *kernel_path, double *out);

uint64_t kdna_kgen_variant_id(uint32_t raw, uint32_t dom, double lock, double score,
                              double k1, double k2, double k3, double k4, double k5);

#ifdef __cplusplus
}
#endif

#endif
