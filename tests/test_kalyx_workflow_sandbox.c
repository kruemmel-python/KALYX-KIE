#include "kalyx_dispatch.h"
#include "kalyx_common.h"

#include <stdio.h>
#include <string.h>

static void join_path(char *out, size_t cap, const char *a, const char *b) {
    snprintf(out, cap, "%s/%s", a, b);
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int main(int argc, char **argv) {
    char env[512], resp[512], bad_resp[512], sandbox[512], bad_sandbox[512], audit[512];
    KalyxDispatchResult dr;
    const char *dir = argc > 1 ? argv[1] : ".";
    const char *env_text =
        "---\n"
        "schema: KIE01\n"
        "system: KALYX\n"
        "domain: app\n"
        "intent: workflow_sandbox\n"
        "authority: application_state\n"
        "response_contract: KRESP01\n"
        "audit_contract: KAUDIT01\n"
        "---\n\n"
        "# KALYX Interaction Envelope\n\n"
        "## Allowed Actions\n\n"
        "```json\n"
        "[\n"
        "  {\"name\":\"workflow\",\"args\":{\"target\":[\"multi_action_sandbox\"],\"mode\":[\"markdown\"],\"theme\":[\"plain\"]}},\n"
        "  {\"name\":\"emit_ui_command\",\"args\":{\"target\":[\"export_document\",\"open_preview\",\"save_as\"],\"mode\":[\"markdown\"],\"theme\":[\"plain\"]}}\n"
        "]\n"
        "```\n\n"
        "## Forbidden Actions\n\n"
        "```json\n"
        "[{\"kind\":\"forbidden_target\",\"name\":\"delete_all_files\"}]\n"
        "```\n\n"
        "## Validation Rules\n\n"
        "```json\n"
        "[{\"kind\":\"forbidden_target\",\"name\":\"delete_all_files\"}]\n"
        "```\n\n"
        "## Runtime State\n\n```json\n{\"app\":\"test\"}\n```\n";
    const char *resp_text =
        "# KALYX LLM Response\n\n## Human Response\n\nWorkflow.\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"Workflow\",\"risk\":\"low\",\"requires_confirmation\":false,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"workflow\",\"target\":\"multi_action_sandbox\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"},\"workflow\":[{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}},{\"command\":\"emit_ui_command\",\"target\":\"open_preview\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}},{\"command\":\"emit_ui_command\",\"target\":\"save_as\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}}]}}\n"
        "```\n";
    const char *bad_resp_text =
        "# KALYX LLM Response\n\n## Human Response\n\nBad.\n\n## Machine Result\n\n```json\n"
        "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"Bad\",\"risk\":\"low\",\"requires_confirmation\":false,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"workflow\",\"target\":\"multi_action_sandbox\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"},\"workflow\":[{\"command\":\"emit_ui_command\",\"target\":\"delete_all_files\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}}]}}\n"
        "```\n";

    join_path(env, sizeof(env), dir, "workflow_test.kie.md");
    join_path(resp, sizeof(resp), dir, "workflow_test.kresp.md");
    join_path(bad_resp, sizeof(bad_resp), dir, "workflow_bad.kresp.md");
    join_path(sandbox, sizeof(sandbox), dir, "workflow_sandbox");
    join_path(bad_sandbox, sizeof(bad_sandbox), dir, "workflow_bad_sandbox");
    join_path(audit, sizeof(audit), dir, "workflow.kdispatch.json");

    if (kalyx_write_text_file(env, env_text) != KALYX_OK) return 1;
    if (kalyx_write_text_file(resp, resp_text) != KALYX_OK) return 2;
    if (kalyx_write_text_file(bad_resp, bad_resp_text) != KALYX_OK) return 3;

    memset(&dr, 0, sizeof(dr));
    if (kalyx_dispatch_workflow_sandbox_files(env, resp, sandbox, &dr) != KALYX_OK) return 4;
    if (dr.workflow_step_count != 3u || dr.workflow_executed_count != 3u) return 5;
    if (strcmp(dr.command, "workflow") != 0 || strcmp(dr.target, "multi_action_sandbox") != 0) return 6;
    if (!file_exists(dr.artifact_file)) return 7;
    if (kalyx_write_dispatch_audit_file(&dr, audit) != KALYX_OK) return 8;
    if (!file_exists(audit)) return 9;

    memset(&dr, 0, sizeof(dr));
    if (kalyx_dispatch_workflow_sandbox_files(env, bad_resp, bad_sandbox, &dr) == KALYX_OK) return 10;
    if (dr.workflow_executed_count != 0u) return 11;
    return 0;
}
