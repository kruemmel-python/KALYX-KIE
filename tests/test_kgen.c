#include "kdna_kgen.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr,"usage: test_kgen file.kgen min_n\n"); return 2; }
    kdna_kgen_header h; double *p=NULL;
    int rc=kdna_kgen_read_file(argv[1],&h,&p);
    if(rc!=KDNA_OK){fprintf(stderr,"read failed: %s\n",kdna_status_str(rc)); return 1;}
    size_t min_n=(size_t)strtoull(argv[2],NULL,10);
    if(h.n<min_n || h.field_count!=KDNA_KGEN_FIELDS || h.steps==0){fprintf(stderr,"bad header\n"); free(p); return 1;}
    size_t n=(size_t)h.n; size_t resonant=0, bad=0;
    for(size_t i=0;i<n;i++){
        double E=p[kdna_kgen_idx(KDNA_KGEN_E,n,i)];
        double Phi=p[kdna_kgen_idx(KDNA_KGEN_PHI,n,i)];
        double x=p[kdna_kgen_idx(KDNA_KGEN_X,n,i)];
        if(!isfinite(E)||!isfinite(Phi)||!isfinite(x)||fabs(x - 0.5*(E+Phi)) > 1e-10*fmax(1.0,fabs(x))) bad++;
        double raw=p[kdna_kgen_idx(KDNA_KGEN_RAW,n,i)];
        double dom=p[kdna_kgen_idx(KDNA_KGEN_DOM,n,i)];
        if(!(raw>=1.0&&raw<=5.0&&dom>=1.0&&dom<=5.0)) bad++;
        double r=p[kdna_kgen_idx(KDNA_KGEN_RESONANCE,n,i)];
        if(!isfinite(r) || r < -1e-12 || r > 1.000000000001) bad++;
        if(r>0.10) resonant++;
        double vid=p[kdna_kgen_idx(KDNA_KGEN_VARIANT_ID,n,i)];
        if(!(vid>0.0)) bad++;
    }
    if(bad){fprintf(stderr,"bad cells: %zu\n",bad); free(p); return 1;}
    printf("kgen_ok n=%llu steps=%llu resonant=%zu\n",(unsigned long long)h.n,(unsigned long long)h.steps,resonant);
    free(p); return 0;
}
