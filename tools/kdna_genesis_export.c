#include "kdna_kgen.h"
#include "kdna_ksoa.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_kgen_payload(const char *path, kdna_kgen_header *h, double **payload) {
    *payload = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) { fclose(f); return KDNA_EIO; }
    int rc = kdna_kgen_validate_header(h);
    if (rc != KDNA_OK) { fclose(f); return rc; }
    double *p = (double *)malloc((size_t)h->payload_bytes);
    if (!p) { fclose(f); return KDNA_ENOMEM; }
    if (fread(p, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) {
        free(p); fclose(f); return KDNA_EIO;
    }
    if (fclose(f) != 0) { free(p); return KDNA_EIO; }
    *payload = p;
    return KDNA_OK;
}

static void usage(void) {
    fprintf(stderr, "kdna_genesis_export --kgen genesis.kgen --out signature.ksoa\n");
}

int main(int argc, char **argv) {
    const char *kgen_path = NULL;
    const char *out_path = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--kgen") == 0 && i + 1 < argc) kgen_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else { usage(); return 2; }
    }
    if (!kgen_path || !out_path) { usage(); return 2; }

    kdna_kgen_header gh;
    double *gp = NULL;
    int rc = read_kgen_payload(kgen_path, &gh, &gp);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_genesis_export: cannot read '%s': %s\n", kgen_path, kdna_status_str(rc));
        return 3;
    }
    const size_t n = (size_t)gh.n;
    uint64_t payload_bytes = 0u;
    rc = kdna_ksoa_payload_bytes(n, &payload_bytes);
    if (rc != KDNA_OK) { free(gp); return 3; }
    double *sp = (double *)calloc(1u, (size_t)payload_bytes);
    if (!sp) { free(gp); return 3; }

    for (size_t i = 0; i < n; ++i) {
        sp[kdna_idx(KDNA_X, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_X, n, i)];
        sp[kdna_idx(KDNA_K01, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_K01, n, i)];
        sp[kdna_idx(KDNA_K02, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_K02, n, i)];
        sp[kdna_idx(KDNA_K03, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_K03, n, i)];
        sp[kdna_idx(KDNA_K04, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_K04, n, i)];
        sp[kdna_idx(KDNA_K05, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_K05, n, i)];
        sp[kdna_idx(KDNA_EK, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_EK, n, i)];
        sp[kdna_idx(KDNA_AK, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_AK, n, i)];
        sp[kdna_idx(KDNA_LK, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_LOCK, n, i)];
        sp[kdna_idx(KDNA_RAW, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_RAW, n, i)];
        sp[kdna_idx(KDNA_DOM, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_DOM, n, i)];
        sp[kdna_idx(KDNA_S01, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_S01, n, i)];
        sp[kdna_idx(KDNA_S02, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_S02, n, i)];
        sp[kdna_idx(KDNA_S03, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_S03, n, i)];
        sp[kdna_idx(KDNA_S04, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_S04, n, i)];
        sp[kdna_idx(KDNA_S05, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_S05, n, i)];
        sp[kdna_idx(KDNA_DOM_SCORE, n, i)] = gp[kdna_kgen_idx(KDNA_KGEN_DOM_SCORE, n, i)];
    }

    kdna_ksoa_header sh;
    rc = kdna_ksoa_init_header(&sh, n, gh.x_min, gh.x_max, gh.dx, KDNA_KSOA_BACKEND_CPU);
    if (rc == KDNA_OK) rc = kdna_ksoa_write_file(out_path, &sh, sp);
    if (rc != KDNA_OK) {
        fprintf(stderr, "kdna_genesis_export: cannot write '%s': %s\n", out_path, kdna_status_str(rc));
        free(gp); free(sp); return 3;
    }
    printf("kdna_genesis_export: wrote %s source=%s n=%zu fields=%u payload_bytes=%llu\n",
           out_path, kgen_path, n, (unsigned)KDNA_FIELDS, (unsigned long long)sh.payload_bytes);
    free(gp); free(sp);
    return 0;
}
