/*
  KALYX-ORIGIN v0.9 - Family Signature Scan
  Apache-2.0, Ralf Kruemmel / KALYX

  Scans FASTA windows for exact occurrences of a declared motif-family signature.
  This is a diagnostic, not an origin proof.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define KOSIG_VERSION "KOSIG009"

typedef struct {
    char kmer[64];
    char rc[64];
    double weight;
    uint32_t k;
} motif_t;

typedef struct {
    char *s;
    uint64_t n;
} fasta_t;

static void usage(void) {
    printf("kalyx_signature_scan v0.9 --fasta chr.fa --signature sig.csv --out-csv out.csv [options]\n\n");
    printf("Options:\n");
    printf("  --label-prefix NAME        default: sigwin\n");
    printf("  --base-start N             default: 0\n");
    printf("  --window-bases N           default: 1048576\n");
    printf("  --step-bases N             default: window-bases\n");
    printf("  --windows N                default: 32\n");
    printf("  --period-max N             default: 512\n");
    printf("  --append                   append to output CSV\n");
    printf("  --help                     show help\n\n");
    printf("Signature CSV:\n");
    printf("  header with kmer[,weight] or plain one-kmer-per-line.\n\n");
    printf("Semantics:\n");
    printf("  Counts exact motif and reverse-complement hits per FASTA window, signature density,\n");
    printf("  RC balance and a lightweight hit-position period scan. This is not an origin proof.\n");
}

static int parse_u64(const char *s, uint64_t *out) {
    char *end = NULL; unsigned long long v;
    if (!s || !*s) return 0;
    v = strtoull(s, &end, 0);
    if (!end || *end) return 0;
    *out = (uint64_t)v; return 1;
}
static int parse_u32(const char *s, uint32_t *out) {
    uint64_t v=0; if (!parse_u64(s,&v) || v>0xffffffffu) return 0; *out=(uint32_t)v; return 1;
}

static uint64_t fnv1a64_buf(const unsigned char *p, uint64_t n) {
    uint64_t h = 1469598103934665603ull;
    for (uint64_t i=0;i<n;i++){ h ^= (uint64_t)p[i]; h *= 1099511628211ull; }
    return h;
}

static char comp_base(char c) {
    switch((char)toupper((unsigned char)c)) {
        case 'A': return 'T';
        case 'C': return 'G';
        case 'G': return 'C';
        case 'T': return 'A';
        default: return 'N';
    }
}
static void revcomp(const char *in, char *out) {
    size_t n = strlen(in);
    for (size_t i=0;i<n;i++) out[i] = comp_base(in[n-1-i]);
    out[n]=0;
}
static int valid_kmer(const char *s) {
    for (const char *p=s; *p; ++p) {
        char c=(char)toupper((unsigned char)*p);
        if (!(c=='A'||c=='C'||c=='G'||c=='T')) return 0;
    }
    return 1;
}
static void trim(char *s) {
    size_t n=strlen(s);
    while(n && (s[n-1]=='\r'||s[n-1]=='\n'||isspace((unsigned char)s[n-1]))) s[--n]=0;
    char *p=s; while(*p && isspace((unsigned char)*p)) p++;
    if (p!=s) memmove(s,p,strlen(p)+1);
}
static char *field(char *line, int idx) {
    static char buf[256];
    int cur=0; char *p=line; char *start=p;
    while(1) {
        if (*p==',' || *p==0 || *p=='\n' || *p=='\r') {
            if (cur==idx) {
                size_t n=(size_t)(p-start);
                if (n>=sizeof(buf)) n=sizeof(buf)-1;
                memcpy(buf,start,n); buf[n]=0; trim(buf);
                if (buf[0]=='"' && strlen(buf)>=2 && buf[strlen(buf)-1]=='"') {
                    memmove(buf, buf+1, strlen(buf));
                    buf[strlen(buf)-1]=0;
                }
                return buf;
            }
            if (*p==0 || *p=='\n' || *p=='\r') break;
            cur++; start=p+1;
        }
        p++;
    }
    buf[0]=0; return buf;
}

static fasta_t read_fasta(const char *path) {
    fasta_t f = {0};
    FILE *fp=fopen(path,"rb");
    if(!fp){ fprintf(stderr,"cannot open fasta: %s\n", path); exit(2); }
    uint64_t cap=1024*1024;
    f.s=(char*)malloc((size_t)cap);
    if(!f.s){ fprintf(stderr,"oom\n"); exit(2); }
    char line[8192];
    while(fgets(line,sizeof(line),fp)) {
        if(line[0]=='>') continue;
        for(char *p=line; *p; ++p) {
            char c=(char)toupper((unsigned char)*p);
            if(c=='\r'||c=='\n'||isspace((unsigned char)c)) continue;
            if(f.n+1>=cap){ cap*=2; f.s=(char*)realloc(f.s,(size_t)cap); if(!f.s){fprintf(stderr,"oom\n"); exit(2);} }
            f.s[f.n++]=c;
        }
    }
    fclose(fp);
    return f;
}

static int read_signature(const char *path, motif_t **out, uint32_t *outn) {
    FILE *fp=fopen(path,"rb");
    if(!fp){ fprintf(stderr,"cannot open signature: %s\n", path); return 0; }
    uint32_t cap=128, n=0; motif_t *m=(motif_t*)calloc(cap,sizeof(motif_t));
    char line[4096]; int header_checked=0; int kmer_col=0, weight_col=-1;
    while(fgets(line,sizeof(line),fp)) {
        trim(line);
        if(!line[0] || line[0]=='#') continue;
        char tmp[4096]; strncpy(tmp,line,sizeof(tmp)-1); tmp[sizeof(tmp)-1]=0;
        if(!header_checked) {
            header_checked=1;
            char low[4096]; strncpy(low,line,sizeof(low)-1); low[sizeof(low)-1]=0;
            for(char *p=low; *p; ++p) *p=(char)tolower((unsigned char)*p);
            if(strstr(low,"kmer")) {
                // simple header scan
                int col=0; char *tok=strtok(low,",");
                while(tok) {
                    trim(tok);
                    if(strcmp(tok,"kmer")==0) kmer_col=col;
                    if(strcmp(tok,"weight")==0 || strcmp(tok,"total_count")==0 || strcmp(tok,"count")==0) weight_col=col;
                    tok=strtok(NULL,","); col++;
                }
                continue;
            }
        }
        char line2[4096]; strncpy(line2,line,sizeof(line2)-1); line2[sizeof(line2)-1]=0;
        char *km=field(line2,kmer_col);
        char kmcopy[64]; strncpy(kmcopy,km,sizeof(kmcopy)-1); kmcopy[sizeof(kmcopy)-1]=0;
        for(char *p=kmcopy; *p; ++p) *p=(char)toupper((unsigned char)*p);
        if(!valid_kmer(kmcopy)) continue;
        double w=1.0;
        if(weight_col>=0) {
            char line3[4096]; strncpy(line3,line,sizeof(line3)-1); line3[sizeof(line3)-1]=0;
            char *ws=field(line3,weight_col);
            if(ws && *ws) { double x=strtod(ws,NULL); if(x>0) w=x; }
        }
        if(n>=cap){ cap*=2; m=(motif_t*)realloc(m,cap*sizeof(motif_t)); if(!m){fprintf(stderr,"oom\n"); exit(2);} }
        memset(&m[n],0,sizeof(motif_t));
        strncpy(m[n].kmer,kmcopy,sizeof(m[n].kmer)-1);
        m[n].k=(uint32_t)strlen(kmcopy);
        m[n].weight=w;
        revcomp(m[n].kmer,m[n].rc);
        n++;
    }
    fclose(fp);
    *out=m; *outn=n; return n>0;
}

static int csv_needs_header(const char *path, int append) {
    if(!append) return 1;
    FILE *fp=fopen(path,"rb");
    if(!fp) return 1;
    int c=fgetc(fp); fclose(fp); return c==EOF;
}

static int match_at(const char *s, uint64_t n, uint64_t pos, const char *pat, uint32_t k) {
    if(pos+k>n) return 0;
    for(uint32_t i=0;i<k;i++) if(s[pos+i]!=pat[i]) return 0;
    return 1;
}

int main(int argc, char **argv) {
    const char *fasta_path=NULL, *sig_path=NULL, *out_csv=NULL, *label_prefix="sigwin";
    uint64_t base_start=0, window_bases=1048576, step_bases=0, windows=32;
    uint32_t period_max=512; int append=0;
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i],"--fasta")==0 && i+1<argc) fasta_path=argv[++i];
        else if(strcmp(argv[i],"--signature")==0 && i+1<argc) sig_path=argv[++i];
        else if(strcmp(argv[i],"--out-csv")==0 && i+1<argc) out_csv=argv[++i];
        else if(strcmp(argv[i],"--label-prefix")==0 && i+1<argc) label_prefix=argv[++i];
        else if(strcmp(argv[i],"--base-start")==0 && i+1<argc) { if(!parse_u64(argv[++i],&base_start)){usage();return 2;} }
        else if(strcmp(argv[i],"--window-bases")==0 && i+1<argc) { if(!parse_u64(argv[++i],&window_bases)||window_bases<2){usage();return 2;} }
        else if(strcmp(argv[i],"--step-bases")==0 && i+1<argc) { if(!parse_u64(argv[++i],&step_bases)||step_bases<1){usage();return 2;} }
        else if(strcmp(argv[i],"--windows")==0 && i+1<argc) { if(!parse_u64(argv[++i],&windows)||windows<1){usage();return 2;} }
        else if(strcmp(argv[i],"--period-max")==0 && i+1<argc) { if(!parse_u32(argv[++i],&period_max)||period_max<1){usage();return 2;} }
        else if(strcmp(argv[i],"--append")==0) append=1;
        else if(strcmp(argv[i],"--help")==0) { usage(); return 0; }
        else { usage(); return 2; }
    }
    if(!fasta_path || !sig_path || !out_csv){ usage(); return 2; }
    if(step_bases==0) step_bases=window_bases;

    motif_t *motifs=NULL; uint32_t motif_n=0;
    if(!read_signature(sig_path,&motifs,&motif_n)) return 2;
    fasta_t f=read_fasta(fasta_path);

    FILE *out=fopen(out_csv, append?"ab":"wb");
    if(!out){ fprintf(stderr,"cannot write: %s\n", out_csv); return 2; }
    if(csv_needs_header(out_csv, append)) {
        fprintf(out,"version,label,fasta,signature,motif_count,base_start_0,base_end_0,base_len,A,C,G,T,N,other,gc_rate,n_rate,valid_base_rate,total_hits,total_rc_hits,weighted_hits,signature_density,rc_balance,best_period,best_period_match_rate,region_hash\n");
    }

    for(uint64_t wi=0; wi<windows; ++wi) {
        uint64_t start=base_start + wi*step_bases;
        if(start>=f.n) break;
        uint64_t end=start+window_bases;
        if(end>f.n) end=f.n;
        uint64_t len=end-start;
        if(len<2) break;

        uint64_t A=0,C=0,G=0,T=0,N=0,O=0;
        for(uint64_t i=start;i<end;i++){
            switch(f.s[i]){case 'A':A++;break;case 'C':C++;break;case 'G':G++;break;case 'T':T++;break;case 'N':N++;break;default:O++;break;}
        }
        double gc=(C+G)?((double)(C+G)/(double)(len?len:1)):0.0;
        double nr=(double)N/(double)(len?len:1);
        double valid=(double)(A+C+G+T)/(double)(len?len:1);

        uint64_t total_hits=0,total_rc_hits=0;
        double weighted_hits=0.0;
        // hit positions for period scan
        uint8_t *hitmask=(uint8_t*)calloc((size_t)len,1);
        if(!hitmask){fprintf(stderr,"oom\n"); return 2;}
        for(uint64_t pos=start; pos<end; ++pos) {
            int any=0;
            for(uint32_t mi=0; mi<motif_n; ++mi) {
                uint32_t k=motifs[mi].k;
                if(pos+k>end) continue;
                if(match_at(f.s,f.n,pos,motifs[mi].kmer,k)) {
                    total_hits++; weighted_hits += motifs[mi].weight; any=1;
                }
                if(match_at(f.s,f.n,pos,motifs[mi].rc,k)) {
                    total_rc_hits++; any=1;
                }
            }
            if(any) hitmask[pos-start]=1;
        }
        uint32_t best_p=0; double best_rate=0.0;
        uint32_t pmax=period_max;
        if(pmax>=len) pmax=(uint32_t)(len>1?len-1:1);
        for(uint32_t p=1;p<=pmax;p++){
            uint64_t pairs=0, matches=0;
            for(uint64_t i=0;i+p<len;i++){
                if(hitmask[i]||hitmask[i+p]) { pairs++; if(hitmask[i]&&hitmask[i+p]) matches++; }
            }
            if(pairs>0) {
                double r=(double)matches/(double)pairs;
                if(r>best_rate){ best_rate=r; best_p=p; }
            }
        }
        free(hitmask);
        double density=(double)total_hits/(double)(len?len:1);
        double rcbal=(double)(total_rc_hits+1)/(double)(total_hits+total_rc_hits+1);
        uint64_t h=fnv1a64_buf((const unsigned char*)(f.s+start), len);
        char label[256]; snprintf(label,sizeof(label),"%s_%04llu",label_prefix,(unsigned long long)wi);
        fprintf(out,"%s,%s,%s,%s,%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%.12f,%.12f,%.12f,%llu,%llu,%.12f,%.12f,%.12f,%u,%.12f,0x%016llx\n",
            KOSIG_VERSION,label,fasta_path,sig_path,motif_n,
            (unsigned long long)start,(unsigned long long)end,(unsigned long long)len,
            (unsigned long long)A,(unsigned long long)C,(unsigned long long)G,(unsigned long long)T,(unsigned long long)N,(unsigned long long)O,
            gc,nr,valid,(unsigned long long)total_hits,(unsigned long long)total_rc_hits,weighted_hits,density,rcbal,best_p,best_rate,(unsigned long long)h);
        printf("kalyx_signature_scan v0.9: %s base=[%llu,%llu] len=%llu hits=%llu rc=%llu density=%.9f rc_balance=%.9f period=%u rate=%.6f\n",
            label,(unsigned long long)start,(unsigned long long)end,(unsigned long long)len,
            (unsigned long long)total_hits,(unsigned long long)total_rc_hits,density,rcbal,best_p,best_rate);
    }
    fclose(out);
    free(motifs); free(f.s);
    return 0;
}
