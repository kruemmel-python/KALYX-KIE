#ifndef KALYX_HASH_H
#define KALYX_HASH_H

#include <stddef.h>
#include <stdint.h>
#include "kalyx_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KALYX_SHA256_HEX_BYTES 65u

typedef struct KalyxSha256 {
    uint32_t state[8];
    uint64_t bit_len;
    unsigned char block[64];
    size_t block_len;
} KalyxSha256;

void kalyx_sha256_init(KalyxSha256 *ctx);
void kalyx_sha256_update(KalyxSha256 *ctx, const void *data, size_t len);
void kalyx_sha256_final(KalyxSha256 *ctx, unsigned char out[32]);
void kalyx_sha256_hex(const unsigned char digest[32], char out_hex[KALYX_SHA256_HEX_BYTES]);
KalyxStatus kalyx_sha256_file_hex(const char *path, char out_hex[KALYX_SHA256_HEX_BYTES]);
KalyxStatus kalyx_sha256_text_hex(const char *text, char out_hex[KALYX_SHA256_HEX_BYTES]);

#ifdef __cplusplus
}
#endif

#endif
