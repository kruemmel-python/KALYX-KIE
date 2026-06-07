#ifndef KALYX_HOST_H
#define KALYX_HOST_H

#include "kalyx_response.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum KalyxHostDecision {
    KALYX_HOST_REJECT = 0,
    KALYX_HOST_ACCEPT_ANSWER = 1,
    KALYX_HOST_QUEUE_CONFIRMATION = 2,
    KALYX_HOST_DISPATCH_COMMAND = 3
} KalyxHostDecision;

typedef struct KalyxHostPlan {
    KalyxHostDecision decision;
    char command_name[96];
    char reason[256];
} KalyxHostPlan;

KalyxStatus kalyx_host_plan_from_validation(const KalyxValidationResult *validation,
                                            KalyxHostPlan *out_plan);
const char *kalyx_host_decision_name(KalyxHostDecision decision);

#ifdef __cplusplus
}
#endif

#endif
