#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t fnv1a_file(const char *path, uint64_t *size_out) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0u;
    uint64_t h = 1469598103934665603ull;
    uint64_t n = 0u;
    unsigned char buf[8192];
    for (;;) {
        size_t got = fread(buf, 1u, sizeof(buf), f);
        for (size_t i = 0; i < got; ++i) { h ^= (uint64_t)buf[i]; h *= 1099511628211ull; }
        n += (uint64_t)got;
        if (got < sizeof(buf)) break;
    }
    fclose(f);
    if (size_out) *size_out = n;
    return h;
}

static void json_escape(FILE *f, const char *s) {
    fputc('"', f);
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { fputc('\\', f); fputc(c, f); }
        else if (c == '\n') fputs("\\n", f);
        else if (c == '\r') fputs("\\r", f);
        else if (c == '\t') fputs("\\t", f);
        else if (c < 32u) fprintf(f, "\\u%04x", (unsigned)c);
        else fputc(c, f);
    }
    fputc('"', f);
}

int main(int argc, char **argv) {
    const char *out = NULL;
    int first_file = 1;
    if (argc < 2) {
        fprintf(stderr, "usage: kdna_manifest --out manifest.json <artifact> [...]\n");
        return 2;
    }
    FILE *fout = stdout;
    int start = 1;
    if (argc >= 4 && strcmp(argv[1], "--out") == 0) {
        out = argv[2];
        start = 3;
        fout = fopen(out, "wb");
        if (!fout) { fprintf(stderr, "cannot write manifest\n"); return 2; }
    }
    time_t now = time(NULL);
    fprintf(fout, "{\n");
    fprintf(fout, "  \"magic\":\"KMANIFEST1\",\n");
    fprintf(fout, "  \"version\":1,\n");
    fprintf(fout, "  \"theory\":\"KDNA/SUBQG\",\n");
    fprintf(fout, "  \"created_unix\":%lld,\n", (long long)now);
    fprintf(fout, "  \"hash\":\"fnv1a64\",\n");
    fprintf(fout, "  \"artifacts\":[\n");
    for (int i = start; i < argc; ++i) {
        uint64_t size = 0u;
        uint64_t h = fnv1a_file(argv[i], &size);
        if (!first_file) fprintf(fout, ",\n");
        first_file = 0;
        fprintf(fout, "    {\"path\":");
        json_escape(fout, argv[i]);
        fprintf(fout, ",\"size\":%llu,\"fnv1a64\":\"0x%016llx\"}", (unsigned long long)size, (unsigned long long)h);
    }
    fprintf(fout, "\n  ]\n}\n");
    if (fout != stdout) fclose(fout);
    if (out) printf("kdna_manifest: wrote %s artifacts=%d\n", out, argc - start);
    return 0;
}
