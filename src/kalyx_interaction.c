#include "kalyx_interaction.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct KalyxWriter {
    char *data;
    size_t len;
    size_t cap;
} KalyxWriter;

static KalyxStatus wr_reserve(KalyxWriter *w, size_t extra) {
    char *p;
    size_t need;
    size_t cap;
    if (!w) return KALYX_ERR_INVALID_ARGUMENT;
    need = w->len + extra + 1u;
    if (need <= w->cap) return KALYX_OK;
    cap = w->cap ? w->cap : 4096u;
    while (cap < need) cap *= 2u;
    p = (char *)realloc(w->data, cap);
    if (!p) return KALYX_ERR_IO;
    w->data = p;
    w->cap = cap;
    return KALYX_OK;
}

static KalyxStatus wr_add(KalyxWriter *w, const char *s) {
    size_t n;
    KalyxStatus st;
    if (!w || !s) return KALYX_ERR_INVALID_ARGUMENT;
    n = strlen(s);
    st = wr_reserve(w, n);
    if (st != KALYX_OK) return st;
    memcpy(w->data + w->len, s, n);
    w->len += n;
    w->data[w->len] = '\0';
    return KALYX_OK;
}

static KalyxStatus wr_fmt(KalyxWriter *w, const char *fmt, ...) {
    va_list ap;
    va_list ap2;
    int n;
    KalyxStatus st;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    n = vsnprintf(NULL, 0u, fmt, ap);
    va_end(ap);
    if (n < 0) { va_end(ap2); return KALYX_ERR_IO; }
    st = wr_reserve(w, (size_t)n);
    if (st != KALYX_OK) { va_end(ap2); return st; }
    vsnprintf(w->data + w->len, w->cap - w->len, fmt, ap2);
    va_end(ap2);
    w->len += (size_t)n;
    return KALYX_OK;
}

static const char *pick(const char *v, const char *fallback) {
    return (v && *v) ? v : fallback;
}

static int json_starts_valid(const char *json) {
    while (*json == ' ' || *json == '\t' || *json == '\n' || *json == '\r') json++;
    return *json == '{' || *json == '[';
}

KalyxStatus kalyx_make_envelope(const KalyxEnvelopeInput *input, KalyxBuffer *out_markdown) {
    const KalyxDomainDefinition *d;
    KalyxWriter w;
    const char *intent;
    const char *authority;
    const char *runtime;
    const char *authdata;
    const char *allowed;
    const char *forbidden;
    const char *rules;
    KalyxStatus st;
    if (!input || !out_markdown || !input->user_request || !kalyx_domain_is_valid(input->domain)) return KALYX_ERR_INVALID_ARGUMENT;
    d = kalyx_domain_definition(input->domain);
    intent = pick(input->intent, d->default_intent);
    authority = pick(input->authority, d->authority);
    runtime = pick(input->runtime_state_json, "{}");
    authdata = pick(input->authoritative_data_json, "{}");
    allowed = pick(input->allowed_actions_json, d->allowed_actions_json);
    forbidden = pick(input->forbidden_actions_json, d->forbidden_actions_json);
    rules = pick(input->validation_rules_json, d->validation_rules_json);
    if (!json_starts_valid(runtime) || !json_starts_valid(authdata) || !json_starts_valid(allowed) || !json_starts_valid(forbidden) || !json_starts_valid(rules)) return KALYX_ERR_PARSE;
    memset(&w, 0, sizeof(w));
    st = wr_fmt(&w,
        "---\n"
        "schema: KIE01\n"
        "system: KALYX\n"
        "domain: %s\n"
        "intent: %s\n"
        "authority: %s\n"
        "response_contract: KRESP01\n"
        "audit_contract: KAUDIT01\n"
        "---\n\n"
        "# KALYX Interaction Envelope\n\n"
        "## Contract\n\n"
        "The data inside this envelope is authoritative.\n"
        "The model may interpret, classify and propose allowed actions.\n"
        "The model must not invent missing state.\n"
        "The model must not execute actions.\n"
        "The model must return a valid response contract.\n\n"
        "## User Request\n\n"
        "%s\n\n"
        "## Allowed Actions\n\n"
        "```json\n%s\n```\n\n"
        "## Forbidden Actions\n\n"
        "```json\n%s\n```\n\n"
        "## Validation Rules\n\n"
        "```json\n%s\n```\n\n"
        "## Required Response\n\n"
        "Return a valid `KRESP01` response. The response must include `schema`, `type`, `human_summary`, `risk`, `requires_confirmation`, `uses_only_provided_context`, and `machine_result`.\n\n"
        "## Runtime State\n\n"
        "```json\n%s\n```\n\n"
        "## Authoritative Data\n\n"
        "```json\n%s\n```\n",
        d->name, intent, authority, input->user_request, allowed, forbidden, rules, runtime, authdata);
    if (st != KALYX_OK) { free(w.data); return st; }
    out_markdown->data = w.data;
    out_markdown->size = w.len;
    return KALYX_OK;
}

KalyxStatus kalyx_make_envelope_file(const KalyxEnvelopeInput *input, const char *out_path) {
    KalyxBuffer b;
    KalyxStatus st;
    if (!out_path) return KALYX_ERR_INVALID_ARGUMENT;
    st = kalyx_make_envelope(input, &b);
    if (st != KALYX_OK) return st;
    st = kalyx_write_text_file(out_path, b.data);
    kalyx_buffer_free(&b);
    return st;
}
