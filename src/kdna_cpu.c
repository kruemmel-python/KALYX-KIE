#include "kdna.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static double kdna_clamp(double x, double a, double b) {
    return x < a ? a : (x > b ? b : x);
}

static double kdna_log_abs(double x, const kdna_constants *c) {
    return log(fabs(x) + c->eps);
}

static double kdna_exp_clamp(double x, const kdna_constants *c) {
    return exp(kdna_clamp(x, c->exp_min, c->exp_max));
}

static double kdna_safe_div(double n, double d, const kdna_constants *c) {
    const double s = signbit(d) ? -1.0 : 1.0; /* handles -0 exactly as specified */
    const double ad = fabs(d);
    return n / (s * (ad > c->eps ? ad : c->eps));
}

static double kdna_finite_or_zero(double x, const kdna_constants *c) {
    return (isfinite(x) && fabs(x) < c->hard_max) ? x : 0.0;
}

void kdna_default_constants(kdna_constants *c) {
    if (!c) return;
    c->cA =  0.579366;
    c->cB =  1.93491;
    c->cC = -0.431022;
    c->cD = -3.01298;
    c->cE = -0.0388623;
    c->eps = 1.0e-12;
    c->exp_min = -60.0;
    c->exp_max =  60.0;
    c->hard_max = 1.0e100;
}

const char *kdna_status_str(int code) {
    switch (code) {
        case KDNA_OK: return "ok";
        case KDNA_EINVAL: return "invalid argument";
        case KDNA_ENOMEM: return "out of memory";
        case KDNA_EOPENCL: return "OpenCL runtime error";
        case KDNA_ENO_DEVICE: return "no OpenCL device/platform";
        case KDNA_EIO: return "I/O error";
        case KDNA_EBUILD: return "OpenCL program build failed";
        default: return "unknown error";
    }
}

static int kdna_validate(const double *x, double *out, size_t n, const kdna_constants *c) {
    if (!x || !out || !c || n == 0u) return KDNA_EINVAL;
    if (!(c->eps > 0.0) || !(c->exp_min < c->exp_max) || !(c->hard_max > 0.0)) return KDNA_EINVAL;
    return KDNA_OK;
}

static void kdna_eval_one(double x, const kdna_constants *c, double *v) {
    const double D = c->cD - tanh(c->cD);

    const double r = kdna_safe_div(c->cA - (x + tanh(x)), x, c);
    const double k1 = kdna_finite_or_zero(
        sin(kdna_exp_clamp(kdna_log_abs(c->cA, c), c)) *
        kdna_safe_div(tanh(c->cB), kdna_exp_clamp(r, c), c),
        c);

    const double k2 = kdna_finite_or_zero(
        kdna_log_abs(kdna_exp_clamp(tanh(cos(x)), c) * x, c),
        c);

    const double folded = cos(sin(kdna_safe_div(x * x, x, c)));
    const double k3 = kdna_finite_or_zero(
        cos(-c->cC * kdna_safe_div(folded, D, c)),
        c);

    const double exp_cE = kdna_exp_clamp(c->cE, c);
    const double exp_x = kdna_exp_clamp(x, c);
    const double a = sin(cos(x * exp_cE));
    const double b = kdna_exp_clamp(exp_x, c);
    const double cx = kdna_log_abs(x * cos(cos(exp_x)), c);
    const double k4 = kdna_finite_or_zero(
        tanh(kdna_safe_div(a, b, c) * cx),
        c);

    const double cascade = (x + x * tanh(x)) * kdna_log_abs(x * cos(cos(exp_x)), c);
    const double k5 = kdna_finite_or_zero(
        tanh(kdna_safe_div(cascade, D, c)) * exp_cE,
        c);

    const double e = sqrt(k1*k1 + k2*k2 + k3*k3 + k4*k4 + k5*k5);
    const double attr = (k1*1.0 + k2*2.0 + k3*3.0 + k4*4.0 + k5*5.0) / 5.0;
    const double lock = tanh(e + fabs(attr));

    const double ak[5] = { fabs(k1), fabs(k2), fabs(k3), fabs(k4), fabs(k5) };
    uint32_t raw = 1u;
    double raw_score = ak[0];
    for (uint32_t i = 1u; i < 5u; ++i) {
        if (ak[i] > raw_score) { raw_score = ak[i]; raw = i + 1u; }
    }

    const double s1 = fabs(k1) / 1.70;
    const double s2 = fabs(k2) / 0.45;
    const double s3 = fabs(k3 - 0.985) / 0.006;
    const double s4 = fabs(k4) / 0.035;
    const double s5 = fabs(k5) / 0.55;
    const double ss[5] = { s1, s2, s3, s4, s5 };
    uint32_t dom = 1u;
    double dom_score = ss[0];
    for (uint32_t i = 1u; i < 5u; ++i) {
        if (ss[i] > dom_score) { dom_score = ss[i]; dom = i + 1u; }
    }

    v[KDNA_X] = x;
    v[KDNA_K01] = k1;
    v[KDNA_K02] = k2;
    v[KDNA_K03] = k3;
    v[KDNA_K04] = k4;
    v[KDNA_K05] = k5;
    v[KDNA_EK] = e;
    v[KDNA_AK] = attr;
    v[KDNA_LK] = lock;
    v[KDNA_RAW] = (double)raw;
    v[KDNA_DOM] = (double)dom;
    v[KDNA_S01] = s1;
    v[KDNA_S02] = s2;
    v[KDNA_S03] = s3;
    v[KDNA_S04] = s4;
    v[KDNA_S05] = s5;
    v[KDNA_DOM_SCORE] = dom_score;
}

int kdna_eval_cpu(const double *x, double *out, size_t n, const kdna_constants *constants) {
    kdna_constants local;
    if (!constants) {
        kdna_default_constants(&local);
        constants = &local;
    }
    int rc = kdna_validate(x, out, n, constants);
    if (rc != KDNA_OK) return rc;

    for (size_t i = 0u; i < n; ++i) {
        double v[KDNA_FIELDS];
        kdna_eval_one(x[i], constants, v);
        for (uint32_t f = 0u; f < KDNA_FIELDS; ++f) {
            out[kdna_idx(f, n, i)] = v[f];
        }
    }
    return KDNA_OK;
}
