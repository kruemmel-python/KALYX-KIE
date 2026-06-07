
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define KCTX_VERSION "KCTX001"

typedef struct {
    const char* fasta;
    const char* out_csv;
    const char* label;
    uint64_t symbol_skip;
    uint64_t symbol_count;
    uint32_t k;
    int append;
} args_t;

static void usage(void) {
    printf("kalyx_fasta_context v0.5 --fasta chr.fa --symbol-skip N --symbol-count N --out-csv out.csv [options]\n\n");
    printf("Options:\n");
    printf("  --label NAME             row label, default: region\n");
    printf("  --k N                    k-mer length used before KDNA projection, default: 16\n");
    printf("  --append                 append to CSV; header is written if file is missing/empty\n");
    printf("  --help                   show help\n\n");
    printf("Semantics:\n");
    printf("  Maps KDNA/KSTREAM symbol coordinates back to approximate FASTA base coordinates by\n");
    printf("  counting valid A/C/G/T k-mers. N or non-ACGT breaks the k-mer run.\n");
    printf("  This is a coordinate/context diagnostic, not a biological origin proof.\n");
}

static int parse_u64(const char* s, uint64_t* out) {
    if (!s || !*s) return 0;
    char* end = NULL;
    unsigned long long v = strtoull(s, &end, 0);
    if (!end || *end) return 0;
    *out = (uint64_t)v;
    return 1;
}

static int parse_u32(const char* s, uint32_t* out) {
    uint64_t v = 0;
    if (!parse_u64(s, &v) || v > 4096u || v < 1u) return 0;
    *out = (uint32_t)v;
    return 1;
}

static int file_empty_or_missing(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return 1;
    int c = fgetc(f);
    fclose(f);
    return c == EOF;
}

static int is_base_acgt(int c) {
    c = toupper((unsigned char)c);
    return c == 'A' || c == 'C' || c == 'G' || c == 'T';
}

static int is_base_n(int c) {
    c = toupper((unsigned char)c);
    return c == 'N';
}

static int scan_map_symbols_to_bases(const char* fasta, uint32_t k, uint64_t symbol_skip, uint64_t symbol_count,
                                     uint64_t* base_start, uint64_t* base_end,
                                     uint64_t* symbols_seen, uint64_t* observed_symbols) {
    FILE* f = fopen(fasta, "rb");
    if (!f) return 0;

    uint64_t sym_idx = 0;
    uint64_t target_end = symbol_skip + symbol_count;
    uint64_t pos = 0;
    uint64_t run = 0;
    int in_header = 0;
    int have = 0;
    *base_start = 0;
    *base_end = 0;
    *observed_symbols = 0;

    int ch;
    while ((ch = fgetc(f)) != EOF) {
        if (in_header) {
            if (ch == '\n' || ch == '\r') in_header = 0;
            continue;
        }
        if (ch == '>') {
            in_header = 1;
            continue;
        }
        if (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') continue;

        int u = toupper((unsigned char)ch);
        if (is_base_acgt(u)) run++;
        else run = 0;

        if (run >= k) {
            if (sym_idx >= symbol_skip && sym_idx < target_end) {
                uint64_t kmer_start = pos + 1u - (uint64_t)k;
                if (!have) {
                    *base_start = kmer_start;
                    have = 1;
                }
                *base_end = pos;
                (*observed_symbols)++;
            }
            sym_idx++;
            if (sym_idx >= target_end && have) {
                /* We have mapped the full requested symbol range; still enough. */
                break;
            }
        }
        pos++;
    }
    fclose(f);
    *symbols_seen = sym_idx;
    return have;
}

static int scan_base_stats(const char* fasta, uint64_t base_start, uint64_t base_end,
                           uint64_t* a, uint64_t* c, uint64_t* g, uint64_t* t,
                           uint64_t* n, uint64_t* other) {
    FILE* f = fopen(fasta, "rb");
    if (!f) return 0;
    *a = *c = *g = *t = *n = *other = 0;

    uint64_t pos = 0;
    int in_header = 0;
    int ch;
    while ((ch = fgetc(f)) != EOF) {
        if (in_header) {
            if (ch == '\n' || ch == '\r') in_header = 0;
            continue;
        }
        if (ch == '>') { in_header = 1; continue; }
        if (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') continue;

        if (pos >= base_start && pos <= base_end) {
            int u = toupper((unsigned char)ch);
            if (u == 'A') (*a)++;
            else if (u == 'C') (*c)++;
            else if (u == 'G') (*g)++;
            else if (u == 'T') (*t)++;
            else if (u == 'N') (*n)++;
            else (*other)++;
        }
        if (pos > base_end) break;
        pos++;
    }
    fclose(f);
    return 1;
}

int main(int argc, char** argv) {
    args_t args;
    memset(&args, 0, sizeof(args));
    args.label = "region";
    args.k = 16u;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fasta") == 0 && i + 1 < argc) args.fasta = argv[++i];
        else if (strcmp(argv[i], "--out-csv") == 0 && i + 1 < argc) args.out_csv = argv[++i];
        else if (strcmp(argv[i], "--label") == 0 && i + 1 < argc) args.label = argv[++i];
        else if (strcmp(argv[i], "--symbol-skip") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &args.symbol_skip)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--symbol-count") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &args.symbol_count)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--k") == 0 && i + 1 < argc) { if (!parse_u32(argv[++i], &args.k)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--append") == 0) args.append = 1;
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (!args.fasta || !args.out_csv || args.symbol_count == 0 || args.k == 0) {
        usage();
        return 2;
    }

    uint64_t base_start = 0, base_end = 0, symbols_seen = 0, observed_symbols = 0;
    int mapped = scan_map_symbols_to_bases(args.fasta, args.k, args.symbol_skip, args.symbol_count,
                                           &base_start, &base_end, &symbols_seen, &observed_symbols);

    uint64_t a=0, c=0, g=0, t=0, n=0, other=0;
    uint64_t base_len = 0;
    if (mapped) {
        base_len = base_end >= base_start ? (base_end - base_start + 1u) : 0u;
        if (!scan_base_stats(args.fasta, base_start, base_end, &a, &c, &g, &t, &n, &other)) return 3;
    }

    uint64_t acgt = a + c + g + t;
    uint64_t total_bases = acgt + n + other;
    double gc_rate = acgt ? (double)(g + c) / (double)acgt : 0.0;
    double n_rate = total_bases ? (double)n / (double)total_bases : 0.0;
    double valid_rate = total_bases ? (double)acgt / (double)total_bases : 0.0;

    int need_header = !args.append || file_empty_or_missing(args.out_csv);
    FILE* out = fopen(args.out_csv, args.append ? "ab" : "wb");
    if (!out) return 4;
    if (need_header) {
        fprintf(out, "version,label,fasta,k,symbol_skip,symbol_count,base_start_0,base_end_0,base_len,observed_symbols,total_symbols_seen,A,C,G,T,N,other,gc_rate,n_rate,valid_base_rate,status\n");
    }
    fprintf(out, "%s,%s,%s,%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%.12f,%.12f,%.12f,%s\n",
            KCTX_VERSION,
            args.label,
            args.fasta,
            args.k,
            (unsigned long long)args.symbol_skip,
            (unsigned long long)args.symbol_count,
            (unsigned long long)(mapped ? base_start : 0),
            (unsigned long long)(mapped ? base_end : 0),
            (unsigned long long)base_len,
            (unsigned long long)observed_symbols,
            (unsigned long long)symbols_seen,
            (unsigned long long)a, (unsigned long long)c, (unsigned long long)g, (unsigned long long)t,
            (unsigned long long)n, (unsigned long long)other,
            gc_rate, n_rate, valid_rate,
            mapped ? "ok" : "unmapped");
    fclose(out);

    printf("kalyx_fasta_context v0.5: label=%s symbol_skip=%llu symbol_count=%llu base=[%llu,%llu] len=%llu GC=%.6f N=%.6f status=%s\n",
           args.label,
           (unsigned long long)args.symbol_skip,
           (unsigned long long)args.symbol_count,
           (unsigned long long)(mapped ? base_start : 0),
           (unsigned long long)(mapped ? base_end : 0),
           (unsigned long long)base_len,
           gc_rate, n_rate,
           mapped ? "ok" : "unmapped");
    return mapped ? 0 : 5;
}
