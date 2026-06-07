#include "kalyx_json.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct KalyxJsonMember {
    char *key;
    struct KalyxJsonNode *value;
    struct KalyxJsonMember *next;
} KalyxJsonMember;

struct KalyxJsonNode {
    KalyxJsonType type;
    char *string_value;
    int bool_value;
    struct KalyxJsonNode **items;
    size_t item_count;
    KalyxJsonMember *members;
};

typedef struct Parser {
    const char *p;
} Parser;

static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

static KalyxJsonNode *new_node(KalyxJsonType type) {
    KalyxJsonNode *n = (KalyxJsonNode *)calloc(1u, sizeof(KalyxJsonNode));
    if (n) n->type = type;
    return n;
}

static void free_node(KalyxJsonNode *n) {
    size_t i;
    KalyxJsonMember *m;
    if (!n) return;
    free(n->string_value);
    for (i = 0u; i < n->item_count; i++) free_node(n->items[i]);
    free(n->items);
    m = n->members;
    while (m) {
        KalyxJsonMember *next = m->next;
        free(m->key);
        free_node(m->value);
        free(m);
        m = next;
    }
    free(n);
}

static char *parse_string_raw(Parser *ps) {
    const char *p = ps->p;
    char *out;
    size_t cap = 32u;
    size_t len = 0u;
    if (*p != '"') return NULL;
    p++;
    out = (char *)calloc(cap, 1u);
    if (!out) return NULL;
    while (*p && *p != '"') {
        char c = *p++;
        if (c == '\\') {
            c = *p++;
            switch (c) {
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'u':
                    /* KALYX keeps unicode escapes ASCII-stable in the validator. */
                    if (isxdigit((unsigned char)p[0]) && isxdigit((unsigned char)p[1]) && isxdigit((unsigned char)p[2]) && isxdigit((unsigned char)p[3])) {
                        c = '?';
                        p += 4;
                    } else { free(out); return NULL; }
                    break;
                default: free(out); return NULL;
            }
        }
        if (len + 2u >= cap) {
            char *np;
            cap *= 2u;
            np = (char *)realloc(out, cap);
            if (!np) { free(out); return NULL; }
            out = np;
        }
        out[len++] = c;
    }
    if (*p != '"') { free(out); return NULL; }
    p++;
    out[len] = '\0';
    ps->p = p;
    return out;
}

static KalyxJsonNode *parse_value(Parser *ps);

static KalyxJsonNode *parse_array(Parser *ps) {
    KalyxJsonNode *arr = new_node(KALYX_JSON_ARRAY);
    if (!arr || *ps->p != '[') return arr;
    ps->p++;
    ps->p = skip_ws(ps->p);
    if (*ps->p == ']') { ps->p++; return arr; }
    for (;;) {
        KalyxJsonNode *v;
        KalyxJsonNode **items;
        ps->p = skip_ws(ps->p);
        v = parse_value(ps);
        if (!v) { free_node(arr); return NULL; }
        items = (KalyxJsonNode **)realloc(arr->items, sizeof(KalyxJsonNode *) * (arr->item_count + 1u));
        if (!items) { free_node(v); free_node(arr); return NULL; }
        arr->items = items;
        arr->items[arr->item_count++] = v;
        ps->p = skip_ws(ps->p);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == ']') { ps->p++; return arr; }
        free_node(arr); return NULL;
    }
}

static KalyxJsonNode *parse_object(Parser *ps) {
    KalyxJsonNode *obj = new_node(KALYX_JSON_OBJECT);
    KalyxJsonMember **tail;
    if (!obj || *ps->p != '{') return obj;
    tail = &obj->members;
    ps->p++;
    ps->p = skip_ws(ps->p);
    if (*ps->p == '}') { ps->p++; return obj; }
    for (;;) {
        KalyxJsonMember *m;
        char *key;
        ps->p = skip_ws(ps->p);
        key = parse_string_raw(ps);
        if (!key) { free_node(obj); return NULL; }
        ps->p = skip_ws(ps->p);
        if (*ps->p != ':') { free(key); free_node(obj); return NULL; }
        ps->p++;
        ps->p = skip_ws(ps->p);
        m = (KalyxJsonMember *)calloc(1u, sizeof(KalyxJsonMember));
        if (!m) { free(key); free_node(obj); return NULL; }
        m->key = key;
        m->value = parse_value(ps);
        if (!m->value) { free(m->key); free(m); free_node(obj); return NULL; }
        *tail = m;
        tail = &m->next;
        ps->p = skip_ws(ps->p);
        if (*ps->p == ',') { ps->p++; continue; }
        if (*ps->p == '}') { ps->p++; return obj; }
        free_node(obj); return NULL;
    }
}

static KalyxJsonNode *parse_number(Parser *ps) {
    const char *p = ps->p;
    KalyxJsonNode *n;
    if (*p == '-') p++;
    if (!isdigit((unsigned char)*p)) return NULL;
    while (isdigit((unsigned char)*p)) p++;
    if (*p == '.') { p++; while (isdigit((unsigned char)*p)) p++; }
    if (*p == 'e' || *p == 'E') { p++; if (*p == '+' || *p == '-') p++; while (isdigit((unsigned char)*p)) p++; }
    n = new_node(KALYX_JSON_NUMBER);
    if (!n) return NULL;
    ps->p = p;
    return n;
}

static KalyxJsonNode *parse_value(Parser *ps) {
    KalyxJsonNode *n;
    ps->p = skip_ws(ps->p);
    if (*ps->p == '{') return parse_object(ps);
    if (*ps->p == '[') return parse_array(ps);
    if (*ps->p == '"') {
        n = new_node(KALYX_JSON_STRING);
        if (!n) return NULL;
        n->string_value = parse_string_raw(ps);
        if (!n->string_value) { free_node(n); return NULL; }
        return n;
    }
    if (strncmp(ps->p, "true", 4u) == 0) { n = new_node(KALYX_JSON_BOOL); if (!n) return NULL; n->bool_value = 1; ps->p += 4; return n; }
    if (strncmp(ps->p, "false", 5u) == 0) { n = new_node(KALYX_JSON_BOOL); if (!n) return NULL; n->bool_value = 0; ps->p += 5; return n; }
    if (strncmp(ps->p, "null", 4u) == 0) { n = new_node(KALYX_JSON_NULL); if (!n) return NULL; ps->p += 4; return n; }
    return parse_number(ps);
}

KalyxStatus kalyx_json_parse(const char *text, KalyxJsonDocument *doc) {
    Parser ps;
    if (!text || !doc) return KALYX_ERR_INVALID_ARGUMENT;
    doc->root = NULL;
    ps.p = skip_ws(text);
    doc->root = parse_value(&ps);
    if (!doc->root) return KALYX_ERR_PARSE;
    ps.p = skip_ws(ps.p);
    if (*ps.p != '\0') { kalyx_json_free(doc); return KALYX_ERR_PARSE; }
    return KALYX_OK;
}

void kalyx_json_free(KalyxJsonDocument *doc) {
    if (!doc) return;
    free_node(doc->root);
    doc->root = NULL;
}

const KalyxJsonNode *kalyx_json_root(const KalyxJsonDocument *doc) { return doc ? doc->root : NULL; }
KalyxJsonType kalyx_json_type(const KalyxJsonNode *node) { return node ? node->type : KALYX_JSON_NULL; }

const KalyxJsonNode *kalyx_json_object_get(const KalyxJsonNode *object, const char *key) {
    KalyxJsonMember *m;
    if (!object || object->type != KALYX_JSON_OBJECT || !key) return NULL;
    for (m = object->members; m; m = m->next) if (strcmp(m->key, key) == 0) return m->value;
    return NULL;
}

const KalyxJsonNode *kalyx_json_path(const KalyxJsonNode *root, const char *dot_path) {
    const KalyxJsonNode *cur = root;
    const char *p = dot_path;
    char key[96];
    if (!root || !dot_path) return NULL;
    while (*p) {
        size_t n = 0u;
        while (*p && *p != '.') {
            if (n + 1u < sizeof(key)) key[n++] = *p;
            p++;
        }
        key[n] = '\0';
        cur = kalyx_json_object_get(cur, key);
        if (!cur) return NULL;
        if (*p == '.') p++;
    }
    return cur;
}

size_t kalyx_json_array_size(const KalyxJsonNode *array) { return (array && array->type == KALYX_JSON_ARRAY) ? array->item_count : 0u; }
const KalyxJsonNode *kalyx_json_array_get(const KalyxJsonNode *array, size_t index) { return (array && array->type == KALYX_JSON_ARRAY && index < array->item_count) ? array->items[index] : NULL; }
const char *kalyx_json_string_value(const KalyxJsonNode *node) { return (node && node->type == KALYX_JSON_STRING) ? node->string_value : NULL; }
int kalyx_json_bool_value(const KalyxJsonNode *node, int *out) { if (!node || node->type != KALYX_JSON_BOOL || !out) return 0; *out = node->bool_value; return 1; }

int kalyx_json_string_array_contains(const KalyxJsonNode *array, const char *value) {
    size_t i;
    if (!array || array->type != KALYX_JSON_ARRAY || !value) return 0;
    for (i = 0u; i < array->item_count; i++) {
        const char *s = kalyx_json_string_value(array->items[i]);
        if (s && strcmp(s, value) == 0) return 1;
    }
    return 0;
}
