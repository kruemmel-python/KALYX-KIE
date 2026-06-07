#include "kalyx_policy.h"

#include <stdio.h>
#include <string.h>

static void err(char *error, size_t cap, const char *msg) {
    if (error && cap) snprintf(error, cap, "%s", msg ? msg : "policy validation failed");
}

static int string_has(const char *hay, const char *needle) {
    return hay && needle && *needle && strstr(hay, needle) != NULL;
}

static int node_string_contains(const KalyxJsonNode *root, const char *path, const char *needle) {
    const KalyxJsonNode *n = kalyx_json_path(root, path);
    const char *s = kalyx_json_string_value(n);
    return string_has(s, needle);
}

KalyxStatus kalyx_policy_validate(const KalyxJsonNode *policy_rules,
                                  const KalyxJsonNode *response_root,
                                  KalyxRisk risk,
                                  int requires_confirmation,
                                  char *error,
                                  size_t error_cap) {
    size_t i;
    if (!policy_rules || kalyx_json_type(policy_rules) != KALYX_JSON_ARRAY) return KALYX_ERR_INVALID_ARGUMENT;
    if (risk >= KALYX_RISK_HIGH && !requires_confirmation) {
        err(error, error_cap, "high or critical risk requires confirmation");
        return KALYX_ERR_RESPONSE_REJECTED;
    }
    for (i = 0u; i < kalyx_json_array_size(policy_rules); i++) {
        const KalyxJsonNode *rule = kalyx_json_array_get(policy_rules, i);
        const char *kind = kalyx_json_string_value(kalyx_json_object_get(rule, "kind"));
        if (!kind) continue;
        if (strcmp(kind, "forbidden_command") == 0) {
            const char *name = kalyx_json_string_value(kalyx_json_object_get(rule, "name"));
            const char *cmd = kalyx_json_string_value(kalyx_json_path(response_root, "machine_result.command"));
            if (name && cmd && strcmp(name, cmd) == 0) { err(error, error_cap, "response uses a forbidden command"); return KALYX_ERR_FORBIDDEN_ACTION; }
        } else if (strcmp(kind, "forbidden_target") == 0) {
            const char *name = kalyx_json_string_value(kalyx_json_object_get(rule, "name"));
            const char *target = kalyx_json_string_value(kalyx_json_path(response_root, "machine_result.target"));
            if (name && target && strcmp(name, target) == 0) { err(error, error_cap, "response uses a forbidden target"); return KALYX_ERR_FORBIDDEN_ACTION; }
        } else if (strcmp(kind, "forbidden_claim_contains") == 0) {
            const char *path = kalyx_json_string_value(kalyx_json_object_get(rule, "path"));
            const char *contains = kalyx_json_string_value(kalyx_json_object_get(rule, "contains"));
            if (path && contains && node_string_contains(response_root, path, contains)) { err(error, error_cap, "response contains a forbidden claim"); return KALYX_ERR_FORBIDDEN_ACTION; }
        } else if (strcmp(kind, "requires_confirmation_if_risk_at_least") == 0) {
            const char *level = kalyx_json_string_value(kalyx_json_object_get(rule, "risk"));
            KalyxRisk threshold = KALYX_RISK_HIGH;
            if (level && strcmp(level, "critical") == 0) threshold = KALYX_RISK_CRITICAL;
            else if (level && strcmp(level, "medium") == 0) threshold = KALYX_RISK_MEDIUM;
            else if (level && strcmp(level, "low") == 0) threshold = KALYX_RISK_LOW;
            if (risk >= threshold && !requires_confirmation) { err(error, error_cap, "policy requires confirmation for this risk level"); return KALYX_ERR_RESPONSE_REJECTED; }
        }
    }
    return KALYX_OK;
}
