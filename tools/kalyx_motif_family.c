
// kalyx_motif_family.c - KALYX-ORIGIN v0.8 Motif-Family / Consensus Diagnostics
// Apache-2.0, Ralf Kruemmel / KALYX
//
// Purpose:
//   Reads v0.7 top-kmer CSV output and clusters motifs into Hamming-neighborhood
//   families. Exports family consensus, reverse-complement balance,
//   per-family membership and diagnostic edges.
//
// This is an origin diagnostic, not origin proof.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef KALYX_MOTIF_FAMILY_VERSION
#define KALYX_MOTIF_FAMILY_VERSION "KMFAM008"
#endif

typedef struct {
    char version[32];
    char label[128];
    uint32_t k;
    uint32_t rank;
    char *kmer;
    uint64_t count;
    double frequency;
    char *revcomp;
    uint64_t revcomp_count;
    double gap_mean;
    double gap_std;
    uint64_t first_pos;
    uint64_t last_pos;
    uint64_t global_index;
} motif_rec_t;

typedef struct {
    motif_rec_t *v;
    size_t n;
    size_t cap;
} rec_vec_t;

typedef struct {
    uint32_t k;
    int family_id;
    size_t size;
    uint64_t total_count;
    double total_frequency;
    char *consensus;
    double consensus_support;
    double member_entropy_bits;
    double rc_balance;
    double gap_mean_weighted;
    double gap_std_weighted;
    char *top_member;
    uint64_t top_count;
    size_t labels_count;
} family_row_t;

typedef struct {
    family_row_t *v;
    size_t n;
    size_t cap;
} family_vec_t;

static char *xstrdup(const char *s) {
    size_t n = s ? strlen(s) : 0;
    char *p = (char*)malloc(n + 1);
    if (!p) { fprintf(stderr, "oom\n"); exit(10); }
    if (n) memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static void chomp(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) { s[n-1]=0; n--; }
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

static double parse_double_locale(const char *s) {
    if (!s) return 0.0;
    char buf[128];
    size_t n = strlen(s);
    if (n >= sizeof(buf)) n = sizeof(buf)-1;
    for (size_t i=0;i<n;i++) buf[i] = (s[i] == ',') ? '.' : s[i];
    buf[n]=0;
    return atof(buf);
}

static void rec_vec_push(rec_vec_t *a, motif_rec_t r) {
    if (a->n == a->cap) {
        size_t nc = a->cap ? a->cap * 2 : 1024;
        motif_rec_t *nv = (motif_rec_t*)realloc(a->v, nc * sizeof(motif_rec_t));
        if (!nv) { fprintf(stderr,"oom\n"); exit(10); }
        a->v = nv; a->cap = nc;
    }
    a->v[a->n++] = r;
}

static void family_vec_push(family_vec_t *a, family_row_t r) {
    if (a->n == a->cap) {
        size_t nc = a->cap ? a->cap * 2 : 128;
        family_row_t *nv = (family_row_t*)realloc(a->v, nc * sizeof(family_row_t));
        if (!nv) { fprintf(stderr,"oom\n"); exit(10); }
        a->v = nv; a->cap = nc;
    }
    a->v[a->n++] = r;
}

static int split_csv_simple(char *line, char **cols, int max_cols) {
    int n=0;
    char *p=line;
    while (p && n < max_cols) {
        cols[n++] = p;
        char *c = strchr(p, ',');
        if (!c) break;
        *c = 0;
        p = c + 1;
    }
    return n;
}

static uint32_t hamming(const char *a, const char *b) {
    uint32_t d=0;
    if (!a || !b) return 0xffffffffu;
    while (*a && *b) { if (*a != *b) d++; a++; b++; }
    if (*a || *b) return 0xffffffffu;
    return d;
}

static int base_idx(char c) {
    switch(toupper((unsigned char)c)) {
        case 'A': return 0;
        case 'C': return 1;
        case 'G': return 2;
        case 'T': return 3;
        default: return -1;
    }
}

static char base_char(int i) {
    static const char m[4] = {'A','C','G','T'};
    if (i < 0 || i > 3) return 'N';
    return m[i];
}

static int uf_find(int *p, int x) {
    while (p[x] != x) { p[x] = p[p[x]]; x = p[x]; }
    return x;
}
static void uf_union(int *p, int a, int b) {
    int ra = uf_find(p,a), rb = uf_find(p,b);
    if (ra != rb) p[rb] = ra;
}

static int cmp_rec_count_desc(const void *A, const void *B) {
    const motif_rec_t *a = *(const motif_rec_t* const*)A;
    const motif_rec_t *b = *(const motif_rec_t* const*)B;
    if (a->count < b->count) return 1;
    if (a->count > b->count) return -1;
    return strcmp(a->kmer, b->kmer);
}

static int cmp_family_desc(const void *A, const void *B) {
    const family_row_t *a = (const family_row_t*)A;
    const family_row_t *b = (const family_row_t*)B;
    if (a->total_count < b->total_count) return 1;
    if (a->total_count > b->total_count) return -1;
    if (a->k < b->k) return -1;
    if (a->k > b->k) return 1;
    return a->family_id - b->family_id;
}

static int label_seen(motif_rec_t **members, size_t n, size_t idx) {
    for (size_t i=0;i<idx;i++) {
        if (strcmp(members[i]->label, members[idx]->label) == 0) return 1;
    }
    return 0;
}

static family_row_t compute_family(uint32_t k, int family_id, motif_rec_t **members, size_t n) {
    family_row_t f;
    memset(&f, 0, sizeof(f));
    f.k = k;
    f.family_id = family_id;
    f.size = n;
    f.consensus = (char*)malloc((size_t)k + 1);
    if (!f.consensus) { fprintf(stderr,"oom\n"); exit(10); }
    uint64_t **pos_counts = (uint64_t**)calloc(k, sizeof(uint64_t*));
    if (!pos_counts) { fprintf(stderr,"oom\n"); exit(10); }
    for (uint32_t i=0;i<k;i++) {
        pos_counts[i] = (uint64_t*)calloc(4, sizeof(uint64_t));
        if (!pos_counts[i]) { fprintf(stderr,"oom\n"); exit(10); }
    }

    uint64_t max_count = 0;
    const char *top = "";
    double gap_sum = 0.0, gap_std_sum = 0.0;
    uint64_t rc_sum = 0;
    f.labels_count = 0;

    for (size_t m=0;m<n;m++) {
        motif_rec_t *r = members[m];
        f.total_count += r->count;
        f.total_frequency += r->frequency;
        rc_sum += r->revcomp_count;
        gap_sum += r->gap_mean * (double)r->count;
        gap_std_sum += r->gap_std * (double)r->count;
        if (r->count > max_count) { max_count = r->count; top = r->kmer; }
        if (!label_seen(members,n,m)) f.labels_count++;
        for (uint32_t p=0;p<k;p++) {
            int bi = base_idx(r->kmer[p]);
            if (bi >= 0) pos_counts[p][bi] += r->count;
        }
    }

    double support_sum = 0.0;
    for (uint32_t p=0;p<k;p++) {
        uint64_t best=0; int besti=0;
        for (int b=0;b<4;b++) {
            if (pos_counts[p][b] > best) { best = pos_counts[p][b]; besti = b; }
        }
        f.consensus[p] = base_char(besti);
        support_sum += (double)best;
    }
    f.consensus[k] = 0;
    f.consensus_support = (f.total_count && k) ? support_sum / ((double)f.total_count * (double)k) : 0.0;

    double ent = 0.0;
    if (f.total_count) {
        for (size_t m=0;m<n;m++) {
            double p = (double)members[m]->count / (double)f.total_count;
            if (p > 0) ent -= p * (log(p) / log(2.0));
        }
        f.gap_mean_weighted = gap_sum / (double)f.total_count;
        f.gap_std_weighted = gap_std_sum / (double)f.total_count;
        f.rc_balance = (double)rc_sum / (double)f.total_count;
    }
    f.member_entropy_bits = ent;
    f.top_member = xstrdup(top);
    f.top_count = max_count;

    for (uint32_t i=0;i<k;i++) free(pos_counts[i]);
    free(pos_counts);
    return f;
}

static void usage(void) {
    printf(
        "kalyx_motif_family v0.8 --top-kmers file.csv --out-dir DIR [options]\n"
        "\n"
        "Options:\n"
        "  --k N                    restrict to one k; default: 0 = all k values\n"
        "  --hamming N              maximum Hamming distance for family edges, default: 2\n"
        "  --min-count N            ignore motif rows with count below N, default: 1\n"
        "  --max-rows N             safety cap, default: 20000\n"
        "  --rc-link 0|1            also link motif to reverse-complement neighbors, default: 0\n"
        "  --label NAME             run label for log/report context, default: motif_family\n"
        "  --help                   show help\n"
        "\n"
        "Inputs:\n"
        "  v0.7 combined top-kmer CSV: version,label,k,rank,kmer,...\n"
        "\n"
        "Outputs:\n"
        "  kalyx_origin_v0_8_families.csv\n"
        "  kalyx_origin_v0_8_members.csv\n"
        "  kalyx_origin_v0_8_edges.csv\n"
        "\n"
        "Semantics:\n"
        "  Motif-family clustering for ORIGIN candidates. This is not origin proof.\n"
    );
}

int main(int argc, char **argv) {
    const char *top_path = NULL;
    const char *out_dir = NULL;
    const char *label = "motif_family";
    uint32_t k_filter = 0, ham = 2, rc_link = 0;
    uint64_t min_count = 1, max_rows = 20000;

    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i],"--top-kmers")==0 && i+1<argc) top_path = argv[++i];
        else if (strcmp(argv[i],"--out-dir")==0 && i+1<argc) out_dir = argv[++i];
        else if (strcmp(argv[i],"--label")==0 && i+1<argc) label = argv[++i];
        else if (strcmp(argv[i],"--k")==0 && i+1<argc) { if(!parse_u32(argv[++i],&k_filter)) { usage(); return 2; } }
        else if (strcmp(argv[i],"--hamming")==0 && i+1<argc) { if(!parse_u32(argv[++i],&ham)) { usage(); return 2; } }
        else if (strcmp(argv[i],"--min-count")==0 && i+1<argc) { if(!parse_u64(argv[++i],&min_count)) { usage(); return 2; } }
        else if (strcmp(argv[i],"--max-rows")==0 && i+1<argc) { if(!parse_u64(argv[++i],&max_rows)) { usage(); return 2; } }
        else if (strcmp(argv[i],"--rc-link")==0 && i+1<argc) { if(!parse_u32(argv[++i],&rc_link) || rc_link>1) { usage(); return 2; } }
        else if (strcmp(argv[i],"--help")==0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (!top_path || !out_dir) { usage(); return 2; }

    FILE *in = fopen(top_path, "rb");
    if (!in) { fprintf(stderr, "cannot read top-kmers CSV: %s\n", top_path); return 3; }

    rec_vec_t recs = {0};
    char line[8192];
    uint64_t line_no = 0;
    while (fgets(line, sizeof(line), in)) {
        line_no++;
        chomp(line);
        if (!line[0]) continue;
        if (strncmp(line, "version,", 8) == 0 || strncmp(line, "\"version\"", 9) == 0) continue;

        char *cols[32] = {0};
        int n = split_csv_simple(line, cols, 32);
        if (n < 16) continue;

        motif_rec_t r;
        memset(&r, 0, sizeof(r));
        snprintf(r.version, sizeof(r.version), "%s", cols[0]);
        snprintf(r.label, sizeof(r.label), "%s", cols[1]);
        if (!parse_u32(cols[2], &r.k)) continue;
        if (k_filter && r.k != k_filter) continue;
        if (!parse_u32(cols[3], &r.rank)) r.rank = 0;
        r.kmer = xstrdup(cols[4]);
        if (!parse_u64(cols[6], &r.count)) r.count = 0;
        if (r.count < min_count) { free(r.kmer); continue; }
        r.frequency = parse_double_locale(cols[7]);
        r.revcomp = xstrdup(cols[8]);
        if (!parse_u64(cols[10], &r.revcomp_count)) r.revcomp_count = 0;
        if (!parse_u64(cols[12], &r.first_pos)) r.first_pos = 0;
        if (!parse_u64(cols[13], &r.last_pos)) r.last_pos = 0;
        r.gap_mean = parse_double_locale(cols[14]);
        r.gap_std = parse_double_locale(cols[15]);
        r.global_index = recs.n;

        if (strlen(r.kmer) != r.k) { free(r.kmer); free(r.revcomp); continue; }
        rec_vec_push(&recs, r);
        if (recs.n >= max_rows) break;
    }
    fclose(in);

#ifdef _WIN32
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "if not exist \"%s\" mkdir \"%s\"", out_dir, out_dir);
    system(cmd);
#else
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", out_dir);
    system(cmd);
#endif

    char path_fam[4096], path_mem[4096], path_edge[4096];
    snprintf(path_fam, sizeof(path_fam), "%s/kalyx_origin_v0_8_families.csv", out_dir);
    snprintf(path_mem, sizeof(path_mem), "%s/kalyx_origin_v0_8_members.csv", out_dir);
    snprintf(path_edge, sizeof(path_edge), "%s/kalyx_origin_v0_8_edges.csv", out_dir);

    FILE *fam = fopen(path_fam, "wb");
    FILE *mem = fopen(path_mem, "wb");
    FILE *edg = fopen(path_edge, "wb");
    if (!fam || !mem || !edg) { fprintf(stderr, "cannot write output CSVs in: %s\n", out_dir); return 4; }

    fprintf(fam, "version,run_label,k,family_id,size,total_count,total_frequency,consensus,consensus_support,member_entropy_bits,rc_balance,gap_mean_weighted,gap_std_weighted,top_member,top_count,labels_count\n");
    fprintf(mem, "version,run_label,k,family_id,member_rank,label,kmer,count,frequency,revcomp,revcomp_count,dist_to_consensus,first_pos,last_pos,gap_mean,gap_std\n");
    fprintf(edg, "version,run_label,k,source_kmer,target_kmer,source_label,target_label,hamming,source_count,target_count,edge_kind\n");

    family_vec_t families = {0};

    for (uint32_t k=1;k<=31;k++) {
        if (k_filter && k != k_filter) continue;

        size_t n=0;
        for (size_t i=0;i<recs.n;i++) if (recs.v[i].k == k) n++;
        if (!n) continue;

        motif_rec_t **arr = (motif_rec_t**)malloc(n * sizeof(motif_rec_t*));
        int *uf = (int*)malloc(n * sizeof(int));
        if (!arr || !uf) { fprintf(stderr,"oom\n"); return 10; }

        size_t idx=0;
        for (size_t i=0;i<recs.n;i++) if (recs.v[i].k == k) { arr[idx] = &recs.v[i]; uf[idx]=(int)idx; idx++; }

        for (size_t i=0;i<n;i++) {
            for (size_t j=i+1;j<n;j++) {
                uint32_t d = hamming(arr[i]->kmer, arr[j]->kmer);
                if (d <= ham) {
                    uf_union(uf, (int)i, (int)j);
                    fprintf(edg, "%s,%s,%u,%s,%s,%s,%s,%u,%llu,%llu,hamming\n",
                        KALYX_MOTIF_FAMILY_VERSION,label,k,arr[i]->kmer,arr[j]->kmer,arr[i]->label,arr[j]->label,d,
                        (unsigned long long)arr[i]->count,(unsigned long long)arr[j]->count);
                } else if (rc_link) {
                    uint32_t drc = hamming(arr[i]->revcomp, arr[j]->kmer);
                    if (drc <= ham) {
                        uf_union(uf, (int)i, (int)j);
                        fprintf(edg, "%s,%s,%u,%s,%s,%s,%s,%u,%llu,%llu,rc_hamming\n",
                            KALYX_MOTIF_FAMILY_VERSION,label,k,arr[i]->revcomp,arr[j]->kmer,arr[i]->label,arr[j]->label,drc,
                            (unsigned long long)arr[i]->count,(unsigned long long)arr[j]->count);
                    }
                }
            }
        }

        int fam_id = 0;
        int *root_to_fam = (int*)malloc(n * sizeof(int));
        if (!root_to_fam) { fprintf(stderr,"oom\n"); return 10; }
        for (size_t i=0;i<n;i++) root_to_fam[i] = -1;

        for (size_t i=0;i<n;i++) {
            int r = uf_find(uf, (int)i);
            if (root_to_fam[r] < 0) root_to_fam[r] = fam_id++;
        }

        for (int f=0; f<fam_id; f++) {
            size_t m=0;
            for (size_t i=0;i<n;i++) if (root_to_fam[uf_find(uf,(int)i)] == f) m++;
            motif_rec_t **members = (motif_rec_t**)malloc(m * sizeof(motif_rec_t*));
            if (!members) { fprintf(stderr,"oom\n"); return 10; }
            size_t mi=0;
            for (size_t i=0;i<n;i++) if (root_to_fam[uf_find(uf,(int)i)] == f) members[mi++] = arr[i];

            qsort(members, m, sizeof(motif_rec_t*), cmp_rec_count_desc);
            family_row_t fr = compute_family(k, f, members, m);
            family_vec_push(&families, fr);

            for (size_t rnk=0;rnk<m;rnk++) {
                uint32_t dc = hamming(members[rnk]->kmer, fr.consensus);
                fprintf(mem, "%s,%s,%u,%d,%llu,%s,%s,%llu,%.12f,%s,%llu,%u,%llu,%llu,%.12f,%.12f\n",
                    KALYX_MOTIF_FAMILY_VERSION,label,k,f,(unsigned long long)(rnk+1),members[rnk]->label,members[rnk]->kmer,
                    (unsigned long long)members[rnk]->count,members[rnk]->frequency,members[rnk]->revcomp,
                    (unsigned long long)members[rnk]->revcomp_count,dc,
                    (unsigned long long)members[rnk]->first_pos,(unsigned long long)members[rnk]->last_pos,
                    members[rnk]->gap_mean,members[rnk]->gap_std);
            }
            free(members);
        }

        free(root_to_fam);
        free(uf);
        free(arr);
    }

    qsort(families.v, families.n, sizeof(family_row_t), cmp_family_desc);
    for (size_t i=0;i<families.n;i++) {
        family_row_t *f = &families.v[i];
        fprintf(fam, "%s,%s,%u,%d,%llu,%llu,%.12f,%s,%.12f,%.12f,%.12f,%.12f,%.12f,%s,%llu,%llu\n",
            KALYX_MOTIF_FAMILY_VERSION,label,f->k,f->family_id,(unsigned long long)f->size,
            (unsigned long long)f->total_count,f->total_frequency,f->consensus,f->consensus_support,
            f->member_entropy_bits,f->rc_balance,f->gap_mean_weighted,f->gap_std_weighted,
            f->top_member,(unsigned long long)f->top_count,(unsigned long long)f->labels_count);
    }

    fclose(fam); fclose(mem); fclose(edg);

    printf("kalyx_motif_family v0.8: rows=%llu families=%llu hamming=%u rc_link=%u out=%s\n",
        (unsigned long long)recs.n, (unsigned long long)families.n, ham, rc_link, out_dir);

    for (size_t i=0;i<recs.n;i++) { free(recs.v[i].kmer); free(recs.v[i].revcomp); }
    free(recs.v);
    for (size_t i=0;i<families.n;i++) { free(families.v[i].consensus); free(families.v[i].top_member); }
    free(families.v);
    return 0;
}
