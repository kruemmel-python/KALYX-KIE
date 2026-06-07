
// KALYX-ORIGIN v0.6 Raw FASTA k-mer / repeat context diagnostics
// Apache-2.0, Ralf Krümmel / KALYX
//
// Purpose:
//   Map already identified ORIGIN FASTA base windows to raw sequence diagnostics:
//   A/C/G/T/N, GC, valid rate, rolling k-mer uniqueness/concentration,
//   homopolymer and dinucleotide tandem runs.
// This is a context diagnostic, not an origin proof.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#define KREP_VERSION "KREPEAT001"

typedef struct {
    uint64_t key;
    uint64_t count;
    uint8_t used;
} entry_t;

static void usage(void) {
    printf("kalyx_repeat_context v0.6 --fasta chr.fa --base-start N --base-end N --out-csv out.csv [options]\n\n");
    printf("Options:\n");
    printf("  --label NAME             row label, default: region\n");
    printf("  --k N                    k-mer length, 1..31, default: 16\n");
    printf("  --append                 append to CSV; header written if file missing/empty\n");
    printf("  --help                   show help\n\n");
    printf("Semantics:\n");
    printf("  Computes raw FASTA-level context for ORIGIN windows: base composition,\n");
    printf("  N/GC, k-mer uniqueness/top concentration, k-mer entropy, longest\n");
    printf("  homopolymer and dinucleotide tandem runs. This is not an origin proof.\n");
}

static int parse_u64(const char *s, uint64_t *out) {
    if (!s || !*s) return 0;
    errno = 0;
    char *end = NULL;
    #if defined(_WIN32)
    uint64_t v = _strtoui64(s, &end, 10);
#else
    uint64_t v = strtoull(s, &end, 10);
#endif
    if (errno || !end || *end) return 0;
    *out = v;
    return 1;
}

static int parse_u32(const char *s, uint32_t *out) {
    uint64_t v = 0;
    if (!parse_u64(s, &v) || v > 0xffffffffu) return 0;
    *out = (uint32_t)v;
    return 1;
}

static int is_acgt(char c) {
    c = (char)toupper((unsigned char)c);
    return c == 'A' || c == 'C' || c == 'G' || c == 'T';
}

static int enc2(char c, uint64_t *v) {
    c = (char)toupper((unsigned char)c);
    if (c == 'A') { *v = 0; return 1; }
    if (c == 'C') { *v = 1; return 1; }
    if (c == 'G') { *v = 2; return 1; }
    if (c == 'T') { *v = 3; return 1; }
    return 0;
}

static uint64_t next_pow2(uint64_t x) {
    uint64_t p = 1;
    while (p < x && p < (1ull<<62)) p <<= 1;
    return p;
}

static uint64_t mix64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

static int table_add(entry_t *tab, uint64_t cap, uint64_t key, uint64_t *unique) {
    uint64_t i = mix64(key) & (cap - 1);
    for (uint64_t step = 0; step < cap; ++step) {
        if (!tab[i].used) {
            tab[i].used = 1;
            tab[i].key = key;
            tab[i].count = 1;
            (*unique)++;
            return 1;
        }
        if (tab[i].key == key) {
            tab[i].count++;
            return 1;
        }
        i = (i + 1) & (cap - 1);
    }
    return 0;
}

static int cmp_u64_desc(const void *a, const void *b) {
    uint64_t aa = *(const uint64_t*)a;
    uint64_t bb = *(const uint64_t*)b;
    return (aa < bb) - (aa > bb);
}

static int file_exists_nonempty(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    fclose(f);
    return sz > 0;
}

int main(int argc, char **argv) {
    const char *fasta = NULL;
    const char *out_csv = NULL;
    const char *label = "region";
    uint64_t base_start = 0, base_end = 0;
    uint32_t k = 16;
    int append = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--fasta") == 0 && i + 1 < argc) fasta = argv[++i];
        else if (strcmp(argv[i], "--out-csv") == 0 && i + 1 < argc) out_csv = argv[++i];
        else if (strcmp(argv[i], "--label") == 0 && i + 1 < argc) label = argv[++i];
        else if (strcmp(argv[i], "--base-start") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &base_start)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--base-end") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &base_end)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--k") == 0 && i + 1 < argc) { if (!parse_u32(argv[++i], &k) || k < 1 || k > 31) { usage(); return 2; } }
        else if (strcmp(argv[i], "--append") == 0) append = 1;
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (!fasta || !out_csv || base_end <= base_start) { usage(); return 2; }

    uint64_t region_len = base_end - base_start + 1;
    char *seq = (char*)malloc((size_t)region_len);
    if (!seq) { fprintf(stderr, "out of memory seq len=%llu\n", (unsigned long long)region_len); return 3; }

    FILE *f = fopen(fasta, "rb");
    if (!f) { fprintf(stderr, "cannot open fasta %s\n", fasta); free(seq); return 4; }

    uint64_t pos = 0, got = 0;
    int c;
    int in_header = 0;
    while ((c = fgetc(f)) != EOF) {
        if (c == '>') { in_header = 1; continue; }
        if (in_header) {
            if (c == '\n' || c == '\r') in_header = 0;
            continue;
        }
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') continue;
        if (pos >= base_start && pos <= base_end) {
            if (got < region_len) seq[got++] = (char)toupper((unsigned char)c);
        }
        pos++;
        if (pos > base_end) break;
    }
    fclose(f);

    if (got != region_len) {
        fprintf(stderr, "FASTA region short: requested=%llu got=%llu\n",
                (unsigned long long)region_len, (unsigned long long)got);
        free(seq);
        return 5;
    }

    uint64_t A=0,C=0,G=0,T=0,N=0,O=0;
    uint64_t longest_homopolymer = 0, cur_homo = 0;
    char last = 0;
    for (uint64_t i=0;i<got;i++) {
        char b = seq[i];
        if (b == 'A') A++;
        else if (b == 'C') C++;
        else if (b == 'G') G++;
        else if (b == 'T') T++;
        else if (b == 'N') N++;
        else O++;

        if (i == 0 || b != last) cur_homo = 1;
        else cur_homo++;
        if (cur_homo > longest_homopolymer) longest_homopolymer = cur_homo;
        last = b;
    }
    uint64_t valid = A+C+G+T;
    double gc_rate = valid ? ((double)(G+C) / (double)valid) : 0.0;
    double n_rate = got ? ((double)N / (double)got) : 0.0;
    double valid_rate = got ? ((double)valid / (double)got) : 0.0;

    // Dinucleotide tandem maximum: longest consecutive repetition of same 2-mer.
    uint64_t longest_dinuc_tandem_bases = 0;
    if (got >= 4) {
        uint64_t run_units = 1;
        for (uint64_t i=2; i+1<got; i+=2) {
            if (is_acgt(seq[i-2]) && is_acgt(seq[i-1]) &&
                seq[i] == seq[i-2] && seq[i+1] == seq[i-1]) {
                run_units++;
            } else {
                if (run_units * 2 > longest_dinuc_tandem_bases) longest_dinuc_tandem_bases = run_units * 2;
                run_units = 1;
            }
        }
        if (run_units * 2 > longest_dinuc_tandem_bases) longest_dinuc_tandem_bases = run_units * 2;
    }

    uint64_t max_possible_kmers = (got >= k) ? (got - k + 1) : 0;
    uint64_t cap = next_pow2(max_possible_kmers * 4 + 1024);
    entry_t *tab = (entry_t*)calloc((size_t)cap, sizeof(entry_t));
    if (!tab) { fprintf(stderr, "out of memory table cap=%llu\n", (unsigned long long)cap); free(seq); return 6; }

    uint64_t mask = (k == 32) ? UINT64_MAX : ((1ULL << (2*k)) - 1ULL);
    uint64_t rolling = 0, run = 0, valid_kmers = 0, unique_kmers = 0;
    for (uint64_t i=0;i<got;i++) {
        uint64_t v=0;
        if (!enc2(seq[i], &v)) {
            rolling = 0;
            run = 0;
            continue;
        }
        rolling = ((rolling << 2) | v) & mask;
        run++;
        if (run >= k) {
            valid_kmers++;
            if (!table_add(tab, cap, rolling, &unique_kmers)) {
                fprintf(stderr, "hash table full\n");
                free(tab); free(seq); return 7;
            }
        }
    }

    uint64_t *counts = NULL;
    if (unique_kmers) counts = (uint64_t*)malloc((size_t)unique_kmers * sizeof(uint64_t));
    if (unique_kmers && !counts) { fprintf(stderr, "out of memory counts\n"); free(tab); free(seq); return 8; }
    uint64_t j=0;
    double entropy = 0.0;
    for (uint64_t i=0;i<cap;i++) {
        if (tab[i].used) {
            counts[j++] = tab[i].count;
            double p = (double)tab[i].count / (double)valid_kmers;
            entropy -= p * (log(p) / log(2.0));
        }
    }
    if (counts) qsort(counts, (size_t)unique_kmers, sizeof(uint64_t), cmp_u64_desc);
    uint64_t top1=0, top8=0, top32=0;
    for (uint64_t i=0;i<unique_kmers;i++) {
        if (i < 1) top1 += counts[i];
        if (i < 8) top8 += counts[i];
        if (i < 32) top32 += counts[i];
    }

    double top1_mass = valid_kmers ? (double)top1 / (double)valid_kmers : 0.0;
    double top8_mass = valid_kmers ? (double)top8 / (double)valid_kmers : 0.0;
    double top32_mass = valid_kmers ? (double)top32 / (double)valid_kmers : 0.0;
    double unique_ratio = valid_kmers ? (double)unique_kmers / (double)valid_kmers : 0.0;

    int write_header = !append || !file_exists_nonempty(out_csv);
    FILE *out = fopen(out_csv, append ? "ab" : "wb");
    if (!out) { fprintf(stderr, "cannot write %s\n", out_csv); free(counts); free(tab); free(seq); return 9; }
    if (write_header) {
        fprintf(out, "version,label,fasta,k,base_start_0,base_end_0,base_len,A,C,G,T,N,other,gc_rate,n_rate,valid_base_rate,valid_kmers,unique_kmers,unique_kmer_ratio,kmer_entropy_bits,kmer_top1_mass,kmer_top8_mass,kmer_top32_mass,longest_homopolymer,longest_dinuc_tandem_bases,status\n");
    }
    fprintf(out,
        "%s,%s,%s,%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%.12f,%.12f,%.12f,%llu,%llu,%.12f,%.12f,%.12f,%.12f,%.12f,%llu,%llu,ok\n",
        KREP_VERSION, label, fasta, k,
        (unsigned long long)base_start, (unsigned long long)base_end, (unsigned long long)got,
        (unsigned long long)A, (unsigned long long)C, (unsigned long long)G, (unsigned long long)T,
        (unsigned long long)N, (unsigned long long)O,
        gc_rate, n_rate, valid_rate,
        (unsigned long long)valid_kmers, (unsigned long long)unique_kmers,
        unique_ratio, entropy, top1_mass, top8_mass, top32_mass,
        (unsigned long long)longest_homopolymer,
        (unsigned long long)longest_dinuc_tandem_bases);
    fclose(out);

    printf("kalyx_repeat_context v0.6: label=%s base=[%llu,%llu] len=%llu k=%u unique_kmers=%llu entropy=%.9f top32=%.9f N=%.9f GC=%.9f homo=%llu dinuc=%llu\n",
        label,
        (unsigned long long)base_start, (unsigned long long)base_end, (unsigned long long)got,
        k, (unsigned long long)unique_kmers, entropy, top32_mass, n_rate, gc_rate,
        (unsigned long long)longest_homopolymer, (unsigned long long)longest_dinuc_tandem_bases);

    free(counts);
    free(tab);
    free(seq);
    return 0;
}
