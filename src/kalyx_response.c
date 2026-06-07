#include "kalyx_response.h"
#include "kalyx_action.h"
#include "kalyx_json.h"
#include "kalyx_policy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(KalyxValidationResult *r, KalyxStatus st, const char *msg) {
    if (!r) return;
    r->status = st;
    r->accepted = 0;
    if (msg) snprintf(r->error, sizeof(r->error), "%s", msg);
}

const char *kalyx_response_type_name(KalyxResponseType type) {
    switch (type) {
        case KALYX_RESPONSE_ANSWER: return "answer";
        case KALYX_RESPONSE_COMMAND: return "command";
        case KALYX_RESPONSE_CLARIFICATION: return "clarification";
        case KALYX_RESPONSE_REJECTION: return "rejection";
        case KALYX_RESPONSE_DIAGNOSTIC: return "diagnostic";
        default: return "unknown";
    }
}

const char *kalyx_risk_name(KalyxRisk risk) {
    switch (risk) {
        case KALYX_RISK_NONE: return "none";
        case KALYX_RISK_LOW: return "low";
        case KALYX_RISK_MEDIUM: return "medium";
        case KALYX_RISK_HIGH: return "high";
        case KALYX_RISK_CRITICAL: return "critical";
        default: return "unknown";
    }
}

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

static int parse_type(const char *s, KalyxResponseType *out) {
    if (strcmp(s, "answer") == 0) *out = KALYX_RESPONSE_ANSWER;
    else if (strcmp(s, "command") == 0) *out = KALYX_RESPONSE_COMMAND;
    else if (strcmp(s, "clarification") == 0) *out = KALYX_RESPONSE_CLARIFICATION;
    else if (strcmp(s, "rejection") == 0) *out = KALYX_RESPONSE_REJECTION;
    else if (strcmp(s, "diagnostic") == 0) *out = KALYX_RESPONSE_DIAGNOSTIC;
    else return 0;
    return 1;
}

static int parse_risk(const char *s, KalyxRisk *out) {
    if (strcmp(s, "none") == 0) *out = KALYX_RISK_NONE;
    else if (strcmp(s, "low") == 0) *out = KALYX_RISK_LOW;
    else if (strcmp(s, "medium") == 0) *out = KALYX_RISK_MEDIUM;
    else if (strcmp(s, "high") == 0) *out = KALYX_RISK_HIGH;
    else if (strcmp(s, "critical") == 0) *out = KALYX_RISK_CRITICAL;
    else return 0;
    return 1;
}

static int envelope_yaml_value(const char *env, const char *key, char *out, size_t cap) {
    char needle[80];
    const char *p;
    size_t n = 0u;
    if (snprintf(needle, sizeof(needle), "%s:", key) >= (int)sizeof(needle)) return 0;
    p = strstr(env, needle);
    if (!p) return 0;
    p += strlen(needle);
    p = skip_ws(p);
    while (*p && *p != '\n' && *p != '\r') {
        if (n + 1u < cap) out[n++] = *p;
        p++;
    }
    while (n > 0u && (out[n - 1u] == ' ' || out[n - 1u] == '\t')) n--;
    out[n] = '\0';
    return n > 0u;
}

static const char *required_string(const KalyxJsonNode *root, const char *path) {
    return kalyx_json_string_value(kalyx_json_path(root, path));
}

KalyxStatus kalyx_validate_response_text(const char *envelope_markdown,
                                         const char *response_markdown,
                                         KalyxValidationResult *out) {
    KalyxBuffer result_json = {0};
    KalyxBuffer allowed_json = {0};
    KalyxBuffer policy_json = {0};
    KalyxJsonDocument result_doc = {0};
    KalyxJsonDocument allowed_doc = {0};
    KalyxJsonDocument policy_doc = {0};
    char env_schema[32], env_contract[32];
    const char *schema;
    const char *type_s;
    const char *risk_s;
    const char *summary;
    const char *command;
    KalyxResponseType type;
    KalyxRisk risk;
    int requires_confirmation;
    int uses_only_context;
    KalyxStatus st;
    char errbuf[512];

    if (!envelope_markdown || !response_markdown || !out) return KALYX_ERR_INVALID_ARGUMENT;
    memset(out, 0, sizeof(*out));
    out->status = KALYX_ERR_RESPONSE_REJECTED;

    if (!envelope_yaml_value(envelope_markdown, "schema", env_schema, sizeof(env_schema)) || strcmp(env_schema, KALYX_SCHEMA_KIE) != 0) {
        set_error(out, KALYX_ERR_SCHEMA, "envelope schema is not KIE01");
        return out->status;
    }
    if (!envelope_yaml_value(envelope_markdown, "response_contract", env_contract, sizeof(env_contract)) || strcmp(env_contract, KALYX_SCHEMA_KRESP) != 0) {
        set_error(out, KALYX_ERR_SCHEMA, "envelope response contract is not KRESP01");
        return out->status;
    }

    st = kalyx_extract_markdown_json_block(response_markdown, "Machine Result", &result_json);
    if (st != KALYX_OK) { set_error(out, KALYX_ERR_PARSE, "response has no Machine Result JSON block"); return out->status; }
    st = kalyx_extract_markdown_json_block(envelope_markdown, "Allowed Actions", &allowed_json);
    if (st != KALYX_OK) { set_error(out, KALYX_ERR_PARSE, "envelope has no Allowed Actions block"); goto done; }
    st = kalyx_extract_markdown_json_block(envelope_markdown, "Validation Rules", &policy_json);
    if (st != KALYX_OK) { set_error(out, KALYX_ERR_PARSE, "envelope has no Validation Rules block"); goto done; }

    if (kalyx_json_parse(result_json.data, &result_doc) != KALYX_OK) { set_error(out, KALYX_ERR_PARSE, "response Machine Result is not valid JSON"); goto done; }
    if (kalyx_json_parse(allowed_json.data, &allowed_doc) != KALYX_OK) { set_error(out, KALYX_ERR_PARSE, "Allowed Actions is not valid JSON"); goto done; }
    if (kalyx_json_parse(policy_json.data, &policy_doc) != KALYX_OK) { set_error(out, KALYX_ERR_PARSE, "Validation Rules is not valid JSON"); goto done; }

    schema = required_string(kalyx_json_root(&result_doc), "schema");
    if (!schema || strcmp(schema, KALYX_SCHEMA_KRESP) != 0) { set_error(out, KALYX_ERR_SCHEMA, "response schema is not KRESP01"); goto done; }
    type_s = required_string(kalyx_json_root(&result_doc), "type");
    if (!type_s || !parse_type(type_s, &type)) { set_error(out, KALYX_ERR_MISSING_REQUIRED_FIELD, "response type is missing or invalid"); goto done; }
    summary = required_string(kalyx_json_root(&result_doc), "human_summary");
    if (!summary || !*summary) { set_error(out, KALYX_ERR_MISSING_REQUIRED_FIELD, "human_summary is missing"); goto done; }
    risk_s = required_string(kalyx_json_root(&result_doc), "risk");
    if (!risk_s || !parse_risk(risk_s, &risk)) { set_error(out, KALYX_ERR_MISSING_REQUIRED_FIELD, "risk is missing or invalid"); goto done; }
    if (!kalyx_json_bool_value(kalyx_json_path(kalyx_json_root(&result_doc), "requires_confirmation"), &requires_confirmation)) {
        set_error(out, KALYX_ERR_MISSING_REQUIRED_FIELD, "requires_confirmation is missing"); goto done;
    }
    if (!kalyx_json_bool_value(kalyx_json_path(kalyx_json_root(&result_doc), "uses_only_provided_context"), &uses_only_context)) {
        set_error(out, KALYX_ERR_MISSING_REQUIRED_FIELD, "uses_only_provided_context is missing"); goto done;
    }
    if (!kalyx_json_path(kalyx_json_root(&result_doc), "machine_result")) {
        set_error(out, KALYX_ERR_MISSING_REQUIRED_FIELD, "machine_result is missing"); goto done;
    }
    if (!uses_only_context) { set_error(out, KALYX_ERR_RESPONSE_REJECTED, "response declares external context usage"); goto done; }

    memset(errbuf, 0, sizeof(errbuf));
    st = kalyx_policy_validate(kalyx_json_root(&policy_doc), kalyx_json_root(&result_doc), risk, requires_confirmation, errbuf, sizeof(errbuf));
    if (st != KALYX_OK) { set_error(out, st, errbuf); goto done; }

    command = required_string(kalyx_json_root(&result_doc), "machine_result.command");
    if (type == KALYX_RESPONSE_COMMAND) {
        const KalyxJsonNode *machine_result = kalyx_json_path(kalyx_json_root(&result_doc), "machine_result");
        memset(errbuf, 0, sizeof(errbuf));
        st = kalyx_action_validate(kalyx_json_root(&allowed_doc), machine_result, errbuf, sizeof(errbuf));
        if (st != KALYX_OK) { set_error(out, st, errbuf); goto done; }
        snprintf(out->command_name, sizeof(out->command_name), "%s", command ? command : "");
    }

    out->status = KALYX_OK;
    out->accepted = 1;
    out->response_type = type;
    out->risk = risk;
    out->requires_confirmation = requires_confirmation;
    out->uses_only_provided_context = uses_only_context;
    out->error[0] = '\0';

done:
    st = out->status;
    kalyx_json_free(&result_doc);
    kalyx_json_free(&allowed_doc);
    kalyx_json_free(&policy_doc);
    kalyx_buffer_free(&result_json);
    kalyx_buffer_free(&allowed_json);
    kalyx_buffer_free(&policy_json);
    return st;
}

KalyxStatus kalyx_validate_response_files(const char *envelope_path,
                                          const char *response_path,
                                          KalyxValidationResult *out) {
    KalyxBuffer env;
    KalyxBuffer resp;
    KalyxStatus st;
    if (!envelope_path || !response_path || !out) return KALYX_ERR_INVALID_ARGUMENT;
    st = kalyx_read_text_file(envelope_path, &env);
    if (st != KALYX_OK) return st;
    st = kalyx_read_text_file(response_path, &resp);
    if (st != KALYX_OK) { kalyx_buffer_free(&env); return st; }
    st = kalyx_validate_response_text(env.data, resp.data, out);
    kalyx_buffer_free(&env);
    kalyx_buffer_free(&resp);
    return st;
}
