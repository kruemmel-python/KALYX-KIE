#ifndef KALYX_DISPATCH_H
#define KALYX_DISPATCH_H

#include "kalyx_response.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum KalyxDispatchDecision {
    KALYX_DISPATCH_REJECT = 0,
    KALYX_DISPATCH_SHOW_ANSWER = 1,
    KALYX_DISPATCH_QUEUE_CONFIRMATION = 2,
    KALYX_DISPATCH_SANDBOX_EXECUTED = 3
} KalyxDispatchDecision;

typedef struct KalyxDispatchResult {
    KalyxStatus status;
    KalyxDispatchDecision decision;
    char command[96];
    char target[96];
    char sandbox_dir[512];
    char artifact_file[512];
    char reason[512];
    unsigned int workflow_step_count;
    unsigned int workflow_executed_count;
} KalyxDispatchResult;

const char *kalyx_dispatch_decision_name(KalyxDispatchDecision decision);
KalyxStatus kalyx_dispatch_sandbox_files(const char *envelope_path,
                                          const char *response_path,
                                          const char *sandbox_dir,
                                          KalyxDispatchResult *out);
KalyxStatus kalyx_dispatch_workflow_sandbox_files(const char *envelope_path,
                                                   const char *response_path,
                                                   const char *sandbox_dir,
                                                   KalyxDispatchResult *out);
KalyxStatus kalyx_write_dispatch_audit_file(const KalyxDispatchResult *result,
                                            const char *audit_path);

#ifdef __cplusplus
}
#endif

#endif
