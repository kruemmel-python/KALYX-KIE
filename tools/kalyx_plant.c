/*
 * KALYX-PLANT v0.2
 * Open, auditable planted-transition positive-control benchmark.
 *
 * Purpose:
 *   Create a declared artificial transition structure in a uint64 stream so
 *   KALYX/KGRAM can prove sensitivity to known generated order.
 *
 * This is NOT a covert channel. It writes an audit manifest and positions list.
 *
 * Apache-2.0, Ralf Kruemmel / KALYX
 */
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KPLANT_VERSION "KPLANT002"

static void usage(void) {
    fprintf(stderr,
        "kalyx_plant --in carrier.u64 --out planted.u64 --manifest plant_manifest.csv [options]\n"
        "\n"
        "Options:\n"
        "  --control out.u64          write copied/truncated original control stream\n"
        "  --pattern-out out.u64      write repeated plant pattern source stream\n"
        "  --payload TEXT             public benchmark label/payload, default: KALYX_PLANT\n"
        "  --seed HEX_OR_U64          default: 0x4b504c414e543032\n"
        "  --max-symbols N            0 = full file, default: 0\n"
        "  --pattern-len N            5..4096, default: 37\n"
        "  --period N                 nominal distance between plant anchors, default: 149\n"
        "  --offset N                 nominal first anchor offset, default: 17\n"
        "  --schedule periodic|jittered  default: jittered\n"
        "  --jitter N                 anchor jitter range, default: 0; script uses 23\n"
        "  --mode replace             only mode in v0.2\n"
        "  --positions out.csv        optional full position audit list\n"
        "  --help                     show help\n"
        "\n"
        "v0.2 defaults are intentionally de-aligned from common null parameters:\n"
        "  pattern_len=37, period=149, offset=17. Use block-size 4099 and\n"
        "  rotation-offset 4103 in null tests for a harder positive-control profile.\n"
        "\n"
        "Safety/semantics:\n"
        "  This tool creates declared positive controls. It is not designed for\n"
        "  non-detectable communication, stealth embedding, or evasion.\n");
}

static int parse_u64(const char *s, uint64_t *out) {
    if (!s || !*s || !out) return 0;
    errno = 0;
    char *end = NULL;
    unsigned long long v = 0;
    if (strlen(s) > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        v = strtoull(s + 2, &end, 16);
    } else {
        v = strtoull(s, &end, 10);
    }
    if (errno || !end || *end) return 0;
    *out = (uint64_t)v;
    return 1;
}

static uint64_t splitmix64(uint64_t *x) {
    uint64_t z;
    *x += UINT64_C(0x9e3779b97f4a7c15);
    z = *x;
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

static uint64_t fnv1a64_bytes(const void *data, size_t n, uint64_t seed) {
    const unsigned char *p = (const unsigned char *)data;
    uint64_t h = UINT64_C(1469598103934665603) ^ seed;
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint64_t)p[i];
        h *= UINT64_C(1099511628211);
    }
    return h ? h : 1u;
}

static uint64_t mix64(uint64_t x) {
    x ^= x >> 30;
    x *= UINT64_C(0xbf58476d1ce4e5b9);
    x ^= x >> 27;
    x *= UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    return x ? x : 1u;
}

static uint64_t file_size_bytes(FILE *f) {
    if (fseek(f, 0, SEEK_END) != 0) return 0;
#if defined(_WIN32)
    __int64 pos = _ftelli64(f);
    if (pos < 0) return 0;
#else
    long pos = ftell(f);
    if (pos < 0) return 0;
#endif
    if (fseek(f, 0, SEEK_SET) != 0) return 0;
    return (uint64_t)pos;
}

static int write_all_u64(const char *path, const uint64_t *v, uint64_t n) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "cannot open for write: %s\n", path);
        return 0;
    }
    size_t wr = fwrite(v, sizeof(uint64_t), (size_t)n, f);
    if (wr != (size_t)n) {
        fprintf(stderr, "short write: %s\n", path);
        fclose(f);
        return 0;
    }
    if (fclose(f) != 0) {
        fprintf(stderr, "close failed: %s\n", path);
        return 0;
    }
    return 1;
}

static uint64_t hash_u64_stream(const uint64_t *v, uint64_t n) {
    uint64_t h = UINT64_C(1469598103934665603);
    for (uint64_t i = 0; i < n; ++i) {
        uint64_t x = v[i];
        for (int b = 0; b < 8; ++b) {
            h ^= (uint8_t)(x & 0xffu);
            h *= UINT64_C(1099511628211);
            x >>= 8;
        }
    }
    return h ? h : 1u;
}

static int is_primeish(uint64_t x) {
    if (x < 2) return 0;
    if ((x % 2u) == 0u) return x == 2u;
    for (uint64_t d = 3u; d * d <= x && d < 100000u; d += 2u) {
        if ((x % d) == 0u) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *in_path = NULL;
    const char *out_path = NULL;
    const char *manifest_path = NULL;
    const char *control_path = NULL;
    const char *pattern_out_path = NULL;
    const char *positions_path = NULL;
    const char *payload = "KALYX_PLANT";
    const char *mode = "replace";
    const char *schedule = "jittered";
    uint64_t seed = UINT64_C(0x4b504c414e543032); /* KPLANT02 */
    uint64_t max_symbols = 0;
    uint64_t pattern_len = 37;
    uint64_t period = 149;
    uint64_t offset = 17;
    uint64_t jitter = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--in") == 0 && i + 1 < argc) in_path = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
        else if (strcmp(argv[i], "--manifest") == 0 && i + 1 < argc) manifest_path = argv[++i];
        else if (strcmp(argv[i], "--control") == 0 && i + 1 < argc) control_path = argv[++i];
        else if (strcmp(argv[i], "--pattern-out") == 0 && i + 1 < argc) pattern_out_path = argv[++i];
        else if (strcmp(argv[i], "--positions") == 0 && i + 1 < argc) positions_path = argv[++i];
        else if (strcmp(argv[i], "--payload") == 0 && i + 1 < argc) payload = argv[++i];
        else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) mode = argv[++i];
        else if (strcmp(argv[i], "--schedule") == 0 && i + 1 < argc) schedule = argv[++i];
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &seed)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--max-symbols") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &max_symbols)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--pattern-len") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &pattern_len)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--period") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &period)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--offset") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &offset)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--jitter") == 0 && i + 1 < argc) { if (!parse_u64(argv[++i], &jitter)) { usage(); return 2; } }
        else if (strcmp(argv[i], "--help") == 0) { usage(); return 0; }
        else { usage(); return 2; }
    }

    if (!in_path || !out_path || !manifest_path) { usage(); return 2; }
    if (strcmp(mode, "replace") != 0) {
        fprintf(stderr, "unsupported mode: %s\n", mode);
        return 2;
    }
    if (strcmp(schedule, "periodic") != 0 && strcmp(schedule, "jittered") != 0) {
        fprintf(stderr, "unsupported schedule: %s\n", schedule);
        return 2;
    }
    if (pattern_len < 5 || pattern_len > 4096) {
        fprintf(stderr, "pattern-len must be 5..4096\n");
        return 2;
    }
    if (period <= pattern_len) {
        fprintf(stderr, "period must be > pattern-len\n");
        return 2;
    }
    if (jitter >= period / 2u) {
        fprintf(stderr, "jitter must be < period/2\n");
        return 2;
    }

    FILE *in = fopen(in_path, "rb");
    if (!in) {
        fprintf(stderr, "cannot open: %s\n", in_path);
        return 1;
    }

    uint64_t bytes = file_size_bytes(in);
    if (bytes == 0 || (bytes % 8) != 0) {
        fprintf(stderr, "input must be non-empty uint64 stream: %s\n", in_path);
        fclose(in);
        return 1;
    }
    uint64_t n = bytes / 8u;
    if (max_symbols > 0 && max_symbols < n) n = max_symbols;
    if (n < pattern_len + offset + 1u) {
        fprintf(stderr, "stream too short for offset/pattern: n=%" PRIu64 "\n", n);
        fclose(in);
        return 1;
    }
    if (n > (uint64_t)((size_t)-1 / sizeof(uint64_t))) {
        fprintf(stderr, "stream too large for this build\n");
        fclose(in);
        return 1;
    }

    uint64_t *control = (uint64_t *)calloc((size_t)n, sizeof(uint64_t));
    uint64_t *planted = (uint64_t *)calloc((size_t)n, sizeof(uint64_t));
    uint64_t *pattern = (uint64_t *)calloc((size_t)pattern_len, sizeof(uint64_t));
    uint64_t *pattern_src = NULL;
    if (!control || !planted || !pattern) {
        fprintf(stderr, "out of memory\n");
        fclose(in);
        free(control); free(planted); free(pattern);
        return 1;
    }

    size_t rd = fread(control, sizeof(uint64_t), (size_t)n, in);
    fclose(in);
    if (rd != (size_t)n) {
        fprintf(stderr, "short read: %s\n", in_path);
        free(control); free(planted); free(pattern);
        return 1;
    }
    memcpy(planted, control, (size_t)n * sizeof(uint64_t));

    uint64_t payload_hash = fnv1a64_bytes(payload, strlen(payload), seed);

    /* v0.2 pattern: deterministic seeded permutation-cycle values.
       A prime-ish pattern_len is recommended so common null block sizes do not
       align with the cycle. */
    for (uint64_t i = 0; i < pattern_len; ++i) pattern[i] = i;
    uint64_t prng = seed ^ payload_hash ^ UINT64_C(0x504c414e543032aa);
    for (uint64_t i = pattern_len - 1u; i > 0u; --i) {
        uint64_t j = splitmix64(&prng) % (i + 1u);
        uint64_t tmp = pattern[i]; pattern[i] = pattern[j]; pattern[j] = tmp;
    }
    for (uint64_t i = 0; i < pattern_len; ++i) {
        uint64_t x = payload_hash ^ seed ^ (pattern[i] * UINT64_C(0x9e3779b97f4a7c15)) ^ (i * UINT64_C(0xbf58476d1ce4e5b9));
        pattern[i] = mix64(x);
    }

    uint64_t anchors = 0;
    uint64_t writes = 0;
    uint64_t base = offset;
    uint64_t next_min = offset;
    uint64_t schedule_rng = seed ^ payload_hash ^ UINT64_C(0x5343484544554c45);
    FILE *pf = NULL;
    if (positions_path) {
        pf = fopen(positions_path, "wb");
        if (!pf) {
            fprintf(stderr, "cannot write positions: %s\n", positions_path);
            free(control); free(planted); free(pattern);
            return 1;
        }
        fprintf(pf, "anchor_index,base_position,jitter_delta,position,length\n");
    }

    while (base + pattern_len <= n) {
        int64_t delta = 0;
        if (strcmp(schedule, "jittered") == 0 && jitter > 0u) {
            uint64_t span = jitter * 2u + 1u;
            uint64_t raw = splitmix64(&schedule_rng) % span;
            delta = (int64_t)raw - (int64_t)jitter;
        }

        uint64_t pos;
        if (delta < 0) {
            uint64_t neg = (uint64_t)(-delta);
            pos = (base > neg) ? (base - neg) : 0u;
        } else {
            pos = base + (uint64_t)delta;
        }

        if (pos < next_min) pos = next_min;
        if (pos + pattern_len > n) break;

        for (uint64_t j = 0; j < pattern_len; ++j) {
            planted[pos + j] = pattern[j];
            writes++;
        }
        if (pf) fprintf(pf, "%" PRIu64 ",%" PRIu64 ",%" PRId64 ",%" PRIu64 ",%" PRIu64 "\n",
                        anchors, base, delta, pos, pattern_len);
        anchors++;
        next_min = pos + pattern_len + 1u;
        if (UINT64_MAX - base < period) break;
        base += period;
    }
    if (pf) fclose(pf);

    if (anchors == 0u || writes == 0u) {
        fprintf(stderr, "no plant positions selected\n");
        free(control); free(planted); free(pattern);
        return 1;
    }

    if (!write_all_u64(out_path, planted, n)) {
        free(control); free(planted); free(pattern);
        return 1;
    }
    if (control_path && !write_all_u64(control_path, control, n)) {
        free(control); free(planted); free(pattern);
        return 1;
    }

    if (pattern_out_path) {
        pattern_src = (uint64_t *)calloc((size_t)n, sizeof(uint64_t));
        if (!pattern_src) {
            fprintf(stderr, "out of memory pattern_src\n");
            free(control); free(planted); free(pattern);
            return 1;
        }
        for (uint64_t i = 0; i < n; ++i) pattern_src[i] = pattern[i % pattern_len];
        if (!write_all_u64(pattern_out_path, pattern_src, n)) {
            free(control); free(planted); free(pattern); free(pattern_src);
            return 1;
        }
    }

    FILE *mf = fopen(manifest_path, "wb");
    if (!mf) {
        fprintf(stderr, "cannot write manifest: %s\n", manifest_path);
        free(control); free(planted); free(pattern); free(pattern_src);
        return 1;
    }
    uint64_t control_hash = hash_u64_stream(control, n);
    uint64_t planted_hash = hash_u64_stream(planted, n);
    uint64_t pattern_hash = hash_u64_stream(pattern, pattern_len);
    double density = (double)writes / (double)n;

    fprintf(mf, "key,value\n");
    fprintf(mf, "version,%s\n", KPLANT_VERSION);
    fprintf(mf, "purpose,declared_positive_control\n");
    fprintf(mf, "mode,replace\n");
    fprintf(mf, "schedule,%s\n", schedule);
    fprintf(mf, "input,%s\n", in_path);
    fprintf(mf, "output,%s\n", out_path);
    if (control_path) fprintf(mf, "control,%s\n", control_path);
    if (pattern_out_path) fprintf(mf, "pattern_out,%s\n", pattern_out_path);
    fprintf(mf, "payload_label,%s\n", payload);
    fprintf(mf, "payload_fnv1a64,0x%016" PRIx64 "\n", payload_hash);
    fprintf(mf, "seed,0x%016" PRIx64 "\n", seed);
    fprintf(mf, "symbols,%" PRIu64 "\n", n);
    fprintf(mf, "pattern_len,%" PRIu64 "\n", pattern_len);
    fprintf(mf, "pattern_len_primeish,%d\n", is_primeish(pattern_len));
    fprintf(mf, "period,%" PRIu64 "\n", period);
    fprintf(mf, "offset,%" PRIu64 "\n", offset);
    fprintf(mf, "jitter,%" PRIu64 "\n", jitter);
    fprintf(mf, "anchors,%" PRIu64 "\n", anchors);
    fprintf(mf, "writes,%" PRIu64 "\n", writes);
    fprintf(mf, "density,%.17g\n", density);
    fprintf(mf, "control_fnv1a64,0x%016" PRIx64 "\n", control_hash);
    fprintf(mf, "planted_fnv1a64,0x%016" PRIx64 "\n", planted_hash);
    fprintf(mf, "pattern_fnv1a64,0x%016" PRIx64 "\n", pattern_hash);
    fprintf(mf, "safety_note,open_declared_benchmark_not_covert_channel\n");
    fclose(mf);

    printf("kalyx_plant v0.2: in=%s out=%s n=%" PRIu64 " pattern_len=%" PRIu64
           " period=%" PRIu64 " offset=%" PRIu64 " schedule=%s jitter=%" PRIu64
           " anchors=%" PRIu64 " writes=%" PRIu64
           " density=%.9g payload_hash=0x%016" PRIx64 "\n",
           in_path, out_path, n, pattern_len, period, offset, schedule, jitter,
           anchors, writes, density, payload_hash);
    printf("manifest=%s\n", manifest_path);
    if (control_path) printf("control=%s\n", control_path);
    if (pattern_out_path) printf("pattern_out=%s\n", pattern_out_path);
    if (positions_path) printf("positions=%s\n", positions_path);

    free(control); free(planted); free(pattern); free(pattern_src);
    return 0;
}
