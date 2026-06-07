#include "kdna_kgen.h"
#include "kdna_ksoa.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double clampd(double x, double a, double b) { return x < a ? a : (x > b ? b : x); }
static double finite0(double x, double hard_max) { return (isfinite(x) && fabs(x) < hard_max) ? x : 0.0; }
static double log_abs(double x, const kdna_constants *c) { return log(fabs(x) + c->eps); }
static double exp_clamp(double x, const kdna_constants *c) { return exp(clampd(x, c->exp_min, c->exp_max)); }
static double safe_div(double n, double d, const kdna_constants *c) {
    const double s = signbit(d) ? -1.0 : 1.0;
    const double ad = fabs(d);
    return n / (s * (ad > c->eps ? ad : c->eps));
}
static uint64_t hash64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}
static double unit_noise(uint64_t seed, uint64_t i) {
    const uint64_t h = hash64(seed ^ (i * 0x9e3779b97f4a7c15ULL));
    const double u = (double)(h >> 11) * (1.0 / 9007199254740992.0);
    return 2.0 * u - 1.0;
}

static void eval_k(double x, const kdna_constants *c, double *v) {
    const double D = c->cD - tanh(c->cD);
    const double r = safe_div(c->cA - (x + tanh(x)), x, c);
    const double k1 = finite0(
        sin(exp_clamp(log_abs(c->cA, c), c)) *
        safe_div(tanh(c->cB), exp_clamp(r, c), c), c->hard_max);
    const double k2 = finite0(log_abs(exp_clamp(tanh(cos(x)), c) * x, c), c->hard_max);
    const double folded = cos(sin(safe_div(x * x, x, c)));
    const double k3 = finite0(cos(-c->cC * safe_div(folded, D, c)), c->hard_max);
    const double exp_cE = exp_clamp(c->cE, c);
    const double exp_x = exp_clamp(x, c);
    const double a = sin(cos(x * exp_cE));
    const double b = exp_clamp(exp_x, c);
    const double cx = log_abs(x * cos(cos(exp_x)), c);
    const double k4 = finite0(tanh(safe_div(a, b, c) * cx), c->hard_max);
    const double cascade = (x + x * tanh(x)) * log_abs(x * cos(cos(exp_x)), c);
    const double k5 = finite0(tanh(safe_div(cascade, D, c)) * exp_cE, c->hard_max);

    const double ek = sqrt(k1*k1 + k2*k2 + k3*k3 + k4*k4 + k5*k5);
    const double ak = (k1 + 2.0*k2 + 3.0*k3 + 4.0*k4 + 5.0*k5) / 5.0;
    const double lock = tanh(ek + fabs(ak));
    const double s1 = fabs(k1) / 1.70;
    const double s2 = fabs(k2) / 0.45;
    const double s3 = fabs(k3 - 0.985) / 0.006;
    const double s4 = fabs(k4) / 0.035;
    const double s5 = fabs(k5) / 0.55;

    uint32_t raw = 1u; double raw_s = fabs(k1);
    if (fabs(k2) > raw_s) { raw_s = fabs(k2); raw = 2u; }
    if (fabs(k3) > raw_s) { raw_s = fabs(k3); raw = 3u; }
    if (fabs(k4) > raw_s) { raw_s = fabs(k4); raw = 4u; }
    if (fabs(k5) > raw_s) { raw_s = fabs(k5); raw = 5u; }

    uint32_t dom = 1u; double ds = s1;
    if (s2 > ds) { ds = s2; dom = 2u; }
    if (s3 > ds) { ds = s3; dom = 3u; }
    if (s4 > ds) { ds = s4; dom = 4u; }
    if (s5 > ds) { ds = s5; dom = 5u; }

    v[0]=k1; v[1]=k2; v[2]=k3; v[3]=k4; v[4]=k5; v[5]=ek; v[6]=ak; v[7]=lock;
    v[8]=(double)raw; v[9]=(double)dom; v[10]=s1; v[11]=s2; v[12]=s3; v[13]=s4; v[14]=s5; v[15]=ds;
}

uint64_t kdna_kgen_variant_id(uint32_t raw, uint32_t dom, double lock, double score,
                              double k1, double k2, double k3, double k4, double k5) {
    int64_t ql = (int64_t)floor(lock * 4096.0);
    int64_t qs = (int64_t)floor(log1p(fabs(score)) * 2048.0);
    int64_t qk[5] = {
        (int64_t)floor(tanh(k1) * 32767.0),
        (int64_t)floor(tanh(k2) * 32767.0),
        (int64_t)floor(tanh(k3) * 32767.0),
        (int64_t)floor(tanh(k4) * 32767.0),
        (int64_t)floor(tanh(k5) * 32767.0)
    };
    uint64_t h = 0x4b444e415f564152ULL ^ (uint64_t)raw ^ ((uint64_t)dom << 8);
    h = hash64(h ^ (uint64_t)ql);
    h = hash64(h ^ ((uint64_t)qs << 1));
    for (int i=0;i<5;i++) h = hash64(h ^ (uint64_t)qk[i] ^ ((uint64_t)i << 56));
    return h;
}

void kdna_kgen_default_params(kdna_kgen_params *p) {
    if (!p) return;
    p->n = 4096u;
    p->steps = 128u;
    p->dt = 0.01;
    p->x_min = -8.0;
    p->x_max = 8.0;
    p->seed = 0x4b444e415f4b4745ULL;
    p->sigma = 0.35;
    p->energy_coupling = 0.12;
    p->phase_coupling = 0.10;
    p->damping = 0.015;
    p->drive = 0.05;
    p->resonance_threshold = 0.62;
}
const char *kdna_kgen_field_name(uint32_t f) {
    static const char *names[KDNA_KGEN_FIELDS] = {
        "E","Phi","x","dE","dPhi","phase_velocity","energy_velocity","coupling","resonance",
        "K01","K02","K03","K04","K05","E_K","A_K","Lock","RAW","D",
        "S01","S02","S03","S04","S05","dominanceScore","variant_id","resonance_id",
        "time","gate","injection_potential","stability","alignment"
    };
    return f < KDNA_KGEN_FIELDS ? names[f] : "unknown";
}
int kdna_kgen_payload_bytes(size_t n, uint64_t *bytes_out) {
    if (!bytes_out || n == 0u) return KDNA_EINVAL;
    if (n > ((size_t)-1) / (KDNA_KGEN_FIELDS * sizeof(double))) return KDNA_EINVAL;
    *bytes_out = (uint64_t)((size_t)KDNA_KGEN_FIELDS * n * sizeof(double));
    return KDNA_OK;
}
int kdna_kgen_init_header(kdna_kgen_header *h, const kdna_kgen_params *p) {
    if (!h || !p || p->n == 0u || p->steps == 0u || !(p->dt > 0.0) || !(p->x_max > p->x_min)) return KDNA_EINVAL;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;
    uint64_t pb = 0u; int rc = kdna_kgen_payload_bytes(p->n, &pb); if (rc != KDNA_OK) return rc;
    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KGEN_MAGIC, 8u);
    h->version = KDNA_KGEN_VERSION; h->header_bytes = KDNA_KGEN_HEADER_BYTES; h->field_count = KDNA_KGEN_FIELDS;
    h->n = (uint64_t)p->n; h->steps = (uint64_t)p->steps; h->seed = p->seed; h->dt = p->dt;
    h->x_min = p->x_min; h->x_max = p->x_max; h->dx = p->n > 1u ? (p->x_max-p->x_min)/(double)(p->n-1u) : 0.0;
    h->sigma = p->sigma; h->energy_coupling = p->energy_coupling; h->phase_coupling = p->phase_coupling;
    h->resonance_threshold = p->resonance_threshold; h->payload_bytes = pb;
    h->flags = KDNA_KGEN_FLAG_LE_IEEE754_DOUBLE | KDNA_KGEN_FLAG_SUBQG_GENESIS | KDNA_KGEN_FLAG_DETERMINISTIC | KDNA_KGEN_FLAG_RESIDENT_WAVES;
    return KDNA_OK;
}
int kdna_kgen_validate_header(const kdna_kgen_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KGEN_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KGEN_VERSION || h->header_bytes != KDNA_KGEN_HEADER_BYTES || h->field_count != KDNA_KGEN_FIELDS) return KDNA_EINVAL;
    if (h->n == 0u || h->steps == 0u || !(h->dt > 0.0) || !(h->x_max > h->x_min)) return KDNA_EINVAL;
    if (h->payload_bytes != h->n * (uint64_t)KDNA_KGEN_FIELDS * (uint64_t)sizeof(double)) return KDNA_EINVAL;
    if ((h->flags & KDNA_KGEN_FLAG_SUBQG_GENESIS) == 0u) return KDNA_EINVAL;
    return KDNA_OK;
}

static void kgen_initial(const kdna_kgen_params *p, size_t i, double *E, double *Phi, double *dE, double *dPhi) {
    const double dx = p->n > 1u ? (p->x_max - p->x_min) / (double)(p->n - 1u) : 0.0;
    const double q = p->x_min + dx * (double)i;
    const double no = unit_noise(p->seed, (uint64_t)i);
    const double ph = 0.17 * (double)i + 0.013 * no;
    *E = q + 0.35 * sin(ph) + 0.06 * no;
    *Phi = q + 0.35 * cos(ph * 0.73) - 0.04 * no;
    *dE = 0.02 * cos(ph);
    *dPhi = -0.02 * sin(ph * 0.73);
}

int kdna_kgen_eval_cpu(const kdna_kgen_params *p, const kdna_constants *c, double *out) {
    if (!p || !c || !out || p->n == 0u || p->steps == 0u) return KDNA_EINVAL;
    const size_t n = p->n;
    for (size_t i=0;i<n;i++) {
        double E, Phi, dE, dPhi;
        kgen_initial(p, i, &E, &Phi, &dE, &dPhi);
        double coupling_last = 0.0;
        for (size_t t=0;t<p->steps;t++) {
            const double x = 0.5 * (E + Phi);
            const double align = exp(-fabs(E - Phi) / (p->sigma > 1e-12 ? p->sigma : 1e-12));
            const double harmonic = sin(0.031 * (double)t + 0.007 * (double)i);
            const double coupling = p->energy_coupling * align + p->drive * harmonic;
            const double ndE = dE + p->dt * (-p->damping*dE + coupling - 0.003 * E + 0.018 * sin(Phi));
            const double ndPhi = dPhi + p->dt * (-p->damping*dPhi + p->phase_coupling * (E - Phi) + 0.011 * cos(x));
            E += p->dt * ndE;
            Phi += p->dt * ndPhi;
            dE = finite0(ndE, c->hard_max); dPhi = finite0(ndPhi, c->hard_max);
            E = finite0(E, c->hard_max); Phi = finite0(Phi, c->hard_max);
            coupling_last = coupling;
        }
        const double x = 0.5 * (E + Phi);
        double k[16]; eval_k(x, c, k);
        const double alignment = exp(-fabs(E - Phi) / (p->sigma > 1e-12 ? p->sigma : 1e-12));
        const double gate = tanh(log1p(fabs(k[15])) * alignment);
        const double resonance = alignment * k[7] * tanh(log1p(fabs(k[15])));
        const double injection = resonance * (0.5 + 0.5 * k[7]);
        const double stability = tanh(k[7] + alignment + 1.0/(1.0 + fabs(dE) + fabs(dPhi)));
        const uint64_t vid = kdna_kgen_variant_id((uint32_t)k[8], (uint32_t)k[9], k[7], k[15], k[0], k[1], k[2], k[3], k[4]);
        const uint64_t rid = hash64(vid ^ p->seed ^ (uint64_t)i ^ ((uint64_t)p->steps << 32));
        out[kdna_kgen_idx(KDNA_KGEN_E,n,i)] = E;
        out[kdna_kgen_idx(KDNA_KGEN_PHI,n,i)] = Phi;
        out[kdna_kgen_idx(KDNA_KGEN_X,n,i)] = x;
        out[kdna_kgen_idx(KDNA_KGEN_DE,n,i)] = dE;
        out[kdna_kgen_idx(KDNA_KGEN_DPHI,n,i)] = dPhi;
        out[kdna_kgen_idx(KDNA_KGEN_PHASE_VELOCITY,n,i)] = dPhi;
        out[kdna_kgen_idx(KDNA_KGEN_ENERGY_VELOCITY,n,i)] = dE;
        out[kdna_kgen_idx(KDNA_KGEN_COUPLING,n,i)] = coupling_last;
        out[kdna_kgen_idx(KDNA_KGEN_RESONANCE,n,i)] = resonance;
        for (uint32_t j=0;j<5;j++) out[kdna_kgen_idx(KDNA_KGEN_K01+j,n,i)] = k[j];
        out[kdna_kgen_idx(KDNA_KGEN_EK,n,i)] = k[5];
        out[kdna_kgen_idx(KDNA_KGEN_AK,n,i)] = k[6];
        out[kdna_kgen_idx(KDNA_KGEN_LOCK,n,i)] = k[7];
        out[kdna_kgen_idx(KDNA_KGEN_RAW,n,i)] = k[8];
        out[kdna_kgen_idx(KDNA_KGEN_DOM,n,i)] = k[9];
        for (uint32_t j=0;j<5;j++) out[kdna_kgen_idx(KDNA_KGEN_S01+j,n,i)] = k[10+j];
        out[kdna_kgen_idx(KDNA_KGEN_DOM_SCORE,n,i)] = k[15];
        out[kdna_kgen_idx(KDNA_KGEN_VARIANT_ID,n,i)] = (double)vid;
        out[kdna_kgen_idx(KDNA_KGEN_RESONANCE_ID,n,i)] = (double)rid;
        out[kdna_kgen_idx(KDNA_KGEN_TIME,n,i)] = (double)p->steps * p->dt;
        out[kdna_kgen_idx(KDNA_KGEN_GATE,n,i)] = gate;
        out[kdna_kgen_idx(KDNA_KGEN_INJECTION_POTENTIAL,n,i)] = injection;
        out[kdna_kgen_idx(KDNA_KGEN_STABILITY,n,i)] = stability;
        out[kdna_kgen_idx(KDNA_KGEN_ALIGNMENT,n,i)] = alignment;
    }
    return KDNA_OK;
}

/* OpenCL path: the kernel performs the same resident SUBQG E/Phi stepping per point. */
typedef intptr_t cl_context_properties; typedef int32_t cl_int; typedef uint32_t cl_uint; typedef uint64_t cl_ulong; typedef uint64_t cl_device_type; typedef uint64_t cl_command_queue_properties; typedef uint64_t cl_mem_flags; typedef uint64_t cl_bool;
typedef struct _cl_platform_id *cl_platform_id; typedef struct _cl_device_id *cl_device_id; typedef struct _cl_context *cl_context; typedef struct _cl_command_queue *cl_command_queue; typedef struct _cl_program *cl_program; typedef struct _cl_kernel *cl_kernel; typedef struct _cl_mem *cl_mem;
#define CL_SUCCESS 0
#define CL_DEVICE_TYPE_GPU (1ull << 2)
#define CL_DEVICE_TYPE_DEFAULT (1ull << 0)
#define CL_MEM_WRITE_ONLY (1ull << 1)
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
typedef cl_int (*p_clReleaseMemObject)(cl_mem); typedef cl_int (*p_clReleaseKernel)(cl_kernel); typedef cl_int (*p_clReleaseProgram)(cl_program); typedef cl_int (*p_clReleaseCommandQueue)(cl_command_queue); typedef cl_int (*p_clReleaseContext)(cl_context);
#if defined(_WIN32)
typedef HMODULE dynlib;
#define LOADSYM(lib,name) GetProcAddress((lib),(name))
static dynlib loadlib(void){return LoadLibraryA("OpenCL.dll");} static void closelib(dynlib l){if(l)FreeLibrary(l);}
#else
typedef void *dynlib;
#define LOADSYM(lib,name) dlsym((lib),(name))
static dynlib loadlib(void){dynlib l=dlopen("libOpenCL.so.1",RTLD_NOW|RTLD_LOCAL); if(!l)l=dlopen("libOpenCL.so",RTLD_NOW|RTLD_LOCAL); return l;} static void closelib(dynlib l){if(l)dlclose(l);}
#endif
typedef struct api { dynlib lib; p_clGetPlatformIDs clGetPlatformIDs; p_clGetDeviceIDs clGetDeviceIDs; p_clCreateContext clCreateContext; p_clCreateCommandQueue clCreateCommandQueue; p_clCreateProgramWithSource clCreateProgramWithSource; p_clBuildProgram clBuildProgram; p_clGetProgramBuildInfo clGetProgramBuildInfo; p_clCreateKernel clCreateKernel; p_clCreateBuffer clCreateBuffer; p_clSetKernelArg clSetKernelArg; p_clEnqueueNDRangeKernel clEnqueueNDRangeKernel; p_clEnqueueReadBuffer clEnqueueReadBuffer; p_clFinish clFinish; p_clReleaseMemObject clReleaseMemObject; p_clReleaseKernel clReleaseKernel; p_clReleaseProgram clReleaseProgram; p_clReleaseCommandQueue clReleaseCommandQueue; p_clReleaseContext clReleaseContext; } api;
#define LS(a,n) do{(a)->n=(p_##n)LOADSYM((a)->lib,#n); if(!(a)->n) return KDNA_EOPENCL;}while(0)
static int load_api(api *a){memset(a,0,sizeof(*a)); a->lib=loadlib(); if(!a->lib)return KDNA_EOPENCL; LS(a,clGetPlatformIDs); LS(a,clGetDeviceIDs); LS(a,clCreateContext); LS(a,clCreateCommandQueue); LS(a,clCreateProgramWithSource); LS(a,clBuildProgram); LS(a,clGetProgramBuildInfo); LS(a,clCreateKernel); LS(a,clCreateBuffer); LS(a,clSetKernelArg); LS(a,clEnqueueNDRangeKernel); LS(a,clEnqueueReadBuffer); LS(a,clFinish); LS(a,clReleaseMemObject); LS(a,clReleaseKernel); LS(a,clReleaseProgram); LS(a,clReleaseCommandQueue); LS(a,clReleaseContext); return KDNA_OK;}
#undef LS
static char *read_file(const char *path, size_t *len_out){FILE*f=fopen(path,"rb"); if(!f)return NULL; fseek(f,0,SEEK_END); long n=ftell(f); rewind(f); if(n<0){fclose(f);return NULL;} char*b=(char*)calloc((size_t)n+1,1); if(!b){fclose(f);return NULL;} if(fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;} fclose(f); *len_out=(size_t)n; return b;}

int kdna_kgen_eval_opencl(const kdna_kgen_params *p, const kdna_constants *c, const char *kernel_path, double *out) {
    if (!p || !c || !kernel_path || !out || p->n == 0u) return KDNA_EINVAL;
    api cl; int rc=load_api(&cl); if(rc!=KDNA_OK) return rc;
    char *src=NULL; cl_context ctx=NULL; cl_command_queue q=NULL; cl_program prog=NULL; cl_kernel ker=NULL; cl_mem obuf=NULL;
    cl_uint np=0; cl_platform_id *plats=NULL; cl_platform_id plat=NULL; cl_device_id dev=NULL; cl_int e=0;
    if(cl.clGetPlatformIDs(0,NULL,&np)!=CL_SUCCESS || np==0){rc=KDNA_ENO_DEVICE; goto done;}
    plats=(cl_platform_id*)calloc(np,sizeof(*plats)); if(!plats){rc=KDNA_ENOMEM;goto done;} cl.clGetPlatformIDs(np,plats,NULL);
    for(cl_uint i=0;i<np&&!dev;i++){cl_device_id d=NULL; if(cl.clGetDeviceIDs(plats[i],CL_DEVICE_TYPE_GPU,1,&d,NULL)==CL_SUCCESS){plat=plats[i];dev=d;}}
    for(cl_uint i=0;i<np&&!dev;i++){cl_device_id d=NULL; if(cl.clGetDeviceIDs(plats[i],CL_DEVICE_TYPE_DEFAULT,1,&d,NULL)==CL_SUCCESS){plat=plats[i];dev=d;}}
    free(plats); plats=NULL; if(!dev){rc=KDNA_ENO_DEVICE;goto done;}
    cl_context_properties props[]={CL_CONTEXT_PLATFORM,(cl_context_properties)plat,0};
    ctx=cl.clCreateContext(props,1,&dev,NULL,NULL,&e); if(!ctx||e!=CL_SUCCESS){rc=KDNA_EOPENCL;goto done;}
    q=cl.clCreateCommandQueue(ctx,dev,0,&e); if(!q||e!=CL_SUCCESS){rc=KDNA_EOPENCL;goto done;}
    size_t slen=0; src=read_file(kernel_path,&slen); if(!src){rc=KDNA_EIO;goto done;}
    const char*srcs[]={src}; prog=cl.clCreateProgramWithSource(ctx,1,srcs,&slen,&e); if(!prog||e!=CL_SUCCESS){rc=KDNA_EOPENCL;goto done;}
    e=cl.clBuildProgram(prog,1,&dev,"-cl-std=CL1.2",NULL,NULL);
    if(e!=CL_SUCCESS){size_t logn=0; cl.clGetProgramBuildInfo(prog,dev,CL_PROGRAM_BUILD_LOG,0,NULL,&logn); if(logn){char*log=(char*)calloc(logn+1,1); if(log){cl.clGetProgramBuildInfo(prog,dev,CL_PROGRAM_BUILD_LOG,logn,log,NULL); fprintf(stderr,"%s\n",log); free(log);}} rc=KDNA_EBUILD;goto done;}
    ker=cl.clCreateKernel(prog,"kdna_kgen_kernel",&e); if(!ker||e!=CL_SUCCESS){rc=KDNA_EOPENCL;goto done;}
    uint64_t pb=0; rc=kdna_kgen_payload_bytes(p->n,&pb); if(rc!=KDNA_OK)goto done;
    obuf=cl.clCreateBuffer(ctx,CL_MEM_WRITE_ONLY,(size_t)pb,NULL,&e); if(!obuf||e!=CL_SUCCESS){rc=KDNA_EOPENCL;goto done;}
    cl_ulong n=(cl_ulong)p->n, steps=(cl_ulong)p->steps, seed=(cl_ulong)p->seed; cl_uint arg=0;
#define SET(v) do{e=cl.clSetKernelArg(ker,arg++,sizeof(v),&(v)); if(e!=CL_SUCCESS){rc=KDNA_EOPENCL;goto done;}}while(0)
    SET(obuf); SET(n); SET(steps); SET(p->dt); SET(p->x_min); SET(p->x_max); SET(seed); SET(p->sigma); SET(p->energy_coupling); SET(p->phase_coupling); SET(p->damping); SET(p->drive); SET(p->resonance_threshold);
    SET(c->cA); SET(c->cB); SET(c->cC); SET(c->cD); SET(c->cE); SET(c->eps); SET(c->exp_min); SET(c->exp_max); SET(c->hard_max);
#undef SET
    { size_t g=p->n; e=cl.clEnqueueNDRangeKernel(q,ker,1,NULL,&g,NULL,0,NULL,NULL); if(e!=CL_SUCCESS||cl.clFinish(q)!=CL_SUCCESS){rc=KDNA_EOPENCL;goto done;} }
    e=cl.clEnqueueReadBuffer(q,obuf,CL_TRUE,0,(size_t)pb,out,0,NULL,NULL); if(e!=CL_SUCCESS){rc=KDNA_EOPENCL;goto done;}
done:
    if(obuf)cl.clReleaseMemObject(obuf); if(ker)cl.clReleaseKernel(ker); if(prog)cl.clReleaseProgram(prog); if(q)cl.clReleaseCommandQueue(q); if(ctx)cl.clReleaseContext(ctx); if(src)free(src); if(plats)free(plats); closelib(cl.lib); return rc;
}

int kdna_kgen_write_file(const char *path, const kdna_kgen_header *h, const double *payload) {
    if (!path || !h || !payload) return KDNA_EINVAL;
    int rc = kdna_kgen_validate_header(h); if (rc != KDNA_OK) return rc;
    FILE *f=fopen(path,"wb"); if(!f)return KDNA_EIO; int ok=1;
    if(fwrite(h,1,sizeof(*h),f)!=sizeof(*h))ok=0;
    if(ok && fwrite(payload,1,(size_t)h->payload_bytes,f)!=(size_t)h->payload_bytes)ok=0;
    if(fclose(f)!=0)ok=0; return ok?KDNA_OK:KDNA_EIO;
}
int kdna_kgen_read_header_file(const char *path, kdna_kgen_header *h) {
    if(!path||!h)return KDNA_EINVAL; FILE*f=fopen(path,"rb"); if(!f)return KDNA_EIO; size_t g=fread(h,1,sizeof(*h),f); int ok=fclose(f)==0; if(g!=sizeof(*h)||!ok)return KDNA_EIO; return kdna_kgen_validate_header(h);
}
int kdna_kgen_read_file(const char *path, kdna_kgen_header *h, double **payload_out) {
    if(!path||!h||!payload_out)return KDNA_EINVAL; *payload_out=NULL; FILE*f=fopen(path,"rb"); if(!f)return KDNA_EIO;
    if(fread(h,1,sizeof(*h),f)!=sizeof(*h)){fclose(f);return KDNA_EIO;} int rc=kdna_kgen_validate_header(h); if(rc!=KDNA_OK){fclose(f);return rc;}
    double *p=(double*)malloc((size_t)h->payload_bytes); if(!p){fclose(f);return KDNA_ENOMEM;}
    if(fread(p,1,(size_t)h->payload_bytes,f)!=(size_t)h->payload_bytes){free(p);fclose(f);return KDNA_EIO;}
    if(fclose(f)!=0){free(p);return KDNA_EIO;} *payload_out=p; return KDNA_OK;
}
