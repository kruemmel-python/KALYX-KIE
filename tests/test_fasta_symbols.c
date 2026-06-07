
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int read_u64(const char *path, uint64_t *buf, size_t n) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t got = fread(buf, sizeof(uint64_t), n, f);
    fclose(f);
    return got == n;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    uint64_t v[4] = {0,0,0,0};
    if (!read_u64(argv[1], v, 4)) return 3;
    /* ACGTAC with K=3:
       ACG = 0b000110 = 6
       CGT = 0b011011 = 27
       GTA = 0b101100 = 44
       TAC = 0b110001 = 49 */
    if (v[0] != 6u || v[1] != 27u || v[2] != 44u || v[3] != 49u) {
        fprintf(stderr, "unexpected symbols: %llu %llu %llu %llu\n",
                (unsigned long long)v[0], (unsigned long long)v[1],
                (unsigned long long)v[2], (unsigned long long)v[3]);
        return 4;
    }
    return 0;
}
