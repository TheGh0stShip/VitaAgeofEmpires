#include "action_repair.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

enum { NOTICE_VALUE = 0x26a, REPAIRER_TEMPLATE = 0x9c, BEACH = 2 };

static int ops_ok(const AoeRepairActionOps *o) {
    return o && o->move.resolve && o->move.target_released &&
           o->move.target_attached && o->move.unit_notice &&
           o->move.select_graphic && o->move.clear_secondary && o->graphic &&
           o->tick_seconds && o->distance && o->face && o->may_repair &&
           o->repair && o->event && o->play_sound && o->start_move &&
           o->sub_update && o->sub_clear && o->order_serial &&
           o->owner_unit_at && o->head_target && o->head_state &&
           o->head_destination && o->terrain_at && o->fits && o->move_to;
}

static int is_boat(const AoeLiveUnit *u) {
    int16_t c = u && u->core.unit_template ? u->core.unit_template->unit_class
                                           : -1;
    return c == 2 || c == 0x14 || c == 0x15 || c == 0x16;
}

static void notice(AoeRepairActionRor *a, const AoeRepairActionOps *o,
                   int32_t code) {
    o->move.unit_notice(o->move.context, a->base.unit, code, NOTICE_VALUE, 0);
}

static void show(AoeRepairActionRor *a, const AoeRepairActionOps *o,
                 int16_t id) {
    o->move.select_graphic(o->move.context, a->base.unit,
                           o->graphic(o->move.context, id));
}

static int set_target(AoeRepairActionRor *a, AoeLiveUnit *t,
                      const AoeRepairActionOps *o) {
    return aoe_action_set_target_ror(&a->base, t, &o->move);
}

static void copy_target_position(AoeRepairActionRor *a) {
    a->base.x = a->base.target->core.position.x38;
    a->base.y = a->base.target->core.position.y3c;
    a->base.z = a->base.target->core.position.z40;
}

int aoe_unit_line_terrain_point_ror(AoeLiveUnit *unit, const int32_t from[2],
                                    const int32_t to[2], int32_t out[2],
                                    int terrain_a, int terrain_b,
                                    int require_fit,
                                    const AoeLineSearchOps *o) {
    double dx, dy;
    int32_t steps, last_x = -999, last_y = -999;
    float step_x, step_y, px, py;
    void *c;
    if (!unit || !from || !to || !out || !o || !o->terrain_at || !o->fits)
        return -1;
    c = o->context;
    dx = (double)(to[0] - from[0]);
    dy = (double)(to[1] - from[1]);
    if (dx == 0.0 && dy == 0.0) return 0;
    steps = aoe_ftol32(fabs(dx) < fabs(dy) ? dy : dx);
    steps = (steps < 0 ? -steps : steps) * 2;
    step_x = (float)(dx / steps);
    step_y = (float)(dy / steps);
    px = (float)from[0];
    py = (float)from[1];
    for (int32_t i = 0; i < steps; ++i) {
        int32_t tx, ty, terrain;
        px = px + step_x;
        py = py + step_y;
        tx = aoe_ftol32(px);
        ty = aoe_ftol32(py);
        if (tx == last_x && ty == last_y) continue;
        last_x = tx;
        last_y = ty;
        terrain = o->terrain_at(c, tx, ty) & 0xff;
        if ((terrain == terrain_a || terrain == terrain_b) &&
            require_fit == 1 && o->fits(c, unit, px, py) == 1) {
            out[0] = tx;
            out[1] = ty;
            return 1;
        }
    }
    return 0;
}

int aoe_repair_action_meet_boat_ror(AoeRepairActionRor *a,
                                    const AoeRepairActionOps *o) {
    void *c;
    AoeLiveUnit *u, *boat, *other;
    int32_t from[2], to[2], tile[2];
    float x, y;
    if (!a || !a->base.unit || !a->base.target || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    boat = a->base.target;
    if (!is_boat(boat)) return 1;
    for (uint32_t i = 0; (other = o->owner_unit_at(c, u, i)); ++i) {
        if (other == u || !other->core.unit_template ||
            other->core.unit_template->id != REPAIRER_TEMPLATE ||
            o->head_target(c, other) != boat ||
            o->head_state(c, other) == 2)
            continue;
        /* Another repairer is already bringing the boat in. */
        if (o->head_state(c, boat) == 0x0b) {
            o->head_destination(c, boat, &a->base.x, &a->base.y);
        } else {
            a->base.x = boat->core.position.x38;
            a->base.y = boat->core.position.y3c;
        }
        return 1;
    }
    from[0] = aoe_ftol32(u->core.position.x38);
    from[1] = aoe_ftol32(u->core.position.y3c);
    to[0] = aoe_ftol32(boat->core.position.x38);
    to[1] = aoe_ftol32(boat->core.position.y3c);
    {
        AoeLineSearchOps line = {c, o->terrain_at, o->fits};
        if (aoe_unit_line_terrain_point_ror(u, from, to, tile, BEACH, BEACH,
                                            1, &line) != 1)
            return 1;
    }
    x = (float)tile[0] + 0.5f;
    y = (float)tile[1] + 0.5f;
    a->base.x = x;
    a->base.y = y;
    (void)o->move_to(c, boat, x, y, boat->core.position.z40, 0.0f, 1);
    a->boat_order = o->order_serial(c, boat);
    return 1;
}

int aoe_repair_action_init_target_ror(AoeRepairActionRor *a,
                                      AoeLiveUnit *unit,
                                      const DatUnitTask *task,
                                      AoeLiveUnit *target,
                                      const AoeRepairActionOps *o) {
    if (!a || !unit || !task || !ops_ok(o)) return 0;
    memset(a, 0, sizeof(*a));
    a->base.type = 0x6a;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    if (!set_target(a, target, o)) return 0;
    if (target) {
        copy_target_position(a);
        if (!aoe_repair_action_meet_boat_ror(a, o)) return 0;
    }
    return 1;
}

int aoe_repair_action_init_point_ror(AoeRepairActionRor *a,
                                     AoeLiveUnit *unit,
                                     const DatUnitTask *task, float x,
                                     float y, float z) {
    if (!a || !unit || !task) return 0;
    memset(a, 0, sizeof(*a));
    a->base.type = 0x6a;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    return 1;
}

int aoe_repair_action_set_state_ror(AoeRepairActionRor *a, uint8_t state,
                                    const AoeRepairActionOps *o) {
    void *c;
    AoeLiveUnit *u, *t;
    if (!a || !a->base.unit || !a->task || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    o->sub_clear(c, a);
    a->base.state = state;
    switch (state) {
    case 1:
        if (!set_target(a, NULL, o)) return 0;
        if (!a->base.quiet) notice(a, o, 0x1fa);
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 2:
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 3:
        notice(a, o, 0x202);
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 4:
        if (a->base.target) copy_target_position(a);
        if (!o->start_move(c, a, a->base.target,
                           u->core.unit_template->max_range,
                           o->graphic(c, a->task->approach_graphic), 0))
            return aoe_repair_action_set_state_ror(a, 0x0d, o);
        return 1;
    case 6:
        if (a->base.target) copy_target_position(a);
        show(a, o, a->task->approach_graphic);
        return 1;
    case 7:
        show(a, o, a->task->action_graphic);
        if (a->task->action_sound >= 0)
            o->play_sound(c, a->task->action_sound);
        return 1;
    case 0x0a:
        show(a, o, u->core.unit_template->standing_graphic);
        a->timer = 1.0f;
        t = a->base.target;
        if (t && is_boat(t) && o->order_serial(c, t) == a->boat_order)
            (void)o->move_to(c, t, u->core.position.x38,
                             u->core.position.y3c, u->core.position.z40,
                             0.0f, 1);
        return 1;
    case 0x0b:
        if (!o->start_move(c, a, NULL, 0.0f,
                           o->graphic(c, a->task->approach_graphic), 1))
            return aoe_repair_action_set_state_ror(a, 0x0d, o);
        return 1;
    default:
        return 1;
    }
}

int aoe_repair_action_activate_ror(AoeRepairActionRor *a, int arg,
                                   const AoeRepairActionOps *o) {
    if (!a) return 0;
    return aoe_repair_action_set_state_ror(a, arg && a->base.target ? 4 : 3,
                                           o);
}

int aoe_repair_action_walk_ror(AoeRepairActionRor *a, float x, float y,
                               float z, const AoeRepairActionOps *o) {
    if (!a || !ops_ok(o)) return 0;
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    if (!set_target(a, NULL, o)) return 0;
    return aoe_repair_action_set_state_ror(a, 0x0b, o);
}

int aoe_repair_action_retarget_ror(AoeRepairActionRor *a, AoeLiveUnit *t,
                                   float x, float y, float z,
                                   const AoeRepairActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    if (!t) return aoe_repair_action_walk_ror(a, x, y, z, o);
    if (t == a->base.target && (a->base.state == 4 || a->base.state == 6))
        return 1;
    if (!set_target(a, t, o)) return 0;
    copy_target_position(a);
    if (!aoe_repair_action_meet_boat_ror(a, o)) return 0;
    return aoe_repair_action_set_state_ror(a, 4, o);
}

/* dist(unit, target) - 2.0 <= 0.4 (0x004aaeb0; fsub 2.0f, fcomp 0.4). */
static int close_enough(AoeRepairActionRor *a, const AoeRepairActionOps *o) {
    return o->distance(o->move.context, a->base.unit, a->base.target) -
               2.0 <= 0.4;
}

/* Lost or refused target: +0x54(0), state 1. */
static int drop(AoeRepairActionRor *a, const AoeRepairActionOps *o) {
    if (!set_target(a, NULL, o)) return 0;
    return aoe_repair_action_set_state_ror(a, 1, o);
}

/* State 7 (0x004b5bb0). */
static int work(AoeRepairActionRor *a, const AoeRepairActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    if (!t) return aoe_repair_action_set_state_ror(a, 1, o);
    if (t->core.position.x38 == a->base.x &&
        a->base.y == t->core.position.y3c) {
        /* fld tick; fmul work rate; fmul task work value; fstp */
        float amount = (float)((double)o->tick_seconds(c) *
                               u->core.unit_template->work_rate *
                               a->task->work_amount);
        switch (o->repair(c, t, amount)) {
        case 1:
            return aoe_repair_action_set_state_ror(a, 1, o);
        case 2:
            notice(a, o, 0x1fb);
            o->event(c, 0x6b, u->runtime_owner_index, t->core.unit_id);
            return aoe_repair_action_set_state_ror(a, 2, o);
        default:
            return 1;
        }
    }
    /* The target moved: follow it while close, else go idle. */
    if (!close_enough(a, o)) return aoe_repair_action_set_state_ror(a, 3, o);
    copy_target_position(a);
    return 1;
}

int aoe_repair_action_update_ror(AoeRepairActionRor *a,
                                 const AoeRepairActionOps *o,
                                 uint8_t *result) {
    void *c;
    AoeLiveUnit *t;
    if (!a || !a->base.unit || !a->task || !result || !ops_ok(o)) return 0;
    c = o->move.context;
    *result = 0;
    if (a->base.target_id != -1 && !o->move.resolve(c, a->base.target_id))
        return drop(a, o);
    if (a->base.secondary_id != -1 &&
        !o->move.resolve(c, a->base.secondary_id))
        o->move.clear_secondary(c, &a->base);
    t = a->base.target;
    if (t && (t->core.state != 2 || !o->may_repair(c, a->base.unit, t)))
        return drop(a, o);
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 4:
        switch (o->sub_update(c, a)) {
        case 1: case 2: case 3:
            return aoe_repair_action_set_state_ror(
                a, close_enough(a, o) ? 6 : 0x0a, o);
        case 4: case 5:
            return drop(a, o);
        default:
            return 1;
        }
    case 6:
        if (!t) return aoe_repair_action_set_state_ror(a, 1, o);
        if (o->face(c, a->base.unit, t))
            return aoe_repair_action_set_state_ror(a, 7, o);
        return 1;
    case 7:
        return work(a, o);
    case 0x0a:
        if (!t) return 1;
        a->timer = a->timer - o->tick_seconds(c);
        if (a->timer > 0.0f) return 1;
        if (close_enough(a, o))
            return aoe_repair_action_set_state_ror(a, 6, o);
        a->timer = 1.0f;
        return 1;
    case 0x0b:
        switch (o->sub_update(c, a)) {
        case 1: case 2: case 3: case 5:
            return aoe_repair_action_set_state_ror(a, 1, o);
        case 4:
            if (!set_target(a, NULL, o)) return 0;
            return aoe_repair_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 0x0d:
        notice(a, o, 0x1f9);
        if (!aoe_repair_action_set_state_ror(a, 2, o)) return 0;
        *result = 3;
        return 1;
    default:
        return 1;
    }
}

/* ---- unit +0x8c ----------------------------------------------------- */

/* Root 0x004aad30. */
static int repair_root(AoeLiveUnit *u, float amount) {
    double sum = (double)amount + u->core.hit_points;
    double max;
    u->core.hit_points = (float)sum;
    if (amount < 0.0f && sum < 1.0) u->core.hit_points = 1.0f;
    max = (double)u->core.unit_template->hit_points;
    if (max > u->core.hit_points) return 0;
    u->core.hit_points = (float)max;
    return 1;
}

/* The paid share of 0x004b0780 / 0x004acb70: g = H x amount / T (the
   float store; the comparison uses the unrounded value), capped at the
   missing hit points; multiplier g / 2H. */
static int repair_paid(AoeLiveUnit *u, float amount, int pop_flag,
                       AoeRepairPayFn pay, void *context, float *gain) {
    const DatUnit *t = u->core.unit_template;
    double h = (double)t->hit_points;
    double g = h * amount / (double)t->train_time;
    float gf = (float)g;
    if (g + u->core.hit_points > h) gf = (float)(h - u->core.hit_points);
    *gain = gf;
    return pay(context, u, (float)((double)gf / (2.0 * h)), pop_flag);
}

static int repair_tribe(AoeLiveUnit *u, float amount, AoeRepairPayFn pay,
                        void *context) {
    float gain;
    if (!is_boat(u)) return repair_root(u, amount);
    if (!pay || u->core.unit_template->train_time == 0) return -1;
    if (!repair_paid(u, amount, 1, pay, context, &gain)) return 2;
    return repair_root(u, gain);
}

int aoe_live_unit_repair_ror(AoeLiveUnit *u, float amount, AoeRepairPayFn pay,
                             void *context) {
    const DatUnit *t;
    float gain;
    if (!u || !(t = u->core.unit_template)) return -1;
    if (t->type < 70) return repair_root(u, amount);
    if (t->type == 70) return repair_tribe(u, amount, pay, context);
    /* The original divides by a zero train time; refuse instead. */
    if (!pay || t->train_time == 0) return -1;
    if (!repair_paid(u, amount, 0, pay, context, &gain)) return 2;
    return repair_tribe(u, gain, pay, context);
}

int aoe_unit_path_terrain_point_ror(AoeLiveUnit *unit, AoeUnitPathRor *path,
                                    int terrain, float *x, float *y,
                                    const AoeLineSearchOps *ops) {
    int32_t i;
    if (!path || !path->points || path->count < 2 || !x || !y) return 0;
    for (i = 0; i + 1 < path->count; ++i) {
        const AoePathWaypointRor *a = &path->points[i];
        const AoePathWaypointRor *b = &path->points[i + 1];
        int32_t from[2], to[2], out[2] = {0, 0};
        from[0] = (int32_t)a->x; from[1] = (int32_t)a->y;
        to[0] = (int32_t)b->x; to[1] = (int32_t)b->y;
        path->cursor = i + 1;
        if (aoe_unit_line_terrain_point_ror(unit, from, to, out, terrain,
                                            terrain, 1, ops) == 1) {
            *x = *y = (float)out[0] + 0.5f; /* 0x00542468 */
            return 1;
        }
    }
    *x = -1.0f;
    *y = -1.0f;
    return 0;
}
