#include "kalyx_interaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
    puts("KALYX-KIE v1.0 Final Envelope Builder");
    puts("");
    puts("Usage:");
    puts("  kalyx_make_envelope --domain app|game|agent|code|research|workflow --intent NAME --request FILE --out FILE [--state FILE]");
    puts("  kalyx_make_envelope --domain app --intent summarize_document --document FILE --document-type markdown|text|json --request FILE --out FILE");
    puts("  kalyx_make_envelope --prompt-pack DIR --domain app --intent NAME --out FILE [--state FILE]");
    puts("  kalyx_make_envelope --version");
    puts("");
    puts("Purpose:");
    puts("  Build a KIE01 interaction envelope from state/document data or a .kpromptpack directory.");
    puts("");
    puts("Output:");
    puts("  .kie.md envelope containing request, state, allowed actions, validation rules, and required KRESP01 contract.");
}


static const char *arg_value(int argc, char **argv, const char *name) {
    int i;
    for (i = 1; i + 1 < argc; i++) if (strcmp(argv[i], name) == 0) return argv[i + 1];
    return NULL;
}

static const char *base_name(const char *path) {
    const char *a;
    const char *b;
    if (!path) return "document";
    a = strrchr(path, '/');
    b = strrchr(path, '\\');
    if (a && b) return (a > b) ? a + 1 : b + 1;
    if (a) return a + 1;
    if (b) return b + 1;
    return path;
}

static char *join_path(const char *dir, const char *file) {
    size_t a, b;
    char *out;
    if (!dir || !file) return NULL;
    a = strlen(dir);
    b = strlen(file);
    out = (char *)calloc(a + b + 2u, 1u);
    if (!out) return NULL;
    memcpy(out, dir, a);
    if (a > 0u && dir[a - 1u] != '/' && dir[a - 1u] != '\\') out[a++] = '/';
    memcpy(out + a, file, b);
    return out;
}

static KalyxStatus append_raw(char **buf, size_t *len, size_t *cap, const char *s, size_t n) {
    char *p;
    size_t need;
    size_t c;
    if (!buf || !len || !cap || !s) return KALYX_ERR_INVALID_ARGUMENT;
    need = *len + n + 1u;
    if (need > *cap) {
        c = *cap ? *cap : 4096u;
        while (c < need) c *= 2u;
        p = (char *)realloc(*buf, c);
        if (!p) return KALYX_ERR_IO;
        *buf = p;
        *cap = c;
    }
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = '\0';
    return KALYX_OK;
}

static KalyxStatus append_cstr(char **buf, size_t *len, size_t *cap, const char *s) {
    return append_raw(buf, len, cap, s, strlen(s));
}

static KalyxStatus append_json_string(char **buf, size_t *len, size_t *cap, const char *s) {
    const unsigned char *p;
    char tmp[8];
    KalyxStatus st;
    st = append_cstr(buf, len, cap, "\"");
    if (st != KALYX_OK) return st;
    for (p = (const unsigned char *)s; *p; p++) {
        switch (*p) {
            case '\\': st = append_cstr(buf, len, cap, "\\\\"); break;
            case '"': st = append_cstr(buf, len, cap, "\\\""); break;
            case '\b': st = append_cstr(buf, len, cap, "\\b"); break;
            case '\f': st = append_cstr(buf, len, cap, "\\f"); break;
            case '\n': st = append_cstr(buf, len, cap, "\\n"); break;
            case '\r': st = append_cstr(buf, len, cap, "\\r"); break;
            case '\t': st = append_cstr(buf, len, cap, "\\t"); break;
            default:
                if (*p < 0x20u) {
                    snprintf(tmp, sizeof(tmp), "\\u%04x", (unsigned int)*p);
                    st = append_cstr(buf, len, cap, tmp);
                } else {
                    st = append_raw(buf, len, cap, (const char *)p, 1u);
                }
                break;
        }
        if (st != KALYX_OK) return st;
    }
    return append_cstr(buf, len, cap, "\"");
}

static KalyxStatus build_document_state_json(const char *document_path,
                                             const char *document_type,
                                             const char *document_name,
                                             const char *document_text,
                                             KalyxBuffer *out) {
    char *buf = NULL;
    size_t len = 0u;
    size_t cap = 0u;
    KalyxStatus st;
    if (!document_path || !document_text || !out) return KALYX_ERR_INVALID_ARGUMENT;
    out->data = NULL;
    out->size = 0u;
    st = append_cstr(&buf, &len, &cap, "{\n  \"app\": \"KALYX-KIE\",\n  \"input_adapter\": \"document\",\n  \"document_name\": ");
    if (st != KALYX_OK) goto fail;
    st = append_json_string(&buf, &len, &cap, document_name && *document_name ? document_name : base_name(document_path));
    if (st != KALYX_OK) goto fail;
    st = append_cstr(&buf, &len, &cap, ",\n  \"document_path\": ");
    if (st != KALYX_OK) goto fail;
    st = append_json_string(&buf, &len, &cap, document_path);
    if (st != KALYX_OK) goto fail;
    st = append_cstr(&buf, &len, &cap, ",\n  \"document_type\": ");
    if (st != KALYX_OK) goto fail;
    st = append_json_string(&buf, &len, &cap, document_type && *document_type ? document_type : "text");
    if (st != KALYX_OK) goto fail;
    st = append_cstr(&buf, &len, &cap, ",\n  \"document_encoding\": \"utf-8-or-binary-preserved-text\",\n  \"authoritative_document\": ");
    if (st != KALYX_OK) goto fail;
    st = append_json_string(&buf, &len, &cap, document_text);
    if (st != KALYX_OK) goto fail;
    st = append_cstr(&buf, &len, &cap, "\n}\n");
    if (st != KALYX_OK) goto fail;
    out->data = buf;
    out->size = len;
    return KALYX_OK;
fail:
    free(buf);
    return st;
}

int main(int argc, char **argv) {
    const char *prompt_pack = arg_value(argc, argv, "--prompt-pack");
    const char *domain_s = arg_value(argc, argv, "--domain");
    const char *intent = arg_value(argc, argv, "--intent");
    const char *state_path = arg_value(argc, argv, "--state");
    const char *document_path = arg_value(argc, argv, "--document");
    const char *document_type = arg_value(argc, argv, "--document-type");
    const char *document_name = arg_value(argc, argv, "--document-name");
    const char *request_path = arg_value(argc, argv, "--request");
    const char *out_path = arg_value(argc, argv, "--out");
    const char *authority = arg_value(argc, argv, "--authority");
    const char *data_path = arg_value(argc, argv, "--data");
    const char *allowed_path = arg_value(argc, argv, "--allowed");
    const char *forbidden_path = arg_value(argc, argv, "--forbidden");
    const char *rules_path = arg_value(argc, argv, "--rules");
    char *pack_request_path = NULL;
    char *pack_allowed_path = NULL;
    char *pack_rules_path = NULL;
    char *pack_state_path = NULL;
    KalyxDomain domain;
    KalyxBuffer state = {0}, request = {0}, data = {0}, allowed = {0}, forbidden = {0}, rules = {0}, document = {0};
    KalyxEnvelopeInput in;
    KalyxStatus st;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) { usage(); return 0; }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) { puts(KALYX_VERSION); return 0; }
    if (prompt_pack) {
        if (!domain_s || !intent || !out_path) { usage(); return 2; }
        pack_request_path = join_path(prompt_pack, "request.txt");
        pack_allowed_path = join_path(prompt_pack, "allowed_actions.json");
        pack_rules_path = join_path(prompt_pack, "policy_rules.json");
        pack_state_path = join_path(prompt_pack, "example_state.json");
        if (!pack_request_path || !pack_allowed_path || !pack_rules_path || !pack_state_path) { fprintf(stderr, "cannot allocate prompt-pack paths\n"); return 2; }
        if (!request_path) request_path = pack_request_path;
        if (!allowed_path) allowed_path = pack_allowed_path;
        if (!rules_path) rules_path = pack_rules_path;
        if (!state_path && !document_path) state_path = pack_state_path;
    }
    if (!domain_s || (!state_path && !document_path) || !request_path || !out_path) { usage(); return 2; }
    if (state_path && document_path) { fprintf(stderr, "use either --state or --document, not both\n"); return 2; }
    st = kalyx_domain_from_string(domain_s, &domain);
    if (st != KALYX_OK) { fprintf(stderr, "invalid domain: %s\n", domain_s); return 2; }
    if (state_path) {
        st = kalyx_read_text_file(state_path, &state);
        if (st != KALYX_OK) { fprintf(stderr, "cannot read state: %s\n", state_path); return 3; }
    } else {
        st = kalyx_read_text_file(document_path, &document);
        if (st != KALYX_OK) { fprintf(stderr, "cannot read document: %s\n", document_path); return 3; }
        st = build_document_state_json(document_path, document_type, document_name, document.data, &state);
        if (st != KALYX_OK) { fprintf(stderr, "cannot build document state: %s\n", kalyx_status_string(st)); kalyx_buffer_free(&document); return 3; }
    }
    st = kalyx_read_text_file(request_path, &request);
    if (st != KALYX_OK) { fprintf(stderr, "cannot read request: %s\n", request_path); kalyx_buffer_free(&state); kalyx_buffer_free(&document); return 3; }
    if (data_path && (st = kalyx_read_text_file(data_path, &data)) != KALYX_OK) { fprintf(stderr, "cannot read data: %s\n", data_path); return 3; }
    if (allowed_path && (st = kalyx_read_text_file(allowed_path, &allowed)) != KALYX_OK) { fprintf(stderr, "cannot read allowed: %s\n", allowed_path); return 3; }
    if (forbidden_path && (st = kalyx_read_text_file(forbidden_path, &forbidden)) != KALYX_OK) { fprintf(stderr, "cannot read forbidden: %s\n", forbidden_path); return 3; }
    if (rules_path && (st = kalyx_read_text_file(rules_path, &rules)) != KALYX_OK) { fprintf(stderr, "cannot read rules: %s\n", rules_path); return 3; }
    memset(&in, 0, sizeof(in));
    in.domain = domain;
    in.intent = intent;
    in.authority = authority;
    in.user_request = request.data;
    in.runtime_state_json = state.data;
    in.authoritative_data_json = data.data ? data.data : state.data;
    in.allowed_actions_json = allowed.data;
    in.forbidden_actions_json = forbidden.data;
    in.validation_rules_json = rules.data;
    st = kalyx_make_envelope_file(&in, out_path);
    kalyx_buffer_free(&state); kalyx_buffer_free(&request); kalyx_buffer_free(&data); kalyx_buffer_free(&document);
    kalyx_buffer_free(&allowed); kalyx_buffer_free(&forbidden); kalyx_buffer_free(&rules);
    free(pack_request_path); free(pack_allowed_path); free(pack_rules_path); free(pack_state_path);
    if (st != KALYX_OK) { fprintf(stderr, "kalyx_make_envelope failed: %s\n", kalyx_status_string(st)); return 4; }
    printf("kalyx_make_envelope: wrote %s\n", out_path);
    return 0;
}
