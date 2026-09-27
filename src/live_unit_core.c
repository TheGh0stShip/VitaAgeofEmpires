#include "live_unit_core.h"

int aoe_live_unit_core_begin(const DatUnit *unit_template,
                             AoePlayerResources *owner_resources,
                             AoePlayerEdition edition,
                             AoeOwnedVisibilityRegistry *registry,
                             float x, float y, float z,
                             AoeLiveUnitCore *out) {
    AoeLiveUnitCore next = {0};
    if (!unit_template || !unit_template->present || !owner_resources ||
        !registry || !out ||
        !aoe_visibility_registry_valid(&registry->view) ||
        (edition != AOE_PLAYER_BASE && edition != AOE_PLAYER_RISE_OF_ROME))
        return 0;
    next.unit_template = unit_template;
    next.class_code = 10;
    next.hit_points = (float)unit_template->hit_points;
    next.positive_hit_points = next.hit_points > 0.0f;
    next.type_25 = unit_template->type == 25;
    next.state = 2;
    if (!aoe_unit_storage_initialize(unit_template, owner_resources,
                                      edition, &next.storage)) return 0;
    if (!aoe_live_unit_allocate_id(unit_template, registry,
                                   &next.unit_id)) return 0;
    next.position = (AoeVisibilityUnitRef){x, y, z};
    *out = next;
    return 1;
}

int aoe_live_unit_core_apply_surface(AoeLiveUnitCore *unit,
                                      const ScnTile *tiles,
                                      uint32_t width, uint32_t height) {
    float surface_height, placed_z;
    if (!unit || !unit->unit_template ||
        !terrain_surface_height_at(tiles, width, height,
                                   unit->position.x38, unit->position.y3c,
                                   &surface_height) ||
        !terrain_unit_surface_z(unit->unit_template, unit->position.z40,
                                surface_height, &placed_z)) return 0;
    unit->position.z40 = placed_z;
    return 1;
}

int aoe_live_unit_health_flag_set(AoeLiveUnitCore *unit, uint8_t is_positive,
                                   void **registration_node,
                                   const AoeLiveUnitRegistrationHooks *hooks) {
    if (!unit || is_positive > 1) return 0;
    if (unit->positive_hit_points == is_positive) return 1;
    if (is_positive) {
        unit->registration_pending = 1;
        unit->positive_hit_points = 1;
        return 1;
    }
    if (unit->registration_pending) {
        unit->positive_hit_points = 0;
        unit->registration_pending = 0;
        return 1;
    }
    if (!registration_node || !hooks || !hooks->remove ||
        !hooks->register_unit) return 0;
    hooks->remove(hooks->context, unit, unit->positive_hit_points,
                  unit->type_25, *registration_node);
    unit->positive_hit_points = 0;
    *registration_node = hooks->register_unit(hooks->context, unit, 0,
                                              unit->type_25);
    return 1;
}

int aoe_live_unit_root_middle_state(AoeLiveUnitCore *unit,
                                     void **registration_node,
                                     const AoeLiveUnitRegistrationHooks *hooks,
                                     const AoeLiveUnitMiddleActionHooks *action) {
    if (!unit ||
        (unit->action_flags_36 & 1 &&
         (!action || !action->clear_active_action)) ||
        (unit->positive_hit_points && !unit->registration_pending &&
         (!registration_node || !hooks || !hooks->remove ||
          !hooks->register_unit))) return 0;
    if (unit->action_flags_36 & 1)
        action->clear_active_action(action->context, unit);
    if (!aoe_live_unit_health_flag_set(unit, 0, registration_node, hooks))
        return 0;
    unit->hit_points = 0.0f;
    return 1;
}
