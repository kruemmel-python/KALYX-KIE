#ifndef KALYX_AUDIT_H
#define KALYX_AUDIT_H

#include "kalyx_response.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KalyxAuditInput {
    const char *envelope_file;
    const char *response_file;
    const char *provider;
    const char *model;
    double temperature;
    int max_tokens;
    const KalyxValidationResult *validation;
} KalyxAuditInput;

KalyxStatus kalyx_write_audit_file(const KalyxAuditInput *input, const char *audit_path);

#ifdef __cplusplus
}
#endif

#endif
