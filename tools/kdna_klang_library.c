#include "kdna_kllib.h"
#include "kdna_kstream.h"
#include "kdna_kgram.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_CONFIGS 128u
#define MAX_CELLS 1024u

typedef struct input_config {
    const char* name;
    const char* field;
    uint32_t window;
    uint32_t symbol_bits;
    const char* de_symbols;
    const char* de_kstream;
    const char* de_kdna;
    const char* de_kgram;
    const char* en_symbols;
    const char* en_kstream;
    const char* en_kdna;
    const char* en_kgram;
    const char* matrix_csv;
    const char* null_csv;
    const char* report_html;
    const char* observed_csv;
} input_config;

static void usage(void) {
    printf(
        "kdna_klang_library --config name field window symbol_bits de_symbols de_kstream de_kdna de_kgram en_symbols en_kstream en_kdna en_kgram matrix_csv null_csv report_html observed_vs_null_csv --out out.kllib [--json out.json] [--csv out.csv]\n"
        "kdna_klang_library --a file.kllib\n\n"
        "KLLIB001 indexes KLANG/KFIELD artifacts without recomputing KSTREAM, KDNA, KGRAM, matrices or null models.\n"
    );
}

static uint64_t fnv1a64_update(uint64_t h, const void* data, size_t n) {
    const unsigned char* p = (const unsigned char*)data;
    for (size_t i = 0; i < n; ++i) { h ^= (uint64_t)p[i]; h *= 1099511628211ull; }
    return h;
}

static int file_hash_size(const char* path, uint64_t* out_hash, uint64_t* out_size) {
    *out_hash = 0; *out_size = 0;
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    unsigned char buf[65536];
    uint64_t h = 1469598103934665603ull;
    uint64_t total = 0;
    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), f);
        if (n) {
            h = fnv1a64_update(h, buf, n);
            total += (uint64_t)n;
        }
        if (n < sizeof(buf)) {
            if (ferror(f)) { fclose(f); return -2; }
            break;
        }
    }
    fclose(f);
    *out_hash = h;
    *out_size = total;
    return 0;
}

static void copy_str(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return;
    if (!src) src = "";
    snprintf(dst, cap, "%s", src);
}

static void json_escape(FILE* f, const char* s) {
    fputc('"', f);
    if (s) {
        for (const unsigned char* p=(const unsigned char*)s; *p; ++p) {
            unsigned char c = *p;
            if (c == '"' || c == '\\') { fputc('\\', f); fputc((int)c, f); }
            else if (c == '\n') fputs("\\n", f);
            else if (c == '\r') fputs("\\r", f);
            else if (c == '\t') fputs("\\t", f);
            else if (c < 32) fprintf(f, "\\u%04x", (unsigned)c);
            else fputc((int)c, f);
        }
    }
    fputc('"', f);
}

static char* trim(char* s) {
    while (*s && isspace((unsigned char)*s)) ++s;
    char* e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) --e;
    *e = 0;
    return s;
}

static int csv_split_quoted(char* line, char** fields, int max_fields) {
    int n = 0;
    char* p = line;
    while (*p && n < max_fields) {
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '"') {
            ++p;
            fields[n++] = p;
            char* out = p;
            for (;;) {
                if (*p == 0) { *out = 0; break; }
                if (*p == '"' && p[1] == '"') { *out++ = '"'; p += 2; continue; }
                if (*p == '"') {
                    *out = 0; ++p;
                    while (*p && *p != ',' && *p != ';') ++p;
                    if (*p == ',' || *p == ';') ++p;
                    break;
                }
                *out++ = *p++;
            }
        } else {
            fields[n++] = p;
            while (*p && *p != ',' && *p != ';') ++p;
            if (*p) { *p = 0; ++p; }
            fields[n-1] = trim(fields[n-1]);
        }
    }
    return n;
}

static double parse_double_locale(const char* s) {
    char tmp[128];
    size_t n = strlen(s);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    for (size_t i=0; i<n; ++i) tmp[i] = (s[i] == ',') ? '.' : s[i];
    tmp[n] = 0;
    return strtod(tmp, NULL);
}

static uint64_t parse_u64_text(const char* s) {
    if (!s || !*s) return 0;
    char tmp[128];
    size_t n = strlen(s);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;
    memcpy(tmp, s, n); tmp[n] = 0;
    return (uint64_t)strtoull(tmp, NULL, 10);
}

static int read_kstream(const char* path, kdna_kstream_header* h) {
    memset(h, 0, sizeof(*h));
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(h, 1, sizeof(*h), f);
    fclose(f);
    if (n != sizeof(*h)) return -2;
    if (memcmp(h->magic, KDNA_KSTREAM_MAGIC, 8) != 0) return -3;
    return 0;
}

static int read_kgram(const char* path, kdna_kgram_header* h) {
    memset(h, 0, sizeof(*h));
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(h, 1, sizeof(*h), f);
    fclose(f);
    if (n != sizeof(*h)) return -2;
    if (memcmp(h->magic, KDNA_KGRAM_MAGIC, 8) != 0) return -3;
    return 0;
}

static int parse_observed_csv(const char* path, const char* config_name,
                              kdna_kllib_cell_record* cells, uint32_t* cell_count,
                              double* self_obs, double* cross_obs,
                              double* self_delta, double* cross_delta) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    char line[4096];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -2; }

    int self_n = 0, cross_n = 0;
    double so = 0, co = 0, sd = 0, cd = 0;

    while (fgets(line, sizeof(line), f)) {
        char* s = trim(line);
        if (!*s) continue;
        char* fields[16] = {0};
        int nf = csv_split_quoted(s, fields, 16);
        if (nf < 12) continue;
        if (*cell_count >= MAX_CELLS) { fclose(f); return -3; }

        kdna_kllib_cell_record* c = &cells[(*cell_count)++];
        memset(c, 0, sizeof(*c));
        copy_str(c->config, sizeof(c->config), config_name);
        copy_str(c->row, sizeof(c->row), fields[0]);
        copy_str(c->col, sizeof(c->col), fields[1]);
        copy_str(c->cell_class, sizeof(c->cell_class), fields[2]);
        c->observed_accuracy = parse_double_locale(fields[3]);
        c->baseline_accuracy = parse_double_locale(fields[4]);
        c->lift = parse_double_locale(fields[5]);
        c->surprise_rate = parse_double_locale(fields[6]);
        c->out_of_grammar = parse_u64_text(fields[7]);
        c->grammar_edges = parse_u64_text(fields[8]);
        c->avg_null_accuracy = parse_double_locale(fields[9]);
        c->observed_minus_null = parse_double_locale(fields[10]);
        copy_str(c->null_modes, sizeof(c->null_modes), fields[11]);

        if (strcmp(fields[2], "self") == 0) {
            so += c->observed_accuracy;
            sd += c->observed_minus_null;
            ++self_n;
        } else {
            co += c->observed_accuracy;
            cd += c->observed_minus_null;
            ++cross_n;
        }
    }
    fclose(f);

    *self_obs = self_n ? so / (double)self_n : 0.0;
    *cross_obs = cross_n ? co / (double)cross_n : 0.0;
    *self_delta = self_n ? sd / (double)self_n : 0.0;
    *cross_delta = cross_n ? cd / (double)cross_n : 0.0;
    return 0;
}

static int inspect_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return 2; }
    kdna_kllib_header h;
    size_t n = fread(&h, 1, sizeof(h), f);
    fclose(f);
    if (n != sizeof(h) || memcmp(h.magic, KDNA_KLLIB_MAGIC, 8) != 0) {
        fprintf(stderr, "not KLLIB001: %s\n", path);
        return 2;
    }
    printf("file: %s\n", path);
    printf("  magic: KLLIB001 version:%u configs:%u cells:%u\n", h.version, h.config_count, h.cell_count);
    printf("  config_record_bytes:%u cell_record_bytes:%u header_bytes:%u\n", h.config_record_bytes, h.cell_record_bytes, h.header_bytes);
    printf("  total_de_symbols:%" PRIu64 " total_en_symbols:%" PRIu64 "\n", h.total_de_symbols, h.total_en_symbols);
    printf("  total_de_kgram_rules:%" PRIu64 " total_en_kgram_rules:%" PRIu64 "\n", h.total_de_kgram_rules, h.total_en_kgram_rules);
    printf("  artifact_bytes:%" PRIu64 " flags:0x%" PRIx64 " hash:0x%016" PRIx64 "\n", h.total_artifact_bytes, h.flags, h.library_hash);
    printf("  avg_self_acc:%.12g avg_cross_acc:%.12g\n", h.avg_self_observed_accuracy, h.avg_cross_observed_accuracy);
    printf("  avg_self_delta:%.12g avg_cross_delta:%.12g\n", h.avg_self_observed_minus_null, h.avg_cross_observed_minus_null);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }

    const char* out_path = NULL;
    const char* json_path = NULL;
    const char* csv_path = NULL;
    const char* inspect = NULL;

    input_config inputs[MAX_CONFIGS];
    uint32_t input_count = 0;

    for (int i=1; i<argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else if (strcmp(argv[i], "--a") == 0 && i + 1 < argc) { inspect = argv[++i]; }
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) { out_path = argv[++i]; }
        else if (strcmp(argv[i], "--json") == 0 && i + 1 < argc) { json_path = argv[++i]; }
        else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) { csv_path = argv[++i]; }
        else if (strcmp(argv[i], "--config") == 0) {
            if (i + 16 >= argc) { fprintf(stderr, "--config needs 16 arguments\n"); return 2; }
            if (input_count >= MAX_CONFIGS) { fprintf(stderr, "too many configs\n"); return 2; }
            input_config* c = &inputs[input_count++];
            c->name = argv[++i];
            c->field = argv[++i];
            c->window = (uint32_t)strtoul(argv[++i], NULL, 10);
            c->symbol_bits = (uint32_t)strtoul(argv[++i], NULL, 10);
            c->de_symbols = argv[++i];
            c->de_kstream = argv[++i];
            c->de_kdna = argv[++i];
            c->de_kgram = argv[++i];
            c->en_symbols = argv[++i];
            c->en_kstream = argv[++i];
            c->en_kdna = argv[++i];
            c->en_kgram = argv[++i];
            c->matrix_csv = argv[++i];
            c->null_csv = argv[++i];
            c->report_html = argv[++i];
            c->observed_csv = argv[++i];
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            usage();
            return 2;
        }
    }

    if (inspect) return inspect_file(inspect);
    if (!out_path || input_count == 0) { usage(); return 2; }

    kdna_kllib_config_record configs[MAX_CONFIGS];
    kdna_kllib_cell_record cells[MAX_CELLS];
    memset(configs, 0, sizeof(configs));
    memset(cells, 0, sizeof(cells));
    uint32_t cell_count = 0;

    kdna_kllib_header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, KDNA_KLLIB_MAGIC, 8);
    h.version = KDNA_KLLIB_VERSION;
    h.header_bytes = KDNA_KLLIB_HEADER_BYTES;
    h.config_record_bytes = KDNA_KLLIB_CONFIG_RECORD_BYTES;
    h.cell_record_bytes = KDNA_KLLIB_CELL_RECORD_BYTES;
    h.config_count = input_count;
    h.flags = KDNA_KLLIB_FLAG_LE | KDNA_KLLIB_FLAG_COMPLETE | KDNA_KLLIB_FLAG_HAS_NULLS | KDNA_KLLIB_FLAG_HAS_REPORTS;

    double sum_self_obs = 0, sum_cross_obs = 0, sum_self_delta = 0, sum_cross_delta = 0;
    uint32_t metric_configs = 0;

    for (uint32_t i=0; i<input_count; ++i) {
        const input_config* in = &inputs[i];
        kdna_kllib_config_record* r = &configs[i];
        memset(r, 0, sizeof(*r));
        copy_str(r->name, sizeof(r->name), in->name);
        copy_str(r->field, sizeof(r->field), in->field);
        r->window = in->window;
        r->symbol_bits = in->symbol_bits;
        r->flags = 0x0fu;

        kdna_kstream_header ks_de, ks_en;
        kdna_kgram_header kg_de, kg_en;
        if (read_kstream(in->de_kstream, &ks_de) != 0) { fprintf(stderr, "bad de kstream: %s\n", in->de_kstream); return 3; }
        if (read_kstream(in->en_kstream, &ks_en) != 0) { fprintf(stderr, "bad en kstream: %s\n", in->en_kstream); return 3; }
        if (read_kgram(in->de_kgram, &kg_de) != 0) { fprintf(stderr, "bad de kgram: %s\n", in->de_kgram); return 3; }
        if (read_kgram(in->en_kgram, &kg_en) != 0) { fprintf(stderr, "bad en kgram: %s\n", in->en_kgram); return 3; }

        r->de_kstream_symbols = ks_de.symbols_written;
        r->en_kstream_symbols = ks_en.symbols_written;
        r->de_symbols = ks_de.symbols_written;
        r->en_symbols = ks_en.symbols_written;
        r->de_kgram_rules = kg_de.rule_count;
        r->en_kgram_rules = kg_en.rule_count;
        r->de_kgram_source_n = kg_de.source_n;
        r->en_kgram_source_n = kg_en.source_n;

        uint64_t bytes = 0, hash = 0;
        if (file_hash_size(in->de_symbols, &r->de_symbols_hash, &r->de_symbols_bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->de_symbols); return 3; }
        if (file_hash_size(in->en_symbols, &r->en_symbols_hash, &r->en_symbols_bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->en_symbols); return 3; }
        if (file_hash_size(in->de_kstream, &r->de_kstream_hash, &bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->de_kstream); return 3; }
        if (file_hash_size(in->en_kstream, &r->en_kstream_hash, &bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->en_kstream); return 3; }
        if (file_hash_size(in->de_kdna, &r->de_kdna_hash, &bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->de_kdna); return 3; }
        if (file_hash_size(in->en_kdna, &r->en_kdna_hash, &bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->en_kdna); return 3; }
        if (file_hash_size(in->de_kgram, &r->de_kgram_hash, &r->de_kgram_bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->de_kgram); return 3; }
        if (file_hash_size(in->en_kgram, &r->en_kgram_hash, &r->en_kgram_bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->en_kgram); return 3; }
        if (file_hash_size(in->matrix_csv, &r->matrix_csv_hash, &r->matrix_csv_bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->matrix_csv); return 3; }
        if (file_hash_size(in->null_csv, &r->null_csv_hash, &r->null_csv_bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->null_csv); return 3; }
        if (file_hash_size(in->report_html, &r->report_html_hash, &r->report_html_bytes) != 0) { fprintf(stderr, "cannot hash %s\n", in->report_html); return 3; }

        double so=0, co=0, sd=0, cd=0;
        if (parse_observed_csv(in->observed_csv, in->name, cells, &cell_count, &so, &co, &sd, &cd) != 0) {
            fprintf(stderr, "cannot parse observed_vs_null: %s\n", in->observed_csv);
            return 3;
        }
        r->avg_self_observed_accuracy = so;
        r->avg_cross_observed_accuracy = co;
        r->avg_self_observed_minus_null = sd;
        r->avg_cross_observed_minus_null = cd;

        sum_self_obs += so; sum_cross_obs += co; sum_self_delta += sd; sum_cross_delta += cd;
        ++metric_configs;

        copy_str(r->de_symbols_path, sizeof(r->de_symbols_path), in->de_symbols);
        copy_str(r->en_symbols_path, sizeof(r->en_symbols_path), in->en_symbols);
        copy_str(r->de_kgram_path, sizeof(r->de_kgram_path), in->de_kgram);
        copy_str(r->en_kgram_path, sizeof(r->en_kgram_path), in->en_kgram);
        copy_str(r->matrix_csv_path, sizeof(r->matrix_csv_path), in->matrix_csv);
        copy_str(r->null_csv_path, sizeof(r->null_csv_path), in->null_csv);
        copy_str(r->report_html_path, sizeof(r->report_html_path), in->report_html);

        h.total_de_symbols += r->de_symbols;
        h.total_en_symbols += r->en_symbols;
        h.total_de_kgram_rules += r->de_kgram_rules;
        h.total_en_kgram_rules += r->en_kgram_rules;
        h.total_artifact_bytes += r->de_symbols_bytes + r->en_symbols_bytes + r->de_kgram_bytes + r->en_kgram_bytes + r->matrix_csv_bytes + r->null_csv_bytes + r->report_html_bytes;
    }

    h.cell_count = cell_count;
    h.config_payload_bytes = (uint64_t)input_count * KDNA_KLLIB_CONFIG_RECORD_BYTES;
    h.cell_payload_bytes = (uint64_t)cell_count * KDNA_KLLIB_CELL_RECORD_BYTES;
    if (metric_configs) {
        h.avg_self_observed_accuracy = sum_self_obs / (double)metric_configs;
        h.avg_cross_observed_accuracy = sum_cross_obs / (double)metric_configs;
        h.avg_self_observed_minus_null = sum_self_delta / (double)metric_configs;
        h.avg_cross_observed_minus_null = sum_cross_delta / (double)metric_configs;
    }

    uint64_t lh = 1469598103934665603ull;
    lh = fnv1a64_update(lh, configs, (size_t)h.config_payload_bytes);
    lh = fnv1a64_update(lh, cells, (size_t)h.cell_payload_bytes);
    h.library_hash = lh;

    FILE* out = fopen(out_path, "wb");
    if (!out) { fprintf(stderr, "cannot write %s\n", out_path); return 4; }
    if (fwrite(&h, 1, sizeof(h), out) != sizeof(h) ||
        fwrite(configs, KDNA_KLLIB_CONFIG_RECORD_BYTES, input_count, out) != input_count ||
        fwrite(cells, KDNA_KLLIB_CELL_RECORD_BYTES, cell_count, out) != cell_count) {
        fprintf(stderr, "write failed %s\n", out_path);
        fclose(out); return 4;
    }
    fclose(out);

    if (csv_path) {
        FILE* csv = fopen(csv_path, "wb");
        if (!csv) { fprintf(stderr, "cannot write %s\n", csv_path); return 4; }
        fprintf(csv, "config,row,col,class,observed_accuracy,baseline_accuracy,lift,surprise_rate,out_of_grammar,grammar_edges,avg_null_accuracy,observed_minus_null,null_modes\n");
        for (uint32_t i=0; i<cell_count; ++i) {
            kdna_kllib_cell_record* c = &cells[i];
            fprintf(csv, "%s,%s,%s,%s,%.17g,%.17g,%.17g,%.17g,%" PRIu64 ",%" PRIu64 ",%.17g,%.17g,%s\n",
                    c->config, c->row, c->col, c->cell_class, c->observed_accuracy, c->baseline_accuracy, c->lift, c->surprise_rate,
                    c->out_of_grammar, c->grammar_edges, c->avg_null_accuracy, c->observed_minus_null, c->null_modes);
        }
        fclose(csv);
    }

    if (json_path) {
        FILE* jf = fopen(json_path, "wb");
        if (!jf) { fprintf(stderr, "cannot write %s\n", json_path); return 4; }
        fprintf(jf, "{\n  \"magic\":\"KLLIB001\",\n  \"version\":1,\n  \"configs\":%u,\n  \"cells\":%u,\n", input_count, cell_count);
        fprintf(jf, "  \"total_de_symbols\":%" PRIu64 ",\n  \"total_en_symbols\":%" PRIu64 ",\n", h.total_de_symbols, h.total_en_symbols);
        fprintf(jf, "  \"total_de_kgram_rules\":%" PRIu64 ",\n  \"total_en_kgram_rules\":%" PRIu64 ",\n", h.total_de_kgram_rules, h.total_en_kgram_rules);
        fprintf(jf, "  \"avg_self_observed_accuracy\":%.17g,\n  \"avg_cross_observed_accuracy\":%.17g,\n", h.avg_self_observed_accuracy, h.avg_cross_observed_accuracy);
        fprintf(jf, "  \"avg_self_observed_minus_null\":%.17g,\n  \"avg_cross_observed_minus_null\":%.17g,\n", h.avg_self_observed_minus_null, h.avg_cross_observed_minus_null);
        fprintf(jf, "  \"config_records\":[\n");
        for (uint32_t i=0; i<input_count; ++i) {
            kdna_kllib_config_record* r = &configs[i];
            fprintf(jf, "    {\"name\":"); json_escape(jf, r->name);
            fprintf(jf, ",\"field\":"); json_escape(jf, r->field);
            fprintf(jf, ",\"window\":%u,\"symbol_bits\":%u,\"de_symbols\":%" PRIu64 ",\"en_symbols\":%" PRIu64 ",\"de_kgram_rules\":%" PRIu64 ",\"en_kgram_rules\":%" PRIu64 ",\"avg_cross_observed_accuracy\":%.17g,\"avg_cross_observed_minus_null\":%.17g}",
                    r->window, r->symbol_bits, r->de_symbols, r->en_symbols, r->de_kgram_rules, r->en_kgram_rules, r->avg_cross_observed_accuracy, r->avg_cross_observed_minus_null);
            fprintf(jf, "%s\n", (i + 1u < input_count) ? "," : "");
        }
        fprintf(jf, "  ],\n  \"cells\":[\n");
        for (uint32_t i=0; i<cell_count; ++i) {
            kdna_kllib_cell_record* c = &cells[i];
            fprintf(jf, "    {\"config\":"); json_escape(jf, c->config);
            fprintf(jf, ",\"row\":"); json_escape(jf, c->row);
            fprintf(jf, ",\"col\":"); json_escape(jf, c->col);
            fprintf(jf, ",\"class\":"); json_escape(jf, c->cell_class);
            fprintf(jf, ",\"observed_accuracy\":%.17g,\"lift\":%.17g,\"avg_null_accuracy\":%.17g,\"observed_minus_null\":%.17g}",
                    c->observed_accuracy, c->lift, c->avg_null_accuracy, c->observed_minus_null);
            fprintf(jf, "%s\n", (i + 1u < cell_count) ? "," : "");
        }
        fprintf(jf, "  ]\n}\n");
        fclose(jf);
    }

    printf("kdna_klang_library: wrote %s configs=%u cells=%u total_artifact_bytes=%" PRIu64 " hash=0x%016" PRIx64 "\n",
           out_path, input_count, cell_count, h.total_artifact_bytes, h.library_hash);
    if (json_path) printf("json=%s\n", json_path);
    if (csv_path) printf("csv=%s\n", csv_path);
    return 0;
}
