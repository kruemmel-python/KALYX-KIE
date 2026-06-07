#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define KDNA_FIELDS 17u
#define KDNA_X 0u
#define KDNA_K01 1u
#define KDNA_K02 2u
#define KDNA_K03 3u
#define KDNA_K04 4u
#define KDNA_K05 5u
#define KDNA_EK 6u
#define KDNA_AK 7u
#define KDNA_LK 8u
#define KDNA_RAW 9u
#define KDNA_DOM 10u
#define KDNA_S01 11u
#define KDNA_S02 12u
#define KDNA_S03 13u
#define KDNA_S04 14u
#define KDNA_S05 15u
#define KDNA_DOM_SCORE 16u

static double kdna_clamp(double x, double a, double b) {
    return x < a ? a : (x > b ? b : x);
}

static double kdna_log_abs(double x, double eps) {
    return log(fabs(x) + eps);
}

static double kdna_exp_clamp(double x, double exp_min, double exp_max) {
    return exp(kdna_clamp(x, exp_min, exp_max));
}

static double kdna_safe_div(double n, double d, double eps) {
    const double s = signbit(d) ? -1.0 : 1.0;
    const double ad = fabs(d);
    return n / (s * (ad > eps ? ad : eps));
}

static double kdna_finite_or_zero(double x, double hard_max) {
    return (isfinite(x) && fabs(x) < hard_max) ? x : 0.0;
}

__kernel void kdna_eval_kernel(
    __global const double *xv,
    __global double *out,
    const ulong n,
    const double cA,
    const double cB,
    const double cC,
    const double cD,
    const double cE,
    const double eps,
    const double exp_min,
    const double exp_max,
    const double hard_max
) {
    const ulong i = (ulong)get_global_id(0);
    if (i >= n) return;

    const double x = xv[i];
    const double D = cD - tanh(cD);

    const double r = kdna_safe_div(cA - (x + tanh(x)), x, eps);
    const double k1 = kdna_finite_or_zero(
        sin(kdna_exp_clamp(kdna_log_abs(cA, eps), exp_min, exp_max)) *
        kdna_safe_div(tanh(cB), kdna_exp_clamp(r, exp_min, exp_max), eps),
        hard_max);

    const double k2 = kdna_finite_or_zero(
        kdna_log_abs(kdna_exp_clamp(tanh(cos(x)), exp_min, exp_max) * x, eps),
        hard_max);

    const double folded = cos(sin(kdna_safe_div(x * x, x, eps)));
    const double k3 = kdna_finite_or_zero(
        cos(-cC * kdna_safe_div(folded, D, eps)),
        hard_max);

    const double exp_cE = kdna_exp_clamp(cE, exp_min, exp_max);
    const double exp_x = kdna_exp_clamp(x, exp_min, exp_max);
    const double a = sin(cos(x * exp_cE));
    const double b = kdna_exp_clamp(exp_x, exp_min, exp_max);
    const double cx = kdna_log_abs(x * cos(cos(exp_x)), eps);
    const double k4 = kdna_finite_or_zero(
        tanh(kdna_safe_div(a, b, eps) * cx),
        hard_max);

    const double cascade = (x + x * tanh(x)) * kdna_log_abs(x * cos(cos(exp_x)), eps);
    const double k5 = kdna_finite_or_zero(
        tanh(kdna_safe_div(cascade, D, eps)) * exp_cE,
        hard_max);

    const double e = sqrt(k1*k1 + k2*k2 + k3*k3 + k4*k4 + k5*k5);
    const double attr = (k1*1.0 + k2*2.0 + k3*3.0 + k4*4.0 + k5*5.0) / 5.0;
    const double lock = tanh(e + fabs(attr));

    double ak0 = fabs(k1), ak1 = fabs(k2), ak2 = fabs(k3), ak3 = fabs(k4), ak4 = fabs(k5);
    uint raw = 1u;
    double raw_score = ak0;
    if (ak1 > raw_score) { raw_score = ak1; raw = 2u; }
    if (ak2 > raw_score) { raw_score = ak2; raw = 3u; }
    if (ak3 > raw_score) { raw_score = ak3; raw = 4u; }
    if (ak4 > raw_score) { raw_score = ak4; raw = 5u; }

    const double s1 = fabs(k1) / 1.70;
    const double s2 = fabs(k2) / 0.45;
    const double s3 = fabs(k3 - 0.985) / 0.006;
    const double s4 = fabs(k4) / 0.035;
    const double s5 = fabs(k5) / 0.55;

    uint dom = 1u;
    double dom_score = s1;
    if (s2 > dom_score) { dom_score = s2; dom = 2u; }
    if (s3 > dom_score) { dom_score = s3; dom = 3u; }
    if (s4 > dom_score) { dom_score = s4; dom = 4u; }
    if (s5 > dom_score) { dom_score = s5; dom = 5u; }

    out[KDNA_X * n + i] = x;
    out[KDNA_K01 * n + i] = k1;
    out[KDNA_K02 * n + i] = k2;
    out[KDNA_K03 * n + i] = k3;
    out[KDNA_K04 * n + i] = k4;
    out[KDNA_K05 * n + i] = k5;
    out[KDNA_EK * n + i] = e;
    out[KDNA_AK * n + i] = attr;
    out[KDNA_LK * n + i] = lock;
    out[KDNA_RAW * n + i] = (double)raw;
    out[KDNA_DOM * n + i] = (double)dom;
    out[KDNA_S01 * n + i] = s1;
    out[KDNA_S02 * n + i] = s2;
    out[KDNA_S03 * n + i] = s3;
    out[KDNA_S04 * n + i] = s4;
    out[KDNA_S05 * n + i] = s5;
    out[KDNA_DOM_SCORE * n + i] = dom_score;
}
