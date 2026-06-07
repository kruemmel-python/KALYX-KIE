#include "kdna_ksoa.h"

#include <errno.h>
#include <string.h>

typedef char kdna_ksoa_header_size_must_be_128[(sizeof(kdna_ksoa_header) == KDNA_KSOA_HEADER_BYTES) ? 1 : -1];

int kdna_ksoa_host_is_little_endian(void) {
    const uint16_t x = 1u;
    return *((const uint8_t *)&x) == 1u;
}

const char *kdna_ksoa_backend_name(uint32_t backend) {
    switch (backend) {
        case KDNA_KSOA_BACKEND_CPU: return "cpu";
        case KDNA_KSOA_BACKEND_OPENCL: return "opencl";
        default: return "unknown";
    }
}

static int checked_mul_size(size_t a, size_t b, size_t *out) {
    if (a != 0u && b > ((size_t)-1) / a) return 0;
    *out = a * b;
    return 1;
}

int kdna_ksoa_payload_bytes(size_t n, uint64_t *bytes_out) {
    if (!bytes_out || n == 0u) return KDNA_EINVAL;

    size_t cells = 0u;
    size_t bytes = 0u;
    if (!checked_mul_size((size_t)KDNA_FIELDS, n, &cells)) return KDNA_EINVAL;
    if (!checked_mul_size(cells, sizeof(double), &bytes)) return KDNA_EINVAL;
    if ((uint64_t)bytes != (uint64_t)cells * (uint64_t)sizeof(double)) return KDNA_EINVAL;

    *bytes_out = (uint64_t)bytes;
    return KDNA_OK;
}

int kdna_ksoa_init_header(kdna_ksoa_header *h,
                          size_t n,
                          double x_min,
                          double x_max,
                          double dx,
                          uint32_t backend) {
    if (!h || n == 0u) return KDNA_EINVAL;
    if (backend != KDNA_KSOA_BACKEND_CPU && backend != KDNA_KSOA_BACKEND_OPENCL) return KDNA_EINVAL;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    uint64_t payload_bytes = 0u;
    int rc = kdna_ksoa_payload_bytes(n, &payload_bytes);
    if (rc != KDNA_OK) return rc;

    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KSOA_MAGIC, 8u);
    h->version = KDNA_KSOA_VERSION;
    h->header_bytes = KDNA_KSOA_HEADER_BYTES;
    h->fields = KDNA_FIELDS;
    h->backend = backend;
    h->n = (uint64_t)n;
    h->x_min = x_min;
    h->x_max = x_max;
    h->dx = dx;
    h->payload_bytes = payload_bytes;
    h->flags = KDNA_KSOA_FLAG_LE_IEEE754_DOUBLE;
    return KDNA_OK;
}

int kdna_ksoa_validate_header(const kdna_ksoa_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KSOA_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KSOA_VERSION) return KDNA_EINVAL;
    if (h->header_bytes != KDNA_KSOA_HEADER_BYTES) return KDNA_EINVAL;
    if (h->fields != KDNA_FIELDS) return KDNA_EINVAL;
    if (h->backend != KDNA_KSOA_BACKEND_CPU && h->backend != KDNA_KSOA_BACKEND_OPENCL) return KDNA_EINVAL;
    if (h->n == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KSOA_FLAG_LE_IEEE754_DOUBLE) == 0u) return KDNA_EINVAL;

    uint64_t expected_payload = 0u;
    int rc = kdna_ksoa_payload_bytes((size_t)h->n, &expected_payload);
    if (rc != KDNA_OK) return rc;
    if (h->payload_bytes != expected_payload) return KDNA_EINVAL;
    return KDNA_OK;
}

static int write_exact(FILE *f, const void *ptr, size_t bytes) {
    return fwrite(ptr, 1u, bytes, f) == bytes;
}

static int read_exact(FILE *f, void *ptr, size_t bytes) {
    return fread(ptr, 1u, bytes, f) == bytes;
}

int kdna_ksoa_write_file(const char *path,
                         const kdna_ksoa_header *h,
                         const double *payload) {
    if (!path || !h || !payload) return KDNA_EINVAL;

    int rc = kdna_ksoa_validate_header(h);
    if (rc != KDNA_OK) return rc;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;

    int ok = write_exact(f, h, sizeof(*h)) &&
             write_exact(f, payload, (size_t)h->payload_bytes);

    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

int kdna_ksoa_read_header_file(const char *path, kdna_ksoa_header *h) {
    if (!path || !h) return KDNA_EINVAL;

    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;

    const int ok = read_exact(f, h, sizeof(*h));
    const int close_ok = (fclose(f) == 0);
    if (!ok || !close_ok) return KDNA_EIO;

    return kdna_ksoa_validate_header(h);
}
