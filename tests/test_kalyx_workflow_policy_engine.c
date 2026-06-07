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

static int file_contains(const char *path, const char *needle) {
    KalyxBuffer b = {0};
    int ok;
    if (kalyx_read_text_file(path, &b) != KALYX_OK) return 0;
    ok = b.data && strstr(b.data, needle) != 0;
    kalyx_buffer_free(&b);
    return ok;
}

static const char *env_text =
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
    "## Forbidden Actions\n\n```json\n[]\n```\n\n"
    "## Validation Rules\n\n```json\n[]\n```\n\n"
    "## Runtime State\n\n```json\n{\"app\":\"test\"}\n```\n";

static const char *valid_resp =
    "# KALYX LLM Response\n\n## Human Response\n\nWorkflow.\n\n## Machine Result\n\n```json\n"
    "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"Workflow\",\"risk\":\"low\",\"requires_confirmation\":false,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"workflow\",\"target\":\"multi_action_sandbox\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"},\"workflow\":[{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}},{\"command\":\"emit_ui_command\",\"target\":\"open_preview\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}},{\"command\":\"emit_ui_command\",\"target\":\"save_as\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}}]}}\n"
    "```\n";

static const char *order_resp =
    "# KALYX LLM Response\n\n## Human Response\n\nBad order.\n\n## Machine Result\n\n```json\n"
    "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"Bad order\",\"risk\":\"low\",\"requires_confirmation\":false,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"workflow\",\"target\":\"multi_action_sandbox\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"},\"workflow\":[{\"command\":\"emit_ui_command\",\"target\":\"open_preview\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}},{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}}]}}\n"
    "```\n";

static const char *duplicate_resp =
    "# KALYX LLM Response\n\n## Human Response\n\nDuplicate.\n\n## Machine Result\n\n```json\n"
    "{\"schema\":\"KRESP01\",\"type\":\"command\",\"human_summary\":\"Duplicate\",\"risk\":\"low\",\"requires_confirmation\":false,\"uses_only_provided_context\":true,\"machine_result\":{\"command\":\"workflow\",\"target\":\"multi_action_sandbox\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"},\"workflow\":[{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}},{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}}]}}\n"
    "```\n";

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";
    char env[512], valid[512], order[512], dup[512], sandbox[512], order_sandbox[512], dup_sandbox[512];
    KalyxDispatchResult dr;

    join_path(env, sizeof(env), dir, "workflow_policy.kie.md");
    join_path(valid, sizeof(valid), dir, "workflow_policy_valid.kresp.md");
    join_path(order, sizeof(order), dir, "workflow_policy_order.kresp.md");
    join_path(dup, sizeof(dup), dir, "workflow_policy_duplicate.kresp.md");
    join_path(sandbox, sizeof(sandbox), dir, "workflow_policy_sandbox");
    join_path(order_sandbox, sizeof(order_sandbox), dir, "workflow_policy_order_sandbox");
    join_path(dup_sandbox, sizeof(dup_sandbox), dir, "workflow_policy_duplicate_sandbox");

    if (kalyx_write_text_file(env, env_text) != KALYX_OK) return 1;
    if (kalyx_write_text_file(valid, valid_resp) != KALYX_OK) return 2;
    if (kalyx_write_text_file(order, order_resp) != KALYX_OK) return 3;
    if (kalyx_write_text_file(dup, duplicate_resp) != KALYX_OK) return 4;

    memset(&dr, 0, sizeof(dr));
    if (kalyx_dispatch_workflow_sandbox_files(env, valid, sandbox, &dr) != KALYX_OK) return 10;
    if (dr.workflow_step_count != 3u || dr.workflow_executed_count != 3u) return 11;
    if (!file_exists(dr.artifact_file)) return 12;
    if (!file_contains(dr.artifact_file, "\"status\": \"ok\"")) return 13;
    if (!file_contains(dr.artifact_file, "\"status\": \"ok\"")) return 14;

    memset(&dr, 0, sizeof(dr));
    if (kalyx_dispatch_workflow_sandbox_files(env, order, order_sandbox, &dr) == KALYX_OK) return 20;
    if (dr.workflow_executed_count != 0u) return 21;
    if (!file_exists(dr.artifact_file)) return 22;
    if (!file_contains(dr.artifact_file, "\"status\": \"aborted\"")) return 23;
    if (!file_contains(dr.artifact_file, "requires previous successful export_document")) return 24;

    memset(&dr, 0, sizeof(dr));
    if (kalyx_dispatch_workflow_sandbox_files(env, dup, dup_sandbox, &dr) == KALYX_OK) return 30;
    if (dr.workflow_executed_count != 0u) return 31;
    if (!file_exists(dr.artifact_file)) return 32;
    if (!file_contains(dr.artifact_file, "duplicate workflow target")) return 33;

    return 0;
}
