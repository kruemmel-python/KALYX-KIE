#ifndef KALYX_POLICY_H
#define KALYX_POLICY_H

#include "kalyx_json.h"
#include "kalyx_response.h"

#ifdef __cplusplus
extern "C" {
#endif

KalyxStatus kalyx_policy_validate(const KalyxJsonNode *policy_rules,
                                  const KalyxJsonNode *response_root,
                                  KalyxRisk risk,
                                  int requires_confirmation,
                                  char *error,
                                  size_t error_cap);

#ifdef __cplusplus
}
#endif

#endif
