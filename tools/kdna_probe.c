#include "kdna_ksoa.h"
#include "kdna_kreg.h"
#include "kdna_klib.h"
#include "kdna_kgram.h"
#include "kdna_krun.h"
#include "kdna_kgen.h"
#include "kdna_kvar.h"
#include "kdna_dyn.h"
#include "kdna_kexp.h"
#include "kdna_kmap.h"
#include "kdna_kgenome.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static uint64_t fnv1a_file(const char *path, uint64_t *size_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0u;
    uint64_t h = 1469598103934665603ull;
    uint64_t n = 0u;
    unsigned char buf[8192];
    for (;;) {
        size_t got = fread(buf, 1u, sizeof(buf), f);
        for (size_t i = 0; i < got; ++i) { h ^= (uint64_t)buf[i]; h *= 1099511628211ull; }
        n += (uint64_t)got;
        if (got < sizeof(buf)) break;
    }
    fclose(f);
    if (size_out) *size_out = n;
    return h;
}

static int read_magic(const char *path, char m[9]) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t got = fread(m, 1u, 8u, f);
    fclose(f);
    m[got] = 0;
    return got == 8u;
}


typedef struct ksieve_edge {
    uint64_t from;
    uint64_t to;
} ksieve_edge;

typedef struct ksieve_count {
    uint64_t value;
    uint64_t count;
} ksieve_count;

static uint64_t ksieve_hash64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static double ksieve_abs(double x) { return x < 0.0 ? -x : x; }

static uint32_t ksieve_raw_bin(double x, uint32_t bins) {
    if (bins < 2u) bins = 2u;
    double u = (x + 1.0) * 0.5;
    if (u < 0.0) u = 0.0;
    if (u > 1.0) u = 1.0;
    uint32_t b = (uint32_t)floor(u * (double)bins);
    return b >= bins ? bins - 1u : b;
}

static double ksieve_entropy_counts(const uint64_t *counts, size_t n, uint64_t total) {
    if (!counts || total == 0u) return 0.0;
    double h = 0.0;
    for (size_t i = 0u; i < n; ++i) {
        if (counts[i]) {
            const double p = (double)counts[i] / (double)total;
            h -= p * log(p);
        }
    }
    return h;
}

static uint64_t ksieve_variant_hash_from_k(const double *out, size_t n, size_t i) {
    const uint64_t raw = (uint64_t)(out[kdna_idx(KDNA_RAW, n, i)] + 0.5);
    const uint64_t dom = (uint64_t)(out[kdna_idx(KDNA_DOM, n, i)] + 0.5);
    const int64_t lock_q = (int64_t)floor(out[kdna_idx(KDNA_LK, n, i)] * 32.0);
    const int64_t score_q = (int64_t)floor(log1p(ksieve_abs(out[kdna_idx(KDNA_DOM_SCORE, n, i)])) * 16.0);
    const int64_t k1 = (int64_t)floor(log1p(ksieve_abs(out[kdna_idx(KDNA_K01, n, i)])) * 8.0);
    const int64_t k2 = (int64_t)floor(out[kdna_idx(KDNA_K02, n, i)] * 16.0);
    const int64_t k3 = (int64_t)floor(out[kdna_idx(KDNA_K03, n, i)] * 16.0);
    const int64_t k4 = (int64_t)floor(out[kdna_idx(KDNA_K04, n, i)] * 24.0);
    const int64_t k5 = (int64_t)floor(out[kdna_idx(KDNA_K05, n, i)] * 16.0);
    uint64_t h = 0x4b444e415f534956ULL; /* KDNA_SIV */
    h ^= ksieve_hash64(raw + 0x11ULL);
    h ^= ksieve_hash64((dom << 8) + 0x22ULL);
    h ^= ksieve_hash64((uint64_t)lock_q);
    h ^= ksieve_hash64((uint64_t)score_q);
    h ^= ksieve_hash64((uint64_t)k1);
    h ^= ksieve_hash64((uint64_t)k2);
    h ^= ksieve_hash64((uint64_t)k3);
    h ^= ksieve_hash64((uint64_t)k4);
    h ^= ksieve_hash64((uint64_t)k5);
    return h ? h : 1ULL;
}

static int ksieve_add_count(ksieve_count *counts, size_t *count_n, size_t cap, uint64_t value) {
    for (size_t i = 0u; i < *count_n; ++i) {
        if (counts[i].value == value) { counts[i].count++; return 1; }
    }
    if (*count_n >= cap) return 0;
    counts[*count_n].value = value;
    counts[*count_n].count = 1u;
    (*count_n)++;
    return 1;
}

static uint64_t ksieve_most_common(const ksieve_count *counts, size_t count_n, uint64_t fallback, uint64_t *count_out) {
    uint64_t best = fallback;
    uint64_t best_n = 0u;
    for (size_t i = 0u; i < count_n; ++i) {
        if (counts[i].count > best_n) {
            best_n = counts[i].count;
            best = counts[i].value;
        }
    }
    if (count_out) *count_out = best_n;
    return best;
}

static int ksieve_edge_exists(const ksieve_edge *edges, size_t edge_n, uint64_t from, uint64_t to) {
    for (size_t i = 0u; i < edge_n; ++i) {
        if (edges[i].from == from && edges[i].to == to) return 1;
    }
    return 0;
}

static int ksieve_read_kgram(const char *path, kdna_kgram_header *h, kdna_krule_record **rules_out) {
    if (!path || !h || !rules_out) return KDNA_EINVAL;
    *rules_out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return KDNA_EIO; }
    int rc = kdna_kgram_validate_header(h);
    if (rc != KDNA_OK) { fclose(f); return rc; }
    if (h->rule_count > 0u) {
        kdna_krule_record *r = (kdna_krule_record *)malloc((size_t)h->payload_bytes);
        if (!r) { fclose(f); return KDNA_ENOMEM; }
        if (fread(r, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) {
            free(r); fclose(f); return KDNA_EIO;
        }
        *rules_out = r;
    }
    if (fclose(f) != 0) {
        free(*rules_out); *rules_out = NULL; return KDNA_EIO;
    }
    return KDNA_OK;
}

static int ksieve_read_symbol_stream(const char *path, uint64_t n, uint64_t **symbols_out) {
    if (!path || !symbols_out || n < 2u) return KDNA_EINVAL;
    *symbols_out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    uint64_t *s = (uint64_t *)calloc((size_t)n, sizeof(uint64_t));
    if (!s) { fclose(f); return KDNA_ENOMEM; }
    if (fread(s, sizeof(uint64_t), (size_t)n, f) != (size_t)n) {
        free(s); fclose(f); return KDNA_EIO;
    }
    fclose(f);
    *symbols_out = s;
    return KDNA_OK;
}

static int ksieve_project_kdat(const char *path,
                               uint32_t bins,
                               uint64_t **symbols_out,
                               size_t *n_out,
                               kdna_kdat_header *dh_out,
                               double *entropy_raw_out,
                               double *mean_lock_out,
                               double *mean_score_out,
                               double *max_score_out,
                               double *null_hits_out) {
    kdna_kdat_header dh;
    double *x = NULL;
    int rc = kdna_kdat_read_file(path, &dh, &x);
    if (rc != KDNA_OK) return rc;
    const size_t n = (size_t)dh.n;
    if (n < 8u) { free(x); return KDNA_EINVAL; }

    double *out = (double *)calloc((size_t)KDNA_FIELDS * n, sizeof(double));
    uint64_t *symbols = (uint64_t *)calloc(n, sizeof(uint64_t));
    uint64_t *raw_counts = (uint64_t *)calloc(bins ? bins : 32u, sizeof(uint64_t));
    if (!out || !symbols || !raw_counts) {
        free(x); free(out); free(symbols); free(raw_counts); return KDNA_ENOMEM;
    }

    kdna_constants c;
    kdna_default_constants(&c);
    rc = kdna_eval_cpu(x, out, n, &c);
    if (rc != KDNA_OK) {
        free(x); free(out); free(symbols); free(raw_counts); return rc;
    }

    double mean_lock = 0.0, mean_score = 0.0, max_score = 0.0, null_hits = 0.0;
    for (size_t i = 0u; i < n; ++i) {
        raw_counts[ksieve_raw_bin(x[i], bins)]++;
        symbols[i] = ksieve_variant_hash_from_k(out, n, i);
        mean_lock += out[kdna_idx(KDNA_LK, n, i)];
        const double score = out[kdna_idx(KDNA_DOM_SCORE, n, i)];
        mean_score += score;
        if (score > max_score) max_score = score;
        if (fabs(x[i]) < 1.0e-4) null_hits += 1.0;
    }

    if (dh_out) *dh_out = dh;
    if (entropy_raw_out) *entropy_raw_out = ksieve_entropy_counts(raw_counts, bins, dh.n);
    if (mean_lock_out) *mean_lock_out = mean_lock / (double)n;
    if (mean_score_out) *mean_score_out = mean_score / (double)n;
    if (max_score_out) *max_score_out = max_score;
    if (null_hits_out) *null_hits_out = null_hits;
    *symbols_out = symbols;
    *n_out = n;

    free(x); free(out); free(raw_counts);
    return KDNA_OK;
}


static int cmp_u64_qsort(const void *a, const void *b) {
    const uint64_t x = *(const uint64_t *)a;
    const uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static int cmp_edge_qsort(const void *a, const void *b) {
    const ksieve_edge *ea = (const ksieve_edge *)a;
    const ksieve_edge *eb = (const ksieve_edge *)b;
    if (ea->from != eb->from) return (ea->from > eb->from) - (ea->from < eb->from);
    return (ea->to > eb->to) - (ea->to < eb->to);
}

static int ksieve_edge_exists_sorted(const ksieve_edge *edges, size_t edge_n, uint64_t from, uint64_t to) {
    size_t lo = 0u, hi = edge_n;
    while (lo < hi) {
        const size_t mid = lo + ((hi - lo) >> 1);
        const ksieve_edge *e = &edges[mid];
        if (e->from < from || (e->from == from && e->to < to)) lo = mid + 1u;
        else hi = mid;
    }
    return lo < edge_n && edges[lo].from == from && edges[lo].to == to;
}

static int ksieve_count_sorted_symbols(const uint64_t *symbols, size_t n,
                                       size_t *unique_out,
                                       uint64_t *top_value_out,
                                       uint64_t *top_count_out,
                                       double *entropy_out) {
    if (!symbols || n == 0u || !unique_out || !top_value_out || !top_count_out || !entropy_out) return KDNA_EINVAL;
    uint64_t *tmp = (uint64_t *)malloc(n * sizeof(uint64_t));
    if (!tmp) return KDNA_ENOMEM;
    memcpy(tmp, symbols, n * sizeof(uint64_t));
    qsort(tmp, n, sizeof(uint64_t), cmp_u64_qsort);
    size_t unique = 0u;
    uint64_t top_v = tmp[0], top_c = 0u;
    double entropy = 0.0;
    for (size_t i = 0u; i < n;) {
        const uint64_t v = tmp[i];
        size_t j = i + 1u;
        while (j < n && tmp[j] == v) ++j;
        const uint64_t c = (uint64_t)(j - i);
        unique++;
        if (c > top_c) { top_c = c; top_v = v; }
        const double p = (double)c / (double)n;
        entropy -= p * log(p);
        i = j;
    }
    free(tmp);
    *unique_out = unique;
    *top_value_out = top_v;
    *top_count_out = top_c;
    *entropy_out = entropy;
    return KDNA_OK;
}

static int ksieve_most_common_sorted(const uint64_t *symbols, size_t n, uint64_t fallback,
                                     uint64_t *value_out, uint64_t *count_out) {
    if (!value_out || !count_out) return KDNA_EINVAL;
    if (!symbols || n == 0u) { *value_out = fallback; *count_out = 0u; return KDNA_OK; }
    uint64_t *tmp = (uint64_t *)malloc(n * sizeof(uint64_t));
    if (!tmp) return KDNA_ENOMEM;
    memcpy(tmp, symbols, n * sizeof(uint64_t));
    qsort(tmp, n, sizeof(uint64_t), cmp_u64_qsort);
    uint64_t best_v = tmp[0], best_c = 0u;
    for (size_t i = 0u; i < n;) {
        const uint64_t v = tmp[i];
        size_t j = i + 1u;
        while (j < n && tmp[j] == v) ++j;
        const uint64_t c = (uint64_t)(j - i);
        if (c > best_c) { best_c = c; best_v = v; }
        i = j;
    }
    free(tmp);
    *value_out = best_v;
    *count_out = best_c;
    return KDNA_OK;
}

/* ---- Optional OpenCL acceleration for K-Sieve transition correlation ---- */
typedef intptr_t ks_cl_context_properties;
typedef int32_t ks_cl_int;
typedef uint32_t ks_cl_uint;
typedef uint64_t ks_cl_ulong;
typedef uint64_t ks_cl_device_type;
typedef uint64_t ks_cl_command_queue_properties;
typedef uint64_t ks_cl_mem_flags;
typedef uint64_t ks_cl_bool;
typedef struct _cl_platform_id *ks_cl_platform_id;
typedef struct _cl_device_id *ks_cl_device_id;
typedef struct _cl_context *ks_cl_context;
typedef struct _cl_command_queue *ks_cl_command_queue;
typedef struct _cl_program *ks_cl_program;
typedef struct _cl_kernel *ks_cl_kernel;
typedef struct _cl_mem *ks_cl_mem;

#define KS_CL_SUCCESS 0
#define KS_CL_DEVICE_TYPE_GPU (1ull << 2)
#define KS_CL_DEVICE_TYPE_DEFAULT (1ull << 0)
#define KS_CL_MEM_READ_ONLY (1ull << 2)
#define KS_CL_MEM_WRITE_ONLY (1ull << 1)
#define KS_CL_MEM_COPY_HOST_PTR (1ull << 5)
#define KS_CL_TRUE 1
#define KS_CL_PROGRAM_BUILD_LOG 0x1183
#define KS_CL_CONTEXT_PLATFORM 0x1084

typedef ks_cl_int (*ks_p_clGetPlatformIDs)(ks_cl_uint, ks_cl_platform_id *, ks_cl_uint *);
typedef ks_cl_int (*ks_p_clGetDeviceIDs)(ks_cl_platform_id, ks_cl_device_type, ks_cl_uint, ks_cl_device_id *, ks_cl_uint *);
typedef ks_cl_context (*ks_p_clCreateContext)(const ks_cl_context_properties *, ks_cl_uint, const ks_cl_device_id *, void *, void *, ks_cl_int *);
typedef ks_cl_command_queue (*ks_p_clCreateCommandQueue)(ks_cl_context, ks_cl_device_id, ks_cl_command_queue_properties, ks_cl_int *);
typedef ks_cl_program (*ks_p_clCreateProgramWithSource)(ks_cl_context, ks_cl_uint, const char **, const size_t *, ks_cl_int *);
typedef ks_cl_int (*ks_p_clBuildProgram)(ks_cl_program, ks_cl_uint, const ks_cl_device_id *, const char *, void *, void *);
typedef ks_cl_int (*ks_p_clGetProgramBuildInfo)(ks_cl_program, ks_cl_device_id, ks_cl_uint, size_t, void *, size_t *);
typedef ks_cl_kernel (*ks_p_clCreateKernel)(ks_cl_program, const char *, ks_cl_int *);
typedef ks_cl_mem (*ks_p_clCreateBuffer)(ks_cl_context, ks_cl_mem_flags, size_t, void *, ks_cl_int *);
typedef ks_cl_int (*ks_p_clSetKernelArg)(ks_cl_kernel, ks_cl_uint, size_t, const void *);
typedef ks_cl_int (*ks_p_clEnqueueNDRangeKernel)(ks_cl_command_queue, ks_cl_kernel, ks_cl_uint, const size_t *, const size_t *, const size_t *, ks_cl_uint, const void *, void *);
typedef ks_cl_int (*ks_p_clEnqueueReadBuffer)(ks_cl_command_queue, ks_cl_mem, ks_cl_bool, size_t, size_t, void *, ks_cl_uint, const void *, void *);
typedef ks_cl_int (*ks_p_clFinish)(ks_cl_command_queue);
typedef ks_cl_int (*ks_p_clReleaseMemObject)(ks_cl_mem);
typedef ks_cl_int (*ks_p_clReleaseKernel)(ks_cl_kernel);
typedef ks_cl_int (*ks_p_clReleaseProgram)(ks_cl_program);
typedef ks_cl_int (*ks_p_clReleaseCommandQueue)(ks_cl_command_queue);
typedef ks_cl_int (*ks_p_clReleaseContext)(ks_cl_context);

#if defined(_WIN32)
typedef HMODULE ks_dynlib;
#define KS_LOAD_SYMBOL(lib, name) GetProcAddress((lib), (name))
static ks_dynlib ks_load_lib(void) { return LoadLibraryA("OpenCL.dll"); }
static void ks_close_lib(ks_dynlib lib) { if (lib) FreeLibrary(lib); }
#else
typedef void *ks_dynlib;
#define KS_LOAD_SYMBOL(lib, name) dlsym((lib), (name))
static ks_dynlib ks_load_lib(void) {
    ks_dynlib lib = dlopen("libOpenCL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!lib) lib = dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL);
    return lib;
}
static void ks_close_lib(ks_dynlib lib) { if (lib) dlclose(lib); }
#endif

typedef struct ks_ocl_api {
    ks_dynlib lib;
    ks_p_clGetPlatformIDs clGetPlatformIDs;
    ks_p_clGetDeviceIDs clGetDeviceIDs;
    ks_p_clCreateContext clCreateContext;
    ks_p_clCreateCommandQueue clCreateCommandQueue;
    ks_p_clCreateProgramWithSource clCreateProgramWithSource;
    ks_p_clBuildProgram clBuildProgram;
    ks_p_clGetProgramBuildInfo clGetProgramBuildInfo;
    ks_p_clCreateKernel clCreateKernel;
    ks_p_clCreateBuffer clCreateBuffer;
    ks_p_clSetKernelArg clSetKernelArg;
    ks_p_clEnqueueNDRangeKernel clEnqueueNDRangeKernel;
    ks_p_clEnqueueReadBuffer clEnqueueReadBuffer;
    ks_p_clFinish clFinish;
    ks_p_clReleaseMemObject clReleaseMemObject;
    ks_p_clReleaseKernel clReleaseKernel;
    ks_p_clReleaseProgram clReleaseProgram;
    ks_p_clReleaseCommandQueue clReleaseCommandQueue;
    ks_p_clReleaseContext clReleaseContext;
} ks_ocl_api;

#define KS_LOAD(api, name) do { (api)->name = (ks_p_##name)KS_LOAD_SYMBOL((api)->lib, #name); if (!(api)->name) return KDNA_EOPENCL; } while (0)
static int ks_ocl_load_api(ks_ocl_api *api) {
    memset(api, 0, sizeof(*api));
    api->lib = ks_load_lib();
    if (!api->lib) return KDNA_EOPENCL;
    KS_LOAD(api, clGetPlatformIDs); KS_LOAD(api, clGetDeviceIDs);
    KS_LOAD(api, clCreateContext); KS_LOAD(api, clCreateCommandQueue);
    KS_LOAD(api, clCreateProgramWithSource); KS_LOAD(api, clBuildProgram);
    KS_LOAD(api, clGetProgramBuildInfo); KS_LOAD(api, clCreateKernel); KS_LOAD(api, clCreateBuffer);
    KS_LOAD(api, clSetKernelArg); KS_LOAD(api, clEnqueueNDRangeKernel); KS_LOAD(api, clEnqueueReadBuffer);
    KS_LOAD(api, clFinish); KS_LOAD(api, clReleaseMemObject); KS_LOAD(api, clReleaseKernel);
    KS_LOAD(api, clReleaseProgram); KS_LOAD(api, clReleaseCommandQueue); KS_LOAD(api, clReleaseContext);
    return KDNA_OK;
}
#undef KS_LOAD

static char *ks_read_text_file(const char *path, size_t *len_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return NULL; }
    rewind(f);
    char *s = (char *)calloc((size_t)n + 1u, 1u);
    if (!s) { fclose(f); return NULL; }
    if (fread(s, 1u, (size_t)n, f) != (size_t)n) { free(s); fclose(f); return NULL; }
    fclose(f);
    if (len_out) *len_out = (size_t)n;
    return s;
}

static int ks_pick_device(ks_ocl_api *cl, ks_cl_platform_id *platform, ks_cl_device_id *device) {
    ks_cl_uint np = 0u;
    if (cl->clGetPlatformIDs(0u, NULL, &np) != KS_CL_SUCCESS || np == 0u) return KDNA_ENO_DEVICE;
    ks_cl_platform_id *ps = (ks_cl_platform_id *)calloc(np, sizeof(*ps));
    if (!ps) return KDNA_ENOMEM;
    if (cl->clGetPlatformIDs(np, ps, NULL) != KS_CL_SUCCESS) { free(ps); return KDNA_EOPENCL; }
    for (ks_cl_uint i = 0u; i < np; ++i) {
        ks_cl_device_id d = NULL;
        if (cl->clGetDeviceIDs(ps[i], KS_CL_DEVICE_TYPE_GPU, 1u, &d, NULL) == KS_CL_SUCCESS && d) {
            *platform = ps[i]; *device = d; free(ps); return KDNA_OK;
        }
    }
    for (ks_cl_uint i = 0u; i < np; ++i) {
        ks_cl_device_id d = NULL;
        if (cl->clGetDeviceIDs(ps[i], KS_CL_DEVICE_TYPE_DEFAULT, 1u, &d, NULL) == KS_CL_SUCCESS && d) {
            *platform = ps[i]; *device = d; free(ps); return KDNA_OK;
        }
    }
    free(ps); return KDNA_ENO_DEVICE;
}

static int ksieve_correlate_opencl(const uint64_t *symbols,
                                   size_t n,
                                   const ksieve_edge *edges,
                                   size_t edge_n,
                                   size_t train_n,
                                   uint64_t baseline,
                                   const char *kernel_path,
                                   uint64_t *baseline_ok_out,
                                   uint64_t *grammar_ok_out,
                                   uint64_t *oog_out) {
    if (!symbols || !edges || !kernel_path || !baseline_ok_out || !grammar_ok_out || !oog_out) return KDNA_EINVAL;
    if (n < 2u || train_n >= n - 1u) return KDNA_EINVAL;

    int rc = KDNA_OK;
    ks_ocl_api cl;
    rc = ks_ocl_load_api(&cl);
    if (rc != KDNA_OK) return rc;

    ks_cl_platform_id platform = NULL;
    ks_cl_device_id device = NULL;
    ks_cl_context ctx = NULL;
    ks_cl_command_queue q = NULL;
    ks_cl_program prog = NULL;
    ks_cl_kernel kern = NULL;
    ks_cl_mem b_symbols = NULL, b_from = NULL, b_to = NULL, b_base = NULL, b_gram = NULL;
    char *source = NULL;
    uint64_t *edge_from = NULL, *edge_to = NULL;
    unsigned char *base_hits = NULL, *gram_hits = NULL;

    rc = ks_pick_device(&cl, &platform, &device);
    if (rc != KDNA_OK) goto done;

    ks_cl_int err = 0;
    ks_cl_context_properties props[] = { KS_CL_CONTEXT_PLATFORM, (ks_cl_context_properties)platform, 0 };
    ctx = cl.clCreateContext(props, 1u, &device, NULL, NULL, &err);
    if (!ctx || err != KS_CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }
    q = cl.clCreateCommandQueue(ctx, device, 0u, &err);
    if (!q || err != KS_CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

    size_t source_len = 0u;
    source = ks_read_text_file(kernel_path, &source_len);
    if (!source) { rc = KDNA_EIO; goto done; }
    const char *srcs[] = { source };
    prog = cl.clCreateProgramWithSource(ctx, 1u, srcs, &source_len, &err);
    if (!prog || err != KS_CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }
    err = cl.clBuildProgram(prog, 1u, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != KS_CL_SUCCESS) {
        size_t log_len = 0u;
        cl.clGetProgramBuildInfo(prog, device, KS_CL_PROGRAM_BUILD_LOG, 0u, NULL, &log_len);
        if (log_len) {
            char *log = (char *)calloc(log_len + 1u, 1u);
            if (log) {
                cl.clGetProgramBuildInfo(prog, device, KS_CL_PROGRAM_BUILD_LOG, log_len, log, NULL);
                fprintf(stderr, "%s\n", log);
                free(log);
            }
        }
        rc = KDNA_EBUILD; goto done;
    }
    kern = cl.clCreateKernel(prog, "kdna_ksieve_kernel", &err);
    if (!kern || err != KS_CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

    edge_from = (uint64_t *)malloc(edge_n * sizeof(uint64_t));
    edge_to = (uint64_t *)malloc(edge_n * sizeof(uint64_t));
    const size_t test_trans = n - train_n - 1u;
    base_hits = (unsigned char *)calloc(test_trans, 1u);
    gram_hits = (unsigned char *)calloc(test_trans, 1u);
    if (!edge_from || !edge_to || !base_hits || !gram_hits) { rc = KDNA_ENOMEM; goto done; }
    for (size_t i = 0u; i < edge_n; ++i) { edge_from[i] = edges[i].from; edge_to[i] = edges[i].to; }

    b_symbols = cl.clCreateBuffer(ctx, KS_CL_MEM_READ_ONLY | KS_CL_MEM_COPY_HOST_PTR, n * sizeof(uint64_t), (void *)symbols, &err);
    b_from = cl.clCreateBuffer(ctx, KS_CL_MEM_READ_ONLY | KS_CL_MEM_COPY_HOST_PTR, edge_n * sizeof(uint64_t), edge_from, &err);
    b_to = cl.clCreateBuffer(ctx, KS_CL_MEM_READ_ONLY | KS_CL_MEM_COPY_HOST_PTR, edge_n * sizeof(uint64_t), edge_to, &err);
    b_base = cl.clCreateBuffer(ctx, KS_CL_MEM_WRITE_ONLY, test_trans * sizeof(unsigned char), NULL, &err);
    b_gram = cl.clCreateBuffer(ctx, KS_CL_MEM_WRITE_ONLY, test_trans * sizeof(unsigned char), NULL, &err);
    if (!b_symbols || !b_from || !b_to || !b_base || !b_gram) { rc = KDNA_EOPENCL; goto done; }

    ks_cl_ulong nn = (ks_cl_ulong)n;
    ks_cl_ulong ee = (ks_cl_ulong)edge_n;
    ks_cl_ulong tt = (ks_cl_ulong)test_trans;
    ks_cl_ulong tr = (ks_cl_ulong)train_n;
    ks_cl_ulong bl = (ks_cl_ulong)baseline;
    ks_cl_uint ai = 0u;
#define KS_SETARG(v) do { err = cl.clSetKernelArg(kern, ai++, sizeof(v), &(v)); if (err != KS_CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; } } while (0)
    KS_SETARG(b_symbols); KS_SETARG(b_from); KS_SETARG(b_to); KS_SETARG(b_base); KS_SETARG(b_gram);
    KS_SETARG(nn); KS_SETARG(ee); KS_SETARG(tr); KS_SETARG(tt); KS_SETARG(bl);
#undef KS_SETARG

    {
        const size_t global = test_trans;
        err = cl.clEnqueueNDRangeKernel(q, kern, 1u, NULL, &global, NULL, 0u, NULL, NULL);
        if (err != KS_CL_SUCCESS || cl.clFinish(q) != KS_CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }
        err = cl.clEnqueueReadBuffer(q, b_base, KS_CL_TRUE, 0u, test_trans, base_hits, 0u, NULL, NULL);
        if (err != KS_CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }
        err = cl.clEnqueueReadBuffer(q, b_gram, KS_CL_TRUE, 0u, test_trans, gram_hits, 0u, NULL, NULL);
        if (err != KS_CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }
    }

    *baseline_ok_out = 0u;
    *grammar_ok_out = 0u;
    for (size_t i = 0u; i < test_trans; ++i) {
        if (base_hits[i]) (*baseline_ok_out)++;
        if (gram_hits[i]) (*grammar_ok_out)++;
    }
    *oog_out = (uint64_t)test_trans - *grammar_ok_out;
    rc = KDNA_OK;

done:
    if (b_symbols) cl.clReleaseMemObject(b_symbols);
    if (b_from) cl.clReleaseMemObject(b_from);
    if (b_to) cl.clReleaseMemObject(b_to);
    if (b_base) cl.clReleaseMemObject(b_base);
    if (b_gram) cl.clReleaseMemObject(b_gram);
    if (kern) cl.clReleaseKernel(kern);
    if (prog) cl.clReleaseProgram(prog);
    if (q) cl.clReleaseCommandQueue(q);
    if (ctx) cl.clReleaseContext(ctx);
    if (source) free(source);
    if (edge_from) free(edge_from);
    if (edge_to) free(edge_to);
    if (base_hits) free(base_hits);
    if (gram_hits) free(gram_hits);
    ks_close_lib(cl.lib);
    return rc;
}

static void ksieve_usage(FILE *f) {
    fprintf(f,
        "kdna_probe --sieve --data input.kdat --gram grammar.kgram --out result.krep [--train 0.70] [--bins 32] [--backend cpu|opencl]\n"
        "kdna_probe --sieve --symbols stream.u64 --n N --gram grammar.kgram --out result.krep [--train 0.70] [--bins 32] [--backend cpu|opencl]\n"
        "\n"
        "K-Sieve: validates an external binary stream against an existing KGRAM01.\n"
        "No grammar learning, no prediction engine, no data-mining heuristic.\n"
        "Outputs KEXP v2-compatible KREP0001 metrics: entropy_raw, kgram_accuracy, lift, out_of_grammar.\n");
}

static int run_ksieve(int argc, char **argv) {
    const char *data_path = NULL;
    const char *symbols_path = NULL;
    const char *gram_path = NULL;
    const char *out_path = NULL;
    double train_fraction = 0.70;
    uint32_t bins = 32u;
    uint64_t symbol_n = 0u;
    const char *backend = "cpu";
    const char *kernel_path = "kernels/kdna_ksieve.cl";

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) data_path = argv[++i];
        else if (strcmp(argv[i], "--symbols") == 0 && i + 1 < argc) symbols_path = argv[++i];
        else if (strcmp(argv[i], "--n") == 0 && i + 1 < argc) symbol_n = (uint64_t)strtoull(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--gram") == 0 && i + 1 < argc) gram_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--train") == 0 && i + 1 < argc) train_fraction = strtod(argv[++i], NULL);
        else if (strcmp(argv[i], "--bins") == 0 && i + 1 < argc) bins = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--backend") == 0 && i + 1 < argc) backend = argv[++i];
        else if (strcmp(argv[i], "--kernel") == 0 && i + 1 < argc) kernel_path = argv[++i];
        else if (strcmp(argv[i], "--help") == 0) { ksieve_usage(stdout); return 0; }
        else { ksieve_usage(stderr); return 2; }
    }

    if ((!data_path && !symbols_path) || (data_path && symbols_path) || !gram_path || !out_path) {
        ksieve_usage(stderr);
        return 2;
    }
    if (bins < 4u) bins = 32u;
    if (!(train_fraction > 0.05 && train_fraction < 0.95)) train_fraction = 0.70;

    kdna_kgram_header gh;
    kdna_krule_record *rules = NULL;
    int rc = ksieve_read_kgram(gram_path, &gh, &rules);
    if (rc != KDNA_OK) {
        fprintf(stderr, "ksieve: cannot read KGRAM '%s': %s\n", gram_path, kdna_status_str(rc));
        return 2;
    }

    ksieve_edge *edges = (ksieve_edge *)calloc((size_t)gh.rule_count, sizeof(ksieve_edge));
    if (gh.rule_count && !edges) { free(rules); return 2; }
    for (size_t i = 0u; i < (size_t)gh.rule_count; ++i) {
        edges[i].from = rules[i].from_id;
        edges[i].to = rules[i].to_id;
    }
    if (gh.rule_count) qsort(edges, (size_t)gh.rule_count, sizeof(ksieve_edge), cmp_edge_qsort);

    uint64_t *symbols = NULL;
    size_t n = 0u;
    kdna_kdat_header dh;
    memset(&dh, 0, sizeof(dh));
    double entropy_raw = 0.0, mean_lock = 0.0, mean_score = 0.0, max_score = 0.0, null_hits = 0.0;
    uint32_t kind = KDNA_KEXP_KSIEVE;
    uint64_t seed = 0u;
    double x_min = 0.0, x_max = 0.0, x_mean = 0.0, x_var = 0.0;

    if (data_path) {
        rc = ksieve_project_kdat(data_path, bins, &symbols, &n, &dh, &entropy_raw, &mean_lock, &mean_score, &max_score, &null_hits);
        if (rc != KDNA_OK) {
            fprintf(stderr, "ksieve: cannot project KDAT '%s': %s\n", data_path, kdna_status_str(rc));
            free(edges); free(rules); return 2;
        }
        kind = dh.kind;
        seed = dh.seed;
        x_min = dh.x_min; x_max = dh.x_max; x_mean = dh.mean; x_var = dh.variance;
    } else {
        if (symbol_n < 8u) { fprintf(stderr, "ksieve: --symbols requires --n >= 8\n"); free(edges); free(rules); return 2; }
        rc = ksieve_read_symbol_stream(symbols_path, symbol_n, &symbols);
        if (rc != KDNA_OK) {
            fprintf(stderr, "ksieve: cannot read symbol stream '%s': %s\n", symbols_path, kdna_status_str(rc));
            free(edges); free(rules); return 2;
        }
        n = (size_t)symbol_n;
        /* entropy and uniqueness are computed below through an O(n log n) sorted pass.
           The original K-Sieve v1 path used a linear list of counts and was O(n^2)
           for large external streams such as chr17 k-mers. */
    }

    size_t unique_variants = 0u;
    uint64_t top_var = 0u;
    uint64_t top_var_count = 0u;
    double entropy_symbols = 0.0;
    rc = ksieve_count_sorted_symbols(symbols, n, &unique_variants, &top_var, &top_var_count, &entropy_symbols);
    if (rc != KDNA_OK) {
        fprintf(stderr, "ksieve: symbol count failed: %s\n", kdna_status_str(rc));
        free(symbols); free(edges); free(rules); return 2;
    }
    if (!data_path) entropy_raw = entropy_symbols;

    size_t train_n = (size_t)floor((double)n * train_fraction);
    if (train_n < 4u) train_n = 4u;
    if (train_n >= n - 2u) train_n = n - 2u;

    uint64_t baseline_count = 0u;
    uint64_t baseline = 0u;
    rc = ksieve_most_common_sorted(symbols + 1u, train_n - 1u, symbols[train_n], &baseline, &baseline_count);
    if (rc != KDNA_OK) {
        fprintf(stderr, "ksieve: baseline count failed: %s\n", kdna_status_str(rc));
        free(symbols); free(edges); free(rules); return 2;
    }

    uint64_t baseline_ok = 0u, grammar_ok = 0u, oog = 0u;
    uint64_t test_trans = (uint64_t)(n - train_n - 1u);
    if (strcmp(backend, "opencl") == 0) {
        rc = ksieve_correlate_opencl(symbols, n, edges, (size_t)gh.rule_count,
                                     train_n, baseline, kernel_path,
                                     &baseline_ok, &grammar_ok, &oog);
        if (rc != KDNA_OK) {
            fprintf(stderr, "ksieve: OpenCL backend unavailable/failed (%s), falling back to cpu-fast\n", kdna_status_str(rc));
            baseline_ok = 0u; grammar_ok = 0u; oog = 0u;
            for (size_t i = train_n; i + 1u < n; ++i) {
                const uint64_t actual_to = symbols[i + 1u];
                if (actual_to == baseline) baseline_ok++;
                if (ksieve_edge_exists_sorted(edges, (size_t)gh.rule_count, symbols[i], actual_to)) grammar_ok++;
                else oog++;
            }
        }
    } else if (strcmp(backend, "cpu") == 0 || strcmp(backend, "cpu-fast") == 0) {
        for (size_t i = train_n; i + 1u < n; ++i) {
            const uint64_t actual_to = symbols[i + 1u];
            if (actual_to == baseline) baseline_ok++;
            if (ksieve_edge_exists_sorted(edges, (size_t)gh.rule_count, symbols[i], actual_to)) grammar_ok++;
            else oog++;
        }
    } else {
        fprintf(stderr, "ksieve: unknown backend '%s'\n", backend);
        free(symbols); free(edges); free(rules); return 2;
    }

    kdna_kexp_result_record rec;
    memset(&rec, 0, sizeof(rec));
    rec.id = 1u;
    rec.n = (uint64_t)n;
    rec.train_n = (uint64_t)train_n;
    rec.seed = seed;
    rec.kind = kind;
    rec.raw_bins = bins;
    rec.unique_raw_bins = (uint32_t)(unique_variants < (size_t)UINT32_MAX ? unique_variants : UINT32_MAX);
    rec.unique_variants = (uint32_t)(unique_variants < (size_t)UINT32_MAX ? unique_variants : UINT32_MAX);
    rec.train_transitions = (uint64_t)(train_n - 1u);
    rec.test_transitions = test_trans;
    rec.grammar_edges = gh.rule_count;
    rec.out_of_grammar = oog;
    rec.x_min = x_min; rec.x_max = x_max; rec.x_mean = x_mean; rec.x_variance = x_var;
    rec.entropy_raw = entropy_raw;
    rec.entropy_variant = entropy_raw;
    rec.compression_ratio = unique_variants ? (double)n / (double)unique_variants : 0.0;
    rec.baseline_accuracy = test_trans ? (double)baseline_ok / (double)test_trans : 0.0;
    rec.kgram_accuracy = test_trans ? (double)grammar_ok / (double)test_trans : 0.0;
    rec.kgram_lift = rec.kgram_accuracy - rec.baseline_accuracy;
    rec.surprise_rate = test_trans ? (double)oog / (double)test_trans : 0.0;
    rec.mean_lock = mean_lock;
    rec.mean_dom_score = mean_score;
    rec.max_dom_score = max_score;
    rec.null_membrane_hits = null_hits;
    rec.top_variant_id = top_var;
    rec.top_variant_count = top_var_count;

    kdna_krep_header rh;
    rc = kdna_krep_init_header(&rh, 1u);
    if (rc == KDNA_OK) rc = kdna_krep_write_file(out_path, &rh, &rec);
    if (rc != KDNA_OK) {
        fprintf(stderr, "ksieve: cannot write KREP '%s': %s\n", out_path, kdna_status_str(rc));
        free(symbols); free(edges); free(rules); return 2;
    }

    printf("K-Sieve result source:%s grammar:%s out:%s\n",
           data_path ? data_path : symbols_path, gram_path, out_path);
    printf("  entropy_raw:%.17g\n", rec.entropy_raw);
    printf("  baseline_accuracy:%.17g\n", rec.baseline_accuracy);
    printf("  kgram_accuracy:%.17g\n", rec.kgram_accuracy);
    printf("  lift:%+.17g\n", rec.kgram_lift);
    printf("  out_of_grammar:%" PRIu64 "\n", rec.out_of_grammar);
    printf("  surprise_rate:%.17g\n", rec.surprise_rate);
    printf("  grammar_edges:%" PRIu64 " unique_variants:%u test_transitions:%" PRIu64 "\n",
           rec.grammar_edges, rec.unique_variants, rec.test_transitions);

    free(symbols); free(edges); free(rules);
    return 0;
}


int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "--sieve") == 0) return run_ksieve(argc, argv);
    if (argc < 2) {
        fprintf(stderr, "usage: kdna_probe <artifact> [...]\n");
        return 2;
    }
    for (int ai = 1; ai < argc; ++ai) {
        const char *path = argv[ai];
        char magic[9] = {0};
        uint64_t size = 0u;
        uint64_t hash = fnv1a_file(path, &size);
        printf("file: %s\n  size:%llu fnv1a64:0x%016llx\n", path, (unsigned long long)size, (unsigned long long)hash);
        if (!read_magic(path, magic)) { printf("  error: cannot read magic\n"); continue; }
        printf("  magic: %.8s\n", magic);

        if (memcmp(magic, KDNA_KSOA_MAGIC, 8u) == 0) {
            kdna_ksoa_header h; int rc = kdna_ksoa_read_header_file(path, &h);
            printf("  type: KSOA rc:%s n:%llu fields:%u backend:%s payload:%llu x:[%.17g,%.17g] dx:%.17g flags:0x%llx\n",
                kdna_status_str(rc), (unsigned long long)h.n, h.fields, kdna_ksoa_backend_name(h.backend),
                (unsigned long long)h.payload_bytes, h.x_min, h.x_max, h.dx, (unsigned long long)h.flags);
        } else if (memcmp(magic, KDNA_KREG_MAGIC, 8u) == 0) {
            kdna_kreg_header h; int rc = kdna_kreg_read_header_file(path, &h);
            printf("  type: KREG rc:%s segments:%llu record_bytes:%u payload:%llu source_n:%llu\n",
                kdna_status_str(rc), (unsigned long long)h.segment_count, h.record_bytes, (unsigned long long)h.payload_bytes, (unsigned long long)h.source_n);
        } else if (memcmp(magic, KDNA_KLIB_MAGIC, 8u) == 0) {
            kdna_klib_header h; int rc = kdna_klib_read_header_file(path, &h);
            printf("  type: KLIB rc:%s words:%llu source_n:%llu payload:%llu x:[%.17g,%.17g]\n",
                kdna_status_str(rc), (unsigned long long)h.word_count, (unsigned long long)h.source_n, (unsigned long long)h.payload_bytes, h.x_min, h.x_max);
        } else if (memcmp(magic, KDNA_KGRAM_MAGIC, 8u) == 0) {
            kdna_kgram_header h; int rc = kdna_kgram_read_header_file(path, &h);
            printf("  type: KGRAM rc:%s rules:%llu source_words:%llu source_n:%llu payload:%llu\n",
                kdna_status_str(rc), (unsigned long long)h.rule_count, (unsigned long long)h.source_word_count,
                (unsigned long long)h.source_n, (unsigned long long)h.payload_bytes);
        } else if (memcmp(magic, KDNA_KRUN_MAGIC, 8u) == 0) {
            kdna_krun_header h; int rc = kdna_krun_read_header_file(path, &h);
            printf("  type: KRUN rc:%s steps:%llu source_rules:%llu source_n:%llu payload:%llu\n",
                kdna_status_str(rc), (unsigned long long)h.step_count, (unsigned long long)h.source_rule_count,
                (unsigned long long)h.source_n, (unsigned long long)h.payload_bytes);
        } else if (memcmp(magic, KDNA_KGEN_MAGIC, 8u) == 0) {
            kdna_kgen_header h; int rc = kdna_kgen_read_header_file(path, &h);
            printf("  type: KGEN rc:%s n:%llu steps:%llu fields:%u payload:%llu dt:%.17g seed:%llu\n",
                kdna_status_str(rc), (unsigned long long)h.n, (unsigned long long)h.steps, h.field_count,
                (unsigned long long)h.payload_bytes, h.dt, (unsigned long long)h.seed);
        } else if (memcmp(magic, KDNA_KVAR_MAGIC, 8u) == 0) {
            kdna_kvar_header h; int rc = kdna_kvar_read_header_file(path, &h);
            printf("  type: KVAR rc:%s variants:%llu source_n:%llu payload:%llu x:[%.17g,%.17g]\n",
                kdna_status_str(rc), (unsigned long long)h.variant_count, (unsigned long long)h.source_n,
                (unsigned long long)h.payload_bytes, h.x_min, h.x_max);
        } else if (memcmp(magic, KDNA_KDAT_MAGIC, 8u) == 0) {
            kdna_kdat_header h;
            double *x = NULL;
            int rc = kdna_kdat_read_file(path, &h, &x);
            if (x) free(x);
            printf("  type: KDAT rc:%s kind:%s n:%llu payload:%llu x:[%.17g,%.17g] seed:%llu\n",
                kdna_status_str(rc), kdna_kexp_kind_name(h.kind), (unsigned long long)h.n,
                (unsigned long long)h.payload_bytes, h.x_min, h.x_max, (unsigned long long)h.seed);

        } else if (memcmp(magic, "KFSUM001", 8u) == 0) {
            typedef struct kfsum_probe_header {
                char magic[8];
                uint32_t version;
                uint32_t header_bytes;
                uint32_t k;
                uint32_t flags;
                uint64_t bases_total;
                uint64_t bases_valid;
                uint64_t bases_invalid;
                uint64_t symbols_written;
                uint64_t reset_count;
                uint64_t contig_count;
                uint64_t max_symbols;
                uint64_t payload_bytes;
                uint64_t reserved[5];
            } kfsum_probe_header;
            FILE *f = fopen(path, "rb");
            kfsum_probe_header h;
            int ok = 0;
            if (f && fread(&h, 1u, sizeof(h), f) == sizeof(h) &&
                h.version == 1u && h.header_bytes == 128u &&
                h.payload_bytes == h.symbols_written * 8u) ok = 1;
            if (f) fclose(f);
            printf("  type: KFSUM rc:%s k:%u symbols:%llu bases_total:%llu valid:%llu invalid:%llu resets:%llu contigs:%llu max_symbols:%llu payload:%llu flags:0x%x\n",
                ok ? "ok" : "bad",
                h.k,
                (unsigned long long)h.symbols_written,
                (unsigned long long)h.bases_total,
                (unsigned long long)h.bases_valid,
                (unsigned long long)h.bases_invalid,
                (unsigned long long)h.reset_count,
                (unsigned long long)h.contig_count,
                (unsigned long long)h.max_symbols,
                (unsigned long long)h.payload_bytes,
                h.flags);

        } else if (memcmp(magic, KDNA_KGENOME_MAGIC, 8u) == 0) {
            FILE *f = fopen(path, "rb");
            kdna_kgenome_header h;
            int ok = 0;
            if (f && fread(&h, 1u, sizeof(h), f) == sizeof(h) &&
                h.header_bytes == KDNA_KGENOME_HEADER_BYTES &&
                h.record_bytes == KDNA_KGENOME_RECORD_BYTES &&
                h.payload_bytes == h.matrix_count * KDNA_KGENOME_RECORD_BYTES) ok = 1;
            if (f) fclose(f);
            printf("  type: KGENOME rc:%s sources:%llu cells:%llu record_bytes:%u payload:%llu train:%.17g bins:%u\n",
                ok ? "ok" : "bad",
                (unsigned long long)h.source_count,
                (unsigned long long)h.matrix_count,
                h.record_bytes,
                (unsigned long long)h.payload_bytes,
                h.train_ratio,
                h.bins);
        } else if (memcmp(magic, KDNA_KMAP_MAGIC, 8u) == 0) {
            kdna_kmap_header h; int rc = kdna_kmap_read_header_file(path, &h);
            printf("  type: KMAP rc:%s n:%llu mode:%s record_bytes:%u payload:%llu x:[%.17g,%.17g] k:%llu\n",
                kdna_status_str(rc), (unsigned long long)h.n, kdna_kmap_mode_name(h.mode), h.record_bytes,
                (unsigned long long)h.payload_bytes, h.x_min, h.x_max, (unsigned long long)h.kmer_k);
        } else if (memcmp(magic, KDNA_KREP_MAGIC, 8u) == 0) {
            kdna_krep_header h;
            kdna_kexp_result_record *records = NULL;
            int rc = kdna_krep_read_file(path, &h, &records);
            if (records) free(records);
            printf("  type: KREP rc:%s experiments:%llu record_bytes:%u payload:%llu flags:0x%llx\n",
                kdna_status_str(rc), (unsigned long long)h.experiment_count, h.record_bytes,
                (unsigned long long)h.payload_bytes, (unsigned long long)h.flags);
        } else if (memcmp(magic, KDNA_DYN_MAGIC, 8u) == 0) {
            FILE *f = fopen(path, "rb");
            kdna_dyn_header h;
            int rc = KDNA_EIO;
            if (f && fread(&h, 1u, sizeof(h), f) == sizeof(h)) rc = KDNA_OK;
            if (f) fclose(f);
            if (rc == KDNA_OK && (memcmp(h.magic, KDNA_DYN_MAGIC, 8u) != 0 || h.header_bytes != KDNA_DYN_HEADER_BYTES || h.field_count != KDNA_DYN_FIELDS)) rc = KDNA_EINVAL;
            printf("  type: KDYN rc:%s n:%llu steps:%llu fields:%u payload:%llu dt:%.17g flags:0x%llx\n",
                kdna_status_str(rc), (unsigned long long)h.n, (unsigned long long)h.steps, h.field_count,
                (unsigned long long)h.payload_bytes, h.dt, (unsigned long long)h.flags);
        } else {
            printf("  type: unknown\n");
        }
    }
    return 0;
}
