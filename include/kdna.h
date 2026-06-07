#ifndef KDNA_H
#define KDNA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_OPS 5u
#define KDNA_FIELDS 17u

enum kdna_field {
    KDNA_X = 0,
    KDNA_K01 = 1,
    KDNA_K02 = 2,
    KDNA_K03 = 3,
    KDNA_K04 = 4,
    KDNA_K05 = 5,
    KDNA_EK = 6,
    KDNA_AK = 7,
    KDNA_LK = 8,
    KDNA_RAW = 9,
    KDNA_DOM = 10,
    KDNA_S01 = 11,
    KDNA_S02 = 12,
    KDNA_S03 = 13,
    KDNA_S04 = 14,
    KDNA_S05 = 15,
    KDNA_DOM_SCORE = 16
};

enum kdna_status {
    KDNA_OK = 0,
    KDNA_EINVAL = -1,
    KDNA_ENOMEM = -2,
    KDNA_EOPENCL = -3,
    KDNA_ENO_DEVICE = -4,
    KDNA_EIO = -5,
    KDNA_EBUILD = -6
};

typedef struct kdna_constants {
    double cA;
    double cB;
    double cC;
    double cD;
    double cE;
    double eps;
    double exp_min;
    double exp_max;
    double hard_max;
} kdna_constants;

static inline size_t kdna_idx(uint32_t field, size_t n, size_t i) {
    return ((size_t)field * n) + i;
}

void kdna_default_constants(kdna_constants *c);
const char *kdna_status_str(int code);

/*
  out is plane-major / SoA:
    out[field * n + i]
  fields are kdna_field.
  RAW and DOM are encoded as 1..5 in double cells to keep one flat ABI buffer.
*/
int kdna_eval_cpu(const double *x, double *out, size_t n, const kdna_constants *constants);
int kdna_eval_opencl(const double *x, double *out, size_t n, const kdna_constants *constants, const char *kernel_path);

#ifdef __cplusplus
}
#endif

#endif
