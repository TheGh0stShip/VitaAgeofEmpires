#include "dat.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "world_combat.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Garrison and transport on a real Rise of Rome scenario: a villager
   ordered onto its own transport boat (template 17) takes the enter task
   (action 3, 0x00402440) and goes inside (+0xd0, 0x004aa940): off the map
   tile, into the boat's contents. The boat's transport action (12,
   default task, 0x004067d0) is then sent to unload where it is (+0x34
   0x00406b80 -> state 6): 0x004ab280 places the villager on the shore
   again. Zones (0x00521060) separate the boat's water from the land. */
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
    AoeLiveUnit *villager = NULL, *boat = NULL;
    int16_t code = -1;
    int ticks, wx = -1, wy = -1;
    uint8_t unloaded = 0;
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
    setvbuf(stdout, NULL, _IONBF, 0);
    p = &runtime.players[1];
    for (uint32_t i = 0; i < p->unit_count && !villager; ++i)
        if (p->units[i].constructed && p->units[i].core.state == 2 &&
            p->units[i].core.unit_template->id == 83)
            villager = &p->units[i];
    assert(villager);
    /* A water tile (terrain 1, east neighbour water too) whose west
       neighbour is land, nearest to the villager. */
    {
        double best = 1e30;
        for (uint32_t y = 2; y + 2 < runtime.map.height; ++y)
            for (uint32_t x = 2; x + 2 < runtime.map.width; ++x) {
                const ScnTile *t = runtime.map.tiles;
                double dx, dy, d;
                uint8_t w = t[y * runtime.map.width + x - 1].terrain_id;
                if (t[y * runtime.map.width + x].terrain_id != 1 ||
                    t[y * runtime.map.width + x + 1].terrain_id != 1 ||
                    w == 1 || w == 0x16)
                    continue;
                dx = x - villager->core.position.x38;
                dy = y - villager->core.position.y3c;
                d = dx * dx + dy * dy;
                if (d < best) {
                    best = d;
                    wx = (int)x;
                    wy = (int)y;
                }
            }
    }
    assert(wx > 0);
    assert(aoe_scenario_runtime_editor_create_ror(
        &runtime, 1, 17, (float)wx + 1.5f, (float)wy + 0.5f, 0.0f, &boat) &&
        boat);
    villager->core.position.x38 = (float)wx - 0.5f;
    villager->core.position.y3c = (float)wy + 0.5f;
    printf("coast at %d,%d; boat capacity %d\n", wx, wy,
           boat->core.unit_template->garrison_capacity);
    /* Zones: the boat's tile and the land tile differ for the boat. */
    assert(aoe_world_motion_zone_ror(&motion, boat, wx + 1, wy) !=
           aoe_world_motion_zone_ror(&motion, boat, wx - 1, wy));
    for (int i = 0; i < 10; ++i)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    assert(head_type(boat) == 0x0c);
    assert(aoe_world_combat_order_ror(&combat, villager, boat,
                                      boat->core.position.x38,
                                      boat->core.position.y3c, 0.0f, &code));
    printf("order code %d head %d\n", code, head_type(villager));
    assert(head_type(villager) == 3);
    for (ticks = 0; ticks < 2000 && !villager->placement.attached_unit;
         ++ticks)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    printf("villager boarded after %d updates\n", ticks);
    assert(villager->placement.attached_unit == &boat->core &&
           !villager->placement.tile && boat->contents.count == 1);
    /* Unload where the boat is: +0x34 with its own position. */
    {
        /* (Calling the boat ashore replaced its transport action with a
           move, +0x154.) An order at the shore point routes task 12. */
        assert(aoe_world_combat_order_ror(&combat, boat, NULL,
                                          (float)wx - 0.5f, (float)wy + 0.5f,
                                          0.0f, &code));
        printf("boat order code %d head %d\n", code, head_type(boat));
        /* Sailing to the point (11) and idling (2); the next order then
           retargets the transport action (+0x34 0x00406b80): the point is
           within 2 in a zone next to the boat, so it unloads (6). */
        for (ticks = 0; ticks < 400; ++ticks)
            assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
        printf("boat head %d state %d at %.2f,%.2f\n", head_type(boat),
               boat->derived_action_list && boat->derived_action_list->head
                   ? ((AoeMoveActionRor *)
                          boat->derived_action_list->head->action)->state
                   : 0,
               boat->core.position.x38, boat->core.position.y3c);
        assert(aoe_world_combat_order_ror(&combat, boat, NULL,
                                          (float)wx - 0.5f, (float)wy + 0.5f,
                                          0.0f, &code));
    }
    for (ticks = 0; ticks < 2000 && boat->contents.count; ++ticks)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    printf("unloaded after %d updates: villager at %.2f,%.2f on tile %d\n",
           ticks, villager->core.position.x38, villager->core.position.y3c,
           villager->placement.tile != NULL);
    assert(boat->contents.count == 0 && !villager->placement.attached_unit &&
           villager->placement.tile);
    (void)unloaded;
    assert(combat.object_failures == 0);
    return 0;
}
