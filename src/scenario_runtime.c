#include "scenario_runtime.h"

#include <stdlib.h>
#include <string.h>

void aoe_scenario_runtime_destroy_ror(AoeScenarioRuntimeRor *runtime) {
    if (!runtime) return;
    for (size_t i = runtime->editor_unit_count; i > 0; --i) {
        AoeLiveUnit *unit = runtime->editor_units[i - 1];
        int16_t owner_index = unit ? unit->runtime_owner_index : -1;
        if (unit && unit->constructed && owner_index >= 0 &&
            owner_index < AOE_SCENARIO_RUNTIME_PLAYERS)
            aoe_live_unit_destroy_ror(
                unit, &runtime->players[owner_index].owner, &runtime->world,
                &runtime->players[owner_index].vision_binding);
        free(unit);
    }
    free(runtime->editor_units);
    for (int p = AOE_SCENARIO_RUNTIME_PLAYERS - 1; p >= 0; --p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        if (player->units)
            for (uint32_t i = player->unit_count; i > 0; --i)
                if (player->units[i - 1].constructed)
                    aoe_live_unit_destroy_ror(
                        &player->units[i - 1], &player->owner,
                        &runtime->world, &player->vision_binding);
        free(player->units);
        free(player->resource_values);
        aoe_visibility_watch_destroy(&player->owner.visibility_watch40);
        aoe_player_vision_state_destroy(&player->vision);
        aoe_player_templates_free(&player->templates);
    }
    free(runtime->draw_scratch);
    aoe_owned_visibility_registry_destroy(&runtime->world);
    aoe_runtime_map_destroy(&runtime->map);
    memset(runtime, 0, sizeof(*runtime));
    runtime->active_player = -1;
}

int aoe_scenario_runtime_build_ror(
    AoeScenarioRuntimeRor *runtime, const ScnMap *scenario,
    const DatCivilizations *civilizations, const DatGraphics *graphics,
    const DatTerrains *terrains, const DatBorders *borders,
    const DatMapGeometry *geometry, uint32_t rng_seed) {
    if (!runtime || !scenario || !civilizations || !graphics || !terrains ||
        !borders || !geometry || runtime->initialized || runtime->map.tiles ||
        runtime->world.view.positive || runtime->world.view.negative)
        return 0;
    runtime->active_player = -1;
    runtime->rng.state = rng_seed;
    runtime->graphics = graphics;
    runtime->geometry = geometry;
    for (unsigned p = 1; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        uint16_t count = scenario->player_details[p - 1].diplomacy_count;
        if (count > AOE_SCENARIO_RUNTIME_PLAYERS)
            count = AOE_SCENARIO_RUNTIME_PLAYERS;
        for (uint16_t q = 0; q < count; ++q)
            runtime->diplomacy[p][q] =
                (int32_t)scenario->player_details[p - 1].diplomacy_values[q];
    }
    if (!aoe_runtime_map_from_scenario(&runtime->map, scenario, geometry,
                                        terrains, borders) ||
        !aoe_owned_visibility_registry_init(&runtime->world))
        goto fail;

    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        const ScnPlayerUnits *records = &scenario->player_units[p];
        uint32_t civ_id = p ? scenario->player_setup[p - 1].civilization_id : 0;
        const DatCiv *civ;
        if (civ_id >= civilizations->count) goto fail;
        civ = &civilizations->civs[civ_id];
        player->player_index = (uint8_t)p;
        player->civilization_id = civ_id;
        player->unit_count = records->count;
        if (!aoe_player_templates_clone_civ(civ, &player->templates) ||
            !aoe_player_vision_state_init(&player->vision,
                                          &runtime->map.masks, (uint8_t)p))
            goto fail;
        player->resource_values = malloc(
            (size_t)(civ->resource_count ? civ->resource_count : 1) *
            sizeof(float));
        player->units = calloc(records->count ? records->count : 1,
                               sizeof(*player->units));
        if (!player->resource_values || !player->units) goto fail;
        if (civ->resource_count)
            memcpy(player->resource_values, civ->resources,
                   (size_t)civ->resource_count * sizeof(float));
        player->resources = (AoePlayerResources){
            (int16_t)civ->resource_count, player->resource_values};
        player->vision_binding.owner_grid = &player->vision.grid;
        player->vision_binding.owner_index = (int16_t)p;
        player->vision_binding.active_index = -1;
        player->vision_binding.explored_ops.owner_context =
            &player->vision.owner_cells58;
        player->vision_binding.explored_ops.record_owner =
            aoe_explored_cells_record;

        for (uint32_t i = 0; i < records->count; ++i) {
            const ScnUnit *record = &records->units[i];
            DatUnit *definition = aoe_player_templates_get(
                &player->templates, record->object_id);
            int32_t reserved_id;
            memcpy(&reserved_id, &record->spawn_id, sizeof(reserved_id));
            runtime->world.reserved_id = reserved_id;
            runtime->world.special_id_mode = 1;
            if (!definition || !aoe_live_unit_construct_ror(
                    &player->units[i], definition, graphics,
                    &player->resources, &runtime->world, &player->owner,
                    runtime->map.tiles, runtime->map.objects, geometry,
                    runtime->map.width, runtime->map.height,
                    &player->vision_binding, &runtime->rng,
                    record->x, record->y, record->z) ||
                player->vision.owner_cells58.allocation_failed ||
                player->units[i].core.unit_id != reserved_id)
                goto fail;
            player->units[i].runtime_owner_index = (int16_t)p;
            runtime->world.special_id_mode = 0;
            ++runtime->constructed_units;
            if (record->state <= 2) {
                if (!aoe_live_unit_scenario_dispatch_ror(
                        &player->units[i], record->state, record->rotation,
                        &(AoeLiveUnitScenarioDispatchOps){0})) goto fail;
                ++runtime->low_state_units;
            } else if (record->state >= 7) {
                if (!aoe_live_unit_scenario_high_root_ror(
                        &player->units[i], &player->resources,
                        &player->owner, &runtime->world,
                        &player->vision_binding)) goto fail;
                ++runtime->high_state_units;
            } else {
                /* No extracted campaign currently uses this branch, but a
                   general runtime may not silently invent its callbacks. */
                goto fail;
            }
        }
    }
    runtime->initialized = 1;
    return 1;
fail:
    aoe_scenario_runtime_destroy_ror(runtime);
    return 0;
}

int aoe_scenario_runtime_set_active_player_ror(
    AoeScenarioRuntimeRor *runtime, int16_t player_index) {
    if (!runtime || !runtime->initialized || player_index < 0 ||
        player_index >= AOE_SCENARIO_RUNTIME_PLAYERS)
        return 0;
    if (runtime->active_player >= 0 &&
        !aoe_runtime_map_bind_player_vision(
            &runtime->map, &runtime->players[runtime->active_player].vision,
            0)) return 0;
    if (!aoe_runtime_map_bind_player_vision(
            &runtime->map, &runtime->players[player_index].vision, 1))
        return 0;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeLiveUnitVisionBinding *binding =
            &runtime->players[p].vision_binding;
        binding->active_index = player_index;
        binding->active_relations = runtime->diplomacy[player_index];
        binding->relation_count = AOE_SCENARIO_RUNTIME_PLAYERS;
        binding->explored_ops.active_context =
            &runtime->players[player_index].vision.active_cells6c;
        binding->explored_ops.record_active = aoe_explored_cells_record;
    }
    runtime->active_player = player_index;
    return 1;
}

int aoe_scenario_runtime_editor_create_ror(
    AoeScenarioRuntimeRor *runtime, uint8_t player_index, uint16_t object_id,
    float x, float y, float z, AoeLiveUnit **out_unit) {
    AoeScenarioRuntimePlayerRor *player;
    AoeLiveUnit *unit;
    AoeLiveUnit **grown;
    size_t next_capacity;
    DatUnit *definition;
    if (out_unit) *out_unit = NULL;
    if (!runtime || !runtime->initialized ||
        player_index >= AOE_SCENARIO_RUNTIME_PLAYERS || !runtime->graphics ||
        !runtime->geometry)
        return 0;
    player = &runtime->players[player_index];
    definition = aoe_player_templates_get(&player->templates, object_id);
    if (!definition) return 0;
    if (runtime->editor_unit_count == runtime->editor_unit_capacity) {
        next_capacity = runtime->editor_unit_capacity
            ? runtime->editor_unit_capacity * 2 : 16;
        if (next_capacity < runtime->editor_unit_capacity ||
            next_capacity > SIZE_MAX / sizeof(*grown))
            return 0;
        grown = realloc(runtime->editor_units,
                        next_capacity * sizeof(*grown));
        if (!grown) return 0;
        runtime->editor_units = grown;
        runtime->editor_unit_capacity = next_capacity;
    }
    unit = calloc(1, sizeof(*unit));
    if (!unit) return 0;
    runtime->world.special_id_mode = 0;
    if (!aoe_live_unit_construct_ror(
            unit, definition, runtime->graphics, &player->resources,
            &runtime->world, &player->owner, runtime->map.tiles,
            runtime->map.objects, runtime->geometry, runtime->map.width,
            runtime->map.height, &player->vision_binding, &runtime->rng,
            x, y, z) || player->vision.owner_cells58.allocation_failed) {
        if (unit->constructed)
            aoe_live_unit_destroy_ror(unit, &player->owner, &runtime->world,
                                      &player->vision_binding);
        free(unit);
        return 0;
    }
    unit->runtime_owner_index = player_index;
    runtime->editor_units[runtime->editor_unit_count++] = unit;
    ++runtime->constructed_units;
    if (out_unit) *out_unit = unit;
    return 1;
}

void aoe_scenario_runtime_building_spawn_stack_ror(
    void *context, AoeLiveUnit *building, int16_t unit_id,
    float x, float y, float z) {
    AoeScenarioRuntimeRor *runtime = context;
    int16_t owner_index;
    if (!runtime || !building || unit_id < 0) return;
    owner_index = building->runtime_owner_index;
    if (owner_index < 0 || owner_index >= AOE_SCENARIO_RUNTIME_PLAYERS)
        return;
    (void)aoe_scenario_runtime_editor_create_ror(
        runtime, (uint8_t)owner_index, (uint16_t)unit_id,
        x, y, z, NULL);
}

void aoe_scenario_runtime_building_invalidate_foundation_ror(
    void *context, AoeLiveUnit *building,
    int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    AoeScenarioRuntimeRor *runtime = context;
    (void)building;
    if (!runtime) return;
    (void)aoe_runtime_map_invalidate_ror(
        &runtime->map, x0, y0, x1, y1, 0);
}

static AoeLiveUnit *runtime_find_core(AoeScenarioRuntimeRor *runtime,
                                     const void *object) {
    if (!runtime || !runtime->initialized || !object) return NULL;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p)
        for (uint32_t i = 0; i < runtime->players[p].unit_count; ++i)
            if (&runtime->players[p].units[i].core == object)
                return &runtime->players[p].units[i];
    for (size_t i = 0; i < runtime->editor_unit_count; ++i)
        if (runtime->editor_units[i] &&
            &runtime->editor_units[i]->core == object)
            return runtime->editor_units[i];
    return NULL;
}

int16_t aoe_scenario_runtime_terrain_object_owner_ror(
    void *context, const void *object) {
    AoeLiveUnit *unit = runtime_find_core(context, object);
    return unit ? unit->runtime_owner_index : -1;
}

int16_t aoe_scenario_runtime_terrain_object_template_ror(
    void *context, const void *object) {
    AoeLiveUnit *unit = runtime_find_core(context, object);
    return unit && unit->core.unit_template ? unit->core.unit_template->id : -1;
}

int aoe_scenario_runtime_terrain_remove_object_ror(
    void *context, void *object) {
    AoeScenarioRuntimeRor *runtime = context;
    AoeLiveUnit *unit = runtime_find_core(runtime, object);
    int16_t owner_index;
    if (!unit) return 0;
    owner_index = unit->runtime_owner_index;
    if (owner_index < 0 || owner_index >= AOE_SCENARIO_RUNTIME_PLAYERS)
        return 0;
    return aoe_live_unit_scenario_high_root_ror(
        unit, &runtime->players[owner_index].resources,
        &runtime->players[owner_index].owner, &runtime->world,
        &runtime->players[owner_index].vision_binding);
}

int aoe_scenario_runtime_tick_sprites_ror(
    AoeScenarioRuntimeRor *runtime, float world_delta,
    AoeScenarioRuntimeSpriteTickResult *result) {
    AoeScenarioRuntimeSpriteTickResult next = {0};
    if (!runtime || !runtime->initialized || !result)
        return 0;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        for (uint32_t i = 0; i < player->unit_count; ++i) {
            AoeLiveUnit *unit = &player->units[i];
            uint8_t type;
            ++next.units_visited;
            if (!unit->constructed || !unit->core.unit_template)
                return 0;
            type = unit->core.unit_template->type;
            if (type != 10 && type != 90) {
                ++next.inherited_tick_units;
                next.sprite_instances +=
                    aoe_sprite_list_count(&unit->sprites.list);
            }
            if (!aoe_live_unit_tick_sprites_ror(unit, world_delta))
                return 0;
        }
    }
    for (size_t i = 0; i < runtime->editor_unit_count; ++i) {
        AoeLiveUnit *unit = runtime->editor_units[i];
        uint8_t type;
        ++next.units_visited;
        if (!unit || !unit->constructed || !unit->core.unit_template)
            return 0;
        type = unit->core.unit_template->type;
        if (type != 10 && type != 90) {
            ++next.inherited_tick_units;
            next.sprite_instances += aoe_sprite_list_count(&unit->sprites.list);
        }
        if (!aoe_live_unit_tick_sprites_ror(unit, world_delta)) return 0;
    }
    *result = next;
    return 1;
}
