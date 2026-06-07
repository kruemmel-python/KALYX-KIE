#include "kalyx_dispatch.h"
#include "kalyx_common.h"

#include <stdio.h>
#include <string.h>

static void join_path(char *out, size_t cap, const char *a, const char *b) {
    snprintf(out, cap, "%s/%s", a, b);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";
    char env[512], resp[512], sandbox[512], dispatch_audit[512];
    KalyxDispatchResult dr;
    const char *envelope =
        "---\n"
        "schema: KIE01\n"
        "system: KALYX\n"
        "domain: app\n"
        "intent: host_dispatch_demo\n"
        "authority: application_state\n"
        "response_contract: KRESP01\n"
        "audit_contract: KAUDIT01\n"
        "---\n\n"
        "# KALYX Interaction Envelope\n\n"
        "## Allowed Actions\n\n"
        "```json\n"
        "[\n"
        "  {\"name\":\"emit_ui_command\",\"args\":{\"target\":[\"export_document\",\"open_preview\"],\"mode\":[\"html\",\"markdown\"],\"theme\":[\"dark\",\"plain\"]}}\n"
        "]\n"
        "```\n\n"
        "## Validation Rules\n\n"
        "```json\n"
        "[\n"
        "  {\"kind\":\"requires_confirmation_if_risk_at_least\",\"risk\":\"high\"},\n"
        "  {\"kind\":\"forbidden_command\",\"name\":\"execute_external_command\"},\n"
        "  {\"kind\":\"forbidden_target\",\"name\":\"delete_all_files\"}\n"
        "]\n"
        "```\n\n"
        "## Runtime State\n\n"
        "```json\n{\"app\":\"demo\"}\n```\n";
    const char *response =
        "# KALYX LLM Response\n\n"
        "## Human Response\n\n"
        "I propose a sandbox export.\n\n"
        "## Machine Result\n\n"
        "```json\n"
        "{\n"
        "  \"schema\": \"KRESP01\",\n"
        "  \"type\": \"command\",\n"
        "  \"human_summary\": \"Export a sandbox markdown copy.\",\n"
        "  \"risk\": \"low\",\n"
        "  \"requires_confirmation\": false,\n"
        "  \"uses_only_provided_context\": true,\n"
        "  \"machine_result\": {\n"
        "    \"command\": \"emit_ui_command\",\n"
        "    \"target\": \"export_document\",\n"
        "    \"args\": {\"mode\": \"markdown\", \"theme\": \"dark\"}\n"
        "  }\n"
        "}\n"
        "```\n";
    join_path(env, sizeof(env), dir, "dispatch_test.kie.md");
    join_path(resp, sizeof(resp), dir, "dispatch_test.kresp.md");
    join_path(sandbox, sizeof(sandbox), dir, "dispatch_sandbox");
    join_path(dispatch_audit, sizeof(dispatch_audit), dir, "dispatch_test.kdispatch.json");
    if (kalyx_write_text_file(env, envelope) != KALYX_OK) return 1;
    if (kalyx_write_text_file(resp, response) != KALYX_OK) return 2;
    if (kalyx_dispatch_sandbox_files(env, resp, sandbox, &dr) != KALYX_OK) return 3;
    if (dr.decision != KALYX_DISPATCH_SANDBOX_EXECUTED) return 4;
    if (strcmp(dr.command, "emit_ui_command") != 0) return 5;
    if (strcmp(dr.target, "export_document") != 0) return 6;
    if (!strstr(dr.artifact_file, "export_document_sandbox.md")) return 7;
    if (kalyx_write_dispatch_audit_file(&dr, dispatch_audit) != KALYX_OK) return 8;
    return 0;
}
