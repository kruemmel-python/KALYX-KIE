
/*
  KALYX-ORIGIN v0.4
  Patternless / contextual local-origin diagnostics for uint64 streams.

  This tool measures origin-relevant diagnostics. It does NOT prove origin.
  It is intended for auditible scientific scans:
    - symbol entropy / concentration
    - transition-edge entropy / concentration
    - optional hamming drift against a control stream
    - optional transition-edge compatibility against a pattern stream
    - optional PLANT position manifest summarization

  Output: one CSV row per invocation.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

#define KORIGIN_VERSION "KORIGIN004"

typedef struct {
    uint64_t key;
    uint64_t count;
    uint8_t used;
} sym_slot_t;

typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t count;
    uint8_t used;
} edge_slot_t;

static uint64_t fnv1a64_bytes(const void *data, size_t len, uint64_t h) {
    const unsigned char *p = (const unsigned char*)data;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ull;
    }
    return h;
}

static uint64_t mix64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebull;
    x ^= x >> 31;
    return x;
}

static uint64_t edge_hash(uint64_t a, uint64_t b) {
    return mix64(a ^ (mix64(b) + 0x9e3779b97f4a7c15ull + (a << 6) + (a >> 2)));
}

static uint64_t next_pow2_u64(uint64_t x) {
    if (x < 16) return 16;
    x--;
    x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8;
    x |= x >> 16; x |= x >> 32;
    return x + 1;
}

static int parse_u64_arg(const char *s, uint64_t *out) {
    if (!s || !*s) return 0;
    errno = 0;
    char *end = NULL;
    int base = 10;
    if (strlen(s) > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) base = 16;
    unsigned long long v = strtoull(s, &end, base);
    if (errno || !end || *end != '\0') return 0;
    *out = (uint64_t)v;
    return 1;
}

static void usage(void) {
    puts("kalyx_origin v0.4 --in stream.u64 --label NAME --out-csv metrics.csv [options]\n");
    puts("Options:");
    puts("  --max-symbols N              maximum symbols to read, default: 0 = all available after skip");
    puts("  --skip-symbols N             symbols to skip before reading, default: 0");
    puts("  --control file.u64           optional same-length control stream for hamming drift");
    puts("  --control-skip N             symbols to skip in control stream, default: 0");
    puts("  --pattern file.u64           optional pattern stream; measures transition-edge compatibility");
    puts("  --positions file.csv         optional PLANT audit positions/anchors CSV");
    puts("  --plant-manifest file.csv    optional KPLANT key,value manifest");
    puts("  --append                     append row; write header only if file missing/empty");
    puts("  --help                       show help\n");
    puts("Semantics:");
    puts("  KALYX-ORIGIN v0.4 is patternless-capable and supports zoom scans via --skip-symbols.");
    puts("  It measures origin-relevant diagnostics, not final origin proof.");
}

static int file_size_bytes(const char *path, uint64_t *bytes) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
#if defined(_WIN32)
    long long pos = _ftelli64(f);
#else
    long pos = ftell(f);
#endif
    fclose(f);
    if (pos < 0) return 0;
    *bytes = (uint64_t)pos;
    return 1;
}

static uint64_t *read_u64_stream(const char *path, uint64_t skip_symbols, uint64_t max_symbols, uint64_t *out_n, uint64_t *out_hash) {
    *out_n = 0;
    *out_hash = 1469598103934665603ull;

    uint64_t bytes = 0;
    if (!file_size_bytes(path, &bytes)) {
        fprintf(stderr, "kalyx_origin: cannot stat %s\n", path);
        return NULL;
    }
    uint64_t total = bytes / 8u;
    if (skip_symbols >= total) {
        fprintf(stderr, "kalyx_origin: skip beyond EOF path=%s skip=%llu total=%llu\n",
            path, (unsigned long long)skip_symbols, (unsigned long long)total);
        return NULL;
    }
    uint64_t available = total - skip_symbols;
    uint64_t n = (max_symbols == 0 || max_symbols > available) ? available : max_symbols;
    if (n < 2) {
        fprintf(stderr, "kalyx_origin: need at least 2 symbols path=%s n=%llu\n", path, (unsigned long long)n);
        return NULL;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "kalyx_origin: cannot open %s\n", path);
        return NULL;
    }
#if defined(_WIN32)
    if (_fseeki64(f, (long long)(skip_symbols * 8u), SEEK_SET) != 0) {
#else
    if (fseek(f, (long)(skip_symbols * 8u), SEEK_SET) != 0) {
#endif
        fclose(f);
        fprintf(stderr, "kalyx_origin: seek failed %s\n", path);
        return NULL;
    }

    uint64_t *buf = (uint64_t*)malloc((size_t)n * sizeof(uint64_t));
    if (!buf) {
        fclose(f);
        fprintf(stderr, "kalyx_origin: out of memory reading %llu symbols\n", (unsigned long long)n);
        return NULL;
    }

    size_t got = fread(buf, sizeof(uint64_t), (size_t)n, f);
    fclose(f);
    if (got != (size_t)n) {
        free(buf);
        fprintf(stderr, "kalyx_origin: short read %s expected=%llu got=%zu\n", path, (unsigned long long)n, got);
        return NULL;
    }

    uint64_t h = 1469598103934665603ull;
    h = fnv1a64_bytes(buf, (size_t)n * sizeof(uint64_t), h);
    *out_n = n;
    *out_hash = h;
    return buf;
}

static int sym_add(sym_slot_t *tab, uint64_t cap, uint64_t key) {
    uint64_t i = mix64(key) & (cap - 1);
    for (;;) {
        if (!tab[i].used) {
            tab[i].used = 1;
            tab[i].key = key;
            tab[i].count = 1;
            return 1;
        }
        if (tab[i].key == key) {
            tab[i].count++;
            return 1;
        }
        i = (i + 1) & (cap - 1);
    }
}

static int edge_add(edge_slot_t *tab, uint64_t cap, uint64_t a, uint64_t b) {
    uint64_t i = edge_hash(a, b) & (cap - 1);
    for (;;) {
        if (!tab[i].used) {
            tab[i].used = 1;
            tab[i].a = a;
            tab[i].b = b;
            tab[i].count = 1;
            return 1;
        }
        if (tab[i].a == a && tab[i].b == b) {
            tab[i].count++;
            return 1;
        }
        i = (i + 1) & (cap - 1);
    }
}

static int edge_exists(edge_slot_t *tab, uint64_t cap, uint64_t a, uint64_t b) {
    uint64_t i = edge_hash(a, b) & (cap - 1);
    for (;;) {
        if (!tab[i].used) return 0;
        if (tab[i].a == a && tab[i].b == b) return 1;
        i = (i + 1) & (cap - 1);
    }
}

static int cmp_u64_desc(const void *pa, const void *pb) {
    const uint64_t a = *(const uint64_t*)pa;
    const uint64_t b = *(const uint64_t*)pb;
    return (a < b) - (a > b);
}

static double entropy_from_counts_u64(const uint64_t *counts, uint64_t m, uint64_t total) {
    if (total == 0) return 0.0;
    double H = 0.0;
    const double invlog2 = 1.0 / log(2.0);
    for (uint64_t i = 0; i < m; ++i) {
        if (counts[i] == 0) continue;
        double p = (double)counts[i] / (double)total;
        H -= p * log(p) * invlog2;
    }
    return H;
}

static double top_mass(uint64_t *counts, uint64_t m, uint64_t total, uint64_t topn) {
    if (m == 0 || total == 0) return 0.0;
    qsort(counts, (size_t)m, sizeof(uint64_t), cmp_u64_desc);
    uint64_t k = topn < m ? topn : m;
    uint64_t s = 0;
    for (uint64_t i = 0; i < k; ++i) s += counts[i];
    return (double)s / (double)total;
}

typedef struct {
    uint64_t unique_symbols;
    double entropy_bits;
    double top1_mass;
    double top8_mass;
    double top32_mass;
    uint64_t unique_edges;
    double edge_entropy_bits;
    double edge_top1_mass;
    double edge_top8_mass;
    double edge_top32_mass;
} metrics_t;

static int compute_metrics(const uint64_t *x, uint64_t n, metrics_t *m, edge_slot_t **out_edges, uint64_t *out_edge_cap) {
    memset(m, 0, sizeof(*m));

    uint64_t sym_cap = next_pow2_u64(n * 2u);
    sym_slot_t *stab = (sym_slot_t*)calloc((size_t)sym_cap, sizeof(sym_slot_t));
    if (!stab) return 0;

    uint64_t edge_n = n - 1u;
    uint64_t edge_cap = next_pow2_u64(edge_n * 4u);
    edge_slot_t *etab = (edge_slot_t*)calloc((size_t)edge_cap, sizeof(edge_slot_t));
    if (!etab) { free(stab); return 0; }

    for (uint64_t i = 0; i < n; ++i) sym_add(stab, sym_cap, x[i]);
    for (uint64_t i = 0; i + 1 < n; ++i) edge_add(etab, edge_cap, x[i], x[i+1]);

    uint64_t us = 0;
    for (uint64_t i = 0; i < sym_cap; ++i) if (stab[i].used) us++;
    uint64_t ue = 0;
    for (uint64_t i = 0; i < edge_cap; ++i) if (etab[i].used) ue++;

    uint64_t *sc = (uint64_t*)malloc((size_t)us * sizeof(uint64_t));
    uint64_t *ec = (uint64_t*)malloc((size_t)ue * sizeof(uint64_t));
    uint64_t *sc2 = (uint64_t*)malloc((size_t)us * sizeof(uint64_t));
    uint64_t *ec2 = (uint64_t*)malloc((size_t)ue * sizeof(uint64_t));
    if (!sc || !ec || !sc2 || !ec2) {
        free(stab); free(etab); free(sc); free(ec); free(sc2); free(ec2);
        return 0;
    }

    uint64_t p = 0;
    for (uint64_t i = 0; i < sym_cap; ++i) if (stab[i].used) sc[p++] = stab[i].count;
    p = 0;
    for (uint64_t i = 0; i < edge_cap; ++i) if (etab[i].used) ec[p++] = etab[i].count;

    memcpy(sc2, sc, (size_t)us * sizeof(uint64_t));
    memcpy(ec2, ec, (size_t)ue * sizeof(uint64_t));
    m->unique_symbols = us;
    m->entropy_bits = entropy_from_counts_u64(sc, us, n);
    m->top1_mass = top_mass(sc2, us, n, 1);
    memcpy(sc2, sc, (size_t)us * sizeof(uint64_t));
    m->top8_mass = top_mass(sc2, us, n, 8);
    memcpy(sc2, sc, (size_t)us * sizeof(uint64_t));
    m->top32_mass = top_mass(sc2, us, n, 32);

    memcpy(ec2, ec, (size_t)ue * sizeof(uint64_t));
    m->unique_edges = ue;
    m->edge_entropy_bits = entropy_from_counts_u64(ec, ue, edge_n);
    m->edge_top1_mass = top_mass(ec2, ue, edge_n, 1);
    memcpy(ec2, ec, (size_t)ue * sizeof(uint64_t));
    m->edge_top8_mass = top_mass(ec2, ue, edge_n, 8);
    memcpy(ec2, ec, (size_t)ue * sizeof(uint64_t));
    m->edge_top32_mass = top_mass(ec2, ue, edge_n, 32);

    free(stab);
    free(sc); free(ec); free(sc2); free(ec2);

    if (out_edges) {
        *out_edges = etab;
        *out_edge_cap = edge_cap;
    } else {
        free(etab);
    }
    return 1;
}

static uint64_t count_lines_data(const char *path) {
    if (!path) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char line[4096];
    uint64_t count = 0;
    int first = 1;
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) continue;
        if (first) {
            first = 0;
            if (strstr(line, "pos") || strstr(line, "anchor")) continue;
        }
        count++;
    }
    fclose(f);
    return count;
}

static void csv_header(FILE *out) {
    fputs(
        "version,label,path,n,skip_symbols,fnv1a64,"
        "unique_symbols,entropy_bits,top1_mass,top8_mass,top32_mass,"
        "unique_edges,edge_entropy_bits,edge_top1_mass,edge_top8_mass,edge_top32_mass,"
        "control_hamming,pattern_edge_accuracy,positions_count,position_density,"
        "gap_mean,gap_std,gap_min,gap_max,jitter_abs_mean,"
        "plant_like_score,origin_concentration_score,origin_order_score\n", out);
}

int main(int argc, char **argv) {
    const char *in = NULL, *label = NULL, *out_csv = NULL;
    const char *control = NULL, *pattern = NULL, *positions = NULL, *plant_manifest = NULL;
    uint64_t max_symbols = 0, skip_symbols = 0, control_skip = 0;
    int append = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--in") == 0 && i + 1 < argc) in = argv[++i];
        else if (strcmp(argv[i], "--label") == 0 && i + 1 < argc) label = argv[++i];
        else if (strcmp(argv[i], "--out-csv") == 0 && i + 1 < argc) out_csv = argv[++i];
        else if (strcmp(argv[i], "--max-symbols") == 0 && i + 1 < argc) { if (!parse_u64_arg(argv[++i], &max_symbols)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--skip-symbols") == 0 && i + 1 < argc) { if (!parse_u64_arg(argv[++i], &skip_symbols)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--control") == 0 && i + 1 < argc) control = argv[++i];
        else if (strcmp(argv[i], "--control-skip") == 0 && i + 1 < argc) { if (!parse_u64_arg(argv[++i], &control_skip)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--pattern") == 0 && i + 1 < argc) pattern = argv[++i];
        else if (strcmp(argv[i], "--positions") == 0 && i + 1 < argc) positions = argv[++i];
        else if (strcmp(argv[i], "--plant-manifest") == 0 && i + 1 < argc) plant_manifest = argv[++i];
        else if (strcmp(argv[i], "--append") == 0) append = 1;
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (!in || !label || !out_csv) {
        usage();
        return 2;
    }

    uint64_t n = 0, hash = 0;
    uint64_t *x = read_u64_stream(in, skip_symbols, max_symbols, &n, &hash);
    if (!x) return 1;

    metrics_t m;
    edge_slot_t *edges = NULL;
    uint64_t edge_cap = 0;
    if (!compute_metrics(x, n, &m, &edges, &edge_cap)) {
        fprintf(stderr, "kalyx_origin: metric computation failed\n");
        free(x);
        return 1;
    }

    double control_hamming = -1.0;
    if (control) {
        uint64_t cn = 0, ch = 0;
        uint64_t *c = read_u64_stream(control, control_skip, n, &cn, &ch);
        if (!c || cn != n) {
            fprintf(stderr, "kalyx_origin: control length mismatch\n");
            free(c); free(x); free(edges);
            return 1;
        }
        uint64_t diff = 0;
        for (uint64_t i = 0; i < n; ++i) if (x[i] != c[i]) diff++;
        control_hamming = (double)diff / (double)n;
        free(c);
    }

    double pattern_edge_accuracy = 0.0;
    if (pattern) {
        uint64_t pn = 0, ph = 0;
        uint64_t *p = read_u64_stream(pattern, 0, n, &pn, &ph);
        if (!p || pn < 2) {
            fprintf(stderr, "kalyx_origin: cannot read pattern\n");
            free(p); free(x); free(edges);
            return 1;
        }
        metrics_t pm;
        edge_slot_t *pedges = NULL;
        uint64_t pedge_cap = 0;
        if (!compute_metrics(p, pn, &pm, &pedges, &pedge_cap)) {
            fprintf(stderr, "kalyx_origin: pattern metric computation failed\n");
            free(p); free(x); free(edges);
            return 1;
        }
        uint64_t matches = 0, trans = n - 1u;
        for (uint64_t i = 0; i + 1 < n; ++i) {
            if (edge_exists(pedges, pedge_cap, x[i], x[i+1])) matches++;
        }
        pattern_edge_accuracy = (double)matches / (double)trans;
        free(pedges);
        free(p);
    }

    uint64_t positions_count = count_lines_data(positions);
    double position_density = 0.0;
    if (positions_count > 0) position_density = (double)positions_count / (double)n;

    double gap_mean = 0.0, gap_std = 0.0, gap_min = 0.0, gap_max = 0.0, jitter_abs_mean = 0.0;
    (void)plant_manifest;
    /* v0.4 keeps detailed gap reconstruction in PowerShell for now.
       The C ABI exposes fields so reports remain schema-stable. */

    double origin_concentration_score = 0.5 * m.top32_mass + 0.5 * m.edge_top32_mass;
    double origin_order_score = pattern ? pattern_edge_accuracy : 0.0;
    double plant_like_score = pattern
        ? (0.70 * pattern_edge_accuracy + 0.15 * m.top32_mass + 0.15 * m.edge_top32_mass)
        : origin_concentration_score;

    int need_header = 1;
    if (append) {
        FILE *test = fopen(out_csv, "rb");
        if (test) {
            if (fseek(test, 0, SEEK_END) == 0 && ftell(test) > 0) need_header = 0;
            fclose(test);
        }
    }
    FILE *out = fopen(out_csv, append ? "ab" : "wb");
    if (!out) {
        fprintf(stderr, "kalyx_origin: cannot write %s\n", out_csv);
        free(x); free(edges);
        return 1;
    }
    if (need_header) csv_header(out);

    fprintf(out,
        "%s,%s,%s,%llu,%llu,0x%016llx,"
        "%llu,%.12f,%.12f,%.12f,%.12f,"
        "%llu,%.12f,%.12f,%.12f,%.12f,"
        "%.12f,%.12f,%llu,%.12f,"
        "%.12f,%.12f,%.12f,%.12f,%.12f,"
        "%.12f,%.12f,%.12f\n",
        KORIGIN_VERSION, label, in,
        (unsigned long long)n,
        (unsigned long long)skip_symbols,
        (unsigned long long)hash,
        (unsigned long long)m.unique_symbols,
        m.entropy_bits, m.top1_mass, m.top8_mass, m.top32_mass,
        (unsigned long long)m.unique_edges,
        m.edge_entropy_bits, m.edge_top1_mass, m.edge_top8_mass, m.edge_top32_mass,
        control_hamming, pattern_edge_accuracy,
        (unsigned long long)positions_count, position_density,
        gap_mean, gap_std, gap_min, gap_max, jitter_abs_mean,
        plant_like_score, origin_concentration_score, origin_order_score
    );
    fclose(out);

    printf("kalyx_origin v0.4: label=%s n=%llu skip=%llu entropy=%.9f top32=%.9f edge_entropy=%.9f edge_top32=%.9f pattern_edge=%.9f concentration=%.9f\n",
        label,
        (unsigned long long)n,
        (unsigned long long)skip_symbols,
        m.entropy_bits, m.top32_mass, m.edge_entropy_bits, m.edge_top32_mass,
        pattern_edge_accuracy, origin_concentration_score);

    free(x);
    free(edges);
    return 0;
}
