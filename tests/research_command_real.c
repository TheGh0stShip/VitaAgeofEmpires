#include "command_production.h"
#include "dat.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "world_combat.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Production commands on a real Rise of Rome scenario: command 0x65
   (0x004e8780) starts a research action (0x004b60b0) on player 1's town
   centre, which pays through the manager (0x004eb100), progresses with
   tick x work rate (0x004b6290 -> 0x004eab23) and completes the tech
   (0x004eaa3d). A second command while it runs takes the player
   fallback. Command 0x64 (0x004e85e0) then trains a villager through a
   non-prepaid train action (0x004b4f10). */
static AoeLiveUnit *town_centre(AoeScenarioRuntimeRor *r) {
    AoeScenarioRuntimePlayerRor *p = &r->players[1];
    for (uint32_t i = 0; i < p->unit_count; ++i) {
        AoeLiveUnit *u = &p->units[i];
        if (u->constructed && u->core.state == 2 &&
            u->core.unit_template->id == 109)
            return u;
    }
    return NULL;
}
static int16_t head_type(const AoeLiveUnit *u) {
    const AoeActionList *l = u->derived_action_list;
    int16_t t;
    if (!l || !l->head || !l->head->action) return 0;
    memcpy(&t, l->head->action, sizeof(t));
    return t;
}
static uint32_t villagers(AoeScenarioRuntimeRor *r) {
    AoeScenarioRuntimePlayerRor *p = &r->players[1];
    uint32_t n = 0;
    for (uint32_t i = 0; i < p->unit_count; ++i)
        if (p->units[i].constructed && p->units[i].core.state == 2 &&
            p->units[i].core.unit_template->id == 83)
            ++n;
    for (size_t i = 0; i < r->editor_unit_count; ++i) {
        AoeLiveUnit *u = r->editor_units[i];
        if (u && u->constructed && u->runtime_owner_index == 1 &&
            u->core.state == 2 && u->core.unit_template->id == 83)
            ++n;
    }
    return n;
}

int main(int argc, char **argv) {
    static DatBlob blob;
    static DatIntro intro;
    static DatSounds sounds;
    static DatGraphics graphics;
    static DatTerrains terrains;
    static DatBorders borders;
    static DatMapGeometry geometry;
    static DatRandomMaps random_maps;
    static DatEffects effects;
    static DatTechs techs;
    static DatCivilizations civs;
    static ScnMap scenario;
    static AoeScenarioRuntimeRor runtime;
    static AoeWorldMotionRor motion;
    static AoeWorldCombatRor combat;
    AoeScenarioRuntimePlayerRor *p;
    AoeLiveUnit *tc;
    uint8_t cmd[AOE_COMMAND_PRODUCTION_SIZE_ROR];
    int16_t tech = -1;
    float before[4];
    int ticks, limit;
    uint32_t v0;
    if (argc != 3) return 2;
    if (!dat_load(argv[2], &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !dat_parse_techs(&blob, &civs, &techs) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario))
        return 1;
    assert(aoe_scenario_runtime_build_ror(&runtime, &scenario, &intro, &civs,
                                          &graphics, &terrains, &borders,
                                          &geometry, &techs, &effects, NULL,
                                          1));
    assert(aoe_scenario_runtime_start_game_ror(&runtime));
    assert(aoe_world_motion_init_ror(&motion, &runtime));
    assert(aoe_world_combat_init_ror(&combat, &motion));
    p = &runtime.players[1];
    tc = town_centre(&runtime);
    assert(tc && tc->core.unit_template->work_rate > 0.0f);
    /* The first available tech researched at a town centre. */
    for (int16_t i = 0; i < p->research.count && tech < 0; ++i)
        if (p->research.entries[i].state == 1 &&
            techs.techs[i].research_location == 109 &&
            techs.techs[i].research_time > 0)
            tech = i;
    assert(tech >= 0);
    for (int i = 0; i < 4; ++i) {
        p->resources.values[i] = 5000.0f;
        before[i] = 5000.0f;
    }
    if (p->resources.count > 29) p->resources.values[29] = 0.0f;

    assert(aoe_command_encode_production_ror(cmd, sizeof(cmd),
                                             AOE_COMMAND_RESEARCH_ROR,
                                             tc->core.unit_id, 1, tech, 9));
    assert(aoe_world_combat_command_production_ror(&combat, cmd,
                                                   sizeof(cmd)) == 1);
    assert(head_type(tc) == 0x67);
    assert(p->research.entries[tech].state == 2);
    for (int i = 0; i < 3; ++i) {
        const DatTechCost *c = &techs.techs[tech].costs[i];
        if (c->resource_type >= 0 && c->resource_type < 4 && c->paid)
            before[c->resource_type] -= (float)c->amount;
    }
    for (int i = 0; i < 4; ++i) assert(p->resources.values[i] == before[i]);
    /* Busy: the player fallback answers. */
    assert(aoe_world_combat_command_production_ror(&combat, cmd,
                                                   sizeof(cmd)) == 1);
    assert(combat.production_fallbacks == 1);

    limit = (int)((float)techs.techs[tech].research_time /
                  (0.05f * tc->core.unit_template->work_rate)) + 4;
    for (ticks = 0; ticks < limit && !combat.researched; ++ticks)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    assert(combat.researched == 1 && combat.research_failures == 0);
    assert(p->research.entries[tech].state == 3);
    printf("tech %d (%s) researched in %d updates of %d s\n", tech,
           techs.techs[tech].name ? techs.techs[tech].name : "?", ticks,
           techs.techs[tech].research_time);
    for (int i = 0; i < 2; ++i)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    assert(head_type(tc) != 0x67);

    /* Command 0x64: a villager from the same town centre. */
    v0 = villagers(&runtime);
    assert(aoe_command_encode_production_ror(cmd, sizeof(cmd),
                                             AOE_COMMAND_TRAIN_ROR,
                                             tc->core.unit_id, 1, 83, -1));
    p->resources.values[0] = 5000.0f;
    assert(aoe_world_combat_command_production_ror(&combat, cmd,
                                                   sizeof(cmd)) == 1);
    assert(head_type(tc) == 0x66);
    assert(p->resources.values[0] < 5000.0f); /* charged on activation */
    for (ticks = 0; ticks < 2000 && villagers(&runtime) == v0; ++ticks)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    assert(villagers(&runtime) == v0 + 1);
    printf("villager trained in %d updates\n", ticks);
    return 0;
}
