#ifndef KDNA_KEXP_H
#define KDNA_KEXP_H

#include "kdna.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KDNA_KDAT_MAGIC "KDAT0001"
#define KDNA_KREP_MAGIC "KREP0001"
#define KDNA_KEXP_VERSION 1u
#define KDNA_KDAT_HEADER_BYTES 128u
#define KDNA_KREP_HEADER_BYTES 128u
#define KDNA_KREP_RECORD_BYTES 512u

#define KDNA_KEXP_FLAG_LE_IEEE754_DOUBLE 1ull
#define KDNA_KEXP_FLAG_KDNA_VARIANTS     2ull
#define KDNA_KEXP_FLAG_GRAMMAR_TEST      4ull
#define KDNA_KEXP_FLAG_DETERMINISTIC     8ull

enum kdna_kexp_kind {
    KDNA_KEXP_PRNG = 1,
    KDNA_KEXP_LOGISTIC = 2,
    KDNA_KEXP_MARKOV = 3,
    KDNA_KEXP_HIDDEN_GRAMMAR = 4,
    KDNA_KEXP_BROWNIAN = 5,
    KDNA_KEXP_QUASI_PERIODIC = 6,
    KDNA_KEXP_KSIEVE = 7
};

typedef struct kdna_kdat_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t reserved0;

    uint64_t n;
    uint64_t seed;
    uint32_t kind;
    uint32_t reserved1;

    double x_min;
    double x_max;
    double mean;
    double variance;

    uint64_t payload_bytes;
    uint64_t flags;
    uint8_t reserved[32];
} kdna_kdat_header;

typedef struct kdna_krep_header {
    char magic[8];
    uint32_t version;
    uint32_t header_bytes;
    uint32_t record_bytes;
    uint32_t reserved0;

    uint64_t experiment_count;
    uint64_t payload_bytes;
    uint64_t created_unix;
    uint64_t flags;

    uint8_t reserved[72];
} kdna_krep_header;

typedef struct kdna_kexp_result_record {
    uint64_t id;
    uint64_t n;
    uint64_t train_n;
    uint64_t seed;

    uint32_t kind;
    uint32_t raw_bins;
    uint32_t unique_raw_bins;
    uint32_t unique_variants;

    uint64_t train_transitions;
    uint64_t test_transitions;
    uint64_t grammar_edges;
    uint64_t out_of_grammar;

    double x_min;
    double x_max;
    double x_mean;
    double x_variance;

    double entropy_raw;
    double entropy_variant;
    double compression_ratio;

    double baseline_accuracy;
    double kgram_accuracy;
    double kgram_lift;
    double surprise_rate;

    double mean_lock;
    double mean_dom_score;
    double max_dom_score;
    double null_membrane_hits;

    uint64_t top_variant_id;
    uint64_t top_variant_count;
    uint64_t top_edge_from;
    uint64_t top_edge_to;
    uint64_t top_edge_count;

    double reserved_d[34];
} kdna_kexp_result_record;

typedef char kdna_kdat_header_size_must_be_128[(sizeof(kdna_kdat_header) == KDNA_KDAT_HEADER_BYTES) ? 1 : -1];
typedef char kdna_krep_header_size_must_be_128[(sizeof(kdna_krep_header) == KDNA_KREP_HEADER_BYTES) ? 1 : -1];
typedef char kdna_kexp_record_size_must_be_512[(sizeof(kdna_kexp_result_record) == KDNA_KREP_RECORD_BYTES) ? 1 : -1];

const char *kdna_kexp_kind_name(uint32_t kind);
int kdna_kexp_kind_from_name(const char *name, uint32_t *kind_out);

int kdna_kdat_init_header(kdna_kdat_header *h,
                          uint64_t n,
                          uint64_t seed,
                          uint32_t kind,
                          const double *x);
int kdna_kdat_validate_header(const kdna_kdat_header *h);
int kdna_kdat_write_file(const char *path, const kdna_kdat_header *h, const double *x);
int kdna_kdat_read_file(const char *path, kdna_kdat_header *h, double **x_out);

int kdna_krep_init_header(kdna_krep_header *h, uint64_t experiment_count);
int kdna_krep_validate_header(const kdna_krep_header *h);
int kdna_krep_write_file(const char *path,
                         const kdna_krep_header *h,
                         const kdna_kexp_result_record *records);
int kdna_krep_read_file(const char *path,
                        kdna_krep_header *h,
                        kdna_kexp_result_record **records_out);

int kdna_kexp_generate(uint32_t kind, size_t n, uint64_t seed, double *x);
int kdna_kexp_analyze(const double *x,
                      size_t n,
                      uint32_t kind,
                      uint64_t seed,
                      double train_fraction,
                      uint32_t raw_bins,
                      const kdna_constants *constants,
                      kdna_kexp_result_record *result);

#ifdef __cplusplus
}
#endif

#endif
