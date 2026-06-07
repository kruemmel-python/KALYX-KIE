#include "kalyx_interaction.h"
#include <string.h>

int main(void) {
    int i;
    const char *names[] = {"app","game","agent","code","research","workflow"};
    for (i = 0; i < 6; i++) {
        KalyxDomain d;
        KalyxEnvelopeInput in;
        KalyxBuffer env = {0};
        if (kalyx_domain_from_string(names[i], &d) != KALYX_OK) return 1 + i;
        memset(&in, 0, sizeof(in));
        in.domain = d;
        in.user_request = "domain smoke";
        in.runtime_state_json = "{}";
        in.authoritative_data_json = "{}";
        if (kalyx_make_envelope(&in, &env) != KALYX_OK) return 10 + i;
        if (!strstr(env.data, names[i]) || !strstr(env.data, "KRESP01")) return 20 + i;
        kalyx_buffer_free(&env);
    }
    return 0;
}
