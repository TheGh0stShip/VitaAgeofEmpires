#include "action_artifact.h"

#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

static void init_base(AoeMoveActionRor *b, int16_t type, AoeLiveUnit *unit) {
    memset(b, 0, sizeof(*b));
    b->type = type;
    b->unit = unit;
    b->target_id = -1;
    b->secondary_id = -1;
}

/* The tile rectangle of 0x004b1120 / 0x004b2a30: position -/+ (1 + size)
   (unrounded x87 sums), truncated to int16, clamped to the map
   (x to world +0x28 +0x08, y to +0x0c). r = {x0, y0, x1, y1}. */
static int area(const AoeLiveUnit *u, const AoeObjectActionOps *o,
                int32_t r[4]) {
    const DatUnit *t = u->core.unit_template;
    void *c = o->move.context;
    int32_t w = o->map_width(c), h = o->map_height(c);
    r[0] = (int16_t)aoe_ftol32((double)u->core.position.x38 - 1.0 -
                               t->size[0]);
    r[1] = (int16_t)aoe_ftol32((double)u->core.position.y3c - 1.0 -
                               t->size[1]);
    r[2] = (int16_t)aoe_ftol32((double)u->core.position.x38 + t->size[0] +
                               1.0);
    r[3] = (int16_t)aoe_ftol32((double)t->size[1] + u->core.position.y3c +
                               1.0);
    if (r[0] < 0) r[0] = 0;
    if (r[1] < 0) r[1] = 0;
    if (r[2] >= w) r[2] = (int16_t)(w - 1);
    if (r[3] >= h) r[3] = (int16_t)(h - 1);
    return r[1] <= r[3];
}

/* ---- artifact (0x6b) ------------------------------------------------ */

static int artifact_ops_ok(const AoeObjectActionOps *o) {
    return o && o->tick_seconds && o->player_count && o->map_width &&
           o->map_height && o->tile_object && o->start_move &&
           o->sub_update && o->sub_clear && o->graphic && o->change_owner;
}

int aoe_artifact_action_init_ror(AoeArtifactActionRor *a, AoeLiveUnit *unit,
                                 const DatUnitTask *task,
                                 AoeLiveUnit *target) {
    if (!a || !unit || !task) return 0;
    init_base(&a->base, 0x6b, unit);
    a->task = task;
    a->base.target = target; /* raw store, no 0x00407380 */
    return 1;
}

int aoe_artifact_action_init_point_ror(AoeArtifactActionRor *a,
                                       AoeLiveUnit *unit,
                                       const DatUnitTask *task, float x,
                                       float y, float z) {
    if (!a || !unit || !task) return 0;
    init_base(&a->base, 0x6b, unit);
    a->task = task;
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    return 1;
}

static int walk(AoeArtifactActionRor *a, const AoeObjectActionOps *o) {
    void *c = o->move.context;
    o->sub_clear(c, &a->base);
    if (!o->start_move(c, &a->base, o->graphic(c, a->task->moving_graphic)))
        return 1;
    return aoe_artifact_action_set_state_ror(a, 6, o);
}

int aoe_artifact_action_set_state_ror(AoeArtifactActionRor *a, uint8_t st,
                                      const AoeObjectActionOps *o) {
    if (!a || !a->task || !artifact_ops_ok(o)) return 0;
    a->base.state = st;
    switch (st) {
    case 4:
        return walk(a, o);
    case 5: {
        AoeLiveUnit *t = a->base.target;
        o->sub_clear(o->move.context, &a->base);
        if (t) {
            a->base.x = t->core.position.x38;
            a->base.y = t->core.position.y3c;
            a->base.z = t->core.position.z40;
        }
        a->base.target = NULL;
        return walk(a, o);
    }
    case 0x0a:
        a->timer = 3.0f;
        return 1;
    default:
        return 1;
    }
}

int aoe_artifact_action_activate_ror(AoeArtifactActionRor *a, int arg,
                                     const AoeObjectActionOps *o) {
    if (!a) return 0;
    if (!arg) return 1;
    if (a->base.target) return aoe_artifact_action_set_state_ror(a, 5, o);
    if (a->base.x != -1.0f && a->base.y != -1.0f)
        return aoe_artifact_action_set_state_ror(a, 4, o);
    return aoe_artifact_action_set_state_ror(a, 6, o);
}

int aoe_artifact_action_retarget_ror(AoeArtifactActionRor *a, AoeLiveUnit *t,
                                     float x, float y, float z,
                                     const AoeObjectActionOps *o) {
    if (!a) return 0;
    if (t) {
        a->base.target = t;
        return aoe_artifact_action_set_state_ror(a, 5, o);
    }
    a->base.x = x;
    a->base.y = y;
    a->base.z = z;
    return aoe_artifact_action_set_state_ror(a, 4, o);
}

int16_t aoe_artifact_claimant_ror(const AoeArtifactActionRor *a,
                                  const AoeObjectActionOps *o) {
    void *c;
    int16_t n, last = -1, found = 0;
    uint8_t *count;
    int32_t r[4];
    if (!a || !artifact_ops_ok(o)) return -1;
    c = o->move.context;
    n = o->player_count(c);
    if (n <= 0 || !(count = calloc((size_t)n, 1))) return -1;
    if (area(a->base.unit, o, r))
        for (int32_t y = r[1]; y <= r[3]; ++y)
            for (int32_t x = r[0]; x <= r[2]; ++x) {
                AoeLiveUnit *u;
                for (uint32_t i = 0; (u = o->tile_object(c, x, y, i)); ++i) {
                    int16_t cls = u->core.unit_template->unit_class;
                    if (cls == 1 || cls == 0x0b || cls == 0x1e ||
                        u->core.state != 2)
                        continue;
                    if (u->runtime_owner_index >= 0 &&
                        u->runtime_owner_index < n)
                        ++count[u->runtime_owner_index];
                }
            }
    for (int16_t i = 1; i < n; ++i)
        if (count[i]) {
            ++found;
            last = i;
        }
    free(count);
    if (found == 1 && last > 0 &&
        last != a->base.unit->runtime_owner_index)
        return last;
    return -1;
}

int aoe_artifact_action_update_ror(AoeArtifactActionRor *a,
                                   const AoeObjectActionOps *o,
                                   uint8_t *result) {
    void *c;
    uint8_t r;
    if (!a || !result || !artifact_ops_ok(o)) return 0;
    c = o->move.context;
    *result = 0;
    r = o->sub_update(c, &a->base);
    if (r >= 1 && r <= 3) o->sub_clear(c, &a->base);
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 6: {
        int16_t who = aoe_artifact_claimant_ror(a, o);
        if (who < 0) return aoe_artifact_action_set_state_ror(a, 0x0a, o);
        o->sub_clear(c, &a->base);
        return o->change_owner(c, a->base.unit, who);
    }
    case 0x0a:
        a->timer = a->timer - o->tick_seconds(c);
        if (a->timer > 0.0f) return 1;
        return aoe_artifact_action_set_state_ror(a, 6, o);
    case 100:
        if (!o->change_owner(c, a->base.unit, 0)) return 0;
        return aoe_artifact_action_set_state_ror(a, 6, o);
    default:
        return 1;
    }
}

/* ---- discovery (0x6c) ----------------------------------------------- */

int aoe_discovery_action_init_ror(AoeDiscoveryActionRor *a,
                                  AoeLiveUnit *unit, const DatUnitTask *task,
                                  AoeLiveUnit *target,
                                  const AoeObjectActionOps *o) {
    int16_t n;
    if (!a || !unit || !task || !o || !o->player_count) return 0;
    init_base(&a->base, 0x6c, unit);
    a->task = task;
    if (target && !aoe_action_set_target_ror(&a->base, target, &o->move))
        return 0;
    n = o->player_count(o->move.context);
    if (n <= 0 || !(a->found = calloc((size_t)n, 1))) return 0;
    a->found_count = n;
    return 1;
}

void aoe_discovery_action_free_ror(AoeDiscoveryActionRor *a) {
    if (!a) return;
    free(a->found);
    a->found = NULL;
}

int aoe_discovery_action_activate_ror(AoeDiscoveryActionRor *a, int arg) {
    if (!a) return 0;
    if (arg) a->base.state = 6;
    return 1;
}

/* 0x004b2a30. */
static void discover(AoeDiscoveryActionRor *a, const AoeObjectActionOps *o) {
    void *c = o->move.context;
    int32_t r[4];
    if (!area(a->base.unit, o, r)) return;
    for (int32_t y = r[1]; y <= r[3]; ++y)
        for (int32_t x = r[0]; x <= r[2]; ++x) {
            AoeLiveUnit *u;
            for (uint32_t i = 0; (u = o->tile_object(c, x, y, i)); ++i) {
                int16_t p = u->runtime_owner_index;
                if (p == a->base.unit->runtime_owner_index || p < 0 ||
                    p >= a->found_count || a->found[p])
                    continue;
                a->found[p] = 1;
                o->create(c, p, 0xa2, u->core.position.x38,
                          u->core.position.y3c, u->core.position.z40);
            }
        }
}

int aoe_discovery_action_update_ror(AoeDiscoveryActionRor *a,
                                    const AoeObjectActionOps *o,
                                    uint8_t *result) {
    if (!a || !result || !artifact_ops_ok(o) || !o->create || !a->found)
        return 0;
    *result = 0;
    switch (a->base.state) {
    case 1:
        *result = 1;
        return 1;
    case 6:
        discover(a, o);
        a->base.state = 0x0a;
        a->timer = 3.0f;
        return 1;
    case 0x0a:
        a->timer = a->timer - o->tick_seconds(o->move.context);
        if (a->timer > 0.0f) return 1;
        a->base.state = 6;
        return 1;
    default:
        return 1;
    }
}

/* ---- wonder (0x78) -------------------------------------------------- */

static int wonder_ops_ok(const AoeObjectActionOps *o) {
    return o && o->tick_seconds && o->player_count && o->vision && o->event &&
           o->countdown_set && o->countdown_clear && o->win;
}

int aoe_wonder_action_init_ror(AoeWonderActionRor *a, AoeLiveUnit *unit) {
    if (!a || !unit) return 0;
    init_base(&a->base, 0x78, unit);
    a->timer = 0.0f;
    return 1;
}

static void wonder_event(AoeWonderActionRor *a, const AoeObjectActionOps *o,
                         uint32_t e) {
    AoeLiveUnit *u = a->base.unit;
    o->event(o->move.context, e, u->runtime_owner_index,
             aoe_ftol32(u->core.position.x38),
             aoe_ftol32(u->core.position.y3c));
}

int aoe_wonder_action_set_state_ror(AoeWonderActionRor *a, uint8_t st,
                                    const AoeObjectActionOps *o) {
    void *c;
    AoeLiveUnit *u;
    int16_t n;
    if (!a || !wonder_ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    n = o->player_count(c);
    a->base.state = st;
    if (st == 2) {
        for (int16_t i = 1; i < n; ++i)
            if (i != u->runtime_owner_index) o->vision(c, u, i, 1, 1);
        wonder_event(a, o, 0x6c);
    } else if (st == 6) {
        for (int16_t i = 1; i < n; ++i)
            if (i != u->runtime_owner_index) {
                o->vision(c, u, i, 0, 1);
                o->vision(c, u, i, 1, 0);
            }
        if (!o->countdown_disabled)
            o->countdown_set(c, u->runtime_owner_index, 2000);
        wonder_event(a, o, 0x6d);
    }
    return 1;
}

int aoe_wonder_action_activate_ror(AoeWonderActionRor *a, int arg,
                                   const AoeObjectActionOps *o) {
    if (!a) return 0;
    return arg ? aoe_wonder_action_set_state_ror(a, 2, o) : 1;
}

int aoe_wonder_action_update_ror(AoeWonderActionRor *a,
                                 const AoeObjectActionOps *o,
                                 uint8_t *result) {
    void *c;
    AoeLiveUnit *u;
    if (!a || !result || !wonder_ops_ok(o)) return 0;
    c = o->move.context;
    u = a->base.unit;
    *result = 0;
    if (a->base.state == 2) {
        if (u->core.state == 2) return aoe_wonder_action_set_state_ror(a, 6, o);
        return 1;
    }
    if (a->base.state != 6 || !(a->timer < 2000.0f)) return 1;
    /* fld tick; fadd st,st; fadd timer; fst timer */
    {
        double years = 2.0 * o->tick_seconds(c) + a->timer;
        a->timer = (float)years;
        if (o->countdown_disabled) return 1;
        o->countdown_set(c, u->runtime_owner_index,
                         aoe_ftol32(2000.0 - years));
    }
    if (a->timer < 2000.0f) return 1;
    o->win(c, u->runtime_owner_index);
    if (o->game_over) *o->game_over = 100;
    return 1;
}

int aoe_wonder_action_destroy_ror(AoeWonderActionRor *a,
                                  const AoeObjectActionOps *o,
                                  int owner_has_other) {
    void *c;
    AoeLiveUnit *u;
    int16_t n;
    if (!a || !wonder_ops_ok(o)) return 0;
    if (o->tearing_down) return 1;
    c = o->move.context;
    u = a->base.unit;
    n = o->player_count(c);
    if (a->base.state == 2)
        for (int16_t i = 1; i < n; ++i)
            if (i != u->runtime_owner_index) o->vision(c, u, i, 0, 1);
    if (a->base.state == 6) {
        for (int16_t i = 1; i < n; ++i)
            if (i != u->runtime_owner_index) o->vision(c, u, i, 0, 0);
        if (!o->countdown_disabled) {
            o->countdown_clear(c, u->runtime_owner_index);
            if (owner_has_other)
                o->countdown_set(c, u->runtime_owner_index, 2000);
        }
    }
    wonder_event(a, o, 0x6e);
    return 1;
}
