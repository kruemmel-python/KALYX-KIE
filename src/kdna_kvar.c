#include "kdna_kvar.h"
#include "kdna_ksoa.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct variant_sample {
    uint64_t variant_id;
    size_t index;
} variant_sample;

static int cmp_sample_variant(const void *a, const void *b) {
    const variant_sample *x = (const variant_sample *)a;
    const variant_sample *y = (const variant_sample *)b;
    if (x->variant_id < y->variant_id) return -1;
    if (x->variant_id > y->variant_id) return 1;
    if (x->index < y->index) return -1;
    if (x->index > y->index) return 1;
    return 0;
}

static double field_at(const double *p, size_t n, uint32_t field, size_t i) {
    return p[kdna_kgen_idx(field, n, i)];
}

static double absd(double x) { return x < 0.0 ? -x : x; }

static uint32_t raw_at(const double *p, size_t n, size_t i) {
    const double v = field_at(p, n, KDNA_KGEN_RAW, i);
    return (uint32_t)(v + 0.5);
}

static uint32_t dom_at(const double *p, size_t n, size_t i) {
    const double v = field_at(p, n, KDNA_KGEN_DOM, i);
    return (uint32_t)(v + 0.5);
}

static uint64_t variant_at(const double *p, size_t n, size_t i) {
    const double v = field_at(p, n, KDNA_KGEN_VARIANT_ID, i);
    return (uint64_t)v;
}

int kdna_kvar_payload_bytes(size_t variant_count, uint64_t *bytes_out) {
    if (!bytes_out) return KDNA_EINVAL;
    if (variant_count == 0u) {
        *bytes_out = 0u;
        return KDNA_OK;
    }
    if (variant_count > ((size_t)-1) / sizeof(kdna_kvar_record)) return KDNA_EINVAL;
    const size_t bytes = variant_count * sizeof(kdna_kvar_record);
    if ((uint64_t)bytes != (uint64_t)variant_count * (uint64_t)sizeof(kdna_kvar_record)) return KDNA_EINVAL;
    *bytes_out = (uint64_t)bytes;
    return KDNA_OK;
}

int kdna_kvar_init_header(kdna_kvar_header *h,
                          size_t variant_count,
                          const kdna_kgen_header *source) {
    if (!h || !source) return KDNA_EINVAL;
    int rc = kdna_kgen_validate_header(source);
    if (rc != KDNA_OK) return rc;
    if (!kdna_ksoa_host_is_little_endian()) return KDNA_EIO;

    uint64_t payload_bytes = 0u;
    rc = kdna_kvar_payload_bytes(variant_count, &payload_bytes);
    if (rc != KDNA_OK) return rc;

    memset(h, 0, sizeof(*h));
    memcpy(h->magic, KDNA_KVAR_MAGIC, 8u);
    h->version = KDNA_KVAR_VERSION;
    h->header_bytes = KDNA_KVAR_HEADER_BYTES;
    h->record_bytes = KDNA_KVAR_RECORD_BYTES;
    h->variant_count = (uint64_t)variant_count;
    h->source_n = source->n;
    h->source_steps = source->steps;
    h->source_seed = source->seed;
    h->sample_count = source->n;
    h->payload_bytes = payload_bytes;
    h->flags = KDNA_KVAR_FLAG_LE_IEEE754_DOUBLE |
               KDNA_KVAR_FLAG_SOURCE_KGEN_V1 |
               KDNA_KVAR_FLAG_VARIANT_DNA |
               KDNA_KVAR_FLAG_ADJACENCY_INDEX;
    h->x_min = source->x_min;
    h->x_max = source->x_max;
    h->time_value = (double)source->steps * source->dt;
    h->resonance_threshold = source->resonance_threshold;
    return KDNA_OK;
}

int kdna_kvar_validate_header(const kdna_kvar_header *h) {
    if (!h) return KDNA_EINVAL;
    if (memcmp(h->magic, KDNA_KVAR_MAGIC, 8u) != 0) return KDNA_EINVAL;
    if (h->version != KDNA_KVAR_VERSION) return KDNA_EINVAL;
    if (h->header_bytes != KDNA_KVAR_HEADER_BYTES) return KDNA_EINVAL;
    if (h->record_bytes != KDNA_KVAR_RECORD_BYTES) return KDNA_EINVAL;
    if (h->source_n == 0u || h->sample_count != h->source_n) return KDNA_EINVAL;
    if (h->source_steps == 0u) return KDNA_EINVAL;
    if (h->variant_count == 0u) return KDNA_EINVAL;
    if (h->payload_bytes != h->variant_count * (uint64_t)sizeof(kdna_kvar_record)) return KDNA_EINVAL;
    if ((h->flags & KDNA_KVAR_FLAG_LE_IEEE754_DOUBLE) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KVAR_FLAG_SOURCE_KGEN_V1) == 0u) return KDNA_EINVAL;
    if ((h->flags & KDNA_KVAR_FLAG_VARIANT_DNA) == 0u) return KDNA_EINVAL;
    if (!isfinite(h->x_min) || !isfinite(h->x_max) || !isfinite(h->time_value)) return KDNA_EINVAL;
    return KDNA_OK;
}

int kdna_kvar_write_file(const char *path,
                         const kdna_kvar_header *h,
                         const kdna_kvar_record *records) {
    if (!path || !h || !records) return KDNA_EINVAL;
    int rc = kdna_kvar_validate_header(h);
    if (rc != KDNA_OK) return rc;

    FILE *f = fopen(path, "wb");
    if (!f) return KDNA_EIO;
    int ok = 1;
    if (fwrite(h, 1u, sizeof(*h), f) != sizeof(*h)) ok = 0;
    if (ok && fwrite(records, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) ok = 0;
    if (fclose(f) != 0) ok = 0;
    return ok ? KDNA_OK : KDNA_EIO;
}

int kdna_kvar_read_header_file(const char *path, kdna_kvar_header *h) {
    if (!path || !h) return KDNA_EINVAL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    const size_t got = fread(h, 1u, sizeof(*h), f);
    const int close_ok = fclose(f) == 0;
    if (got != sizeof(*h) || !close_ok) return KDNA_EIO;
    return kdna_kvar_validate_header(h);
}

int kdna_kvar_read_file(const char *path,
                        kdna_kvar_header *h,
                        kdna_kvar_record **records_out) {
    if (!path || !h || !records_out) return KDNA_EINVAL;
    *records_out = NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return KDNA_EIO;
    if (fread(h, 1u, sizeof(*h), f) != sizeof(*h)) {
        fclose(f);
        return KDNA_EIO;
    }
    int rc = kdna_kvar_validate_header(h);
    if (rc != KDNA_OK) {
        fclose(f);
        return rc;
    }
    kdna_kvar_record *records = (kdna_kvar_record *)calloc((size_t)h->variant_count, sizeof(kdna_kvar_record));
    if (!records) {
        fclose(f);
        return KDNA_ENOMEM;
    }
    if (fread(records, 1u, (size_t)h->payload_bytes, f) != (size_t)h->payload_bytes) {
        free(records);
        fclose(f);
        return KDNA_EIO;
    }
    if (fclose(f) != 0) {
        free(records);
        return KDNA_EIO;
    }
    *records_out = records;
    return KDNA_OK;
}

static uint32_t variant_flags(const kdna_kvar_record *r, double threshold) {
    uint32_t flags = 0u;
    if (r->resonance_max >= threshold || r->resonance_mean >= threshold) flags |= KDNA_KVAR_REC_FLAG_RESONANT;
    if (r->injection_max >= 0.65 || r->injection_mean >= 0.50) flags |= KDNA_KVAR_REC_FLAG_HIGH_INJECTION;
    if (r->stability_mean >= 0.90) flags |= KDNA_KVAR_REC_FLAG_HIGH_STABILITY;
    if (r->raw != r->dom) flags |= KDNA_KVAR_REC_FLAG_RAW_DOM_MISMATCH;
    if (r->sample_count > 1u) flags |= KDNA_KVAR_REC_FLAG_REPEATED;
    if (r->predecessor_id != 0u) flags |= KDNA_KVAR_REC_FLAG_HAS_PREDECESSOR;
    if (r->successor_id != 0u) flags |= KDNA_KVAR_REC_FLAG_HAS_SUCCESSOR;
    if (r->x_min <= 0.0 && r->x_max >= 0.0) flags |= KDNA_KVAR_REC_FLAG_NULL_NEAR;
    return flags;
}

static uint64_t dominant_neighbor(const double *payload,
                                  size_t n,
                                  const variant_sample *samples,
                                  size_t begin,
                                  size_t end,
                                  int predecessor,
                                  uint64_t *count_out) {
    uint64_t best_id = 0u;
    uint64_t best_count = 0u;
    for (size_t a = begin; a < end; ++a) {
        const size_t i = samples[a].index;
        if (predecessor) {
            if (i == 0u) continue;
            best_id = variant_at(payload, n, i - 1u);
        } else {
            if (i + 1u >= n) continue;
            best_id = variant_at(payload, n, i + 1u);
        }
        if (best_id != samples[a].variant_id) {
            best_count = 1u;
            break;
        }
    }
    if (best_count == 0u) {
        if (count_out) *count_out = 0u;
        return 0u;
    }

    /* n is small enough in expected KGEN use for a second pass to count the candidate.
       This deliberately avoids heap hash tables in the ABI layer. */
    uint64_t count = 0u;
    for (size_t a = begin; a < end; ++a) {
        const size_t i = samples[a].index;
        uint64_t id = 0u;
        if (predecessor) {
            if (i == 0u) continue;
            id = variant_at(payload, n, i - 1u);
        } else {
            if (i + 1u >= n) continue;
            id = variant_at(payload, n, i + 1u);
        }
        if (id == best_id) count++;
    }
    if (count_out) *count_out = count;
    return best_id;
}

int kdna_kvar_build_from_kgen(const kdna_kgen_header *source_header,
                              const double *payload,
                              kdna_kvar_header *out_header,
                              kdna_kvar_record **records_out) {
    if (!source_header || !payload || !out_header || !records_out) return KDNA_EINVAL;
    *records_out = NULL;
    int rc = kdna_kgen_validate_header(source_header);
    if (rc != KDNA_OK) return rc;

    const size_t n = (size_t)source_header->n;
    variant_sample *samples = (variant_sample *)calloc(n, sizeof(variant_sample));
    if (!samples) return KDNA_ENOMEM;

    for (size_t i = 0u; i < n; ++i) {
        samples[i].variant_id = variant_at(payload, n, i);
        samples[i].index = i;
        if (samples[i].variant_id == 0u) {
            free(samples);
            return KDNA_EINVAL;
        }
    }
    qsort(samples, n, sizeof(variant_sample), cmp_sample_variant);

    size_t variant_count = 0u;
    for (size_t i = 0u; i < n; ) {
        size_t j = i + 1u;
        while (j < n && samples[j].variant_id == samples[i].variant_id) j++;
        variant_count++;
        i = j;
    }
    if (variant_count == 0u) {
        free(samples);
        return KDNA_EINVAL;
    }

    kdna_kvar_record *records = (kdna_kvar_record *)calloc(variant_count, sizeof(kdna_kvar_record));
    if (!records) {
        free(samples);
        return KDNA_ENOMEM;
    }

    size_t rix = 0u;
    for (size_t i = 0u; i < n; ) {
        size_t j = i + 1u;
        while (j < n && samples[j].variant_id == samples[i].variant_id) j++;

        kdna_kvar_record *r = &records[rix];
        const size_t first_sample_i = samples[i].index;

        r->variant_id = samples[i].variant_id;
        r->resonance_id_min = (uint64_t)field_at(payload, n, KDNA_KGEN_RESONANCE_ID, first_sample_i);
        r->resonance_id_max = r->resonance_id_min;
        r->first_i = (uint64_t)first_sample_i;
        r->last_i = (uint64_t)first_sample_i;
        r->sample_count = (uint64_t)(j - i);
        r->raw = raw_at(payload, n, first_sample_i);
        r->dom = dom_at(payload, n, first_sample_i);

        r->x_min = field_at(payload, n, KDNA_KGEN_X, first_sample_i);
        r->x_max = r->x_min;
        r->resonance_min = field_at(payload, n, KDNA_KGEN_RESONANCE, first_sample_i);
        r->resonance_max = r->resonance_min;
        r->stability_min = field_at(payload, n, KDNA_KGEN_STABILITY, first_sample_i);
        r->stability_max = r->stability_min;
        r->lock_min = field_at(payload, n, KDNA_KGEN_LOCK, first_sample_i);
        r->lock_max = r->lock_min;

        double sum_x = 0.0, sum_e = 0.0, sum_phi = 0.0, sum_res = 0.0, sum_inj = 0.0;
        double sum_stability = 0.0, sum_align = 0.0, sum_lock = 0.0, sum_score = 0.0;
        double sum_time = 0.0, sum_i = 0.0;
        for (size_t a = i; a < j; ++a) {
            const size_t idx = samples[a].index;
            const double x = field_at(payload, n, KDNA_KGEN_X, idx);
            const double res = field_at(payload, n, KDNA_KGEN_RESONANCE, idx);
            const double inj = field_at(payload, n, KDNA_KGEN_INJECTION_POTENTIAL, idx);
            const double st = field_at(payload, n, KDNA_KGEN_STABILITY, idx);
            const double lk = field_at(payload, n, KDNA_KGEN_LOCK, idx);
            const double sc = field_at(payload, n, KDNA_KGEN_DOM_SCORE, idx);
            const uint64_t rid = (uint64_t)field_at(payload, n, KDNA_KGEN_RESONANCE_ID, idx);

            if (idx < r->first_i) r->first_i = (uint64_t)idx;
            if (idx > r->last_i) r->last_i = (uint64_t)idx;
            if (rid < r->resonance_id_min) r->resonance_id_min = rid;
            if (rid > r->resonance_id_max) r->resonance_id_max = rid;

            if (x < r->x_min) r->x_min = x;
            if (x > r->x_max) r->x_max = x;
            if (res < r->resonance_min) r->resonance_min = res;
            if (res > r->resonance_max) r->resonance_max = res;
            if (st < r->stability_min) r->stability_min = st;
            if (st > r->stability_max) r->stability_max = st;
            if (lk < r->lock_min) r->lock_min = lk;
            if (lk > r->lock_max) r->lock_max = lk;
            if (inj > r->injection_max) r->injection_max = inj;
            if (sc > r->score_max) r->score_max = sc;

            sum_x += x;
            sum_e += field_at(payload, n, KDNA_KGEN_E, idx);
            sum_phi += field_at(payload, n, KDNA_KGEN_PHI, idx);
            sum_res += res;
            sum_inj += inj;
            sum_stability += st;
            sum_align += field_at(payload, n, KDNA_KGEN_ALIGNMENT, idx);
            sum_lock += lk;
            sum_score += sc;
            sum_time += field_at(payload, n, KDNA_KGEN_TIME, idx);
            sum_i += (double)idx;

            const uint32_t k_fields[5] = {KDNA_KGEN_K01, KDNA_KGEN_K02, KDNA_KGEN_K03, KDNA_KGEN_K04, KDNA_KGEN_K05};
            const uint32_t s_fields[5] = {KDNA_KGEN_S01, KDNA_KGEN_S02, KDNA_KGEN_S03, KDNA_KGEN_S04, KDNA_KGEN_S05};
            for (size_t op = 0u; op < 5u; ++op) {
                const double kv = field_at(payload, n, k_fields[op], idx);
                const double sv = field_at(payload, n, s_fields[op], idx);
                r->k_mean[op] += kv;
                r->s_mean[op] += sv;
                if (absd(kv) > r->k_absmax[op]) r->k_absmax[op] = absd(kv);
                if (absd(sv) > r->s_absmax[op]) r->s_absmax[op] = absd(sv);
            }
        }

        const double inv = 1.0 / (double)r->sample_count;
        r->x_mean = sum_x * inv;
        r->e_mean = sum_e * inv;
        r->phi_mean = sum_phi * inv;
        r->resonance_mean = sum_res * inv;
        r->injection_mean = sum_inj * inv;
        r->stability_mean = sum_stability * inv;
        r->alignment_mean = sum_align * inv;
        r->lock_mean = sum_lock * inv;
        r->score_mean = sum_score * inv;
        r->time_mean = sum_time * inv;
        r->centroid_i = sum_i * inv;
        r->span_i = (double)(r->last_i - r->first_i + 1u);
        for (size_t op = 0u; op < 5u; ++op) {
            r->k_mean[op] *= inv;
            r->s_mean[op] *= inv;
        }

        r->predecessor_id = dominant_neighbor(payload, n, samples, i, j, 1, &r->predecessor_count);
        r->successor_id = dominant_neighbor(payload, n, samples, i, j, 0, &r->successor_count);
        r->flags = variant_flags(r, source_header->resonance_threshold);

        rix++;
        i = j;
    }

    rc = kdna_kvar_init_header(out_header, variant_count, source_header);
    if (rc != KDNA_OK) {
        free(records);
        free(samples);
        return rc;
    }

    free(samples);
    *records_out = records;
    return KDNA_OK;
}

const char *kdna_kvar_record_flag_name(uint32_t flag) {
    switch (flag) {
        case KDNA_KVAR_REC_FLAG_RESONANT: return "resonant";
        case KDNA_KVAR_REC_FLAG_HIGH_INJECTION: return "high_injection";
        case KDNA_KVAR_REC_FLAG_HIGH_STABILITY: return "high_stability";
        case KDNA_KVAR_REC_FLAG_RAW_DOM_MISMATCH: return "raw_dom_mismatch";
        case KDNA_KVAR_REC_FLAG_REPEATED: return "repeated";
        case KDNA_KVAR_REC_FLAG_HAS_PREDECESSOR: return "has_predecessor";
        case KDNA_KVAR_REC_FLAG_HAS_SUCCESSOR: return "has_successor";
        case KDNA_KVAR_REC_FLAG_NULL_NEAR: return "null_near";
        default: return "unknown";
    }
}
