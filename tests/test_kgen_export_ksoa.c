#include "kdna_ksoa.h"
#include "kdna.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: test_kgen_export_ksoa <file.ksoa> <expected_n>\n");
        return 2;
    }
    kdna_ksoa_header h;
    int rc = kdna_ksoa_read_header_file(argv[1], &h);
    if (rc != KDNA_OK) {
        fprintf(stderr, "header invalid: %s\n", kdna_status_str(rc));
        return 1;
    }
    const unsigned long long expected = strtoull(argv[2], NULL, 10);
    if (h.n != (uint64_t)expected || h.fields != KDNA_FIELDS || h.backend != KDNA_KSOA_BACKEND_CPU) {
        fprintf(stderr, "bad header n=%llu fields=%u backend=%u\n", (unsigned long long)h.n, h.fields, h.backend);
        return 1;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) return 1;
    if (fseek(f, (long)h.header_bytes, SEEK_SET) != 0) { fclose(f); return 1; }
    const size_t cells = (size_t)h.n * (size_t)KDNA_FIELDS;
    double *p = (double *)calloc(cells, sizeof(double));
    if (!p) { fclose(f); return 1; }
    if (fread(p, sizeof(double), cells, f) != cells) { free(p); fclose(f); return 1; }
    fclose(f);
    for (size_t i = 0; i < cells; ++i) {
        if (!isfinite(p[i])) {
            fprintf(stderr, "nonfinite at %zu\n", i);
            free(p); return 1;
        }
    }
    for (size_t i = 0; i < (size_t)h.n; ++i) {
        const double raw = p[kdna_idx(KDNA_RAW, (size_t)h.n, i)];
        const double dom = p[kdna_idx(KDNA_DOM, (size_t)h.n, i)];
        if (raw < 1.0 || raw > 5.0 || dom < 1.0 || dom > 5.0) {
            fprintf(stderr, "bad RAW/D at %zu\n", i);
            free(p); return 1;
        }
    }
    printf("kgen_export_ksoa_ok n=%llu\n", (unsigned long long)h.n);
    free(p);
    return 0;
}
