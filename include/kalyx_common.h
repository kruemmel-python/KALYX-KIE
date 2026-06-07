#ifndef KALYX_COMMON_H
#define KALYX_COMMON_H

#include <stddef.h>
#include "kalyx_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KALYX_VERSION "1.0-kie-v1.0-final"
#define KALYX_SCHEMA_KIE "KIE01"
#define KALYX_SCHEMA_KRESP "KRESP01"
#define KALYX_SCHEMA_KAUDIT "KAUDIT01"

typedef struct KalyxBuffer {
    char *data;
    size_t size;
} KalyxBuffer;

KalyxStatus kalyx_read_text_file(const char *path, KalyxBuffer *out);
KalyxStatus kalyx_write_text_file(const char *path, const char *text);
void kalyx_buffer_free(KalyxBuffer *buf);
int kalyx_text_contains_token(const char *text, const char *token);
KalyxStatus kalyx_extract_markdown_json_block(const char *markdown,
                                              const char *section_name,
                                              KalyxBuffer *json_out);

#ifdef __cplusplus
}
#endif

#endif
