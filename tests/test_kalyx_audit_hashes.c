#include "kalyx_audit.h"
#include "kalyx_hash.h"
#include "kalyx_interaction.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    char env_path[512], resp_path[512], audit_path[512], hash[KALYX_SHA256_HEX_BYTES];
    KalyxEnvelopeInput in;
    KalyxValidationResult vr;
    KalyxAuditInput ai;
    KalyxBuffer audit = {0};
    const char *resp =
        "# KALYX LLM Response\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"answer\",\"human_summary\":\"x\",\"risk\":\"none\",\"requires_confirmation\":false,\"uses_only_provided_context\":true,\"machine_result\":{\"answer\":\"ok\"}}\n"
        "```\n";
    if (argc != 2) return 1;
    snprintf(env_path, sizeof(env_path), "%s/kalyx_audit_test.kie.md", argv[1]);
    snprintf(resp_path, sizeof(resp_path), "%s/kalyx_audit_test.kresp.md", argv[1]);
    snprintf(audit_path, sizeof(audit_path), "%s/kalyx_audit_test.kaudit.json", argv[1]);
    memset(&in, 0, sizeof(in));
    in.domain = KALYX_DOMAIN_APP; in.user_request = "x"; in.runtime_state_json = "{}"; in.authoritative_data_json = "{}";
    if (kalyx_make_envelope_file(&in, env_path) != KALYX_OK) return 2;
    if (kalyx_write_text_file(resp_path, resp) != KALYX_OK) return 3;
    if (kalyx_validate_response_files(env_path, resp_path, &vr) != KALYX_OK) return 4;
    memset(&ai, 0, sizeof(ai)); ai.envelope_file = env_path; ai.response_file = resp_path; ai.validation = &vr;
    if (kalyx_write_audit_file(&ai, audit_path) != KALYX_OK) return 5;
    if (kalyx_sha256_file_hex(env_path, hash) != KALYX_OK) return 6;
    if (kalyx_read_text_file(audit_path, &audit) != KALYX_OK) return 7;
    if (!strstr(audit.data, hash) || !strstr(audit.data, "\"validated\": true")) return 8;
    kalyx_buffer_free(&audit);
    return 0;
}
