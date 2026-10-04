#include "dat.h"
#include "scenario_runtime.h"
#include "scn.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    DatBlob blob;
    DatIntro intro;
    DatSounds sounds;
    DatGraphics graphics;
    DatTerrains terrains;
    DatBorders borders;
    DatMapGeometry geometry;
    DatRandomMaps random_maps;
    DatEffects effects;
    DatTechs techs;
    DatCivilizations civs;
    ScnMap scenario;
} Inputs;

static int build(AoeScenarioRuntimeRor *runtime, Inputs *in,
                 const AoeGameOptionsRor *options) {
    memset(runtime, 0, sizeof(*runtime));
    return aoe_scenario_runtime_build_ror(
        runtime, &in->scenario, &in->intro, &in->civs, &in->graphics,
        &in->terrains, &in->borders, &in->geometry, &in->techs, &in->effects,
        options, 1);
}

static int16_t tech_state(const AoeScenarioRuntimePlayerRor *p, float id) {
    return p->research.entries[(int)id].state;
}

/* Scenario player load (0x004f02d0/0x00507e50) and game start (0x0050aaa0
   from 0x0050b7c6) on EXPANS_1_1 with the patched RoR 1.0a rules. */
int main(int argc, char **argv) {
    static Inputs in;
    static AoeScenarioRuntimeRor runtime;
    AoeGameOptionsRor options = aoe_game_options_default_ror();
    const AoeScenarioRuntimePlayerRor *p1;
    uint32_t enabled = 0;
    if (argc != 3) return 2;
    if (!dat_load(argv[2], &in.blob) || !dat_parse_intro(&in.blob, &in.intro) ||
        !dat_parse_sounds(&in.blob, &in.intro, &in.sounds) ||
        !dat_parse_graphics(&in.blob, &in.sounds, &in.graphics) ||
        !dat_parse_terrains(&in.blob, &in.intro, &in.graphics, &in.terrains) ||
        !dat_parse_borders(&in.blob, &in.terrains, &in.borders) ||
        !dat_parse_map_geometry(&in.blob, &in.terrains, &in.borders,
                                &in.geometry) ||
        !dat_parse_random_maps(&in.blob, &in.geometry, &in.random_maps) ||
        !dat_parse_effects(&in.blob, &in.random_maps, &in.effects) ||
        !dat_parse_civilizations(&in.blob, &in.effects, &in.civs) ||
        !dat_parse_techs(&in.blob, &in.civs, &in.techs) ||
        !scn_load_map(argv[1], (uint8_t)in.intro.terrain_count, &in.scenario))
        return 1;

    assert(build(&runtime, &in, NULL));
    p1 = &runtime.players[1];
    assert(runtime.player_count == in.scenario.player_count + 1);
    assert(!runtime.players[0].research.entries);
    assert(p1->research.entries);
    /* Scenario stocks replace the civ defaults: wood 600 for player 1. */
    assert(p1->resource_values[0] == in.scenario.resources[0].food);
    assert(p1->resource_values[1] == 600.0f);
    assert(p1->resource_values[2] == in.scenario.resources[0].stone);
    assert(p1->resource_values[3] == in.scenario.resources[0].gold);
    for (uint16_t i = 0; i < p1->templates.count; ++i) {
        const DatUnit *t = aoe_player_templates_get(
            (AoePlayerTemplates *)&p1->templates, i);
        enabled += t && t->enabled;
    }
    printf("player 1: civ %u tree %d, food %.0f wood %.0f stone %.0f gold "
           "%.0f, %u templates enabled of %u, age %d, tool %d bronze %d\n",
           p1->civilization_id, p1->tech_tree_id, p1->resource_values[0],
           p1->resource_values[1], p1->resource_values[2],
           p1->resource_values[3], enabled, p1->templates.count,
           in.scenario.starting_age[0],
           tech_state(p1, p1->resource_values[25]),
           tech_state(p1, p1->resource_values[23]));
    /* Templates start disabled; completed techs re-enable some. */
    assert(enabled > 0 && enabled < p1->templates.count);

    assert(aoe_scenario_runtime_start_game_ror(&runtime));
    assert(p1->resource_values[32] == 50.0f);
    assert(p1->resource_values[33] == 0.0f);
    assert(p1->resource_values[30] == 500.0f);
    assert(p1->resource_values[45] == 1.0f);
    assert(p1->outcome.event_count == 23);
    assert(p1->outcome.events->index_02 == 23 &&
           p1->outcome.events->resource_index_04 == 42 &&
           p1->outcome.events->weight_0c == 100);
    assert(runtime.tracker_updates == (uint32_t)runtime.player_count - 1);
    aoe_scenario_runtime_destroy_ror(&runtime);

    /* Deathmatch stocks, Bronze start and option +0x985 (no event 8). */
    options.option_ae5 = 1;
    options.starting_age = 4;
    options.option_985 = 1;
    assert(build(&runtime, &in, &options));
    assert(aoe_scenario_runtime_start_game_ror(&runtime));
    p1 = &runtime.players[1];
    assert(p1->resource_values[0] == 20000.0f &&
           p1->resource_values[1] == 20000.0f &&
           p1->resource_values[2] == 5000.0f &&
           p1->resource_values[3] == 10000.0f);
    assert(tech_state(p1, p1->resource_values[25]) == 3);
    assert(tech_state(p1, p1->resource_values[23]) == 3);
    assert(p1->outcome.event_count == 22);
    for (const AoeOutcomeEvent *e = p1->outcome.events; e; e = e->next)
        assert(e->index_02 != 8);
    aoe_scenario_runtime_destroy_ror(&runtime);

    /* Resource level 2 then civ 10's 40 food deduction. */
    options = aoe_game_options_default_ror();
    options.resource_level = 2;
    assert(build(&runtime, &in, &options));
    assert(aoe_scenario_runtime_start_game_ror(&runtime));
    for (int16_t p = 1; p < runtime.player_count; ++p) {
        const AoeScenarioRuntimePlayerRor *pl = &runtime.players[p];
        assert(pl->resource_values[0] ==
               (pl->civilization_id == 10 ? 460.0f : 500.0f));
        assert(pl->resource_values[1] == 500.0f &&
               pl->resource_values[2] == 250.0f &&
               pl->resource_values[3] == 0.0f);
    }
    aoe_scenario_runtime_destroy_ror(&runtime);
    printf("game start: 23 events, population 50, deathmatch and level 2 "
           "stocks\n");
    return 0;
}
