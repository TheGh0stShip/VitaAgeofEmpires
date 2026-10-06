#include "action_convert.h"

#include <math.h>
#include <string.h>

#include "aoe_ftol.h"

enum { NOTICE_VALUE = 0x25c, FAITH = 0x22 };

static int ops_ok(const AoeConvertActionOps *o) {
    return o && o->move.resolve && o->move.target_released &&
           o->move.target_attached && o->move.unit_notice &&
           o->move.select_graphic && o->move.clear_secondary && o->graphic &&
           o->tick_seconds && o->face && o->allied && o->owner_kind &&
           o->resource && o->event && o->rand && o->civ && o->change_owner &&
           o->resource_add && o->set_carried && o->flare &&
           o->target_notice && o->play_sound && o->start_move &&
           o->sub_update && o->sub_clear;
}

static void notice(AoeConvertActionRor *a, const AoeConvertActionOps *o,
                   int32_t code) {
    o->move.unit_notice(o->move.context, a->base.unit, code, NOTICE_VALUE, 0);
}

static void show(AoeConvertActionRor *a, const AoeConvertActionOps *o,
                 int16_t id) {
    o->move.select_graphic(o->move.context, a->base.unit,
                           o->graphic(o->move.context, id));
}

static void follow(AoeConvertActionRor *a) {
    a->base.x = a->base.target->core.position.x38;
    a->base.y = a->base.target->core.position.y3c;
    a->base.z = a->base.target->core.position.z40;
}

static void init(AoeConvertActionRor *a, AoeLiveUnit *unit,
                 const DatUnitTask *task) {
    memset(a, 0, sizeof(*a));
    a->base.type = 0x68;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    a->range = 1.0f;
}

int aoe_convert_action_init_target_ror(AoeConvertActionRor *a,
                                       AoeLiveUnit *unit,
                                       const DatUnitTask *task,
                                       AoeLiveUnit *target,
                                       const AoeMoveActionOps *o) {
    if (!a || !unit || !task || !o) return 0;
    init(a, unit, task);
    if (!aoe_action_set_target_ror(&a->base, target, o)) return 0;
    if (target) follow(a);
    return 1;
}

int aoe_convert_action_init_point_ror(AoeConvertActionRor *a,
                                      AoeLiveUnit *unit,
                                      const DatUnitTask *task, float x,
                                      float y, float z) {
    if (!a || !unit || !task) return 0;
    init(a, unit, task);
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    return 1;
}

int aoe_convert_action_set_state_ror(AoeConvertActionRor *a, uint8_t state,
                                     const AoeConvertActionOps *o) {
    void *c;
    AoeLiveUnit *u, *t;
    if (!a || !a->base.unit || !a->task || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    o->sub_clear(c, &a->base);
    a->base.state = state;
    switch (state) {
    case 1: case 2: case 0x0d: case 0x0e:
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 3:
        notice(a, o, 0x202);
        show(a, o, u->core.unit_template->standing_graphic);
        return 1;
    case 4:
        if (a->base.target) follow(a);
        if (!o->start_move(c, &a->base, a->base.target, a->range,
                           o->graphic(c, a->task->approach_graphic), 0))
            return aoe_convert_action_set_state_ror(a, 0x0d, o);
        return 1;
    case 6:
        show(a, o, a->task->approach_graphic);
        return 1;
    case 7:
        show(a, o, a->task->action_graphic);
        if (a->task->action_sound >= 0)
            o->play_sound(c, a->task->action_sound);
        t = a->base.target;
        if (t && t->runtime_owner_index > 0)
            o->flare(c, t, u->core.position.x38, u->core.position.y3c,
                     u->core.position.z40);
        a->timer = a->task->work_timer;
        return 1;
    case 0x0b:
        if (!o->start_move(c, &a->base, NULL, a->range,
                           o->graphic(c, a->task->approach_graphic), 1))
            return aoe_convert_action_set_state_ror(a, 0x0d, o);
        return 1;
    default:
        return 1;
    }
}

/* Unconvertible or not yet allowed: owner kind 1 walks to the point. */
static int refuse(AoeConvertActionRor *a, const AoeConvertActionOps *o,
                  uint32_t event) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit;
    if (!aoe_convert_action_set_state_ror(
            a, o->owner_kind(c, u) == 1 ? 0x0b : 0x0d, o))
        return 0;
    if (event)
        o->event(c, event, u->runtime_owner_index, 0, 0, 0);
    return 1;
}

int aoe_convert_action_activate_ror(AoeConvertActionRor *a, int active,
                                    const AoeConvertActionOps *o) {
    void *c;
    AoeLiveUnit *u, *t;
    const DatUnit *tt;
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    t = a->base.target;
    if (t && (tt = t->core.unit_template)) {
        int16_t id = tt->id, cls = tt->unit_class;
        a->same_owner = t->runtime_owner_index == u->runtime_owner_index;
        if (id == 109 || cls == 0x1b || id == 0x9f || id == 0x9e ||
            id == 0xa3 || id == 0x114)
            return refuse(a, o, id == 109 ? 0x79 : id == 0x114 ? 0x7a : 0);
        if ((double)u->core.storage.carried_amount < 100.0) {
            if (!aoe_convert_action_set_state_ror(a, 0x0d, o)) return 0;
            o->event(c, 0x76, u->runtime_owner_index, 0, 0, 0);
            return 1;
        }
        if (cls == 3) {
            a->range = 1.0f;
            if (!(o->resource(c, u, 28) > 0.0f)) return refuse(a, o, 0x78);
        } else {
            a->range = u->core.unit_template->max_range;
            if (cls == 0x1e) return aoe_convert_action_set_state_ror(a, 0x0d, o);
            if (cls == 0x12 && !(o->resource(c, u, 27) > 0.0f))
                return refuse(a, o, 0x77);
        }
    }
    return aoe_convert_action_set_state_ror(a, active && t ? 4 : 3, o);
}

int aoe_convert_action_retarget_ror(AoeConvertActionRor *a, AoeLiveUnit *t,
                                    const AoeConvertActionOps *o,
                                    int *accepted) {
    if (!a || !accepted || !ops_ok(o)) return 0;
    *accepted = 0;
    if (!t) return 1;
    *accepted = 1;
    if (t == a->base.target &&
        (a->base.state == 4 || a->base.state == 6 || a->base.state == 7))
        return 1;
    if (!aoe_action_set_target_ror(&a->base, t, &o->move)) return 0;
    follow(a);
    return aoe_convert_action_activate_ror(a, 1, o);
}

static int idle(AoeConvertActionRor *a, const AoeConvertActionOps *o) {
    if (!aoe_action_set_target_ror(&a->base, NULL, &o->move)) return 0;
    return aoe_convert_action_set_state_ror(a, 3, o);
}

/* State 7 (0x004b2461). */
static int attempt(AoeConvertActionRor *a, const AoeConvertActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    const DatUnit *tt;
    double chance, need;
    int16_t cls;
    if (!t) return aoe_convert_action_set_state_ror(a, 3, o);
    o->target_notice(c, t, u);
    a->timer = a->timer - o->tick_seconds(c);
    if (a->timer > 0.0f) return 1;
    if (t->runtime_owner_index > 0)
        o->flare(c, t, u->core.position.x38, u->core.position.y3c,
                 u->core.position.z40);
    /* fild rand(); fmul 1/32767 (as a float constant) */
    chance = (double)o->rand(c) * (double)3.0518509447574615e-05f;
    tt = t->core.unit_template;
    cls = tt->unit_class;
    if (o->civ(c, t) == 0x10)
        chance *= 4.0;
    else if (cls == 0x15 || cls == 2 || cls == 0x14 || cls == 0x16 ||
             cls == 0x12)
        chance += chance;
    else if (cls == 0x23 || cls == 0x17)
        chance *= 8.0;
    need = (double)u->core.unit_template->work_rate * a->task->work_amount;
    if (chance <= need) {
        int16_t to = u->runtime_owner_index;
        o->event(c, 0x7b, to, t->runtime_owner_index,
                 aoe_ftol32(t->core.position.x38),
                 aoe_ftol32(t->core.position.y3c));
        if (!o->change_owner(c, t, to)) return 0;
        o->resource_add(c, u, 41, 1.0f);
        o->set_carried(c, u, FAITH, 0.0f);
        if (!aoe_convert_action_set_state_ror(a, 3, o)) return 0;
    }
    return aoe_convert_action_set_state_ror(a, 4, o);
}

int aoe_convert_action_update_ror(AoeConvertActionRor *a,
                                  const AoeConvertActionOps *o,
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
        if (o->allied(c, a->base.unit, t) == 1) {
            if (a->base.state == 7 || a->base.state == 6 ||
                a->base.state == 4)
                o->set_carried(c, a->base.unit, FAITH, 0.0f);
            if (!aoe_convert_action_set_state_ror(a, 1, o)) return 0;
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
            return aoe_convert_action_set_state_ror(a, 6, o);
        case 3:
            return aoe_convert_action_set_state_ror(a, 0x0d, o);
        case 4: case 5:
            if (!aoe_action_set_target_ror(&a->base, NULL, &o->move))
                return 0;
            return aoe_convert_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 6:
        if (!t) return aoe_convert_action_set_state_ror(a, 3, o);
        if (o->face(c, a->base.unit, t))
            return aoe_convert_action_set_state_ror(a, 7, o);
        return 1;
    case 7:
        return attempt(a, o);
    case 0x0b:
        switch (o->sub_update(c, &a->base)) {
        case 1: case 2: case 5:
            return aoe_convert_action_set_state_ror(a, 3, o);
        case 3:
            return aoe_convert_action_set_state_ror(a, 0x0d, o);
        case 4:
            if (!aoe_action_set_target_ror(&a->base, NULL, &o->move))
                return 0;
            return aoe_convert_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 0x0d:
        notice(a, o, 0x1f9);
        if (!aoe_convert_action_set_state_ror(a, 2, o)) return 0;
        *result = 3;
        return 1;
    default:
        return 1;
    }
}

int aoe_convert_action_complete_ror(AoeConvertActionRor *a,
                                    const AoeConvertActionOps *o,
                                    int *converted) {
    AoeLiveUnit *u, *t;
    void *c;
    if (converted) *converted = 0;
    if (!a || !ops_ok(o) || !(u = a->base.unit)) return 0;
    t = a->base.target;
    if (!t || !t->core.unit_template || t->core.unit_template->unit_class == 0x12)
        return 1;
    c = o->move.context;
    if (a->base.state != 7) {
        double dx = (double)t->core.position.x38 - u->core.position.x38;
        double dy = (double)t->core.position.y3c - u->core.position.y3c;
        if (!(sqrt(dx * dx + dy * dy) - 0.5 <= (double)a->range)) return 1;
    }
    o->event(c, 0x7b, u->runtime_owner_index, t->runtime_owner_index,
             aoe_ftol32(t->core.position.x38), aoe_ftol32(t->core.position.y3c));
    if (!o->change_owner(c, t, u->runtime_owner_index)) return 0;
    o->resource_add(c, u, 41, 1.0f);
    o->set_carried(c, u, FAITH, 0.0f);
    if (converted) *converted = 1;
    return aoe_convert_action_set_state_ror(a, 3, o);
}
