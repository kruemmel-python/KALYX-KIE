#include "kalyx_action.h"

#include <stdio.h>
#include <string.h>

static void err(char *error, size_t cap, const char *msg) {
    if (error && cap) snprintf(error, cap, "%s", msg ? msg : "action validation failed");
}

static const KalyxJsonNode *find_action(const KalyxJsonNode *allowed_actions, const char *command) {
    size_t i;
    if (!allowed_actions || kalyx_json_type(allowed_actions) != KALYX_JSON_ARRAY || !command) return NULL;
    for (i = 0u; i < kalyx_json_array_size(allowed_actions); i++) {
        const KalyxJsonNode *a = kalyx_json_array_get(allowed_actions, i);
        const char *name = kalyx_json_string_value(kalyx_json_object_get(a, "name"));
        if (name && strcmp(name, command) == 0) return a;
    }
    return NULL;
}

KalyxStatus kalyx_action_validate(const KalyxJsonNode *allowed_actions,
                                  const KalyxJsonNode *machine_result,
                                  char *error,
                                  size_t error_cap) {
    const char *command;
    const KalyxJsonNode *action;
    const KalyxJsonNode *allowed_args;
    const KalyxJsonNode *target_node;
    const char *target;
    const KalyxJsonNode *response_args;
    size_t i;
    if (!allowed_actions || !machine_result) return KALYX_ERR_INVALID_ARGUMENT;
    command = kalyx_json_string_value(kalyx_json_object_get(machine_result, "command"));
    if (!command || !*command) { err(error, error_cap, "machine_result.command is missing"); return KALYX_ERR_MISSING_REQUIRED_FIELD; }
    action = find_action(allowed_actions, command);
    if (!action) { err(error, error_cap, "command is not listed in Allowed Actions"); return KALYX_ERR_UNKNOWN_ACTION; }

    target_node = kalyx_json_object_get(machine_result, "target");
    target = kalyx_json_string_value(target_node);
    allowed_args = kalyx_json_object_get(action, "args");
    if (target && allowed_args) {
        const KalyxJsonNode *allowed_targets = kalyx_json_object_get(allowed_args, "target");
        if (allowed_targets && !kalyx_json_string_array_contains(allowed_targets, target)) {
            err(error, error_cap, "machine_result.target is not allowed for this command");
            return KALYX_ERR_RESPONSE_REJECTED;
        }
    }

    response_args = kalyx_json_object_get(machine_result, "args");
    if (allowed_args && response_args) {
        static const char *keys[] = {"mode", "theme", "service_id", "route", "finding_class", "severity", "format", "scope", "target"};
        for (i = 0u; i < sizeof(keys) / sizeof(keys[0]); i++) {
            const KalyxJsonNode *value_node = kalyx_json_object_get(response_args, keys[i]);
            const KalyxJsonNode *allowed_values = kalyx_json_object_get(allowed_args, keys[i]);
            const char *value = kalyx_json_string_value(value_node);
            if (value && allowed_values && !kalyx_json_string_array_contains(allowed_values, value)) {
                err(error, error_cap, "machine_result.args contains a value outside the allowed action contract");
                return KALYX_ERR_RESPONSE_REJECTED;
            }
        }
    }
    return KALYX_OK;
}
