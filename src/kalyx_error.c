#include "kalyx_error.h"

const char *kalyx_status_string(KalyxStatus status) {
    switch (status) {
        case KALYX_OK: return "ok";
        case KALYX_ERR_INVALID_ARGUMENT: return "invalid_argument";
        case KALYX_ERR_IO: return "io";
        case KALYX_ERR_PARSE: return "parse";
        case KALYX_ERR_SCHEMA: return "schema";
        case KALYX_ERR_FORBIDDEN_ACTION: return "forbidden_action";
        case KALYX_ERR_UNKNOWN_ACTION: return "unknown_action";
        case KALYX_ERR_MISSING_REQUIRED_FIELD: return "missing_required_field";
        case KALYX_ERR_HASH_MISMATCH: return "hash_mismatch";
        case KALYX_ERR_RESPONSE_REJECTED: return "response_rejected";
        default: return "unknown";
    }
}
