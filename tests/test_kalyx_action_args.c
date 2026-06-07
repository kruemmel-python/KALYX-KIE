#include "kalyx_interaction.h"
#include "kalyx_response.h"
#include <string.h>

static int make_env(KalyxBuffer *env) {
    KalyxEnvelopeInput in;
    memset(&in, 0, sizeof(in));
    in.domain = KALYX_DOMAIN_APP;
    in.user_request = "Export as html.";
    in.runtime_state_json = "{}";
    in.authoritative_data_json = "{}";
    return kalyx_make_envelope(&in, env) == KALYX_OK;
}

int main(void) {
    KalyxBuffer env = {0};
    KalyxValidationResult vr;
    const char *bad_target =
        "# R\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"x\",\"risk\":\"low\",\"requires_confirmation\":true,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"emit_ui_command\",\"target\":\"delete_all_files\",\"args\":{\"mode\":\"html\"}}}\n"
        "```\n";
    const char *bad_arg =
        "# R\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"x\",\"risk\":\"low\",\"requires_confirmation\":true,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"exe\"}}}\n"
        "```\n";
    const char *good =
        "# R\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"x\",\"risk\":\"low\",\"requires_confirmation\":true,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"html\",\"theme\":\"dark\"}}}\n"
        "```\n";
    if (!make_env(&env)) return 1;
    if (kalyx_validate_response_text(env.data, bad_target, &vr) != KALYX_ERR_FORBIDDEN_ACTION) return 2;
    if (kalyx_validate_response_text(env.data, bad_arg, &vr) != KALYX_ERR_RESPONSE_REJECTED) return 3;
    if (kalyx_validate_response_text(env.data, good, &vr) != KALYX_OK) return 4;
    kalyx_buffer_free(&env);
    return 0;
}
