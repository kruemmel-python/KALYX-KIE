#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define KDNA_KGEN_FIELDS 32u
#define KDNA_KGEN_E 0u
#define KDNA_KGEN_PHI 1u
#define KDNA_KGEN_X 2u
#define KDNA_KGEN_DE 3u
#define KDNA_KGEN_DPHI 4u
#define KDNA_KGEN_PHASE_VELOCITY 5u
#define KDNA_KGEN_ENERGY_VELOCITY 6u
#define KDNA_KGEN_COUPLING 7u
#define KDNA_KGEN_RESONANCE 8u
#define KDNA_KGEN_K01 9u
#define KDNA_KGEN_K02 10u
#define KDNA_KGEN_K03 11u
#define KDNA_KGEN_K04 12u
#define KDNA_KGEN_K05 13u
#define KDNA_KGEN_EK 14u
#define KDNA_KGEN_AK 15u
#define KDNA_KGEN_LOCK 16u
#define KDNA_KGEN_RAW 17u
#define KDNA_KGEN_DOM 18u
#define KDNA_KGEN_S01 19u
#define KDNA_KGEN_S02 20u
#define KDNA_KGEN_S03 21u
#define KDNA_KGEN_S04 22u
#define KDNA_KGEN_S05 23u
#define KDNA_KGEN_DOM_SCORE 24u
#define KDNA_KGEN_VARIANT_ID 25u
#define KDNA_KGEN_RESONANCE_ID 26u
#define KDNA_KGEN_TIME 27u
#define KDNA_KGEN_GATE 28u
#define KDNA_KGEN_INJECTION_POTENTIAL 29u
#define KDNA_KGEN_STABILITY 30u
#define KDNA_KGEN_ALIGNMENT 31u

static double clampd(double x, double a, double b) { return x < a ? a : (x > b ? b : x); }
static double finite0(double x, double hard_max) { return (isfinite(x) && fabs(x) < hard_max) ? x : 0.0; }
static double log_abs(double x, double eps) { return log(fabs(x) + eps); }
static double exp_clamp(double x, double mn, double mx) { return exp(clampd(x, mn, mx)); }
static double safe_div(double n, double d, double eps) {
    double s = signbit(d) ? -1.0 : 1.0;
    double ad = fabs(d);
    return n / (s * (ad > eps ? ad : eps));
}
static ulong hash64(ulong x) {
    x ^= x >> 30; x *= (ulong)0xbf58476d1ce4e5b9UL;
    x ^= x >> 27; x *= (ulong)0x94d049bb133111ebUL;
    x ^= x >> 31;
    return x;
}
static double unit_noise(ulong seed, ulong i) {
    ulong h = hash64(seed ^ (i * (ulong)0x9e3779b97f4a7c15UL));
    double u = (double)(h >> 11) * (1.0 / 9007199254740992.0);
    return 2.0 * u - 1.0;
}
static void eval_k(double x, double cA, double cB, double cC, double cD, double cE,
                   double eps, double exp_min, double exp_max, double hard_max,
                   __private double *v) {
    double D = cD - tanh(cD);
    double r = safe_div(cA - (x + tanh(x)), x, eps);
    double k1 = finite0(sin(exp_clamp(log_abs(cA, eps), exp_min, exp_max)) * safe_div(tanh(cB), exp_clamp(r, exp_min, exp_max), eps), hard_max);
    double k2 = finite0(log_abs(exp_clamp(tanh(cos(x)), exp_min, exp_max) * x, eps), hard_max);
    double folded = cos(sin(safe_div(x * x, x, eps)));
    double k3 = finite0(cos(-cC * safe_div(folded, D, eps)), hard_max);
    double exp_cE = exp_clamp(cE, exp_min, exp_max);
    double exp_x = exp_clamp(x, exp_min, exp_max);
    double a = sin(cos(x * exp_cE));
    double b = exp_clamp(exp_x, exp_min, exp_max);
    double cx = log_abs(x * cos(cos(exp_x)), eps);
    double k4 = finite0(tanh(safe_div(a, b, eps) * cx), hard_max);
    double cascade = (x + x * tanh(x)) * log_abs(x * cos(cos(exp_x)), eps);
    double k5 = finite0(tanh(safe_div(cascade, D, eps)) * exp_cE, hard_max);
    double ek = sqrt(k1*k1 + k2*k2 + k3*k3 + k4*k4 + k5*k5);
    double ak = (k1 + 2.0*k2 + 3.0*k3 + 4.0*k4 + 5.0*k5) / 5.0;
    double lock = tanh(ek + fabs(ak));
    double s1 = fabs(k1) / 1.70;
    double s2 = fabs(k2) / 0.45;
    double s3 = fabs(k3 - 0.985) / 0.006;
    double s4 = fabs(k4) / 0.035;
    double s5 = fabs(k5) / 0.55;
    uint raw = 1u; double rs = fabs(k1);
    if (fabs(k2) > rs) { rs = fabs(k2); raw = 2u; }
    if (fabs(k3) > rs) { rs = fabs(k3); raw = 3u; }
    if (fabs(k4) > rs) { rs = fabs(k4); raw = 4u; }
    if (fabs(k5) > rs) { rs = fabs(k5); raw = 5u; }
    uint dom = 1u; double ds = s1;
    if (s2 > ds) { ds = s2; dom = 2u; }
    if (s3 > ds) { ds = s3; dom = 3u; }
    if (s4 > ds) { ds = s4; dom = 4u; }
    if (s5 > ds) { ds = s5; dom = 5u; }
    v[0]=k1; v[1]=k2; v[2]=k3; v[3]=k4; v[4]=k5; v[5]=ek; v[6]=ak; v[7]=lock;
    v[8]=(double)raw; v[9]=(double)dom; v[10]=s1; v[11]=s2; v[12]=s3; v[13]=s4; v[14]=s5; v[15]=ds;
}
static ulong variant_id(uint raw, uint dom, double lock, double score, double k1, double k2, double k3, double k4, double k5) {
    long ql = (long)floor(lock * 4096.0);
    long qs = (long)floor(log1p(fabs(score)) * 2048.0);
    long q0 = (long)floor(tanh(k1) * 32767.0);
    long q1 = (long)floor(tanh(k2) * 32767.0);
    long q2 = (long)floor(tanh(k3) * 32767.0);
    long q3 = (long)floor(tanh(k4) * 32767.0);
    long q4 = (long)floor(tanh(k5) * 32767.0);
    ulong h = (ulong)0x4b444e415f564152UL ^ (ulong)raw ^ ((ulong)dom << 8);
    h = hash64(h ^ (ulong)ql); h = hash64(h ^ ((ulong)qs << 1));
    h = hash64(h ^ (ulong)q0); h = hash64(h ^ (ulong)q1 ^ ((ulong)1 << 56));
    h = hash64(h ^ (ulong)q2 ^ ((ulong)2 << 56)); h = hash64(h ^ (ulong)q3 ^ ((ulong)3 << 56));
    h = hash64(h ^ (ulong)q4 ^ ((ulong)4 << 56));
    return h;
}

__kernel void kdna_kgen_kernel(
    __global double *out,
    const ulong n,
    const ulong steps,
    const double dt,
    const double x_min,
    const double x_max,
    const ulong seed,
    const double sigma,
    const double energy_coupling,
    const double phase_coupling,
    const double damping,
    const double drive,
    const double resonance_threshold,
    const double cA, const double cB, const double cC, const double cD, const double cE,
    const double eps, const double exp_min, const double exp_max, const double hard_max
) {
    ulong i = (ulong)get_global_id(0);
    if (i >= n) return;
    double dx = n > 1UL ? (x_max - x_min) / (double)(n - 1UL) : 0.0;
    double q = x_min + dx * (double)i;
    double no = unit_noise(seed, i);
    double ph = 0.17 * (double)i + 0.013 * no;
    double E = q + 0.35 * sin(ph) + 0.06 * no;
    double Phi = q + 0.35 * cos(ph * 0.73) - 0.04 * no;
    double dE = 0.02 * cos(ph);
    double dPhi = -0.02 * sin(ph * 0.73);
    double coupling_last = 0.0;
    for (ulong t = 0UL; t < steps; ++t) {
        double x = 0.5 * (E + Phi);
        double sg = sigma > 1.0e-12 ? sigma : 1.0e-12;
        double align = exp(-fabs(E - Phi) / sg);
        double harmonic = sin(0.031 * (double)t + 0.007 * (double)i);
        double coupling = energy_coupling * align + drive * harmonic;
        double ndE = dE + dt * (-damping*dE + coupling - 0.003 * E + 0.018 * sin(Phi));
        double ndPhi = dPhi + dt * (-damping*dPhi + phase_coupling * (E - Phi) + 0.011 * cos(x));
        E += dt * ndE; Phi += dt * ndPhi;
        dE = finite0(ndE, hard_max); dPhi = finite0(ndPhi, hard_max);
        E = finite0(E, hard_max); Phi = finite0(Phi, hard_max);
        coupling_last = coupling;
    }
    double x = 0.5 * (E + Phi);
    double k[16]; eval_k(x, cA,cB,cC,cD,cE,eps,exp_min,exp_max,hard_max,k);
    double sg = sigma > 1.0e-12 ? sigma : 1.0e-12;
    double alignment = exp(-fabs(E - Phi) / sg);
    double gate = tanh(log1p(fabs(k[15])) * alignment);
    double resonance = alignment * k[7] * tanh(log1p(fabs(k[15])));
    double injection = resonance * (0.5 + 0.5 * k[7]);
    double stability = tanh(k[7] + alignment + 1.0/(1.0 + fabs(dE) + fabs(dPhi)));
    ulong vid = variant_id((uint)k[8], (uint)k[9], k[7], k[15], k[0], k[1], k[2], k[3], k[4]);
    ulong rid = hash64(vid ^ seed ^ i ^ (steps << 32));
    out[KDNA_KGEN_E*n+i] = E;
    out[KDNA_KGEN_PHI*n+i] = Phi;
    out[KDNA_KGEN_X*n+i] = x;
    out[KDNA_KGEN_DE*n+i] = dE;
    out[KDNA_KGEN_DPHI*n+i] = dPhi;
    out[KDNA_KGEN_PHASE_VELOCITY*n+i] = dPhi;
    out[KDNA_KGEN_ENERGY_VELOCITY*n+i] = dE;
    out[KDNA_KGEN_COUPLING*n+i] = coupling_last;
    out[KDNA_KGEN_RESONANCE*n+i] = resonance;
    for (uint j=0u; j<5u; ++j) out[(KDNA_KGEN_K01+j)*n+i] = k[j];
    out[KDNA_KGEN_EK*n+i] = k[5];
    out[KDNA_KGEN_AK*n+i] = k[6];
    out[KDNA_KGEN_LOCK*n+i] = k[7];
    out[KDNA_KGEN_RAW*n+i] = k[8];
    out[KDNA_KGEN_DOM*n+i] = k[9];
    for (uint j=0u; j<5u; ++j) out[(KDNA_KGEN_S01+j)*n+i] = k[10+j];
    out[KDNA_KGEN_DOM_SCORE*n+i] = k[15];
    out[KDNA_KGEN_VARIANT_ID*n+i] = (double)vid;
    out[KDNA_KGEN_RESONANCE_ID*n+i] = (double)rid;
    out[KDNA_KGEN_TIME*n+i] = (double)steps * dt;
    out[KDNA_KGEN_GATE*n+i] = gate;
    out[KDNA_KGEN_INJECTION_POTENTIAL*n+i] = injection;
    out[KDNA_KGEN_STABILITY*n+i] = stability;
    out[KDNA_KGEN_ALIGNMENT*n+i] = alignment;
    (void)resonance_threshold;
}
