#include "kalyx_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

KalyxStatus kalyx_read_text_file(const char *path, KalyxBuffer *out) {
    FILE *f;
    long n;
    char *buf;
    if (!path || !out) return KALYX_ERR_INVALID_ARGUMENT;
    out->data = NULL;
    out->size = 0;
    f = fopen(path, "rb");
    if (!f) return KALYX_ERR_IO;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return KALYX_ERR_IO; }
    n = ftell(f);
    if (n < 0) { fclose(f); return KALYX_ERR_IO; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return KALYX_ERR_IO; }
    buf = (char *)calloc((size_t)n + 1u, 1u);
    if (!buf) { fclose(f); return KALYX_ERR_IO; }
    if ((size_t)n != fread(buf, 1u, (size_t)n, f)) {
        free(buf);
        fclose(f);
        return KALYX_ERR_IO;
    }
    if (fclose(f) != 0) {
        free(buf);
        return KALYX_ERR_IO;
    }
    out->data = buf;
    out->size = (size_t)n;
    return KALYX_OK;
}

KalyxStatus kalyx_write_text_file(const char *path, const char *text) {
    FILE *f;
    size_t n;
    if (!path || !text) return KALYX_ERR_INVALID_ARGUMENT;
    f = fopen(path, "wb");
    if (!f) return KALYX_ERR_IO;
    n = strlen(text);
    if (n != fwrite(text, 1u, n, f)) { fclose(f); return KALYX_ERR_IO; }
    if (fclose(f) != 0) return KALYX_ERR_IO;
    return KALYX_OK;
}

void kalyx_buffer_free(KalyxBuffer *buf) {
    if (!buf) return;
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
}

int kalyx_text_contains_token(const char *text, const char *token) {
    const char *p;
    size_t n;
    if (!text || !token || !*token) return 0;
    n = strlen(token);
    for (p = strstr(text, token); p; p = strstr(p + 1, token)) {
        const char before = (p == text) ? '\0' : p[-1];
        const char after = p[n];
        const int lb = before == '\0' || before == '"' || before == '\'' || before == '[' || before == '{' || before == ':' || before == ',' || before == '\n' || before == ' ' || before == '\t';
        const int rb = after == '\0' || after == '"' || after == '\'' || after == ']' || after == '}' || after == ':' || after == ',' || after == '\n' || after == ' ' || after == '\t';
        if (lb && rb) return 1;
    }
    return 0;
}

static int is_line_start(const char *base, const char *p) {
    return p == base || p[-1] == '\n' || p[-1] == '\r';
}

static int fence_toggles_at_line(const char *p) {
    return p[0] == '`' && p[1] == '`' && p[2] == '`';
}

static const char *find_section_heading_outside_fences(const char *markdown, const char *section_name) {
    char heading[160];
    size_t heading_len;
    const char *p;
    int in_fence = 0;
    if (snprintf(heading, sizeof(heading), "## %s", section_name) >= (int)sizeof(heading)) return NULL;
    heading_len = strlen(heading);
    for (p = markdown; *p;) {
        if (is_line_start(markdown, p) && fence_toggles_at_line(p)) {
            in_fence = !in_fence;
        }
        if (!in_fence && is_line_start(markdown, p) && strncmp(p, heading, heading_len) == 0) {
            char c = p[heading_len];
            if (c == '\0' || c == '\n' || c == '\r' || c == ' ' || c == '\t') return p;
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    return NULL;
}

static const char *find_json_fence_after_section(const char *markdown, const char *section_start) {
    const char *p;
    int in_fence = 0;
    for (p = section_start; *p;) {
        if (is_line_start(markdown, p) && !in_fence && p != section_start && p[0] == '#' && p[1] == '#') return NULL;
        if (is_line_start(markdown, p) && strncmp(p, "```json", 7) == 0) return p;
        if (is_line_start(markdown, p) && fence_toggles_at_line(p)) in_fence = !in_fence;
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    return NULL;
}

KalyxStatus kalyx_extract_markdown_json_block(const char *markdown,
                                              const char *section_name,
                                              KalyxBuffer *json_out) {
    const char *start;
    const char *fence;
    const char *body;
    const char *end;
    size_t n;
    if (!markdown || !json_out) return KALYX_ERR_INVALID_ARGUMENT;
    json_out->data = NULL;
    json_out->size = 0;
    if (section_name && *section_name) {
        start = find_section_heading_outside_fences(markdown, section_name);
        if (!start) return KALYX_ERR_PARSE;
    } else {
        start = markdown;
    }
    fence = find_json_fence_after_section(markdown, start);
    if (!fence) return KALYX_ERR_PARSE;
    body = strchr(fence, '\n');
    if (!body) return KALYX_ERR_PARSE;
    body++;
    end = strstr(body, "```");
    if (!end) return KALYX_ERR_PARSE;
    n = (size_t)(end - body);
    while (n > 0u && (body[n - 1u] == '\n' || body[n - 1u] == '\r')) n--;
    json_out->data = (char *)calloc(n + 1u, 1u);
    if (!json_out->data) return KALYX_ERR_IO;
    memcpy(json_out->data, body, n);
    json_out->size = n;
    return KALYX_OK;
}
