#include "dat.h"
#include "scenario_editor_runtime.h"
#include "scenario_runtime.h"
#include "scn.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int editor_redraw_status;

static void editor_redraw(void *context, int status) {
    (void)context;
    editor_redraw_status = status;
}

/* Builds one simultaneous owned world from original scenario records through
   the recovered common RoR constructor. State 0..2 records also run their
   original load virtual. State 7+ dispatch is counted but not replaced with
   a direct state-byte write while its required virtuals remain untranslated. */
int main(int argc, char **argv) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    DatTerrains terrains = {0};
    DatBorders borders = {0};
    DatMapGeometry geometry = {0};
    DatRandomMaps random_maps = {0};
    DatEffects effects = {0};
    DatCivilizations civs = {0};
    ScnMap scenario = {0};
    AoeScenarioRuntimeRor runtime = {0};
    AoeScenarioRuntimeSpriteTickResult tick = {0};
    AoeLiveUnit *editor_unit = NULL;
    int ok = 0;
    if (argc != 3) {
        fprintf(stderr, "usage: runtime_full_units_real SCENARIO EMPIRES.DAT\n");
        return 2;
    }
    if (!dat_load(argv[2], &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario) ||
        !aoe_scenario_runtime_build_ror(
            &runtime, &scenario, &intro, &civs, &graphics, &terrains, &borders,
            &geometry, 1) ||
        !aoe_scenario_runtime_set_active_player_ror(&runtime, 1)) goto done;

    AoeLiveUnit *terrain_removed = NULL;
    AoeBuildingCompletionWorldRuntimeOps completion_world;
    if (!aoe_scenario_runtime_building_completion_world_ops_ror(
            &runtime, NULL, NULL, &completion_world)) goto done;
    for (uint32_t i = 0; i < runtime.players[0].unit_count &&
                         !terrain_removed; ++i) {
        AoeLiveUnit *candidate = &runtime.players[0].units[i];
        int32_t tile_x = (int32_t)candidate->core.position.x38;
        int32_t tile_y = (int32_t)candidate->core.position.y3c;
        if (candidate->core.state >= 7 ||
            candidate->core.unit_template->type != 10 ||
            tile_x < 0 || tile_y < 0 ||
            (uint32_t)tile_x >= runtime.map.width ||
            (uint32_t)tile_y >= runtime.map.height ||
            terrains.terrains[0].elevation_frames[
                runtime.map.shapes[(size_t)tile_y * runtime.map.width +
                                   (uint32_t)tile_x]].frame_count <= 0)
            continue;
        for (uint16_t j = 0;
             j < terrains.terrains[0].terrain_units_used_count; ++j)
            if (terrains.terrains[0].terrain_unit_ids[j] ==
                candidate->core.unit_template->id) {
                terrain_removed = candidate;
                break;
            }
    }
    if (!terrain_removed ||
        aoe_scenario_runtime_terrain_object_owner_ror(
            &runtime, &terrain_removed->core) != 0 ||
        aoe_scenario_runtime_terrain_object_template_ror(
            &runtime, &terrain_removed->core) !=
            terrain_removed->core.unit_template->id)
        goto done;
    {
        int16_t x = (int16_t)terrain_removed->core.position.x38;
        int16_t y = (int16_t)terrain_removed->core.position.y3c;
        AoeBuildingFoundationBounds bounds = {x, y, x, y};
        size_t index = (size_t)(uint16_t)y * runtime.map.width +
                       (uint16_t)x;
        if (!completion_world.paint_foundation(
                completion_world.context, terrain_removed, &bounds, 0) ||
            terrain_removed->core.state != 7 ||
            !completion_world.paint_foundation(
                completion_world.context, terrain_removed, &bounds, 7) ||
            runtime.map.tiles[index].terrain_id != 7 ||
            runtime.map.cache_flags[index] != 0xcc ||
            runtime.map.draw_flags[index] != 0)
            goto done;
    }

    unsigned editor_player = 0;
    while (editor_player < AOE_SCENARIO_RUNTIME_PLAYERS &&
           !scenario.player_units[editor_player].count)
        ++editor_player;
    if (editor_player == AOE_SCENARIO_RUNTIME_PLAYERS) goto done;
    const ScnUnit *editor_source =
        &scenario.player_units[editor_player].units[0];
    if (!aoe_scenario_runtime_set_active_player_ror(
            &runtime, (int16_t)editor_player)) goto done;
    uint64_t loaded_units = runtime.constructed_units;
    AoeScenarioEditorRuntimeContext editor_context = {
        &runtime, &intro, (uint8_t)editor_player, 0, -1};
    AoeScenarioEditorPlacementOps editor_ops = {
        aoe_scenario_editor_runtime_adjust,
        aoe_scenario_editor_runtime_blocked,
        aoe_scenario_editor_runtime_create,
        aoe_scenario_editor_runtime_relocate, NULL,
        aoe_scenario_editor_runtime_nonzero_layer,
        editor_redraw, &editor_context};
    float place_x = 0, place_y = 0;
    uint16_t place_id = editor_source->object_id;
    int found = 0;
    DatUnit *place_template = aoe_player_templates_get(
        &runtime.players[editor_player].templates, place_id);
    if (!place_template) goto done;
    for (uint32_t py = 0; py < runtime.map.height && !found; ++py)
        for (uint32_t px = 0; px < runtime.map.width && !found; ++px) {
            float ax = (float)px + 0.5f, ay = (float)py + 0.5f;
            if (aoe_scenario_editor_runtime_adjust(
                    &editor_context, place_template, &ax, &ay, 1) &&
                !aoe_scenario_editor_runtime_blocked(
                    &editor_context, place_template, ax, ay,
                    place_template->placement_class)) {
                place_x = ax;
                place_y = ay;
                found = 1;
            }
        }
    void *editor_created = NULL;
    editor_redraw_status = 0;
    if (!found || !aoe_scenario_editor_place_ror(
            &runtime.players[editor_player].templates, place_id, 8, 0,
            place_x, place_y, &editor_ops,
            &editor_created) ||
        !(editor_unit = editor_created) ||
        editor_redraw_status !=
            (aoe_scenario_editor_live_layer_ror(editor_unit) ? 1 : 2) ||
        !editor_unit || !editor_unit->constructed ||
        runtime.constructed_units != loaded_units + 1 ||
        runtime.editor_unit_count != 1)
        goto done;

    /* Building completion uses the same owner-template +0x18 factory without
       consuming its ignored return value. Bind that path to owned storage. */
    aoe_scenario_runtime_building_spawn_stack_ror(
        &runtime, editor_unit, (int16_t)place_id,
        place_x, place_y, editor_source->z);
    if (runtime.constructed_units != loaded_units + 2 ||
        runtime.editor_unit_count != 2 ||
        runtime.editor_units[1]->runtime_owner_index != (int16_t)editor_player ||
        runtime.editor_units[1]->core.unit_template != place_template ||
        runtime.editor_units[1]->core.position.x38 != place_x ||
        runtime.editor_units[1]->core.position.y3c != place_y)
        goto done;
    {
        int32_t tile_x = (int32_t)place_x;
        int32_t tile_y = (int32_t)place_y;
        size_t tile_index = (size_t)tile_y * runtime.map.width +
                            (uint32_t)tile_x;
        runtime.map.draw_flags[tile_index] = 0x7e;
        aoe_scenario_runtime_building_invalidate_foundation_ror(
            &runtime, editor_unit, tile_x, tile_y, tile_x, tile_y);
        if (runtime.map.cache_flags[tile_index] != 0xcc ||
            runtime.map.draw_flags[tile_index] != 0)
            goto done;
    }

    /* Editor submode 1 is the original move-selected-unit path, not a
       second factory call. The outer mouse-up handler restores state 2. */
    if (aoe_scenario_editor_runtime_finish_move(&editor_context)) goto done;
    if (!aoe_scenario_editor_runtime_begin_move(
            &editor_context, editor_unit) || editor_unit->core.state != 7 ||
        aoe_scenario_editor_runtime_begin_move(&editor_context, editor_unit))
        goto done;
    editor_ops.selected_unit = editor_unit;
    if (!aoe_scenario_editor_place_ror(
            &runtime.players[editor_player].templates, place_id, 8, 1,
            place_x, place_y, &editor_ops,
            &editor_created) || editor_created != editor_unit ||
        runtime.constructed_units != loaded_units + 2 ||
        runtime.editor_unit_count != 2)
        goto done;
    if (!aoe_scenario_editor_runtime_finish_move(&editor_context) ||
        editor_context.moving_unit || editor_unit->core.state != 2 ||
        aoe_scenario_editor_runtime_finish_move(&editor_context))
        goto done;

    AoeTerrainTransitionOps terrain_ops;
    float terrain_x = 0.0f, terrain_y = 0.0f;
    int terrain_found = 0;
    int16_t terrain_spawn_id = terrains.terrains[0].terrain_unit_ids[0];
    if (!aoe_scenario_runtime_terrain_transition_ops_ror(
            &runtime, &terrain_ops)) goto done;
    for (uint32_t py = 0; py < runtime.map.height && !terrain_found; ++py)
        for (uint32_t px = 0; px < runtime.map.width && !terrain_found; ++px) {
            float x = (float)px + 0.5f, y = (float)py + 0.5f;
            if (!terrain_ops.blocked(
                    terrain_ops.context, terrain_spawn_id, x, y)) {
                terrain_x = x;
                terrain_y = y;
                terrain_found = 1;
            }
        }
    if (!terrain_found || !terrain_ops.create(
            terrain_ops.context, terrain_spawn_id, terrain_x, terrain_y) ||
        runtime.constructed_units != loaded_units + 3 ||
        runtime.editor_unit_count != 3 ||
        runtime.editor_units[2]->runtime_owner_index != 0 ||
        runtime.editor_units[2]->core.unit_template->id != terrain_spawn_id)
        goto done;

    if (loaded_units != runtime.low_state_units + runtime.high_state_units ||
        !runtime.constructed_units ||
        !aoe_scenario_runtime_tick_sprites_ror(&runtime, 1.0f / 60.0f,
                                                &tick) ||
        tick.units_visited != runtime.constructed_units)
        goto done;
    printf("%s: %" PRIu64 " simultaneous full live units constructed; %"
           PRIu64 " low-state and %" PRIu64
           " high-state virtuals dispatched; %" PRIu64
           " units reached the inherited sprite tick (%" PRIu64
           " sprite instances)\n",
           argv[1], runtime.constructed_units, runtime.low_state_units,
           runtime.high_state_units, tick.inherited_tick_units,
           tick.sprite_instances);
    ok = 1;
done:
    aoe_scenario_runtime_destroy_ror(&runtime);
    scn_free_map(&scenario);
    dat_free_civilizations(&civs);
    dat_free_effects(&effects);
    dat_free_random_maps(&random_maps);
    dat_free_borders(&borders);
    dat_free_terrains(&terrains);
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok ? 0 : 1;
}
