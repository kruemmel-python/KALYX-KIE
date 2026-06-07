#include "kalyx_audit.h"

#include "kalyx_hash.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif


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
    for (i = 0u; i < len; i++) if (buf[i] == '\\') buf[i] = '/';
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

static const char *safe_s(const char *s, const char *fallback) {
    return (s && *s) ? s : fallback;
}

static void write_json_string(FILE *f, const char *s) {
    const unsigned char *p = (const unsigned char *)safe_s(s, "");
    fputc('"', f);
    while (*p) {
        switch (*p) {
            case '\\': fputs("\\\\", f); break;
            case '"': fputs("\\\"", f); break;
            case '\n': fputs("\\n", f); break;
            case '\r': fputs("\\r", f); break;
            case '\t': fputs("\\t", f); break;
            default:
                if (*p < 32u) fprintf(f, "\\u%04x", (unsigned int)*p);
                else fputc(*p, f);
                break;
        }
        p++;
    }
    fputc('"', f);
}

KalyxStatus kalyx_write_audit_file(const KalyxAuditInput *input, const char *audit_path) {
    char env_hash[KALYX_SHA256_HEX_BYTES];
    char resp_hash[KALYX_SHA256_HEX_BYTES];
    FILE *f;
    KalyxStatus st;
    const KalyxValidationResult *v;
    if (!input || !audit_path || !input->envelope_file || !input->response_file || !input->validation) return KALYX_ERR_INVALID_ARGUMENT;
    st = kalyx_sha256_file_hex(input->envelope_file, env_hash);
    if (st != KALYX_OK) return st;
    st = kalyx_sha256_file_hex(input->response_file, resp_hash);
    if (st != KALYX_OK) return st;
    if (!ensure_parent_directory(audit_path)) return KALYX_ERR_IO;
    f = fopen(audit_path, "wb");
    if (!f) return KALYX_ERR_IO;
    v = input->validation;
    fputs("{\n  \"schema\": \"KAUDIT01\",\n  \"kalyx_version\": \"" KALYX_VERSION "\",\n  \"envelope_file\": ", f);
    write_json_string(f, input->envelope_file);
    fputs(",\n  \"envelope_sha256\": ", f);
    write_json_string(f, env_hash);
    fputs(",\n  \"response_file\": ", f);
    write_json_string(f, input->response_file);
    fputs(",\n  \"response_sha256\": ", f);
    write_json_string(f, resp_hash);
    fprintf(f, ",\n  \"validated\": %s,\n  \"validation_errors\": ", v->status == KALYX_OK ? "true" : "false");
    if (v->status == KALYX_OK) {
        fputs("[]", f);
    } else {
        fputc('[', f);
        write_json_string(f, v->error[0] ? v->error : "validation rejected");
        fputc(']', f);
    }
    fputs(",\n  \"transport\": {\n    \"provider\": ", f);
    write_json_string(f, safe_s(input->provider, "offline_file"));
    fputs(",\n    \"model\": ", f);
    write_json_string(f, safe_s(input->model, "configured_outside_envelope"));
    fprintf(f,
        ",\n    \"temperature\": %.17g,\n    \"max_tokens\": %d\n  },\n  \"result\": {\n    \"accepted\": %s,\n    \"status\": ",
        input->temperature,
        input->max_tokens,
        v->accepted ? "true" : "false");
    write_json_string(f, kalyx_status_string(v->status));
    fputs(",\n    \"response_schema\": \"KRESP01\",\n    \"response_type\": ", f);
    write_json_string(f, kalyx_response_type_name(v->response_type));
    fputs(",\n    \"risk\": ", f);
    write_json_string(f, kalyx_risk_name(v->risk));
    fprintf(f,
        ",\n    \"requires_confirmation\": %s,\n    \"uses_only_provided_context\": %s,\n    \"command\": ",
        v->requires_confirmation ? "true" : "false",
        v->uses_only_provided_context ? "true" : "false");
    write_json_string(f, v->command_name);
    fputs("\n  }\n}\n", f);
    if (fclose(f) != 0) return KALYX_ERR_IO;
    return KALYX_OK;
}
