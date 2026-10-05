#include "action_build.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

static int ops_ok(const AoeBuildActionOps *o) {
    return o && o->move.resolve && o->move.target_released &&
           o->move.target_attached && o->move.unit_notice &&
           o->move.select_graphic && o->move.clear_secondary && o->graphic &&
           o->tick_seconds && o->owner_kind && o->face && o->wall_at &&
           o->add_build_work && o->reset_group && o->request_repair &&
           o->order_onto && o->play_sound && o->start_move &&
           o->sub_update && o->sub_clear;
}

static void notice(AoeBuildActionRor *a, const AoeBuildActionOps *o,
                   int32_t code) {
    o->move.unit_notice(o->move.context, a->base.unit, code, 0x25a, 0);
}

static void show(AoeBuildActionRor *a, const AoeBuildActionOps *o,
                 int16_t id) {
    o->move.select_graphic(o->move.context, a->base.unit,
                           o->graphic(o->move.context, id));
}

static int set_target(AoeBuildActionRor *a, AoeLiveUnit *t,
                      const AoeBuildActionOps *o) {
    return aoe_action_set_target_ror(&a->base, t, &o->move);
}

int aoe_build_action_init_target_ror(AoeBuildActionRor *a, AoeLiveUnit *unit,
                                     const DatUnitTask *task,
                                     AoeLiveUnit *target,
                                     const AoeBuildActionOps *o) {
    if (!a || !unit || !task || !target || !ops_ok(o)) return 0;
    memset(a, 0, sizeof(*a));
    a->base.type = 0x65;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    if (target->runtime_owner_index == unit->runtime_owner_index) {
        if (!set_target(a, target, o)) return 0;
        a->base.x = target->core.position.x38;
        a->base.y = target->core.position.y3c;
        a->base.z = target->core.position.z40;
    }
    return 1;
}

int aoe_build_action_init_point_ror(AoeBuildActionRor *a, AoeLiveUnit *unit,
                                    const DatUnitTask *task, float x,
                                    float y, float z) {
    if (!a || !unit || !task) return 0;
    memset(a, 0, sizeof(*a));
    a->base.type = 0x65;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    return 1;
}

/* State 7 entry (0x004b16a8): diagonal to the foundation tile, two walls
   of the unit's owner on the neighbouring tiles block the work. */
static int blocked_by_walls(AoeBuildActionRor *a, const AoeBuildActionOps *o) {
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    const DatUnit *tt = t->core.unit_template;
    int32_t ux = aoe_ftol32(u->core.position.x38);
    int32_t uy = aoe_ftol32(u->core.position.y3c);
    int32_t tx = aoe_ftol32(t->core.position.x38);
    int32_t ty = aoe_ftol32(t->core.position.y3c);
    int32_t dx, dy, x1, y1, x2, y2;
    if (tt && (float)aoe_ftol32(tt->size[0]) == tt->size[0] &&
        (float)aoe_ftol32(tt->size[1]) == tt->size[1]) {
        /* Whole-tile footprints: step back across the shared corner. */
        if ((ty > uy && tx < ux) || (ty < uy && tx > ux)) --ty;
    }
    dx = ux - tx;
    dy = uy - ty;
    if (abs(dx) != abs(dy)) return 0;
    if (dx > 0 && dy > 0) {
        x1 = ux; y1 = uy - 1; x2 = ux - 1; y2 = uy;
    } else if (dx > 0 || (dx == 0 && dy == 0) || (dx < 0 && dy == 0)) {
        x1 = ux - 1; y1 = uy; x2 = ux; y2 = uy + 1;
    } else if (dy < 0) {
        x1 = ux + 1; y1 = uy; x2 = ux; y2 = uy + 1;
    } else {
        x1 = ux; y1 = uy - 1; x2 = ux + 1; y2 = uy;
    }
    return o->wall_at(o->move.context, u, x1, y1) &&
           o->wall_at(o->move.context, u, x2, y2);
}

int aoe_build_action_set_state_ror(AoeBuildActionRor *a, uint8_t state,
                                   const AoeBuildActionOps *o) {
    void *c;
    AoeLiveUnit *u;
    if (!a || !a->base.unit || !a->task || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    o->sub_clear(c, a);
    a->base.state = state;
    switch (state) {
    case 1: case 0x0d:
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 2:
        show(a, o, u->core.unit_template->standing_graphic);
        if (!set_target(a, NULL, o)) return 0;
        o->move.clear_secondary(c, &a->base);
        a->base.x = a->base.y = a->base.z = -1.0f;
        return 1;
    case 3:
        if (o->owner_kind(c, u) == 1) {
            notice(a, o, 0x1fa);
            if (!aoe_build_action_set_state_ror(a, 1, o)) return 0;
        } else {
            notice(a, o, 0x202);
        }
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 4:
        if (a->base.target) {
            a->base.x = a->base.target->core.position.x38;
            a->base.y = a->base.target->core.position.y3c;
            a->base.z = a->base.target->core.position.z40;
        }
        if (!o->start_move(c, a, a->base.target, a->task->work_range,
                           o->graphic(c, a->task->moving_graphic), 0))
            return aoe_build_action_set_state_ror(a, 0x0d, o);
        return 1;
    case 6:
        show(a, o, a->task->moving_graphic);
        return 1;
    case 7:
        if (!a->base.target || blocked_by_walls(a, o))
            return aoe_build_action_set_state_ror(a, 0x0d, o);
        show(a, o, a->task->proceeding_graphic);
        if (a->task->gathering_sound >= 0)
            o->play_sound(c, a->task->gathering_sound);
        return 1;
    case 0x0b:
        if (!o->start_move(c, a, NULL, 0.0f,
                           o->graphic(c, a->task->moving_graphic), 1))
            return aoe_build_action_set_state_ror(a, 0x0d, o);
        return 1;
    default:
        return 1;
    }
}

int aoe_build_action_activate_ror(AoeBuildActionRor *a, int arg,
                                  const AoeBuildActionOps *o) {
    if (!a) return 0;
    return aoe_build_action_set_state_ror(a, arg && a->base.target ? 4 : 3,
                                          o);
}

int aoe_build_action_walk_ror(AoeBuildActionRor *a, float x, float y,
                              float z, const AoeBuildActionOps *o) {
    if (!a || !ops_ok(o)) return 0;
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    if (!set_target(a, NULL, o)) return 0;
    return aoe_build_action_set_state_ror(a, 0x0b, o);
}

int aoe_build_action_retarget_ror(AoeBuildActionRor *a, AoeLiveUnit *t,
                                  float x, float y, float z,
                                  const AoeBuildActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    if (!t || t->runtime_owner_index != a->base.unit->runtime_owner_index)
        return aoe_build_action_walk_ror(a, x, y, z, o);
    if (t == a->base.target &&
        (a->base.state == 4 || a->base.state == 6))
        return 1;
    if (!set_target(a, t, o)) return 0;
    return aoe_build_action_set_state_ror(a, 4, o);
}

/* State 7 (0x004b1a42). */
static int work(AoeBuildActionRor *a, const AoeBuildActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    float amount;
    if (a->timer < 0.0f && t && t->core.storage.carried_amount > 0.0f) {
        /* A finished farm: go and farm it. */
        o->reset_group(c, u);
        o->order_onto(c, u, t);
        return 1;
    }
    amount = o->instant_build_option
                 ? 1000.0f
                 : (float)((double)o->tick_seconds(c) *
                           u->core.unit_template->work_rate *
                           a->task->work_value1);
    if (t) {
        int completed = 0;
        if (!o->add_build_work(c, t, amount, &completed)) return 0;
        if (!completed) return 1;
        if (t->core.unit_template && t->core.unit_template->id == 0x32) {
            a->timer = -1.0f;
            return 1;
        }
    }
    return aoe_build_action_set_state_ror(a, 3, o);
}

int aoe_build_action_update_ror(AoeBuildActionRor *a,
                                const AoeBuildActionOps *o,
                                uint8_t *result) {
    void *c;
    AoeLiveUnit *t;
    if (!a || !a->base.unit || !a->task || !result || !ops_ok(o)) return 0;
    c = o->move.context;
    *result = 0;
    if (a->base.target_id != -1 && !o->move.resolve(c, a->base.target_id)) {
        if (!set_target(a, NULL, o)) return 0;
        if (a->base.state != 2) return aoe_build_action_set_state_ror(a, 3, o);
    }
    if (a->base.secondary_id != -1 &&
        !o->move.resolve(c, a->base.secondary_id))
        o->move.clear_secondary(c, &a->base);
    t = a->base.target;
    if (t && t->core.state != 0 && a->timer == 0.0f) {
        /* Finished: repair a damaged building, else let it go. */
        if ((double)(t->core.unit_template
                         ? t->core.unit_template->hit_points
                         : 0) > t->core.hit_points) {
            o->reset_group(c, a->base.unit);
            o->request_repair(c, a->base.unit, t);
            return 1;
        }
        if (!set_target(a, NULL, o)) return 0;
        if (a->base.state != 2) return aoe_build_action_set_state_ror(a, 3, o);
    }
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 4:
        switch (o->sub_update(c, a)) {
        case 1: case 2:
            return aoe_build_action_set_state_ror(a, 6, o);
        case 3:
            return aoe_build_action_set_state_ror(a, 3, o);
        case 4: case 5:
            if (!set_target(a, NULL, o)) return 0;
            return aoe_build_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 6:
        if (o->face(c, a->base.unit, a->base.target))
            return aoe_build_action_set_state_ror(a, 7, o);
        return 1;
    case 7:
        return work(a, o);
    case 0x0b:
        switch (o->sub_update(c, a)) {
        case 1: case 2: case 3: case 5:
            return aoe_build_action_set_state_ror(a, 3, o);
        case 4:
            if (!set_target(a, NULL, o)) return 0;
            return aoe_build_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 0x0d:
        notice(a, o, 0x1f9);
        if (!aoe_build_action_set_state_ror(a, 2, o)) return 0;
        *result = 3;
        return 1;
    default:
        return 1;
    }
}
