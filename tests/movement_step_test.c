#include "movement.h"

#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct {
    AoeMovementRor *m;
    AoeMovementSpriteRor sprite;
    int has_sprite, blocked, places, rand_value, fits_goal, kind;
    float tick, terrain, place_z_delta;
    int has_terrain;
} Env;

static void order_flag(void *c, AoeLiveUnit *u, int32_t v) { (void)c; (void)u; (void)v; }
static int32_t owner_kind(void *c, AoeLiveUnit *u) { (void)u; return ((Env *)c)->kind; }
static int32_t find(void *c, const AoePathRequestRor *r) { (void)c; (void)r; return 0; }
static void count(void *c) { (void)c; }
static int fits(void *c, AoeLiveUnit *u, float x, float y) {
    Env *e = c;
    (void)u; (void)y;
    if (x == 30.5f) return e->fits_goal;
    return !e->blocked;
}
static void place(void *c, AoeLiveUnit *u, float x, float y, float z) {
    Env *e = c;
    ++e->places;
    u->core.position.x38 = x;
    u->core.position.y3c = y;
    u->core.position.z40 = z + e->place_z_delta;
}
static void stop(void *c, AoeLiveUnit *u) { (void)c; (void)u; }
static void pop(void *c, AoeLiveUnit *u) { (void)c; (void)u; }
static uint8_t zone(void *c, AoeLiveUnit *u, int32_t x, int32_t y) {
    (void)c; (void)u; (void)x; (void)y;
    return 0;
}
static const AoeMovementSpriteRor *sprite(void *c, AoeLiveUnit *u) {
    Env *e = c;
    (void)u;
    return e->has_sprite ? &e->sprite : NULL;
}
static float tick(void *c) { return ((Env *)c)->tick; }
static int target_position(void *c, int32_t id, float *x, float *y) {
    (void)c; (void)id;
    *x = 11.0f;
    *y = 10.5f;
    return 1;
}
static int terrain(void *c, AoeLiveUnit *u, float *out) {
    Env *e = c;
    (void)u;
    *out = e->terrain;
    return e->has_terrain;
}
static int32_t rnd(void *c) { return ((Env *)c)->rand_value; }

int main(void) {
    DatUnit tmpl;
    AoeLiveUnit unit;
    AoeMovementRor m;
    AoePathBudgetRor budget = {-1, -1, 0};
    Env env;
    AoeMovementTickOps ops = {
        {{&env, order_flag, owner_kind, find, count, fits, place},
         stop, pop, zone, count, NULL},
        sprite, tick, target_position, terrain, rnd};
    float a;

    /* 0x0044ef90 wrapping and snapping. */
    a = -1.0f;
    aoe_movement_snap_angle_ror(&a, -1);
    assert(a == (float)(-1.0 + 6.2831854820251465));
    a = (float)(6.2831854820251465);
    aoe_movement_snap_angle_ror(&a, -1);
    assert(a == (float)6.2831854820251465);
    a = (float)(2 * 0.7853981852531433);
    aoe_movement_snap_angle_ror(&a, 8);
    assert(a == (float)(2 * 0.7853981852531433));
    a = 0.8f; /* within 0.1 of pi/4 at i = 1 */
    aoe_movement_snap_angle_ror(&a, 8);
    assert(a == 0.7853981852531433f);
    /* The shared slot: 1.0 picks i = 1 (diff 0.2146), then i = 4
       (1 - 4 * 0.2146 = 0.1416), then i = 7 (1 - 7 * 0.1416 = 0.0088 <
       0.1), so it snaps to 7 pi / 4 rather than pi / 4. */
    a = 1.0f;
    aoe_movement_snap_angle_ror(&a, 8);
    assert(a == (float)(7 * 0.7853981852531433));
    a = 0.3f; /* 16 facets use pi/8 */
    aoe_movement_snap_angle_ror(&a, 16);
    assert(a == 0.39269909262657166f);

    memset(&tmpl, 0, sizeof(tmpl));
    memset(&unit, 0, sizeof(unit));
    memset(&m, 0, sizeof(m));
    memset(&env, 0, sizeof(env));
    env.m = &m;
    unit.core.unit_template = &tmpl;

    /* 0x0044bbd0 facet frame: facing 0 with 8 facets -> ftol(-1+0.5+8). */
    env.sprite = (AoeMovementSpriteRor){1.0f, 8, 2};
    aoe_movement_update_facet_ror(&unit, &m, &env.sprite);
    assert(unit.core.scenario_frame == 7);
    m.facing = 0.7853981852531433f;
    aoe_movement_update_facet_ror(&unit, &m, &env.sprite);
    assert(unit.core.scenario_frame == 0);
    env.sprite.flags = 0;
    unit.core.scenario_frame = 9;
    aoe_movement_update_facet_ror(&unit, &m, &env.sprite);
    assert(unit.core.scenario_frame == 9);

    /* Straight east from a stop: speed from template * sprite multiplier,
       aimed from the tile centre; flat ground resets the speed. */
    tmpl.speed = 1.0f;
    tmpl.size[2] = 1.0f;
    env.has_sprite = 1;
    env.sprite = (AoeMovementSpriteRor){2.0f, 8, 2};
    env.tick = 0.25f;
    m.facing = 0.0f;
    m.best = 100000.0f;
    unit.core.position.x38 = 10.5f;
    unit.core.position.y3c = 10.5f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 20.5f, 10.5f) ==
           0);
    assert(unit.core.position.x38 == 11.0f &&
           unit.core.position.y3c == 10.5f && m.current_speed == 2.0f &&
           m.velocity_x == 0.5f && m.best == 9.5f &&
           m.facet_history[0] == unit.core.scenario_frame);

    /* Downhill (z dropped by more than 0.02) multiplies by 1.03, then the
       terrain multiplier. */
    env.place_z_delta = -0.5f;
    env.has_terrain = 1;
    env.terrain = 0.5f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 20.5f, 10.5f) ==
           0 && m.current_speed == (float)(2.0 * (float)(0.5 * 1.03f)));
    env.place_z_delta = 0.0f;
    env.has_terrain = 0;

    /* Blocked with no wait yet: wait rand%6 * 0.1 + 0.5 / speed. */
    env.blocked = 1;
    env.rand_value = 8; /* % 6 == 2 */
    m.current_speed = 2.0f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 20.5f, 10.5f) ==
           0 && m.current_speed == 0.0f &&
           m.blocked_wait == (float)(2 * (double)0.1f + 0.25));

    /* The wait drains by speed * tick (the template speed restarts at
       2 * 0.25 = 0.5 per tick). Expiring it falls through in the same
       tick: still blocked, so the wait clears and, with no target and an
       occupied goal, the call returns 2. */
    m.target_id = -1;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 20.5f, 10.5f) ==
           2 && m.blocked_wait == 0.0f);
    /* A longer wait just drains: speed is zeroed and nothing moves. */
    m.blocked_wait = 2.0f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 20.5f, 10.5f) ==
           0 && m.blocked_wait == 1.5f && m.current_speed == 0.0f);
    /* Expired and still blocked: the target 0.5 away is within range 0.5
       plus 0.3, but not within the 0.3 margin alone. */
    m.target_id = 7;
    m.blocked_wait = -1.0f;
    m.range = 0.5f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 20.5f, 10.5f) ==
           5 && m.blocked_wait == 0.0f);
    m.range = 0.0f;
    m.blocked_wait = -1.0f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 20.5f, 10.5f) ==
           2);
    /* Without a target, an occupied goal returns 2. */
    m.target_id = -1;
    m.blocked_wait = -1.0f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 20.5f, 10.5f) ==
           2);
    /* A free goal decrements the retry baseline; at 0 that is 4. */
    m.blocked_wait = -1.0f;
    m.retry_baseline = 1;
    env.fits_goal = 1;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 30.5f, 10.5f) ==
           4 && m.retry_baseline == 0);
    /* Without budget, the retry counts and the call returns 1. */
    m.blocked_wait = -1.0f;
    m.retry_baseline = 5;
    budget = (AoePathBudgetRor){0, -1, 0};
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 30.5f, 10.5f) ==
           1 && m.retry == 1);
    /* A failed re-path (find returns 0 with an empty path) cannot advance:
       4. */
    budget = (AoePathBudgetRor){-1, -1, 0};
    m.blocked_wait = -1.0f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 30.5f, 10.5f) ==
           4 && m.retry == 0);
    env.blocked = 0;

    /* Turning: heading north (pi/2) from facing 0 with a 1 s per-facet
       delay turns one facet per allowed tick and keeps moving. */
    tmpl.rotation_speed = 1.0f;
    m.facing = 0.0f;
    m.turn_timer = 0.0f;
    m.current_speed = 2.0f;
    unit.core.position.x38 = 10.5f;
    unit.core.position.y3c = 10.5f;
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 10.5f, 20.5f) ==
           0);
    /* The tick (0.25) drains the timer first, then one facet adds 1 s. */
    assert(m.facing == 0.7853981852531433f && m.turn_timer == 0.75f);
    /* The timer blocks further turning until it runs out. */
    assert(aoe_movement_step_to_ror(&unit, &m, &budget, &ops, 10.5f, 20.5f) ==
           0 && m.facing == 0.7853981852531433f && m.turn_timer == 0.5f);

    /* The bound tick: the update steps toward the current waypoint through
       the translated 0x0044cde0 (status 2, return 1). */
    {
        AoePathWaypointRor wp = {15.5f, 10.5f, 0.0f, 0};
        tmpl.rotation_speed = 0.0f;
        m.facing = 0.0f;
        m.current_speed = 2.0f;
        m.dest_x = 15.5f;
        m.dest_y = 10.5f;
        m.best = 100000.0f;
        unit.core.position.x38 = 10.5f;
        unit.core.position.y3c = 10.5f;
        aoe_unit_path_clear_ror(&m.path);
        assert(aoe_unit_path_prepend_ror(&m.path, &wp));
        assert(aoe_movement_tick_ror(&unit, &m, &budget, &ops) == 1 &&
               m.status == 2 && unit.core.position.x38 == 11.0f &&
               m.best == 5.0f); /* pre-move distance to the waypoint */
    }

    assert(aoe_movement_step_to_ror(&unit, &m, &budget, NULL, 0, 0) == -1);
    aoe_unit_path_free_ror(&m.path);
    return 0;
}
