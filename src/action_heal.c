#include "action_heal.h"

#include <string.h>

enum { NOTICE_VALUE = 0x25b };

static int ops_ok(const AoeHealActionOps *o) {
    return o && o->move.resolve && o->move.target_released &&
           o->move.target_attached && o->move.unit_notice &&
           o->move.select_graphic && o->move.clear_secondary && o->graphic &&
           o->tick_seconds && o->distance && o->face && o->allowed &&
           o->heal && o->heal_rate && o->play_sound && o->start_move &&
           o->sub_update && o->sub_clear;
}

static int is_boat(const AoeLiveUnit *u) {
    int16_t c = u && u->core.unit_template ? u->core.unit_template->unit_class
                                           : -1;
    return c == 2 || c == 0x14 || c == 0x15 || c == 0x16;
}

static void notice(AoeHealActionRor *a, const AoeHealActionOps *o,
                   int32_t code) {
    o->move.unit_notice(o->move.context, a->base.unit, code, NOTICE_VALUE, 0);
}

static void show(AoeHealActionRor *a, const AoeHealActionOps *o, int16_t id) {
    o->move.select_graphic(o->move.context, a->base.unit,
                           o->graphic(o->move.context, id));
}

static void follow(AoeHealActionRor *a) {
    a->base.x = a->base.target->core.position.x38;
    a->base.y = a->base.target->core.position.y3c;
    a->base.z = a->base.target->core.position.z40;
}

static void init(AoeHealActionRor *a, AoeLiveUnit *unit,
                 const DatUnitTask *task) {
    memset(a, 0, sizeof(*a));
    a->base.type = 0x69;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
}

/* 0x004b2c00 copies the target's position unconditionally. */
int aoe_heal_action_init_target_ror(AoeHealActionRor *a, AoeLiveUnit *unit,
                                    const DatUnitTask *task,
                                    AoeLiveUnit *target,
                                    const AoeMoveActionOps *o) {
    if (!a || !unit || !task || !target || !o) return 0;
    init(a, unit, task);
    if (!aoe_action_set_target_ror(&a->base, target, o)) return 0;
    follow(a);
    return 1;
}

int aoe_heal_action_init_point_ror(AoeHealActionRor *a, AoeLiveUnit *unit,
                                   const DatUnitTask *task, float x, float y,
                                   float z) {
    if (!a || !unit || !task) return 0;
    init(a, unit, task);
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    return 1;
}

int aoe_heal_action_set_state_ror(AoeHealActionRor *a, uint8_t state,
                                  const AoeHealActionOps *o) {
    void *c;
    AoeLiveUnit *u;
    if (!a || !a->base.unit || !a->task || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    o->sub_clear(c, &a->base);
    a->base.state = state;
    switch (state) {
    case 2:
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 3:
        notice(a, o, 0x202);
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 4:
        if (a->base.target) follow(a);
        if (!o->start_move(c, &a->base, a->base.target, 0.4f,
                           o->graphic(c, a->task->moving_graphic), 0))
            return aoe_heal_action_set_state_ror(a, 0x0d, o);
        return 1;
    case 6:
        if (a->base.target) follow(a);
        show(a, o, a->task->moving_graphic);
        return 1;
    case 7:
        show(a, o, a->task->proceeding_graphic);
        if (a->task->gathering_sound >= 0)
            o->play_sound(c, a->task->gathering_sound);
        return 1;
    case 0x0b:
        if (!o->start_move(c, &a->base, NULL, 0.0f,
                           o->graphic(c, a->task->moving_graphic), 1))
            return aoe_heal_action_set_state_ror(a, 0x0d, o);
        return 1;
    default:
        return 1;
    }
}

int aoe_heal_action_activate_ror(AoeHealActionRor *a,
                                 const AoeHealActionOps *o) {
    if (!a) return 0;
    if (!a->base.target) return aoe_heal_action_set_state_ror(a, 3, o);
    if (is_boat(a->base.target)) return 1;
    return aoe_heal_action_set_state_ror(a, 4, o);
}

int aoe_heal_action_retarget_ror(AoeHealActionRor *a, AoeLiveUnit *t,
                                 const AoeHealActionOps *o, int *accepted) {
    if (!a || !accepted || !ops_ok(o)) return 0;
    *accepted = 0;
    if (!t) return 1;
    *accepted = 1;
    if (t == a->base.target &&
        (a->base.state == 4 || a->base.state == 6 || a->base.state == 7))
        return 1;
    if (!aoe_action_set_target_ror(&a->base, t, &o->move)) return 0;
    follow(a);
    return aoe_heal_action_activate_ror(a, o);
}

static int idle(AoeHealActionRor *a, const AoeHealActionOps *o) {
    if (!aoe_action_set_target_ror(&a->base, NULL, &o->move)) return 0;
    return aoe_heal_action_set_state_ror(a, 3, o);
}

/* State 7 (0x004b30c3). */
static int work(AoeHealActionRor *a, const AoeHealActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    double d;
    if (!t) return aoe_heal_action_set_state_ror(a, 3, o);
    if (t->core.position.x38 == a->base.x &&
        a->base.y == t->core.position.y3c) {
        float amount = (float)((double)o->tick_seconds(c) *
                               u->core.unit_template->work_rate *
                               a->task->work_value1);
        float rate = o->heal_rate(c, u);
        if (!((double)rate <= (double)0.1f)) amount = amount * rate;
        switch (o->heal(c, t, amount)) {
        case 1:
            return aoe_heal_action_set_state_ror(a, 3, o);
        case 2:
            notice(a, o, 0x1fb);
            return aoe_heal_action_set_state_ror(a, 2, o);
        default:
            return 1;
        }
    }
    d = o->distance(c, u, t);
    if (d <= (double)a->task->work_range || d <= 0.5) {
        follow(a);
        return 1;
    }
    return aoe_heal_action_set_state_ror(a, 3, o);
}

int aoe_heal_action_update_ror(AoeHealActionRor *a, const AoeHealActionOps *o,
                               uint8_t *result) {
    void *c;
    AoeLiveUnit *t;
    if (!a || !a->base.unit || !a->task || !result || !ops_ok(o)) return 0;
    c = o->move.context;
    *result = 0;
    if (a->base.target_id != -1 && !o->move.resolve(c, a->base.target_id))
        return idle(a, o);
    if (a->base.secondary_id != -1 &&
        !o->move.resolve(c, a->base.secondary_id))
        o->move.clear_secondary(c, &a->base);
    t = a->base.target;
    if (t) {
        if (t->core.state != 2) return idle(a, o);
        if (!o->allowed(c, a->base.unit, t)) {
            if (!aoe_heal_action_set_state_ror(a, 1, o)) return 0;
            notice(a, o, 0x1fb);
            *result = 4;
            return 1;
        }
    }
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 4:
        switch (o->sub_update(c, &a->base)) {
        case 1: case 2:
            return aoe_heal_action_set_state_ror(a, 6, o);
        case 3:
            return aoe_heal_action_set_state_ror(a, 0x0d, o);
        case 4: case 5:
            if (!aoe_action_set_target_ror(&a->base, NULL, &o->move))
                return 0;
            return aoe_heal_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 6:
        if (!t) return aoe_heal_action_set_state_ror(a, 3, o);
        if (o->face(c, a->base.unit, t))
            return aoe_heal_action_set_state_ror(a, 7, o);
        return 1;
    case 7:
        return work(a, o);
    case 0x0b:
        switch (o->sub_update(c, &a->base)) {
        case 1: case 2: case 5:
            return aoe_heal_action_set_state_ror(a, 3, o);
        case 3:
            return aoe_heal_action_set_state_ror(a, 0x0d, o);
        case 4:
            if (!aoe_action_set_target_ror(&a->base, NULL, &o->move))
                return 0;
            return aoe_heal_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 0x0d:
        notice(a, o, 0x1f9);
        if (!aoe_heal_action_set_state_ror(a, 2, o)) return 0;
        *result = 3;
        return 1;
    default:
        return 1;
    }
}
