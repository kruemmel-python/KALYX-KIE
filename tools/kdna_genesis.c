#include "kdna_kgen.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
    printf("kdna_genesis\n");
    printf("  simulate: --out file.kgen --backend cpu|opencl [--kernel kernels/kdna_genesis.cl] [--n N] [--steps S] [--dt DT] [--seed SEED]\n");
    printf("  inspect:  --a file.kgen [--b other.kgen]\n");
}

static double field_min(const double *p, size_t n, uint32_t f) {
    double m = p[kdna_kgen_idx(f,n,0u)];
    for (size_t i=1;i<n;i++) { double v=p[kdna_kgen_idx(f,n,i)]; if(v<m)m=v; }
    return m;
}
static double field_max(const double *p, size_t n, uint32_t f) {
    double m = p[kdna_kgen_idx(f,n,0u)];
    for (size_t i=1;i<n;i++) { double v=p[kdna_kgen_idx(f,n,i)]; if(v>m)m=v; }
    return m;
}
static void inspect_one(const kdna_kgen_header *h, const double *p) {
    printf("file: KGEN n:%llu steps:%llu fields:%u dt:%.17g x:[%.17g,%.17g] seed:%llu payload:%llu\n",
        (unsigned long long)h->n,(unsigned long long)h->steps,h->field_count,h->dt,h->x_min,h->x_max,
        (unsigned long long)h->seed,(unsigned long long)h->payload_bytes);
    printf("SUBQG genesis stats:\n");
    size_t n=(size_t)h->n;
    for (uint32_t f=0; f<KDNA_KGEN_FIELDS; ++f) {
        double mn=field_min(p,n,f), mx=field_max(p,n,f), ma=0.0; size_t non=0u;
        for(size_t i=0;i<n;i++){double v=p[kdna_kgen_idx(f,n,i)]; if(!isfinite(v))non++; if(fabs(v)>ma)ma=fabs(v);}
        printf("  %-20s min:% .17e max:% .17e maxAbs:% .17e nonfinite:%zu\n", kdna_kgen_field_name(f), mn, mx, ma, non);
    }
}
static int compare(const kdna_kgen_header *a, const double *pa, const kdna_kgen_header *b, const double *pb) {
    if (a->n != b->n || a->field_count != b->field_count) { fprintf(stderr,"dimension mismatch\n"); return 2; }
    size_t n=(size_t)a->n; double ga=0.0, gr=0.0; uint32_t gf=0; size_t gi=0; size_t raw_mis=0, dom_mis=0, var_mis=0;
    for(uint32_t f=0; f<KDNA_KGEN_FIELDS; ++f) {
        double ma=0.0, mr=0.0; size_t mi=0;
        for(size_t i=0;i<n;i++){
            double x=pa[kdna_kgen_idx(f,n,i)], y=pb[kdna_kgen_idx(f,n,i)];
            double d=fabs(x-y), r=d/fmax(1.0,fmax(fabs(x),fabs(y)));
            if(d>ma){ma=d;mi=i;} if(r>mr)mr=r;
        }
        if(ma>ga){ga=ma;gf=f;gi=mi;} if(mr>gr)gr=mr;
        printf("  %-20s max_abs:% .17e max_rel:% .17e at_i:%zu\n",kdna_kgen_field_name(f),ma,mr,mi);
    }
    for(size_t i=0;i<n;i++){
        if((int)pa[kdna_kgen_idx(KDNA_KGEN_RAW,n,i)]!=(int)pb[kdna_kgen_idx(KDNA_KGEN_RAW,n,i)]) raw_mis++;
        if((int)pa[kdna_kgen_idx(KDNA_KGEN_DOM,n,i)]!=(int)pb[kdna_kgen_idx(KDNA_KGEN_DOM,n,i)]) dom_mis++;
        if((uint64_t)pa[kdna_kgen_idx(KDNA_KGEN_VARIANT_ID,n,i)]!=(uint64_t)pb[kdna_kgen_idx(KDNA_KGEN_VARIANT_ID,n,i)]) var_mis++;
    }
    printf("RAW_mismatches:%zu D_mismatches:%zu variant_id_mismatches:%zu\n", raw_mis, dom_mis, var_mis);
    printf("global_max_abs:% .17e field:%s index:%zu\n",ga,kdna_kgen_field_name(gf),gi);
    printf("global_max_rel:% .17e\n",gr);
    return 0;
}

int main(int argc, char **argv) {
    const char *out=NULL,*backend="cpu",*kernel="kernels/kdna_genesis.cl",*a_path=NULL,*b_path=NULL;
    kdna_kgen_params p; kdna_kgen_default_params(&p);
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--out")==0 && i+1<argc) out=argv[++i];
        else if(strcmp(argv[i],"--backend")==0 && i+1<argc) backend=argv[++i];
        else if(strcmp(argv[i],"--kernel")==0 && i+1<argc) kernel=argv[++i];
        else if(strcmp(argv[i],"--n")==0 && i+1<argc) p.n=(size_t)strtoull(argv[++i],NULL,10);
        else if(strcmp(argv[i],"--steps")==0 && i+1<argc) p.steps=(size_t)strtoull(argv[++i],NULL,10);
        else if(strcmp(argv[i],"--dt")==0 && i+1<argc) p.dt=strtod(argv[++i],NULL);
        else if(strcmp(argv[i],"--seed")==0 && i+1<argc) p.seed=(uint64_t)strtoull(argv[++i],NULL,0);
        else if(strcmp(argv[i],"--sigma")==0 && i+1<argc) p.sigma=strtod(argv[++i],NULL);
        else if(strcmp(argv[i],"--a")==0 && i+1<argc) a_path=argv[++i];
        else if(strcmp(argv[i],"--b")==0 && i+1<argc) b_path=argv[++i];
        else if(strcmp(argv[i],"--help")==0){usage();return 0;}
        else {usage();return 2;}
    }
    if(a_path){
        kdna_kgen_header ha,hb; double *pa=NULL,*pb=NULL;
        int rc=kdna_kgen_read_file(a_path,&ha,&pa); if(rc!=KDNA_OK){fprintf(stderr,"cannot read %s: %s\n",a_path,kdna_status_str(rc));return 2;}
        inspect_one(&ha,pa);
        if(b_path){rc=kdna_kgen_read_file(b_path,&hb,&pb); if(rc!=KDNA_OK){fprintf(stderr,"cannot read %s: %s\n",b_path,kdna_status_str(rc));free(pa);return 2;} printf("compare:\n"); int cr=compare(&ha,pa,&hb,pb); free(pb); free(pa); return cr;}
        free(pa); return 0;
    }
    if(!out){usage();return 2;}
    kdna_constants c; kdna_default_constants(&c);
    uint64_t pb=0; int rc=kdna_kgen_payload_bytes(p.n,&pb); if(rc!=KDNA_OK)return 2;
    double *payload=(double*)calloc(1,(size_t)pb); if(!payload)return 2;
    if(strcmp(backend,"cpu")==0) rc=kdna_kgen_eval_cpu(&p,&c,payload);
    else if(strcmp(backend,"opencl")==0) rc=kdna_kgen_eval_opencl(&p,&c,kernel,payload);
    else {fprintf(stderr,"unknown backend\n"); free(payload); return 2;}
    if(rc!=KDNA_OK){fprintf(stderr,"genesis failed: %s\n",kdna_status_str(rc)); free(payload); return 3;}
    kdna_kgen_header h; rc=kdna_kgen_init_header(&h,&p); if(rc==KDNA_OK)rc=kdna_kgen_write_file(out,&h,payload);
    if(rc!=KDNA_OK){fprintf(stderr,"write failed: %s\n",kdna_status_str(rc)); free(payload); return 2;}
    printf("kdna_genesis: wrote %s backend=%s n=%zu steps=%zu fields=%u payload_bytes=%llu dt=%.17g seed=%llu\n",
        out,backend,p.n,p.steps,(unsigned)KDNA_KGEN_FIELDS,(unsigned long long)h.payload_bytes,p.dt,(unsigned long long)p.seed);
    free(payload); return 0;
}
