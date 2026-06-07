#include "kdna_kreg.h"

#include <string.h>

int kdna_kreg_payload_bytes(size_t segment_count, uint64_t *bytes_out) {
    if (!bytes_out || segment_count == 0u) return KDNA_EINVAL;
    if (segment_count > ((size_t)-1) / sizeof(kdna_kreg_record)) return KDNA_EINVAL;
    const size_t bytes = segment_count * sizeof(kdna_kreg_record);
    if ((uint64_t)bytes != (uint64_t)segment_count * (uint64_t)sizeof(kdna_kreg_record)) return KDNA_EINVAL;
    *bytes_out = (uint64_t)bytes;
    return KDNA_OK;
}

int kdna_kreg_init_header(kdna_kreg_header *h,
                          size_t segment_count,
                          size_t source_n,
                          double x_min,
                          double x_max,
                          double dx) {
    if (!h || segment_count == 0u || source_n == 0u) return KDNA_EINVAL;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    uint64_t payload_bytes = 0u;
    int rc = kdna_kreg_payload_bytes(segment_count, &payload_bytes);
    if (rc != KDNA_OK) return rc;

    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KREG_MAGIC, 8u);
    h->version = KDNA_KREG_VERSION;
    h->header_bytes = KDNA_KREG_HEADER_BYTES;
    h->record_bytes = KDNA_KREG_RECORD_BYTES;
    h->source_fields = KDNA_FIELDS;
    h->segment_count = (uint64_t)segment_count;
    h->source_n = (uint64_t)source_n;
    h->x_min = x_min;
    h->x_max = x_max;
    h->dx = dx;
    h->payload_bytes = payload_bytes;
    h->flags = KDNA_KREG_FLAG_LE_IEEE754_DOUBLE | KDNA_KREG_FLAG_SOURCE_KSOA_V1;
    return KDNA_OK;
}

int kdna_kreg_validate_header(const kdna_kreg_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KREG_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KREG_VERSION) return KDNA_EINVAL;
    if (h->header_bytes != KDNA_KREG_HEADER_BYTES) return KDNA_EINVAL;
    if (h->record_bytes != KDNA_KREG_RECORD_BYTES) return KDNA_EINVAL;
    if (h->source_fields != KDNA_FIELDS) return KDNA_EINVAL;
    if (h->segment_count == 0u || h->source_n == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KREG_FLAG_LE_IEEE754_DOUBLE) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KREG_FLAG_SOURCE_KSOA_V1) == 0u) return KDNA_EINVAL;

    uint64_t expected_payload = 0u;
    int rc = kdna_kreg_payload_bytes((size_t)h->segment_count, &expected_payload);
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

int kdna_kreg_write_file(const char *path,
                         const kdna_kreg_header *h,
                         const kdna_kreg_record *records) {
    if (!path || !h || !records) return KDNA_EINVAL;

    int rc = kdna_kreg_validate_header(h);
    if (rc != KDNA_OK) return rc;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;

    int ok = write_exact(f, h, sizeof(*h)) &&
             write_exact(f, records, (size_t)h->payload_bytes);
    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

int kdna_kreg_read_header_file(const char *path, kdna_kreg_header *h) {
    if (!path || !h) return KDNA_EINVAL;

    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;

    const int ok = read_exact(f, h, sizeof(*h));
    const int close_ok = (fclose(f) == 0);
    if (!ok || !close_ok) return KDNA_EIO;

    return kdna_kreg_validate_header(h);
}
