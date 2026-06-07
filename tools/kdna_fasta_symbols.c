
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KFSUM_MAGIC "KFSUM001"
#define KFSUM_VERSION 1u
#define KFSUM_HEADER_BYTES 128u
#define KFSUM_FLAG_LE_U64 0x1u
#define KFSUM_FLAG_FASTA 0x2u
#define KFSUM_FLAG_KMER_2BIT 0x4u
#define KFSUM_FLAG_UNLIMITED 0x8u

typedef struct kfsum_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t k;
    uint32_t flags;
    uint64_t bases_total;
    uint64_t bases_valid;
    uint64_t bases_invalid;
    uint64_t symbols_written;
    uint64_t reset_count;
    uint64_t contig_count;
    uint64_t max_symbols;
    uint64_t payload_bytes;
    uint64_t reserved[5];
} kfsum_header;

static void usage(void) {
    fprintf(stderr,
        "kdna_fasta_symbols — streaming FASTA -> 2-bit k-mer uint64 stream\n"
        "emit:\n"
        "  kdna_fasta_symbols --in chr.fa --out chr_k16.u64 --k 16 [--max-symbols 0] [--summary chr.kfsum]\n"
        "inspect:\n"
        "  kdna_fasta_symbols --a chr.kfsum\n"
        "\n"
        "Semantics:\n"
        "  A=00 C=01 G=10 T=11. Non-ACGT symbols reset the rolling k-mer window.\n"
        "  --max-symbols 0 means unlimited / full FASTA stream.\n");
}

static int parse_u64(const char *s, uint64_t *v) {
    char *end = NULL;
    unsigned long long x;
    if (!s || !v) return 0;
    x = strtoull(s, &end, 0);
    if (!end || *end != 0) return 0;
    *v = (uint64_t)x;
    return 1;
}

static int base2(int c) {
    switch (c) {
        case 'A': case 'a': return 0;
        case 'C': case 'c': return 1;
        case 'G': case 'g': return 2;
        case 'T': case 't': return 3;
        default: return -1;
    }
}

static uint64_t mask_for_k(uint32_t k) {
    if (k == 0u) return 0u;
    if (k >= 32u) return UINT64_MAX;
    return (1ULL << (2u * k)) - 1ULL;
}

static int write_summary(const char *path, const kfsum_header *h) {
    FILE *f;
    if (!path || !h) return 0;
    f = fopen(path, "wb");
    if (!f) return 0;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return 0; }
    return fclose(f) == 0;
}

static int read_summary(const char *path, kfsum_header *h) {
    FILE *f;
    if (!path || !h) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return 0; }
    fclose(f);
    return memcmp(h->magic, KFSUM_MAGIC, 8u) == 0 && h->version == KFSUM_VERSION && h->header_bytes == KFSUM_HEADER_BYTES;
}

static int inspect_summary(const char *path) {
    kfsum_header h;
    if (!read_summary(path, &h)) {
        fprintf(stderr, "cannot read KFSUM '%s'\n", path);
        return 2;
    }
    printf("file: KFSUM k:%u bases_total:%" PRIu64 " valid:%" PRIu64 " invalid:%" PRIu64
           " symbols:%" PRIu64 " resets:%" PRIu64 " contigs:%" PRIu64 " max_symbols:%" PRIu64
           " payload:%" PRIu64 " flags:0x%" PRIx32 "\n",
           h.k, h.bases_total, h.bases_valid, h.bases_invalid, h.symbols_written,
           h.reset_count, h.contig_count, h.max_symbols, h.payload_bytes, h.flags);
    return 0;
}

int main(int argc, char **argv) {
    const char *in_path = NULL;
    const char *out_path = NULL;
    const char *summary_path = NULL;
    const char *inspect_path = NULL;
    uint64_t max_symbols = 0u;
    uint32_t k = 16u;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--in") == 0 && i + 1 < argc) in_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--summary") == 0 && i + 1 < argc) summary_path = argv[++i];
        else if (strcmp(argv[i], "--k") == 0 && i + 1 < argc) {
            uint64_t kv = 0u;
            if (!parse_u64(argv[++i], &kv) || kv < 1u || kv > 31u) { usage(); return 2; }
            k = (uint32_t)kv;
        } else if (strcmp(argv[i], "--max-symbols") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &max_symbols)) { usage(); return 2; }
        } else if (strcmp(argv[i], "--a") == 0 && i + 1 < argc) inspect_path = argv[++i];
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (inspect_path) return inspect_summary(inspect_path);
    if (!in_path || !out_path) { usage(); return 2; }

    FILE *in = fopen(in_path, "rb");
    if (!in) { fprintf(stderr, "cannot open input '%s'\n", in_path); return 2; }
    FILE *out = fopen(out_path, "wb");
    if (!out) { fclose(in); fprintf(stderr, "cannot open output '%s'\n", out_path); return 2; }

    const uint64_t mask = mask_for_k(k);
    uint64_t rolling = 0u;
    uint32_t valid_run = 0u;
    int at_line_start = 1;
    int in_header = 0;
    uint64_t bases_total = 0u, bases_valid = 0u, bases_invalid = 0u, symbols = 0u, resets = 0u, contigs = 0u;
    int c;
    int last_was_reset = 1;

    while ((c = fgetc(in)) != EOF) {
        if (at_line_start && c == '>') {
            in_header = 1;
            contigs++;
        }
        if (c == '\n' || c == '\r') {
            at_line_start = 1;
            if (c == '\n') in_header = 0;
            continue;
        }
        if (in_header) {
            at_line_start = 0;
            continue;
        }
        at_line_start = 0;

        int v = base2(c);
        if (v < 0) {
            bases_total++;
            bases_invalid++;
            if (!last_was_reset) resets++;
            last_was_reset = 1;
            rolling = 0u;
            valid_run = 0u;
            continue;
        }

        bases_total++;
        bases_valid++;
        last_was_reset = 0;
        rolling = ((rolling << 2u) | (uint64_t)v) & mask;
        if (valid_run < k) valid_run++;
        if (valid_run >= k) {
            if (fwrite(&rolling, sizeof(uint64_t), 1u, out) != 1u) {
                fclose(in); fclose(out);
                fprintf(stderr, "write failed\n");
                return 2;
            }
            symbols++;
            if (max_symbols != 0u && symbols >= max_symbols) break;
        }
    }

    int ok = (fclose(in) == 0);
    ok = (fclose(out) == 0) && ok;
    if (!ok) { fprintf(stderr, "I/O close failed\n"); return 2; }

    kfsum_header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, KFSUM_MAGIC, 8u);
    h.version = KFSUM_VERSION;
    h.header_bytes = KFSUM_HEADER_BYTES;
    h.k = k;
    h.flags = KFSUM_FLAG_LE_U64 | KFSUM_FLAG_FASTA | KFSUM_FLAG_KMER_2BIT;
    if (max_symbols == 0u) h.flags |= KFSUM_FLAG_UNLIMITED;
    h.bases_total = bases_total;
    h.bases_valid = bases_valid;
    h.bases_invalid = bases_invalid;
    h.symbols_written = symbols;
    h.reset_count = resets;
    h.contig_count = contigs;
    h.max_symbols = max_symbols;
    h.payload_bytes = symbols * (uint64_t)sizeof(uint64_t);

    if (summary_path && !write_summary(summary_path, &h)) {
        fprintf(stderr, "cannot write summary '%s'\n", summary_path);
        return 2;
    }

    printf("kdna_fasta_symbols: in=%s out=%s k=%u symbols=%" PRIu64
           " bases_total=%" PRIu64 " valid=%" PRIu64 " invalid=%" PRIu64
           " resets=%" PRIu64 " contigs=%" PRIu64 " max_symbols=%" PRIu64 "\n",
           in_path, out_path, k, symbols, bases_total, bases_valid, bases_invalid,
           resets, contigs, max_symbols);
    if (summary_path) printf("summary=%s\n", summary_path);
    return 0;
}
