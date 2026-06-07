#include "kdna.h"
#include "kdna_kgram.h"
#include "kdna_kgenome.h"
#include "kdna_kglib.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct kfsum_header_local {
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
} kfsum_header_local;

typedef struct entry_arg {
    char name[32];
    char raw[260];
    char kfsum[260];
    char kdna[260];
    char kgram[260];
} entry_arg;

static const char *human24[] = {
    "chr1","chr2","chr3","chr4","chr5","chr6",
    "chr7","chr8","chr9","chr10","chr11","chr12",
    "chr13","chr14","chr15","chr16","chr17","chr18",
    "chr19","chr20","chr21","chr22","chrX","chrY"
};

static void usage(void) {
    printf("kdna_genome_library — KGENOME Enterprise artifact index\n");
    printf("create from existing files, no recomputation:\n");
    printf("  kdna_genome_library --dir GenomeOutFull --human24 --out library.kglib --json library.json --csv library.csv [--matrix file.kgenome] [--matrix-csv file.csv]\n");
    printf("  kdna_genome_library --entry NAME raw.u64 summary.kfsum kdna.u64 grammar.kgram [--entry ...] --out library.kglib --json library.json --csv library.csv\n");
    printf("inspect:\n");
    printf("  kdna_genome_library --a library.kglib\n");
}

static void copystr(char *dst, size_t n, const char *src) {
    if (!dst || n == 0) return;
    memset(dst, 0, n);
    if (src) {
#ifdef _MSC_VER
        strncpy_s(dst, n, src, _TRUNCATE);
#else
        strncpy(dst, src, n - 1u);
#endif
    }
}

static int is_dash(const char *s) {
    return s && strcmp(s, "-") == 0;
}

static uint64_t fnv1a_file(const char *path, uint64_t *bytes_out, int *present_out) {
    uint64_t h = 1469598103934665603ull;
    unsigned char buf[1u << 16];
    uint64_t bytes = 0u;
    FILE *f;
    if (bytes_out) *bytes_out = 0u;
    if (present_out) *present_out = 0;
    if (!path || is_dash(path)) return 0u;
    f = fopen(path, "rb");
    if (!f) return 0u;
    if (present_out) *present_out = 1;
    for (;;) {
        size_t got = fread(buf, 1u, sizeof(buf), f);
        for (size_t i = 0u; i < got; ++i) {
            h ^= (uint64_t)buf[i];
            h *= 1099511628211ull;
        }
        bytes += (uint64_t)got;
        if (got < sizeof(buf)) {
            if (ferror(f)) { fclose(f); return 0u; }
            break;
        }
    }
    fclose(f);
    if (bytes_out) *bytes_out = bytes;
    return h;
}

static int read_kfsum(const char *path, kfsum_header_local *h) {
    FILE *f;
    if (!path || is_dash(path) || !h) return 0;
    memset(h, 0, sizeof(*h));
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return 0; }
    fclose(f);
    return memcmp(h->magic, "KFSUM001", 8u) == 0 && h->version == 1u && h->header_bytes == 128u;
}

static int read_kgenome_header(const char *path, kdna_kgenome_header *h) {
    FILE *f;
    if (!path || is_dash(path) || !h) return 0;
    memset(h, 0, sizeof(*h));
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return 0; }
    fclose(f);
    return memcmp(h->magic, KDNA_KGENOME_MAGIC, 8u) == 0 &&
           h->version == KDNA_KGENOME_VERSION &&
           h->header_bytes == KDNA_KGENOME_HEADER_BYTES &&
           h->record_bytes == KDNA_KGENOME_RECORD_BYTES;
}

static void build_paths(entry_arg *e, const char *dir, const char *name) {
    char tmp[260];
    copystr(e->name, sizeof(e->name), name);
#ifdef _MSC_VER
#define SNPRINTF _snprintf_s
    SNPRINTF(e->raw, sizeof(e->raw), _TRUNCATE, "%s\\%s_k16.u64", dir, name);
    SNPRINTF(e->kfsum, sizeof(e->kfsum), _TRUNCATE, "%s\\%s_k16.kfsum", dir, name);
    SNPRINTF(e->kdna, sizeof(e->kdna), _TRUNCATE, "%s\\%s_k16_kdna.u64", dir, name);
    SNPRINTF(e->kgram, sizeof(e->kgram), _TRUNCATE, "%s\\%s_k16_self.kgram", dir, name);
#undef SNPRINTF
#else
    snprintf(e->raw, sizeof(e->raw), "%s/%s_k16.u64", dir, name);
    snprintf(e->kfsum, sizeof(e->kfsum), "%s/%s_k16.kfsum", dir, name);
    snprintf(e->kdna, sizeof(e->kdna), "%s/%s_k16_kdna.u64", dir, name);
    snprintf(e->kgram, sizeof(e->kgram), "%s/%s_k16_self.kgram", dir, name);
#endif
    (void)tmp;
}

static void fill_record(kdna_kglib_record *r, const entry_arg *e) {
    int present = 0;
    kfsum_header_local ks;
    kdna_kgram_header gh;

    memset(r, 0, sizeof(*r));
    copystr(r->name, sizeof(r->name), e->name);
    copystr(r->raw_path, sizeof(r->raw_path), e->raw);
    copystr(r->kfsum_path, sizeof(r->kfsum_path), e->kfsum);
    copystr(r->kdna_path, sizeof(r->kdna_path), e->kdna);
    copystr(r->kgram_path, sizeof(r->kgram_path), e->kgram);

    r->raw_hash = fnv1a_file(e->raw, &r->raw_bytes, &present);
    if (present) {
        r->status_flags |= KDNA_KGLIB_REC_RAW_PRESENT;
        if (r->raw_bytes % 8u == 0u) r->raw_symbols = r->raw_bytes / 8u;
    }

    r->kfsum_hash = fnv1a_file(e->kfsum, &r->kfsum_bytes, &present);
    if (present && read_kfsum(e->kfsum, &ks)) {
        r->status_flags |= KDNA_KGLIB_REC_KFSUM_PRESENT;
        r->kfsum_symbols = ks.symbols_written;
        r->kfsum_bases_total = ks.bases_total;
        r->kfsum_bases_valid = ks.bases_valid;
        r->kfsum_bases_invalid = ks.bases_invalid;
        r->kfsum_resets = ks.reset_count;
        r->kfsum_contigs = ks.contig_count;
        if (r->raw_symbols == r->kfsum_symbols && ks.payload_bytes == r->raw_bytes) {
            r->status_flags |= KDNA_KGLIB_REC_RAW_KFSUM_MATCH;
        }
    }

    r->kdna_hash = fnv1a_file(e->kdna, &r->kdna_bytes, &present);
    if (present) {
        r->status_flags |= KDNA_KGLIB_REC_KDNA_PRESENT;
        if (r->kdna_bytes % 8u == 0u) r->kdna_symbols = r->kdna_bytes / 8u;
        if (r->raw_symbols != 0u && r->raw_symbols == r->kdna_symbols) {
            r->status_flags |= KDNA_KGLIB_REC_RAW_KDNA_MATCH;
        }
    }

    r->kgram_hash = fnv1a_file(e->kgram, &r->kgram_bytes, &present);
    if (present) {
        r->status_flags |= KDNA_KGLIB_REC_KGRAM_PRESENT;
        if (kdna_kgram_read_header_file(e->kgram, &gh) == KDNA_OK) {
            r->status_flags |= KDNA_KGLIB_REC_KGRAM_VALID;
            r->kgram_rules = gh.rule_count;
            r->kgram_source_n = gh.source_n;
            r->kgram_payload_bytes = gh.payload_bytes;
        }
    }

    if ((r->status_flags & (KDNA_KGLIB_REC_RAW_PRESENT | KDNA_KGLIB_REC_KFSUM_PRESENT |
                            KDNA_KGLIB_REC_KDNA_PRESENT | KDNA_KGLIB_REC_KGRAM_PRESENT |
                            KDNA_KGLIB_REC_RAW_KFSUM_MATCH | KDNA_KGLIB_REC_RAW_KDNA_MATCH |
                            KDNA_KGLIB_REC_KGRAM_VALID)) ==
        (KDNA_KGLIB_REC_RAW_PRESENT | KDNA_KGLIB_REC_KFSUM_PRESENT |
         KDNA_KGLIB_REC_KDNA_PRESENT | KDNA_KGLIB_REC_KGRAM_PRESENT |
         KDNA_KGLIB_REC_RAW_KFSUM_MATCH | KDNA_KGLIB_REC_RAW_KDNA_MATCH |
         KDNA_KGLIB_REC_KGRAM_VALID)) {
        r->status_flags |= KDNA_KGLIB_REC_COMPLETE;
    }
}

static int write_kglib(const char *path, const kdna_kglib_header *h, const kdna_kglib_record *r) {
    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return KDNA_EIO; }
    if (h->chrom_count && fwrite(r, sizeof(kdna_kglib_record), h->chrom_count, f) != h->chrom_count) {
        fclose(f); return KDNA_EIO;
    }
    if (fclose(f) != 0) return KDNA_EIO;
    return KDNA_OK;
}

static int write_csv(const char *path, const kdna_kglib_record *r, size_t n) {
    FILE *f = fopen(path, "w");
    if (!f) return KDNA_EIO;
    fprintf(f, "name,status_flags,raw_symbols,kdna_symbols,kfsum_symbols,kgram_rules,kgram_source_n,raw_bytes,kdna_bytes,kgram_bytes,raw_hash,kfsum_hash,kdna_hash,kgram_hash,bases_total,bases_valid,bases_invalid,resets,contigs\n");
    for (size_t i = 0u; i < n; ++i) {
        fprintf(f, "%s,0x%llx,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,0x%016llx,0x%016llx,0x%016llx,0x%016llx,%llu,%llu,%llu,%llu,%llu\n",
                r[i].name,
                (unsigned long long)r[i].status_flags,
                (unsigned long long)r[i].raw_symbols,
                (unsigned long long)r[i].kdna_symbols,
                (unsigned long long)r[i].kfsum_symbols,
                (unsigned long long)r[i].kgram_rules,
                (unsigned long long)r[i].kgram_source_n,
                (unsigned long long)r[i].raw_bytes,
                (unsigned long long)r[i].kdna_bytes,
                (unsigned long long)r[i].kgram_bytes,
                (unsigned long long)r[i].raw_hash,
                (unsigned long long)r[i].kfsum_hash,
                (unsigned long long)r[i].kdna_hash,
                (unsigned long long)r[i].kgram_hash,
                (unsigned long long)r[i].kfsum_bases_total,
                (unsigned long long)r[i].kfsum_bases_valid,
                (unsigned long long)r[i].kfsum_bases_invalid,
                (unsigned long long)r[i].kfsum_resets,
                (unsigned long long)r[i].kfsum_contigs);
    }
    if (fclose(f) != 0) return KDNA_EIO;
    return KDNA_OK;
}

static int write_json(const char *path, const kdna_kglib_header *h, const kdna_kglib_record *r,
                      const char *matrix_path, const char *matrix_csv_path) {
    FILE *f = fopen(path, "w");
    if (!f) return KDNA_EIO;
    fprintf(f, "{\n");
    fprintf(f, "  \"magic\":\"KGLIB001\",\n");
    fprintf(f, "  \"version\":1,\n");
    fprintf(f, "  \"chrom_count\":%u,\n", h->chrom_count);
    fprintf(f, "  \"flags\":\"0x%llx\",\n", (unsigned long long)h->flags);
    fprintf(f, "  \"total_raw_symbols\":%llu,\n", (unsigned long long)h->total_raw_symbols);
    fprintf(f, "  \"total_kdna_symbols\":%llu,\n", (unsigned long long)h->total_kdna_symbols);
    fprintf(f, "  \"total_kgram_rules\":%llu,\n", (unsigned long long)h->total_kgram_rules);
    fprintf(f, "  \"total_artifact_bytes\":%llu,\n", (unsigned long long)h->total_artifact_bytes);
    fprintf(f, "  \"matrix\":{\"path\":\"%s\",\"hash\":\"0x%016llx\",\"present\":%s},\n",
            matrix_path ? matrix_path : "",
            (unsigned long long)h->matrix_hash,
            (h->flags & KDNA_KGLIB_FLAG_MATRIX_PRESENT) ? "true" : "false");
    fprintf(f, "  \"matrix_csv\":{\"path\":\"%s\",\"hash\":\"0x%016llx\",\"present\":%s},\n",
            matrix_csv_path ? matrix_csv_path : "",
            (unsigned long long)h->matrix_csv_hash,
            (h->flags & KDNA_KGLIB_FLAG_MATRIX_CSV_PRESENT) ? "true" : "false");
    fprintf(f, "  \"chromosomes\":[\n");
    for (uint32_t i = 0u; i < h->chrom_count; ++i) {
        fprintf(f, "    {\"name\":\"%s\",\"complete\":%s,\"status\":\"0x%llx\","
                   "\"raw_symbols\":%llu,\"kdna_symbols\":%llu,\"kfsum_symbols\":%llu,"
                   "\"kgram_rules\":%llu,\"kgram_source_n\":%llu,"
                   "\"bases_total\":%llu,\"bases_valid\":%llu,\"bases_invalid\":%llu,"
                   "\"raw_hash\":\"0x%016llx\",\"kdna_hash\":\"0x%016llx\",\"kgram_hash\":\"0x%016llx\"}%s\n",
                r[i].name,
                (r[i].status_flags & KDNA_KGLIB_REC_COMPLETE) ? "true" : "false",
                (unsigned long long)r[i].status_flags,
                (unsigned long long)r[i].raw_symbols,
                (unsigned long long)r[i].kdna_symbols,
                (unsigned long long)r[i].kfsum_symbols,
                (unsigned long long)r[i].kgram_rules,
                (unsigned long long)r[i].kgram_source_n,
                (unsigned long long)r[i].kfsum_bases_total,
                (unsigned long long)r[i].kfsum_bases_valid,
                (unsigned long long)r[i].kfsum_bases_invalid,
                (unsigned long long)r[i].raw_hash,
                (unsigned long long)r[i].kdna_hash,
                (unsigned long long)r[i].kgram_hash,
                (i + 1u == h->chrom_count) ? "" : ",");
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    if (fclose(f) != 0) return KDNA_EIO;
    return KDNA_OK;
}

static int inspect_kglib(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return 2; }
    kdna_kglib_header h;
    if (fread(&h, 1u, sizeof(h), f) != sizeof(h)) { fclose(f); return 2; }
    if (memcmp(h.magic, KDNA_KGLIB_MAGIC, 8u) != 0 ||
        h.version != KDNA_KGLIB_VERSION ||
        h.header_bytes != KDNA_KGLIB_HEADER_BYTES ||
        h.record_bytes != KDNA_KGLIB_RECORD_BYTES) {
        fclose(f); fprintf(stderr, "invalid KGLIB\n"); return 2;
    }
    printf("file: KGLIB chroms:%u record_bytes:%u payload:%llu flags:0x%llx raw_symbols:%llu kdna_symbols:%llu kgram_rules:%llu artifact_bytes:%llu matrix_hash:0x%016llx csv_hash:0x%016llx\n",
           h.chrom_count, h.record_bytes, (unsigned long long)h.payload_bytes,
           (unsigned long long)h.flags, (unsigned long long)h.total_raw_symbols,
           (unsigned long long)h.total_kdna_symbols, (unsigned long long)h.total_kgram_rules,
           (unsigned long long)h.total_artifact_bytes,
           (unsigned long long)h.matrix_hash, (unsigned long long)h.matrix_csv_hash);
    kdna_kglib_record *r = (kdna_kglib_record *)calloc(h.chrom_count ? h.chrom_count : 1u, sizeof(kdna_kglib_record));
    if (!r) { fclose(f); return 2; }
    if (fread(r, sizeof(kdna_kglib_record), h.chrom_count, f) != h.chrom_count) { free(r); fclose(f); return 2; }
    fclose(f);
    for (uint32_t i = 0u; i < h.chrom_count; ++i) {
        printf("%s complete:%s raw:%llu kdna:%llu kfsum:%llu rules:%llu source_n:%llu status:0x%llx\n",
               r[i].name,
               (r[i].status_flags & KDNA_KGLIB_REC_COMPLETE) ? "yes" : "no",
               (unsigned long long)r[i].raw_symbols,
               (unsigned long long)r[i].kdna_symbols,
               (unsigned long long)r[i].kfsum_symbols,
               (unsigned long long)r[i].kgram_rules,
               (unsigned long long)r[i].kgram_source_n,
               (unsigned long long)r[i].status_flags);
    }
    free(r);
    return 0;
}

int main(int argc, char **argv) {
    entry_arg entries[128];
    size_t entry_n = 0u;
    const char *dir = NULL;
    const char *out = NULL;
    const char *json = NULL;
    const char *csv = NULL;
    const char *matrix = NULL;
    const char *matrix_csv = NULL;
    int use_human24 = 0;

    if (argc == 3 && strcmp(argv[1], "--a") == 0) return inspect_kglib(argv[2]);
    if (argc < 2) { usage(); return 2; }

    memset(entries, 0, sizeof(entries));
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) dir = argv[++i];
        else if (strcmp(argv[i], "--human24") == 0) use_human24 = 1;
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out = argv[++i];
        else if (strcmp(argv[i], "--json") == 0 && i + 1 < argc) json = argv[++i];
        else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) csv = argv[++i];
        else if (strcmp(argv[i], "--matrix") == 0 && i + 1 < argc) matrix = argv[++i];
        else if (strcmp(argv[i], "--matrix-csv") == 0 && i + 1 < argc) matrix_csv = argv[++i];
        else if (strcmp(argv[i], "--entry") == 0 && i + 5 < argc) {
            if (entry_n >= 128u) { fprintf(stderr, "too many entries\n"); return 2; }
            copystr(entries[entry_n].name, sizeof(entries[entry_n].name), argv[++i]);
            copystr(entries[entry_n].raw, sizeof(entries[entry_n].raw), argv[++i]);
            copystr(entries[entry_n].kfsum, sizeof(entries[entry_n].kfsum), argv[++i]);
            copystr(entries[entry_n].kdna, sizeof(entries[entry_n].kdna), argv[++i]);
            copystr(entries[entry_n].kgram, sizeof(entries[entry_n].kgram), argv[++i]);
            entry_n++;
        } else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (use_human24) {
        if (!dir) { fprintf(stderr, "--human24 requires --dir\n"); return 2; }
        entry_n = 24u;
        for (size_t i = 0u; i < 24u; ++i) build_paths(&entries[i], dir, human24[i]);
    }

    if (!out || !json || !csv || entry_n == 0u) { usage(); return 2; }

    kdna_kglib_record *records = (kdna_kglib_record *)calloc(entry_n, sizeof(kdna_kglib_record));
    if (!records) return 2;
    kdna_kglib_header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, KDNA_KGLIB_MAGIC, 8u);
    h.version = KDNA_KGLIB_VERSION;
    h.header_bytes = KDNA_KGLIB_HEADER_BYTES;
    h.record_bytes = KDNA_KGLIB_RECORD_BYTES;
    h.chrom_count = (uint32_t)entry_n;
    h.payload_bytes = (uint64_t)entry_n * (uint64_t)KDNA_KGLIB_RECORD_BYTES;
    h.flags = KDNA_KGLIB_FLAG_LE;
    if (use_human24 && entry_n == 24u) h.flags |= KDNA_KGLIB_FLAG_HUMAN24;

    for (size_t i = 0u; i < entry_n; ++i) {
        fill_record(&records[i], &entries[i]);
        h.total_raw_symbols += records[i].raw_symbols;
        h.total_kdna_symbols += records[i].kdna_symbols;
        h.total_kgram_rules += records[i].kgram_rules;
        h.total_artifact_bytes += records[i].raw_bytes + records[i].kfsum_bytes + records[i].kdna_bytes + records[i].kgram_bytes;
    }

    int present = 0;
    uint64_t matrix_bytes = 0u, csv_bytes = 0u;
    h.matrix_hash = fnv1a_file(matrix, &matrix_bytes, &present);
    if (present) {
        kdna_kgenome_header mh;
        if (read_kgenome_header(matrix, &mh)) h.flags |= KDNA_KGLIB_FLAG_MATRIX_PRESENT;
        h.total_artifact_bytes += matrix_bytes;
    }
    h.matrix_csv_hash = fnv1a_file(matrix_csv, &csv_bytes, &present);
    if (present) {
        h.flags |= KDNA_KGLIB_FLAG_MATRIX_CSV_PRESENT;
        h.total_artifact_bytes += csv_bytes;
    }

    int complete = 1;
    for (size_t i = 0u; i < entry_n; ++i) {
        if ((records[i].status_flags & KDNA_KGLIB_REC_COMPLETE) == 0u) complete = 0;
    }
    if (complete && (h.flags & KDNA_KGLIB_FLAG_MATRIX_PRESENT) && (h.flags & KDNA_KGLIB_FLAG_MATRIX_CSV_PRESENT)) {
        h.flags |= KDNA_KGLIB_FLAG_COMPLETE;
    }

    int rc = write_kglib(out, &h, records);
    if (rc == KDNA_OK) rc = write_csv(csv, records, entry_n);
    if (rc == KDNA_OK) rc = write_json(json, &h, records, matrix, matrix_csv);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_genome_library: write failed: %s\n", kdna_status_str(rc));
        free(records);
        return 2;
    }

    printf("kdna_genome_library: wrote %s %s %s chroms=%zu complete=%s total_raw_symbols=%llu total_kdna_symbols=%llu total_kgram_rules=%llu artifact_bytes=%llu\n",
           out, json, csv, entry_n, (h.flags & KDNA_KGLIB_FLAG_COMPLETE) ? "yes" : "no",
           (unsigned long long)h.total_raw_symbols,
           (unsigned long long)h.total_kdna_symbols,
           (unsigned long long)h.total_kgram_rules,
           (unsigned long long)h.total_artifact_bytes);
    free(records);
    return 0;
}
