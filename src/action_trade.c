#include "action_trade.h"

#include <string.h>

#include "aoe_ftol.h"

enum { NOTICE_VALUE = 0x267, GOODS = 9, GOLD = 3 };

static int ops_ok(const AoeTradeActionOps *o) {
    return o && o->move.resolve && o->move.target_released &&
           o->move.target_attached && o->move.unit_notice &&
           o->move.select_graphic && o->graphic && o->tick_seconds &&
           o->face && o->resource && o->resource_store && o->resource_add &&
           o->civ && o->find_home && o->play_sound && o->start_move &&
           o->sub_update && o->sub_clear;
}

static void notice(AoeTradeActionRor *a, const AoeTradeActionOps *o,
                   int32_t code, int32_t argument) {
    o->move.unit_notice(o->move.context, a->base.unit, code, NOTICE_VALUE,
                        argument);
}

static int set_target(AoeTradeActionRor *a, AoeLiveUnit *t,
                      const AoeTradeActionOps *o) {
    return aoe_action_set_target_ror(&a->base, t, &o->move);
}

/* 0x004073f0 on +0x14 / +0x1c. */
static void set_home(AoeTradeActionRor *a, AoeLiveUnit *site,
                     const AoeTradeActionOps *o) {
    void *c = o->move.context;
    if (a->home && a->home != site) {
        AoeLiveUnit *old = o->move.resolve(c, a->base.secondary_id);
        if (old) o->move.target_released(c, old, a->base.unit);
        a->base.secondary_id = -1;
    }
    if (a->home == site) return;
    a->home = site;
    if (site) {
        a->base.secondary_id = site->core.unit_id;
        o->move.target_attached(c, site, a->base.unit, a->base.type);
    }
}

/* 0x00403b00 / 0x00403b30. */
static int carrying(const AoeTradeActionRor *a) {
    const AoeLiveUnit *u = a->base.unit;
    int16_t in = a->task->resource_in;
    return in != -1 && u->core.storage.carried_resource_type == in &&
           aoe_ftol32(u->core.storage.carried_amount) > 0;
}
static const void *move_graphic(AoeTradeActionRor *a,
                                const AoeTradeActionOps *o) {
    const void *g = carrying(a) ? o->graphic(o->move.context,
                                             a->task->carrying_graphic)
                                : NULL;
    return g ? g : o->graphic(o->move.context, a->task->moving_graphic);
}
static const void *idle_graphic(AoeTradeActionRor *a,
                                const AoeTradeActionOps *o) {
    const void *g = carrying(a) ? o->graphic(o->move.context,
                                             a->task->carrying_graphic)
                                : NULL;
    return g ? g
             : o->graphic(o->move.context,
                          a->base.unit->core.unit_template->standing_graphic);
}
static void show(AoeTradeActionRor *a, const AoeTradeActionOps *o,
                 const void *g) {
    o->move.select_graphic(o->move.context, a->base.unit, g);
}
static void follow(AoeTradeActionRor *a) {
    a->base.x = a->base.target->core.position.x38;
    a->base.y = a->base.target->core.position.y3c;
    a->base.z = a->base.target->core.position.z40;
}
/* Unit +0x138 (0x004afb70): templates 45 and 50 stay usable when empty. */
static int keeps_empty(const AoeLiveUnit *t) {
    int16_t id = t->core.unit_template ? t->core.unit_template->id : -1;
    return id == 0x32 || id == 0x2d;
}
/* Unit +0x140 (0x004afbc0): no zone match for classes 2 and 21. */
static int zone_search(const AoeLiveUnit *u) {
    int16_t c = u->core.unit_template->unit_class;
    return c != 0x15 && c != 2;
}
static void goods_type(AoeLiveUnit *u) {
    int16_t t = u->core.storage.carried_resource_type;
    if (t != 0 && t != 2 && t != 1) u->core.storage.carried_resource_type = 1;
}
static int is_home(const AoeTradeActionRor *a, const AoeLiveUnit *t) {
    const AoeLiveUnit *u = a->base.unit;
    return t->runtime_owner_index == u->runtime_owner_index &&
           t->core.unit_template &&
           t->core.unit_template->id == u->core.unit_template->drop_sites[0];
}

static void init(AoeTradeActionRor *a, AoeLiveUnit *unit,
                 const DatUnitTask *task) {
    memset(a, 0, sizeof(*a));
    a->base.type = 0x6f;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    a->origin_x = a->origin_y = -1.0f;
}

int aoe_trade_action_init_target_ror(AoeTradeActionRor *a, AoeLiveUnit *unit,
                                     const DatUnitTask *task,
                                     AoeLiveUnit *target,
                                     const AoeMoveActionOps *o) {
    if (!a || !unit || !task || !target || !o) return 0;
    init(a, unit, task);
    if (!aoe_action_set_target_ror(&a->base, target, o)) return 0;
    follow(a);
    return 1;
}

int aoe_trade_action_init_point_ror(AoeTradeActionRor *a, AoeLiveUnit *unit,
                                    const DatUnitTask *task, float x,
                                    float y, float z) {
    if (!a || !unit || !task) return 0;
    init(a, unit, task);
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    return 1;
}

int aoe_trade_action_set_state_ror(AoeTradeActionRor *a, uint8_t state,
                                   const AoeTradeActionOps *o) {
    void *c;
    AoeLiveUnit *u, *t;
    if (!a || !a->base.unit || !a->task || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    t = a->base.target;
    o->sub_clear(c, &a->base);
    a->base.state = state;
    switch (state) {
    case 2:
        show(a, o, idle_graphic(a, o));
        if (!set_target(a, NULL, o)) return 0;
        set_home(a, NULL, o);
        a->base.x = a->base.y = a->base.z = -1.0f;
        return 1;
    case 3:
        notice(a, o, 0x202, 0);
        show(a, o, idle_graphic(a, o));
        return 1;
    case 4:
        if (!t) return aoe_trade_action_set_state_ror(a, 0x0d, o);
        follow(a);
        if (!o->start_move(c, &a->base, t, a->task->work_range,
                           move_graphic(a, o), 0))
            return aoe_trade_action_set_state_ror(a, 0x0d, o);
        return 1;
    case 6:
        show(a, o, move_graphic(a, o));
        return 1;
    case 7:
        show(a, o, o->graphic(c, a->task->proceeding_graphic));
        if (a->task->gathering_sound >= 0)
            o->play_sound(c, a->task->gathering_sound);
        if (t) {
            a->origin_x = t->core.position.x38;
            a->origin_y = t->core.position.y3c;
        } else {
            a->origin_x = a->origin_y = -1.0f;
        }
        return 1;
    case 8:
        if (u->core.storage.carried_amount > 0.0f) {
            const DatUnit *ut = u->core.unit_template;
            if (!(a->home && a->home->core.unit_template &&
                  a->home->core.unit_template->id == ut->drop_sites[0]))
                set_home(a,
                         ut->drop_sites[0] != -1
                             ? o->find_home(c, u, ut->drop_sites[0],
                                            ut->drop_sites[1],
                                            u->core.position.x38,
                                            u->core.position.y3c,
                                            zone_search(u) ? u : NULL)
                             : NULL,
                         o);
            if (!a->home) {
                notice(a, o, 0x203, 0);
                return aoe_trade_action_set_state_ror(a, 0x0d, o);
            }
            if (!o->start_move(c, &a->base, a->home, 0.0f,
                               move_graphic(a, o), 0))
                return aoe_trade_action_set_state_ror(a, 0x0d, o);
            return 1;
        }
        if (t && !(t->core.storage.carried_amount > 0.0f) &&
            !keeps_empty(t)) {
            if (!set_target(a, NULL, o)) return 0;
            return aoe_trade_action_set_state_ror(a, 3, o);
        }
        return 1;
    case 0x0b:
        if (!o->start_move(c, &a->base, NULL, 0.0f, move_graphic(a, o), 1))
            return aoe_trade_action_set_state_ror(a, 0x0d, o);
        return 1;
    default:
        return 1;
    }
}

int aoe_trade_action_activate_ror(AoeTradeActionRor *a, int arg,
                                  const AoeTradeActionOps *o) {
    if (!a || !a->base.unit) return 0;
    goods_type(a->base.unit);
    if (!arg) return aoe_trade_action_set_state_ror(a, 3, o);
    return aoe_trade_action_set_state_ror(a, a->base.target ? 4 : 2, o);
}

static int to_point(AoeTradeActionRor *a, float x, float y, float z,
                    const AoeTradeActionOps *o) {
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    if (!set_target(a, NULL, o)) return 0;
    return aoe_trade_action_set_state_ror(a, 0x0b, o);
}

int aoe_trade_action_retarget_ror(AoeTradeActionRor *a, AoeLiveUnit *t,
                                  float x, float y, float z,
                                  const AoeTradeActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    if (!t) return to_point(a, x, y, z, o);
    if (t == a->base.target &&
        (a->base.state == 4 || a->base.state == 6 || a->base.state == 7))
        return 1;
    if (is_home(a, t)) {
        set_home(a, t, o);
        return aoe_trade_action_set_state_ror(a, 8, o);
    }
    if (!set_target(a, t, o)) return 0;
    return aoe_trade_action_set_state_ror(a, 4, o);
}

int aoe_trade_action_deliver_ror(AoeTradeActionRor *a, AoeLiveUnit *t,
                                 float x, float y, float z,
                                 const AoeTradeActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    if (t && is_home(a, t)) {
        if (t == a->home && a->base.state == 8) return 1;
        set_home(a, t, o);
        return aoe_trade_action_set_state_ror(a, 8, o);
    }
    return to_point(a, x, y, z, o);
}

int aoe_trade_action_stop_ror(AoeTradeActionRor *a,
                              const AoeTradeActionOps *o) {
    return aoe_trade_action_set_state_ror(a, 2, o);
}

/* timer < 0: count up by the tick; nonzero once it passes 0. */
static int wait_over(AoeTradeActionRor *a, const AoeTradeActionOps *o) {
    double v = (double)o->tick_seconds(o->move.context) + a->timer;
    a->timer = (float)v;
    return v > 0.0;
}

/* State 7 (0x004b6d13). */
static int load(AoeTradeActionRor *a, const AoeTradeActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    float cap, want, have, goods;
    int16_t type;
    if (!t)
        return aoe_trade_action_set_state_ror(
            a, u->core.storage.carried_amount > 0.0f ? 8 : 3, o);
    goods_type(u);
    type = u->core.storage.carried_resource_type;
    cap = (float)u->core.unit_template->resource_capacity;
    want = (float)((double)cap - u->core.storage.carried_amount);
    have = o->resource(c, u, type);
    if (have < want) want = have;
    goods = o->resource(c, t, GOODS);
    if (goods < want) want = goods;
    o->resource_add(c, u, type, -want);
    o->resource_store(c, t, GOODS,
                      (float)((double)o->resource(c, t, GOODS) - want));
    u->core.storage.carried_amount =
        (float)((double)want + u->core.storage.carried_amount);
    if (!(u->core.storage.carried_amount < cap))
        return aoe_trade_action_set_state_ror(a, 8, o);
    if (o->resource(c, t, GOODS) > 0.0f || keeps_empty(t)) return 1;
    return aoe_trade_action_set_state_ror(a, 8, o);
}

/* State 8 arrival (0x004b6ec6). */
static int arrive(AoeTradeActionRor *a, const AoeTradeActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit;
    if (a->home && a->home->runtime_owner_index != u->runtime_owner_index)
        set_home(a, NULL, o);
    if (!a->home) return aoe_trade_action_set_state_ror(a, 8, o);
    if (u->core.storage.carried_amount > 0.0f) {
        double rate;
        float gold;
        if (a->task->deposit_sound >= 0)
            o->play_sound(c, a->task->deposit_sound);
        if (!(a->origin_x < 0.0f) && !(a->origin_y < 0.0f)) {
            double dx = (double)a->origin_x - a->home->core.position.x38;
            double dy = (double)a->origin_y - a->home->core.position.y3c;
            rate = (dx * dx + dy * dy) * (double)0.00025125627871602774f *
                   (double)0.699999988079071f;
        } else {
            rate = (double)0.3499999940395355f;
        }
        if (o->civ(c, u) == 0x0f) rate += rate;
        if (rate < 0.35) rate = (double)0.3499999940395355f;
        gold = (float)(rate * u->core.storage.carried_amount);
        if (!(gold <= 75.0f)) gold = 75.0f;
        o->resource_add(c, u, GOLD, gold);
        u->core.storage.carried_amount = 0.0f; /* +0x80(type, 0) */
    }
    if (!a->base.target) return aoe_trade_action_set_state_ror(a, 3, o);
    return aoe_trade_action_set_state_ror(a, 4, o);
}

int aoe_trade_action_update_ror(AoeTradeActionRor *a,
                                const AoeTradeActionOps *o,
                                uint8_t *result) {
    void *c;
    AoeLiveUnit *u, *t;
    if (!a || !a->base.unit || !a->task || !result || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    *result = 0;
    if (a->base.target_id != -1 && !o->move.resolve(c, a->base.target_id) &&
        !set_target(a, NULL, o))
        return 0;
    if (a->base.secondary_id != -1 &&
        !o->move.resolve(c, a->base.secondary_id))
        set_home(a, NULL, o);
    if (a->base.target && a->base.target->core.state >= 7 &&
        !set_target(a, NULL, o))
        return 0;
    if (a->home && a->home->core.state >= 7) set_home(a, NULL, o);
    t = a->base.target;
    if (t && t->runtime_owner_index == u->runtime_owner_index) {
        if (!set_target(a, NULL, o)) return 0;
        set_home(a, NULL, o);
        if (!aoe_trade_action_set_state_ror(a, 1, o)) return 0;
        if (!a->base.quiet) notice(a, o, 0x1fb, a->base.target_id);
        *result = 4;
        return 1;
    }
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 4:
        if (a->timer < 0.0f) {
            if (!wait_over(a, o)) return 1;
            a->timer = 0.0f;
            return aoe_trade_action_set_state_ror(a, t ? 4 : 3, o);
        }
        switch (o->sub_update(c, &a->base)) {
        case 1: case 2:
            return aoe_trade_action_set_state_ror(a, 6, o);
        case 3: case 4:
            a->timer = -0.5f;
            return 1;
        case 5:
            if (!set_target(a, NULL, o)) return 0;
            return aoe_trade_action_set_state_ror(a, 3, o);
        default:
            return 1;
        }
    case 6:
        if (!t) return aoe_trade_action_set_state_ror(a, 3, o);
        if (!o->face(c, u, t)) return 1;
        if (t->core.storage.carried_amount > 0.0f || keeps_empty(t))
            return aoe_trade_action_set_state_ror(a, 7, o);
        if (!set_target(a, NULL, o)) return 0;
        return aoe_trade_action_set_state_ror(a, 3, o);
    case 7:
        return load(a, o);
    case 8:
        if (a->timer < 0.0f) {
            if (!wait_over(a, o)) return 1;
            a->timer = 0.0f;
            return aoe_trade_action_set_state_ror(a, 8, o);
        }
        switch (o->sub_update(c, &a->base)) {
        case 1: case 2:
            return arrive(a, o);
        case 3: case 4: case 5:
            a->timer = -0.5f;
            return 1;
        default:
            return 1;
        }
    case 0x0b:
        if (a->timer < 0.0f) {
            if (!wait_over(a, o)) return 1;
            a->timer = 0.0f;
            return aoe_trade_action_set_state_ror(a, t ? 4 : 3, o);
        }
        switch (o->sub_update(c, &a->base)) {
        case 1: case 2: case 5:
            return aoe_trade_action_set_state_ror(a, 3, o);
        case 3: case 4:
            a->timer = -0.5f;
            return 1;
        default:
            return 1;
        }
    case 0x0d:
        notice(a, o, 0x1f9, 0);
        if (!aoe_trade_action_set_state_ror(a, 2, o)) return 0;
        *result = 3;
        return 1;
    default:
        return 1;
    }
}
