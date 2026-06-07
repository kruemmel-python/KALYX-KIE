#include "kalyx_interaction.h"
#include "kalyx_response.h"
#include <string.h>

int main(void) {
    KalyxEnvelopeInput in;
    KalyxBuffer env = {0};
    KalyxValidationResult vr;
    const char *resp =
        "# KALYX LLM Response\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"x\",\"risk\":\"low\",\"requires_confirmation\":true,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"execute_external_command\",\"target\":\"shell\"}}\n"
        "```\n";
    memset(&in, 0, sizeof(in));
    in.domain = KALYX_DOMAIN_APP;
    in.user_request = "x";
    in.runtime_state_json = "{}";
    in.authoritative_data_json = "{}";
    if (kalyx_make_envelope(&in, &env) != KALYX_OK) return 1;
    if (kalyx_validate_response_text(env.data, resp, &vr) != KALYX_ERR_FORBIDDEN_ACTION) return 2;
    kalyx_buffer_free(&env);
    return 0;
}
