#include "kalyx_interaction.h"
#include "kalyx_response.h"
#include <string.h>

static int make_env(KalyxBuffer *env) {
    KalyxEnvelopeInput in;
    memset(&in, 0, sizeof(in));
    in.domain = KALYX_DOMAIN_APP;
    in.intent = "export_document";
    in.user_request = "Export.";
    in.runtime_state_json = "{\"current_file\":\"notes.md\"}";
    in.authoritative_data_json = "{\"available_exports\":[\"html\"]}";
    return kalyx_make_envelope(&in, env) == KALYX_OK;
}

int main(void) {
    KalyxBuffer env = {0};
    KalyxValidationResult vr;
    const char *no_json = "# KALYX LLM Response\n\n## Human Response\nNo machine block.\n";
    const char *bad_schema =
        "# KALYX LLM Response\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"BAD\",\"type\":\"answer\",\"human_summary\":\"x\",\"risk\":\"none\",\"requires_confirmation\":false,\"uses_only_provided_context\":true,\"machine_result\":{}}\n"
        "```\n";
    const char *good =
        "# KALYX LLM Response\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"answer\",\"human_summary\":\"x\",\"risk\":\"none\",\"requires_confirmation\":false,\"uses_only_provided_context\":true,\"machine_result\":{\"answer\":\"ok\"}}\n"
        "```\n";
    if (!make_env(&env)) return 1;
    if (kalyx_validate_response_text(env.data, no_json, &vr) != KALYX_ERR_PARSE) return 2;
    if (kalyx_validate_response_text(env.data, bad_schema, &vr) != KALYX_ERR_SCHEMA) return 3;
    if (kalyx_validate_response_text(env.data, good, &vr) != KALYX_OK || !vr.accepted) return 4;
    kalyx_buffer_free(&env);
    return 0;
}
