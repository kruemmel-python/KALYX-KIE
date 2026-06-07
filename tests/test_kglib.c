#include "kdna_kglib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    if (sizeof(kdna_kglib_header) != KDNA_KGLIB_HEADER_BYTES) {
        fprintf(stderr, "bad KGLIB header size\n");
        return 1;
    }
    if (sizeof(kdna_kglib_record) != KDNA_KGLIB_RECORD_BYTES) {
        fprintf(stderr, "bad KGLIB record size\n");
        return 1;
    }
    kdna_kglib_header h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, KDNA_KGLIB_MAGIC, 8u);
    h.version = KDNA_KGLIB_VERSION;
    h.header_bytes = KDNA_KGLIB_HEADER_BYTES;
    h.record_bytes = KDNA_KGLIB_RECORD_BYTES;
    h.chrom_count = 24u;
    if (memcmp(h.magic, "KGLIB001", 8u) != 0) return 1;
    printf("kglib_abi_ok header=%u record=%u\n", KDNA_KGLIB_HEADER_BYTES, KDNA_KGLIB_RECORD_BYTES);
    return 0;
}
