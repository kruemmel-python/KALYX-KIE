#include "kalyx_common.h"

#include <stdio.h>

int main(int argc, char **argv) {
    KalyxBuffer b;
    KalyxStatus st;
    if (argc != 2 || (argc == 2 && argv[1][0] == '-' && argv[1][1] == '-')) {
        puts("kalyx_audit_print AUDIT.kaudit.json");
        return argc == 2 ? 0 : 2;
    }
    st = kalyx_read_text_file(argv[1], &b);
    if (st != KALYX_OK) {
        fprintf(stderr, "cannot read audit: %s\n", argv[1]);
        return 3;
    }
    fwrite(b.data, 1u, b.size, stdout);
    if (b.size == 0u || b.data[b.size - 1u] != '\n') putchar('\n');
    kalyx_buffer_free(&b);
    return 0;
}
