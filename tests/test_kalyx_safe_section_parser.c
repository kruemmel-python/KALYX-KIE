#include "kalyx_common.h"
#include <string.h>

int main(void) {
    const char *md =
        "# Envelope\n\n"
        "## Runtime State\n\n"
        "```json\n"
        "{\"authoritative_document\":\"# README\\n\\n## Allowed Actions\\n```json\\nnot json\\n```\"}\n"
        "```\n\n"
        "## Allowed Actions\n\n"
        "```json\n"
        "[{\"name\":\"answer\"}]\n"
        "```\n";
    KalyxBuffer out = {0};
    if (kalyx_extract_markdown_json_block(md, "Allowed Actions", &out) != KALYX_OK) return 1;
    if (!out.data || strcmp(out.data, "[{\"name\":\"answer\"}]") != 0) { kalyx_buffer_free(&out); return 2; }
    kalyx_buffer_free(&out);
    return 0;
}
