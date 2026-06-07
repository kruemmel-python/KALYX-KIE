#include "kalyx_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define KALYX_MKDIR(path) _mkdir(path)
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#define KALYX_MKDIR(path) mkdir(path, 0777)
#endif

typedef struct KalyxPromptProfile {
    const char *name;
    const char *domain;
    const char *intent;
    const char *mode;
    const char *default_goal;
    const char *description;
    const char *targets[8];
    int target_count;
    const char *required_order[8];
    int order_count;
    int workflow;
    int max_steps;
    const char *default_mode;
    const char *default_theme;
} KalyxPromptProfile;

static const KalyxPromptProfile profiles[] = {
    {"app.summary_export", "app", "summarize_and_export", "workflow",
     "Fasse README.md für neue Benutzer zusammen und erzeuge danach einen sicheren Markdown-Export in der Sandbox.",
     "App-Dokument zusammenfassen und als Sandbox-Artefakt exportieren.",
     {"export_document", "open_preview", "save_as"}, 3,
     {"export_document", "open_preview", "save_as"}, 3, 1, 8, "markdown", "plain"},
    {"app.code_review", "app", "code_review_report", "workflow",
     "Analysiere bereitgestellten Code, erstelle einen Review-Bericht und exportiere ihn in die Sandbox.",
     "Code-/Dokumentreview mit Sandbox-Bericht.",
     {"export_document", "open_preview", "save_as"}, 3,
     {"export_document", "open_preview", "save_as"}, 3, 1, 8, "markdown", "plain"},
    {"game.npc_action", "game", "npc_action_plan", "command",
     "Erzeuge einen sicheren NPC-Aktionsvorschlag für eine Spielwelt ohne echte externe Seiteneffekte.",
     "Spiel-/NPC-Aktion als validierbarer UI-Command.",
     {"export_document", "open_preview"}, 2,
     {"export_document", "open_preview"}, 2, 1, 4, "markdown", "rpg"},
    {"tool.file_transform", "app", "file_transform_sandbox", "workflow",
     "Transformiere eine Datei ausschließlich als Sandbox-Kopie und erzeuge einen Preview-Request.",
     "Werkzeugprofil für sichere Datei-Transformation ohne Überschreiben.",
     {"export_document", "open_preview", "save_as"}, 3,
     {"export_document", "open_preview", "save_as"}, 3, 1, 8, "markdown", "plain"},
    {"research.hypothesis_report", "research", "hypothesis_report", "workflow",
     "Erstelle einen klar getrennten Forschungsbericht mit Fakten, Hypothesen, Unsicherheiten und Sandbox-Export.",
     "Forschungsbericht mit strikter Trennung von Fakt und Hypothese.",
     {"export_document", "open_preview", "save_as"}, 3,
     {"export_document", "open_preview", "save_as"}, 3, 1, 8, "markdown", "scientific"},
    {"workflow.multi_action", "workflow", "multi_action_workflow", "workflow",
     "Erzeuge einen mehrstufigen Sandbox-Workflow mit Export, Preview und Save-As-Artefakt.",
     "Generisches Multi-Action-Workflow-Profil.",
     {"export_document", "open_preview", "save_as"}, 3,
     {"export_document", "open_preview", "save_as"}, 3, 1, 8, "markdown", "plain"}
};

static int profile_count(void) { return (int)(sizeof(profiles) / sizeof(profiles[0])); }

static char *join_path(const char *dir, const char *file);

static const KalyxPromptProfile *find_profile(const char *name) {
    int i;
    if (!name) return NULL;
    for (i = 0; i < profile_count(); i++) if (strcmp(profiles[i].name, name) == 0) return &profiles[i];
    return NULL;
}


static char *dup_range(const char *a, const char *b) {
    size_t n;
    char *out;
    if (!a || !b || b < a) return NULL;
    n = (size_t)(b - a);
    out = (char *)calloc(n + 1u, 1u);
    if (!out) return NULL;
    memcpy(out, a, n);
    return out;
}

static const char *skip_ws(const char *p) {
    while (p && *p && isspace((unsigned char)*p)) p++;
    return p;
}

static char *json_get_string_value(const char *json, const char *key) {
    char needle[128];
    const char *p;
    const char *q;
    const char *start;
    char *out;
    size_t klen;
    if (!json || !key) return NULL;
    klen = strlen(key);
    if (klen + 4u >= sizeof(needle)) return NULL;
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    p = strstr(json, needle);
    if (!p) return NULL;
    p += strlen(needle);
    p = strchr(p, ':');
    if (!p) return NULL;
    p = skip_ws(p + 1);
    if (!p || *p != '"') return NULL;
    p++;
    start = p;
    while (*p) {
        if (*p == '\\' && p[1]) { p += 2; continue; }
        if (*p == '"') break;
        p++;
    }
    if (*p != '"') return NULL;
    out = (char *)calloc((size_t)(p - start) + 1u, 1u);
    if (!out) return NULL;
    q = start;
    start = out;
    while (q < p) {
        if (*q == '\\' && q + 1 < p) {
            q++;
            switch (*q) { case 'n': *out++ = '\n'; break; case 'r': *out++ = '\r'; break; case 't': *out++ = '\t'; break; default: *out++ = *q; break; }
            q++;
        } else {
            *out++ = *q++;
        }
    }
    *out = '\0';
    return (char *)start;
}

static int json_get_int_value(const char *json, const char *key, int fallback) {
    char needle[128];
    const char *p;
    if (!json || !key || strlen(key) + 4u >= sizeof(needle)) return fallback;
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    p = strstr(json, needle);
    if (!p) return fallback;
    p += strlen(needle);
    p = strchr(p, ':');
    if (!p) return fallback;
    p = skip_ws(p + 1);
    return p ? atoi(p) : fallback;
}

static int json_get_bool_value(const char *json, const char *key, int fallback) {
    char needle[128];
    const char *p;
    if (!json || !key || strlen(key) + 4u >= sizeof(needle)) return fallback;
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    p = strstr(json, needle);
    if (!p) return fallback;
    p += strlen(needle);
    p = strchr(p, ':');
    if (!p) return fallback;
    p = skip_ws(p + 1);
    if (!p) return fallback;
    if (strncmp(p, "true", 4) == 0) return 1;
    if (strncmp(p, "false", 5) == 0) return 0;
    return fallback;
}

static int json_get_string_array(const char *json, const char *key, const char **items, int max_items) {
    char needle[128];
    const char *p;
    int count = 0;
    if (!json || !key || !items || max_items <= 0 || strlen(key) + 4u >= sizeof(needle)) return 0;
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    p = strstr(json, needle);
    if (!p) return 0;
    p += strlen(needle);
    p = strchr(p, ':');
    if (!p) return 0;
    p = skip_ws(p + 1);
    if (!p || *p != '[') return 0;
    p++;
    while (*p && *p != ']' && count < max_items) {
        p = skip_ws(p);
        if (*p == ',') { p++; continue; }
        if (*p != '"') break;
        p++;
        {
            const char *start = p;
            while (*p) {
                if (*p == '\\' && p[1]) { p += 2; continue; }
                if (*p == '"') break;
                p++;
            }
            if (*p != '"') break;
            items[count++] = dup_range(start, p);
            p++;
        }
    }
    return count;
}

static int ends_with(const char *s, const char *suffix) {
    size_t a, b;
    if (!s || !suffix) return 0;
    a = strlen(s); b = strlen(suffix);
    return a >= b && strcmp(s + a - b, suffix) == 0;
}

static int validate_profile(const KalyxPromptProfile *p, char *err, size_t err_size) {
    int i;
    if (!p || !p->name || !*p->name || !p->domain || !*p->domain || !p->intent || !*p->intent || !p->mode || !*p->mode) {
        snprintf(err, err_size, "profile requires name/domain/intent/mode"); return 0;
    }
    if (strcmp(p->mode, "workflow") != 0 && strcmp(p->mode, "command") != 0) {
        snprintf(err, err_size, "profile mode must be workflow or command"); return 0;
    }
    if (p->target_count <= 0 || p->target_count > 8 || p->order_count < 0 || p->order_count > 8) {
        snprintf(err, err_size, "profile target/order count out of range"); return 0;
    }
    if (p->max_steps <= 0 || p->max_steps > 32) { snprintf(err, err_size, "profile max_steps must be 1..32"); return 0; }
    for (i = 0; i < p->target_count; i++) if (!p->targets[i] || !*p->targets[i]) { snprintf(err, err_size, "empty target in profile"); return 0; }
    return 1;
}

static void init_custom_profile_defaults(KalyxPromptProfile *out) {
    memset(out, 0, sizeof(*out));
    out->name = "custom.file_profile";
    out->domain = "app";
    out->intent = "custom_profile_intent";
    out->mode = "workflow";
    out->default_goal = "Erzeuge einen sicheren Sandbox-Workflow.";
    out->description = "Benutzerdefiniertes KALYX-Profil.";
    out->targets[0] = "export_document";
    out->targets[1] = "open_preview";
    out->targets[2] = "save_as";
    out->target_count = 3;
    out->required_order[0] = "export_document";
    out->required_order[1] = "open_preview";
    out->required_order[2] = "save_as";
    out->order_count = 3;
    out->workflow = 1;
    out->max_steps = 8;
    out->default_mode = "markdown";
    out->default_theme = "plain";
}

static int load_profile_file(const char *path, KalyxPromptProfile *out, char *err, size_t err_size) {
    KalyxBuffer b = {0};
    char *v;
    int n;
    if (kalyx_read_text_file(path, &b) != KALYX_OK) { snprintf(err, err_size, "cannot read profile file: %s", path); return 0; }
    init_custom_profile_defaults(out);
    if ((v = json_get_string_value(b.data, "name"))) out->name = v;
    if ((v = json_get_string_value(b.data, "domain"))) out->domain = v;
    if ((v = json_get_string_value(b.data, "intent"))) out->intent = v;
    if ((v = json_get_string_value(b.data, "mode"))) out->mode = v;
    if ((v = json_get_string_value(b.data, "default_goal"))) out->default_goal = v;
    if ((v = json_get_string_value(b.data, "description"))) out->description = v;
    if ((v = json_get_string_value(b.data, "default_mode"))) out->default_mode = v;
    if ((v = json_get_string_value(b.data, "default_theme"))) out->default_theme = v;
    n = json_get_string_array(b.data, "targets", out->targets, 8); if (n > 0) out->target_count = n;
    n = json_get_string_array(b.data, "required_order", out->required_order, 8); if (n > 0) out->order_count = n;
    out->workflow = json_get_bool_value(b.data, "workflow", out->workflow);
    out->max_steps = json_get_int_value(b.data, "max_steps", out->max_steps);
    kalyx_buffer_free(&b);
    return validate_profile(out, err, err_size);
}

static char *profile_path_from_dir(const char *dir, const char *name) {
    char file[256];
    if (!dir || !name || strlen(name) + 16u >= sizeof(file)) return NULL;
    snprintf(file, sizeof(file), "%s.kprofile.json", name);
    return join_path(dir, file);
}

static int list_profile_dir(const char *dir) {
    int found = 0;
#ifdef _WIN32
    char *pattern = join_path(dir, "*.kprofile.json");
    WIN32_FIND_DATAA data;
    HANDLE h;
    if (!pattern) return 0;
    h = FindFirstFileA(pattern, &data);
    free(pattern);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        printf("  %s\n    source=%s\n", data.cFileName, dir);
        found++;
    } while (FindNextFileA(h, &data));
    FindClose(h);
#else
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d) return 0;
    while ((e = readdir(d)) != NULL) {
        if (ends_with(e->d_name, ".kprofile.json")) { printf("  %s\n    source=%s\n", e->d_name, dir); found++; }
    }
    closedir(d);
#endif
    return found;
}

static void list_profiles(void) {
    int i;
    puts("KALYX Prompt Profiles:");
    for (i = 0; i < profile_count(); i++) {
        printf("  %s\n    domain=%s intent=%s mode=%s\n    %s\n",
               profiles[i].name, profiles[i].domain, profiles[i].intent,
               profiles[i].mode, profiles[i].description);
    }
}

static void list_profiles_with_dir(const char *dir) {
    list_profiles();
    if (dir) {
        puts("Custom KALYX Profile Registry:");
        if (!list_profile_dir(dir)) printf("  no custom profiles found in %s\n", dir);
    }
}

static void usage(void) {
    puts("KALYX-KIE v1.0 Final Prompt Author");
    puts("");
    puts("Usage:");
    puts("  kalyx_prompt_author --domain app|game|agent|code|research|workflow --intent NAME --goal TEXT [--out FILE] [--pack DIR]");
    puts("  kalyx_prompt_author --profile NAME [--profile-dir DIR] [--goal TEXT] [--out FILE] [--pack DIR]");
    puts("  kalyx_prompt_author --profile-file FILE [--goal TEXT] [--out FILE] [--pack DIR]");
    puts("  kalyx_prompt_author --list-profiles [--profile-dir DIR]");
    puts("  kalyx_prompt_author --version");
    puts("");
    puts("Purpose:");
    puts("  Generate KPROMPT01 prompt assets, KCONTRACT01 governance contracts, and reusable .kpromptpack directories.");
    puts("");
    puts("Examples:");
    puts("  kalyx_prompt_author --list-profiles --profile-dir .\\profiles");
    puts("  kalyx_prompt_author --profile app.summary_export --goal \"Fasse README.md zusammen\" --pack .\\out\\pack.kpromptpack");
    puts("  kalyx_prompt_author --profile-file .\\profiles\\custom.summary_export.kprofile.json --pack .\\out\\custom.kpromptpack");
    puts("");
    puts("Schemas:");
    puts("  KPROMPT01   human-readable prompt asset");
    puts("  KCONTRACT01 machine-readable action and workflow contract");
    puts("  KPROFILE01  reusable domain/profile definition");
}

static const char *arg_value(int argc, char **argv, const char *name) {
    int i;
    for (i = 1; i + 1 < argc; i++) if (strcmp(argv[i], name) == 0) return argv[i + 1];
    return NULL;
}

static int has_arg(int argc, char **argv, const char *name) {
    int i;
    for (i = 1; i < argc; i++) if (strcmp(argv[i], name) == 0) return 1;
    return 0;
}

static int ensure_dir(const char *path) {
    if (!path || !*path) return 0;
    if (KALYX_MKDIR(path) == 0) return 0;
    if (errno == EEXIST) return 0;
    return -1;
}

static char *join_path(const char *dir, const char *file) {
    size_t a, b;
    char *out;
    char sep = '/';
    if (!dir || !file) return NULL;
    a = strlen(dir);
    b = strlen(file);
    out = (char *)calloc(a + b + 2u, 1u);
    if (!out) return NULL;
    memcpy(out, dir, a);
    if (a > 0u && dir[a - 1u] != '/' && dir[a - 1u] != '\\') out[a++] = sep;
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
    KalyxStatus st = append_cstr(buf, len, cap, "\"");
    if (st != KALYX_OK) return st;
    for (p = (const unsigned char *)(s ? s : ""); *p; p++) {
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

static KalyxStatus append_json_string_array(char **buf, size_t *len, size_t *cap, const char * const *items, int count) {
    int i;
    KalyxStatus st = append_cstr(buf, len, cap, "[");
    if (st != KALYX_OK) return st;
    for (i = 0; i < count; i++) {
        if (i) { st = append_cstr(buf, len, cap, ", "); if (st != KALYX_OK) return st; }
        st = append_json_string(buf, len, cap, items[i]);
        if (st != KALYX_OK) return st;
    }
    return append_cstr(buf, len, cap, "]");
}

static char *make_kprompt(const char *domain, const char *intent, const char *goal, const KalyxPromptProfile *profile) {
    char *buf = NULL;
    size_t len = 0u, cap = 0u;
    append_cstr(&buf, &len, &cap, "---\nschema: KPROMPT01\nkalyx_version: ");
    append_cstr(&buf, &len, &cap, KALYX_VERSION);
    append_cstr(&buf, &len, &cap, "\ndomain: "); append_cstr(&buf, &len, &cap, domain);
    append_cstr(&buf, &len, &cap, "\nintent: "); append_cstr(&buf, &len, &cap, intent);
    append_cstr(&buf, &len, &cap, "\nmode: "); append_cstr(&buf, &len, &cap, profile ? profile->mode : "workflow");
    append_cstr(&buf, &len, &cap, "\nprofile: "); append_cstr(&buf, &len, &cap, profile ? profile->name : "custom");
    append_cstr(&buf, &len, &cap, "\ncontract_schema: KCONTRACT01\nresponse_contract: KRESP01\naudit_contracts: [KAUDIT01, KDISPATCH01, KWORKFLOW01]\n---\n\n");
    append_cstr(&buf, &len, &cap, "# KALYX Prompt Authoring Asset\n\n## Goal\n\n");
    append_cstr(&buf, &len, &cap, goal);
    append_cstr(&buf, &len, &cap, "\n\n## Profile\n\n");
    append_cstr(&buf, &len, &cap, profile ? profile->name : "custom");
    append_cstr(&buf, &len, &cap, "\n\n## User-facing style\n\nDeutsch, verständlich, professionell, präzise.\n\n## Safety\n\n- Keine OS-Befehle.\n- Keine echten Dateien überschreiben.\n- Nur Sandbox-Artefakte schreiben.\n- Nutze ausschließlich erlaubte Aktionen.\n\n## Required result\n\nAntworte als gültige KALYX KRESP01 Response. Für Workflows muss `machine_result.command = \"workflow\"` sein.\n");
    return buf;
}

static char *make_request(const char *goal, const KalyxPromptProfile *profile) {
    char *buf = NULL;
    size_t len = 0u, cap = 0u;
    int i;
    append_cstr(&buf, &len, &cap, "Antworte ausschließlich als gültige KALYX KRESP01 Response.\n\nZiel:\n");
    append_cstr(&buf, &len, &cap, goal);
    append_cstr(&buf, &len, &cap, "\n\n");
    if (profile) {
        append_cstr(&buf, &len, &cap, "Aktives KALYX-Profil: "); append_cstr(&buf, &len, &cap, profile->name); append_cstr(&buf, &len, &cap, "\n");
        append_cstr(&buf, &len, &cap, profile->workflow ? "Du musst type=\"command\" und machine_result.command=\"workflow\" verwenden.\n" : "Du musst type=\"command\" verwenden.\n");
        append_cstr(&buf, &len, &cap, "Erlaubte Targets: ");
        for (i = 0; i < profile->target_count; i++) { if (i) append_cstr(&buf, &len, &cap, ", "); append_cstr(&buf, &len, &cap, profile->targets[i]); }
        append_cstr(&buf, &len, &cap, "\nPolicy-Reihenfolge: ");
        for (i = 0; i < profile->order_count; i++) { if (i) append_cstr(&buf, &len, &cap, " -> "); append_cstr(&buf, &len, &cap, profile->required_order[i]); }
        append_cstr(&buf, &len, &cap, "\n");
    } else {
        append_cstr(&buf, &len, &cap, "Wenn ein Workflow benötigt wird, verwende type=\"command\" und machine_result.command=\"workflow\".\nErlaubte Sandbox-Ziele sind export_document, open_preview und save_as.\n");
    }
    append_cstr(&buf, &len, &cap, "Keine OS-Befehle. Keine echten Dateien überschreiben. Nur Sandbox-Artefakte.\n");
    return buf;
}

static char *make_contract(const char *domain, const char *intent, const char *goal, const KalyxPromptProfile *profile) {
    char *buf = NULL;
    size_t len = 0u, cap = 0u;
    char tmp[32];
    append_cstr(&buf, &len, &cap, "{\n  \"schema\": \"KCONTRACT01\",\n  \"kalyx_version\": ");
    append_json_string(&buf, &len, &cap, KALYX_VERSION);
    append_cstr(&buf, &len, &cap, ",\n  \"domain\": "); append_json_string(&buf, &len, &cap, domain);
    append_cstr(&buf, &len, &cap, ",\n  \"intent\": "); append_json_string(&buf, &len, &cap, intent);
    append_cstr(&buf, &len, &cap, ",\n  \"profile\": "); append_json_string(&buf, &len, &cap, profile ? profile->name : "custom");
    append_cstr(&buf, &len, &cap, ",\n  \"profile_description\": "); append_json_string(&buf, &len, &cap, profile ? profile->description : "custom prompt pack");
    append_cstr(&buf, &len, &cap, ",\n  \"goal\": "); append_json_string(&buf, &len, &cap, goal);
    append_cstr(&buf, &len, &cap, ",\n  \"response_schema\": \"KRESP01\",\n  \"allowed_actions\": [\n    {\n      \"name\": \"emit_ui_command\",\n      \"targets\": ");
    if (profile) append_json_string_array(&buf, &len, &cap, profile->targets, profile->target_count); else append_cstr(&buf, &len, &cap, "[\"export_document\", \"open_preview\", \"save_as\"]");
    append_cstr(&buf, &len, &cap, ",\n      \"args\": {\n        \"mode\": [\"markdown\", \"html\", \"pdf\", \"reveal\"],\n        \"theme\": [\"plain\", \"dark\", \"scientific\", \"rpg\"]\n      }\n    },\n    {\n      \"name\": \"workflow\",\n      \"targets\": [\"multi_action_sandbox\"],\n      \"args\": {\n        \"mode\": [\"markdown\"],\n        \"theme\": [\"plain\"]\n      }\n    }\n  ],\n  \"forbidden_actions\": [\"execute_external_command\", \"delete_files\", \"overwrite_real_files\"],\n  \"workflow_policy\": {\n    \"max_steps\": ");
    snprintf(tmp, sizeof(tmp), "%d", profile ? profile->max_steps : 8);
    append_cstr(&buf, &len, &cap, tmp);
    append_cstr(&buf, &len, &cap, ",\n    \"sandbox_only\": true,\n    \"required_order\": ");
    if (profile) append_json_string_array(&buf, &len, &cap, profile->required_order, profile->order_count); else append_cstr(&buf, &len, &cap, "[\"export_document\", \"open_preview\", \"save_as\"]");
    append_cstr(&buf, &len, &cap, ",\n    \"forbid_duplicate_targets\": true\n  },\n  \"defaults\": {\n    \"mode\": "); append_json_string(&buf, &len, &cap, profile ? profile->default_mode : "markdown");
    append_cstr(&buf, &len, &cap, ",\n    \"theme\": "); append_json_string(&buf, &len, &cap, profile ? profile->default_theme : "plain");
    append_cstr(&buf, &len, &cap, "\n  }\n}\n");
    return buf;
}

static const char *allowed_actions_json =
"[\n"
"  {\n"
"    \"name\": \"emit_ui_command\",\n"
"    \"targets\": [\"export_document\", \"open_preview\", \"save_as\"],\n"
"    \"args\": {\n"
"      \"mode\": [\"markdown\", \"html\", \"pdf\", \"reveal\"],\n"
"      \"theme\": [\"plain\", \"dark\", \"scientific\", \"rpg\"]\n"
"    }\n"
"  },\n"
"  {\n"
"    \"name\": \"workflow\",\n"
"    \"targets\": [\"multi_action_sandbox\"],\n"
"    \"args\": {\n"
"      \"mode\": [\"markdown\"],\n"
"      \"theme\": [\"plain\"]\n"
"    }\n"
"  }\n"
"]\n";

static const char *policy_rules_json =
"[\n"
"  {\"kind\": \"requires_confirmation_if_risk_at_least\", \"risk\": \"medium\"},\n"
"  {\"kind\": \"forbidden_command\", \"command\": \"execute_external_command\"},\n"
"  {\"kind\": \"forbidden_target\", \"target\": \"delete_all_files\"},\n"
"  {\"kind\": \"forbidden_claim\", \"contains\": \"already executed\"}\n"
"]\n";

static const char *example_state_json =
"{\n"
"  \"app\": \"KALYX-KIE prompt-pack demo\",\n"
"  \"current_document\": \"README.md\",\n"
"  \"unsaved_changes\": false,\n"
"  \"available_exports\": [\"markdown\", \"html\", \"pdf\"],\n"
"  \"sandbox_only\": true\n"
"}\n";

static const char *expected_response_shape_md =
"# Expected KRESP01 Workflow Response\n\n"
"```json\n"
"{\n"
"  \"schema\": \"KRESP01\",\n"
"  \"type\": \"command\",\n"
"  \"human_summary\": \"Workflow wird in der Sandbox vorbereitet.\",\n"
"  \"risk\": \"low\",\n"
"  \"requires_confirmation\": false,\n"
"  \"uses_only_provided_context\": true,\n"
"  \"machine_result\": {\n"
"    \"command\": \"workflow\",\n"
"    \"target\": \"multi_action_sandbox\",\n"
"    \"args\": {\"mode\": \"markdown\", \"theme\": \"plain\"},\n"
"    \"workflow\": [\n"
"      {\"command\": \"emit_ui_command\", \"target\": \"export_document\", \"args\": {\"mode\": \"markdown\", \"theme\": \"plain\"}},\n"
"      {\"command\": \"emit_ui_command\", \"target\": \"open_preview\", \"args\": {\"mode\": \"markdown\", \"theme\": \"plain\"}},\n"
"      {\"command\": \"emit_ui_command\", \"target\": \"save_as\", \"args\": {\"mode\": \"markdown\", \"theme\": \"plain\"}}\n"
"    ]\n"
"  }\n"
"}\n"
"```\n";

static KalyxStatus write_pack_file(const char *pack, const char *name, const char *text) {
    char *path = join_path(pack, name);
    KalyxStatus st;
    if (!path) return KALYX_ERR_IO;
    st = kalyx_write_text_file(path, text);
    free(path);
    return st;
}

static int write_prompt_pack(const char *pack, const char *domain, const char *intent, const char *goal, const KalyxPromptProfile *profile) {
    char *prompt;
    char *contract;
    char *request;
    char *readme;
    size_t need;
    KalyxStatus st;
    if (ensure_dir(pack) != 0) return 1;
    prompt = make_kprompt(domain, intent, goal, profile);
    contract = make_contract(domain, intent, goal, profile);
    request = make_request(goal, profile);
    if (!prompt || !contract || !request) return 1;
    need = strlen(goal) + 1400u;
    readme = (char *)calloc(need, 1u);
    if (!readme) return 1;
    snprintf(readme, need,
        "# KALYX Prompt Pack\n\n"
        "Dieses Paket wurde von `kalyx_prompt_author` erzeugt.\n\n"
        "## Profile\n\n%s\n\n"
        "## Goal\n\n%s\n\n"
        "## Dateien\n\n"
        "- `prompt.kprompt.md`: menschenlesbares KPROMPT01 Prompt-Asset.\n"
        "- `contract.kcontract.json`: maschinenlesbarer KCONTRACT01 Governance-Vertrag.\n"
        "- `request.txt`: direkte LLM-Anfrage für den Envelope.\n"
        "- `allowed_actions.json`: erlaubte Host-Aktionen.\n"
        "- `policy_rules.json`: Sicherheitsregeln.\n"
        "- `example_state.json`: Beispielzustand.\n"
        "- `expected_response_shape.md`: erwartete KRESP01-Form.\n",
        profile ? profile->name : "custom", goal);
    st = write_pack_file(pack, "prompt.kprompt.md", prompt);
    if (st == KALYX_OK) st = write_pack_file(pack, "contract.kcontract.json", contract);
    if (st == KALYX_OK) st = write_pack_file(pack, "request.txt", request);
    if (st == KALYX_OK) st = write_pack_file(pack, "allowed_actions.json", allowed_actions_json);
    if (st == KALYX_OK) st = write_pack_file(pack, "policy_rules.json", policy_rules_json);
    if (st == KALYX_OK) st = write_pack_file(pack, "example_state.json", example_state_json);
    if (st == KALYX_OK) st = write_pack_file(pack, "expected_response_shape.md", expected_response_shape_md);
    if (st == KALYX_OK) st = write_pack_file(pack, "README.md", readme);
    free(prompt); free(contract); free(request); free(readme);
    return st == KALYX_OK ? 0 : 1;
}

int main(int argc, char **argv) {
    const char *domain = arg_value(argc, argv, "--domain");
    const char *intent = arg_value(argc, argv, "--intent");
    const char *goal = arg_value(argc, argv, "--goal");
    const char *profile_name = arg_value(argc, argv, "--profile");
    const char *profile_file = arg_value(argc, argv, "--profile-file");
    const char *profile_dir = arg_value(argc, argv, "--profile-dir");
    const char *out = arg_value(argc, argv, "--out");
    const char *pack = arg_value(argc, argv, "--pack");
    const KalyxPromptProfile *profile = NULL;
    KalyxPromptProfile custom_profile;
    char err[256];
    char *resolved_profile_path = NULL;
    char *prompt;
    KalyxStatus st;

    if (argc >= 2 && has_arg(argc, argv, "--help")) { usage(); return 0; }
    if (argc >= 2 && has_arg(argc, argv, "--version")) { puts(KALYX_VERSION); return 0; }
    if (argc >= 2 && has_arg(argc, argv, "--list-profiles")) { list_profiles_with_dir(profile_dir); return 0; }
    if (profile_file) {
        if (!load_profile_file(profile_file, &custom_profile, err, sizeof(err))) { fprintf(stderr, "invalid profile file: %s (%s)\n", profile_file, err); return 2; }
        profile = &custom_profile;
    } else if (profile_name) {
        profile = find_profile(profile_name);
        if (!profile && profile_dir) {
            resolved_profile_path = profile_path_from_dir(profile_dir, profile_name);
            if (resolved_profile_path && load_profile_file(resolved_profile_path, &custom_profile, err, sizeof(err))) profile = &custom_profile;
        }
        if (!profile) { fprintf(stderr, "unknown profile: %s\n", profile_name); list_profiles_with_dir(profile_dir); free(resolved_profile_path); return 2; }
        free(resolved_profile_path);
    }
    if (profile) {
        if (!domain) domain = profile->domain;
        if (!intent) intent = profile->intent;
        if (!goal) goal = profile->default_goal;
    }
    if (!domain || !intent || !goal || (!out && !pack)) { usage(); return 2; }

    if (out) {
        prompt = make_kprompt(domain, intent, goal, profile);
        if (!prompt) return 3;
        st = kalyx_write_text_file(out, prompt);
        free(prompt);
        if (st != KALYX_OK) { fprintf(stderr, "cannot write prompt: %s\n", out); return 3; }
        printf("kalyx_prompt_author: wrote %s\n", out);
    }
    if (pack) {
        if (write_prompt_pack(pack, domain, intent, goal, profile) != 0) { fprintf(stderr, "cannot write prompt pack: %s\n", pack); return 3; }
        printf("kalyx_prompt_author: wrote prompt pack %s\n", pack);
    }
    return 0;
}
