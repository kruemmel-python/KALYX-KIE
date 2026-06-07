#include "kalyx_dispatch.h"
#include "kalyx_common.h"

#include <stdio.h>
#include <string.h>

static void join_path(char *out, size_t cap, const char *a, const char *b) {
    snprintf(out, cap, "%s/%s", a, b);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";
    char env[512], resp[512], sandbox[512], artifact[512], audit[512];
    KalyxDispatchResult dr;
    const char *envelope =
        "---\n"
        "schema: KIE01\n"
        "system: KALYX\n"
        "domain: app\n"
        "intent: strict_dispatch_gate\n"
        "authority: application_state\n"
        "response_contract: KRESP01\n"
        "audit_contract: KAUDIT01\n"
        "---\n\n"
        "# KALYX Interaction Envelope\n\n"
        "## Allowed Actions\n\n"
        "```json\n"
        "[\n"
        "  {\"name\":\"emit_ui_command\",\"args\":{\"target\":[\"export_document\"],\"mode\":[\"markdown\"],\"theme\":[\"plain\"]}}\n"
        "]\n"
        "```\n\n"
        "## Validation Rules\n\n"
        "```json\n"
        "[{\"kind\":\"forbidden_target\",\"name\":\"delete_all_files\"}]\n"
        "```\n\n"
        "## Runtime State\n\n"
        "```json\n{\"app\":\"demo\"}\n```\n";
    const char *invalid_response =
        "# KALYX LLM Response\n\n"
        "## Human Response\n\n"
        "This looks plausible but uses an invalid response type.\n\n"
        "## Machine Result\n\n"
        "```json\n"
        "{\n"
        "  \"schema\": \"KRESP01\",\n"
        "  \"type\": \"action_proposal\",\n"
        "  \"human_summary\": \"Invalid proposal must not dispatch.\",\n"
        "  \"risk\": \"low\",\n"
        "  \"requires_confirmation\": false,\n"
        "  \"uses_only_provided_context\": true,\n"
        "  \"machine_result\": {\"action\": \"emit_ui_command\"}\n"
        "}\n"
        "```\n";

    join_path(env, sizeof(env), dir, "strict_gate.kie.md");
    join_path(resp, sizeof(resp), dir, "strict_gate_invalid.kresp.md");
    join_path(sandbox, sizeof(sandbox), dir, "strict_gate_sandbox");
    join_path(artifact, sizeof(artifact), sandbox, "export_document_sandbox.md");
    join_path(audit, sizeof(audit), dir, "strict_gate_reject.kdispatch.json");

    if (kalyx_write_text_file(env, envelope) != KALYX_OK) return 1;
    if (kalyx_write_text_file(resp, invalid_response) != KALYX_OK) return 2;
    if (kalyx_dispatch_sandbox_files(env, resp, sandbox, &dr) == KALYX_OK) return 3;
    if (dr.decision != KALYX_DISPATCH_REJECT) return 4;
    if (!strstr(dr.reason, "response type") && !strstr(dr.reason, "invalid")) return 5;
    if (kalyx_write_dispatch_audit_file(&dr, audit) != KALYX_OK) return 6;
    {
        FILE *f = fopen(artifact, "rb");
        if (f) { fclose(f); return 7; }
    }
    return 0;
}
