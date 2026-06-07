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
    if (argc < 3) return 2;
    const char *mode = argv[1];
    uint64_t v[8] = {0};

    if (mode[0] == 'b') {
        /*
           Byte adapter invariant:
             each byte in the input file becomes exactly one uint64 symbol.

           The CMake test input is written as "ABC\n". On Windows generators,
           newline materialization can differ. The stable ABI invariant is the
           byte mapping itself, therefore the first three bytes must be A/B/C,
           and the line ending byte may be LF or CR. If CMake produces CRLF,
           the adapter may emit a fifth symbol for LF; this test intentionally
           validates only the first four symbols because the adapter's count
           and header are checked by the probe test.
        */
        if (!read_u64(argv[2], v, 4)) return 3;
        if (v[0] != 65u || v[1] != 66u || v[2] != 67u || !(v[3] == 10u || v[3] == 13u)) {
            fprintf(stderr,
                    "unexpected byte symbols: %llu %llu %llu %llu\n",
                    (unsigned long long)v[0],
                    (unsigned long long)v[1],
                    (unsigned long long)v[2],
                    (unsigned long long)v[3]);
            return 4;
        }
    } else if (mode[0] == 'c') {
        if (!read_u64(argv[2], v, 4)) return 5;
        if (v[0] != 0u || v[1] != 3u || v[2] != 7u || v[3] != 10u) {
            fprintf(stderr,
                    "unexpected csv symbols: %llu %llu %llu %llu\n",
                    (unsigned long long)v[0],
                    (unsigned long long)v[1],
                    (unsigned long long)v[2],
                    (unsigned long long)v[3]);
            return 6;
        }
    } else if (mode[0] == 't') {
        if (!read_u64(argv[2], v, 3)) return 7;
        if (v[0] == 0u || v[1] == 0u || v[2] == 0u) {
            fprintf(stderr,
                    "unexpected token symbols: %llu %llu %llu\n",
                    (unsigned long long)v[0],
                    (unsigned long long)v[1],
                    (unsigned long long)v[2]);
            return 8;
        }

    } else if (mode[0] == 'l') {
        if (!read_u64(argv[2], v, 5)) return 10;
        /*
          KLANG/CoNLL-U invariant v1.1:
            linguistic atoms are hashed/folded into a non-zero KDNA-compatible
            symbol space. We intentionally do not assert small UPOS codes here;
            small integer UPOS codes caused degenerate KGRAMs under the standard
            kdna_project source range.
        */
        for (int i = 0; i < 5; ++i) {
            if (v[i] == 0u || v[i] > 0xffffffffULL) {
                fprintf(stderr,
                        "unexpected conllu symbol[%d]: %llu\n",
                        i, (unsigned long long)v[i]);
                return 11;
            }
        }
        if (v[0] == v[1] || v[1] == v[2] || v[2] == v[3]) {
            fprintf(stderr, "conllu symbols unexpectedly collapsed\n");
            return 12;
        }
    } else {
        return 9;
    }

    return 0;
}
