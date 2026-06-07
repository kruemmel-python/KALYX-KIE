#ifndef KALYX_DOMAIN_H
#define KALYX_DOMAIN_H

#include <stddef.h>
#include "kalyx_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum KalyxDomain {
    KALYX_DOMAIN_APP = 1,
    KALYX_DOMAIN_GAME = 2,
    KALYX_DOMAIN_AGENT = 3,
    KALYX_DOMAIN_CODE = 4,
    KALYX_DOMAIN_RESEARCH = 5,
    KALYX_DOMAIN_WORKFLOW = 6
} KalyxDomain;

typedef struct KalyxDomainDefinition {
    KalyxDomain domain;
    const char *name;
    const char *authority;
    const char *default_intent;
    const char *allowed_actions_json;
    const char *forbidden_actions_json;
    const char *validation_rules_json;
} KalyxDomainDefinition;

const KalyxDomainDefinition *kalyx_domain_definition(KalyxDomain domain);
KalyxStatus kalyx_domain_from_string(const char *name, KalyxDomain *out);
const char *kalyx_domain_name(KalyxDomain domain);
int kalyx_domain_is_valid(KalyxDomain domain);

#ifdef __cplusplus
}
#endif

#endif
