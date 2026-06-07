#include "kalyx_audit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#endif

static void usage(void) {
    puts("KALYX-KIE v1.0 Final Response Validator");
    puts("");
    puts("Usage:");
    puts("  kalyx_validate_response --envelope FILE --response FILE --audit FILE [--provider NAME] [--model NAME] [--temperature N] [--max-tokens N]");
    puts("  kalyx_validate_response --version");
    puts("");
    puts("Purpose:");
    puts("  Validate a KRESP01 response against a KIE01 envelope and write a KAUDIT01 ledger.");
    puts("");
    puts("Result:");
    puts("  accepted=true only when schema, type, risk, confirmation, context and command contract are valid.");
}


static int file_is_readable(const char *path) {
    FILE *f;
    if (!path || !*path) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int mkdir_one(const char *path) {
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0777) == 0 || errno == EEXIST;
#endif
}

static int ensure_parent_directory(const char *path) {
    char buf[4096];
    size_t len;
    size_t i;
    if (!path || !*path) return 0;
    len = strlen(path);
    if (len >= sizeof(buf)) return 0;
    memcpy(buf, path, len + 1u);
    for (i = 0u; i < len; i++) {
        if (buf[i] == '\\') buf[i] = '/';
    }
    for (i = 0u; i < len; i++) {
        if (buf[i] == '/') {
            if (i == 0u) continue;
            if (i == 2u && buf[1] == ':') continue;
            buf[i] = '\0';
            if (buf[0] != '\0' && !mkdir_one(buf)) return 0;
            buf[i] = '/';
        }
    }
    return 1;
}

static const char *arg_value(int argc, char **argv, const char *name) {
    int i;
    for (i = 1; i + 1 < argc; i++) if (strcmp(argv[i], name) == 0) return argv[i + 1];
    return NULL;
}

int main(int argc, char **argv) {
    const char *envelope = arg_value(argc, argv, "--envelope");
    const char *response = arg_value(argc, argv, "--response");
    const char *audit = arg_value(argc, argv, "--audit");
    const char *provider = arg_value(argc, argv, "--provider");
    const char *model = arg_value(argc, argv, "--model");
    const char *temperature_s = arg_value(argc, argv, "--temperature");
    const char *max_tokens_s = arg_value(argc, argv, "--max-tokens");
    KalyxValidationResult vr;
    KalyxAuditInput ai;
    KalyxStatus st;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) { usage(); return 0; }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) { puts(KALYX_VERSION); return 0; }
    if (!envelope || !response || !audit) { usage(); return 2; }
    if (!file_is_readable(envelope)) {
        fprintf(stderr, "cannot read envelope: %s\n", envelope);
        return 5;
    }
    if (!file_is_readable(response)) {
        fprintf(stderr, "cannot read response: %s\n", response);
        return 5;
    }
    if (!ensure_parent_directory(audit)) {
        fprintf(stderr, "cannot create audit parent directory for: %s\n", audit);
        return 5;
    }
    st = kalyx_validate_response_files(envelope, response, &vr);
    memset(&ai, 0, sizeof(ai));
    ai.envelope_file = envelope;
    ai.response_file = response;
    ai.provider = provider ? provider : "offline_file";
    ai.model = model ? model : "configured_outside_envelope";
    ai.temperature = temperature_s ? atof(temperature_s) : 0.0;
    ai.max_tokens = max_tokens_s ? atoi(max_tokens_s) : 2048;
    ai.validation = &vr;
    {
        KalyxStatus audit_status = kalyx_write_audit_file(&ai, audit);
        if (audit_status != KALYX_OK) {
            fprintf(stderr, "cannot write audit: %s (%s)\n", audit, kalyx_status_string(audit_status));
            return 5;
        }
    }
    if (st != KALYX_OK) {
        fprintf(stderr, "response rejected: %s (%s)\n", kalyx_status_string(st), vr.error);
        return (int)st;
    }
    printf("response accepted: type=%s risk=%s confirmation=%s command=%s audit=%s\n",
           kalyx_response_type_name(vr.response_type),
           kalyx_risk_name(vr.risk),
           vr.requires_confirmation ? "true" : "false",
           vr.command_name[0] ? vr.command_name : "-",
           audit);
    return 0;
}
