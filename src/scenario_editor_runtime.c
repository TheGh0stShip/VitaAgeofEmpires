#include "scenario_editor_runtime.h"

static int runtime_owns_unit(const AoeScenarioRuntimeRor *runtime,
                             const AoeLiveUnit *unit) {
    if (!runtime || !unit) return 0;
    for (unsigned p = 0; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p) {
        const AoeScenarioRuntimePlayerRor *player = &runtime->players[p];
        for (uint32_t i = 0; i < player->unit_count; ++i)
            if (&player->units[i] == unit) return 1;
    }
    for (size_t i = 0; i < runtime->editor_unit_count; ++i)
        if (runtime->editor_units[i] == unit) return 1;
    return 0;
}

int aoe_scenario_editor_runtime_begin_move(
    AoeScenarioEditorRuntimeContext *context, AoeLiveUnit *unit) {
    if (!context || !context->runtime || !context->runtime->initialized ||
        context->moving_unit || !runtime_owns_unit(context->runtime, unit) ||
        !unit->constructed || unit->core.state >= 7)
        return 0;
    unit->core.state = 7;
    context->moving_unit = unit;
    return 1;
}

int aoe_scenario_editor_runtime_finish_move(
    AoeScenarioEditorRuntimeContext *context) {
    AoeLiveUnit *unit;
    if (!context || !context->runtime || !context->runtime->initialized ||
        !(unit = context->moving_unit) ||
        !runtime_owns_unit(context->runtime, unit) || !unit->constructed ||
        unit->core.state != 7)
        return 0;
    unit->core.state = 2;
    context->moving_unit = NULL;
    return 1;
}

uint8_t aoe_scenario_editor_graphic_layer_ror(const DatGraphic *graphic) {
    uint8_t layer = 20;
    if (!graphic) return 0;
    if (!graphic->delta_count) return graphic->layer;
    for (uint16_t i = 0; i < graphic->delta_count; ++i) {
        const DatGraphic *child = graphic->deltas[i].graphic;
        if (!child) return 0;
        if (child->layer < layer) layer = child->layer;
    }
    return layer;
}

uint8_t aoe_scenario_editor_live_layer_ror(const AoeLiveUnit *unit) {
    uint8_t layer = 20;
    size_t count;
    if (!unit || !unit->constructed) return 0;
    count = aoe_sprite_list_count(&unit->sprites.list);
    for (size_t i = 0; i < count; ++i) {
        const DatGraphic *graphic =
            aoe_sprite_list_graphic_at(&unit->sprites.list, i);
        uint8_t candidate = aoe_scenario_editor_graphic_layer_ror(graphic);
        if (candidate < layer) layer = candidate;
    }
    return layer;
}

int aoe_scenario_editor_runtime_nonzero_layer(
    void *opaque, void *created_unit) {
    AoeScenarioEditorRuntimeContext *context = opaque;
    if (!context || !context->runtime || !context->runtime->initialized)
        return 0;
    return aoe_scenario_editor_live_layer_ror(created_unit) != 0;
}

int aoe_scenario_editor_runtime_adjust(
    void *opaque, const DatUnit *unit, float *x, float *y,
    int placement_variant) {
    AoeScenarioEditorRuntimeContext *context = opaque;
    if (!context || !context->runtime || !context->runtime->initialized)
        return 0;
    return aoe_scenario_editor_adjust_position_ror(
        unit, context->runtime->map.width, context->runtime->map.height,
        placement_variant, x, y);
}

int aoe_scenario_editor_runtime_blocked(
    void *opaque, const DatUnit *unit, float x, float y,
    uint8_t placement_class) {
    AoeScenarioEditorRuntimeContext *context = opaque;
    const float *restriction;
    AoeUnitPlacementCheckContext check;
    if (!context || !context->runtime || !context->runtime->initialized ||
        !context->intro || !context->intro->terrain_restrictions ||
        unit->terrain_restriction < 0 ||
        (uint16_t)unit->terrain_restriction >= context->intro->restriction_count)
        return 1;
    restriction = context->intro->terrain_restrictions +
        (size_t)(uint16_t)unit->terrain_restriction *
            context->intro->terrain_count;
    check = (AoeUnitPlacementCheckContext){
        &context->runtime->map, restriction, context->intro->terrain_count,
        1, NULL, 0, 0}; /* Scenario editor mode is original mode 7. */
    context->blocking_unit_id = -1;
    context->last_placement_result = aoe_unit_placement_check_ror(
        unit, &check, x, y, &context->blocking_unit_id,
        1, placement_class, 0, 1, 1, 1);
    return context->last_placement_result != AOE_PLACEMENT_OK;
}

void *aoe_scenario_editor_runtime_create(
    void *opaque, uint16_t object_id, float x, float y, float z,
    int editor_create) {
    AoeScenarioEditorRuntimeContext *context = opaque;
    AoeLiveUnit *unit = NULL;
    if (!context || !context->runtime || editor_create != 1 ||
        !aoe_scenario_runtime_editor_create_ror(
            context->runtime, context->player_index, object_id,
            x, y, z, &unit))
        return NULL;
    return unit;
}

int aoe_scenario_editor_runtime_relocate(
    void *opaque, void *selected_unit, float x, float y, float z) {
    AoeScenarioEditorRuntimeContext *context = opaque;
    AoeLiveUnit *unit = selected_unit;
    int16_t owner_index;
    if (!context || !context->runtime || !context->runtime->initialized ||
        !unit || unit != context->moving_unit || !unit->constructed ||
        unit->core.state != 7 ||
        (owner_index = unit->runtime_owner_index) < 0 ||
        owner_index >= AOE_SCENARIO_RUNTIME_PLAYERS)
        return 0;
    return aoe_live_unit_place_ror_with_vision(
        &unit->core, &unit->placement, context->runtime->map.tiles,
        context->runtime->map.objects, context->runtime->geometry,
        context->runtime->map.width, context->runtime->map.height,
        x, y, z, &context->runtime->players[owner_index].vision_binding);
}
