
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kdna_kstream.h"

#define FNV64_OFFSET 1469598103934665603ULL
#define FNV64_PRIME 1099511628211ULL

static uint64_t fnv1a_update(uint64_t h, const void *data, size_t n) {
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint64_t)p[i];
        h *= FNV64_PRIME;
    }
    return h;
}

static int parse_u64(const char *s, uint64_t *out) {
    char *end = NULL;
    unsigned long long v;
    if (!s || !out) return 0;
    errno = 0;
    v = strtoull(s, &end, 0);
    if (errno || !end || *end != 0) return 0;
    *out = (uint64_t)v;
    return 1;
}

static int parse_u32(const char *s, uint32_t *out) {
    uint64_t v = 0;
    if (!parse_u64(s, &v) || v > UINT32_MAX) return 0;
    *out = (uint32_t)v;
    return 1;
}

static int parse_double_arg(const char *s, double *out) {
    char *end = NULL;
    double v;
    if (!s || !out) return 0;
    errno = 0;
    v = strtod(s, &end);
    if (errno || !end || *end != 0 || !isfinite(v)) return 0;
    *out = v;
    return 1;
}

static void base_name(char *dst, size_t dst_n, const char *src) {
    const char *p = src;
    const char *last = src;
    if (!dst || dst_n == 0) return;
    if (!src) { dst[0] = 0; return; }
    while (*p) {
        if (*p == '/' || *p == '\\') last = p + 1;
        ++p;
    }
    snprintf(dst, dst_n, "%s", last);
}

static int write_u64(FILE *out, uint64_t v, uint64_t *hash) {
    if (fwrite(&v, sizeof(v), 1u, out) != 1u) return 0;
    if (hash) *hash = fnv1a_update(*hash, &v, sizeof(v));
    return 1;
}

static int write_summary_file(const char *path, kdna_kstream_header *h) {
    FILE *f;
    if (!path || !h) return 0;
    f = fopen(path, "wb");
    if (!f) return 0;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) {
        fclose(f);
        return 0;
    }
    return fclose(f) == 0;
}

static int read_summary_file(const char *path, kdna_kstream_header *h) {
    FILE *f;
    if (!path || !h) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return memcmp(h->magic, KDNA_KSTREAM_MAGIC, 8u) == 0 &&
           h->version == KDNA_KSTREAM_VERSION &&
           h->header_bytes == KDNA_KSTREAM_HEADER_BYTES;
}

static int inspect_summary(const char *path) {
    kdna_kstream_header h;
    if (!read_summary_file(path, &h)) {
        fprintf(stderr, "kdna_kstream: cannot read KSTREAM summary '%s'\n", path ? path : "(null)");
        return 2;
    }
    printf("file: KSTREAM kind:%u flags:0x%08x symbol_bits:%u window:%u stride:%u bins:%u\n",
           h.kind, h.flags, h.symbol_bits, h.window, h.stride, h.bins);
    printf("source:%s input_bytes:%" PRIu64 " records:%" PRIu64 " symbols:%" PRIu64
           " skipped:%" PRIu64 " invalid:%" PRIu64 " max_symbols:%" PRIu64 " payload:%" PRIu64 "\n",
           h.source_name, h.input_bytes, h.input_records, h.symbols_written,
           h.skipped_records, h.invalid_records, h.max_symbols, h.payload_bytes);
    printf("min:%0.17g max:%0.17g scale:%0.17g offset:%0.17g input_hash:0x%016" PRIx64 " symbol_hash:0x%016" PRIx64 "\n",
           h.min_value, h.max_value, h.scale, h.offset, h.input_hash, h.symbol_hash);
    return 0;
}

static void usage(void) {
    fprintf(stderr,
        "kdna_stream_csv — numeric CSV column -> quantized uint64 KSTREAM\n"
        "usage:\n"
        "  kdna_stream_csv --in data.csv --out data.u64 --summary data.kstream --column 0 --bins 256 [--delimiter ,] [--header 1] [--min X --max Y] [--max-symbols 0]\n"
        "  kdna_stream_csv --a data.kstream\n");
}

static int get_field(char *line, char delim, uint32_t column, char **field_out) {
    uint32_t col = 0u;
    char *p = line;
    char *start = p;
    if (!line || !field_out) return 0;
    while (1) {
        if (*p == delim || *p == '\n' || *p == '\r' || *p == 0) {
            char end = *p;
            *p = 0;
            if (col == column) {
                *field_out = start;
                return 1;
            }
            if (end == 0 || end == '\n' || end == '\r') return 0;
            start = p + 1;
            col++;
        }
        p++;
    }
}

static int scan_range(const char *path, uint32_t column, char delim, int header, double *minv, double *maxv, uint64_t *valid, uint64_t *invalid, uint64_t *bytes_hash, uint64_t *input_bytes) {
    FILE *f = fopen(path, "rb");
    char line[65536];
    uint64_t line_no = 0, vcount = 0, icount = 0, ih = FNV64_OFFSET, ib = 0;
    double mn = DBL_MAX, mx = -DBL_MAX;
    if (!f) return 0;
    while (fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        ih = fnv1a_update(ih, line, n);
        ib += (uint64_t)n;
        line_no++;
        if (header && line_no == 1) continue;
        char *field = NULL;
        if (!get_field(line, delim, column, &field)) { icount++; continue; }
        double x;
        if (!parse_double_arg(field, &x)) { icount++; continue; }
        if (x < mn) mn = x;
        if (x > mx) mx = x;
        vcount++;
    }
    fclose(f);
    if (vcount == 0) return 0;
    *minv = mn;
    *maxv = mx;
    *valid = vcount;
    *invalid = icount;
    *bytes_hash = ih;
    *input_bytes = ib;
    return 1;
}

int main(int argc, char **argv) {
    const char *in_path = NULL, *out_path = NULL, *summary_path = NULL, *inspect_path = NULL;
    uint32_t column = 0u, bins = 256u;
    char delim = ',';
    int header = 1;
    int have_min = 0, have_max = 0;
    double minv = 0.0, maxv = 0.0;
    uint64_t max_symbols = 0u;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--in") == 0 && i + 1 < argc) in_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--summary") == 0 && i + 1 < argc) summary_path = argv[++i];
        else if (strcmp(argv[i], "--column") == 0 && i + 1 < argc) { if (!parse_u32(argv[++i], &column)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--bins") == 0 && i + 1 < argc) { if (!parse_u32(argv[++i], &bins) || bins < 2u) { usage(); return 2; } }
        else if (strcmp(argv[i], "--delimiter") == 0 && i + 1 < argc) { const char *s = argv[++i]; delim = (strcmp(s, "tab") == 0) ? '\t' : s[0]; }
        else if (strcmp(argv[i], "--header") == 0 && i + 1 < argc) { uint64_t v=0; if (!parse_u64(argv[++i], &v)) { usage(); return 2; } header = (v != 0); }
        else if (strcmp(argv[i], "--min") == 0 && i + 1 < argc) { if (!parse_double_arg(argv[++i], &minv)) { usage(); return 2; } have_min = 1; }
        else if (strcmp(argv[i], "--max") == 0 && i + 1 < argc) { if (!parse_double_arg(argv[++i], &maxv)) { usage(); return 2; } have_max = 1; }
        else if (strcmp(argv[i], "--max-symbols") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &max_symbols)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--a") == 0 && i + 1 < argc) inspect_path = argv[++i];
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }
    if (inspect_path) return inspect_summary(inspect_path);
    if (!in_path || !out_path || !summary_path) { usage(); return 2; }

    uint64_t pre_valid = 0, pre_invalid = 0, input_hash = FNV64_OFFSET, input_bytes = 0;
    int auto_range = !(have_min && have_max);
    if (auto_range) {
        if (!scan_range(in_path, column, delim, header, &minv, &maxv, &pre_valid, &pre_invalid, &input_hash, &input_bytes)) {
            fprintf(stderr, "cannot scan CSV range or no valid numeric data\n");
            return 2;
        }
    }
    if (!(maxv > minv)) { fprintf(stderr, "invalid range min=%g max=%g\n", minv, maxv); return 2; }

    FILE *in = fopen(in_path, "rb");
    if (!in) { fprintf(stderr, "cannot open input '%s'\n", in_path); return 2; }
    FILE *out = fopen(out_path, "wb");
    if (!out) { fclose(in); fprintf(stderr, "cannot open output '%s'\n", out_path); return 2; }

    char line[65536];
    uint64_t line_no = 0, valid = 0, invalid = 0, skipped = 0, symbols = 0, symbol_hash = FNV64_OFFSET;
    if (!auto_range) { input_hash = FNV64_OFFSET; input_bytes = 0; }
    const double scale = ((double)(bins - 1u)) / (maxv - minv);

    while (fgets(line, sizeof(line), in)) {
        size_t n = strlen(line);
        if (!auto_range) { input_hash = fnv1a_update(input_hash, line, n); input_bytes += (uint64_t)n; }
        line_no++;
        if (header && line_no == 1) { skipped++; continue; }
        char *field = NULL;
        if (!get_field(line, delim, column, &field)) { invalid++; continue; }
        double x;
        if (!parse_double_arg(field, &x)) { invalid++; continue; }
        uint64_t bin;
        if (x <= minv) bin = 0u;
        else if (x >= maxv) bin = (uint64_t)(bins - 1u);
        else bin = (uint64_t)((x - minv) * scale);
        if (bin >= bins) bin = bins - 1u;
        if (!write_u64(out, bin, &symbol_hash)) { fclose(in); fclose(out); return 2; }
        valid++;
        symbols++;
        if (max_symbols != 0u && symbols >= max_symbols) break;
    }

    if (fclose(in) != 0 || fclose(out) != 0) { fprintf(stderr, "I/O close failed\n"); return 2; }

    kdna_kstream_header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, KDNA_KSTREAM_MAGIC, 8u);
    h.version = KDNA_KSTREAM_VERSION;
    h.header_bytes = KDNA_KSTREAM_HEADER_BYTES;
    h.kind = KDNA_KSTREAM_KIND_CSV;
    h.flags = KDNA_KSTREAM_FLAG_LE_U64 | KDNA_KSTREAM_FLAG_SUMMARY | KDNA_KSTREAM_FLAG_QUANTIZED;
    if (max_symbols == 0u) h.flags |= KDNA_KSTREAM_FLAG_UNLIMITED;
    if (auto_range) h.flags |= KDNA_KSTREAM_FLAG_AUTO_RANGE;
    h.symbol_bits = 64u;
    h.window = column;
    h.stride = 0u;
    h.bins = bins;
    h.input_bytes = input_bytes;
    h.input_records = valid + invalid + skipped;
    h.symbols_written = symbols;
    h.skipped_records = skipped;
    h.invalid_records = invalid;
    h.max_symbols = max_symbols;
    h.payload_bytes = symbols * 8u;
    h.seed = 0u;
    h.input_hash = input_hash;
    h.symbol_hash = symbol_hash;
    h.min_value = minv;
    h.max_value = maxv;
    h.scale = scale;
    h.offset = minv;
    base_name(h.source_name, sizeof(h.source_name), in_path);

    if (!write_summary_file(summary_path, &h)) { fprintf(stderr, "cannot write summary '%s'\n", summary_path); return 2; }
    printf("kdna_stream_csv: in=%s out=%s summary=%s column=%u bins=%u symbols=%" PRIu64 " invalid=%" PRIu64 " range=[%0.17g,%0.17g]\n",
           in_path, out_path, summary_path, column, bins, symbols, invalid, minv, maxv);
    return 0;
}
