#include "kdna.h"
#include "kdna_dyn.h"
#include "kdna_krun.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct dyn_opts {
    const char *run_path;
    const char *out_path;
    const char *backend;
    const char *kernel;
    const char *a_path;
    const char *b_path;
    size_t n;
    size_t steps;
    double dt;
    uint64_t seed;
    double coupling;
    double drive_pull;
} dyn_opts;

const char *kdna_dyn_field_name_local(uint32_t f) {
    switch (f) {
        case KDNA_DYN_E: return "E";
        case KDNA_DYN_PHI: return "Phi";
        case KDNA_DYN_X: return "x";
        case KDNA_DYN_K01: return "K01";
        case KDNA_DYN_K02: return "K02";
        case KDNA_DYN_K03: return "K03";
        case KDNA_DYN_K04: return "K04";
        case KDNA_DYN_K05: return "K05";
        case KDNA_DYN_EK: return "E_K";
        case KDNA_DYN_AK: return "A_K";
        case KDNA_DYN_LOCK: return "Lock";
        case KDNA_DYN_RAW: return "RAW";
        case KDNA_DYN_DOM: return "D";
        case KDNA_DYN_S01: return "S01";
        case KDNA_DYN_S02: return "S02";
        case KDNA_DYN_S03: return "S03";
        case KDNA_DYN_S04: return "S04";
        case KDNA_DYN_S05: return "S05";
        case KDNA_DYN_DOM_SCORE: return "dominanceScore";
        case KDNA_DYN_GATE: return "gate";
        case KDNA_DYN_GAIN: return "gain";
        case KDNA_DYN_BIAS: return "bias";
        case KDNA_DYN_DRIVE: return "drive";
        case KDNA_DYN_STEP_ID: return "step_id";
        case KDNA_DYN_TIME: return "time";
        default: return "unknown";
    }
}

static void usage(void) {
    fprintf(stderr,
        "kdna_dynamics simulate:\n"
        "  kdna_dynamics --run plan.krun --out out.kdyn --backend cpu|opencl --n N --steps S --dt DT [--seed SEED] [--kernel kernels/kdna_eval.cl]\n"
        "kdna_dynamics inspect/diff:\n"
        "  kdna_dynamics --a file.kdyn [--b other.kdyn]\n");
}

static uint64_t hash64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static double noise01(uint64_t seed, uint64_t i) {
    const uint64_t h = hash64(seed ^ (i * 0x9e3779b97f4a7c15ULL));
    return (double)(h >> 11) * (1.0 / 9007199254740992.0);
}

static double finite0(double x) {
    return (isfinite(x) && fabs(x) < 1.0e100) ? x : 0.0;
}

static int read_krun(const char *path, kdna_krun_header *h, kdna_krun_step_record **steps_out) {
    *steps_out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return KDNA_EIO; }
    int rc = kdna_krun_validate_header(h);
    if (rc != KDNA_OK) { fclose(f); return rc; }
    if (h->step_count == 0u) { fclose(f); return KDNA_EINVAL; }
    kdna_krun_step_record *steps = (kdna_krun_step_record*)malloc((size_t)h->payload_bytes);
    if (!steps) { fclose(f); return KDNA_ENOMEM; }
    if (fread(steps, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) {
        free(steps); fclose(f); return KDNA_EIO;
    }
    fclose(f);
    *steps_out = steps;
    return KDNA_OK;
}

static const kdna_krun_step_record *active_step(const kdna_krun_step_record *steps, size_t step_count, size_t t, size_t total) {
    size_t idx = (t * step_count) / total;
    if (idx >= step_count) idx = step_count - 1u;
    return &steps[idx];
}

static int init_header(kdna_dyn_header *h, const dyn_opts *o, const kdna_krun_header *rh, uint64_t flags) {
    if (!h || !o || !rh || o->n == 0u || o->steps == 0u || !(o->dt > 0.0)) return KDNA_EINVAL;
    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_DYN_MAGIC, 8u);
    h->version = KDNA_DYN_VERSION;
    h->header_bytes = KDNA_DYN_HEADER_BYTES;
    h->field_count = KDNA_DYN_FIELDS;
    h->n = (uint64_t)o->n;
    h->steps = (uint64_t)o->steps;
    h->source_step_count = rh->step_count;
    h->seed = o->seed;
    h->dt = o->dt;
    h->x_min = rh->x_min;
    h->x_max = rh->x_max;
    h->dx = o->n > 1u ? (rh->x_max - rh->x_min) / (double)(o->n - 1u) : 0.0;
    h->coupling = o->coupling;
    h->drive_pull = o->drive_pull;
    h->payload_bytes = kdna_dyn_payload_bytes_inline(h->n);
    h->flags = KDNA_DYN_FLAG_LE_IEEE754_DOUBLE | KDNA_DYN_FLAG_SOURCE_KRUN_V1 |
               KDNA_DYN_FLAG_DETERMINISTIC | KDNA_DYN_FLAG_RESIDENT_UPDATE | flags;
    return KDNA_OK;
}

static int validate_header(const kdna_dyn_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_DYN_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_DYN_VERSION || h->header_bytes != KDNA_DYN_HEADER_BYTES) return KDNA_EINVAL;
    if (h->field_count != KDNA_DYN_FIELDS || h->n == 0u || h->steps == 0u) return KDNA_EINVAL;
    if (h->payload_bytes != kdna_dyn_payload_bytes_inline(h->n)) return KDNA_EINVAL;
    return KDNA_OK;
}

static int write_kdyn(const char *path, const kdna_dyn_header *h, const double *out) {
    if (!path || !h || !out) return KDNA_EINVAL;
    int rc = validate_header(h);
    if (rc != KDNA_OK) return rc;
    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;
    int ok = 1;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) ok = 0;
    if (ok && fwrite(out, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) ok = 0;
    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

static int read_kdyn(const char *path, kdna_dyn_header *h, double **out) {
    *out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return KDNA_EIO; }
    int rc = validate_header(h);
    if (rc != KDNA_OK) { fclose(f); return rc; }
    double *buf = (double*)malloc((size_t)h->payload_bytes);
    if (!buf) { fclose(f); return KDNA_ENOMEM; }
    if (fread(buf, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) {
        free(buf); fclose(f); return KDNA_EIO;
    }
    fclose(f);
    *out = buf;
    return KDNA_OK;
}

static int simulate(const dyn_opts *o) {
    kdna_krun_header rh;
    kdna_krun_step_record *steps = NULL;
    int rc = read_krun(o->run_path, &rh, &steps);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_dynamics: cannot read KRUN '%s': %s\n", o->run_path, kdna_status_str(rc));
        return 2;
    }

    const size_t n = o->n;
    const size_t cells = n * (size_t)KDNA_DYN_FIELDS;
    double *out = (double*)calloc(cells, sizeof(double));
    double *x = (double*)calloc(n, sizeof(double));
    double *e = (double*)calloc(n, sizeof(double));
    double *p = (double*)calloc(n, sizeof(double));
    double *en = (double*)calloc(n, sizeof(double));
    double *pn = (double*)calloc(n, sizeof(double));
    double *kx = (double*)calloc(n * (size_t)KDNA_FIELDS, sizeof(double));
    double *gate = (double*)calloc(n, sizeof(double));
    if (!out || !x || !e || !p || !en || !pn || !kx || !gate) {
        free(steps); free(out); free(x); free(e); free(p); free(en); free(pn); free(kx); free(gate);
        return 2;
    }

    const double dx = n > 1u ? (rh.x_max - rh.x_min) / (double)(n - 1u) : 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double base = rh.x_min + dx * (double)i;
        const double jit = 0.10 * dx * (2.0 * noise01(o->seed, (uint64_t)i) - 1.0);
        x[i] = base + jit;
        e[i] = x[i] + 0.05 * sin(0.071 * (double)i);
        p[i] = x[i] - 0.05 * sin(0.071 * (double)i);
    }

    double last_gain = 0.0, last_bias = 0.0, last_drive = 0.0, last_step_id = 0.0;
    for (size_t t = 0; t < o->steps; ++t) {
        const kdna_krun_step_record *s = active_step(steps, (size_t)rh.step_count, t, o->steps);
        last_gain = s->gain; last_bias = s->bias; last_drive = s->drive_x; last_step_id = (double)s->id;
        const double span = fabs(s->x_end - s->x_start);
        double sigma = 0.18 * (span > fabs(dx) ? span : fabs(dx));
        if (sigma < 4.0 * fabs(dx)) sigma = 4.0 * fabs(dx);
        if (!(sigma > 0.0) || !isfinite(sigma)) sigma = 1.0;
        const double inv2s2 = 1.0 / (2.0 * sigma * sigma);

        kdna_constants c; kdna_default_constants(&c);
        rc = kdna_eval_cpu(x, kx, n, &c);
        if (rc != KDNA_OK) { free(steps); return 2; }

        for (size_t i = 0; i < n; ++i) {
            const size_t il = i == 0u ? n - 1u : i - 1u;
            const size_t ir = i + 1u == n ? 0u : i + 1u;
            gate[i] = exp(-((x[i] - s->drive_x) * (x[i] - s->drive_x)) * inv2s2);
            const double lap_e = e[il] - 2.0 * e[i] + e[ir];
            const double lap_p = p[il] - 2.0 * p[i] + p[ir];
            const double lock = kx[kdna_idx(KDNA_LK, n, i)];
            const double k04 = kx[kdna_idx(KDNA_K04, n, i)];
            const double k05 = kx[kdna_idx(KDNA_K05, n, i)];
            const double k02 = kx[kdna_idx(KDNA_K02, n, i)];

            en[i] = finite0(e[i] + o->dt * (o->coupling * lap_e - 0.035 * e[i] + gate[i] * s->gain * (s->bias - lock) + 0.075 * (k04 - k05)));
            pn[i] = finite0(p[i] + o->dt * (0.05 * lap_p - 0.025 * p[i] + 0.045 * (k02 - p[i]) + 0.01 * gate[i] * log1p(fabs(kx[kdna_idx(KDNA_DOM_SCORE, n, i)]))));
        }
        for (size_t i = 0; i < n; ++i) {
            x[i] = finite0(0.5 * (en[i] + pn[i]) + o->dt * (o->drive_pull * gate[i] * (s->drive_x - x[i]) + 0.02 * kx[kdna_idx(KDNA_K05, n, i)] - 0.015 * x[i]));
            e[i] = en[i];
            p[i] = pn[i];
        }
    }

    kdna_constants c; kdna_default_constants(&c);
    uint64_t backend_flag = 0u;
    if (strcmp(o->backend, "opencl") == 0) {
        rc = kdna_eval_opencl(x, kx, n, &c, o->kernel);
        if (rc != KDNA_OK) {
            fprintf(stderr, "kdna_dynamics: OpenCL KDNA pack unavailable (%s), falling back to CPU KDNA pack\n", kdna_status_str(rc));
            rc = kdna_eval_cpu(x, kx, n, &c);
        } else {
            backend_flag = KDNA_DYN_FLAG_OPENCL_KDNA_PACK;
        }
    } else {
        rc = kdna_eval_cpu(x, kx, n, &c);
    }
    if (rc != KDNA_OK) { free(steps); return 2; }

    for (size_t i = 0; i < n; ++i) {
        out[kdna_dyn_idx(KDNA_DYN_E, n, i)] = e[i];
        out[kdna_dyn_idx(KDNA_DYN_PHI, n, i)] = p[i];
        out[kdna_dyn_idx(KDNA_DYN_X, n, i)] = x[i];
        out[kdna_dyn_idx(KDNA_DYN_K01, n, i)] = kx[kdna_idx(KDNA_K01, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_K02, n, i)] = kx[kdna_idx(KDNA_K02, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_K03, n, i)] = kx[kdna_idx(KDNA_K03, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_K04, n, i)] = kx[kdna_idx(KDNA_K04, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_K05, n, i)] = kx[kdna_idx(KDNA_K05, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_EK, n, i)] = kx[kdna_idx(KDNA_EK, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_AK, n, i)] = kx[kdna_idx(KDNA_AK, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_LOCK, n, i)] = kx[kdna_idx(KDNA_LK, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_RAW, n, i)] = kx[kdna_idx(KDNA_RAW, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_DOM, n, i)] = kx[kdna_idx(KDNA_DOM, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_S01, n, i)] = kx[kdna_idx(KDNA_S01, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_S02, n, i)] = kx[kdna_idx(KDNA_S02, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_S03, n, i)] = kx[kdna_idx(KDNA_S03, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_S04, n, i)] = kx[kdna_idx(KDNA_S04, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_S05, n, i)] = kx[kdna_idx(KDNA_S05, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_DOM_SCORE, n, i)] = kx[kdna_idx(KDNA_DOM_SCORE, n, i)];
        out[kdna_dyn_idx(KDNA_DYN_GATE, n, i)] = gate[i];
        out[kdna_dyn_idx(KDNA_DYN_GAIN, n, i)] = last_gain;
        out[kdna_dyn_idx(KDNA_DYN_BIAS, n, i)] = last_bias;
        out[kdna_dyn_idx(KDNA_DYN_DRIVE, n, i)] = last_drive;
        out[kdna_dyn_idx(KDNA_DYN_STEP_ID, n, i)] = last_step_id;
        out[kdna_dyn_idx(KDNA_DYN_TIME, n, i)] = (double)o->steps * o->dt;
    }

    kdna_dyn_header h;
    rc = init_header(&h, o, &rh, backend_flag);
    if (rc == KDNA_OK) rc = write_kdyn(o->out_path, &h, out);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_dynamics: write failed: %s\n", kdna_status_str(rc));
        free(steps); free(out); free(x); free(e); free(p); free(en); free(pn); free(kx); free(gate);
        return 2;
    }

    printf("kdna_dynamics: wrote %s backend=%s n=%zu steps=%zu fields=%u payload_bytes=%" PRIu64 " dt=%.17g flags=0x%" PRIx64 "\n",
           o->out_path, o->backend, n, o->steps, (unsigned)KDNA_DYN_FIELDS, h.payload_bytes, o->dt, h.flags);

    free(steps); free(out); free(x); free(e); free(p); free(en); free(pn); free(kx); free(gate);
    return 0;
}

static int inspect(const char *a_path, const char *b_path) {
    kdna_dyn_header ha, hb;
    double *a = NULL, *b = NULL;
    int rc = read_kdyn(a_path, &ha, &a);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_dynamics: cannot read '%s': %s\n", a_path, kdna_status_str(rc));
        return 2;
    }
    printf("file: %s\n  magic: %.8s version:%u n:%" PRIu64 " steps:%" PRIu64 " fields:%u payload:%" PRIu64 " flags:0x%" PRIx64 "\n",
           a_path, ha.magic, ha.version, ha.n, ha.steps, ha.field_count, ha.payload_bytes, ha.flags);
    for (uint32_t f = 0; f < KDNA_DYN_FIELDS; ++f) {
        double mn = a[kdna_dyn_idx(f, (size_t)ha.n, 0u)];
        double mx = mn;
        for (size_t i = 0; i < (size_t)ha.n; ++i) {
            const double v = a[kdna_dyn_idx(f, (size_t)ha.n, i)];
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
        printf("  %-15s min:% .17e max:% .17e\n", kdna_dyn_field_name_local(f), mn, mx);
    }
    if (b_path) {
        rc = read_kdyn(b_path, &hb, &b);
        if (rc != KDNA_OK) { fprintf(stderr, "kdna_dynamics: cannot read '%s': %s\n", b_path, kdna_status_str(rc)); free(a); return 2; }
        if (ha.n != hb.n || ha.field_count != hb.field_count) { fprintf(stderr, "KDYN dimensions mismatch\n"); free(a); free(b); return 2; }
        double gabs = 0.0, grel = 0.0;
        uint32_t gf = 0u; size_t gi = 0u;
        uint64_t raw_mis = 0u, dom_mis = 0u;
        for (uint32_t f = 0; f < KDNA_DYN_FIELDS; ++f) {
            double ma = 0.0, mr = 0.0; size_t mi = 0u;
            for (size_t i = 0; i < (size_t)ha.n; ++i) {
                const double av = a[kdna_dyn_idx(f, (size_t)ha.n, i)];
                const double bv = b[kdna_dyn_idx(f, (size_t)ha.n, i)];
                const double d = fabs(av - bv);
                const double r = d / fmax(1.0, fmax(fabs(av), fabs(bv)));
                if (d > ma) { ma = d; mi = i; }
                if (r > mr) mr = r;
                if (f == KDNA_DYN_RAW && (int)av != (int)bv) raw_mis++;
                if (f == KDNA_DYN_DOM && (int)av != (int)bv) dom_mis++;
            }
            printf("  diff %-10s max_abs:% .17e max_rel:% .17e at_i:%zu\n", kdna_dyn_field_name_local(f), ma, mr, mi);
            if (ma > gabs) { gabs = ma; gf = f; gi = mi; }
            if (mr > grel) grel = mr;
        }
        printf("RAW_mismatches:%" PRIu64 " D_mismatches:%" PRIu64 "\n", raw_mis, dom_mis);
        printf("global_max_abs:% .17e field:%s index:%zu\n", gabs, kdna_dyn_field_name_local(gf), gi);
        printf("global_max_rel:% .17e\n", grel);
        free(b);
    }
    free(a);
    return 0;
}

int main(int argc, char **argv) {
    dyn_opts o;
    memset(&o, 0, sizeof(o));
    o.backend = "cpu";
    o.kernel = "kernels\\kdna_eval.cl";
    o.n = 1024u;
    o.steps = 64u;
    o.dt = 0.02;
    o.seed = 0x12345678ull;
    o.coupling = 0.08;
    o.drive_pull = 0.35;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--run") == 0 && i + 1 < argc) o.run_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) o.out_path = argv[++i];
        else if (strcmp(argv[i], "--backend") == 0 && i + 1 < argc) o.backend = argv[++i];
        else if (strcmp(argv[i], "--kernel") == 0 && i + 1 < argc) o.kernel = argv[++i];
        else if (strcmp(argv[i], "--n") == 0 && i + 1 < argc) o.n = (size_t)strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) o.steps = (size_t)strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--dt") == 0 && i + 1 < argc) o.dt = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) o.seed = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--coupling") == 0 && i + 1 < argc) o.coupling = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--drive-pull") == 0 && i + 1 < argc) o.drive_pull = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--a") == 0 && i + 1 < argc) o.a_path = argv[++i];
        else if (strcmp(argv[i], "--b") == 0 && i + 1 < argc) o.b_path = argv[++i];
        else { usage(); return 2; }
    }

    if (o.a_path) return inspect(o.a_path, o.b_path);
    if (!o.run_path || !o.out_path || o.n == 0u || o.steps == 0u || !(o.dt > 0.0)) {
        usage();
        return 2;
    }
    if (strcmp(o.backend, "cpu") != 0 && strcmp(o.backend, "opencl") != 0) {
        fprintf(stderr, "kdna_dynamics: unknown backend '%s'\n", o.backend);
        return 2;
    }
    return simulate(&o);
}
