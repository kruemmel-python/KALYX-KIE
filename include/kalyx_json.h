#ifndef KALYX_JSON_H
#define KALYX_JSON_H

#include <stddef.h>
#include "kalyx_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum KalyxJsonType {
    KALYX_JSON_NULL = 0,
    KALYX_JSON_OBJECT = 1,
    KALYX_JSON_ARRAY = 2,
    KALYX_JSON_STRING = 3,
    KALYX_JSON_BOOL = 4,
    KALYX_JSON_NUMBER = 5
} KalyxJsonType;

typedef struct KalyxJsonNode KalyxJsonNode;

typedef struct KalyxJsonDocument {
    KalyxJsonNode *root;
} KalyxJsonDocument;

KalyxStatus kalyx_json_parse(const char *text, KalyxJsonDocument *doc);
void kalyx_json_free(KalyxJsonDocument *doc);
const KalyxJsonNode *kalyx_json_root(const KalyxJsonDocument *doc);
KalyxJsonType kalyx_json_type(const KalyxJsonNode *node);
const KalyxJsonNode *kalyx_json_object_get(const KalyxJsonNode *object, const char *key);
const KalyxJsonNode *kalyx_json_path(const KalyxJsonNode *root, const char *dot_path);
size_t kalyx_json_array_size(const KalyxJsonNode *array);
const KalyxJsonNode *kalyx_json_array_get(const KalyxJsonNode *array, size_t index);
const char *kalyx_json_string_value(const KalyxJsonNode *node);
int kalyx_json_bool_value(const KalyxJsonNode *node, int *out);
int kalyx_json_string_array_contains(const KalyxJsonNode *array, const char *value);

#ifdef __cplusplus
}
#endif

#endif
