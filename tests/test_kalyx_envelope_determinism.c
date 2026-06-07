#include "kalyx_interaction.h"
#include <stdio.h>
#include <string.h>

static int has(const char *s, const char *n) { return strstr(s, n) != 0; }

int main(void) {
    KalyxEnvelopeInput in;
    KalyxBuffer a = {0}, b = {0};
    int ok;
    memset(&in, 0, sizeof(in));
    in.domain = KALYX_DOMAIN_APP;
    in.intent = "export_document";
    in.user_request = "Export as HTML.";
    in.runtime_state_json = "{\"current_file\":\"README.md\"}";
    in.authoritative_data_json = "{\"available_exports\":[\"html\"]}";
    if (kalyx_make_envelope(&in, &a) != KALYX_OK) return 1;
    if (kalyx_make_envelope(&in, &b) != KALYX_OK) return 2;
    ok = a.size == b.size && memcmp(a.data, b.data, a.size) == 0 &&
         has(a.data, "## Contract") &&
         has(a.data, "## Runtime State") &&
         has(a.data, "## Authoritative Data") &&
         has(a.data, "## Allowed Actions") &&
         has(a.data, "## Forbidden Actions") &&
         has(a.data, "## Validation Rules") &&
         has(a.data, "## Required Response");
    kalyx_buffer_free(&a);
    kalyx_buffer_free(&b);
    return ok ? 0 : 3;
}
