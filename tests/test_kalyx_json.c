#include "kalyx_json.h"
#include <string.h>

int main(void) {
    KalyxJsonDocument doc = {0};
    const char *txt = "{\"schema\":\"KRESP01\",\"machine_result\":{\"command\":\"emit_ui_command\",\"args\":{\"mode\":\"html\"}},\"requires_confirmation\":true}";
    int b = 0;
    if (kalyx_json_parse(txt, &doc) != KALYX_OK) return 1;
    if (strcmp(kalyx_json_string_value(kalyx_json_path(kalyx_json_root(&doc), "machine_result.command")), "emit_ui_command") != 0) return 2;
    if (strcmp(kalyx_json_string_value(kalyx_json_path(kalyx_json_root(&doc), "machine_result.args.mode")), "html") != 0) return 3;
    if (!kalyx_json_bool_value(kalyx_json_path(kalyx_json_root(&doc), "requires_confirmation"), &b) || !b) return 4;
    kalyx_json_free(&doc);
    return 0;
}
