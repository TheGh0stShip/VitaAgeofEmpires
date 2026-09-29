#include "live_unit_factory.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const DatGraphic *standing_graphic(const DatUnit *unit,
                                           const DatGraphics *graphics) {
    int16_t id;
    if (!unit || !graphics) return 0;
    id = unit->standing_graphic;
    if (id < 0) return 0;
    if ((uint16_t)id >= graphics->count || !graphics->graphics[id].present)
        return 0;
    return &graphics->graphics[id];
}

int aoe_live_unit_tick_sprites_ror(AoeLiveUnit *unit, float world_delta) {
    uint8_t type;
    if (!unit || !unit->constructed || !unit->core.unit_template ||
        !isfinite(world_delta) || !isfinite(unit->animation_rate))
        return 0;
    type = unit->core.unit_template->type;
    if (type == 10 || type == 90)
        return 1;
    if (type != 20 && type != 25 && type != 30 && type != 40 &&
        type != 50 && type != 60 && type != 70 && type != 80)
        return 0;
    return aoe_sprite_runtime_tick(&unit->sprites, world_delta,
                                   unit->animation_rate);
}

void aoe_live_unit_destroy_ror(AoeLiveUnit *unit, AoePlayerUpdateState *owner,
                               AoeOwnedVisibilityRegistry *world,
                               AoeLiveUnitVisionBinding *vision) {
    int removed;
    if (!unit) return;
    if (unit->owner_node && owner && world)
        (void)aoe_player_unit_remove(owner, world, &unit->core.position,
                                     unit->core.unit_id, unit->select_2c,
                                     unit->select_30, unit->owner_node, &removed);
    if (unit->placement.tile && vision && unit->core.state == 2)
        (void)aoe_live_unit_vision_change_ror(&unit->core, vision, 0, 0);
    if (unit->placement.tile)
        (void)aoe_object_update_list_detach(unit->placement.tile, &unit->core,
                                            unit->placement.node);
    aoe_sprite_runtime_clear(&unit->sprites);
    free(unit->dependent_unit_ids);
    memset(unit, 0, sizeof(*unit));
}

int aoe_live_unit_update_prelude_ror(
    AoeLiveUnit *unit, AoePlayerUpdateState *owner,
    AoeOwnedVisibilityRegistry *world, int *handled) {
    size_t write = 0;
    if (handled) *handled = 0;
    if (!unit || !unit->constructed || !owner || !world || !handled ||
        (unit->dependent_unit_count && !unit->dependent_unit_ids) ||
        unit->dependent_unit_count > unit->dependent_unit_capacity)
        return 0;
    if (unit->core.registration_pending) {
        AoeObjectUpdateNode *next_node = NULL;
        AoeVisibilityUnitRef *replaced = NULL;
        int removed = 0, needs_reassign = 0;
        unit->core.registration_pending = 0;
        if (!unit->owner_node || !aoe_player_unit_remove(
                owner, world, &unit->core.position, unit->core.unit_id,
                0, unit->core.type_25, unit->owner_node, &removed) ||
            !removed || !aoe_player_unit_register(
                owner, world, &unit->core.position, unit->core.unit_id,
                1, unit->core.type_25, &next_node, &replaced,
                &needs_reassign) || replaced || needs_reassign || !next_node) {
            /* The original has no recoverable allocation-failure rollback;
               fail explicitly rather than fabricating a linked state. */
            unit->owner_node = NULL;
            return 0;
        }
        unit->owner_node = next_node;
        unit->select_2c = 1;
        *handled = 1;
        return 1;
    }
    for (size_t read = 0; read < unit->dependent_unit_count; ++read) {
        AoeVisibilityUnitRef *reference = aoe_visibility_registry_resolve(
            &world->view, unit->dependent_unit_ids[read]);
        AoeLiveUnitCore *other = reference ?
            (AoeLiveUnitCore *)((char *)reference -
                                offsetof(AoeLiveUnitCore, position)) : NULL;
        if (!other || other->state > 2) continue;
        unit->dependent_unit_ids[write++] = unit->dependent_unit_ids[read];
    }
    unit->dependent_unit_count = write;
    return 1;
}

int aoe_live_unit_update_attached_target_ror(
    AoeLiveUnit *unit, const AoeLiveUnitUpdateTargetOps *ops, int *handled) {
    AoeLiveUnitCore *target;
    if (handled) *handled = 0;
    if (!unit || !unit->constructed || !handled) return 0;
    target = unit->placement.attached_unit;
    if (!target) return 1;
    *handled = 1;
    if (target->state <= 2) return 1;
    if (!ops || !ops->set_state) return 0;
    return ops->set_state(ops->context, unit, target->state);
}

int aoe_live_unit_update_normal_state_ror(
    AoeLiveUnit *unit, const AoeLiveUnitUpdateStateOps *ops,
    uint8_t *predicate_result) {
    const DatUnit *definition;
    if (predicate_result) *predicate_result = 0;
    if (!unit || !unit->constructed || !unit->core.unit_template || !ops ||
        !predicate_result || !isfinite(ops->world_delta) ||
        ops->world_delta < 0.0f ||
        !isfinite(ops->comparison_epsilon) ||
        ops->comparison_epsilon < 0.0f)
        return 0;
    definition = unit->core.unit_template;
    switch (unit->core.state) {
    case 2:
        if (!ops->active_update ||
            !ops->active_update(ops->context, unit)) return 0;
        /* Original switch deliberately falls through to state zero. */
        /* fall through */
    case 0:
        if (unit->core.hit_points < 0.0f) {
            if (!ops->set_state || !ops->select_graphic ||
                (definition->dying_sound >= 0 && !ops->play_dying_sound) ||
                !ops->set_state(ops->context, unit, 3) ||
                !ops->select_graphic(ops->context, unit,
                                     definition->dying_graphic)) return 0;
            if (definition->dying_sound >= 0) {
                ops->play_dying_sound(ops->context, unit,
                                      definition->dying_sound);
            }
        }
        return 1;
    case 3:
    case 5:
        if (unit->core.storage.carried_resource_type == -1 ||
            unit->core.storage.carried_amount <= ops->comparison_epsilon) {
            if (!ops->set_state) return 0;
            if (definition->undead_mode) {
                if (!ops->select_graphic) return 0;
                if (!ops->set_state(ops->context, unit, 6) ||
                    !ops->select_graphic(ops->context, unit,
                                         definition->undead_graphic)) return 0;
            } else {
                if (!ops->terminal_cleanup) return 0;
                if (!ops->set_state(ops->context, unit, 7) ||
                    !ops->terminal_cleanup(ops->context, unit)) return 0;
            }
            return 1;
        }
        if (definition->decay_rate > ops->comparison_epsilon) {
            float amount = unit->core.storage.carried_amount -
                ops->world_delta * definition->decay_rate;
            unit->core.storage.carried_amount =
                amount < ops->comparison_epsilon ? 0.0f : amount;
        }
        return 1;
    case 7:
        return ops->set_state && ops->set_state(ops->context, unit, 8);
    case 8:
        *predicate_result = (uint8_t)(1 + (definition->negative_id_mode != 0));
        return 1;
    default:
        return 1;
    }
}

int aoe_live_unit_update_predicate_ror(
    AoeLiveUnit *unit, AoePlayerUpdateState *owner,
    AoeOwnedVisibilityRegistry *world,
    const AoeLiveUnitUpdateTargetOps *target_ops,
    const AoeLiveUnitUpdateStateOps *state_ops,
    uint8_t *predicate_result) {
    int handled = 0;
    if (predicate_result) *predicate_result = 0;
    if (!unit || !owner || !world || !state_ops || !predicate_result)
        return 0;
    if (!aoe_live_unit_update_prelude_ror(unit, owner, world, &handled))
        return 0;
    if (handled) return 1;
    if (!aoe_live_unit_update_attached_target_ror(
            unit, target_ops, &handled)) return 0;
    if (handled) return 1;
    return aoe_live_unit_update_normal_state_ror(
        unit, state_ops, predicate_result);
}

static AoeLiveUnit *live_from_update_object(
    const AoeLiveUnitPlayerUpdateBinding *binding, const void *object) {
    if (!binding || !object || (binding->unit_count && !binding->units))
        return NULL;
    for (size_t i = 0; i < binding->unit_count; ++i) {
        AoeLiveUnit *unit = binding->units[i];
        if (unit && (const void *)&unit->core.position == object)
            return unit;
    }
    return NULL;
}

static int bound_live_predicate(void *opaque, void *object) {
    AoeLiveUnitPlayerUpdateBinding *binding = opaque;
    AoeLiveUnit *unit = live_from_update_object(binding, object);
    uint8_t result = 0;
    if (!binding || binding->failed || !object) {
        if (binding) binding->failed = 1;
        return 0;
    }
    if (!unit) {
        if (!binding->other_predicate) {
            binding->failed = 1;
            return 0;
        }
        return binding->other_predicate(binding->context, object);
    }
    if (!aoe_live_unit_update_predicate_ror(
            unit, binding->owner, binding->world, &binding->target_ops,
            &binding->state_ops, &result)) {
        binding->failed = 1;
        return 0;
    }
    return result != 0;
}

static void bound_live_action(void *opaque, void *object) {
    AoeLiveUnitPlayerUpdateBinding *binding = opaque;
    AoeLiveUnit *unit = live_from_update_object(binding, object);
    if (!binding || binding->failed || !object) {
        if (binding) binding->failed = 1;
        return;
    }
    if (!unit) {
        if (!binding->other_action) binding->failed = 1;
        else binding->other_action(binding->context, object);
        return;
    }
    uint8_t type = unit->core.unit_template->type;
    int cleaned;
    if (type == 80) {
        AoeBuildingUpdate *building = unit->building_update;
        uint32_t *count = building && binding->queue_status_four_count ?
            binding->queue_status_four_count(binding->context,
                                              building->owner_id) : NULL;
        cleaned = building && aoe_building_cleanup_type80_ror(
            building, count, binding->context,
            binding->deallocate_queue_entries) &&
            aoe_live_unit_cleanup_derived_prefix_ror(unit) &&
            binding->common_cleanup &&
            binding->common_cleanup(binding->context, unit);
    } else {
        cleaned = aoe_live_unit_cleanup_derived_prefix_ror(unit) &&
                  binding->common_cleanup &&
                  binding->common_cleanup(binding->context, unit);
    }
    if (!cleaned) {
        binding->failed = 1;
    }
}

static int bound_cooperative_pending(void *opaque) {
    AoeLiveUnitPlayerUpdateBinding *binding = opaque;
    if (!binding || !binding->cooperative_pending) {
        if (binding) binding->failed = 1;
        return 0;
    }
    return binding->cooperative_pending(binding->context);
}

static void bound_cooperative_pump(void *opaque) {
    AoeLiveUnitPlayerUpdateBinding *binding = opaque;
    if (!binding || !binding->cooperative_pump) {
        if (binding) binding->failed = 1;
        return;
    }
    binding->cooperative_pump(binding->context);
}

int aoe_live_unit_player_update_step_ror(
    AoeLiveUnitPlayerUpdateBinding *binding, AoePlayerUpdateState *owner,
    const AoePlayerUpdateOps *ops) {
    AoePlayerUpdateOps bound_ops;
    if (!binding || !owner || !ops || binding->owner != owner ||
        !binding->world || !binding->common_cleanup ||
        !binding->other_predicate || !binding->other_action ||
        (binding->unit_count && !binding->units) ||
        !binding->cooperative_pending || !binding->cooperative_pump)
        return 0;
    binding->failed = 0;
    bound_ops = *ops;
    bound_ops.object_ops.user = binding;
    bound_ops.object_ops.predicate = bound_live_predicate;
    bound_ops.object_ops.action = bound_live_action;
    bound_ops.object_ops.cooperative_pending = bound_cooperative_pending;
    bound_ops.object_ops.cooperative_pump = bound_cooperative_pump;
    if (!aoe_player_update_step(owner, AOE_GROUP_RISE_OF_ROME, &bound_ops))
        return 0;
    return !binding->failed;
}

int aoe_live_unit_cleanup_uses_reuse_pool_ror(const AoeLiveUnit *unit) {
    if (!unit || !unit->constructed || !unit->core.unit_template ||
        !unit->core.unit_template->negative_id_mode) return 0;
    switch (unit->core.unit_template->type) {
    case 10: case 20: case 25: case 30: case 40: case 50: case 60:
        return 1;
    default:
        return 0;
    }
}

int aoe_live_unit_cleanup_derived_prefix_ror(AoeLiveUnit *unit) {
    uint8_t type;
    if (!unit || !unit->constructed || !unit->core.unit_template) return 0;
    type = unit->core.unit_template->type;
    if (((type >= 40 && type <= 70) || type == 80) &&
        unit->derived_action_list)
        return aoe_action_list_clear(unit->derived_action_list);
    return 1;
}

int aoe_live_unit_construct_ror(
    AoeLiveUnit *out, const DatUnit *unit_template, const DatGraphics *graphics,
    AoePlayerResources *owner_resources, AoeOwnedVisibilityRegistry *world,
    AoePlayerUpdateState *owner, const ScnTile *terrain_tiles,
    AoeObjectUpdateList *tile_lists, const DatMapGeometry *geometry,
    uint32_t width, uint32_t height,
    AoeLiveUnitVisionBinding *vision, AoeRng *rng,
    float x, float y, float z) {
    AoeVisibilityUnitRef *replaced = 0;
    int needs_reassign = 0;
    const DatGraphic *standing;
    AoeUnitGraphicTemplate graphic_template;
    if (!out || !unit_template || !graphics || !owner_resources || !world ||
        !owner || !terrain_tiles || !tile_lists || !geometry || !vision || !rng)
        return 0;
    standing = standing_graphic(unit_template, graphics);
    if (unit_template->standing_graphic >= 0 && !standing) return 0;
    /* Tile and registry links retain the unit address. Unlike scalar helper
       state, this object cannot be assembled in a temporary then copied. */
    memset(out, 0, sizeof(*out));
    out->runtime_owner_index = -1;
    if (!aoe_sprite_runtime_init(&out->sprites, rng, 0, 0, 0) ||
        !aoe_live_unit_core_begin(unit_template, owner_resources,
                                  AOE_PLAYER_RISE_OF_ROME, world,
                                  x, y, z, &out->core))
        return 0;
    graphic_template.standing_graphic = standing;
    graphic_template.walking_graphic = 0;
    if (standing && !aoe_unit_graphic_select_runtime(
            &out->graphic, &graphic_template, &out->sprites, 0,
            AOE_PLAYER_RISE_OF_ROME))
        goto fail;
    if (!aoe_live_unit_place_ror_with_vision(
            &out->core, &out->placement, terrain_tiles, tile_lists,
            geometry, width, height, x, y, z, vision))
        goto fail;
    /* The original replacement branch requires the displaced unit's virtual
       rebase method. Do not enter it until that method is translated. */
    if (aoe_visibility_registry_resolve(&world->view, out->core.unit_id))
        goto fail;
    out->select_2c = out->core.positive_hit_points;
    out->select_30 = out->core.type_25;
    if (!aoe_player_unit_register(owner, world, &out->core.position,
                                  out->core.unit_id, out->select_2c,
                                  out->select_30, &out->owner_node, &replaced,
                                  &needs_reassign) || replaced || needs_reassign)
        goto fail;
    out->constructed = 1;
    return 1;
fail:
    aoe_live_unit_destroy_ror(out, owner, world, vision);
    return 0;
}

int aoe_live_unit_scenario_dispatch_ror(
    AoeLiveUnit *unit, uint8_t scenario_state, float rotation,
    const AoeLiveUnitScenarioDispatchOps *ops) {
    uint8_t type;
    const DatGraphic *graphic;
    if (!unit || !unit->constructed || !unit->core.unit_template || !ops)
        return 0;
    type = unit->core.unit_template->type;
    graphic = unit->graphic.current_graphic;
    if (scenario_state <= 2)
        return aoe_live_unit_low_state_ror(&unit->core, graphic, rotation);
    if (scenario_state <= 6) {
        if (type == 70 || type == 80) {
            int16_t kind;
            if (unit->core.state == 2 &&
                ops->derived_middle_threshold <= ops->derived_middle_time &&
                ops->derived_middle_lookup &&
                ops->derived_middle_lookup(ops->context, &kind) &&
                kind == 0x68) {
                if (!ops->derived_middle_kind_68) return 0;
                ops->derived_middle_kind_68(ops->context, unit);
            }
        }
        return aoe_live_unit_root_middle_state(&unit->core,
                                                ops->registration_node,
                                                &ops->registration,
                                                &ops->root_middle_action);
    }
    if (type == 80) {
        return aoe_live_unit_building_high_state_ror(unit,ops);
    }
    if (type == 40 || type == 50 || type == 60 || type == 70) {
        if (!ops->clear_derived_actions) return 0;
        ops->clear_derived_actions(ops->context, unit);
    }
    return aoe_live_unit_root_high_state_ror(&unit->core, &ops->root_high);
}

int aoe_live_unit_building_high_state_ror(
    AoeLiveUnit *unit, const AoeLiveUnitScenarioDispatchOps *ops) {
    if (!unit || !unit->constructed || !unit->core.unit_template ||
        unit->core.unit_template->type != 80 || !ops ||
        !ops->building_pre_high || !ops->remove_building_manager)
        return 0;
    ops->building_pre_high(ops->context,unit);
    if (!aoe_live_unit_cleanup_derived_prefix_ror(unit) ||
        !aoe_live_unit_root_high_state_ror(&unit->core,&ops->root_high) ||
        !ops->remove_building_manager(ops->context,unit)) return 0;
    unit->building_manager_link=NULL;
    return 1;
}

int aoe_live_unit_building_state_three_ror(
    AoeLiveUnit *unit, const AoeBuildingStateThreeOps *ops) {
    AoeBuildingUpdate *building;
    int16_t research;
    if (!unit || !unit->constructed || !unit->core.unit_template ||
        unit->core.unit_template->type != 80 || !unit->building_update ||
        !ops || !ops->building_pre_state_three ||
        !ops->remove_building_manager || !ops->completion_probe ||
        !ops->remove_research || !ops->owner_status_four_count)
        return 0;
    building=unit->building_update;
    if (building->unit_template != unit->core.unit_template ||
        building->state != 2 || unit->core.state != 2) return 0;
    ops->building_pre_state_three(ops->context,unit);
    if (!ops->remove_building_manager(ops->context,unit)) return 0;
    unit->building_manager_link=NULL;
    if (!aoe_building_state_queue_transition_ror(
            building,3,ops->owner_status_four_count,&ops->queue_cancel) ||
        !aoe_live_unit_root_state_set_ror(&unit->core,3,&ops->root_state))
        return 0;
    building->state=unit->core.state;
    if (!ops->completion_probe(ops->context,unit->core.unit_template->id)) {
        research=unit->core.unit_template->building_research_id;
        if (research >= 0) ops->remove_research(ops->context,research);
        ops->remove_research(ops->context,0x4a);
        ops->remove_research(ops->context,0x4b);
        ops->remove_research(ops->context,0x4c);
    }
    return 1;
}

typedef struct {
    AoeLiveUnit *live;
    AoePlayerUpdateState *owner;
    AoeOwnedVisibilityRegistry *world;
    AoeLiveUnitVisionBinding *vision;
    const uint32_t *live_flags;
    int failed;
} HighRootContext;

static void high_remove(void *opaque, AoeLiveUnitCore *core,
                        uint8_t was_positive, uint8_t type_25, void *node) {
    HighRootContext *ctx = opaque;
    int removed = 0;
    if (core != &ctx->live->core ||
        !aoe_player_unit_remove(ctx->owner, ctx->world, &core->position,
                                core->unit_id, was_positive, type_25,
                                node, &removed) || !removed) {
        ctx->failed = 1;
        return;
    }
    ctx->live->owner_node = NULL;
}

static void *high_register(void *opaque, AoeLiveUnitCore *core,
                           uint8_t is_positive, uint8_t type_25) {
    HighRootContext *ctx = opaque;
    AoeObjectUpdateNode *node = NULL;
    AoeVisibilityUnitRef *replaced = NULL;
    int needs_reassign = 0;
    if (core != &ctx->live->core ||
        !aoe_player_unit_register(ctx->owner, ctx->world, &core->position,
                                  core->unit_id, is_positive, type_25,
                                  &node, &replaced, &needs_reassign) ||
        replaced || needs_reassign || !node) {
        ctx->failed = 1;
        return NULL;
    }
    ctx->live->select_2c = is_positive;
    ctx->live->select_30 = type_25;
    ctx->live->owner_node = node;
    return node;
}

static void high_visibility_leave(void *opaque, AoeLiveUnitCore *core,
                                  int alternate) {
    HighRootContext *ctx = opaque;
    if (core != &ctx->live->core ||
        !aoe_live_unit_vision_change_ror(core, ctx->vision, 0, alternate))
        ctx->failed = 1;
}

static void high_visibility_enter(void *opaque, AoeLiveUnitCore *core,
                                  int alternate) {
    HighRootContext *ctx = opaque;
    if (core != &ctx->live->core ||
        !aoe_live_unit_vision_change_ror(core, ctx->vision, 1, alternate))
        ctx->failed = 1;
}

static void high_mode_two_effect(void *opaque, AoeLiveUnitCore *core) {
    HighRootContext *ctx = opaque;
    if (core != &ctx->live->core || !ctx->live_flags ||
        !aoe_visibility_watch_add(&ctx->owner->visibility_watch40,
                                  ctx->vision->owner_index,
                                  ctx->vision->owner_index, &core->position,
                                  ctx->live_flags, core->unit_id))
        ctx->failed = 1;
}

static void high_pre_terminal(void *opaque, AoeLiveUnitCore *core) {
    HighRootContext *ctx = opaque;
    if (core != &ctx->live->core ||
        !aoe_visibility_watch_remove(&ctx->owner->visibility_watch40,
                                     &core->position))
        ctx->failed = 1;
}

int aoe_live_unit_scenario_high_root_ror(
    AoeLiveUnit *unit, AoePlayerResources *owner_resources,
    AoePlayerUpdateState *owner, AoeOwnedVisibilityRegistry *world,
    AoeLiveUnitVisionBinding *vision) {
    HighRootContext context;
    AoeLiveUnitRootHighStateOps ops;
    uint8_t type;
    int32_t tile_x, tile_y;
    if (!unit || !unit->constructed || !unit->core.unit_template ||
        !owner_resources || !owner || !world || !vision)
        return 0;
    type = unit->core.unit_template->type;
    if (type != 10 && type != 20 && type != 25 && type != 30 && type != 90)
        return 0;
    memset(&context, 0, sizeof(context));
    context.live = unit;
    context.owner = owner;
    context.world = world;
    context.vision = vision;
    if (!vision->owner_grid || !vision->owner_grid->tile_masks ||
        !vision->owner_grid->width || !vision->owner_grid->height)
        return 0;
    tile_x = (int32_t)unit->core.position.x38;
    tile_y = (int32_t)unit->core.position.y3c;
    if (tile_x < 0 || tile_y < 0 ||
        (uint32_t)tile_x >= vision->owner_grid->width ||
        (uint32_t)tile_y >= vision->owner_grid->height)
        return 0;
    context.live_flags = &vision->owner_grid->tile_masks[
        (size_t)tile_y * vision->owner_grid->width + (uint32_t)tile_x];
    /* For the remaining +0xa5/+0xa6 modes, nonzero high mask bits dispatch
       per-player methods not yet translated. Mode 2 with no high bits is
       the exact watch-add/watch-remove path used by the campaign trees. */
    if ((unit->core.unit_template->state_notify_mode ||
         unit->core.unit_template->state_effect_mode) &&
        (unit->core.unit_template->state_effect_mode != 2 ||
         (*context.live_flags >> 16) != 0))
        return 0;
    memset(&ops, 0, sizeof(ops));
    ops.state.context = &context;
    ops.state.owner_resources = owner_resources;
    ops.state.edition = AOE_PLAYER_RISE_OF_ROME;
    ops.state.has_map_tile = unit->placement.tile != NULL;
    ops.state.registration_node = (void **)&unit->owner_node;
    ops.state.registration = (AoeLiveUnitRegistrationHooks){
        &context, high_remove, high_register};
    ops.state.visibility_leave = high_visibility_leave;
    ops.state.visibility_enter = high_visibility_enter;
    if (unit->core.unit_template->state_effect_mode == 2) {
        ops.state.mode_two_effect = high_mode_two_effect;
        ops.state.pre_terminal = high_pre_terminal;
    }
    if (!aoe_live_unit_root_high_state_ror(&unit->core, &ops) ||
        context.failed)
        return 0;
    return 1;
}
