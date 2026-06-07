#include "kalyx_dispatch.h"

#include "kalyx_common.h"
#include "kalyx_host.h"
#include "kalyx_json.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

static int mkdir_one(const char *path) {
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0777) == 0 || errno == EEXIST;
#endif
}

static int ensure_dir_path(const char *path) {
    char buf[4096];
    size_t len;
    size_t i;
    if (!path || !*path) return 0;
    len = strlen(path);
    if (len >= sizeof(buf)) return 0;
    memcpy(buf, path, len + 1u);
    for (i = 0u; i < len; i++) if (buf[i] == '\\') buf[i] = '/';
    for (i = 0u; i < len; i++) {
        if (buf[i] == '/') {
            if (i == 0u) continue;
            if (i == 2u && buf[1] == ':') continue;
            buf[i] = '\0';
            if (buf[0] != '\0' && !mkdir_one(buf)) return 0;
            buf[i] = '/';
        }
    }
    return mkdir_one(buf);
}

static int ensure_parent_directory(const char *path) {
    char buf[4096];
    size_t len;
    size_t i;
    if (!path || !*path) return 0;
    len = strlen(path);
    if (len >= sizeof(buf)) return 0;
    memcpy(buf, path, len + 1u);
    for (i = 0u; i < len; i++) if (buf[i] == '\\') buf[i] = '/';
    for (i = 0u; i < len; i++) {
        if (buf[i] == '/') {
            if (i == 0u) continue;
            if (i == 2u && buf[1] == ':') continue;
            buf[i] = '\0';
            if (buf[0] != '\0' && !mkdir_one(buf)) return 0;
            buf[i] = '/';
        }
    }
    return 1;
}

static void json_string(FILE *f, const char *s) {
    const unsigned char *p = (const unsigned char *)(s ? s : "");
    fputc('"', f);
    while (*p) {
        switch (*p) {
            case '\\': fputs("\\\\", f); break;
            case '"': fputs("\\\"", f); break;
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            default:
                if (*p < 32u) fprintf(f, "\\u%04x", (unsigned int)*p);
                else fputc(*p, f);
                break;
        }
        p++;
    }
    fputc('"', f);
}

static KalyxStatus write_text_artifact(const char *path, const char *text) {
    if (!ensure_parent_directory(path)) return KALYX_ERR_IO;
    return kalyx_write_text_file(path, text ? text : "");
}

static KalyxStatus parse_machine_result(const char *response_path,
                                        char *type, size_t type_cap,
                                        char *summary, size_t summary_cap,
                                        char *command, size_t command_cap,
                                        char *target, size_t target_cap,
                                        char *mode, size_t mode_cap,
                                        char *theme, size_t theme_cap) {
    KalyxBuffer resp = {0};
    KalyxBuffer json = {0};
    KalyxJsonDocument doc = {0};
    const KalyxJsonNode *root;
    const char *s;
    KalyxStatus st;
    if (!response_path) return KALYX_ERR_INVALID_ARGUMENT;
    st = kalyx_read_text_file(response_path, &resp);
    if (st != KALYX_OK) return st;
    st = kalyx_extract_markdown_json_block(resp.data, "Machine Result", &json);
    kalyx_buffer_free(&resp);
    if (st != KALYX_OK) return st;
    st = kalyx_json_parse(json.data, &doc);
    kalyx_buffer_free(&json);
    if (st != KALYX_OK) return st;
    root = kalyx_json_root(&doc);
    if ((s = kalyx_json_string_value(kalyx_json_path(root, "type"))) && type_cap) snprintf(type, type_cap, "%s", s);
    if ((s = kalyx_json_string_value(kalyx_json_path(root, "human_summary"))) && summary_cap) snprintf(summary, summary_cap, "%s", s);
    if ((s = kalyx_json_string_value(kalyx_json_path(root, "machine_result.command"))) && command_cap) snprintf(command, command_cap, "%s", s);
    if ((s = kalyx_json_string_value(kalyx_json_path(root, "machine_result.target"))) && target_cap) snprintf(target, target_cap, "%s", s);
    if ((s = kalyx_json_string_value(kalyx_json_path(root, "machine_result.args.mode"))) && mode_cap) snprintf(mode, mode_cap, "%s", s);
    if ((s = kalyx_json_string_value(kalyx_json_path(root, "machine_result.args.theme"))) && theme_cap) snprintf(theme, theme_cap, "%s", s);
    kalyx_json_free(&doc);
    return KALYX_OK;
}

typedef struct WorkflowStepSpec {
    char command[96];
    char target[96];
    char mode[64];
    char theme[64];
    char artifact[512];
    char status[64];
    char reason[256];
} WorkflowStepSpec;

#define KALYX_WORKFLOW_MAX_STEPS 8u

static void node_string_to(char *dst, size_t cap, const KalyxJsonNode *node) {
    const char *s = kalyx_json_string_value(node);
    if (dst && cap) snprintf(dst, cap, "%s", s ? s : "");
}

static int workflow_step_is_allowed(const WorkflowStepSpec *step) {
    if (!step) return 0;
    if (strcmp(step->command, "emit_ui_command") != 0) return 0;
    if (strcmp(step->target, "export_document") != 0 &&
        strcmp(step->target, "open_preview") != 0 &&
        strcmp(step->target, "save_as") != 0) return 0;
    if (step->mode[0] && strcmp(step->mode, "markdown") != 0 && strcmp(step->mode, "html") != 0 && strcmp(step->mode, "pdf") != 0 && strcmp(step->mode, "reveal") != 0) return 0;
    if (step->theme[0] && strcmp(step->theme, "plain") != 0 && strcmp(step->theme, "dark") != 0 && strcmp(step->theme, "scientific") != 0 && strcmp(step->theme, "rpg") != 0) return 0;
    return 1;
}

static KalyxStatus parse_workflow_steps(const char *response_path,
                                        WorkflowStepSpec *steps,
                                        size_t max_steps,
                                        size_t *out_count,
                                        char *error,
                                        size_t error_cap) {
    KalyxBuffer resp = {0};
    KalyxBuffer json = {0};
    KalyxJsonDocument doc = {0};
    const KalyxJsonNode *root;
    const KalyxJsonNode *workflow;
    const char *command;
    const char *target;
    size_t i, n;
    KalyxStatus st;
    if (!response_path || !steps || !out_count) return KALYX_ERR_INVALID_ARGUMENT;
    *out_count = 0u;
    st = kalyx_read_text_file(response_path, &resp);
    if (st != KALYX_OK) return st;
    st = kalyx_extract_markdown_json_block(resp.data, "Machine Result", &json);
    kalyx_buffer_free(&resp);
    if (st != KALYX_OK) return st;
    st = kalyx_json_parse(json.data, &doc);
    kalyx_buffer_free(&json);
    if (st != KALYX_OK) return st;
    root = kalyx_json_root(&doc);
    command = kalyx_json_string_value(kalyx_json_path(root, "machine_result.command"));
    target = kalyx_json_string_value(kalyx_json_path(root, "machine_result.target"));
    if (!command || strcmp(command, "workflow") != 0 || !target || strcmp(target, "multi_action_sandbox") != 0) {
        if (error && error_cap) snprintf(error, error_cap, "machine_result is not a workflow/multi_action_sandbox command");
        kalyx_json_free(&doc);
        return KALYX_ERR_RESPONSE_REJECTED;
    }
    workflow = kalyx_json_path(root, "machine_result.workflow");
    if (!workflow || kalyx_json_type(workflow) != KALYX_JSON_ARRAY) {
        if (error && error_cap) snprintf(error, error_cap, "machine_result.workflow array is missing");
        kalyx_json_free(&doc);
        return KALYX_ERR_MISSING_REQUIRED_FIELD;
    }
    n = kalyx_json_array_size(workflow);
    if (n == 0u || n > max_steps || n > KALYX_WORKFLOW_MAX_STEPS) {
        if (error && error_cap) snprintf(error, error_cap, "workflow step count outside policy bounds");
        kalyx_json_free(&doc);
        return KALYX_ERR_RESPONSE_REJECTED;
    }
    for (i = 0u; i < n; i++) {
        const KalyxJsonNode *step = kalyx_json_array_get(workflow, i);
        memset(&steps[i], 0, sizeof(steps[i]));
        snprintf(steps[i].status, sizeof(steps[i].status), "pending");
        snprintf(steps[i].reason, sizeof(steps[i].reason), "not executed yet");
        node_string_to(steps[i].command, sizeof(steps[i].command), kalyx_json_object_get(step, "command"));
        node_string_to(steps[i].target, sizeof(steps[i].target), kalyx_json_object_get(step, "target"));
        node_string_to(steps[i].mode, sizeof(steps[i].mode), kalyx_json_path(step, "args.mode"));
        node_string_to(steps[i].theme, sizeof(steps[i].theme), kalyx_json_path(step, "args.theme"));
        if (!workflow_step_is_allowed(&steps[i])) {
            if (error && error_cap) snprintf(error, error_cap, "workflow step outside sandbox allowlist");
            kalyx_json_free(&doc);
            return KALYX_ERR_RESPONSE_REJECTED;
        }
    }
    *out_count = n;
    kalyx_json_free(&doc);
    return KALYX_OK;
}

static int workflow_target_rank(const char *target) {
    if (!target) return -1;
    if (strcmp(target, "export_document") == 0) return 1;
    if (strcmp(target, "open_preview") == 0) return 2;
    if (strcmp(target, "save_as") == 0) return 3;
    return -1;
}

static KalyxStatus validate_workflow_policy(WorkflowStepSpec *steps,
                                            size_t count,
                                            char *error,
                                            size_t error_cap) {
    size_t i, j;
    int seen_export = 0;
    int seen_open = 0;
    int last_rank = 0;
    if (!steps || count == 0u || count > KALYX_WORKFLOW_MAX_STEPS) {
        if (error && error_cap) snprintf(error, error_cap, "workflow step count outside policy bounds");
        return KALYX_ERR_RESPONSE_REJECTED;
    }
    for (i = 0u; i < count; i++) {
        int rank = workflow_target_rank(steps[i].target);
        if (rank < 0) {
            snprintf(steps[i].status, sizeof(steps[i].status), "rejected");
            snprintf(steps[i].reason, sizeof(steps[i].reason), "forbidden workflow target");
            if (error && error_cap) snprintf(error, error_cap, "workflow step outside sandbox allowlist");
            return KALYX_ERR_RESPONSE_REJECTED;
        }
        for (j = 0u; j < i; j++) {
            if (strcmp(steps[j].target, steps[i].target) == 0) {
                snprintf(steps[i].status, sizeof(steps[i].status), "rejected");
                snprintf(steps[i].reason, sizeof(steps[i].reason), "duplicate workflow target");
                if (error && error_cap) snprintf(error, error_cap, "duplicate workflow step target: %s", steps[i].target);
                return KALYX_ERR_RESPONSE_REJECTED;
            }
        }
        if (rank < last_rank) {
            snprintf(steps[i].status, sizeof(steps[i].status), "rejected");
            snprintf(steps[i].reason, sizeof(steps[i].reason), "workflow step order violation");
            if (error && error_cap) snprintf(error, error_cap, "workflow step order violation");
            return KALYX_ERR_RESPONSE_REJECTED;
        }
        if ((strcmp(steps[i].target, "open_preview") == 0 || strcmp(steps[i].target, "save_as") == 0) && !seen_export) {
            snprintf(steps[i].status, sizeof(steps[i].status), "rejected");
            snprintf(steps[i].reason, sizeof(steps[i].reason), "requires previous successful export_document");
            if (error && error_cap) snprintf(error, error_cap, "workflow dependency violation: export_document required before %s", steps[i].target);
            return KALYX_ERR_RESPONSE_REJECTED;
        }
        if (strcmp(steps[i].target, "save_as") == 0 && !seen_open) {
            snprintf(steps[i].status, sizeof(steps[i].status), "rejected");
            snprintf(steps[i].reason, sizeof(steps[i].reason), "requires previous successful open_preview");
            if (error && error_cap) snprintf(error, error_cap, "workflow dependency violation: open_preview required before save_as");
            return KALYX_ERR_RESPONSE_REJECTED;
        }
        last_rank = rank;
        if (strcmp(steps[i].target, "export_document") == 0) seen_export = 1;
        if (strcmp(steps[i].target, "open_preview") == 0) seen_open = 1;
    }
    return KALYX_OK;
}

static KalyxStatus write_workflow_step_artifact(const char *sandbox_dir, size_t index, WorkflowStepSpec *step) {
    char path[1024];
    char body[2048];
    const char *mode = step->mode[0] ? step->mode : "markdown";
    const char *theme = step->theme[0] ? step->theme : "plain";
    if (strcmp(step->target, "export_document") == 0) {
        snprintf(path, sizeof(path), "%s/workflow_step_%02u_export_document_sandbox.md", sandbox_dir, (unsigned int)(index + 1u));
        snprintf(body, sizeof(body), "# KALYX Workflow Sandbox Export\n\nThis file is a workflow sandbox artifact. No source file was overwritten.\n\n- step: `%u`\n- command: `%s`\n- target: `%s`\n- mode: `%s`\n- theme: `%s`\n", (unsigned int)(index + 1u), step->command, step->target, mode, theme);
    } else if (strcmp(step->target, "open_preview") == 0) {
        snprintf(path, sizeof(path), "%s/workflow_step_%02u_open_preview_request.json", sandbox_dir, (unsigned int)(index + 1u));
        snprintf(body, sizeof(body), "{\n  \"schema\": \"KDISPATCH01\",\n  \"step\": %u,\n  \"action\": \"open_preview\",\n  \"mode\": \"%s\",\n  \"theme\": \"%s\"\n}\n", (unsigned int)(index + 1u), mode, theme);
    } else if (strcmp(step->target, "save_as") == 0) {
        snprintf(path, sizeof(path), "%s/workflow_step_%02u_save_as_request.json", sandbox_dir, (unsigned int)(index + 1u));
        snprintf(body, sizeof(body), "{\n  \"schema\": \"KDISPATCH01\",\n  \"step\": %u,\n  \"action\": \"save_as\",\n  \"mode\": \"%s\",\n  \"theme\": \"%s\",\n  \"sandbox_only\": true\n}\n", (unsigned int)(index + 1u), mode, theme);
    } else {
        return KALYX_ERR_UNKNOWN_ACTION;
    }
    snprintf(step->artifact, sizeof(step->artifact), "%s", path);
    return write_text_artifact(path, body);
}

static KalyxStatus write_workflow_manifest(const char *sandbox_dir,
                                           const WorkflowStepSpec *steps,
                                           size_t count,
                                           const char *status,
                                           const char *abort_reason,
                                           char *artifact_out,
                                           size_t artifact_cap) {
    char path[1024];
    FILE *f;
    size_t i;
    snprintf(path, sizeof(path), "%s/workflow.kworkflow.json", sandbox_dir);
    if (!ensure_parent_directory(path)) return KALYX_ERR_IO;
    f = fopen(path, "wb");
    if (!f) return KALYX_ERR_IO;
    fputs("{\n  \"schema\": \"KWORKFLOW01\",\n  \"kalyx_version\": \"" KALYX_VERSION "\",\n  \"status\": ", f);
    json_string(f, status ? status : "ok");
    fputs(",\n  \"abort_reason\": ", f);
    json_string(f, abort_reason ? abort_reason : "");
    fputs(",\n  \"step_count\": ", f);
    fprintf(f, "%u", (unsigned int)count);
    fputs(",\n  \"steps\": [\n", f);
    for (i = 0u; i < count; i++) {
        fputs("    {\"index\": ", f); fprintf(f, "%u", (unsigned int)(i + 1u));
        fputs(", \"command\": ", f); json_string(f, steps[i].command);
        fputs(", \"target\": ", f); json_string(f, steps[i].target);
        fputs(", \"status\": ", f); json_string(f, steps[i].status);
        fputs(", \"reason\": ", f); json_string(f, steps[i].reason);
        fputs(", \"artifact\": ", f); json_string(f, steps[i].artifact);
        fputs("}", f);
        if (i + 1u < count) fputc(',', f);
        fputc('\n', f);
    }
    fputs("  ]\n}\n", f);
    if (fclose(f) != 0) return KALYX_ERR_IO;
    if (artifact_out && artifact_cap) snprintf(artifact_out, artifact_cap, "%s", path);
    return KALYX_OK;
}
const char *kalyx_dispatch_decision_name(KalyxDispatchDecision decision) {
    switch (decision) {
        case KALYX_DISPATCH_REJECT: return "reject";
        case KALYX_DISPATCH_SHOW_ANSWER: return "show_answer";
        case KALYX_DISPATCH_QUEUE_CONFIRMATION: return "queue_confirmation";
        case KALYX_DISPATCH_SANDBOX_EXECUTED: return "sandbox_executed";
        default: return "unknown";
    }
}

KalyxStatus kalyx_dispatch_sandbox_files(const char *envelope_path,
                                          const char *response_path,
                                          const char *sandbox_dir,
                                          KalyxDispatchResult *out) {
    KalyxValidationResult vr;
    KalyxHostPlan plan;
    KalyxStatus st;
    char type[64] = {0};
    char summary[1024] = {0};
    char command[96] = {0};
    char target[96] = {0};
    char mode[64] = {0};
    char theme[64] = {0};
    char artifact[1024];
    char body[4096];
    if (!envelope_path || !response_path || !sandbox_dir || !out) return KALYX_ERR_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->status = KALYX_ERR_RESPONSE_REJECTED;
    snprintf(out->sandbox_dir, sizeof(out->sandbox_dir), "%s", sandbox_dir);
    if (!ensure_dir_path(sandbox_dir)) {
        snprintf(out->reason, sizeof(out->reason), "cannot create sandbox directory");
        return KALYX_ERR_IO;
    }
    st = kalyx_validate_response_files(envelope_path, response_path, &vr);
    if (st != KALYX_OK || !vr.accepted) {
        out->decision = KALYX_DISPATCH_REJECT;
        snprintf(out->reason, sizeof(out->reason), "%s", vr.error[0] ? vr.error : "validation rejected response");
        out->status = st;
        return st;
    }
    st = parse_machine_result(response_path, type, sizeof(type), summary, sizeof(summary), command, sizeof(command), target, sizeof(target), mode, sizeof(mode), theme, sizeof(theme));
    if (st != KALYX_OK) {
        out->decision = KALYX_DISPATCH_REJECT;
        snprintf(out->reason, sizeof(out->reason), "cannot parse response Machine Result");
        out->status = st;
        return st;
    }
    snprintf(out->command, sizeof(out->command), "%s", command);
    snprintf(out->target, sizeof(out->target), "%s", target);
    if (strcmp(command, "workflow") == 0 && strcmp(target, "multi_action_sandbox") == 0) {
        return kalyx_dispatch_workflow_sandbox_files(envelope_path, response_path, sandbox_dir, out);
    }
    st = kalyx_host_plan_from_validation(&vr, &plan);
    if (st != KALYX_OK) return st;
    if (plan.decision == KALYX_HOST_REJECT) {
        out->decision = KALYX_DISPATCH_REJECT;
        snprintf(out->reason, sizeof(out->reason), "%s", plan.reason);
        out->status = KALYX_ERR_RESPONSE_REJECTED;
        return out->status;
    }
    if (plan.decision == KALYX_HOST_QUEUE_CONFIRMATION) {
        out->decision = KALYX_DISPATCH_QUEUE_CONFIRMATION;
        snprintf(out->reason, sizeof(out->reason), "%s", plan.reason);
        snprintf(artifact, sizeof(artifact), "%s/dispatch_plan.json", sandbox_dir);
        snprintf(out->artifact_file, sizeof(out->artifact_file), "%s", artifact);
        snprintf(body, sizeof(body),
                 "{\n  \"schema\": \"KDISPATCH01\",\n  \"decision\": \"queue_confirmation\",\n  \"command\": \"%s\",\n  \"target\": \"%s\",\n  \"reason\": \"host confirmation required before sandbox dispatch\"\n}\n",
                 command, target);
        st = write_text_artifact(artifact, body);
        out->status = st;
        return st;
    }
    if (plan.decision == KALYX_HOST_ACCEPT_ANSWER) {
        out->decision = KALYX_DISPATCH_SHOW_ANSWER;
        snprintf(out->reason, sizeof(out->reason), "validated answer written to sandbox");
        snprintf(artifact, sizeof(artifact), "%s/answer.txt", sandbox_dir);
        snprintf(out->artifact_file, sizeof(out->artifact_file), "%s", artifact);
        st = write_text_artifact(artifact, summary[0] ? summary : "validated answer");
        out->status = st;
        return st;
    }
    if (strcmp(command, "emit_ui_command") == 0 && strcmp(target, "open_preview") == 0) {
        out->decision = KALYX_DISPATCH_SANDBOX_EXECUTED;
        snprintf(out->reason, sizeof(out->reason), "safe open_preview request written to sandbox");
        snprintf(artifact, sizeof(artifact), "%s/open_preview_request.json", sandbox_dir);
        snprintf(out->artifact_file, sizeof(out->artifact_file), "%s", artifact);
        snprintf(body, sizeof(body), "{\n  \"schema\": \"KDISPATCH01\",\n  \"action\": \"open_preview\",\n  \"mode\": \"%s\",\n  \"theme\": \"%s\"\n}\n", mode, theme);
        st = write_text_artifact(artifact, body);
        out->status = st;
        return st;
    }
    if (strcmp(command, "emit_ui_command") == 0 && strcmp(target, "export_document") == 0) {
        out->decision = KALYX_DISPATCH_SANDBOX_EXECUTED;
        snprintf(out->reason, sizeof(out->reason), "safe export_document artifact written to sandbox");
        snprintf(artifact, sizeof(artifact), "%s/export_document_sandbox.md", sandbox_dir);
        snprintf(out->artifact_file, sizeof(out->artifact_file), "%s", artifact);
        snprintf(body, sizeof(body), "# KALYX Sandbox Export\n\nThis file is a sandbox artifact. No source file was overwritten.\n\n- command: `%s`\n- target: `%s`\n- mode: `%s`\n- theme: `%s`\n", command, target, mode, theme);
        st = write_text_artifact(artifact, body);
        out->status = st;
        return st;
    }
    out->decision = KALYX_DISPATCH_REJECT;
    snprintf(out->reason, sizeof(out->reason), "validated command is not implemented by sandbox allowlist");
    out->status = KALYX_ERR_UNKNOWN_ACTION;
    return out->status;
}


KalyxStatus kalyx_dispatch_workflow_sandbox_files(const char *envelope_path,
                                                   const char *response_path,
                                                   const char *sandbox_dir,
                                                   KalyxDispatchResult *out) {
    KalyxValidationResult vr;
    KalyxStatus st;
    WorkflowStepSpec steps[16];
    size_t count = 0u;
    size_t i;
    char errbuf[512] = {0};
    if (!envelope_path || !response_path || !sandbox_dir || !out) return KALYX_ERR_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->status = KALYX_ERR_RESPONSE_REJECTED;
    out->decision = KALYX_DISPATCH_REJECT;
    snprintf(out->command, sizeof(out->command), "workflow");
    snprintf(out->target, sizeof(out->target), "multi_action_sandbox");
    snprintf(out->sandbox_dir, sizeof(out->sandbox_dir), "%s", sandbox_dir);
    if (!ensure_dir_path(sandbox_dir)) {
        snprintf(out->reason, sizeof(out->reason), "cannot create workflow sandbox directory");
        return KALYX_ERR_IO;
    }
    st = kalyx_validate_response_files(envelope_path, response_path, &vr);
    if (st != KALYX_OK || !vr.accepted) {
        snprintf(out->reason, sizeof(out->reason), "response rejected before workflow dispatch: %s", vr.error[0] ? vr.error : "validation rejected response");
        out->status = st != KALYX_OK ? st : KALYX_ERR_RESPONSE_REJECTED;
        return out->status;
    }
    st = parse_workflow_steps(response_path, steps, sizeof(steps) / sizeof(steps[0]), &count, errbuf, sizeof(errbuf));
    out->workflow_step_count = (unsigned int)count;
    if (st != KALYX_OK) {
        snprintf(out->reason, sizeof(out->reason), "%s", errbuf[0] ? errbuf : "cannot parse workflow steps");
        out->status = st;
        (void)write_workflow_manifest(sandbox_dir, steps, count, "aborted", out->reason, out->artifact_file, sizeof(out->artifact_file));
        return st;
    }
    st = validate_workflow_policy(steps, count, errbuf, sizeof(errbuf));
    if (st != KALYX_OK) {
        snprintf(out->reason, sizeof(out->reason), "%s", errbuf[0] ? errbuf : "workflow policy rejected sequence");
        out->status = st;
        out->workflow_step_count = (unsigned int)count;
        out->workflow_executed_count = 0u;
        (void)write_workflow_manifest(sandbox_dir, steps, count, "aborted", out->reason, out->artifact_file, sizeof(out->artifact_file));
        return st;
    }
    for (i = 0u; i < count; i++) {
        st = write_workflow_step_artifact(sandbox_dir, i, &steps[i]);
        if (st != KALYX_OK) {
            snprintf(steps[i].status, sizeof(steps[i].status), "failed");
            snprintf(steps[i].reason, sizeof(steps[i].reason), "cannot write workflow step artifact");
            snprintf(out->reason, sizeof(out->reason), "cannot write workflow step artifact %u", (unsigned int)(i + 1u));
            out->status = st;
            out->workflow_step_count = (unsigned int)count;
            out->workflow_executed_count = (unsigned int)i;
            (void)write_workflow_manifest(sandbox_dir, steps, count, "aborted", out->reason, out->artifact_file, sizeof(out->artifact_file));
            return st;
        }
        snprintf(steps[i].status, sizeof(steps[i].status), "ok");
        snprintf(steps[i].reason, sizeof(steps[i].reason), "previous dependencies satisfied");
        out->workflow_executed_count = (unsigned int)(i + 1u);
    }
    st = write_workflow_manifest(sandbox_dir, steps, count, "ok", "", out->artifact_file, sizeof(out->artifact_file));
    out->workflow_step_count = (unsigned int)count;
    out->workflow_executed_count = (unsigned int)count;
    if (st != KALYX_OK) {
        snprintf(out->reason, sizeof(out->reason), "cannot write workflow manifest");
        out->status = st;
        return st;
    }
    out->decision = KALYX_DISPATCH_SANDBOX_EXECUTED;
    out->status = KALYX_OK;
    snprintf(out->reason, sizeof(out->reason), "safe multi-action workflow artifacts written to sandbox");
    return KALYX_OK;
}

KalyxStatus kalyx_write_dispatch_audit_file(const KalyxDispatchResult *result,
                                            const char *audit_path) {
    FILE *f;
    if (!result || !audit_path) return KALYX_ERR_INVALID_ARGUMENT;
    if (!ensure_parent_directory(audit_path)) return KALYX_ERR_IO;
    f = fopen(audit_path, "wb");
    if (!f) return KALYX_ERR_IO;
    fputs("{\n  \"schema\": \"KDISPATCH01\",\n  \"kalyx_version\": \"" KALYX_VERSION "\",\n  \"decision\": ", f);
    json_string(f, kalyx_dispatch_decision_name(result->decision));
    fputs(",\n  \"status\": ", f);
    json_string(f, kalyx_status_string(result->status));
    fputs(",\n  \"command\": ", f);
    json_string(f, result->command);
    fputs(",\n  \"target\": ", f);
    json_string(f, result->target);
    fputs(",\n  \"sandbox_dir\": ", f);
    json_string(f, result->sandbox_dir);
    fputs(",\n  \"artifact_file\": ", f);
    json_string(f, result->artifact_file);
    fputs(",\n  \"workflow_step_count\": ", f);
    fprintf(f, "%u", result->workflow_step_count);
    fputs(",\n  \"workflow_executed_count\": ", f);
    fprintf(f, "%u", result->workflow_executed_count);
    fputs(",\n  \"reason\": ", f);
    json_string(f, result->reason);
    fputs("\n}\n", f);
    return fclose(f) == 0 ? KALYX_OK : KALYX_ERR_IO;
}
