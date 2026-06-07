
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
    fprintf(stderr, "kdna_stream_probe --a file.kstream | file.kstream\n");
}
int main(int argc, char **argv) {
    const char *path = NULL;
    if (argc == 2) path = argv[1];
    else if (argc == 3 && strcmp(argv[1], "--a") == 0) path = argv[2];
    else { usage(); return 2; }
    return inspect_summary(path);
}
