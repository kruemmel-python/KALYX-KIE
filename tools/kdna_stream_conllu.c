
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kdna_kstream.h"

#ifndef KDNA_KSTREAM_KIND_CONLLU
#define KDNA_KSTREAM_KIND_CONLLU 4u
#endif

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

static uint64_t fnv1a_str(uint64_t h, const char *s) {
    return fnv1a_update(h, s, strlen(s));
}

static uint64_t fold32_u64(uint64_t x) {
    uint32_t lo = (uint32_t)(x & 0xffffffffu);
    uint32_t hi = (uint32_t)(x >> 32);
    uint32_t y = lo ^ hi;
    return (uint64_t)(y ? y : 1u);
}

static uint64_t canonical_symbol(uint64_t x, uint32_t symbol_bits) {
    if (symbol_bits == 32u) return fold32_u64(x);
    return x ? x : 1u;
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
    uint64_t v = 0u;
    if (!parse_u64(s, &v) || v > UINT32_MAX) return 0;
    *out = (uint32_t)v;
    return 1;
}

static void lower_ascii(char *s) {
    if (!s) return;
    while (*s) {
        *s = (char)tolower((unsigned char)*s);
        ++s;
    }
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
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return 0; }
    return fclose(f) == 0;
}

static uint64_t upos_code(const char *s) {
    if (!s) return 0u;
    if (strcmp(s, "ADJ") == 0) return 1u;
    if (strcmp(s, "ADP") == 0) return 2u;
    if (strcmp(s, "ADV") == 0) return 3u;
    if (strcmp(s, "AUX") == 0) return 4u;
    if (strcmp(s, "CCONJ") == 0) return 5u;
    if (strcmp(s, "DET") == 0) return 6u;
    if (strcmp(s, "INTJ") == 0) return 7u;
    if (strcmp(s, "NOUN") == 0) return 8u;
    if (strcmp(s, "NUM") == 0) return 9u;
    if (strcmp(s, "PART") == 0) return 10u;
    if (strcmp(s, "PRON") == 0) return 11u;
    if (strcmp(s, "PROPN") == 0) return 12u;
    if (strcmp(s, "PUNCT") == 0) return 13u;
    if (strcmp(s, "SCONJ") == 0) return 14u;
    if (strcmp(s, "SYM") == 0) return 15u;
    if (strcmp(s, "VERB") == 0) return 16u;
    if (strcmp(s, "X") == 0) return 17u;
    return 0u;
}

static uint64_t combine_window(const uint64_t *buf, uint32_t window, uint32_t symbol_bits) {
    uint64_t h = FNV64_OFFSET;
    for (uint32_t i = 0u; i < window; ++i) {
        h = fnv1a_update(h, &buf[i], sizeof(uint64_t));
    }
    return canonical_symbol(h, symbol_bits);
}

static int split_tab(char *line, char **cols, int max_cols) {
    int n = 0;
    char *p = line;
    if (!line || !cols || max_cols <= 0) return 0;
    cols[n++] = p;
    while (*p && n < max_cols) {
        if (*p == '\t') {
            *p = 0;
            cols[n++] = p + 1;
        }
        ++p;
    }
    return n;
}

static int id_is_regular_token(const char *id) {
    if (!id || !*id) return 0;
    while (*id) {
        if (*id == '-' || *id == '.') return 0;
        if (!isdigit((unsigned char)*id)) return 0;
        ++id;
    }
    return 1;
}

static void sanitize_eol(char *s) {
    size_t n;
    if (!s) return;
    n = strlen(s);
    while (n > 0u && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[--n] = 0;
    }
}

static void selected_key(char *dst, size_t cap, const char *field, char **c) {
    const char *form = c[1], *lemma = c[2], *upos = c[3], *feats = c[5], *deprel = c[7];
    if (strcmp(field, "form") == 0) {
        snprintf(dst, cap, "%s", form);
        lower_ascii(dst);
    } else if (strcmp(field, "lemma") == 0) {
        snprintf(dst, cap, "%s", lemma);
        lower_ascii(dst);
    } else if (strcmp(field, "upos") == 0) {
        snprintf(dst, cap, "%s", upos);
    } else if (strcmp(field, "feats") == 0) {
        snprintf(dst, cap, "%s", feats);
    } else if (strcmp(field, "deprel") == 0) {
        snprintf(dst, cap, "%s", deprel);
    } else if (strcmp(field, "upos_feats") == 0) {
        snprintf(dst, cap, "%s|%s", upos, feats);
    } else if (strcmp(field, "upos_deprel") == 0) {
        snprintf(dst, cap, "%s|%s", upos, deprel);
    } else if (strcmp(field, "lemma_upos") == 0) {
        char tmp[256];
        snprintf(tmp, sizeof(tmp), "%s", lemma);
        lower_ascii(tmp);
        snprintf(dst, cap, "%s|%s", tmp, upos);
    } else {
        snprintf(dst, cap, "%s", upos);
    }
}

static uint64_t key_symbol(const char *field, const char *key, uint32_t symbol_bits) {
    uint64_t h;
    (void)field;
    /*
       KLANG v1.1 symbol-space rule:
       Every linguistic atom, including UPOS, is hashed into the same high-entropy
       symbolic substrate. The previous small integer UPOS codes (DET=6, NOUN=8...)
       clustered near zero under the existing kdna_project source range [0, 2^32-1]
       and could collapse downstream KGRAMs to a single edge. Hashing + optional
       32-bit folding preserves cross-language equality for identical tags while
       spreading the symbols across the KDNA projection interval.
    */
    h = fnv1a_str(FNV64_OFFSET, key);
    return canonical_symbol(h, symbol_bits);
}

static void usage(void) {
    fprintf(stderr,
        "kdna_stream_conllu --in file.conllu --out out.u64 --summary out.kstream [options]\n"
        "Options:\n"
        "  --field upos|form|lemma|feats|deprel|upos_feats|upos_deprel|lemma_upos  default: upos\n"
        "  --window N                 default: 1\n"
        "  --stride N                 default: 1\n"
        "  --sentence-boundary reset|token  default: reset\n"
        "  --max-symbols N            default: 0 (unlimited)\n"
        "  --symbol-bits 32|64        default: 32 for KDNA projection compatibility\n"
        "\n"
        "CoNLL-U columns: ID FORM LEMMA UPOS XPOS FEATS HEAD DEPREL DEPS MISC.\n"
        "Multiword tokens and empty nodes are skipped.\n");
}

int main(int argc, char **argv) {
    const char *in_path = NULL;
    const char *out_path = NULL;
    const char *summary_path = NULL;
    const char *field = "upos";
    const char *boundary = "reset";
    uint64_t max_symbols = 0u;
    uint32_t window = 1u, stride = 1u, symbol_bits = 32u;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--in") == 0 && i + 1 < argc) in_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--summary") == 0 && i + 1 < argc) summary_path = argv[++i];
        else if (strcmp(argv[i], "--field") == 0 && i + 1 < argc) field = argv[++i];
        else if (strcmp(argv[i], "--window") == 0 && i + 1 < argc) { if (!parse_u32(argv[++i], &window)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--stride") == 0 && i + 1 < argc) { if (!parse_u32(argv[++i], &stride)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--max-symbols") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &max_symbols)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--symbol-bits") == 0 && i + 1 < argc) { if (!parse_u32(argv[++i], &symbol_bits) || !(symbol_bits == 32u || symbol_bits == 64u)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--sentence-boundary") == 0 && i + 1 < argc) boundary = argv[++i];
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }
    if (!in_path || !out_path || !summary_path || window < 1u || window > 16u || stride < 1u) {
        usage(); return 2;
    }
    if (!(strcmp(boundary, "reset") == 0 || strcmp(boundary, "token") == 0)) {
        usage(); return 2;
    }

    FILE *in = fopen(in_path, "rb");
    if (!in) { fprintf(stderr, "cannot open input '%s'\n", in_path); return 2; }
    FILE *out = fopen(out_path, "wb");
    if (!out) { fclose(in); fprintf(stderr, "cannot open output '%s'\n", out_path); return 2; }

    char line[8192];
    uint64_t input_hash = FNV64_OFFSET, symbol_hash = FNV64_OFFSET;
    uint64_t input_bytes = 0u, input_records = 0u, skipped = 0u, invalid = 0u, symbols = 0u;
    uint64_t rolling[16];
    uint32_t have = 0u, stride_counter = 0u;
    int failed = 0;

    while (fgets(line, sizeof(line), in)) {
        size_t raw_n = strlen(line);
        input_bytes += (uint64_t)raw_n;
        input_hash = fnv1a_update(input_hash, line, raw_n);
        sanitize_eol(line);

        if (line[0] == '#') {
            continue;
        }

        if (line[0] == 0) {
            if (strcmp(boundary, "token") == 0) {
                uint64_t b = UINT64_C(0xffffffffffffffff);
                if (!write_u64(out, b, &symbol_hash)) { failed = 1; break; }
                symbols++;
                if (max_symbols && symbols >= max_symbols) break;
            }
            have = 0u;
            stride_counter = 0u;
            continue;
        }

        char *cols[10] = {0};
        int coln = split_tab(line, cols, 10);
        if (coln < 10 || !id_is_regular_token(cols[0])) {
            skipped++;
            continue;
        }

        input_records++;
        char key[1024];
        selected_key(key, sizeof(key), field, cols);
        if (key[0] == 0 || strcmp(key, "_") == 0) {
            invalid++;
            continue;
        }
        uint64_t sym = key_symbol(field, key, symbol_bits);
        if (sym == 0u) sym = 1u;

        if (window == 1u) {
            if (stride_counter == 0u) {
                if (!write_u64(out, sym, &symbol_hash)) { failed = 1; break; }
                symbols++;
                if (max_symbols && symbols >= max_symbols) break;
            }
            stride_counter = (stride_counter + 1u) % stride;
        } else {
            if (have < window) {
                rolling[have++] = sym;
            } else {
                memmove(rolling, rolling + 1, (window - 1u) * sizeof(uint64_t));
                rolling[window - 1u] = sym;
            }
            if (have >= window) {
                if (stride_counter == 0u) {
                    uint64_t w = combine_window(rolling, window, symbol_bits);
                    if (!write_u64(out, w, &symbol_hash)) { failed = 1; break; }
                    symbols++;
                    if (max_symbols && symbols >= max_symbols) break;
                }
                stride_counter = (stride_counter + 1u) % stride;
            }
        }
    }

    if (ferror(in)) failed = 1;
    fclose(in);
    if (fclose(out) != 0) failed = 1;
    if (failed) { fprintf(stderr, "I/O failed\n"); return 2; }

    kdna_kstream_header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, KDNA_KSTREAM_MAGIC, 8u);
    h.version = KDNA_KSTREAM_VERSION;
    h.header_bytes = KDNA_KSTREAM_HEADER_BYTES;
    h.kind = KDNA_KSTREAM_KIND_CONLLU;
    h.flags = KDNA_KSTREAM_FLAG_LE_U64 | KDNA_KSTREAM_FLAG_SUMMARY | KDNA_KSTREAM_FLAG_HASHED;
    if (max_symbols == 0u) h.flags |= KDNA_KSTREAM_FLAG_UNLIMITED;
    h.symbol_bits = symbol_bits;
    h.window = window;
    h.stride = stride;
    h.bins = 0u;
    h.input_bytes = input_bytes;
    h.input_records = input_records;
    h.symbols_written = symbols;
    h.skipped_records = skipped;
    h.invalid_records = invalid;
    h.max_symbols = max_symbols;
    h.payload_bytes = symbols * sizeof(uint64_t);
    h.seed = FNV64_OFFSET;
    h.input_hash = input_hash;
    h.symbol_hash = symbol_hash;
    base_name(h.source_name, sizeof(h.source_name), in_path);

    if (!write_summary_file(summary_path, &h)) {
        fprintf(stderr, "cannot write summary '%s'\n", summary_path);
        return 2;
    }

    printf("kdna_stream_conllu: in=%s out=%s field=%s window=%u stride=%u boundary=%s symbol_bits=%u records=%" PRIu64
           " symbols=%" PRIu64 " skipped=%" PRIu64 " invalid=%" PRIu64 "\n",
           in_path, out_path, field, window, stride, boundary, symbol_bits, input_records, symbols, skipped, invalid);
    printf("summary=%s\n", summary_path);
    return 0;
}
