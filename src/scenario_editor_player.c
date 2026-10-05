/* Patched RoR 1.0a scenario editor player page 0x00492780. */
#include "scenario_editor_player.h"

#include <stdio.h>
#include <stdlib.h>

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
