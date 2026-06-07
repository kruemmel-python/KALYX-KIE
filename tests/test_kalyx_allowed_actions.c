#include "kalyx_interaction.h"
#include "kalyx_response.h"
#include <string.h>

int main(void) {
    KalyxEnvelopeInput in;
    KalyxBuffer env = {0};
    KalyxValidationResult vr;
    const char *bad =
        "# KALYX LLM Response\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"x\",\"risk\":\"low\",\"requires_confirmation\":true,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"delete_everything\"}}\n"
        "```\n";
    const char *good =
        "# KALYX LLM Response\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"x\",\"risk\":\"low\",\"requires_confirmation\":true,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"html\"}}}\n"
        "```\n";
    memset(&in, 0, sizeof(in));
    in.domain = KALYX_DOMAIN_APP;
    in.user_request = "x";
    in.runtime_state_json = "{}";
    in.authoritative_data_json = "{}";
    if (kalyx_make_envelope(&in, &env) != KALYX_OK) return 1;
    if (kalyx_validate_response_text(env.data, bad, &vr) != KALYX_ERR_UNKNOWN_ACTION) return 2;
    if (kalyx_validate_response_text(env.data, good, &vr) != KALYX_OK || strcmp(vr.command_name, "emit_ui_command") != 0) return 3;
    kalyx_buffer_free(&env);
    return 0;
}
