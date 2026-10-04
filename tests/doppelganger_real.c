#include "dat.h"
#include "scenario_runtime.h"
#include "scn.h"
#include "world_combat.h"
#include "aoe_ftol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Fog memory on a real Rise of Rome scenario: a Gaia gold mine (effect
   mode 1) removed while player 2 has explored but cannot see its tile
   leaves player 2 a doppelganger (root +0x60 0x004aa350 -> template 243
   -> 0x00436030). Its update (0x00436630) drops the dead source and
   removes it once player 2 sees the tile again. */
static AoeLiveUnit *find_doppel(AoeScenarioRuntimeRor *r, int *count) {
    AoeLiveUnit *found = NULL;
    *count = 0;
    for (size_t i = 0; i < r->editor_unit_count; ++i) {
        AoeLiveUnit *u = r->editor_units[i];
        if (u && u->constructed && u->core.class_code == 0x19 &&
            u->core.state < 7) {
            found = u;
            ++*count;
        }
    }
    return found;
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
    AoeLiveUnit *mine = NULL, *d;
    AoeScenarioRuntimePlayerRor *gaia;
    uint32_t *mask;
    int count, ticks;
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
    assert(aoe_scenario_runtime_set_active_player_ror(&runtime, 1));
    assert(aoe_world_motion_init_ror(&motion, &runtime));
    assert(aoe_world_combat_init_ror(&combat, &motion));
    gaia = &runtime.players[0];
    for (uint32_t i = 0; i < gaia->unit_count && !mine; ++i) {
        AoeLiveUnit *u = &gaia->units[i];
        if (u->constructed && u->core.state == 2 && u->core.unit_id >= 0 &&
            u->core.unit_template->id == 66) /* Mine-Gold */
            mine = u;
    }
    assert(mine && mine->core.unit_template->state_effect_mode == 1);
    mask = &runtime.map.masks.masks[
        (size_t)aoe_ftol32(mine->core.position.y3c) * runtime.map.width +
        (uint32_t)aoe_ftol32(mine->core.position.x38)];
    /* Player 1 sees and explored the tile; player 2 only explored it. */
    *mask = (1u << 1) | (1u << 17) | (1u << 18);
    assert(find_doppel(&runtime, &count) == NULL && count == 0);
    assert(aoe_live_unit_scenario_high_root_ror(
        mine, &gaia->resources, &gaia->owner, &runtime.world,
        &gaia->vision_binding));
    assert(mine->core.state == 7);
    d = find_doppel(&runtime, &count);
    assert(d && count == 1 && runtime.doppelgangers_created == 1);
    assert(d->runtime_owner_index == 2);
    assert(d->core.unit_template->id == 243);
    assert(d->doppel.source == mine && d->doppel.source_owner == 0 &&
           d->doppel.source_template == mine->core.unit_template);
    assert(d->graphic.current_graphic == mine->graphic.current_graphic);
    assert(aoe_sprite_list_count(&d->sprites.list) ==
           aoe_sprite_list_count(&mine->sprites.list));
    printf("doppelganger %d for player 2 of mine %d at %.1f,%.1f\n",
           d->core.unit_id, mine->core.unit_id, d->core.position.x38,
           d->core.position.y3c);

    /* The world update drops the dead source but keeps the image while
       player 2 cannot see the tile. */
    for (ticks = 0; ticks < 10; ++ticks)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    d = find_doppel(&runtime, &count);
    assert(d && count == 1 && d->doppel.source == NULL &&
           d->doppel.source_id == -1);

    /* Player 2 sees the tile again: the doppelganger leaves (+0x68). */
    *mask |= 1u << 2;
    for (ticks = 0; ticks < 10 && find_doppel(&runtime, &count); ++ticks)
        assert(aoe_world_combat_update_world_ror(&combat, 0.05f));
    assert(find_doppel(&runtime, &count) == NULL);
    printf("removed after %d updates once player 2 saw the tile\n", ticks);

    aoe_world_combat_destroy_ror(&combat);
    aoe_world_motion_destroy_ror(&motion);
    aoe_scenario_runtime_destroy_ror(&runtime);
    return 0;
}
