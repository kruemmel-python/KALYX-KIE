#ifndef KALYX_ACTION_H
#define KALYX_ACTION_H

#include "kalyx_json.h"
#include "kalyx_error.h"

#ifdef __cplusplus
extern "C" {
#endif

KalyxStatus kalyx_action_validate(const KalyxJsonNode *allowed_actions,
                                  const KalyxJsonNode *machine_result,
                                  char *error,
                                  size_t error_cap);

#ifdef __cplusplus
}
#endif

#endif
