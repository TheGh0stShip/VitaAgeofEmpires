/* Patched RoR 1.0a scenario editor player page 0x00492780. */
#include "scenario_editor_player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

/* Info fields of food, wood, stone and gold. */
static const int info_field[4] = {2, 1, 3, 0};

static char **file_name(AoeScenarioObjectRor *o, int kind, int32_t slot) {
    return kind == AOE_SCENARIO_FILE_AI     ? &o->ai_name[slot]
           : kind == AOE_SCENARIO_FILE_CITY ? &o->city_name[slot]
                                            : &o->per_name[slot];
}

void aoe_editor_store_player_ror(AoeScenarioInfoRor *info,
                                 AoeScenarioObjectRor *obj, int32_t slot,
                                 const AoeEditorPlayerFormRor *f,
                                 float world_resources[4]) {
    const char *files[3];
    if (!info || !obj || !f || slot < 0 || slot >= AOE_SCENARIO_PLAYERS_ROR)
        return;
    for (int i = 0; i < 4; i++) {
        int32_t v = f->resource[i] ? atoi(f->resource[i]) : 0;
        aoe_scenario_set_start_resource_ror(info, slot, info_field[i], v);
        if (world_resources) world_resources[i] = (float)v;
    }
    aoe_scenario_set_player_name_ror(obj, slot, f->name ? f->name : "");
    aoe_scenario_set_value_ror(obj->value_18cc, slot, f->kind_line);
    aoe_scenario_set_value_ror(obj->value_190c, slot, f->value_line + 1);
    files[AOE_SCENARIO_FILE_AI] = f->ai;
    files[AOE_SCENARIO_FILE_CITY] = f->city;
    files[AOE_SCENARIO_FILE_PER] = f->per;
    for (int k = 0; k < 3; k++) {
        char *dest = *file_name(obj, k, slot);
        if (dest) aoe_scenario_set_file_name_ror(dest, files[k], 0);
    }
}

int aoe_editor_show_player_ror(const AoeScenarioInfoRor *info,
                               AoeScenarioObjectRor *obj, int32_t slot,
                               const AoeScenarioFileOpsRor *file_ops,
                               AoeEditorPlayerShownRor *out) {
    if (!info || !obj || !out || !file_ops || slot < 0 ||
        slot >= AOE_SCENARIO_PLAYERS_ROR)
        return 0;
    for (int i = 0; i < 4; i++)
        snprintf(out->resource[i], sizeof out->resource[i], "%d",
                 (int)aoe_scenario_start_resource_ror(info, slot,
                                                      info_field[i]));
    out->name = aoe_scenario_player_name_ror(obj, slot);
    out->kind_line = aoe_scenario_value_ror(obj->value_18cc, slot);
    out->value_line = aoe_scenario_value_ror(obj->value_190c, slot) - 1;
    out->per = aoe_scenario_player_file_ror(obj, AOE_SCENARIO_FILE_PER,
                                            slot, 0, file_ops);
    out->ai = aoe_scenario_player_file_ror(obj, AOE_SCENARIO_FILE_AI, slot,
                                           0, file_ops);
    out->city = aoe_scenario_player_file_ror(obj, AOE_SCENARIO_FILE_CITY,
                                             slot, 0, file_ops);
    return 1;
}

int32_t aoe_editor_set_player_count_ror(AoeScenarioObjectRor *obj,
                                        int32_t world_players,
                                        int32_t count_line,
                                        int16_t *current_player,
                                        int *switched) {
    int32_t n = count_line + 1;
    if (switched) *switched = 0;
    if (obj)
        for (int32_t i = 0; i < world_players - 1; i++)
            aoe_scenario_set_value_ror(obj->value_194c, i, i < n);
    if (current_player && n <= *current_player) {
        *current_player = (int16_t)n;
        if (switched) *switched = 1;
    }
    return n;
}

int aoe_scenario_has_conditions_ror(AoeScenarioInfoRor *info,
                                    const AoeScenarioObjectRor *obj) {
    if (!info || !obj) return 0;
    for (int32_t s = 0; s < AOE_SCENARIO_PLAYERS_ROR; s++) {
        if (!aoe_scenario_value_ror(obj->value_194c, s)) continue;
        aoe_scenario_select_player_ror(info, s);
        for (int32_t i = 0; i < AOE_SCENARIO_CONDITIONS; i++)
            if (info->conditions[s][i].type) return 1;
    }
    return 0;
}

void aoe_scenario_player_defaults_ror(AoeScenarioInfoRor *info,
                                      AoeScenarioObjectRor *obj) {
    static const int32_t start[4] = {0, 200, 200, 150};
    for (int32_t s = 0; s < AOE_SCENARIO_PLAYERS_ROR; s++) {
        for (int f = 0; f < 4; f++) info->start_resources[s][f] = start[f];
        obj->value_194c[s] = 0;
        obj->value_18cc[s] = 0;
        obj->value_190c[s] = s + 1;
        obj->player_kind[s] = 4;
        info->allied_victory[s] = 0;
        info->starting_age[s] = 0;
    }
    obj->value_18cc[0] = 1;
}

void aoe_scenario_record_ex_init_ror(AoeScenarioRecordExRor *r, const char *text, int32_t now,
                                     AoeScenarioInfoRor *info, const AoeScenarioObjectRor *obj) {
    aoe_scenario_record_init_ror(&r->base, text, now);
    r->has_conditions = aoe_scenario_has_conditions_ror(info, obj);
    r->players = obj ? aoe_scenario_active_players_ror(obj) : 0;
}

void aoe_scenario_record_ex_read_ror(AoeScenarioRecordExRor *r, AoeOutcomeStream *in) {
    aoe_scenario_record_read_ror(&r->base, in);
    r->has_conditions = 0;
    r->players = 0;
    if (!r->base.error) {
        r->has_conditions = aoe_stream_i32(in);
        r->players = aoe_stream_i32(in);
    }
}

int32_t aoe_scenario_record_ex_size_ror(const AoeScenarioRecordExRor *r) {
    return aoe_scenario_record_size_ror(&r->base) + 8;
}

void aoe_scenario_record_ex_write_ror(const AoeScenarioRecordExRor *r, AoeOutcomeStream *out) {
    int32_t n = r->base.text ? (int32_t)strlen(r->base.text) + 1 : 0;
    aoe_stream_put_i32(out, aoe_scenario_record_ex_size_ror(r));
    aoe_stream_put_i32(out, r->base.version);
    if (r->base.version > 1) aoe_stream_put_i32(out, r->base.stamp);
    aoe_stream_put_i32(out, n);
    if (r->base.text) aoe_stream_put(out, r->base.text, (size_t)n);
    aoe_stream_put_i32(out, r->has_conditions);
    aoe_stream_put_i32(out, r->players);
}
