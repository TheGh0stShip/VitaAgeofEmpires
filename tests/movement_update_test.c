#include "movement.h"

#include <assert.h>
#include <string.h>

typedef struct {
    AoeMovementRor *m;
    int flags, stops, pops, repaths, finds, places, fits;
    int32_t step_result, find_result, find_count;
    float step_x, step_y;
    uint8_t zones[2];
    uint8_t tiles[12];
} Env;

static void order_flag(void *c, AoeLiveUnit *u, int32_t v) {
    (void)u; (void)v;
    ++((Env *)c)->flags;
}
static int32_t owner_kind(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0; }
static int32_t find(void *c, const AoePathRequestRor *r) {
    Env *e = c;
    AoePathWaypointRor wp = {0};
    (void)r;
    ++e->finds;
    aoe_unit_path_clear_ror(&e->m->path);
    for (int32_t i = 0; i < e->find_count; ++i) {
        wp.x = 20.0f + (float)i;
        wp.y = 20.0f;
        assert(aoe_unit_path_prepend_ror(&e->m->path, &wp));
    }
    return e->find_result;
}
static void count(void *c) { (void)c; }
static int fits(void *c, AoeLiveUnit *u, float x, float y) {
    (void)u; (void)x; (void)y;
    return ((Env *)c)->fits;
}
static void place(void *c, AoeLiveUnit *u, float x, float y, float z) {
    (void)z;
    ++((Env *)c)->places;
    u->core.position.x38 = x;
    u->core.position.y3c = y;
}
static void stop(void *c, AoeLiveUnit *u) { (void)u; ++((Env *)c)->stops; }
static void pop(void *c, AoeLiveUnit *u) {
    Env *e = c;
    (void)u;
    ++e->pops;
    if (e->m->waypoint_count > 0) {
        --e->m->waypoint_count;
        e->m->waypoint_tile += 3;
    }
}
static uint8_t zone(void *c, AoeLiveUnit *u, int32_t x, int32_t y) {
    Env *e = c;
    (void)u; (void)y;
    return x == e->tiles[0] ? e->zones[0] : e->zones[1];
}
static void repath_count(void *c) { ++((Env *)c)->repaths; }
static int32_t step_to(void *c, AoeLiveUnit *u, float x, float y) {
    Env *e = c;
    (void)u;
    e->step_x = x;
    e->step_y = y;
    return e->step_result;
}

static void set_path(AoeMovementRor *m, int n, float x0) {
    AoePathWaypointRor wp = {0};
    aoe_unit_path_clear_ror(&m->path);
    for (int i = n - 1; i >= 0; --i) {
        wp.x = x0 + (float)i;
        wp.y = 5.5f;
        assert(aoe_unit_path_prepend_ror(&m->path, &wp));
    }
    m->path.cursor = 0;
}

int main(void) {
    DatUnit tmpl;
    AoeLiveUnit unit;
    AoeMovementRor m;
    AoePathBudgetRor budget = {-1, -1, 0};
    Env env;
    AoeMovementStepOps ops = {
        {&env, order_flag, owner_kind, find, count, fits, place},
        stop, pop, zone, repath_count, step_to};
    memset(&tmpl, 0, sizeof(tmpl));
    memset(&unit, 0, sizeof(unit));
    memset(&m, 0, sizeof(m));
    memset(&env, 0, sizeof(env));
    env.m = &m;
    unit.core.unit_template = &tmpl;
    tmpl.size[0] = tmpl.size[1] = 0.3f;
    unit.core.position.x38 = 5.5f;
    unit.core.position.y3c = 5.5f;
    m.dest_x = 15.5f;
    m.dest_y = 5.5f;

    /* In range (footprint-adjusted distance 9.7 <= 10): stop, status 1. */
    m.range = 10.0f;
    m.current_speed = 2.0f;
    unit.group_unit_count = 3;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 2 &&
           m.status == 1 && env.stops == 1 && m.current_speed == 0.0f &&
           unit.group_unit_count == 0 && env.flags == 1);

    /* Otherwise the unit steps toward the current waypoint. */
    m.range = 0.0f;
    set_path(&m, 3, 6.5f);
    env.step_result = 0;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           m.status == 2 && env.step_x == 6.5f && m.path.cursor == 0);
    env.step_result = 1;
    m.current_speed = 2.0f;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           m.current_speed == 0.0f);
    env.step_result = 5;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 2 &&
           env.stops == 2 && m.status == 1);

    /* Step result 2 advances to the next waypoint. */
    env.step_result = 2;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           m.path.cursor == 1 && m.status == 2);

    /* Standing exactly on the current waypoint advances first (placing the
       unit there when it fits). */
    unit.core.position.x38 = 7.5f;
    env.fits = 1;
    env.step_result = 0;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           m.path.cursor == 2 && env.places == 1 && env.step_x == 8.5f &&
           m.best == 100000.0f);

    /* Results 3, 4 and 6 give up; without intermediate tiles that is 0. */
    env.step_result = 3;
    m.current_speed = 1.0f;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 0 &&
           m.current_speed == 0.0f && m.status == 2);

    /* Last waypoint reached and the destination still far: no intermediate
       tiles left after the pop -> 0. */
    env.step_result = 2;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 0 &&
           env.pops == 1);

    /* Exhausted path with the destination in reach (radius + range):
       stop with +0x88 cleared. */
    m.dest_x = 8.5f;
    unit.core.position.x38 = 6.5f;
    tmpl.size[0] = tmpl.size[1] = 1.0f; /* distance 1.0, radius 1.414 */
    m.path.cursor = 3; /* past the end: no current waypoint */
    m.current_speed = 1.0f;
    env.step_result = 0;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 2 &&
           m.current_speed == 0.0f && m.status == 1);
    tmpl.size[0] = tmpl.size[1] = 0.3f;

    /* Close to the destination (< 0.5) steps straight toward it. */
    unit.core.position.x38 = 8.4f;
    m.path.cursor = 0;
    env.step_result = 0;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           env.step_x == 8.5f);
    env.step_result = 4;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 2 &&
           m.status == 1);
    /* ...unless the unit sits on its current waypoint: return 2 only. */
    unit.core.position.x38 = 6.5f;
    m.dest_x = 6.5f;
    env.stops = 0;
    m.status = 9;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 2 &&
           !env.stops && m.status == 9);

    /* Exhausted path, far destination, intermediate tiles: pops the reached
       tile, decrements the retry baseline and re-paths to the next tile. */
    memcpy(env.tiles, (uint8_t[]){6, 5, 0, 40, 5, 0, 60, 5, 0}, 9);
    m.waypoint_tile = env.tiles;
    m.waypoint_count = 3;
    m.dest_x = 70.5f;
    m.path.cursor = 5;
    m.retry_baseline = 3;
    env.find_result = 1;
    env.find_count = 2;
    env.step_result = 0;
    unit.group_unit_count = 4;
    budget = (AoePathBudgetRor){1, -1, 0};
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           env.pops == 2 && m.waypoint_count == 2 && m.retry_baseline == 2 &&
           env.finds == 1 && env.repaths == 1 && budget.used == 1 &&
           m.path.cursor == 1 && unit.group_unit_count == 0 &&
           env.step_x == 20.0f && m.status == 2);

    /* Out of budget: counts the retry and keeps moving status. */
    m.path.cursor = 5;
    m.retry = 0;
    m.waypoint_tile = env.tiles + 3; /* 40, then 60: neither is in reach */
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           m.retry == 1 && env.finds == 1 && m.retry_baseline == 1);

    /* Baseline exhausted: give up, re-arming it by 5 while tiles remain. */
    m.path.cursor = 5;
    m.waypoint_tile = env.tiles + 3;
    m.waypoint_count = 3;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           m.retry_baseline == 5 && m.status == 2);

    /* A one-waypoint failed re-path from a centred unit gives up. */
    budget.used = 0;
    m.path.cursor = 5;
    m.retry_baseline = 3;
    m.waypoint_tile = env.tiles + 3;
    m.waypoint_count = 3;
    env.find_result = 0;
    env.find_count = 1;
    unit.core.position.x38 = 6.5f;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           m.retry_baseline == 7 && env.finds == 2);

    /* Off-centre and fitting: the unit snaps to the tile centre, but a
       one-waypoint path cannot advance, so the update still gives up. */
    budget.used = 0;
    m.path.cursor = 5;
    m.waypoint_tile = env.tiles + 3;
    m.waypoint_count = 3;
    unit.core.position.x38 = 6.25f;
    env.places = 0;
    assert(aoe_movement_update_ror(&unit, &m, &budget, &ops) == 1 &&
           env.places == 1 && unit.core.position.x38 == 6.5f);

    aoe_unit_path_free_ror(&m.path);
    return 0;
}
