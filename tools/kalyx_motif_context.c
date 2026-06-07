
// kalyx_motif_context.c - KALYX-ORIGIN v0.7 Motif / Period Diagnostics
// Apache-2.0, Ralf Kruemmel / KALYX
//
// Purpose:
//   Raw FASTA-level motif decomposition for ORIGIN candidate windows.
//   Computes top-kmer table, reverse-complement pairing, top-kmer spacing,
//   and lightweight periodicity diagnostics.
//
// This is an origin diagnostic, not origin proof.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef KALYX_MOTIF_VERSION
#define KALYX_MOTIF_VERSION "KMOTIF007"
#endif

typedef struct {
    uint64_t key;
    uint64_t count;
    uint64_t first_pos;
    uint64_t last_pos;
    uint64_t gap_count;
    double gap_sum;
    double gap_sq_sum;
} kmer_count_t;

typedef struct {
    kmer_count_t *items;
    size_t cap;
    size_t used;
} kmer_table_t;

static uint64_t fnv1a64_buf(const unsigned char *p, size_t n) {
    uint64_t h = 1469598103934665603ull;
    for (size_t i=0;i<n;i++) { h ^= (uint64_t)p[i]; h *= 1099511628211ull; }
    return h;
}

static int parse_u64(const char *s, uint64_t *out) {
    if (!s || !*s) return 0;
    char *end = NULL;
    unsigned long long v = strtoull(s, &end, 0);
    if (!end || *end) return 0;
    *out = (uint64_t)v;
    return 1;
}

static int parse_u32(const char *s, uint32_t *out) {
    uint64_t v=0; if(!parse_u64(s,&v) || v>0xffffffffull) return 0; *out=(uint32_t)v; return 1;
}

static int base_bits(int c, uint64_t *b) {
    switch(toupper((unsigned char)c)) {
        case 'A': *b=0; return 1;
        case 'C': *b=1; return 1;
        case 'G': *b=2; return 1;
        case 'T': *b=3; return 1;
        default: return 0;
    }
}

static char bits_base(uint64_t b) {
    static const char map[4]={'A','C','G','T'};
    return map[b & 3u];
}

static uint64_t revcomp_key(uint64_t key, uint32_t k) {
    uint64_t rc=0;
    for(uint32_t i=0;i<k;i++) {
        uint64_t b = key & 3u;
        uint64_t cb = b ^ 3u; // A<->T, C<->G
        rc = (rc << 2) | cb;
        key >>= 2;
    }
    return rc;
}

static void key_to_seq(uint64_t key, uint32_t k, char *out) {
    for(int i=(int)k-1;i>=0;i--) {
        out[i]=bits_base(key & 3u);
        key >>= 2;
    }
    out[k]=0;
}

static size_t next_pow2(size_t x) {
    size_t p=1; while(p<x) p<<=1; return p;
}

static void table_init(kmer_table_t *t, size_t cap) {
    t->cap = next_pow2(cap < 1024 ? 1024 : cap);
    t->used = 0;
    t->items = (kmer_count_t*)calloc(t->cap, sizeof(kmer_count_t));
    if(!t->items) { fprintf(stderr,"oom table\n"); exit(3); }
}

static void table_free(kmer_table_t *t) {
    free(t->items); t->items=NULL; t->cap=t->used=0;
}

static void table_rehash(kmer_table_t *t);

static kmer_count_t* table_get(kmer_table_t *t, uint64_t key, uint64_t pos) {
    if ((t->used + 1) * 10 >= t->cap * 7) table_rehash(t);
    uint64_t h = key * 11400714819323198485ull;
    size_t idx = (size_t)(h & (t->cap - 1));
    while(1) {
        kmer_count_t *e = &t->items[idx];
        if(e->count == 0) {
            e->key = key; e->count = 0; e->first_pos = pos; e->last_pos = pos;
            e->gap_count = 0; e->gap_sum = 0.0; e->gap_sq_sum = 0.0;
            t->used++;
            return e;
        }
        if(e->key == key) return e;
        idx = (idx + 1) & (t->cap - 1);
    }
}

static void table_rehash(kmer_table_t *t) {
    kmer_count_t *old = t->items;
    size_t oldcap = t->cap;
    kmer_table_t nt; table_init(&nt, oldcap*2);
    for(size_t i=0;i<oldcap;i++) {
        if(old[i].count) {
            uint64_t h = old[i].key * 11400714819323198485ull;
            size_t idx = (size_t)(h & (nt.cap - 1));
            while(nt.items[idx].count) idx = (idx + 1) & (nt.cap - 1);
            nt.items[idx] = old[i];
            nt.used++;
        }
    }
    free(old);
    *t = nt;
}

static void table_add(kmer_table_t *t, uint64_t key, uint64_t pos) {
    kmer_count_t *e = table_get(t,key,pos);
    if(e->count > 0) {
        uint64_t gap = pos - e->last_pos;
        e->gap_count++;
        e->gap_sum += (double)gap;
        e->gap_sq_sum += (double)gap * (double)gap;
        e->last_pos = pos;
    } else {
        e->first_pos = pos; e->last_pos = pos;
    }
    e->count++;
}

static int cmp_count_desc(const void *a, const void *b) {
    const kmer_count_t *x=(const kmer_count_t*)a, *y=(const kmer_count_t*)b;
    if(y->count > x->count) return 1;
    if(y->count < x->count) return -1;
    if(x->key < y->key) return -1;
    if(x->key > y->key) return 1;
    return 0;
}

static unsigned char* load_fasta_sequence(const char *path, uint64_t *len_out, uint64_t counts[8]) {
    FILE *f = fopen(path, "rb");
    if(!f) { fprintf(stderr,"cannot open fasta: %s\n", path); return NULL; }
    size_t cap = 1<<20, n=0;
    unsigned char *seq = (unsigned char*)malloc(cap);
    if(!seq) { fclose(f); return NULL; }
    int c; int in_header=0;
    memset(counts,0,sizeof(uint64_t)*8);
    while((c=fgetc(f)) != EOF) {
        if(c=='>') { in_header=1; continue; }
        if(in_header) { if(c=='\n' || c=='\r') in_header=0; continue; }
        if(c=='\n' || c=='\r' || c==' ' || c=='\t') continue;
        int u=toupper((unsigned char)c);
        if(n==cap) { cap*=2; unsigned char *p=(unsigned char*)realloc(seq,cap); if(!p){free(seq);fclose(f);return NULL;} seq=p; }
        seq[n++] = (unsigned char)u;
        switch(u) {
            case 'A': counts[0]++; break; case 'C': counts[1]++; break;
            case 'G': counts[2]++; break; case 'T': counts[3]++; break;
            case 'N': counts[4]++; break; default: counts[5]++; break;
        }
    }
    fclose(f);
    *len_out = (uint64_t)n;
    return seq;
}

static void usage(void) {
    puts("kalyx_motif_context v0.7 --fasta chr.fa --base-start N --base-end N --out-dir DIR [options]");
    puts("");
    puts("Options:");
    puts("  --label NAME             label for this region, default: region");
    puts("  --k N                    k-mer length 1..31, default: 16");
    puts("  --top N                  export top N k-mers, default: 64");
    puts("  --period-max N           max period/lag to scan, default: 512");
    puts("  --out-prefix NAME        prefix for CSV names, default: label");
    puts("  --help                   show help");
    puts("");
    puts("Outputs:");
    puts("  <prefix>_top_kmers.csv");
    puts("  <prefix>_summary.csv");
    puts("  <prefix>_periods.csv");
    puts("");
    puts("Semantics:");
    puts("  Motif decomposition for KALYX-ORIGIN candidates: top-kmers, reverse");
    puts("  complement pairing, spacing statistics and lightweight period scan.");
    puts("  This is not an origin proof.");
}

static int ensure_dir(const char *d) {
#ifdef _WIN32
    char cmd[4096]; snprintf(cmd,sizeof(cmd),"if not exist \"%s\" mkdir \"%s\"",d,d); return system(cmd);
#else
    char cmd[4096]; snprintf(cmd,sizeof(cmd),"mkdir -p \"%s\"",d); return system(cmd);
#endif
}

static double shannon_from_table(kmer_table_t *t, double total) {
    if(total <= 0) return 0.0;
    double h=0.0;
    for(size_t i=0;i<t->cap;i++) if(t->items[i].count) {
        double p = (double)t->items[i].count / total;
        h -= p * (log(p)/log(2.0));
    }
    return h;
}

static uint64_t find_count(kmer_table_t *t, uint64_t key) {
    uint64_t h = key * 11400714819323198485ull;
    size_t idx = (size_t)(h & (t->cap - 1));
    while(1) {
        kmer_count_t *e=&t->items[idx];
        if(!e->count) return 0;
        if(e->key == key) return e->count;
        idx = (idx + 1) & (t->cap - 1);
    }
}

static void write_period_scan(const char *path, const unsigned char *reg, uint64_t len, uint32_t period_max) {
    FILE *o=fopen(path,"wb");
    if(!o){fprintf(stderr,"cannot write periods: %s\n",path); exit(4);}
    fprintf(o,"period,valid_pairs,match_rate\n");
    for(uint32_t p=1;p<=period_max;p++) {
        uint64_t valid=0, match=0;
        for(uint64_t i=0;i+p<len;i++) {
            uint64_t a,b;
            if(base_bits(reg[i],&a) && base_bits(reg[i+p],&b)) {
                valid++;
                if(a==b) match++;
            }
        }
        double r = valid ? (double)match/(double)valid : 0.0;
        fprintf(o,"%u,%llu,%.12f\n",p,(unsigned long long)valid,r);
    }
    fclose(o);
}

int main(int argc, char **argv) {
    const char *fasta=NULL,*label="region",*outdir=NULL,*outprefix=NULL;
    uint64_t base_start=0, base_end=0;
    uint32_t k=16, topn=64, period_max=512;
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i],"--fasta")==0 && i+1<argc) fasta=argv[++i];
        else if(strcmp(argv[i],"--label")==0 && i+1<argc) label=argv[++i];
        else if(strcmp(argv[i],"--out-dir")==0 && i+1<argc) outdir=argv[++i];
        else if(strcmp(argv[i],"--out-prefix")==0 && i+1<argc) outprefix=argv[++i];
        else if(strcmp(argv[i],"--base-start")==0 && i+1<argc) { if(!parse_u64(argv[++i],&base_start)){usage();return 2;} }
        else if(strcmp(argv[i],"--base-end")==0 && i+1<argc) { if(!parse_u64(argv[++i],&base_end)){usage();return 2;} }
        else if(strcmp(argv[i],"--k")==0 && i+1<argc) { if(!parse_u32(argv[++i],&k) || k<1 || k>31){usage();return 2;} }
        else if(strcmp(argv[i],"--top")==0 && i+1<argc) { if(!parse_u32(argv[++i],&topn) || topn<1){usage();return 2;} }
        else if(strcmp(argv[i],"--period-max")==0 && i+1<argc) { if(!parse_u32(argv[++i],&period_max)){usage();return 2;} }
        else if(strcmp(argv[i],"--help")==0) { usage(); return 0; }
        else { usage(); return 2; }
    }
    if(!fasta || !outdir || base_end <= base_start) { usage(); return 2; }
    if(!outprefix) outprefix=label;
    ensure_dir(outdir);

    uint64_t fasta_counts[8], fasta_len=0;
    unsigned char *seq = load_fasta_sequence(fasta,&fasta_len,fasta_counts);
    if(!seq) return 3;
    if(base_start >= fasta_len) { fprintf(stderr,"base_start beyond fasta len\n"); free(seq); return 2; }
    if(base_end > fasta_len) base_end=fasta_len;
    uint64_t len = base_end - base_start;
    const unsigned char *reg = seq + base_start;

    kmer_table_t tab; table_init(&tab, (size_t)(len*2 + 1024));
    uint64_t mask = (k==32) ? UINT64_MAX : ((1ull << (2*k)) - 1ull);
    uint64_t rolling=0, run=0, valid_kmers=0;
    uint64_t A=0,C=0,G=0,T=0,N=0,O=0;
    for(uint64_t i=0;i<len;i++) {
        uint64_t b=0;
        int ok = base_bits(reg[i],&b);
        switch(toupper(reg[i])) { case 'A':A++;break;case 'C':C++;break;case 'G':G++;break;case 'T':T++;break;case 'N':N++;break;default:O++;break; }
        if(ok) {
            rolling = ((rolling << 2) | b) & mask;
            run++;
            if(run >= k) {
                uint64_t pos = i + 1 - k;
                table_add(&tab, rolling, pos);
                valid_kmers++;
            }
        } else {
            rolling=0; run=0;
        }
    }

    kmer_count_t *arr=(kmer_count_t*)malloc(tab.used * sizeof(kmer_count_t));
    if(!arr){fprintf(stderr,"oom sort\n");return 3;}
    size_t m=0; for(size_t i=0;i<tab.cap;i++) if(tab.items[i].count) arr[m++]=tab.items[i];
    qsort(arr,m,sizeof(kmer_count_t),cmp_count_desc);

    char top_path[4096], sum_path[4096], per_path[4096];
    snprintf(top_path,sizeof(top_path),"%s/%s_top_kmers.csv",outdir,outprefix);
    snprintf(sum_path,sizeof(sum_path),"%s/%s_summary.csv",outdir,outprefix);
    snprintf(per_path,sizeof(per_path),"%s/%s_periods.csv",outdir,outprefix);

    FILE *top=fopen(top_path,"wb");
    if(!top){fprintf(stderr,"cannot write top\n");return 4;}
    fprintf(top,"version,label,k,rank,kmer,key,count,frequency,revcomp,revcomp_key,revcomp_count,rc_pair_count,first_pos,last_pos,gap_mean,gap_std\n");
    char *s=(char*)malloc(k+1), *rcs=(char*)malloc(k+1);
    uint64_t top_limit = topn < m ? topn : (uint32_t)m;
    double top_mass=0.0;
    for(uint64_t r=0;r<top_limit;r++) {
        kmer_count_t *e=&arr[r];
        uint64_t rc=revcomp_key(e->key,k);
        uint64_t rcc=find_count(&tab,rc);
        key_to_seq(e->key,k,s); key_to_seq(rc,k,rcs);
        double freq = valid_kmers ? (double)e->count/(double)valid_kmers : 0.0;
        double mean=0.0, sd=0.0;
        if(e->gap_count) {
            mean=e->gap_sum/(double)e->gap_count;
            double var=e->gap_sq_sum/(double)e->gap_count - mean*mean;
            if(var<0) var=0; sd=sqrt(var);
        }
        top_mass += freq;
        fprintf(top,"%s,%s,%u,%llu,%s,0x%llx,%llu,%.12f,%s,0x%llx,%llu,%llu,%llu,%llu,%.12f,%.12f\n",
            KALYX_MOTIF_VERSION,label,k,(unsigned long long)(r+1),s,(unsigned long long)e->key,
            (unsigned long long)e->count,freq,rcs,(unsigned long long)rc,(unsigned long long)rcc,
            (unsigned long long)(e->count + rcc),(unsigned long long)e->first_pos,(unsigned long long)e->last_pos,mean,sd);
    }
    fclose(top);

    double entropy = shannon_from_table(&tab,(double)valid_kmers);
    double gc = (A+C+G+T) ? (double)(G+C)/(double)(A+C+G+T) : 0.0;
    double nrate = len ? (double)N/(double)len : 0.0;
    FILE *sum=fopen(sum_path,"wb");
    if(!sum){fprintf(stderr,"cannot write summary\n");return 4;}
    fprintf(sum,"version,label,fasta,base_start_0,base_end_0,base_len,k,valid_kmers,unique_kmers,unique_ratio,entropy_bits,topN,topN_mass,A,C,G,T,N,other,gc_rate,n_rate,region_hash\n");
    fprintf(sum,"%s,%s,%s,%llu,%llu,%llu,%u,%llu,%llu,%.12f,%.12f,%u,%.12f,%llu,%llu,%llu,%llu,%llu,%llu,%.12f,%.12f,0x%llx\n",
        KALYX_MOTIF_VERSION,label,fasta,(unsigned long long)base_start,(unsigned long long)base_end,(unsigned long long)len,k,
        (unsigned long long)valid_kmers,(unsigned long long)m,valid_kmers?(double)m/(double)valid_kmers:0.0,entropy,topn,top_mass,
        (unsigned long long)A,(unsigned long long)C,(unsigned long long)G,(unsigned long long)T,(unsigned long long)N,(unsigned long long)O,gc,nrate,
        (unsigned long long)fnv1a64_buf(reg,(size_t)len));
    fclose(sum);

    write_period_scan(per_path, reg, len, period_max);

    printf("kalyx_motif_context v0.7: label=%s base=[%llu,%llu] len=%llu k=%u unique=%llu entropy=%.9f top%u=%.9f N=%.9f GC=%.9f\n",
        label,(unsigned long long)base_start,(unsigned long long)base_end,(unsigned long long)len,k,(unsigned long long)m,entropy,topn,top_mass,nrate,gc);

    free(s); free(rcs); free(arr); table_free(&tab); free(seq);
    return 0;
}
