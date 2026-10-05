#include "scenario_object.h"

#include <stdio.h>
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
