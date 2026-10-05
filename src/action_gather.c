#include "action_gather.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "aoe_ftol.h"

void aoe_unit_set_carried_ror(AoeLiveUnit *u, int16_t type, float amount) {
    if (!u) return;
    u->core.storage.carried_resource_type = type;
    u->core.storage.carried_amount = amount;
}

void aoe_unit_add_carried_ror(AoeLiveUnit *u, float amount, int add,
                              int clamp) {
    if (!u) return;
    if (add)
        u->core.storage.carried_amount =
            (float)((double)amount + u->core.storage.carried_amount);
    else
        u->core.storage.carried_amount = amount;
    if (clamp && u->core.unit_template) {
        float cap = (float)u->core.unit_template->resource_capacity;
        if (cap < u->core.storage.carried_amount ||
            isnan(u->core.storage.carried_amount))
            u->core.storage.carried_amount = cap;
    }
}

int aoe_unit_keeps_empty_target_ror(int16_t id) {
    return id == 0x32 || id == 0x2d;
}

int aoe_unit_keeps_resource_ror(const AoeLiveUnit *v, int16_t attacker) {
    int16_t c = v && v->core.unit_template ? v->core.unit_template->unit_class
                                           : -1;
    if (c != 9 && c != 10) return 1;
    return attacker == 4 || attacker == 0xb;
}

int aoe_unit_zone_drop_search_ror(const AoeLiveUnit *u) {
    int16_t c = u && u->core.unit_template ? u->core.unit_template->unit_class
                                           : -1;
    return c != 0x15 && c != 2;
}

static int ops_ok(const AoeGatherActionOps *o) {
    return o && o->move.resolve && o->move.target_released &&
           o->move.target_attached && o->move.unit_notice &&
           o->move.select_graphic && o->graphic && o->tick_seconds &&
           o->owner_kind && o->face && o->distance && o->player_attribute &&
           o->deposit && o->find_drop_site && o->play_sound &&
           o->start_move && o->sub_update && o->sub_clear &&
           o->start_attack;
}

static void notice(AoeGatherActionRor *a, const AoeGatherActionOps *o,
                   int32_t code, int32_t value, int32_t argument) {
    o->move.unit_notice(o->move.context, a->base.unit, code, value,
                        argument);
}

/* 0x004073f0 on +0x14 / +0x1c. */
static void set_drop_site(AoeGatherActionRor *a, AoeLiveUnit *site,
                          const AoeGatherActionOps *o) {
    void *c = o->move.context;
    if (a->drop_site && a->drop_site != site) {
        AoeLiveUnit *old = o->move.resolve(c, a->base.secondary_id);
        if (old) o->move.target_released(c, old, a->base.unit);
    }
    if (a->drop_site == site) return;
    a->drop_site = site;
    if (site) {
        a->base.secondary_id = site->core.unit_id;
        o->move.target_attached(c, site, a->base.unit, a->base.type);
    }
}

static int set_target(AoeGatherActionRor *a, AoeLiveUnit *t,
                      const AoeGatherActionOps *o) {
    return aoe_action_set_target_ror(&a->base, t, &o->move);
}

/* 0x00403b00 / 0x00403b30: the carrying graphic while holding the task's
   resource, else the task's moving graphic / the template's standing. */
static int carrying(const AoeGatherActionRor *a) {
    const AoeLiveUnit *u = a->base.unit;
    int16_t in = a->task->resource_in;
    return in != -1 && u->core.storage.carried_resource_type == in &&
           aoe_ftol32(u->core.storage.carried_amount) > 0;
}
static const void *move_graphic(AoeGatherActionRor *a,
                                const AoeGatherActionOps *o) {
    const void *g = carrying(a) ? o->graphic(o->move.context,
                                             a->task->carrying_graphic)
                                : NULL;
    return g ? g : o->graphic(o->move.context, a->task->moving_graphic);
}
static const void *idle_graphic(AoeGatherActionRor *a,
                                const AoeGatherActionOps *o) {
    const void *g = carrying(a) ? o->graphic(o->move.context,
                                             a->task->carrying_graphic)
                                : NULL;
    return g ? g
             : o->graphic(o->move.context,
                          a->base.unit->core.unit_template->standing_graphic);
}
static void show(AoeGatherActionRor *a, const AoeGatherActionOps *o,
                 const void *g) {
    o->move.select_graphic(o->move.context, a->base.unit, g);
}

static void clear_point(AoeGatherActionRor *a) {
    a->base.x = a->base.y = a->base.z = -1.0f;
}

/* Own drop site test shared by +0x30, +0x34 and activation. */
static int own_drop_site_with_load(const AoeGatherActionRor *a,
                                   const AoeLiveUnit *t) {
    const AoeLiveUnit *u = a->base.unit;
    const DatUnit *ut = u->core.unit_template;
    int16_t id = t->core.unit_template ? t->core.unit_template->id : -1;
    return t->runtime_owner_index == u->runtime_owner_index &&
           (id == ut->drop_sites[0] || id == ut->drop_sites[1]) &&
           u->core.storage.carried_amount > 0.0f;
}

int aoe_gather_action_init_target_ror(AoeGatherActionRor *a,
                                      AoeLiveUnit *unit,
                                      const DatUnitTask *task,
                                      AoeLiveUnit *target,
                                      const AoeGatherActionOps *ops) {
    if (!a || !unit || !task || !ops_ok(ops)) return 0;
    memset(a, 0, sizeof(*a));
    a->base.type = 5;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    a->target_template = 0;
    if (!set_target(a, target, ops)) return 0;
    if (target) {
        a->base.x = target->core.position.x38;
        a->base.y = target->core.position.y3c;
        a->base.z = target->core.position.z40;
        a->target_template = target->core.unit_template
                                 ? target->core.unit_template->id
                                 : -1;
    }
    return 1;
}

int aoe_gather_action_init_point_ror(AoeGatherActionRor *a,
                                     AoeLiveUnit *unit,
                                     const DatUnitTask *task, float x,
                                     float y, float z) {
    if (!a || !unit || !task) return 0;
    memset(a, 0, sizeof(*a));
    a->base.type = 5;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    a->target_template = -1;
    return 1;
}

static int failed(AoeGatherActionRor *a, const AoeGatherActionOps *o) {
    return aoe_gather_action_set_state_ror(a, 0x0d, o);
}

int aoe_gather_action_set_state_ror(AoeGatherActionRor *a, uint8_t state,
                                    const AoeGatherActionOps *o) {
    void *c;
    AoeLiveUnit *u;
    if (a && a->base.type == 0x6e)
        return aoe_hunt_action_set_state_ror(a, state, o);
    if (!a || !a->base.unit || !a->task || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    o->sub_clear(c, a);
    a->base.state = state;
    switch (state) {
    case 1: case 0x0d: case 0x0e:
        a->timer = 0.0f;
        show(a, o, idle_graphic(a, o));
        return 1;
    case 2:
        a->timer = 0.0f;
        show(a, o, idle_graphic(a, o));
        if (!set_target(a, NULL, o)) return 0;
        set_drop_site(a, NULL, o);
        clear_point(a);
        return 1;
    case 3:
        notice(a, o, 0x202, 0x261, a->target_template);
        a->timer = -10.0f;
        show(a, o, idle_graphic(a, o));
        return 1;
    case 4:
        a->timer = 0.0f;
        if (!a->base.target) return failed(a, o);
        a->base.x = a->base.target->core.position.x38;
        a->base.y = a->base.target->core.position.y3c;
        a->base.z = a->base.target->core.position.z40;
        if (!o->start_move(c, a, a->base.target, a->task->work_range,
                           move_graphic(a, o), 0))
            return failed(a, o);
        return 1;
    case 6:
        a->timer = 0.0f;
        show(a, o, move_graphic(a, o));
        return 1;
    case 7:
        a->timer = 0.0f;
        if (u->core.storage.carried_resource_type != a->task->resource_in)
            aoe_unit_set_carried_ror(u, a->task->resource_in, 0.0f);
        show(a, o, o->graphic(c, a->task->proceeding_graphic));
        if (a->task->gathering_sound >= 0)
            o->play_sound(c, a->task->gathering_sound);
        return 1;
    case 8:
        if (u->core.storage.carried_amount > 0.0f) {
            if (!a->drop_site) {
                const DatUnit *t = u->core.unit_template;
                AoeLiveUnit *site = NULL;
                if (t->drop_sites[0] != -1)
                    site = o->find_drop_site(
                        c, u, t->drop_sites[0], t->drop_sites[1],
                        u->core.position.x38, u->core.position.y3c,
                        aoe_unit_zone_drop_search_ror(u) ? u : NULL);
                set_drop_site(a, site, o);
            }
            if (!a->drop_site) {
                notice(a, o, 0x203, 0x261, 0);
                return failed(a, o);
            }
            if (!o->start_move(c, a, a->drop_site, a->task->work_range,
                               move_graphic(a, o), 0))
                return failed(a, o);
            return 1;
        }
        if (a->base.target &&
            !(a->base.target->core.storage.carried_amount > 0.0f) &&
            !aoe_unit_keeps_empty_target_ror(
                a->base.target->core.unit_template
                    ? a->base.target->core.unit_template->id
                    : -1) &&
            !set_target(a, NULL, o))
            return 0;
        return aoe_gather_action_set_state_ror(a, 3, o);
    case 0x0b: case 0x0f:
        a->timer = 0.0f;
        if (!o->start_move(c, a, NULL, a->task->work_range,
                           move_graphic(a, o), 1))
            return failed(a, o);
        return 1;
    default:
        return 1;
    }
}

int aoe_gather_action_activate_ror(AoeGatherActionRor *a, int arg,
                                   const AoeGatherActionOps *o) {
    AoeLiveUnit *u;
    if (!a || !a->base.unit || !a->task || !ops_ok(o)) return 0;
    u = a->base.unit;
    if (u->core.storage.carried_resource_type != a->task->resource_in)
        aoe_unit_set_carried_ror(u, a->task->resource_in, 0.0f);
    if (!arg || !a->base.target)
        return aoe_gather_action_set_state_ror(a, 3, o);
    if (own_drop_site_with_load(a, a->base.target)) {
        set_drop_site(a, a->base.target, o);
        if (!set_target(a, NULL, o)) return 0;
        clear_point(a);
        return aoe_gather_action_set_state_ror(a, 8, o);
    }
    return aoe_gather_action_set_state_ror(a, 4, o);
}

int aoe_gather_action_stop_ror(AoeGatherActionRor *a,
                               const AoeGatherActionOps *o) {
    return aoe_gather_action_set_state_ror(a, 2, o);
}

static int walk_to_point(AoeGatherActionRor *a, float x, float y, float z,
                         const AoeGatherActionOps *o) {
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    if (!set_target(a, NULL, o)) return 0;
    return aoe_gather_action_set_state_ror(a, 0x0f, o);
}

int aoe_gather_action_deliver_ror(AoeGatherActionRor *a, AoeLiveUnit *t,
                                  float x, float y, float z,
                                  const AoeGatherActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    if (!t || !own_drop_site_with_load(a, t))
        return walk_to_point(a, x, y, z, o);
    if (t == a->drop_site && a->base.state == 8) return 1;
    if (!set_target(a, NULL, o)) return 0;
    clear_point(a);
    set_drop_site(a, t, o);
    return aoe_gather_action_set_state_ror(a, 8, o);
}

int aoe_gather_action_retarget_ror(AoeGatherActionRor *a, AoeLiveUnit *t,
                                   float x, float y, float z,
                                   const AoeGatherActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    if (!t) return walk_to_point(a, x, y, z, o);
    if (t == a->base.target &&
        (a->base.state == 4 || a->base.state == 6 || a->base.state == 7))
        return 1;
    if (own_drop_site_with_load(a, t)) {
        a->timer = 0.0f;
        if (!set_target(a, NULL, o)) return 0;
        clear_point(a);
        set_drop_site(a, t, o);
        return aoe_gather_action_set_state_ror(a, 8, o);
    }
    if (!set_target(a, t, o)) return 0;
    a->base.x = t->core.position.x38;
    a->base.y = t->core.position.y3c;
    a->base.z = t->core.position.z40;
    set_drop_site(a, NULL, o);
    return aoe_gather_action_set_state_ror(a, 4, o);
}

/* State 7 (0x00403738): one tick of work on the target. */
static int work(AoeGatherActionRor *a, const AoeGatherActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    const DatUnit *ut = u->core.unit_template;
    double amount, mult, cap, gained, space;
    float cap_f, gained_f;
    if (!t) {
        return aoe_gather_action_set_state_ror(
            a, u->core.storage.carried_amount > 0.0f ? 8 : 3, o);
    }
    if (o->instant_gather_option)
        amount = t->core.storage.carried_amount;
    else
        amount = (double)o->tick_seconds(c) * ut->work_rate *
                 a->task->work_value1;
    if (amount > t->core.storage.carried_amount)
        amount = t->core.storage.carried_amount;
    mult = a->task->resource_multiplier != -1
               ? o->player_attribute(c, u, a->task->resource_multiplier)
               : 1.0;
    cap_f = (float)((double)ut->resource_capacity * mult);
    cap = cap_f;
    gained_f = (float)(mult * amount);
    gained = gained_f;
    space = cap - u->core.storage.carried_amount;
    if (gained > space) {
        double excess = gained - space;
        gained_f = (float)(gained - excess);
        amount -= excess / mult;
    }
    aoe_unit_add_carried_ror(t, (float)-amount, 1, 1);
    aoe_unit_add_carried_ror(u, gained_f, 1, 0);
    if (!(u->core.storage.carried_amount < cap_f) ||
        (!(t->core.storage.carried_amount > 0.0f) &&
         !aoe_unit_keeps_empty_target_ror(
             t->core.unit_template ? t->core.unit_template->id : -1)))
        return aoe_gather_action_set_state_ror(a, 8, o);
    return 1;
}

/* State 8 arrival (0x00403913): deposit at a drop site within 1.0. */
static int arrive(AoeGatherActionRor *a, const AoeGatherActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit;
    int16_t type;
    if (!a->drop_site) {
        a->timer = 0.0f;
        return aoe_gather_action_set_state_ror(a, 8, o);
    }
    if (o->distance(c, u, a->drop_site) > 1.0) {
        a->timer = -3.0f;
        u->animation_rate = 0.0f; /* +0x1ec = 0x0044b9a0: +0x88 = 0 */
        return 1;
    }
    if (a->task->deposit_sound >= 0)
        o->play_sound(c, a->task->deposit_sound);
    type = a->task->resource_out;
    if (type == -1) type = u->core.storage.carried_resource_type;
    o->deposit(c, u, type, u->core.storage.carried_amount, 1);
    aoe_unit_set_carried_ror(u, -1, 0.0f);
    if (a->base.target) {
        set_drop_site(a, NULL, o);
        return aoe_gather_action_set_state_ror(a, 4, o);
    }
    if (a->base.x != -1.0f && a->base.y != -1.0f)
        return aoe_gather_action_set_state_ror(a, 0x0b, o);
    return aoe_gather_action_set_state_ror(
        a, o->owner_kind(c, u) == 1 ? 1 : 2, o);
}

int aoe_gather_action_update_ror(AoeGatherActionRor *a,
                                 const AoeGatherActionOps *o,
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
        set_drop_site(a, NULL, o);
    if (a->base.target && a->base.target->core.state >= 7 &&
        !set_target(a, NULL, o))
        return 0;
    if (a->drop_site && a->drop_site->core.state >= 7)
        set_drop_site(a, NULL, o);
    t = a->base.target;
    if (t && t->runtime_owner_index != 0 &&
        t->runtime_owner_index != u->runtime_owner_index) {
        if (!aoe_gather_action_set_state_ror(a, 1, o)) return 0;
        notice(a, o, 0x1fb, 0x261, 0);
        *result = 4;
        return 1;
    }
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 3:
        if (a->timer < 0.0f) {
            double v = (double)o->tick_seconds(c) + a->timer;
            a->timer = (float)v;
            if (v > 0.0) {
                a->timer = -10.0f;
                notice(a, o, 0x202, 0x261, a->target_template);
            }
        }
        return 1;
    case 4:
        switch (o->sub_update(c, a)) {
        case 1: case 2:
            return aoe_gather_action_set_state_ror(a, 6, o);
        case 3:
            return aoe_gather_action_set_state_ror(
                a, o->owner_kind(c, u) == 1 ? 0x0d : 3, o);
        case 4: case 5:
            if (!set_target(a, NULL, o)) return 0;
            return aoe_gather_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 6:
        if (!t) return aoe_gather_action_set_state_ror(a, 3, o);
        if (!o->face(c, u, t)) return 1;
        if (!(t->core.storage.carried_amount > 0.0f) &&
            !aoe_unit_keeps_empty_target_ror(
                t->core.unit_template ? t->core.unit_template->id : -1)) {
            if (!set_target(a, NULL, o)) return 0;
            return aoe_gather_action_set_state_ror(a, 3, o);
        }
        return aoe_gather_action_set_state_ror(a, 7, o);
    case 7:
        return work(a, o);
    case 8:
        if (a->timer < 0.0f) {
            double v = (double)o->tick_seconds(c) + a->timer;
            a->timer = (float)v;
            if (!(v > 0.0)) return 1;
            a->timer = 0.0f;
            return aoe_gather_action_set_state_ror(a, 8, o);
        }
        switch (o->sub_update(c, a)) {
        case 1: case 2:
            return arrive(a, o);
        case 3: case 4: case 5:
            a->timer = -3.0f;
            u->animation_rate = 0.0f; /* +0x1ec */
            return 1;
        default:
            return 1;
        }
    case 0x0b: case 0x0f:
        switch (o->sub_update(c, a)) {
        case 1: case 2: case 5:
            if (a->base.state == 0x0b)
                return aoe_gather_action_set_state_ror(a, 3, o);
            return aoe_gather_action_set_state_ror(
                a, o->owner_kind(c, u) == 1 ? 1 : 2, o);
        case 3:
            return aoe_gather_action_set_state_ror(
                a, o->owner_kind(c, u) == 1 ? 0x0d : 3, o);
        case 4:
            if (!set_target(a, NULL, o)) return 0;
            return aoe_gather_action_set_state_ror(
                a, a->base.state == 0x0b ? 0x0b : 0x0f, o);
        default:
            return 1;
        }
    case 0x0d:
        notice(a, o, 0x1f9, 0x261, 0);
        if (!aoe_gather_action_set_state_ror(
                a, o->owner_kind(c, u) == 1 ? 1 : 2, o))
            return 0;
        *result = 3;
        return 1;
    default:
        return 1;
    }
}

/* ---- hunting (0x6e) ------------------------------------------------- */

static int hunt_state(AoeGatherActionRor *a, uint8_t st,
                      const AoeGatherActionOps *o) {
    return aoe_hunt_action_set_state_ror(a, st, o);
}

static void hunt_notice(AoeGatherActionRor *a, const AoeGatherActionOps *o,
                        int32_t code, int32_t argument) {
    notice(a, o, code, 0x265, argument);
}

/* Owner kind 0 idles (2); any other finishes (1). */
static int hunt_rest(AoeGatherActionRor *a, const AoeGatherActionOps *o) {
    return hunt_state(a, o->owner_kind(o->move.context, a->base.unit) == 0
                             ? 2 : 1, o);
}

int aoe_hunt_action_init_target_ror(AoeGatherActionRor *a, AoeLiveUnit *unit,
                                    const DatUnitTask *task,
                                    AoeLiveUnit *target,
                                    const AoeGatherActionOps *ops) {
    if (!a || !unit || !task || !ops_ok(ops)) return 0;
    memset(a, 0, sizeof(*a));
    a->base.type = 0x6e;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    if (!set_target(a, target, ops)) return 0;
    if (target && target->core.state < 7) {
        a->base.x = target->core.position.x38;
        a->base.y = target->core.position.y3c;
        a->base.z = target->core.position.z40;
        a->target_template = target->core.unit_template
                                 ? target->core.unit_template->id
                                 : -1;
    }
    return 1;
}

int aoe_hunt_action_init_point_ror(AoeGatherActionRor *a, AoeLiveUnit *unit,
                                   const DatUnitTask *task, float x, float y,
                                   float z) {
    if (!aoe_gather_action_init_point_ror(a, unit, task, x, y, z)) return 0;
    a->base.type = 0x6e;
    return 1;
}

int aoe_hunt_action_set_state_ror(AoeGatherActionRor *a, uint8_t state,
                                  const AoeGatherActionOps *o) {
    void *c;
    AoeLiveUnit *u, *t;
    if (!a || !a->base.unit || !a->task || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    t = a->base.target;
    o->sub_clear(c, a);
    a->base.state = state;
    switch (state) {
    case 1: case 0x0d: case 0x0e:
        a->timer = 0.0f;
        show(a, o, idle_graphic(a, o));
        return 1;
    case 2:
        show(a, o, idle_graphic(a, o));
        if (!set_target(a, NULL, o)) return 0;
        set_drop_site(a, NULL, o);
        a->timer = 0.0f;
        clear_point(a);
        return 1;
    case 3:
        hunt_notice(a, o, 0x202, a->target_template);
        show(a, o, idle_graphic(a, o));
        a->timer = -10.0f;
        return 1;
    case 4:
        a->timer = 0.0f;
        if (!t) return hunt_state(a, 0x0d, o);
        a->base.x = t->core.position.x38;
        a->base.y = t->core.position.y3c;
        a->base.z = t->core.position.z40;
        if (t->core.state <= 2) return hunt_state(a, 0x0c, o);
        if (!o->start_move(c, a, t, a->task->work_range, move_graphic(a, o),
                           0))
            return hunt_state(a, 0x0d, o);
        return 1;
    case 6:
        a->timer = 0.0f;
        show(a, o, move_graphic(a, o));
        return 1;
    case 7:
        a->timer = 0.0f;
        if (u->core.storage.carried_resource_type != a->task->resource_in)
            aoe_unit_set_carried_ror(u, a->task->resource_in, 0.0f);
        show(a, o, o->graphic(c, a->task->working_graphic));
        if (a->task->gathering_sound >= 0)
            o->play_sound(c, a->task->gathering_sound);
        return 1;
    case 8:
        if (u->core.storage.carried_amount > 0.0f) {
            if (!a->drop_site) {
                const DatUnit *ut = u->core.unit_template;
                set_drop_site(a,
                              ut->drop_sites[0] != -1
                                  ? o->find_drop_site(
                                        c, u, ut->drop_sites[0],
                                        ut->drop_sites[1],
                                        u->core.position.x38,
                                        u->core.position.y3c, u)
                                  : NULL,
                              o);
            }
            if (!a->drop_site) {
                hunt_notice(a, o, 0x203, 0);
                return hunt_rest(a, o);
            }
            if (!o->start_move(c, a, a->drop_site, a->task->work_range,
                               move_graphic(a, o), 0))
                return hunt_state(a, 0x0d, o);
            return 1;
        }
        if (t && !(t->core.storage.carried_amount > 0.0f) &&
            !set_target(a, NULL, o))
            return 0;
        return hunt_state(a, 3, o);
    case 0x0b: case 0x0f:
        a->timer = 0.0f;
        if (!o->start_move(c, a, NULL, a->task->work_range,
                           move_graphic(a, o), 1))
            return hunt_state(a, 0x0d, o);
        return 1;
    case 0x0c:
        a->timer = 0.0f;
        if (!o->start_attack(c, a, move_graphic(a, o),
                             o->graphic(c, a->task->proceeding_graphic),
                             a->task->work_range))
            return hunt_state(a, 0x0d, o);
        return 1;
    default:
        return 1;
    }
}

int aoe_hunt_action_stop_ror(AoeGatherActionRor *a,
                             const AoeGatherActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    return hunt_rest(a, o);
}

int aoe_hunt_action_retarget_ror(AoeGatherActionRor *a, AoeLiveUnit *t,
                                 float x, float y, float z,
                                 const AoeGatherActionOps *o) {
    if (!a || !a->base.unit || !ops_ok(o)) return 0;
    if (!t) {
        a->base.x = x;
        a->base.y = y;
        a->base.z = z;
        if (!set_target(a, NULL, o)) return 0;
        return hunt_state(a, 0x0f, o);
    }
    if (own_drop_site_with_load(a, t)) {
        if (!set_target(a, NULL, o)) return 0;
        clear_point(a);
        a->timer = 0.0f;
        set_drop_site(a, t, o);
        return hunt_state(a, 8, o);
    }
    if (!set_target(a, t, o)) return 0;
    set_drop_site(a, NULL, o);
    return hunt_state(a, 4, o);
}

/* State 7 (0x004b3c5d). */
static int hunt_work(AoeGatherActionRor *a, const AoeGatherActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    const DatUnit *ut = u->core.unit_template;
    double amount, mult, space, gained;
    float cap_f, gained_f;
    if (!t)
        return hunt_state(a, u->core.storage.carried_amount > 0.0f ? 8 : 3,
                          o);
    if (o->instant_gather_option)
        amount = t->core.storage.carried_amount;
    else
        amount = (double)o->tick_seconds(c) * ut->work_rate *
                 a->task->work_value1;
    if (amount > t->core.storage.carried_amount)
        amount = t->core.storage.carried_amount;
    mult = a->task->resource_multiplier != -1
               ? o->player_attribute(c, u, a->task->resource_multiplier)
               : 1.0;
    cap_f = (float)((double)ut->resource_capacity * mult);
    gained_f = (float)(mult * amount);
    space = (double)cap_f - u->core.storage.carried_amount;
    gained = gained_f;
    if (gained > space) {
        double excess = gained - space;
        gained_f = (float)(gained - excess);
        amount -= excess / mult;
    }
    aoe_unit_add_carried_ror(t, (float)-amount, 1, 1);
    aoe_unit_add_carried_ror(u, gained_f, 1, 0);
    if (!(u->core.storage.carried_amount < cap_f) ||
        !(t->core.storage.carried_amount > 0.0f))
        return hunt_state(a, 8, o);
    return 1;
}

/* State 8 arrival (0x004b3e28). */
static int hunt_arrive(AoeGatherActionRor *a, const AoeGatherActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit;
    int16_t type;
    if (!a->drop_site) return hunt_state(a, 8, o);
    if (!(o->distance(c, u, a->drop_site) <= 1.0)) {
        a->timer = -3.0f;
        u->animation_rate = 0.0f; /* +0x1ec */
        return 1;
    }
    if (a->task->deposit_sound >= 0)
        o->play_sound(c, a->task->deposit_sound);
    type = a->task->resource_out;
    if (type == -1) type = u->core.storage.carried_resource_type;
    o->deposit(c, u, type, u->core.storage.carried_amount, 0);
    aoe_unit_set_carried_ror(u, -1, 0.0f);
    if (a->base.target) {
        set_drop_site(a, NULL, o);
        return hunt_state(a, 4, o);
    }
    if (a->base.x != -1.0f && a->base.y != -1.0f)
        return hunt_state(a, 0x0b, o);
    return hunt_rest(a, o);
}

int aoe_hunt_action_update_ror(AoeGatherActionRor *a,
                               const AoeGatherActionOps *o,
                               uint8_t *result) {
    void *c;
    AoeLiveUnit *u, *t;
    int32_t kind;
    if (!a || !a->base.unit || !a->task || !result || !ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    *result = 0;
    if (a->base.target_id != -1 && !o->move.resolve(c, a->base.target_id) &&
        !set_target(a, NULL, o))
        return 0;
    if (a->base.secondary_id != -1 &&
        !o->move.resolve(c, a->base.secondary_id))
        set_drop_site(a, NULL, o);
    if (a->base.target && a->base.target->core.state >= 7 &&
        !set_target(a, NULL, o))
        return 0;
    if (a->drop_site && a->drop_site->core.state >= 7)
        set_drop_site(a, NULL, o);
    t = a->base.target;
    switch (a->base.state) {
    case 1:
        if (o->owner_kind(c, u) == 0 && !hunt_state(a, 2, o)) return 0;
        *result = 1;
        return 1;
    case 3:
        if (a->timer < 0.0f) {
            double v = (double)o->tick_seconds(c) + a->timer;
            a->timer = (float)v;
            if (v > 0.0) {
                a->timer = -10.0f;
                hunt_notice(a, o, 0x202, a->target_template);
            }
        }
        return 1;
    case 4:
        switch (o->sub_update(c, a)) {
        case 1: case 2:
            return hunt_state(a, 6, o);
        case 3:
            return hunt_state(a, o->owner_kind(c, u) == 1 ? 0x0d : 3, o);
        case 4: case 5:
            if (!set_target(a, NULL, o)) return 0;
            return hunt_state(a, 0x0b, o);
        default:
            return 1;
        }
    case 6:
        if (!t) return hunt_state(a, 3, o);
        if (!o->face(c, u, t)) return 1;
        if (t->core.storage.carried_amount > 0.0f) return hunt_state(a, 7, o);
        if (!set_target(a, NULL, o)) return 0;
        return hunt_state(a, 3, o);
    case 7:
        return hunt_work(a, o);
    case 8:
        if (a->timer < 0.0f) {
            double v = (double)o->tick_seconds(c) + a->timer;
            a->timer = (float)v;
            if (!(v > 0.0)) return 1;
            a->timer = 0.0f;
            set_drop_site(a, NULL, o);
            return hunt_state(a, 8, o);
        }
        switch (o->sub_update(c, a)) {
        case 1: case 2:
            return hunt_arrive(a, o);
        case 3: case 4: case 5:
            a->timer = -3.0f;
            u->animation_rate = 0.0f;
            return 1;
        default:
            return 1;
        }
    case 0x0b: case 0x0f:
        switch (o->sub_update(c, a)) {
        case 1: case 2: case 5:
            if (a->base.state == 0x0b) return hunt_state(a, 3, o);
            return hunt_rest(a, o);
        case 3:
            kind = o->owner_kind(c, u);
            return hunt_state(a, kind == 1 ? 0x0d : 3, o);
        case 4:
            if (!set_target(a, NULL, o)) return 0;
            return hunt_state(a, a->base.state == 0x0b ? 0x0b : 0x0f, o);
        default:
            return 1;
        }
    case 0x0c:
        switch (o->sub_update(c, a)) {
        case 1: case 2: case 5:
            return hunt_state(a, t ? 4 : 3, o);
        case 3:
            return hunt_state(a, 3, o);
        case 4:
            if (!set_target(a, NULL, o)) return 0;
            return hunt_state(a, 0x0b, o);
        default:
            return 1;
        }
    case 0x0d:
        hunt_notice(a, o, 0x1f9, 0);
        *result = 3;
        return 1;
    default:
        return 1;
    }
}
