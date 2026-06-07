#include "kalyx_host.h"
#include <string.h>

int main(void) {
    KalyxValidationResult vr;
    KalyxHostPlan plan;
    memset(&vr, 0, sizeof(vr));
    vr.status = KALYX_OK;
    vr.accepted = 1;
    vr.response_type = KALYX_RESPONSE_COMMAND;
    vr.requires_confirmation = 1;
    strcpy(vr.command_name, "emit_ui_command");
    if (kalyx_host_plan_from_validation(&vr, &plan) != KALYX_OK) return 1;
    if (plan.decision != KALYX_HOST_QUEUE_CONFIRMATION) return 2;
    if (strcmp(plan.command_name, "emit_ui_command") != 0) return 3;
    vr.requires_confirmation = 0;
    if (kalyx_host_plan_from_validation(&vr, &plan) != KALYX_OK) return 4;
    if (plan.decision != KALYX_HOST_DISPATCH_COMMAND) return 5;
    return 0;
}
