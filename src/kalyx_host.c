#include "kalyx_host.h"

#include <stdio.h>
#include <string.h>

const char *kalyx_host_decision_name(KalyxHostDecision decision) {
    switch (decision) {
        case KALYX_HOST_REJECT: return "reject";
        case KALYX_HOST_ACCEPT_ANSWER: return "accept_answer";
        case KALYX_HOST_QUEUE_CONFIRMATION: return "queue_confirmation";
        case KALYX_HOST_DISPATCH_COMMAND: return "dispatch_command";
        default: return "unknown";
    }
}

KalyxStatus kalyx_host_plan_from_validation(const KalyxValidationResult *validation,
                                            KalyxHostPlan *out_plan) {
    if (!validation || !out_plan) return KALYX_ERR_INVALID_ARGUMENT;
    memset(out_plan, 0, sizeof(*out_plan));
    if (!validation->accepted || validation->status != KALYX_OK) {
        out_plan->decision = KALYX_HOST_REJECT;
        snprintf(out_plan->reason, sizeof(out_plan->reason), "%s", validation->error[0] ? validation->error : "validation rejected response");
        return KALYX_OK;
    }
    if (validation->requires_confirmation) {
        out_plan->decision = KALYX_HOST_QUEUE_CONFIRMATION;
        snprintf(out_plan->command_name, sizeof(out_plan->command_name), "%s", validation->command_name);
        snprintf(out_plan->reason, sizeof(out_plan->reason), "host confirmation required before dispatch");
        return KALYX_OK;
    }
    if (validation->response_type == KALYX_RESPONSE_COMMAND) {
        out_plan->decision = KALYX_HOST_DISPATCH_COMMAND;
        snprintf(out_plan->command_name, sizeof(out_plan->command_name), "%s", validation->command_name);
        snprintf(out_plan->reason, sizeof(out_plan->reason), "validated command may be dispatched by host");
        return KALYX_OK;
    }
    out_plan->decision = KALYX_HOST_ACCEPT_ANSWER;
    snprintf(out_plan->reason, sizeof(out_plan->reason), "validated non-command response");
    return KALYX_OK;
}
