#include "scenario_object.h"

#include "effects_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *text_buffer(AoeScenarioObjectRor *s, int which) {
    switch (which) {
    case 0: return s->text_1508;
    case 1: return s->text_15e8;
    case 2: return s->text_16c8;
    default: return s->text_17a8;
    }
}

void aoe_scenario_set_description_ror(AoeScenarioObjectRor *s, const char *text) {
    if (!text) {
        s->description[0] = '\0';
        return;
    }
    strcpy(s->description, text);
}

void aoe_scenario_set_text_ror(AoeScenarioObjectRor *s, int which, const char *text) {
    char *d = text_buffer(s, which);
    if (!text) {
        d[0] = '\0';
        return;
    }
    strcpy(d, text);
}

void aoe_scenario_set_player_name_ror(AoeScenarioObjectRor *s, int32_t player,
                                      const char *name) {
    strcpy(s->player_name[player], name);
}

void aoe_scenario_set_file_name_ror(char *dest, const char *name, int strip) {
    if (!name) {
        dest[0] = '\0';
    } else {
        strcpy(dest, name);
    }
    if (strip == 1) dest[strlen(name) - 4] = '\0';
}

void aoe_scenario_set_value_ror(int32_t table[AOE_SCENARIO_PLAYERS_ROR], int32_t player,
                                int32_t value) {
    if (player > -1 && player < 0x10) table[player] = value;
}

int32_t aoe_scenario_value_ror(const int32_t table[AOE_SCENARIO_PLAYERS_ROR], int32_t player) {
    return player > -1 && player < 0x10 ? table[player] : 0;
}

const char *aoe_scenario_text_ror(const AoeScenarioObjectRor *s, int which) {
    return text_buffer((AoeScenarioObjectRor *)s, which);
}

const char *aoe_scenario_player_name_ror(const AoeScenarioObjectRor *s, int32_t player) {
    return s->player_name[player];
}

int32_t aoe_scenario_player_kind_ror(const AoeScenarioObjectRor *s, int32_t player) {
    if (player > -1 && player < 9) {
        int32_t k = s->player_kind[player];
        if (k >= 1 && k <= 5) return k;
    }
    return 0;
}

float aoe_scenario_version_ror(void) { return 1.15f; }

const char *aoe_scenario_player_file_ror(AoeScenarioObjectRor *s, int kind, int32_t player,
                                         int use_files, const AoeScenarioFileOpsRor *ops) {
    static const char *const temp_fmt[3] = {"%stemp_buildlist%d.tmp", "%stemp_CityPlan%d.tmp",
                                            "%stemp_AiRules%d.tmp"};
    static const char *const name_fmt[3] = {"%s%s.ai", "%s%s.cty", "%s%s.per"};
    char *const *names = kind == AOE_SCENARIO_FILE_AI     ? s->ai_name
                         : kind == AOE_SCENARIO_FILE_CITY ? s->city_name
                                                          : s->per_name;
    const void *const *data = kind == AOE_SCENARIO_FILE_AI     ? s->ai_data
                              : kind == AOE_SCENARIO_FILE_CITY ? s->city_data
                                                               : s->per_data;
    const int32_t *size = kind == AOE_SCENARIO_FILE_AI     ? s->ai_size
                          : kind == AOE_SCENARIO_FILE_CITY ? s->city_size
                                                           : s->per_size;
    char *out = kind == AOE_SCENARIO_FILE_AI     ? s->ai_path
                : kind == AOE_SCENARIO_FILE_CITY ? s->city_path
                                                 : s->per_path;
    char random[50], none[50], temp[0x104];
    if (ops->game_state == 7) return names[player];
    ops->string(ops->context, 0x277b, random, sizeof random);
    ops->string(ops->context, 0x2775, none, sizeof none);
    if (!ops->temp_path(ops->context, temp, sizeof temp)) strcpy(temp, ".");
    if (use_files == 1) {
        if (size[player] > 0 && data[player]) {
            snprintf(out, 0xe0, temp_fmt[kind], temp, player);
            if (ops->write_file(ops->context, out, data[player], size[player])) return out;
        }
        if (strcmp(s->ai_name[player], none) != 0 && strcmp(s->ai_name[player], random) != 0) {
            snprintf(out, 0xe0, name_fmt[kind], ops->ai_dir, names[player]);
            return out;
        }
    }
    snprintf(out, 0xe0, "%s", names[player]);
    return out;
}

void aoe_scenario_record_init_ror(AoeScenarioRecordRor *r, const char *text, int32_t now) {
    r->error = 0;
    r->version = 2;
    r->stamp = now;
    r->text = NULL;
    if (text) {
        size_t n = strlen(text) + 1;
        r->text = malloc(n);
        if (r->text) {
            memcpy(r->text, text, n);
            r->text[n - 1] = '\0';
        }
    }
}

void aoe_scenario_record_read_ror(AoeScenarioRecordRor *r, AoeOutcomeStream *in) {
    int32_t n;
    r->error = 0;
    r->version = 0;
    r->stamp = 0;
    r->text = NULL;
    (void)aoe_stream_i32(in); /* the record size */
    r->version = aoe_stream_i32(in);
    if (r->version > 1) r->stamp = aoe_stream_i32(in);
    n = aoe_stream_i32(in);
    if (n > 0) {
        const uint8_t *p;
        r->text = malloc((size_t)n);
        if (!r->text) {
            r->error = 1;
            return;
        }
        p = aoe_stream_take(in, (size_t)n);
        if (p) memcpy(r->text, p, (size_t)n);
    }
}

void aoe_scenario_record_free_ror(AoeScenarioRecordRor *r) {
    free(r->text);
    r->text = NULL;
}

int32_t aoe_scenario_record_size_ror(const AoeScenarioRecordRor *r) {
    return r->text ? (int32_t)strlen(r->text) + 1 + 0xc : 0xc;
}

void aoe_scenario_record_write_ror(const AoeScenarioRecordRor *r, AoeOutcomeStream *out) {
    int32_t n = r->text ? (int32_t)strlen(r->text) + 1 : 0;
    aoe_stream_put_i32(out, aoe_scenario_record_size_ror(r));
    aoe_stream_put_i32(out, r->version);
    if (r->version > 1) aoe_stream_put_i32(out, r->stamp);
    aoe_stream_put_i32(out, n);
    if (r->text) aoe_stream_put(out, r->text, (size_t)n);
}

/* ---- defaults, messages, counts ---- */

static char *fresh(const char *text) {
    char *p = calloc(0xe0, 1);
    if (p && text) {
        strncpy(p, text, 0xdf);
        p[0xdf] = '\0';
    }
    return p;
}

int aoe_scenario_reset_ror(AoeScenarioObjectRor *s,
                           void (*string)(void *context, int32_t id, char *out, int size),
                           void *context) {
    char none[0x1e], random[0x1e];
    int i;
    none[0] = random[0] = '\0';
    string(context, 0x2775, none, (int)sizeof none);
    string(context, 0x277b, random, (int)sizeof random);
    for (i = 0; i < 5; ++i) s->message[i] = NULL;
    s->description = fresh(NULL);
    snprintf(s->text_1508, sizeof s->text_1508, "%s", none);
    snprintf(s->text_15e8, sizeof s->text_15e8, "%s", none);
    snprintf(s->text_16c8, sizeof s->text_16c8, "%s", none);
    snprintf(s->text_17a8, sizeof s->text_17a8, "%s", none);
    for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i) {
        s->player_kind[i] = 0;
        s->value_18cc[i] = 0;
        s->value_190c[i] = 0;
        s->value_194c[i] = 0;
        s->player_name[i][0] = '\0';
        s->ai_name[i] = fresh(random);
        s->city_name[i] = fresh(none);
        s->per_name[i] = fresh(random);
        s->ai_data[i] = s->city_data[i] = s->per_data[i] = NULL;
        s->ai_size[i] = s->city_size[i] = s->per_size[i] = 0;
    }
    return s->description != NULL;
}

void aoe_scenario_release_ror(AoeScenarioObjectRor *s) {
    int i;
    for (i = 0; i < 5; ++i) {
        free(s->message[i]);
        s->message[i] = NULL;
    }
    free(s->description);
    s->description = NULL;
    for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i) {
        free(s->ai_name[i]);
        free(s->city_name[i]);
        free(s->per_name[i]);
        s->ai_name[i] = s->city_name[i] = s->per_name[i] = NULL;
        free((void *)s->ai_data[i]);
        free((void *)s->city_data[i]);
        free((void *)s->per_data[i]);
        s->ai_data[i] = s->city_data[i] = s->per_data[i] = NULL;
    }
}

void aoe_scenario_set_message_ror(AoeScenarioObjectRor *s, const char *text, int32_t which) {
    char *copy = NULL;
    if (text) {
        size_t n = strlen(text) + 1;
        copy = calloc(n, 1);
        if (copy) memcpy(copy, text, n);
    }
    if (which < 0 || which > 4) {
        free(copy);
        return;
    }
    /* Message 2's old text is not freed in the original (it frees only
       when there was none); the port frees it to avoid the leak, with the
       same visible result. */
    free(s->message[which]);
    s->message[which] = copy;
}

const char *aoe_scenario_message_ror(const AoeScenarioObjectRor *s, int32_t which) {
    return (which >= 0 && which <= 4) ? s->message[which] : NULL;
}

void aoe_scenario_append_message_ror(AoeScenarioObjectRor *s, const char *text) {
    if (!s->message[0]) {
        aoe_text_set_ror(&s->message[0], text);
        return;
    }
    aoe_text_set_joined_ror(&s->message[0], s->message[0], text);
}

uint8_t aoe_scenario_field_c_ror(const AoeScenarioObjectRor *s) { return s->field_c; }

int32_t aoe_scenario_active_players_ror(const AoeScenarioObjectRor *s) {
    int32_t i, n = 0;
    for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i)
        if (aoe_scenario_value_ror(s->value_194c, i)) ++n;
    return n;
}

int32_t aoe_scenario_human_players_ror(const AoeScenarioObjectRor *s) {
    int32_t i, n = 0;
    for (i = 0; i < AOE_SCENARIO_PLAYERS_ROR; ++i)
        if (aoe_scenario_value_ror(s->value_194c, i) && aoe_scenario_value_ror(s->value_18cc, i) == 1)
            ++n;
    return n;
}

/* ---- scenario index ---- */

int aoe_scenario_index_load_ror(AoeScenarioIndexRor *x, const char *path,
                                const AoeScenarioIndexOpsRor *ops) {
    AoeOutcomeStream *s;
    int32_t i;
    snprintf(x->path, sizeof x->path, "%s", path);
    x->count = 0;
    x->entries = NULL;
    s = ops->open_read(ops->context, x->path);
    if (s) {
        x->count = aoe_stream_i32(s);
        if (x->count > 0) {
            x->entries = calloc((size_t)x->count, sizeof *x->entries);
            if (!x->entries) {
                x->count = 0;
            } else {
                for (i = 0; i < x->count; ++i) {
                    const uint8_t *p;
                    x->entries[i].id = aoe_stream_i32(s);
                    p = aoe_stream_take(s, 0x104);
                    if (p) memcpy(x->entries[i].name, p, 0x104);
                    x->entries[i].name[0x103] = '\0';
                    x->entries[i].record = calloc(1, sizeof *x->entries[i].record);
                    if (x->entries[i].record) aoe_scenario_record_read_ror(x->entries[i].record, s);
                }
            }
        } else {
            x->count = 0;
        }
        ops->close_read(ops->context, s);
    }
    if (ops->refresh) ops->refresh(ops->context, x);
    return s != NULL;
}

void aoe_scenario_index_save_ror(const AoeScenarioIndexRor *x, const AoeScenarioIndexOpsRor *ops) {
    AoeOutcomeStream *s = ops->open_write(ops->context, x->path);
    int32_t i;
    if (!s) return;
    aoe_stream_put_i32(s, x->count);
    for (i = 0; i < x->count; ++i) {
        aoe_stream_put_i32(s, x->entries[i].id);
        aoe_stream_put(s, x->entries[i].name, 0x104);
        if (x->entries[i].record) aoe_scenario_record_write_ror(x->entries[i].record, s);
    }
    ops->close_write(ops->context, s);
}

void aoe_scenario_index_close_ror(AoeScenarioIndexRor *x, const AoeScenarioIndexOpsRor *ops) {
    int32_t i;
    aoe_scenario_index_save_ror(x, ops);
    if (x->count <= 0) return;
    for (i = 0; i < x->count; ++i) {
        if (x->entries[i].record) {
            aoe_scenario_record_free_ror(x->entries[i].record);
            free(x->entries[i].record);
        }
    }
    free(x->entries);
    x->entries = NULL;
    x->count = 0;
}

const char *aoe_scenario_index_name_ror(const AoeScenarioIndexRor *x, int32_t i) {
    return (i < x->count && i >= 0) ? x->entries[i].name : NULL;
}

AoeScenarioRecordRor *aoe_scenario_index_record_ror(const AoeScenarioIndexRor *x, int32_t i) {
    return (i < x->count && i >= 0) ? x->entries[i].record : NULL;
}
