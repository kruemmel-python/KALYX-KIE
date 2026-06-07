#include "kdna_kgen.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if(argc != 2){fprintf(stderr,"usage: test_kgen_opencl_diff kernel.cl\n"); return 2;}
    kdna_kgen_params p; kdna_kgen_default_params(&p);
    p.n=512u; p.steps=48u; p.dt=0.01; p.seed=0x4b47454e1234ull;
    kdna_constants c; kdna_default_constants(&c);
    uint64_t bytes=0; if(kdna_kgen_payload_bytes(p.n,&bytes)!=KDNA_OK) return 2;
    double *cpu=(double*)calloc(1,(size_t)bytes); double *gpu=(double*)calloc(1,(size_t)bytes);
    if(!cpu||!gpu){free(cpu);free(gpu);return 2;}
    int rc=kdna_kgen_eval_cpu(&p,&c,cpu); if(rc!=KDNA_OK){fprintf(stderr,"cpu fail\n");return 2;}
    rc=kdna_kgen_eval_opencl(&p,&c,argv[1],gpu);
    if(rc==KDNA_EOPENCL||rc==KDNA_ENO_DEVICE||rc==KDNA_EBUILD){printf("SKIP OpenCL genesis unavailable: %s\n",kdna_status_str(rc)); free(cpu); free(gpu); return 0;}
    if(rc!=KDNA_OK){fprintf(stderr,"opencl fail: %s\n",kdna_status_str(rc)); free(cpu);free(gpu);return 2;}
    double ma=0.0,mr=0.0; uint32_t mf=0; size_t mi=0; size_t raw=0,dom=0;
    for(uint32_t f=0; f<KDNA_KGEN_FIELDS; ++f){
        for(size_t i=0;i<p.n;i++){
            double a=cpu[kdna_kgen_idx(f,p.n,i)], b=gpu[kdna_kgen_idx(f,p.n,i)];
            double d=fabs(a-b), r=d/fmax(1.0,fmax(fabs(a),fabs(b)));
            if(d>ma){ma=d;mf=f;mi=i;} if(r>mr)mr=r;
        }
    }
    for(size_t i=0;i<p.n;i++){
        if((int)cpu[kdna_kgen_idx(KDNA_KGEN_RAW,p.n,i)]!=(int)gpu[kdna_kgen_idx(KDNA_KGEN_RAW,p.n,i)]) raw++;
        if((int)cpu[kdna_kgen_idx(KDNA_KGEN_DOM,p.n,i)]!=(int)gpu[kdna_kgen_idx(KDNA_KGEN_DOM,p.n,i)]) dom++;
    }
    printf("kgen_max_abs=%.17g field=%s index=%zu max_rel=%.17g RAW_mismatches=%zu D_mismatches=%zu\n",ma,kdna_kgen_field_name(mf),mi,mr,raw,dom);
    if(raw||dom||mr>1e-9||ma>2e-5){fprintf(stderr,"diff too high\n"); free(cpu);free(gpu); return 1;}
    free(cpu); free(gpu); return 0;
}
