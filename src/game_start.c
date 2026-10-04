#include "scenario_runtime.h"

/* Patched Rise of Rome 1.0a world game start 0x0050aaa0, from 0x0050b7c6
   (after the victory/relic setup) to the end of its per-player loop. Each
   step is its own loop over players 1..count-1 (world +0x3c/+0x40). */

typedef struct {
    uint8_t kind; /* adder 0x0050ead0 + 0x40 * kind */
    uint8_t category, index;
    int32_t resource, field_08, weight_0c;
} StartEvent;

/* 0x0050bb8b..0x0050bd99 in call order; index 8 (kind 2, resource 22) is
   skipped when 0x0041b7c0 (+0x985) is set. */
static const StartEvent start_events[] = {
    {0, 0, 0, 20, 2, 1},    {0, 0, 1, 43, 1, 1},    {0, 0, 2, 44, 1, 1},
    {3, 0, 3, 40, 1, 25},   {0, 1, 4, 49, 100, 1},  {0, 1, 9, 53, 60, 1},
    {4, 1, 5, 37, 1, 1},    {3, 1, 6, 37, 1, 25},   {0, 1, 7, 22, 3, 1},
    {2, 1, 8, 22, 1, 25},   {0, 2, 10, 41, 1, 2},   {2, 2, 11, 41, 1, 25},
    {0, 2, 12, 14, 1, 10},  {0, 2, 13, 7, 1, 10},   {0, 2, 14, 52, 1, 3},
    {0, 2, 16, 55, 1, 50},  {0, 2, 15, 54, 1, 50},  {0, 3, 17, 21, 1, 2},
    {2, 3, 18, 21, 1, 50},  {1, 3, 19, 6, 3, 25},   {1, 3, 20, 6, 4, 25},
    {0, 4, 22, 45, 1, 100}, {0, 4, 23, 42, 1, 100},
};

/* Stores food (0), wood (1), stone (2) and gold (3) as the level loops do
   (+0x00, +0x04, +0x0c, +0x08). */
static void set_stocks(AoeScenarioRuntimePlayerRor *player, float food,
                       float wood, float stone, float gold) {
    float *r = player->resource_values;
    if (player->resources.count < 4) return;
    r[0] = food;
    r[1] = wood;
    r[3] = gold;
    r[2] = stone;
}

/* Player +0x70 (0x0045cfc0): sets one resource. */
static void set_resource(AoeScenarioRuntimePlayerRor *player, int type,
                         float value) {
    if (type < player->resources.count) player->resource_values[type] = value;
}

int aoe_scenario_runtime_start_game_ror(AoeScenarioRuntimeRor *runtime) {
    const AoeGameOptionsRor *o;
    int16_t count;
    int tech_trees;
    if (!runtime || !runtime->initialized) return 0;
    o = &runtime->options;
    count = runtime->player_count;
    if (!o->option_ad6)
        for (int16_t p = 1; p < count; ++p)
            if (!aoe_scenario_runtime_apply_effect_ror(runtime, p, 105))
                return 0;
    if (o->option_ad7 == 1)
        for (int16_t p = 1; p < count; ++p)
            if (!aoe_scenario_runtime_apply_effect_ror(runtime, p, 100))
                return 0;
    tech_trees = !o->option_ad9 && !runtime->scenario_options[2];
    for (int16_t p = 1; p < count; ++p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        if (!tech_trees) {
            if (!aoe_scenario_runtime_apply_effect_ror(runtime, p, 210))
                return 0;
            continue;
        }
        /* Player +0x110: the civ's tech tree effect (-1 does nothing). */
        if (!aoe_scenario_runtime_apply_effect_ror(runtime, p,
                                                   player->tech_tree_id))
            return 0;
        /* Player +0x109 civ byte 7: 0x0045d7a0(123, -1, 13, -0.15f). */
        if (player->civilization_id == 7 &&
            !aoe_scenario_runtime_attribute_ror(runtime, p, AOE_ATTRIBUTE_ADD,
                                                123, -1, 13, -0.15f))
            return 0;
        /* Civ 10: templates[83] +0x14a (first cost amount) = 40. */
        if (player->civilization_id == 10) {
            DatUnit *t = aoe_player_templates_get(&player->templates, 83);
            if (t) t->costs[0].amount = 40;
        }
    }
    for (int16_t p = 1; p < count; ++p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        switch (o->resource_level) {
        case 1: set_stocks(player, 200.0f, 200.0f, 100.0f, 0.0f); break;
        case 2: set_stocks(player, 500.0f, 500.0f, 250.0f, 0.0f); break;
        case 3: set_stocks(player, 1000.0f, 1000.0f, 750.0f, 0.0f); break;
        default: break;
        }
    }
    if (tech_trees)
        for (int16_t p = 1; p < count; ++p) {
            AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
            if (player->civilization_id == 10 && player->resources.count > 0)
                player->resource_values[0] -= 40.0f; /* 0x0054a608 */
        }
    if (o->option_ae5)
        for (int16_t p = 1; p < count; ++p)
            set_stocks(&runtime->players[p], 20000.0f, 20000.0f, 5000.0f,
                       10000.0f);
    if (o->starting_age >= 3 && o->starting_age <= 5) {
        static const int steps[3] = {25, 23, 24};
        for (int16_t p = 1; p < count; ++p) {
            AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
            if (!aoe_research_advance_age_ror(
                    &player->research, runtime->techs, &player->resources,
                    steps[o->starting_age - 3]))
                return 0;
        }
    }
    for (int16_t p = 1; p < count; ++p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        set_resource(player, 32, (float)o->population_limit);
        set_resource(player, 33, 0.0f);
        set_resource(player, 30, 500.0f);
        set_resource(player, 45, 1.0f);
        for (size_t i = 0; i < sizeof(start_events) / sizeof(*start_events);
             ++i) {
            const StartEvent *e = &start_events[i];
            if (e->index == 8 && o->option_985) continue;
            if (!aoe_outcome_event_add_ror(&player->outcome, e->kind,
                                           e->category, e->index,
                                           e->resource, e->field_08,
                                           e->weight_0c))
                return 0;
        }
        ++runtime->tracker_updates;
    }
    return 1;
}
