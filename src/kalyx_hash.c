#include "kalyx_hash.h"

#include <stdio.h>
#include <string.h>

static const uint32_t k256[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

static uint32_t rotr32(uint32_t x, unsigned int n) { return (x >> n) | (x << (32u - n)); }
static uint32_t load_be32(const unsigned char *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3]; }
static void store_be32(unsigned char *p, uint32_t v) { p[0]=(unsigned char)(v>>24); p[1]=(unsigned char)(v>>16); p[2]=(unsigned char)(v>>8); p[3]=(unsigned char)v; }

static void sha256_transform(KalyxSha256 *ctx, const unsigned char block[64]) {
    uint32_t w[64];
    uint32_t a,b,c,d,e,f,g,h;
    size_t i;
    for (i = 0u; i < 16u; i++) w[i] = load_be32(block + i * 4u);
    for (i = 16u; i < 64u; i++) {
        const uint32_t s0 = rotr32(w[i-15u], 7u) ^ rotr32(w[i-15u], 18u) ^ (w[i-15u] >> 3u);
        const uint32_t s1 = rotr32(w[i-2u], 17u) ^ rotr32(w[i-2u], 19u) ^ (w[i-2u] >> 10u);
        w[i] = w[i-16u] + s0 + w[i-7u] + s1;
    }
    a=ctx->state[0]; b=ctx->state[1]; c=ctx->state[2]; d=ctx->state[3];
    e=ctx->state[4]; f=ctx->state[5]; g=ctx->state[6]; h=ctx->state[7];
    for (i = 0u; i < 64u; i++) {
        const uint32_t s1 = rotr32(e, 6u) ^ rotr32(e, 11u) ^ rotr32(e, 25u);
        const uint32_t ch = (e & f) ^ ((~e) & g);
        const uint32_t t1 = h + s1 + ch + k256[i] + w[i];
        const uint32_t s0 = rotr32(a, 2u) ^ rotr32(a, 13u) ^ rotr32(a, 22u);
        const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t t2 = s0 + maj;
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

void kalyx_sha256_init(KalyxSha256 *ctx) {
    ctx->state[0]=0x6a09e667u; ctx->state[1]=0xbb67ae85u; ctx->state[2]=0x3c6ef372u; ctx->state[3]=0xa54ff53au;
    ctx->state[4]=0x510e527fu; ctx->state[5]=0x9b05688cu; ctx->state[6]=0x1f83d9abu; ctx->state[7]=0x5be0cd19u;
    ctx->bit_len = 0u;
    ctx->block_len = 0u;
}

void kalyx_sha256_update(KalyxSha256 *ctx, const void *data, size_t len) {
    const unsigned char *p = (const unsigned char *)data;
    size_t i;
    if (!ctx || (!data && len)) return;
    for (i = 0u; i < len; i++) {
        ctx->block[ctx->block_len++] = p[i];
        if (ctx->block_len == 64u) {
            sha256_transform(ctx, ctx->block);
            ctx->bit_len += 512u;
            ctx->block_len = 0u;
        }
    }
}

void kalyx_sha256_final(KalyxSha256 *ctx, unsigned char out[32]) {
    size_t i;
    uint64_t total_bits;
    if (!ctx || !out) return;
    total_bits = ctx->bit_len + (uint64_t)ctx->block_len * 8u;
    ctx->block[ctx->block_len++] = 0x80u;
    if (ctx->block_len > 56u) {
        while (ctx->block_len < 64u) ctx->block[ctx->block_len++] = 0u;
        sha256_transform(ctx, ctx->block);
        ctx->block_len = 0u;
    }
    while (ctx->block_len < 56u) ctx->block[ctx->block_len++] = 0u;
    for (i = 0u; i < 8u; i++) ctx->block[63u - i] = (unsigned char)(total_bits >> (i * 8u));
    sha256_transform(ctx, ctx->block);
    for (i = 0u; i < 8u; i++) store_be32(out + i * 4u, ctx->state[i]);
}

void kalyx_sha256_hex(const unsigned char digest[32], char out_hex[KALYX_SHA256_HEX_BYTES]) {
    static const char hex[] = "0123456789abcdef";
    size_t i;
    for (i = 0u; i < 32u; i++) {
        out_hex[i * 2u] = hex[digest[i] >> 4u];
        out_hex[i * 2u + 1u] = hex[digest[i] & 15u];
    }
    out_hex[64] = '\0';
}

KalyxStatus kalyx_sha256_file_hex(const char *path, char out_hex[KALYX_SHA256_HEX_BYTES]) {
    FILE *f;
    KalyxSha256 ctx;
    unsigned char buf[4096];
    unsigned char digest[32];
    size_t n;
    if (!path || !out_hex) return KALYX_ERR_INVALID_ARGUMENT;
    f = fopen(path, "rb");
    if (!f) return KALYX_ERR_IO;
    kalyx_sha256_init(&ctx);
    while ((n = fread(buf, 1u, sizeof(buf), f)) > 0u) kalyx_sha256_update(&ctx, buf, n);
    if (ferror(f)) { fclose(f); return KALYX_ERR_IO; }
    if (fclose(f) != 0) return KALYX_ERR_IO;
    kalyx_sha256_final(&ctx, digest);
    kalyx_sha256_hex(digest, out_hex);
    return KALYX_OK;
}

KalyxStatus kalyx_sha256_text_hex(const char *text, char out_hex[KALYX_SHA256_HEX_BYTES]) {
    KalyxSha256 ctx;
    unsigned char digest[32];
    if (!text || !out_hex) return KALYX_ERR_INVALID_ARGUMENT;
    kalyx_sha256_init(&ctx);
    kalyx_sha256_update(&ctx, text, strlen(text));
    kalyx_sha256_final(&ctx, digest);
    kalyx_sha256_hex(digest, out_hex);
    return KALYX_OK;
}
