#include "live_unit_state.h"

#include <limits.h>
#include <math.h>

static int root_state_set(AoeLiveUnitCore *unit, uint8_t new_state,
                          const AoeLiveUnitRootStateOps *ops,
                          int rise_of_rome) {
    uint8_t old_state;
    int leaving_active, pre_terminal, vision_departure, visible_range;
    int state_three_actions, health_reclassify;
    if (!unit || !unit->unit_template || !ops) return 0;
    old_state = unit->state;
    if (new_state == old_state ||
        (rise_of_rome && new_state < old_state && new_state != 0))
        return 1;
    leaving_active = new_state > 2 && old_state < 3;
    pre_terminal = new_state > 6 && old_state < 4;
    vision_departure = new_state > 2 && old_state == 2 && ops->has_map_tile;
    visible_range = unit->unit_template->line_of_sight > 0.0f;
    state_three_actions = new_state == 3 &&
        ops->current_action == ops->default_action &&
        ops->has_action_list && ops->action_count > 1;
    health_reclassify = unit->positive_hit_points &&
        new_state != 2 && new_state != 6 &&
        !unit->registration_pending;
    /* Missing original-method implementations cannot be treated as no-ops. */
    if ((leaving_active && (!ops->owner_resources ||
         (ops->edition != AOE_PLAYER_BASE &&
          ops->edition != AOE_PLAYER_RISE_OF_ROME))) ||
        (rise_of_rome && leaving_active &&
         unit->unit_template->state_effect_mode == 2 &&
         !ops->mode_two_effect) ||
        (pre_terminal &&
         (unit->unit_template->state_notify_mode ||
          unit->unit_template->state_effect_mode) &&
         !ops->pre_terminal) ||
        (vision_departure && !ops->visibility_leave) ||
        (vision_departure && visible_range && !ops->visibility_enter) ||
        (new_state == 7 && old_state < 7 && ops->has_map_tile &&
         visible_range && !ops->visibility_leave) ||
        (state_three_actions && !ops->state_three_actions) ||
        (health_reclassify && (!ops->registration_node ||
         !ops->registration.remove || !ops->registration.register_unit)))
        return 0;
    if (!rise_of_rome && pre_terminal && ops->pre_terminal)
        ops->pre_terminal(ops->context, unit);
    if (leaving_active) {
        if (!aoe_unit_storage_exit_active(unit->unit_template,
                                           ops->owner_resources,
                                           ops->edition)) return 0;
        if (rise_of_rome && unit->unit_template->state_effect_mode == 2)
            ops->mode_two_effect(ops->context, unit);
    }
    if (rise_of_rome && pre_terminal && ops->pre_terminal)
        ops->pre_terminal(ops->context, unit);
    if (vision_departure) {
        ops->visibility_leave(ops->context, unit, 0);
        if (visible_range) ops->visibility_enter(ops->context, unit, 1);
    }
    if (new_state == 7 && old_state < 7 && ops->has_map_tile &&
        visible_range)
        ops->visibility_leave(ops->context, unit, 1);
    unit->state = new_state;
    if (state_three_actions) ops->state_three_actions(ops->context, unit);
    if (unit->positive_hit_points && new_state != 2 && new_state != 6)
        return aoe_live_unit_health_flag_set(unit, 0,
                                              ops->registration_node,
                                              &ops->registration);
    return 1;
}

int aoe_live_unit_root_state_set_ror(AoeLiveUnitCore *unit,
                                      uint8_t new_state,
                                      const AoeLiveUnitRootStateOps *ops) {
    return root_state_set(unit, new_state, ops, 1);
}

int aoe_live_unit_root_state_set_base(AoeLiveUnitCore *unit,
                                       uint8_t new_state,
                                       const AoeLiveUnitRootStateOps *ops) {
    return root_state_set(unit, new_state, ops, 0);
}

int aoe_live_unit_low_state_ror(AoeLiveUnitCore *unit,
                                const DatGraphic *current_graphic,
                                float rotation) {
    int32_t frame;
    if (!unit || !unit->unit_template) return 0;
    switch (unit->unit_template->type) {
    case 10: case 20: case 25: case 90:
        return 1; /* 0x00407370 */
    case 30: case 40: case 50: case 60: case 70: case 80:
        if (!isfinite(rotation) || rotation < (float)INT32_MIN ||
            rotation >= 2147483648.0f) return 0;
        unit->scenario_rotation = rotation; /* 0x0044ca40 +0x9c */
        /* 0x0044c7b0: for a current graphic with sequence flag 2 and a
           positive angle count (+0x60), facet =
           (rotation - pi/4) * angles * (1/2pi) - (-0.5), plus angles when
           below 0.0, truncated (0x005283e0) and stored as a byte. The
           constants are the floats at 0x00549920 (pi/4), 0x00549910
           (1/2pi), 0x00549918 (-0.5) and 0x005498e0 (0.0); double stands
           in for the x87 intermediates. */
        if (current_graphic && (current_graphic->sequence_type & 2) &&
            current_graphic->angle_count > 0) {
            double angles = (double)(float)current_graphic->angle_count;
            double v = ((double)rotation - (double)0.7853981852531433f) *
                           angles * (double)0.15915493667125702f -
                       (double)-0.5f;
            if (v < 0.0) v += angles;
            if (!(v > -9.2e18 && v < 9.2e18)) return 0;
            frame = (int32_t)(int64_t)v;
            unit->scenario_frame = (uint8_t)frame;
        } else {
            unit->scenario_frame = 0;
        }
        return 1;
    default:
        return 0;
    }
}

int aoe_live_unit_root_high_state_ror(
    AoeLiveUnitCore *unit, const AoeLiveUnitRootHighStateOps *ops) {
    if (!unit || !unit->unit_template || !ops) return 0;
    if (unit->state >= 7) return 1;
    if (ops->parent_high_state)
        ops->parent_high_state(ops->context, unit);
    if ((unit->action_flags_36 & 1) && !ops->clear_active_action) return 0;
    if (unit->positive_hit_points && !unit->registration_pending &&
        (!ops->state.registration_node || !ops->state.registration.remove ||
         !ops->state.registration.register_unit)) return 0;
    if (unit->action_flags_36 & 1)
        ops->clear_active_action(ops->context, unit);
    if (!aoe_live_unit_health_flag_set(unit, 0,
                                       ops->state.registration_node,
                                       &ops->state.registration)) return 0;
    return aoe_live_unit_root_state_set_ror(unit, 7, &ops->state);
}
