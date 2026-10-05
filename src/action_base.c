#include "action_base.h"

#include <string.h>

#include "aoe_ftol.h"
#include "unit_garrison.h"

static void init(AoeBaseActionRor *a, int16_t type, AoeLiveUnit *unit,
                 const DatUnitTask *task, float x, float y, float z) {
    memset(a, 0, sizeof(*a));
    a->base.type = type;
    a->base.unit = unit;
    a->base.target_id = -1;
    a->base.secondary_id = -1;
    a->task = task;
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
}

static void show(AoeBaseActionRor *a, const AoeBaseActionOps *o,
                 const void *g) {
    o->move.select_graphic(o->move.context, a->base.unit, g);
}
static const void *standing(AoeBaseActionRor *a, const AoeBaseActionOps *o) {
    return o->graphic(o->move.context,
                      a->base.unit->core.unit_template->standing_graphic);
}
static void notice(AoeBaseActionRor *a, const AoeBaseActionOps *o,
                   int32_t code, int32_t value) {
    o->move.unit_notice(o->move.context, a->base.unit, code, value, 0);
}
static int set_target(AoeBaseActionRor *a, AoeLiveUnit *t,
                      const AoeBaseActionOps *o) {
    return aoe_action_set_target_ror(&a->base, t, &o->move);
}
/* rand() * n / 32767, truncated. */
static int32_t scaled(const AoeBaseActionOps *o, int32_t n) {
    return (int32_t)o->rand(o->move.context) * n / 32767;
}
static int ops_ok(const AoeBaseActionOps *o) {
    return o && o->move.select_graphic && o->graphic && o->tick_seconds &&
           o->start_move && o->sub_update && o->sub_clear;
}

/* ---- fly (10) ------------------------------------------------------- */

int aoe_fly_action_init_ror(AoeBaseActionRor *a, AoeLiveUnit *unit,
                            const DatUnitTask *task, float x, float y,
                            float z) {
    if (!a || !unit || !task) return 0;
    init(a, 0x0a, unit, task, x, y, z);
    return 1;
}

int aoe_fly_action_set_state_ror(AoeBaseActionRor *a, uint8_t st,
                                 const AoeBaseActionOps *o) {
    void *c;
    const void *g;
    if (!a || !ops_ok(o) || !o->rand) return 0;
    c = o->move.context;
    a->base.state = st;
    if (st != 4 && st != 6) {
        o->sub_clear(c, &a->base);
        return 1;
    }
    a->timer = (float)(scaled(o, 10) + 5);
    g = o->graphic(c, st == 4 ? a->task->moving_graphic
                              : a->task->proceeding_graphic);
    show(a, o, g ? g : standing(a, o));
    return 1;
}

int aoe_fly_action_activate_ror(AoeBaseActionRor *a, int arg,
                                const AoeBaseActionOps *o) {
    AoeLiveUnit *u;
    if (!a || !o || !o->place) return 0;
    u = a->base.unit;
    if (arg) o->place(o->move.context, u, u->core.position.x38,
                      u->core.position.y3c, 6.0f);
    return aoe_fly_action_set_state_ror(a, 3, o);
}

int aoe_fly_action_update_ror(AoeBaseActionRor *a, const AoeBaseActionOps *o,
                              uint8_t *result) {
    void *c;
    if (!a || !result || !ops_ok(o) || !o->rand || !o->map_width ||
        !o->map_height)
        return 0;
    c = o->move.context;
    *result = 0;
    if (a->timer > 0.0f) a->timer = a->timer - o->tick_seconds(c);
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 3: {
        AoeLiveUnit *u = a->base.unit;
        a->base.x = (float)scaled(o, o->map_width(c));
        a->base.y = (float)scaled(o, o->map_height(c));
        a->base.z = u->core.position.z40;
        if (!o->start_move(c, &a->base, a->task->work_range,
                           u->graphic.current_graphic, -1))
            return aoe_fly_action_set_state_ror(a, 3, o);
        return aoe_fly_action_set_state_ror(a, 4, o);
    }
    case 4: case 6:
        if (!(a->timer > 0.0f) &&
            !aoe_fly_action_set_state_ror(a, a->base.state == 4 ? 6 : 4, o))
            return 0;
        if (o->sub_update(c, &a->base) == 1)
            return aoe_fly_action_set_state_ror(a, 3, o);
        return 1;
    default:
        return 1;
    }
}

/* ---- enter (3) ------------------------------------------------------ */

static int enter_ops_ok(const AoeBaseActionOps *o) {
    return ops_ok(o) && o->move.resolve && o->move.unit_notice &&
           o->move.clear_secondary && o->distance && o->allied &&
           o->owner_kind && o->head_state && o->head_secondary &&
           o->head_destination && o->move_to && o->enter &&
           o->line.terrain_at && o->line.fits;
}

/* A beach tile on the line to the target: walk to its centre and call
   the target to its corner (forced, range `range`); none: call the target
   to the unit (0x0040250e / 0x00402955). Returns the state-4 point. */
static int call_ashore(AoeBaseActionRor *a, const AoeBaseActionOps *o,
                       float range) { /* constructor 0x0040250e */
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit, *t = a->base.target;
    int32_t from[2], to[2], tile[2];
    from[0] = aoe_ftol32(u->core.position.x38);
    from[1] = aoe_ftol32(u->core.position.y3c);
    to[0] = aoe_ftol32(t->core.position.x38);
    to[1] = aoe_ftol32(t->core.position.y3c);
    if (aoe_unit_line_terrain_point_ror(u, from, to, tile, 2, 2, 1,
                                        &o->line) == 1) {
        a->base.x = (float)tile[0] + 0.5f;
        a->base.y = (float)tile[1] + 0.5f;
        return o->move_to(c, t, (float)tile[0], (float)tile[1],
                          t->core.position.z40, range, 1);
    }
    a->base.x = t->core.position.x38;
    a->base.y = t->core.position.y3c;
    return o->move_to(c, t, u->core.position.x38, u->core.position.y3c,
                      t->core.position.z40, 0.5f, 1);
}

int aoe_enter_action_init_ror(AoeBaseActionRor *a, AoeLiveUnit *unit,
                              const DatUnitTask *task, AoeLiveUnit *t,
                              const AoeBaseActionOps *o) {
    void *c;
    int32_t kind;
    if (!a || !unit || !task || !enter_ops_ok(o)) return 0;
    c = o->move.context;
    init(a, 3, unit, task, -1.0f, -1.0f, -1.0f);
    if (!set_target(a, t, o)) return 0;
    if (!t) return 1;
    a->base.x = t->core.position.x38;
    a->base.y = t->core.position.y3c;
    a->base.z = t->core.position.z40;
    kind = o->owner_kind(c, unit);
    if ((o->head_state(c, t) == 2 &&
         ((aoe_unit_has_room_ror(t) && kind == 0) || kind == 1)) ||
        (o->head_state(c, t) == 0x0b && o->head_secondary(c, t) == 0)) {
        (void)call_ashore(a, o, 0.5f);
        return 1;
    }
    if (o->head_state(c, t) == 0x0b)
        o->head_destination(c, t, &a->base.x, &a->base.y);
    return 1;
}

int aoe_enter_action_set_state_ror(AoeBaseActionRor *a, uint8_t st,
                                   const AoeBaseActionOps *o) {
    void *c;
    if (!a || !ops_ok(o)) return 0;
    c = o->move.context;
    o->sub_clear(c, &a->base);
    a->base.state = st;
    switch (st) {
    case 1: case 0x0d: case 0x0e:
        show(a, o, standing(a, o));
        return 1;
    case 2:
        a->timer = 1.0f;
        show(a, o, standing(a, o));
        return 1;
    case 4:
        if (!o->start_move(c, &a->base, a->task->work_range,
                           o->graphic(c, a->task->moving_graphic), 0))
            return aoe_enter_action_set_state_ror(a, 0x0d, o);
        return 1;
    default:
        return 1;
    }
}

int aoe_enter_action_activate_ror(AoeBaseActionRor *a,
                                  const AoeBaseActionOps *o) {
    return aoe_enter_action_set_state_ror(a, 4, o);
}

/* Within task range + 1.0: enter (0x00402884 / 0x004028f5). */
static int try_enter(AoeBaseActionRor *a, const AoeBaseActionOps *o,
                     uint8_t *result, int *entered) {
    void *c = o->move.context;
    *entered = 0;
    if (!(o->distance(c, a->base.unit, a->base.target) - 1.0 <=
          (double)a->task->work_range))
        return 1;
    *entered = 1;
    (void)o->enter(c, a->base.unit, a->base.target);
    if (!a->base.quiet) notice(a, o, 0x1fa, 0x269);
    if (!aoe_enter_action_set_state_ror(a, 1, o)) return 0;
    *result = 2;
    return 1;
}

int aoe_enter_action_update_ror(AoeBaseActionRor *a,
                                const AoeBaseActionOps *o,
                                uint8_t *result) {
    void *c;
    AoeLiveUnit *t;
    int entered;
    if (!a || !result || !enter_ops_ok(o)) return 0;
    c = o->move.context;
    *result = 0;
    if (a->base.target_id != -1 && !o->move.resolve(c, a->base.target_id))
        goto lost;
    if (a->base.secondary_id != -1 &&
        !o->move.resolve(c, a->base.secondary_id))
        o->move.clear_secondary(c, &a->base);
    t = a->base.target;
    if (t && (t->core.state >= 3 || !o->allied(c, a->base.unit, t)))
        goto lost;
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 2:
        a->timer = a->timer - o->tick_seconds(c);
        if (a->timer > 0.0f) return 1;
        if (!try_enter(a, o, result, &entered)) return 0;
        if (entered) return 1;
        if (o->head_state(c, t) == 2) {
            AoeLiveUnit *u = a->base.unit;
            int32_t from[2], to[2], tile[2];
            from[0] = aoe_ftol32(u->core.position.x38);
            from[1] = aoe_ftol32(u->core.position.y3c);
            to[0] = aoe_ftol32(t->core.position.x38);
            to[1] = aoe_ftol32(t->core.position.y3c);
            if (aoe_unit_line_terrain_point_ror(u, from, to, tile, 2, 2, 1,
                                                &o->line) == 1) {
                float px = (float)tile[0] + 0.5f, py = (float)tile[1] + 0.5f;
                if (px == a->base.x && py == a->base.y) {
                    a->base.x = t->core.position.x38;
                    a->base.y = t->core.position.y3c;
                } else {
                    a->base.x = px;
                    a->base.y = py;
                    (void)o->move_to(c, t, (float)tile[0], (float)tile[1],
                                     t->core.position.z40, 0.0f, 1);
                }
            } else {
                a->base.x = t->core.position.x38;
                a->base.y = t->core.position.y3c;
                (void)o->move_to(c, t, u->core.position.x38,
                                 u->core.position.y3c, t->core.position.z40,
                                 0.5f, 1);
            }
        } else if (o->head_state(c, t) == 0x0b) {
            o->head_destination(c, t, &a->base.x, &a->base.y);
        }
        return aoe_enter_action_set_state_ror(a, 4, o);
    case 4:
        if (!t || (t->core.state >= 3 && t->core.state != 6)) {
            if (!set_target(a, NULL, o)) return 0;
            return aoe_enter_action_set_state_ror(a, 0x0d, o);
        }
        switch (o->sub_update(c, &a->base)) {
        case 1: case 2:
            if (!try_enter(a, o, result, &entered)) return 0;
            if (entered) return 1;
            return aoe_enter_action_set_state_ror(a, 2, o);
        case 3:
            return aoe_enter_action_set_state_ror(a, 2, o);
        case 4: case 5:
            if (!set_target(a, NULL, o)) return 0;
            return aoe_enter_action_set_state_ror(a, 0x0d, o);
        default:
            return 1;
        }
    case 0x0d:
        notice(a, o, 0x1f9, 0x269);
        *result = 3;
        return 1;
    default:
        return 1;
    }
lost:
    if (!aoe_enter_action_set_state_ror(a, 1, o)) return 0;
    if (!set_target(a, NULL, o)) return 0;
    notice(a, o, 0x1fb, 0x269);
    *result = 4;
    return 1;
}

/* ---- explore (4) ---------------------------------------------------- */

int aoe_explore_action_init_ror(AoeBaseActionRor *a, AoeLiveUnit *unit,
                                const DatUnitTask *task, float x, float y,
                                float z) {
    if (!a || !unit) return 0;
    init(a, 4, unit, task, x, y, z);
    return 1;
}

int aoe_explore_action_set_state_ror(AoeBaseActionRor *a, uint8_t st,
                                     const AoeBaseActionOps *o) {
    void *c;
    if (!a || !ops_ok(o) || !o->move.unit_notice) return 0;
    c = o->move.context;
    o->sub_clear(c, &a->base);
    a->base.state = st;
    switch (st) {
    case 1: case 0x0d: case 0x0e:
        show(a, o, standing(a, o));
        return 1;
    case 2:
        show(a, o, standing(a, o));
        if (!set_target(a, NULL, o)) return 0;
        if (o->move.clear_secondary) o->move.clear_secondary(c, &a->base);
        a->base.x = a->base.y = a->base.z = -1.0f;
        return 1;
    case 3:
        notice(a, o, 0x202, 0x25d);
        show(a, o, standing(a, o));
        return 1;
    case 0x0b: {
        const void *g = a->task ? o->graphic(c, a->task->moving_graphic)
                                : NULL;
        if (!g)
            g = o->graphic(c, a->base.unit->core.unit_template->walking_graphic);
        if (!o->start_move(c, &a->base, 0.0f, g, 1))
            return aoe_explore_action_set_state_ror(a, 0x0d, o);
        return 1;
    }
    default:
        return 1;
    }
}

int aoe_explore_action_activate_ror(AoeBaseActionRor *a,
                                    const AoeBaseActionOps *o) {
    if (!a) return 0;
    return aoe_explore_action_set_state_ror(
        a, a->base.x == -1.0f ? 3 : 0x0b, o);
}

int aoe_explore_action_retarget_ror(AoeBaseActionRor *a, float x, float y,
                                    float z, const AoeBaseActionOps *o) {
    if (!a) return 0;
    if (!(x < 0.0f) && !(y < 0.0f)) {
        a->base.x = x;
        a->base.y = y;
        a->base.z = z;
        return aoe_explore_action_set_state_ror(a, 0x0b, o);
    }
    return aoe_explore_action_set_state_ror(a, 3, o);
}

int aoe_explore_action_update_ror(AoeBaseActionRor *a,
                                  const AoeBaseActionOps *o,
                                  uint8_t *result) {
    void *c;
    if (!a || !result || !ops_ok(o)) return 0;
    c = o->move.context;
    *result = 0;
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 0x0b:
        switch (o->sub_update(c, &a->base)) {
        case 1: case 2: case 5:
            return aoe_explore_action_set_state_ror(a, 3, o);
        case 3:
            return aoe_explore_action_set_state_ror(a, 0x0d, o);
        case 4:
            if (!set_target(a, NULL, o)) return 0;
            return aoe_explore_action_set_state_ror(a, 0x0b, o);
        default:
            return 1;
        }
    case 0x0d:
        notice(a, o, 0x1f9, 0x25d);
        if (!aoe_explore_action_set_state_ror(a, 2, o)) return 0;
        *result = 3;
        return 1;
    default:
        return 1;
    }
}

/* ---- transport (12) ------------------------------------------------- */

static int transport_ops_ok(const AoeBaseActionOps *o) {
    return ops_ok(o) && o->distance_to_point && o->zone_at && o->zone_near &&
           o->terrain_at && o->unload && o->contents && o->owner_notice &&
           o->map_width && o->map_height;
}

int aoe_transport_action_init_ror(AoeBaseActionRor *a, AoeLiveUnit *unit,
                                  const DatUnitTask *task, float x, float y,
                                  float z) {
    if (!a || !unit) return 0;
    init(a, 0x0c, unit, task, x, y, z);
    return 1;
}

int aoe_transport_action_set_state_ror(AoeBaseActionRor *a, uint8_t st,
                                       const AoeBaseActionOps *o) {
    void *c;
    if (!a || !ops_ok(o)) return 0;
    c = o->move.context;
    o->sub_clear(c, &a->base);
    a->base.state = st;
    if (st == 2) {
        show(a, o, standing(a, o));
        return 1;
    }
    if (st == 4 || st == 0x0b) {
        const void *g = a->task ? o->graphic(c, a->task->moving_graphic)
                                : standing(a, o);
        if (!o->start_move(c, &a->base, 0.3f, g, 0))
            return aoe_transport_action_set_state_ror(a, 2, o);
    }
    return 1;
}

int aoe_transport_action_activate_ror(AoeBaseActionRor *a, int arg,
                                      const AoeBaseActionOps *o) {
    if (!a) return 0;
    if (arg && !(a->base.x < 0.0f) && !(a->base.y < 0.0f))
        return aoe_transport_action_set_state_ror(a, 0x0b, o);
    return aoe_transport_action_set_state_ror(a, 2, o);
}

/* 0x00406e00: the zone of the action point, or -1 without one. */
static int point_zone(const AoeBaseActionRor *a, const AoeBaseActionOps *o) {
    if (a->base.x == -1.0f) return -1;
    return o->zone_at(o->move.context, a->base.unit,
                      aoe_ftol32(a->base.x), aoe_ftol32(a->base.y));
}

int aoe_transport_action_walk_ror(AoeBaseActionRor *a, float x, float y,
                                  float z, const AoeBaseActionOps *o) {
    if (!a) return 0;
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    a->unload_flag = 0;
    return aoe_transport_action_set_state_ror(a, 0x0b, o);
}

int aoe_transport_action_retarget_ror(AoeBaseActionRor *a, AoeLiveUnit *t,
                                      float x, float y, float z,
                                      const AoeBaseActionOps *o) {
    void *c;
    int zone;
    double d;
    if (!a || !transport_ops_ok(o)) return 0;
    c = o->move.context;
    a->unload_flag = 1;
    zone = o->zone_at(c, a->base.unit, aoe_ftol32(x), aoe_ftol32(y));
    d = o->distance_to_point(c, a->base.unit, x, y, z);
    if (zone != -1 && o->zone_near(c, a->base.unit, zone, 2.0f) == 1 &&
        d <= 2.0) {
        a->base.x = x;
        a->base.y = y;
        a->base.z = z;
        return aoe_transport_action_set_state_ror(a, 6, o);
    }
    if (!(x < 0.0f) && !(y < 0.0f)) {
        a->base.x = x;
        a->base.y = y;
        a->base.z = z;
        return aoe_transport_action_set_state_ror(
            a, t == a->base.unit ? 4 : 0x0b, o);
    }
    return 1;
}

int aoe_transport_shore_ror(const AoeBaseActionRor *a,
                            const AoeBaseActionOps *o) {
    void *c = o->move.context;
    const AoeLiveUnit *u = a->base.unit;
    const DatUnit *t = u->core.unit_template;
    int32_t x0, y0, x1, y1, zone;
    if (a->task && a->task->target_terrain == -1) return 1;
    x0 = aoe_ftol32((double)u->core.position.x38 - t->size[0] - 1.0);
    y0 = aoe_ftol32((double)u->core.position.y3c - t->size[1] - 1.0);
    x1 = aoe_ftol32((double)t->size[0] + u->core.position.x38 + 1.0);
    y1 = aoe_ftol32((double)u->core.position.y3c + t->size[1] + 1.0);
    if (x0 < 0) x0 = 0;
    if (x1 >= o->map_width(c)) x1 = o->map_width(c) - 1;
    if (y0 < 0) y0 = 0;
    if (y1 >= o->map_height(c)) y1 = o->map_height(c) - 1;
    zone = point_zone(a, o);
    for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x) {
            int terrain;
            if (zone != -1 &&
                o->zone_at(c, (AoeLiveUnit *)u, x, y) != zone)
                continue;
            terrain = o->terrain_at(c, x, y);
            if (terrain == 2 || terrain == 4) return 1;
        }
    return 0;
}

/* State 6 (0x004069f7). */
static int unload_here(AoeBaseActionRor *a, const AoeBaseActionOps *o) {
    void *c = o->move.context;
    AoeLiveUnit *u = a->base.unit;
    int zone = point_zone(a, o);
    if (zone != -1 && !o->zone_near(c, u, zone, 2.0f)) {
        o->owner_notice(c, u, 0x207);
        return aoe_transport_action_set_state_ror(a, 2, o);
    }
    if ((int16_t)o->contents(c, u) <= 0)
        return aoe_transport_action_set_state_ror(a, 2, o);
    if (aoe_transport_shore_ror(a, o)) {
        if (o->unload(c, u, point_zone(a, o))) {
            if ((int16_t)o->contents(c, u) <= 0)
                o->owner_notice(c, u, 0x206);
        } else {
            o->owner_notice(c, u, 0x207);
        }
        return aoe_transport_action_set_state_ror(a, 2, o);
    }
    if (a->base.x != -1.0f) {
        int t = o->terrain_at(c, aoe_ftol32(a->base.x),
                              aoe_ftol32(a->base.y));
        if (t == 1 || t == 0x16)
            return aoe_transport_action_set_state_ror(a, 2, o);
    }
    o->owner_notice(c, u, 0x207);
    return aoe_transport_action_set_state_ror(a, 2, o);
}

int aoe_transport_action_update_ror(AoeBaseActionRor *a,
                                    const AoeBaseActionOps *o,
                                    uint8_t *result) {
    void *c;
    uint8_t r;
    if (!a || !result || !transport_ops_ok(o)) return 0;
    c = o->move.context;
    *result = 0;
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 4:
        r = o->sub_update(c, &a->base);
        if (r >= 1 && r <= 5)
            return aoe_transport_action_set_state_ror(a, 6, o);
        return 1;
    case 6:
        return unload_here(a, o);
    case 0x0b:
        r = o->sub_update(c, &a->base);
        if (r >= 1 && r <= 5)
            return aoe_transport_action_set_state_ror(a, 2, o);
        return 1;
    case 0x0d:
        o->move.unit_notice(c, a->base.unit, 0x1f9, 0x266, 0);
        if (!aoe_transport_action_set_state_ror(a, 2, o)) return 0;
        *result = 3;
        return 1;
    default:
        return 1;
    }
}
