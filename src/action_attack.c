#include "action_attack.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "aoe_ftol.h"

static int ops_ok(const AoeAttackActionOps *o) {
    return o && o->move.resolve && o->move.target_released &&
           o->move.target_attached && o->move.clear_secondary &&
           o->move.unit_notice && o->move.select_graphic && o->move.speed &&
           o->can_attack && o->distance_to_point && o->distance_to_target &&
           o->group_holds && o->group_clear_target && o->owner_id &&
           o->refuses_owner && o->visibility && o->attached && o->ready &&
           o->reset_reload && o->sprite_done && o->sprite_reached &&
           o->sprite_frames && o->sprite_flags && o->fire_target &&
           o->fire_point && o->start_move && o->sub_update && o->sub_clear &&
           o->tick_seconds;
}

static void notice(AoeAttackActionRor *a, const AoeAttackActionOps *ops,
                   int32_t code, int32_t value, int32_t argument) {
    if (!a->base.quiet)
        ops->move.unit_notice(ops->move.context, a->base.unit, code, value,
                              argument);
}

static void init_common(AoeAttackActionRor *a, AoeLiveUnit *unit,
                        const AoeAttackParamsRor *p,
                        const AoeAttackParamsRor *d, float default_range) {
    memset(a, 0, sizeof(*a));
    a->base.type = 9;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->move_graphic = p->move_graphic ? p->move_graphic : d->move_graphic;
    a->attack_graphic =
        p->attack_graphic ? p->attack_graphic : d->attack_graphic;
    a->idle_graphic = p->idle_graphic ? p->idle_graphic : d->idle_graphic;
    a->range = p->range;
    /* fcomp 0.0; test ah,0x40: equal or unordered. */
    if (a->range == 0.0f || isnan(a->range)) {
        a->range = default_range;
        if (a->range == 0.0f || isnan(a->range)) a->range = 0.4f;
    }
    a->min_range = p->min_range;
    a->projectile_id = p->projectile_id;
    a->fire_frame = p->fire_frame;
}

int aoe_attack_action_init_target_ror(AoeAttackActionRor *a,
                                      AoeLiveUnit *unit, AoeLiveUnit *target,
                                      const AoeAttackParamsRor *params,
                                      const AoeAttackParamsRor *defaults,
                                      float default_range,
                                      const AoeAttackActionOps *ops) {
    if (!a || !unit || !params || !defaults || !ops_ok(ops)) return 0;
    init_common(a, unit, params, defaults, default_range);
    if (!aoe_action_set_target_ror(&a->base, target, &ops->move)) return 0;
    a->base.x = a->base.y = a->base.z = -1.0f;
    a->friendly = (uint8_t)(target && ops->owner_id(ops->move.context,
                                                     target) ==
                                          ops->owner_id(ops->move.context,
                                                        unit));
    return 1;
}

int aoe_attack_action_init_point_ror(AoeAttackActionRor *a,
                                     AoeLiveUnit *unit, float x, float y,
                                     float z,
                                     const AoeAttackParamsRor *params,
                                     const AoeAttackParamsRor *defaults,
                                     float default_range) {
    if (!a || !unit || !params || !defaults) return 0;
    init_common(a, unit, params, defaults, default_range);
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    a->approach_left = 2;
    return 1;
}

static int is_tower(const AoeLiveUnit *unit) {
    int16_t id = unit->core.unit_template ? unit->core.unit_template->id : -1;
    return id == 79 || id == 199 || id == 69 || id == 278;
}

static int stopped_still(AoeAttackActionRor *a,
                         const AoeAttackActionOps *ops) {
    float speed = ops->move.speed(ops->move.context, a->base.unit);
    return speed == 0.0f || isnan(speed) ||
           ops->group_holds(ops->move.context, a->base.unit);
}

/* 0x00401bdc / 0x00401e7c tails after "too close". */
static int too_close(AoeAttackActionRor *a, int32_t *where,
                     const AoeAttackActionOps *ops) {
    *where = 0;
    notice(a, ops, 0x1fe, 0, 0);
    if (stopped_still(a, ops))
        return aoe_attack_action_set_state_ror(a, 0x0d, ops) ? 0 : -1;
    return 0;
}

/* 0x00401c5c / 0x00401edd tails after "too far". */
static int too_far(AoeAttackActionRor *a, int32_t *where,
                   const AoeAttackActionOps *ops) {
    *where = 2;
    notice(a, ops, 0x1fd, 0, 0);
    if (!aoe_attack_action_set_state_ror(a, stopped_still(a, ops) ? 0x0d : 4,
                                         ops))
        return -1;
    return 0;
}

static int in_range(AoeAttackActionRor *a, int32_t *where,
                    const AoeAttackActionOps *ops) {
    *where = 1;
    return ops->ready(ops->move.context, a->base.unit) != 0;
}

static double reach(const AoeAttackActionRor *a) {
    const DatUnit *t = a->base.unit->core.unit_template;
    double sx = t->size[0], sy = t->size[1];
    return sqrt(sx * sx + sy * sy) + a->range + (double)0.3f;
}

int aoe_attack_action_check_range_ror(AoeAttackActionRor *a, int32_t *where,
                                      const AoeAttackActionOps *ops) {
    AoeLiveUnit *t;
    double d;
    void *c;
    if (!a || !where || !ops_ok(ops) || !a->base.unit ||
        !a->base.unit->core.unit_template)
        return -1;
    c = ops->move.context;
    t = a->base.target;
    if (!ops->can_attack(c, a->base.unit, t, a->base.x, a->base.y)) return 0;
    if (!t) {
        d = ops->distance_to_point(c, a->base.unit, a->base.x, a->base.y,
                                   a->base.z);
        if (!(d >= a->min_range)) return too_close(a, where, ops);
        if (!(d > reach(a))) return in_range(a, where, ops);
        return too_far(a, where, ops);
    }
    if (!t->core.unit_template) return -1;
    /* Visibility, except for Gaia-owned targets and class 3 / 27 ones
       (template +0x14 is the DAT class per loader 0x004413b0). */
    if (ops->owner_id(c, t) > 0 && t->core.unit_template->unit_class != 3 &&
        t->core.unit_template->unit_class != 0x1b) {
        uint8_t v = ops->visibility(c, a->base.unit,
                                    aoe_ftol32(t->core.position.x38),
                                    aoe_ftol32(t->core.position.y3c));
        if (v == 0 || (v == 0x80 && t->core.unit_template->draw_class == 0))
            return aoe_attack_action_set_state_ror(a, 0x0d, ops) ? 0 : -1;
    }
    {
        const AoeVisibilityUnitRef *p = &t->core.position;
        double dx;
        float dyf, dzf;
        if (p->x38 == a->base.x && p->y3c == a->base.y &&
            p->z40 == a->base.z)
            return in_range(a, where, ops);
        dx = (double)p->x38 - a->base.x;
        dyf = (float)((double)p->y3c - a->base.y);
        dzf = (float)((double)p->z40 - a->base.z);
        if (dx >= -0.5 && dx <= 0.5 && dyf >= -0.5f && dyf <= 0.5f &&
            dzf >= -0.5f && dzf <= 0.5f)
            return in_range(a, where, ops);
        d = ops->distance_to_target(c, a->base.unit, t);
        if (!(d >= a->min_range)) return too_close(a, where, ops);
        if (d > reach(a)) return too_far(a, where, ops);
        a->base.x = p->x38;
        a->base.y = p->y3c;
        a->base.z = p->z40;
        return in_range(a, where, ops);
    }
}

int aoe_attack_action_set_state_ror(AoeAttackActionRor *a, uint8_t state,
                                    const AoeAttackActionOps *ops) {
    void *c;
    if (!a || !ops_ok(ops)) return 0;
    c = ops->move.context;
    ops->sub_clear(c, a);
    a->base.state = state;
    switch (state) {
    case 1: case 0x0d: case 0x0e:
        if (is_tower(a->base.unit)) ops->group_clear_target(c, a->base.unit);
        ops->move.select_graphic(c, a->base.unit, a->idle_graphic);
        return 1;
    case 2:
        a->approach_left = 0;
        return 1;
    case 4:
        if (a->approach_left > 0) --a->approach_left;
        if (!ops->start_move(c, a)) {
            notice(a, ops, 0x1f9, 0x262, 0);
            return aoe_attack_action_set_state_ror(a, 1, ops);
        }
        a->timer = 0.0f;
        return 1;
    case 6: {
        int32_t where = -1;
        int r;
        if (a->approach_left > 0 && a->base.target) a->approach_left = 0;
        r = aoe_attack_action_check_range_ror(a, &where, ops);
        if (r < 0) return 0;
        if (r) {
            ops->reset_reload(c, a->base.unit);
            return aoe_attack_action_set_state_ror(a, 7, ops);
        }
        ops->move.select_graphic(c, a->base.unit,
                                 a->projectile_id == -1 ? a->attack_graphic
                                                        : a->idle_graphic);
        return 1;
    }
    case 7:
        ops->move.select_graphic(c, a->base.unit, a->attack_graphic);
        a->firing = 1;
        return 1;
    default:
        return 1;
    }
}

int aoe_attack_action_activate_ror(AoeAttackActionRor *a,
                                   const AoeAttackActionOps *ops) {
    float speed;
    int32_t where = -1;
    int r;
    if (!a || !ops_ok(ops)) return 0;
    speed = ops->move.speed(ops->move.context, a->base.unit);
    if (!(speed == 0.0f || isnan(speed)) &&
        !ops->group_holds(ops->move.context, a->base.unit))
        return aoe_attack_action_set_state_ror(a, 4, ops);
    r = aoe_attack_action_check_range_ror(a, &where, ops);
    if (r < 0) return 0;
    return aoe_attack_action_set_state_ror(a, (r || where == 1) ? 6 : 0x0d,
                                           ops);
}

int aoe_attack_action_retarget_ror(AoeAttackActionRor *a, AoeLiveUnit *target,
                                   const AoeAttackActionOps *ops) {
    if (!a || !ops_ok(ops) || !target) return 0;
    if (target == a->base.target &&
        (a->base.state == 4 || a->base.state == 6 || a->base.state == 7))
        return 1;
    a->approach_left = 0;
    if (!aoe_action_set_target_ror(&a->base, target, &ops->move)) return 0;
    return aoe_attack_action_set_state_ror(a, 4, ops);
}

int aoe_attack_action_stop_ror(AoeAttackActionRor *a,
                               const AoeAttackActionOps *ops) {
    return aoe_attack_action_set_state_ror(a, 1, ops);
}

/* 0x0040190b: unless quiet, notice 0x1fb with the current target ID. */
static int lost(AoeAttackActionRor *a, const AoeAttackActionOps *ops,
                uint8_t *result) {
    notice(a, ops, 0x1fb, 0x258, a->base.target_id);
    *result = 4;
    return 1;
}

static int gone(AoeAttackActionRor *a, const AoeAttackActionOps *ops,
                uint8_t *result) {
    if (!aoe_action_set_target_ror(&a->base, NULL, &ops->move) ||
        !aoe_attack_action_set_state_ror(a, 2, ops))
        return 0;
    *result = 5;
    return 1;
}

/* 0x00401945 / state 6 of the update. */
static int try_fire_setup(AoeAttackActionRor *a,
                          const AoeAttackActionOps *ops) {
    int32_t where = -1;
    int r = aoe_attack_action_check_range_ror(a, &where, ops);
    if (r < 0) return 0;
    if (r) {
        ops->reset_reload(ops->move.context, a->base.unit);
        return aoe_attack_action_set_state_ror(a, 7, ops);
    }
    return 1;
}

static int update_firing(AoeAttackActionRor *a,
                         const AoeAttackActionOps *ops) {
    void *c = ops->move.context;
    AoeLiveUnit *u = a->base.unit;
    if (!a->firing) {
        if (ops->ready(c, u) == 1) {
            if (!a->base.target && !a->approach_left)
                return aoe_attack_action_set_state_ror(a, 2, ops);
            ops->reset_reload(c, u);
            if (!aoe_attack_action_set_state_ror(a, 6, ops)) return 0;
            if (a->base.state == 0x0d || a->base.state == 4) return 1;
            if (!aoe_attack_action_set_state_ror(a, 7, ops)) return 0;
        }
        if (!a->firing) {
            if (ops->sprite_done(c, u) == 1 || !(ops->sprite_flags(c, u) & 1)) {
                if (!a->base.target && !a->approach_left)
                    return aoe_attack_action_set_state_ror(a, 2, ops);
                return aoe_attack_action_set_state_ror(a, 6, ops);
            }
            return 1;
        }
    }
    {
        /* The frame is passed as a dword whose high half the original does
           not clear; only the 16-bit value is reproduced. */
        int32_t frame = a->fire_frame;
        if (!frame)
            frame = aoe_ftol32((double)ops->sprite_frames(c, u) * 0.5);
        if (ops->sprite_reached(c, u, frame)) {
            if (a->base.target)
                ops->fire_target(c, u, a->base.target);
            else
                ops->fire_point(c, u, a->base.x, a->base.y, a->base.z);
            a->firing = 0;
        }
    }
    return 1;
}

int aoe_attack_action_update_ror(AoeAttackActionRor *a,
                                 const AoeAttackActionOps *ops,
                                 uint8_t *result) {
    void *c;
    AoeLiveUnit *t;
    if (!a || !result || !ops_ok(ops) || !a->base.unit) return 0;
    c = ops->move.context;
    *result = 0;
    a->timer = (float)((double)ops->tick_seconds(c) + a->timer);
    if (a->base.target_id != -1 &&
        !ops->move.resolve(c, a->base.target_id))
        return gone(a, ops, result);
    if (a->base.secondary_id != -1 &&
        !ops->move.resolve(c, a->base.secondary_id))
        ops->move.clear_secondary(c, &a->base);
    t = a->base.target;
    if (t) {
        int16_t own;
        if (t->core.state >= 3) return gone(a, ops, result);
        own = ops->owner_id(c, a->base.unit);
        if (own != 0 && ops->refuses_owner(c, a->base.unit,
                                           ops->owner_id(c, t))) {
            if (!aoe_attack_action_set_state_ror(a, 1, ops)) return 0;
            return lost(a, ops, result);
        }
        if (ops->attached(c, t)) {
            if (!aoe_attack_action_set_state_ror(a, 1, ops)) return 0;
            return lost(a, ops, result);
        }
    }
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 2:
        if (ops->sprite_done(c, a->base.unit) != 1) return 1;
        if (!aoe_action_set_target_ror(&a->base, NULL, &ops->move)) return 0;
        notice(a, ops, 0x1fa, 0x258, 0);
        if (!aoe_attack_action_set_state_ror(a, 1, ops)) return 0;
        *result = a->base.quiet == 1 ? 5 : 2;
        return 1;
    case 4:
        switch (ops->sub_update(c, a)) {
        case 1: case 2:
            return aoe_attack_action_set_state_ror(a, 6, ops);
        case 3:
            if (!aoe_attack_action_set_state_ror(a, 0x0d, ops)) return 0;
            notice(a, ops, 0x1f9, 0x258, 0);
            *result = 3;
            return 1;
        case 4:
            if (!aoe_attack_action_set_state_ror(a, 0x0e, ops)) return 0;
            return lost(a, ops, result);
        default:
            return 1;
        }
    case 6:
        return try_fire_setup(a, ops);
    case 7:
        return update_firing(a, ops);
    case 0x0d:
        notice(a, ops, 0x1f9, 0x258, 0);
        *result = 3;
        return 1;
    default:
        return 1;
    }
}

/* 0x00402026 / 0x00402064: the task's +0x38, then +0x3c, then the old
   template graphic. */
static const void *swap_one(const void *g,
                            const AoeAttackTaskGraphicsRor *old_task,
                            const AoeAttackTaskGraphicsRor *new_task,
                            const void *old_t, const void *new_t) {
    if (old_task && new_task) {
        if (g == old_task->proceed) return new_task->proceed;
        if (g == old_task->work) return new_task->work;
    }
    return g == old_t ? new_t : g;
}

void aoe_attack_action_swap_graphics_ror(
    AoeAttackActionRor *a, const AoeAttackTemplateGraphicsRor *old_t,
    const AoeAttackTemplateGraphicsRor *new_t,
    const AoeAttackTaskGraphicsRor *old_task,
    const AoeAttackTaskGraphicsRor *new_task) {
    int tasks;
    if (!a || !old_t || !new_t) return;
    tasks = old_task && new_task;
    if (a->move_graphic) {
        if (tasks && a->move_graphic == old_task->move)
            a->move_graphic = new_task->move;
        else if (a->move_graphic == old_t->walking)
            a->move_graphic = new_t->walking;
    }
    if (a->attack_graphic)
        a->attack_graphic = swap_one(a->attack_graphic, old_task, new_task,
                                     old_t->attack, new_t->attack);
    if (a->idle_graphic)
        a->idle_graphic = swap_one(a->idle_graphic, old_task, new_task,
                                   old_t->standing, new_t->standing);
}
