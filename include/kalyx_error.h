#ifndef KALYX_ERROR_H
#define KALYX_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum KalyxStatus {
    KALYX_OK = 0,
    KALYX_ERR_INVALID_ARGUMENT = 1,
    KALYX_ERR_IO = 2,
    KALYX_ERR_PARSE = 3,
    KALYX_ERR_SCHEMA = 4,
    KALYX_ERR_FORBIDDEN_ACTION = 5,
    KALYX_ERR_UNKNOWN_ACTION = 6,
    KALYX_ERR_MISSING_REQUIRED_FIELD = 7,
    KALYX_ERR_HASH_MISMATCH = 8,
    KALYX_ERR_RESPONSE_REJECTED = 9
} KalyxStatus;

const char *kalyx_status_string(KalyxStatus status);

#ifdef __cplusplus
}
#endif

#endif
