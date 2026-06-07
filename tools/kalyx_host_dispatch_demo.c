#include "kalyx_audit.h"
#include "kalyx_dispatch.h"
#include "kalyx_response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *arg_value(int argc, char **argv, const char *name) {
    int i;
    for (i = 1; i + 1 < argc; i++) if (strcmp(argv[i], name) == 0) return argv[i + 1];
    return NULL;
}

static void usage(void) {
    puts("KALYX-KIE v1.0 Final Host Dispatch Demo");
    puts("");
    puts("Usage:");
    puts("  kalyx_host_dispatch_demo --envelope FILE --response FILE --validation-audit FILE --dispatch-audit FILE --sandbox-dir DIR [--provider NAME] [--model NAME] [--temperature N] [--max-tokens N]");
    puts("  kalyx_host_dispatch_demo --version");
    puts("");
    puts("Purpose:");
    puts("  Re-validate KRESP01, enforce strict dispatch gate, apply workflow policy, and write only sandbox artifacts.");
    puts("");
    puts("Safety:");
    puts("  No OS command execution. No writes outside --sandbox-dir. Invalid validation produces reject audit.");
}

static int file_is_readable(const char *path) {
    FILE *f;
    if (!path || !*path) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int main(int argc, char **argv) {
    const char *envelope = arg_value(argc, argv, "--envelope");
    const char *response = arg_value(argc, argv, "--response");
    const char *validation_audit = arg_value(argc, argv, "--validation-audit");
    const char *dispatch_audit = arg_value(argc, argv, "--dispatch-audit");
    const char *sandbox_dir = arg_value(argc, argv, "--sandbox-dir");
    const char *provider = arg_value(argc, argv, "--provider");
    const char *model = arg_value(argc, argv, "--model");
    const char *temperature_s = arg_value(argc, argv, "--temperature");
    const char *max_tokens_s = arg_value(argc, argv, "--max-tokens");
    KalyxValidationResult vr;
    KalyxAuditInput ai;
    KalyxDispatchResult dr;
    KalyxStatus st;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) { usage(); return 0; }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) { puts(KALYX_VERSION); return 0; }
    if (!envelope || !response || !validation_audit || !dispatch_audit || !sandbox_dir) { usage(); return 2; }
    if (!file_is_readable(envelope)) { fprintf(stderr, "cannot read envelope: %s\n", envelope); return 5; }
    if (!file_is_readable(response)) { fprintf(stderr, "cannot read response: %s\n", response); return 5; }

    st = kalyx_validate_response_files(envelope, response, &vr);
    memset(&ai, 0, sizeof(ai));
    ai.envelope_file = envelope;
    ai.response_file = response;
    ai.provider = provider ? provider : "offline_file";
    ai.model = model ? model : "host_dispatch_demo";
    ai.temperature = temperature_s ? atof(temperature_s) : 0.0;
    ai.max_tokens = max_tokens_s ? atoi(max_tokens_s) : 2048;
    ai.validation = &vr;
    if (kalyx_write_audit_file(&ai, validation_audit) != KALYX_OK) {
        fprintf(stderr, "cannot write validation audit: %s\n", validation_audit);
        return 5;
    }
    if (st != KALYX_OK || !vr.accepted) {
        memset(&dr, 0, sizeof(dr));
        dr.status = st != KALYX_OK ? st : KALYX_ERR_RESPONSE_REJECTED;
        dr.decision = KALYX_DISPATCH_REJECT;
        snprintf(dr.sandbox_dir, sizeof(dr.sandbox_dir), "%s", sandbox_dir);
        snprintf(dr.reason, sizeof(dr.reason), "response rejected before dispatch: %s",
                 vr.error[0] ? vr.error : "validation rejected response");
        if (kalyx_write_dispatch_audit_file(&dr, dispatch_audit) != KALYX_OK) {
            fprintf(stderr, "cannot write rejected dispatch audit: %s\n", dispatch_audit);
            return 5;
        }
        fprintf(stderr, "response rejected before dispatch: %s (%s); dispatch blocked; dispatch_audit=%s\n",
                kalyx_status_string(dr.status), vr.error, dispatch_audit);
        return (int)dr.status;
    }

    st = kalyx_dispatch_sandbox_files(envelope, response, sandbox_dir, &dr);
    if (kalyx_write_dispatch_audit_file(&dr, dispatch_audit) != KALYX_OK) {
        fprintf(stderr, "cannot write dispatch audit: %s\n", dispatch_audit);
        return 5;
    }
    if (st != KALYX_OK) {
        fprintf(stderr, "dispatch rejected: %s (%s)\n", kalyx_status_string(st), dr.reason);
        return (int)st;
    }
    printf("dispatch decision=%s command=%s target=%s artifact=%s dispatch_audit=%s\n",
           kalyx_dispatch_decision_name(dr.decision),
           dr.command[0] ? dr.command : "-",
           dr.target[0] ? dr.target : "-",
           dr.artifact_file[0] ? dr.artifact_file : "-",
           dispatch_audit);
    return 0;
}
