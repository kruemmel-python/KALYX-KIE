#ifndef KALYX_INTERACTION_H
#define KALYX_INTERACTION_H

#include "kalyx_common.h"
#include "kalyx_domain.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct KalyxEnvelopeInput {
    KalyxDomain domain;
    const char *intent;
    const char *authority;
    const char *user_request;
    const char *runtime_state_json;
    const char *authoritative_data_json;
    const char *allowed_actions_json;
    const char *forbidden_actions_json;
    const char *validation_rules_json;
} KalyxEnvelopeInput;

KalyxStatus kalyx_make_envelope(const KalyxEnvelopeInput *input, KalyxBuffer *out_markdown);
KalyxStatus kalyx_make_envelope_file(const KalyxEnvelopeInput *input, const char *out_path);

#ifdef __cplusplus
}
#endif

#endif
