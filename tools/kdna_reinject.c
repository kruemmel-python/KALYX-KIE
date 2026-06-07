
#include "kdna.h"
#include "kdna_ksoa.h"
#include "kdna_krun.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    KDNA_REINJECT_OK = 0,
    KDNA_REINJECT_USAGE = 2,
    KDNA_REINJECT_RUNTIME = 3
};

typedef enum reinject_mode {
    REINJECT_BOUNDARY = 0,
    REINJECT_LINEAR = 1,
    REINJECT_DRIVE = 2
} reinject_mode;

typedef struct krun_file {
    const char *path;
    kdna_krun_header h;
    kdna_krun_step_record *steps;
} krun_file;

typedef struct opts {
    const char *run_path;
    const char *out_path;
    size_t samples_per_step;
    reinject_mode mode;
} opts;

static void usage(FILE *f) {
    fprintf(f,
        "kdna_reinject --run <plan.krun> --out <trace.ksoa> [--samples-per-step n] [--mode boundary|linear|drive]\n"
        "\n"
        "Reinjects a deterministic KRUN plan into a controlled KSOA trace.\n"
        "\n"
        "The tool does not rescan the original field. It materializes the selected\n"
        "runtime plan as an x-sequence and evaluates K01-K05 over that sequence.\n"
        "Output is KSOA0001 with backend=cpu and dx=0 because the control trace is\n"
        "generally non-uniform.\n"
        "\n"
        "Modes:\n"
        "  boundary  piecewise x_start -> x_boundary -> x_end per step (default)\n"
        "  linear    linear x_start -> x_end per step\n"
        "  drive     gain-shaped tanh window around drive_x, clamped to step span\n"
        "\n"
        "Example:\n"
        "  kdna_reinject --run run_causal.krun --out reinject_causal.ksoa --samples-per-step 16 --mode boundary\n"
    );
}

static int parse_size_arg(const char *s, size_t *out) {
    if (!s || !out) return 0;
    errno = 0;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v == 0ull) return 0;
#if SIZE_MAX < UINT64_MAX
    if (v > (unsigned long long)SIZE_MAX) return 0;
#endif
    *out = (size_t)v;
    return 1;
}

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

static void free_krun(krun_file *kf) {
    if (kf) {
        free(kf->steps);
        kf->steps = NULL;
    }
}

static int read_krun_payload(const char *path, krun_file *out) {
    if (!path || !out) return KDNA_EINVAL;
    memset(out, 0, sizeof(*out));
    out->path = path;

    int rc = kdna_krun_read_header_file(path, &out->h);
    if (rc != KDNA_OK) return rc;

    if (out->h.step_count == 0u) return KDNA_EINVAL;
    out->steps = (kdna_krun_step_record *)calloc((size_t)out->h.step_count, sizeof(kdna_krun_step_record));
    if (!out->steps) return KDNA_ENOMEM;

    FILE *f = fopen(path, "rb");
    if (!f) {
        free_krun(out);
        return KDNA_EIO;
    }

    int ok = 1;
    if (fseek(f, (long)out->h.header_bytes, SEEK_SET) != 0) ok = 0;
    if (ok && !read_exact(f, out->steps, (size_t)out->h.payload_bytes)) ok = 0;
    if (fclose(f) != 0) ok = 0;

    if (!ok) {
        free_krun(out);
        return KDNA_EIO;
    }
    return KDNA_OK;
}

static double clampd(double x, double lo, double hi) {
    if (lo > hi) {
        const double tmp = lo;
        lo = hi;
        hi = tmp;
    }
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static double absd(double x) {
    return x < 0.0 ? -x : x;
}

static double sample_boundary(const kdna_krun_step_record *s, double t) {
    if (t <= 0.5) {
        const double u = t * 2.0;
        return s->x_start + (s->x_boundary - s->x_start) * u;
    }
    const double u = (t - 0.5) * 2.0;
    return s->x_boundary + (s->x_end - s->x_boundary) * u;
}

static double sample_linear(const kdna_krun_step_record *s, double t) {
    return s->x_start + (s->x_end - s->x_start) * t;
}

static double sample_drive(const kdna_krun_step_record *s, double t) {
    const double lo = fmin(s->x_start, s->x_end);
    const double hi = fmax(s->x_start, s->x_end);
    const double half_span = 0.5 * absd(s->x_end - s->x_start);
    const double shaped = tanh((2.0 * t - 1.0) * fmax(0.0, s->gain));
    const double x = s->drive_x + shaped * half_span;
    return clampd(x, lo, hi);
}

static double sample_step(const kdna_krun_step_record *s, size_t j, size_t samples, reinject_mode mode) {
    const double t = (samples > 1u) ? ((double)j / (double)(samples - 1u)) : 0.0;
    switch (mode) {
        case REINJECT_LINEAR: return sample_linear(s, t);
        case REINJECT_DRIVE: return sample_drive(s, t);
        case REINJECT_BOUNDARY:
        default: return sample_boundary(s, t);
    }
}

static const char *mode_name(reinject_mode mode) {
    switch (mode) {
        case REINJECT_LINEAR: return "linear";
        case REINJECT_DRIVE: return "drive";
        case REINJECT_BOUNDARY: return "boundary";
        default: return "unknown";
    }
}

static int parse_args(int argc, char **argv, opts *o) {
    memset(o, 0, sizeof(*o));
    o->samples_per_step = 16u;
    o->mode = REINJECT_BOUNDARY;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(stdout);
            exit(KDNA_REINJECT_OK);
        } else if (strcmp(argv[i], "--run") == 0 && i + 1 < argc) {
            o->run_path = argv[++i];
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            o->out_path = argv[++i];
        } else if (strcmp(argv[i], "--samples-per-step") == 0 && i + 1 < argc) {
            if (!parse_size_arg(argv[++i], &o->samples_per_step)) return 0;
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            const char *m = argv[++i];
            if (strcmp(m, "boundary") == 0) o->mode = REINJECT_BOUNDARY;
            else if (strcmp(m, "linear") == 0) o->mode = REINJECT_LINEAR;
            else if (strcmp(m, "drive") == 0) o->mode = REINJECT_DRIVE;
            else return 0;
        } else {
            return 0;
        }
    }

    return o->run_path && o->out_path && o->samples_per_step > 0u;
}

int main(int argc, char **argv) {
    opts o;
    if (!parse_args(argc, argv, &o)) {
        usage(stderr);
        return KDNA_REINJECT_USAGE;
    }

    krun_file rf;
    int rc = read_krun_payload(o.run_path, &rf);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_reinject: cannot read '%s': %s\n", o.run_path, kdna_status_str(rc));
        return KDNA_REINJECT_RUNTIME;
    }

    if ((uint64_t)o.samples_per_step > UINT64_MAX / rf.h.step_count) {
        fprintf(stderr, "kdna_reinject: sample count overflow\n");
        free_krun(&rf);
        return KDNA_REINJECT_USAGE;
    }
    const uint64_t n64 = rf.h.step_count * (uint64_t)o.samples_per_step;
#if SIZE_MAX < UINT64_MAX
    if (n64 > (uint64_t)SIZE_MAX) {
        fprintf(stderr, "kdna_reinject: n too large for host\n");
        free_krun(&rf);
        return KDNA_REINJECT_USAGE;
    }
#endif
    const size_t n = (size_t)n64;
    if (n == 0u || n > ((size_t)-1) / (size_t)KDNA_FIELDS) {
        fprintf(stderr, "kdna_reinject: output size overflow\n");
        free_krun(&rf);
        return KDNA_REINJECT_USAGE;
    }

    double *x = (double *)calloc(n, sizeof(double));
    double *out = (double *)calloc((size_t)KDNA_FIELDS * n, sizeof(double));
    if (!x || !out) {
        free(x);
        free(out);
        free_krun(&rf);
        fprintf(stderr, "kdna_reinject: allocation failed for n=%zu\n", n);
        return KDNA_REINJECT_RUNTIME;
    }

    double xmin = 0.0;
    double xmax = 0.0;
    int have_range = 0;
    size_t k = 0u;
    for (uint64_t si = 0u; si < rf.h.step_count; ++si) {
        const kdna_krun_step_record *s = &rf.steps[si];
        for (size_t j = 0u; j < o.samples_per_step; ++j) {
            const double xv = sample_step(s, j, o.samples_per_step, o.mode);
            x[k++] = xv;
            if (!have_range) {
                xmin = xmax = xv;
                have_range = 1;
            } else {
                if (xv < xmin) xmin = xv;
                if (xv > xmax) xmax = xv;
            }
        }
    }

    kdna_constants c;
    kdna_default_constants(&c);
    rc = kdna_eval_cpu(x, out, n, &c);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_reinject: CPU evaluation failed: %s\n", kdna_status_str(rc));
        free(x);
        free(out);
        free_krun(&rf);
        return KDNA_REINJECT_RUNTIME;
    }

    kdna_ksoa_header kh;
    rc = kdna_ksoa_init_header(&kh, n, xmin, xmax, 0.0, KDNA_KSOA_BACKEND_CPU);
    if (rc == KDNA_OK) rc = kdna_ksoa_write_file(o.out_path, &kh, out);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_reinject: failed writing '%s': %s\n", o.out_path, kdna_status_str(rc));
        free(x);
        free(out);
        free_krun(&rf);
        return KDNA_REINJECT_RUNTIME;
    }

    double min_lock = out[kdna_idx(KDNA_LK, n, 0u)];
    double max_lock = min_lock;
    double min_score = out[kdna_idx(KDNA_DOM_SCORE, n, 0u)];
    double max_score = min_score;
    uint64_t raw_switches = 0u;
    uint64_t dom_switches = 0u;
    for (size_t i = 0u; i < n; ++i) {
        const double lk = out[kdna_idx(KDNA_LK, n, i)];
        const double sc = out[kdna_idx(KDNA_DOM_SCORE, n, i)];
        if (lk < min_lock) min_lock = lk;
        if (lk > max_lock) max_lock = lk;
        if (sc < min_score) min_score = sc;
        if (sc > max_score) max_score = sc;
        if (i > 0u) {
            const int raw0 = (int)out[kdna_idx(KDNA_RAW, n, i - 1u)];
            const int raw1 = (int)out[kdna_idx(KDNA_RAW, n, i)];
            const int dom0 = (int)out[kdna_idx(KDNA_DOM, n, i - 1u)];
            const int dom1 = (int)out[kdna_idx(KDNA_DOM, n, i)];
            if (raw0 != raw1) ++raw_switches;
            if (dom0 != dom1) ++dom_switches;
        }
    }

    printf("kdna_reinject: wrote %s source=%s mode=%s steps=%" PRIu64
           " samples_per_step=%zu n=%zu x_min=%.17g x_max=%.17g payload_bytes=%" PRIu64 "\n",
           o.out_path, o.run_path, mode_name(o.mode), rf.h.step_count,
           o.samples_per_step, n, kh.x_min, kh.x_max, kh.payload_bytes);
    printf("reinject_summary: lock:[%.17g,%.17g] score:[%.17g,%.17g] raw_switches:%" PRIu64
           " dom_switches:%" PRIu64 "\n",
           min_lock, max_lock, min_score, max_score, raw_switches, dom_switches);

    free(x);
    free(out);
    free_krun(&rf);
    return KDNA_REINJECT_OK;
}
