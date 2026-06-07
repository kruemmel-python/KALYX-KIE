#include "kdna.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef intptr_t cl_context_properties;
typedef int32_t cl_int;
typedef uint32_t cl_uint;
typedef uint64_t cl_ulong;
typedef uint64_t cl_device_type;
typedef uint64_t cl_command_queue_properties;
typedef uint64_t cl_mem_flags;
typedef uint64_t cl_bool;
typedef struct _cl_platform_id *cl_platform_id;
typedef struct _cl_device_id *cl_device_id;
typedef struct _cl_context *cl_context;
typedef struct _cl_command_queue *cl_command_queue;
typedef struct _cl_program *cl_program;
typedef struct _cl_kernel *cl_kernel;
typedef struct _cl_mem *cl_mem;

#define CL_SUCCESS 0
#define CL_DEVICE_TYPE_GPU (1ull << 2)
#define CL_DEVICE_TYPE_DEFAULT (1ull << 0)
#define CL_MEM_READ_ONLY (1ull << 2)
#define CL_MEM_WRITE_ONLY (1ull << 1)
#define CL_MEM_COPY_HOST_PTR (1ull << 5)
#define CL_TRUE 1
#define CL_PROGRAM_BUILD_LOG 0x1183
#define CL_CONTEXT_PLATFORM 0x1084

typedef cl_int (*p_clGetPlatformIDs)(cl_uint, cl_platform_id *, cl_uint *);
typedef cl_int (*p_clGetDeviceIDs)(cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *);
typedef cl_context (*p_clCreateContext)(const cl_context_properties *, cl_uint, const cl_device_id *, void *, void *, cl_int *);
typedef cl_command_queue (*p_clCreateCommandQueue)(cl_context, cl_device_id, cl_command_queue_properties, cl_int *);
typedef cl_program (*p_clCreateProgramWithSource)(cl_context, cl_uint, const char **, const size_t *, cl_int *);
typedef cl_int (*p_clBuildProgram)(cl_program, cl_uint, const cl_device_id *, const char *, void *, void *);
typedef cl_int (*p_clGetProgramBuildInfo)(cl_program, cl_device_id, cl_uint, size_t, void *, size_t *);
typedef cl_kernel (*p_clCreateKernel)(cl_program, const char *, cl_int *);
typedef cl_mem (*p_clCreateBuffer)(cl_context, cl_mem_flags, size_t, void *, cl_int *);
typedef cl_int (*p_clSetKernelArg)(cl_kernel, cl_uint, size_t, const void *);
typedef cl_int (*p_clEnqueueNDRangeKernel)(cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *, cl_uint, const void *, void *);
typedef cl_int (*p_clEnqueueReadBuffer)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, void *, cl_uint, const void *, void *);
typedef cl_int (*p_clFinish)(cl_command_queue);
typedef cl_int (*p_clReleaseMemObject)(cl_mem);
typedef cl_int (*p_clReleaseKernel)(cl_kernel);
typedef cl_int (*p_clReleaseProgram)(cl_program);
typedef cl_int (*p_clReleaseCommandQueue)(cl_command_queue);
typedef cl_int (*p_clReleaseContext)(cl_context);

#if defined(_WIN32)
typedef HMODULE kdna_dynlib;
#else
typedef void *kdna_dynlib;
#endif

typedef struct ocl_api {
    kdna_dynlib lib;
    p_clGetPlatformIDs clGetPlatformIDs;
    p_clGetDeviceIDs clGetDeviceIDs;
    p_clCreateContext clCreateContext;
    p_clCreateCommandQueue clCreateCommandQueue;
    p_clCreateProgramWithSource clCreateProgramWithSource;
    p_clBuildProgram clBuildProgram;
    p_clGetProgramBuildInfo clGetProgramBuildInfo;
    p_clCreateKernel clCreateKernel;
    p_clCreateBuffer clCreateBuffer;
    p_clSetKernelArg clSetKernelArg;
    p_clEnqueueNDRangeKernel clEnqueueNDRangeKernel;
    p_clEnqueueReadBuffer clEnqueueReadBuffer;
    p_clFinish clFinish;
    p_clReleaseMemObject clReleaseMemObject;
    p_clReleaseKernel clReleaseKernel;
    p_clReleaseProgram clReleaseProgram;
    p_clReleaseCommandQueue clReleaseCommandQueue;
    p_clReleaseContext clReleaseContext;
} ocl_api;

#if defined(_WIN32)
#define KDNA_LOAD_SYMBOL(lib, name) GetProcAddress((lib), (name))
static kdna_dynlib kdna_opencl_load_library(void) {
    kdna_dynlib lib = LoadLibraryA("OpenCL.dll");
    return lib;
}
static void kdna_opencl_close_library(kdna_dynlib lib) {
    if (lib) FreeLibrary(lib);
}
#else
#define KDNA_LOAD_SYMBOL(lib, name) dlsym((lib), (name))
static kdna_dynlib kdna_opencl_load_library(void) {
    kdna_dynlib lib = dlopen("libOpenCL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!lib) lib = dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL);
    return lib;
}
static void kdna_opencl_close_library(kdna_dynlib lib) {
    if (lib) dlclose(lib);
}
#endif

#define LOAD_SYM(api, name) do { \
    (api)->name = (p_##name)KDNA_LOAD_SYMBOL((api)->lib, #name); \
    if (!(api)->name) return KDNA_EOPENCL; \
} while (0)

static int load_api(ocl_api *api) {
    memset(api, 0, sizeof(*api));
    api->lib = kdna_opencl_load_library();
    if (!api->lib) return KDNA_EOPENCL;
    LOAD_SYM(api, clGetPlatformIDs);
    LOAD_SYM(api, clGetDeviceIDs);
    LOAD_SYM(api, clCreateContext);
    LOAD_SYM(api, clCreateCommandQueue);
    LOAD_SYM(api, clCreateProgramWithSource);
    LOAD_SYM(api, clBuildProgram);
    LOAD_SYM(api, clGetProgramBuildInfo);
    LOAD_SYM(api, clCreateKernel);
    LOAD_SYM(api, clCreateBuffer);
    LOAD_SYM(api, clSetKernelArg);
    LOAD_SYM(api, clEnqueueNDRangeKernel);
    LOAD_SYM(api, clEnqueueReadBuffer);
    LOAD_SYM(api, clFinish);
    LOAD_SYM(api, clReleaseMemObject);
    LOAD_SYM(api, clReleaseKernel);
    LOAD_SYM(api, clReleaseProgram);
    LOAD_SYM(api, clReleaseCommandQueue);
    LOAD_SYM(api, clReleaseContext);
    return KDNA_OK;
}

static char *read_file(const char *path, size_t *len_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = (char *)calloc((size_t)n + 1u, 1u);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1u, (size_t)n, f);
    fclose(f);
    if (got != (size_t)n) { free(buf); return NULL; }
    *len_out = got;
    return buf;
}

int kdna_eval_opencl(const double *x, double *out, size_t n, const kdna_constants *constants, const char *kernel_path) {
    if (!x || !out || n == 0u || !kernel_path) return KDNA_EINVAL;
    kdna_constants local;
    if (!constants) {
        kdna_default_constants(&local);
        constants = &local;
    }

    int rc = KDNA_OK;
    ocl_api cl;
    rc = load_api(&cl);
    if (rc != KDNA_OK) return rc;

    char *source = NULL;
    cl_context ctx = NULL;
    cl_command_queue q = NULL;
    cl_program prog = NULL;
    cl_kernel kernel = NULL;
    cl_mem xbuf = NULL, obuf = NULL;

    cl_uint np = 0;
    cl_int e = cl.clGetPlatformIDs(0, NULL, &np);
    if (e != CL_SUCCESS || np == 0u) { rc = KDNA_ENO_DEVICE; goto done; }

    cl_platform_id *plats = (cl_platform_id *)calloc(np, sizeof(*plats));
    if (!plats) { rc = KDNA_ENOMEM; goto done; }
    e = cl.clGetPlatformIDs(np, plats, NULL);
    if (e != CL_SUCCESS) { free(plats); rc = KDNA_EOPENCL; goto done; }

    cl_platform_id plat = NULL;
    cl_device_id dev = NULL;
    for (cl_uint i = 0; i < np && !dev; ++i) {
        cl_device_id d = NULL;
        if (cl.clGetDeviceIDs(plats[i], CL_DEVICE_TYPE_GPU, 1, &d, NULL) == CL_SUCCESS) {
            plat = plats[i]; dev = d;
        }
    }
    for (cl_uint i = 0; i < np && !dev; ++i) {
        cl_device_id d = NULL;
        if (cl.clGetDeviceIDs(plats[i], CL_DEVICE_TYPE_DEFAULT, 1, &d, NULL) == CL_SUCCESS) {
            plat = plats[i]; dev = d;
        }
    }
    free(plats);
    if (!dev) { rc = KDNA_ENO_DEVICE; goto done; }

    cl_context_properties props[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)plat, 0 };
    ctx = cl.clCreateContext(props, 1, &dev, NULL, NULL, &e);
    if (!ctx || e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

    q = cl.clCreateCommandQueue(ctx, dev, 0, &e);
    if (!q || e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

    size_t source_len = 0;
    source = read_file(kernel_path, &source_len);
    if (!source) { rc = KDNA_EIO; goto done; }

    const char *srcs[] = { source };
    prog = cl.clCreateProgramWithSource(ctx, 1, srcs, &source_len, &e);
    if (!prog || e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

    e = cl.clBuildProgram(prog, 1, &dev, "-cl-std=CL1.2", NULL, NULL);
    if (e != CL_SUCCESS) {
        size_t log_len = 0;
        cl.clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_len);
        if (log_len > 1u) {
            char *log = (char *)calloc(log_len + 1u, 1u);
            if (log) {
                cl.clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, log_len, log, NULL);
                fprintf(stderr, "OpenCL build log:\n%s\n", log);
                free(log);
            }
        }
        rc = KDNA_EBUILD; goto done;
    }

    kernel = cl.clCreateKernel(prog, "kdna_eval_kernel", &e);
    if (!kernel || e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

    const size_t out_len = KDNA_FIELDS * n;
    xbuf = cl.clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n * sizeof(double), (void *)x, &e);
    if (!xbuf || e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }
    obuf = cl.clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, out_len * sizeof(double), NULL, &e);
    if (!obuf || e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

    cl_ulong nn = (cl_ulong)n;
    e  = cl.clSetKernelArg(kernel, 0, sizeof(xbuf), &xbuf);
    e |= cl.clSetKernelArg(kernel, 1, sizeof(obuf), &obuf);
    e |= cl.clSetKernelArg(kernel, 2, sizeof(nn), &nn);
    e |= cl.clSetKernelArg(kernel, 3, sizeof(constants->cA), &constants->cA);
    e |= cl.clSetKernelArg(kernel, 4, sizeof(constants->cB), &constants->cB);
    e |= cl.clSetKernelArg(kernel, 5, sizeof(constants->cC), &constants->cC);
    e |= cl.clSetKernelArg(kernel, 6, sizeof(constants->cD), &constants->cD);
    e |= cl.clSetKernelArg(kernel, 7, sizeof(constants->cE), &constants->cE);
    e |= cl.clSetKernelArg(kernel, 8, sizeof(constants->eps), &constants->eps);
    e |= cl.clSetKernelArg(kernel, 9, sizeof(constants->exp_min), &constants->exp_min);
    e |= cl.clSetKernelArg(kernel, 10, sizeof(constants->exp_max), &constants->exp_max);
    e |= cl.clSetKernelArg(kernel, 11, sizeof(constants->hard_max), &constants->hard_max);
    if (e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

    size_t global = n;
    e = cl.clEnqueueNDRangeKernel(q, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }
    e = cl.clFinish(q);
    if (e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }
    e = cl.clEnqueueReadBuffer(q, obuf, CL_TRUE, 0, out_len * sizeof(double), out, 0, NULL, NULL);
    if (e != CL_SUCCESS) { rc = KDNA_EOPENCL; goto done; }

done:
    if (obuf) cl.clReleaseMemObject(obuf);
    if (xbuf) cl.clReleaseMemObject(xbuf);
    if (kernel) cl.clReleaseKernel(kernel);
    if (prog) cl.clReleaseProgram(prog);
    if (q) cl.clReleaseCommandQueue(q);
    if (ctx) cl.clReleaseContext(ctx);
    free(source);
    kdna_opencl_close_library(cl.lib);
    return rc;
}
