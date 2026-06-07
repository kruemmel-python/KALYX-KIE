#ifndef KALYX_RESPONSE_H
#define KALYX_RESPONSE_H

#include "kalyx_common.h"
#include "kalyx_domain.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum KalyxResponseType {
    KALYX_RESPONSE_ANSWER = 1,
    KALYX_RESPONSE_COMMAND = 2,
    KALYX_RESPONSE_CLARIFICATION = 3,
    KALYX_RESPONSE_REJECTION = 4,
    KALYX_RESPONSE_DIAGNOSTIC = 5
} KalyxResponseType;

typedef enum KalyxRisk {
    KALYX_RISK_NONE = 0,
    KALYX_RISK_LOW = 1,
    KALYX_RISK_MEDIUM = 2,
    KALYX_RISK_HIGH = 3,
    KALYX_RISK_CRITICAL = 4
} KalyxRisk;

typedef struct KalyxValidationResult {
    KalyxStatus status;
    int accepted;
    KalyxResponseType response_type;
    KalyxRisk risk;
    int requires_confirmation;
    int uses_only_provided_context;
    char command_name[96];
    char error[512];
} KalyxValidationResult;

const char *kalyx_response_type_name(KalyxResponseType type);
const char *kalyx_risk_name(KalyxRisk risk);
KalyxStatus kalyx_validate_response_text(const char *envelope_markdown,
                                         const char *response_markdown,
                                         KalyxValidationResult *out);
KalyxStatus kalyx_validate_response_files(const char *envelope_path,
                                          const char *response_path,
                                          KalyxValidationResult *out);

#ifdef __cplusplus
}
#endif

#endif
