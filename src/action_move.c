#include "action_move.h"

#include <math.h>
#include <stddef.h>

#include "aoe_ftol.h"

int aoe_action_set_target_ror(AoeMoveActionRor *a, AoeLiveUnit *target,
                              const AoeMoveActionOps *ops) {
    if (!a || !ops || !ops->resolve || !ops->target_released ||
        !ops->target_attached)
        return 0;
    if (a->target && a->target != target) {
        AoeLiveUnit *old = ops->resolve(ops->context, a->target_id);
        if (old) ops->target_released(ops->context, old, a->unit);
    }
    if (a->target != target) {
        a->target = target;
        if (target) {
            a->target_id = target->core.unit_id;
            ops->target_attached(ops->context, target, a->unit, a->type);
        } else {
            a->target_id = -1;
        }
    }
    return 1;
}

static void base_init(AoeMoveActionRor *a, AoeLiveUnit *unit, float range,
                      const void *graphic, const void *default_graphic) {
    a->type = 1;
    a->unit = unit;
    a->state = 0;
    a->target = NULL;
    a->target_id = -1;
    a->secondary_id = -1;
    a->x = a->y = a->z = 0.0f;
    a->quiet = 0;
    a->range = range <= 0.0f ? 0.4f : range;
    a->graphic = graphic ? graphic : default_graphic;
}

int aoe_move_action_init_target_ror(AoeMoveActionRor *a, AoeLiveUnit *unit,
                                    AoeLiveUnit *target, float range,
                                    const void *graphic,
                                    const void *default_graphic,
                                    const AoeMoveActionOps *ops) {
    if (!a || !unit) return 0;
    base_init(a, unit, range, graphic, default_graphic);
    if (!aoe_action_set_target_ror(a, target, ops)) return 0;
    if (target) {
        a->x = target->core.position.x38;
        a->y = target->core.position.y3c;
        a->z = target->core.position.z40;
    }
    return 1;
}

int aoe_move_action_init_point_ror(AoeMoveActionRor *a, AoeLiveUnit *unit,
                                   float x, float y, float z, float range,
                                   const void *graphic,
                                   const void *default_graphic) {
    if (!a || !unit) return 0;
    base_init(a, unit, range, graphic, default_graphic);
    a->x = x;
    a->y = y;
    a->z = z;
    return 1;
}

int aoe_move_action_set_state_ror(AoeMoveActionRor *a, uint8_t state,
                                  const AoeMoveActionOps *ops) {
    if (!a || !ops || !ops->select_graphic || !ops->standing_graphic) return 0;
    a->state = state;
    if (state == 1 || state == 0x0d || state == 0x0e) {
        if (!a->quiet)
            ops->select_graphic(ops->context, a->unit,
                                ops->standing_graphic(ops->context, a->unit));
    } else if (state == 4) {
        ops->select_graphic(ops->context, a->unit, a->graphic);
    }
    return 1;
}

int aoe_move_action_activate_ror(AoeMoveActionRor *a,
                                 const AoeMoveActionOps *ops) {
    float speed;
    if (!a || !ops || !ops->in_range || !ops->speed) return 0;
    if (a->target && ops->in_range(ops->context, a->unit, a->target, a->range))
        return aoe_move_action_set_state_ror(a, 1, ops);
    speed = ops->speed(ops->context, a->unit);
    /* fcomp 0.0; test ah,0x40: equal or unordered. */
    return aoe_move_action_set_state_ror(
        a, (isnan(speed) || speed == 0.0f) ? 0x0d : 3, ops);
}

static int lose_target(AoeMoveActionRor *a, const AoeMoveActionOps *ops,
                       uint8_t *result) {
    if (!aoe_action_set_target_ror(a, NULL, ops)) return 0;
    if (a->quiet == 1) {
        *result = 5;
        return 1;
    }
    /* +0x18 is re-read after the setter reset it (normally -1). */
    ops->unit_notice(ops->context, a->unit, 0x1fb, 0x262, a->target_id);
    *result = 4;
    return 1;
}

/* dx stays unrounded; dy is stored as a float before its comparisons. */
static int drifted(const AoeMoveActionRor *a) {
    double dx = (double)a->x - a->target->core.position.x38;
    float dy = (float)((double)a->y - a->target->core.position.y3c);
    return !(dx >= -1.0) || dx > 1.0 || !(dy >= -1.0f) || dy > 1.0f;
}

static int repath(AoeMoveActionRor *a, const AoeMoveActionOps *ops,
                  uint8_t *result) {
    ops->unit_notice(ops->context, a->unit, 0x1fc, 0, 0);
    if (!aoe_move_action_set_state_ror(a, 3, ops)) return 0;
    /* The original runs its own update twice and discards both results. */
    for (int i = 0; i < 2; ++i) {
        uint8_t ignored;
        if (!aoe_move_action_update_ror(a, ops, &ignored)) return 0;
    }
    *result = 0;
    return 1;
}

static int arrive_near(AoeMoveActionRor *a, const AoeMoveActionOps *ops,
                       uint8_t *result) {
    if (!a->quiet) ops->unit_notice(ops->context, a->unit, 0x1fa, 0x262, 0);
    if (!aoe_move_action_set_state_ror(a, 1, ops)) return 0;
    *result = 2;
    return 1;
}

int aoe_move_action_update_ror(AoeMoveActionRor *a,
                               const AoeMoveActionOps *ops, uint8_t *result) {
    if (!a || !result || !ops || !ops->resolve || !ops->clear_secondary ||
        !ops->unit_notice || !ops->in_range || !ops->order_flag ||
        !ops->set_destination || !ops->waypoints_stale || !ops->record_tile ||
        !ops->set_range || !ops->set_target_id || !ops->set_target_size ||
        !ops->start || !ops->status)
        return 0;
    if (a->target_id != -1 && !ops->resolve(ops->context, a->target_id))
        return lose_target(a, ops, result);
    if (a->secondary_id != -1 && !ops->resolve(ops->context, a->secondary_id))
        ops->clear_secondary(ops->context, a);
    if (a->target && a->target->core.state >= 7)
        return lose_target(a, ops, result);
    *result = 0;
    switch (a->state) {
    case 1:
        *result = 1;
        return 1;
    case 3: {
        uint8_t tile[3];
        if (a->target &&
            ops->in_range(ops->context, a->unit, a->target, a->range)) {
            if (!aoe_move_action_set_state_ror(a, 1, ops)) return 0;
            if (!a->quiet)
                ops->unit_notice(ops->context, a->unit, 0x1fa, 0x262, 0);
            ops->order_flag(ops->context, a->unit, 0);
            a->unit->group_unit_count = 0; /* unit +0x58 */
            *result = 2;
            return 1;
        }
        if (a->target) {
            a->x = a->target->core.position.x38;
            a->y = a->target->core.position.y3c;
            a->z = a->target->core.position.z40;
        }
        ops->set_destination(ops->context, a->unit, a->x, a->y);
        tile[0] = (uint8_t)aoe_ftol32(a->x);
        tile[1] = (uint8_t)aoe_ftol32(a->y);
        tile[2] = (uint8_t)aoe_ftol32(a->z);
        if (!ops->waypoints_stale(ops->context, a->unit))
            ops->record_tile(ops->context, a->unit, tile);
        ops->set_range(ops->context, a->unit, a->range);
        if (a->target) {
            ops->set_target_id(ops->context, a->unit, a->target->core.unit_id);
            ops->set_target_size(ops->context, a->unit,
                                 a->target->core.unit_template
                                     ? a->target->core.unit_template->size[0]
                                     : 0.0f,
                                 a->target->core.unit_template
                                     ? a->target->core.unit_template->size[1]
                                     : 0.0f);
        } else {
            ops->set_target_id(ops->context, a->unit, -1);
            ops->set_target_size(ops->context, a->unit, 0.0f, 0.0f);
        }
        if (!ops->start(ops->context, a->unit) &&
            !aoe_move_action_set_state_ror(a, 4, ops))
            return 0;
        return 1;
    }
    case 4: {
        int status;
        if (a->target && drifted(a)) return repath(a, ops, result);
        status = ops->status(ops->context, a->unit);
        if (status == 0) {
            if (!aoe_move_action_set_state_ror(a, 1, ops)) return 0;
            if (!a->quiet)
                ops->unit_notice(ops->context, a->unit, 0x1f9, 0x262, 0);
            *result = 3;
            return 1;
        }
        if (status == 2) {
            if (a->target && drifted(a)) return repath(a, ops, result);
            return arrive_near(a, ops, result);
        }
        return 1;
    }
    case 0x0d:
        if (!a->quiet)
            ops->unit_notice(ops->context, a->unit, 0x1f9, 0x262, 0);
        *result = 3;
        return 1;
    default:
        return 1;
    }
}
