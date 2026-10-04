#include "dat.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "world_combat.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Repair on a real Rise of Rome scenario: a player-1 villager ordered
   onto its own damaged building takes the repair task (template 156 by
   the task swap of the order step 0x00405f20), builds the repair action
   (0x004b55c0), walks, faces and repairs it to full hit points through
   the building +0x8c (0x004acb70), which charges the owner
   (0x004f1750). A priest (template 125) created for player 1 and ordered
   onto a wounded friendly unit takes the heal task (0x004b2c00) and heals
   it to full through the same +0x8c method (free for land units). */
static int16_t head_type(const AoeLiveUnit *u) {
    const AoeActionList *l = u->derived_action_list;
    int16_t t;
    if (!l || !l->head || !l->head->action) return 0;
    memcpy(&t, l->head->action, sizeof(t));
    return t;
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
    AoeLiveUnit *villager = NULL, *building = NULL;
    int16_t code = -1;
    int ticks, max;
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
    for (uint32_t i = 0; i < p->unit_count; ++i) {
        AoeLiveUnit *u = &p->units[i];
        if (!u->constructed || u->core.state != 2) continue;
        if (!villager && u->core.unit_template->id == 83) villager = u;
        if (!building && u->core.unit_template->type == 80 &&
            u->core.unit_template->id != 109 &&
            u->core.unit_template->train_time > 0)
            building = u;
    }
    assert(villager && building);
    for (int i = 0; i < 4; ++i) p->resources.values[i] = 1000.0f;
    max = building->core.unit_template->hit_points;
    building->core.hit_points = (float)max / 2.0f;
    assert(aoe_world_combat_order_ror(&combat, villager, building,
                                      building->core.position.x38,
                                      building->core.position.y3c, 0.0f,
                                      &code));
    printf("order code %d, template %d, head %d\n", code,
           villager->core.unit_template->id, head_type(villager));
    assert(head_type(villager) == 0x6a);
    assert(villager->core.unit_template->id == 156);
    for (ticks = 0; ticks < 4000 && building->core.hit_points < (float)max;
         ++ticks)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    printf("%s repaired to %g/%d in %d updates; wood %g stone %g\n",
           building->core.unit_template->name
               ? building->core.unit_template->name : "?",
           building->core.hit_points, max, ticks, p->resources.values[0 + 1],
           p->resources.values[2]);
    assert(building->core.hit_points == (float)max);
    assert(combat.repairs >= 1 && combat.repair_failures == 0);
    assert(p->resources.values[1] < 1000.0f ||
           p->resources.values[2] < 1000.0f);

    /* Heal: a new priest next to the villager. */
    {
        AoeLiveUnit *priest = NULL, *patient = villager;
        int hmax;
        assert(aoe_scenario_runtime_editor_create_ror(
            &runtime, 1, 125, villager->core.position.x38 + 1.5f,
            villager->core.position.y3c, villager->core.position.z40,
            &priest) && priest);
        hmax = patient->core.unit_template->hit_points;
        patient->core.hit_points = 3.0f;
        assert(aoe_world_combat_order_ror(&combat, priest, patient,
                                          patient->core.position.x38,
                                          patient->core.position.y3c, 0.0f,
                                          &code));
        assert(head_type(priest) == 0x69);
        for (ticks = 0;
             ticks < 4000 && patient->core.hit_points < (float)hmax; ++ticks)
            assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
        printf("priest healed %s to %g/%d in %d updates\n",
               patient->core.unit_template->name
                   ? patient->core.unit_template->name : "?",
               patient->core.hit_points, hmax, ticks);
        assert(patient->core.hit_points == (float)hmax);
        assert(combat.heals == 1 && combat.heal_failures == 0);
    }
    return 0;
}
