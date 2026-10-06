#include "movement.h"

#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct {
    int flags, kind, finds, counts, fits_calls, fits, places;
    int32_t result, path_count;
    AoeMovementRor *movement;
    AoePathRequestRor last;
    float fit_x, fit_y, place_x, place_y, place_z;
} Env;

static void order_flag(void *c, AoeLiveUnit *u, int32_t v) {
    (void)u;
    assert(v == 0);
    ++((Env *)c)->flags;
}
static int32_t owner_kind(void *c, AoeLiveUnit *u) {
    (void)u;
    return ((Env *)c)->kind;
}
static int32_t find(void *c, const AoePathRequestRor *r) {
    Env *e = c;
    AoePathWaypointRor wp = {0};
    ++e->finds;
    e->last = *r;
    aoe_unit_path_clear_ror(&e->movement->path);
    for (int32_t i = 0; i < e->path_count; ++i) {
        wp.x = (float)i;
        assert(aoe_unit_path_prepend_ror(&e->movement->path, &wp));
    }
    return e->result;
}
static void count_search(void *c) { ++((Env *)c)->counts; }
static int fits(void *c, AoeLiveUnit *u, float x, float y) {
    Env *e = c;
    (void)u;
    ++e->fits_calls;
    e->fit_x = x;
    e->fit_y = y;
    return e->fits;
}
static void place(void *c, AoeLiveUnit *u, float x, float y, float z) {
    Env *e = c;
    (void)u;
    ++e->places;
    e->place_x = x;
    e->place_y = y;
    e->place_z = z;
}

static void count_clear(void *c) { ++*(int *)c; }

static void slot_tests(void) {
    AoeMovementRor m;
    AoeUnitPathRor path;
    memset(&m, 0, sizeof m);
    m.velocity_x = 1.0f;
    m.turn_timer = 2.0f;
    m.blocked_wait = -1.0f;
    m.field_a0 = 5;
    aoe_movement_reinit_ror(&m);
    assert(m.velocity_x == 0.0f && m.turn_timer == 0.0f && m.blocked_wait == 0.0f && m.field_a0 == 0);
    assert(!aoe_movement_moving_ror(&m));
    m.current_speed = 0.5f;
    assert(aoe_movement_moving_ror(&m));
    aoe_movement_set_hold_ror(&m, 3);
    assert(aoe_movement_hold_ror(&m, 2) == 3 && aoe_movement_hold_ror(&m, 6) == 0);
    aoe_unit_path_init_ror(&path, 4);
    aoe_unit_path_append_ror(&path, 1.5f, 2.0f, 0.0f, 0);
    aoe_unit_path_append_ror(&path, 3.0f, 4.25f, 0.0f, 0);
    aoe_unit_path_set_cursor_ror(&path, 1);
    /* 1.5 + 2 + 3 + 4.25 = 10.75, truncated; the cursor is kept. */
    assert(aoe_unit_path_coord_sum_ror(&path) == 10 && aoe_unit_path_cursor_ror(&path) == 1);
    aoe_unit_path_free_ror(&path);
}

typedef struct {
    int counts[2], finds[2];
    uint8_t alt_during;
    int32_t free_a, free_b;
    AoeMovementRor *m;
    AoePathRequestRor req;
} PathToEnv;
static void pt_count(void *c, int second) { ++((PathToEnv *)c)->counts[second != 0]; }
static int32_t pt_find(void *c, int second, const AoePathRequestRor *r) {
    PathToEnv *e = c;
    ++e->finds[second != 0];
    e->req = *r;
    e->alt_during = e->m->use_alternate_path;
    e->free_a = e->m->free_terrain_a;
    e->free_b = e->m->free_terrain_b;
    return 1;
}
static void test_path_to(void) {
    AoeLiveUnit unit;
    AoeMovementRor m;
    PathToEnv e;
    AoeMovementPathToOps ops;
    uint32_t bits;
    memset(&unit, 0, sizeof(unit));
    memset(&m, 0, sizeof(m));
    memset(&e, 0, sizeof(e));
    e.m = &m;
    ops.context = &e;
    ops.map_width = 10;
    ops.map_height = 8;
    ops.count_search = pt_count;
    ops.find = pt_find;
    unit.core.position.x38 = 3.75f;
    unit.core.position.y3c = 2.5f;
    m.free_terrain_a = 7;
    /* Outside the map: 0 without any change. */
    assert(aoe_movement_path_to_ror(&unit, &m, 10, 2, 1.0f, 5, 1, 2, 3, 0,
                                     -1, 2500, &ops) == 0);
    assert(aoe_movement_path_to_ror(&unit, &m, 2, -1, 1.0f, 5, 1, 2, 3, 0,
                                     -1, 2500, &ops) == 0);
    assert(m.free_terrain_a == 7 && e.finds[0] == 0 && e.counts[0] == 0);
    assert(aoe_movement_path_to_ror(&unit, &m, 9, 7, 1.5f, 5, 0, 2, 3, 1,
                                     4, 100, &ops) == 1);
    assert(e.counts[1] == 1 && e.finds[1] == 1 && e.finds[0] == 0);
    assert(e.alt_during == 1 && e.free_a == 2 && e.free_b == 3);
    assert(m.use_alternate_path == 0 && m.free_terrain_a == -1 &&
           m.free_terrain_b == -1);
    assert(e.req.x1 == 3 && e.req.y1 == 2 && e.req.x2 == 9 && e.req.y2 == 7);
    assert(e.req.unit == &unit && e.req.range == 1.5f && e.req.target_id == 5);
    assert(e.req.mode == 1 && e.req.budget_limited == 0 &&
           e.req.argument_30 == 1 && e.req.check_occupancy == 1 &&
           e.req.step == 1 && e.req.query_filter == 4 &&
           e.req.budget_base == 100);
    memcpy(&bits, &e.req.query_radius, sizeof(bits));
    assert(bits == 1u);
    assert(aoe_movement_path_to_ror(&unit, &m, 0, 0, 1.5f, 5, 1, 2, 3, 0,
                                     -1, 2500, &ops) == 1);
    assert(e.counts[0] == 1 && e.finds[0] == 1);
}

typedef struct {
    int lookups, finds, clears;
    uint8_t alt_during;
    AoeMovementRor *m;
    AoeZoneMapRor *map;
    AoePathRequestRor req;
} ReachEnv;
static int32_t re_index(void *c, AoeLiveUnit *u) {
    (void)u;
    ++((ReachEnv *)c)->lookups;
    return 3;
}
static const AoeZoneMapRor *re_map(void *c, int32_t i) {
    return i == 3 ? ((ReachEnv *)c)->map : NULL;
}
static int32_t re_find(void *c, const AoePathRequestRor *r) {
    ReachEnv *e = c;
    ++e->finds;
    e->req = *r;
    e->alt_during = e->m->use_alternate_path;
    return 0;
}
static void re_clear(void *c) { ++((ReachEnv *)c)->clears; }
static void test_reach_path(void) {
    /* 6 x 1 map: zones 1 1 2 2 1 1. */
    uint8_t zones[6] = {1, 1, 2, 2, 1, 1};
    AoeZoneMapRor map;
    AoeLiveUnit unit;
    AoeMovementRor m;
    ReachEnv e;
    AoeMovementReachOps ops = {0};
    int32_t index = -1, goal[2] = {4, 0};
    uint32_t bits;
    memset(&map, 0, sizeof(map));
    map.width = 6;
    map.height = 1;
    map.zones = zones;
    memset(&unit, 0, sizeof(unit));
    memset(&m, 0, sizeof(m));
    memset(&e, 0, sizeof(e));
    e.m = &m;
    e.map = &map;
    ops.context = &e;
    ops.zone_index = re_index;
    ops.zone_map = re_map;
    ops.find = re_find;
    ops.clear_penalty = re_clear;
    unit.core.position.x38 = 0.75f;
    unit.core.position.y3c = 0.5f;
    /* Same zone (1): searched on the alternate path. */
    assert(aoe_movement_reach_path_ror(&unit, &m, &index, goal, 1.0f, 8,
                                       &ops) == &m.alternate_path);
    assert(index == 3 && e.lookups == 1 && e.finds == 1 && e.clears == 1);
    assert(e.alt_during == 1 && m.use_alternate_path == 0);
    assert(e.req.x1 == 0 && e.req.y1 == 0 && e.req.x2 == 4 && e.req.y2 == 0 &&
           e.req.unit == &unit && e.req.range == 1.0f && e.req.target_id == 8);
    assert(e.req.mode == 1 && e.req.budget_limited == 0 &&
           e.req.argument_30 == 0 && e.req.check_occupancy == 1 &&
           e.req.step == 1 && e.req.query_filter == -1 &&
           e.req.budget_base == -1);
    memcpy(&bits, &e.req.query_radius, sizeof(bits));
    assert(bits == 1u);
    /* Goal in zone 2, no zone-1 tile within 0.5 of tile 3: NULL, cached
       index reused. */
    goal[0] = 3;
    assert(!aoe_movement_reach_path_ror(&unit, &m, &index, goal, 0.5f, 8, &ops));
    assert(e.lookups == 1 && e.finds == 1 && e.clears == 1);
    /* Within 1.0 of tile 3, tile 4 is zone 1: searched. */
    assert(aoe_movement_reach_path_ror(&unit, &m, &index, goal, 1.0f, 8,
                                       &ops) == &m.alternate_path);
    assert(e.finds == 2 && e.clears == 2);
}

static int lp_calls;
static float lp_x[16], lp_y[16];
static int32_t lp_block_x = -1;
static int32_t lp_passable(void *c, AoeLiveUnit *u, float x, float y, int32_t arg) {
    (void)c; (void)u;
    assert(arg == 77);
    if (lp_calls < 16) { lp_x[lp_calls] = x; lp_y[lp_calls] = y; }
    ++lp_calls;
    return (int32_t)x != lp_block_x;
}
static void test_line_passable(void) {
    int32_t from[2] = {0, 0}, to[2] = {3, 0}, blocked[2] = {-5, -5};
    /* Equal ends: 0, nothing checked. */
    assert(aoe_movement_line_passable_ror(NULL, 2, 2, 2, 2, 0.0f, 77,
                                          lp_passable, NULL, NULL) == 0);
    assert(lp_calls == 0);
    /* 6 half steps to x = 3: tiles 0,1,1,2,2,3 -> checks at 0.5, 1.0, 2.0, 3.0. */
    assert(aoe_movement_line_passable_ror(NULL, 0, 0, 3, 0, 0.0f, 77,
                                          lp_passable, NULL, NULL) == 1);
    assert(lp_calls == 4 && lp_x[0] == 0.5f && lp_x[1] == 1.0f &&
           lp_x[2] == 2.0f && lp_x[3] == 3.0f && lp_y[3] == 0.0f);
    /* Within range 2 of the end after the first check. */
    lp_calls = 0;
    assert(aoe_movement_line_passable_ror(NULL, 0, 0, 3, 0, 2.5f, 77,
                                          lp_passable, NULL, NULL) == 1);
    assert(lp_calls == 1);
    /* Blocked at tile 2: 0 and the tile reported by 0x0044e600. */
    lp_calls = 0;
    lp_block_x = 2;
    assert(aoe_movement_line_first_block_ror(NULL, from, to, 0.0f, blocked,
                                             77, lp_passable, NULL) == 0);
    assert(blocked[0] == 2 && blocked[1] == 0 && lp_calls == 3);
    lp_block_x = -1;
}

static AoeZoneMapRor *zp_map;
static int zp_lookups;
static int32_t zp_index(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++zp_lookups; return 0; }
static const AoeZoneMapRor *zp_get(void *c, int32_t i) { (void)c; return i == 0 ? zp_map : NULL; }
static void test_zone_path(void) {
    uint8_t zones[6] = {1, 1, 2, 2, 1, 1};
    AoeZoneMapRor map;
    AoeLiveUnit unit;
    AoeMovementRor m;
    PathToEnv e;
    AoeMovementZonePathOps ops;
    int32_t index = -1;
    memset(&map, 0, sizeof(map));
    map.width = 6;
    map.height = 1;
    map.zones = zones;
    zp_map = &map;
    memset(&unit, 0, sizeof(unit));
    memset(&m, 0, sizeof(m));
    memset(&e, 0, sizeof(e));
    e.m = &m;
    ops.path.context = &e;
    ops.path.map_width = 6;
    ops.path.map_height = 1;
    ops.path.count_search = pt_count;
    ops.path.find = pt_find;
    ops.zone_index = zp_index;
    ops.zone_map = zp_get;
    unit.core.position.x38 = 0.5f;
    unit.core.position.y3c = 0.25f;
    assert(aoe_movement_zone_path_ror(&unit, &index, 6, 0, 1.0f, 3, 1, 0, -1, 2500, &ops) == 0);
    assert(zp_lookups == 0 && e.finds[0] == 0);
    /* Zone 2 goal, no zone-1 tile within 0.5: 0 without a search. */
    assert(aoe_movement_zone_path_ror(&unit, &index, 2, 0, 0.5f, 3, 1, 0, -1, 2500, &ops) == 0);
    assert(zp_lookups == 1 && index == 0 && e.counts[0] == 0);
    assert(aoe_movement_zone_path_ror(&unit, &index, 5, 0, 0.5f, 3, 0, 1, 7, 99, &ops) == 1);
    assert(zp_lookups == 1 && e.counts[1] == 1 && e.finds[1] == 1);
    assert(e.req.mode == 0 && e.req.budget_limited == 0 && e.req.argument_30 == 1 &&
           e.req.x1 == 0 && e.req.y1 == 0 && e.req.x2 == 5 && e.req.target_id == 3 &&
           e.req.query_filter == 7 && e.req.budget_base == 99 && e.alt_during == 0);
}

static int op_collected;
static void op_collect(void *c, int second, void *met) { (void)c; (void)met; op_collected = 1 + second; }
static void test_object_path(void) {
    uint8_t zones[6] = {1, 1, 2, 2, 1, 1};
    AoeZoneMapRor map;
    AoeLiveUnit unit, target;
    AoeMovementRor m;
    PathToEnv e;
    AoeMovementZonePathOps ops;
    int32_t index = -1;
    memset(&map, 0, sizeof(map));
    map.width = 6;
    map.height = 1;
    map.zones = zones;
    zp_map = &map;
    memset(&unit, 0, sizeof(unit));
    memset(&target, 0, sizeof(target));
    memset(&m, 0, sizeof(m));
    memset(&e, 0, sizeof(e));
    memset(&ops, 0, sizeof(ops));
    e.m = &m;
    ops.path.context = &e;
    ops.path.count_search = pt_count;
    ops.path.find = pt_find;
    ops.zone_index = zp_index;
    ops.zone_map = zp_get;
    zp_lookups = 0;
    assert(aoe_movement_object_path_ror(&unit, &index, NULL, 3, 1.0f, 0, 0, -1, -1, &ops) == 0);
    target.core.position.x38 = 2.5f;
    assert(aoe_movement_object_path_ror(&unit, &index, &target, 3, 0.5f, 0, 0, -1, -1, &ops) == 0);
    assert(e.finds[0] == 0 && zp_lookups == 1);
    target.core.position.x38 = 4.5f;
    assert(aoe_movement_object_path_ror(&unit, &index, &target, 3, 0.5f, 1, 1, 7, 9, &ops) == 1);
    assert(e.finds[1] == 1 && e.req.mode == 0 && e.req.x2 == 4 && e.req.target_id == 3 &&
           e.alt_during == 0 && e.req.budget_base == 9);
    assert(aoe_movement_object_path_alt_ror(&unit, &m, &index, &target, 3, 0.5f, 0, 0, 7, -1,
                                            NULL, op_collect, &ops) == 1);
    assert(e.alt_during == 1 && e.req.mode == 1 && m.use_alternate_path == 0 && op_collected == 0);
    assert(aoe_movement_object_path_alt_ror(&unit, &m, &index, &target, 3, 0.5f, 0, 1, 7, 5,
                                            NULL, op_collect, &ops) == 1);
    assert(op_collected == 2);
}


typedef struct {
    int lookups, counts[2], finds[2], second_find_fails;
    AoeMovementRor *m;
    AoeZoneMapRor *map;
    AoeLiveUnit *objects[2];
    AoePathRequestRor req[2];
    uint8_t alt_during[2];
} ViaEnv;
static AoeLiveUnit *rv_resolve(void *c, int32_t id) { ViaEnv *e = c; return id == 20 ? e->objects[0] : id == 21 ? e->objects[1] : NULL; }
static int32_t rv_index(void *c, AoeLiveUnit *u) { (void)u; ++((ViaEnv *)c)->lookups; return 2; }
static const AoeZoneMapRor *rv_map(void *c, int32_t i) { return i == 2 ? ((ViaEnv *)c)->map : NULL; }
static void rv_count(void *c, int second) { ++((ViaEnv *)c)->counts[second]; }
static int32_t rv_find(void *c, int second, const AoePathRequestRor *r) {
    ViaEnv *e = c; int n = e->finds[0] + e->finds[1];
    ++e->finds[second];
    if (n < 2) { e->req[n] = *r; e->alt_during[n] = e->m->use_alternate_path; }
    if (n == 0) { e->m->alternate_path.points[0].x = 2.6f; e->m->alternate_path.points[0].y = 0.2f; e->m->alternate_path.count = 1; return 1; }
    return e->second_find_fails ? 0 : 1;
}
static void test_reach_via(void) {
    uint8_t zones[6] = {1, 1, 2, 2, 1, 1};
    AoeZoneMapRor map; AoeLiveUnit unit, via, goal; AoeMovementRor m; ViaEnv e; AoePathWaypointRor pts[2];
    AoeMovementReachViaOps ops = {0}; int32_t index = -1; uint32_t bits;
    memset(&map, 0, sizeof map); map.width = 6; map.height = 1; map.zones = zones;
    memset(&unit, 0, sizeof unit); memset(&via, 0, sizeof via); memset(&goal, 0, sizeof goal);
    memset(&m, 0, sizeof m); memset(&e, 0, sizeof e); memset(pts, 0, sizeof pts);
    m.alternate_path.points = pts; m.alternate_path.capacity = 2;
    e.m = &m; e.map = &map; e.objects[0] = &via; e.objects[1] = &goal;
    ops.context = &e; ops.resolve = rv_resolve; ops.zone_index = rv_index; ops.zone_map = rv_map; ops.count_search = rv_count; ops.find = rv_find;
    unit.core.position.x38 = 0.75f; via.core.position.x38 = 4.5f; goal.core.position.x38 = 5.5f;
    assert(aoe_movement_reach_via_ror(&unit, &m, &index, 20, 21, 1.5f, 1, 1, 7, 2500, &ops) == 1);
    assert(index == 2 && e.lookups == 1 && e.counts[1] == 1 && e.counts[0] == 0 && e.finds[1] == 2);
    assert(e.req[0].x1 == 0 && e.req[0].x2 == 4 && e.req[0].target_id == 20 && e.req[0].argument_30 == 1 && e.req[0].budget_limited == 1 && e.req[0].query_filter == 7 && e.req[0].budget_base == 2500 && e.req[0].range == 1.5f && e.req[0].mode == 1 && e.req[0].check_occupancy == 1 && e.req[0].step == 1);
    memcpy(&bits, &e.req[0].query_radius, sizeof bits); assert(bits == 1u);
    assert(e.req[1].x1 == 2 && e.req[1].y1 == 0 && e.req[1].x2 == 5 && e.req[1].target_id == 21 && e.req[1].argument_30 == 0);
    assert(e.alt_during[0] == 1 && e.alt_during[1] == 1 && m.use_alternate_path == 0);
    /* The via object in zone 2 with no zone-1 tile within 0.5: refused before any search. */
    via.core.position.x38 = 3.5f;
    assert(aoe_movement_reach_via_ror(&unit, &m, &index, 20, 21, 0.5f, 0, 0, -1, -1, &ops) == 0 && e.finds[0] == 0 && e.lookups == 1);
    assert(aoe_movement_reach_via_ror(&unit, &m, &index, 20, 21, 1.0f, 0, 0, -1, -1, &ops) == 1 && e.counts[0] == 1 && e.finds[0] == 2);
    e.second_find_fails = 1;
    assert(aoe_movement_reach_via_ror(&unit, &m, &index, 20, 21, 1.0f, 0, 0, -1, -1, &ops) == 0 && m.use_alternate_path == 0);
    assert(aoe_movement_reach_via_ror(&unit, &m, &index, 20, 99, 1.0f, 0, 0, -1, -1, &ops) == 0 && e.finds[0] == 3);
    index = 9;
    assert(aoe_movement_reach_via_ror(&unit, &m, &index, 20, 21, 1.0f, 0, 0, -1, -1, &ops) == 0 && e.finds[0] == 3);
}
int main(void) {
    test_zone_path();
    test_object_path();
    test_line_passable();
    test_reach_path();
    test_path_to();
    slot_tests();
    DatUnit tmpl;
    AoeLiveUnit unit;
    AoeMovementRor m;
    AoePathBudgetRor budget = {2, -1, 0};
    Env env;
    AoeMovementOps ops = {&env, order_flag, owner_kind, find, count_search,
                          fits, place};
    const AoePathWaypointRor *wp;
    uint8_t tile[2] = {7, 9};
    uint32_t bits;
    memset(&tmpl, 0, sizeof(tmpl));
    memset(&unit, 0, sizeof(unit));
    memset(&m, 0, sizeof(m));
    memset(&env, 0, sizeof(env));
    env.movement = &m;
    unit.core.unit_template = &tmpl;
    unit.core.position.x38 = 3.25f;
    unit.core.position.y3c = 4.5f;
    unit.core.position.z40 = 1.0f;
    unit.group_unit_count = 5;

    /* Budget 0x0045c180: override < retry, unlimited, or used < max. */
    assert(aoe_path_budget_allows_ror(&budget, 200));
    budget.used = 2;
    assert(!aoe_path_budget_allows_ror(&budget, 200));
    budget.retry_override = 3;
    assert(aoe_path_budget_allows_ror(&budget, 4) &&
           !aoe_path_budget_allows_ror(&budget, 3));
    budget.max_per_tick = -1;
    assert(aoe_path_budget_allows_ror(&budget, 0));
    budget = (AoePathBudgetRor){1, -1, 0};

    /* Height <= 0 (and NaN): direct two-waypoint path, cursor on the
       destination, no budget or search. */
    aoe_movement_set_destination_ror(&m, 10.0f, 11.0f, 1.0f);
    m.dest_direction = 6;
    tmpl.size[2] = NAN;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0);
    assert(m.path.count == 2 && m.path.cursor == 1 && !budget.used &&
           !env.finds && env.flags == 1 && unit.group_unit_count == 0 &&
           m.best == 100000.0f && m.start_x == 3.25f);
    wp = aoe_unit_path_current_ror(&m.path);
    assert(wp && wp->x == 10.0f && wp->y == 11.0f && wp->direction == 6 &&
           m.path.points[0].x == 3.25f);

    /* Out of budget: the retry byte counts and nothing else changes. */
    tmpl.size[2] = 0.5f;
    budget.used = 1;
    m.best = 1.0f;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 2 &&
           m.retry == 1 && env.flags == 1 && m.best == 1.0f && !env.finds);

    /* With budget: the search targets the destination tile with the
       original fixed arguments; kind 0 owners get baseline +25. */
    budget.used = 0;
    m.retry = 4;
    aoe_movement_set_range_ror(&m, 0.75f);
    aoe_movement_set_target_id_ror(&m, 42);
    env.result = 1;
    env.path_count = 3;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0);
    assert(budget.used == 1 && m.retry == 0 && env.counts == 1 &&
           m.retry_baseline == 25 && m.path.cursor == 1);
    assert(env.last.x1 == 3 && env.last.y1 == 4 && env.last.x2 == 10 &&
           env.last.y2 == 11 && env.last.unit == &unit &&
           env.last.range == 0.75f && env.last.target_id == 42 &&
           env.last.mode == 1 && env.last.budget_limited == 1 &&
           env.last.argument_30 == 1 && env.last.check_occupancy == 1 &&
           env.last.step == 1 && env.last.query_filter == -1);
    memcpy(&bits, &env.last.query_radius, sizeof(bits));
    assert(bits == UINT32_MAX);

    /* Intermediate waypoint tiles replace the destination; other owner
       kinds use +15. */
    env.kind = 2;
    m.waypoint_tile = tile;
    m.waypoint_count = 4;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0 &&
           env.last.x2 == 7 && env.last.y2 == 9 && m.retry_baseline == 19);
    m.waypoint_count = 0;

    /* A failed search leaving one waypoint snaps an off-centre unit to its
       tile centre when it fits there. */
    env.result = 0;
    env.path_count = 1;
    env.fits = 1;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 2 &&
           env.places == 1 && env.fit_x == 3.5f && env.fit_y == 4.5f &&
           env.place_x == 3.5f && env.place_y == 4.5f &&
           env.place_z == 1.0f);

    /* Already centred on both axes: no snap, normal return. */
    unit.core.position.x38 = 3.5f;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0 &&
           env.places == 1 && m.path.cursor == 0);

    /* Not fitting, or more than one waypoint, also skips the snap. */
    unit.core.position.x38 = 3.25f;
    env.fits = 0;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0 &&
           env.places == 1);
    env.fits = 1;
    env.path_count = 2;
    env.fits_calls = 0;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0 &&
           !env.fits_calls && m.path.cursor == 1);

    /* Advance clamps at the last waypoint; empty paths have no current. */
    assert(!aoe_unit_path_advance_ror(&m.path) && m.path.cursor == 1);
    aoe_unit_path_clear_ror(&m.path);
    assert(!aoe_unit_path_current_ror(&m.path));
    assert(aoe_movement_start_ror(&unit, &m, &budget, NULL) == -1);
    aoe_unit_path_free_ror(&m.path);

    /* Intermediate waypoint list 0x0044eb40..0x0044ed50. */
    {
        AoeMovementRor w;
        int cleared = 0;
        memset(&w, 0, sizeof(w));
        for (uint8_t i = 0; i < 6; ++i) {
            uint8_t e[3] = {i, (uint8_t)(i + 10), (uint8_t)(i + 20)};
            assert(aoe_movement_waypoint_append_ror(&w, e, 1, NULL, NULL));
        }
        /* 5 entries, then doubled to 10 with zero fill. */
        assert(w.waypoint_count == 6 && w.waypoint_capacity == 10 &&
               w.waypoint_tile[3 * 6] == 0 && w.waypoint_tile[3 * 9 + 2] == 0);
        assert(aoe_movement_waypoint_get_ror(&w, 5)[1] == 15 &&
               !aoe_movement_waypoint_get_ror(&w, 6) &&
               !aoe_movement_waypoint_get_ror(&w, -1));
        /* Index 0 drops entry 0; index 2 drops entry 1 (off by one). */
        aoe_movement_waypoint_remove_ror(&w, 0);
        assert(w.waypoint_count == 5 && w.waypoint_tile[0] == 1);
        aoe_movement_waypoint_remove_ror(&w, 2);
        assert(w.waypoint_count == 4 && w.waypoint_tile[0] == 1 &&
               w.waypoint_tile[3] == 3 && w.waypoint_tile[6] == 4);
        aoe_movement_waypoint_remove_ror(&w, 4); /* out of range */
        assert(w.waypoint_count == 4);
        /* A stale list is cleared (hook only with argument 1) on append. */
        aoe_movement_mark_waypoints_stale_ror(&w);
        {
            uint8_t e[3] = {9, 9, 9};
            assert(aoe_movement_waypoint_append_ror(&w, e, 1, count_clear,
                                                    &cleared));
        }
        assert(w.waypoint_count == 1 && !w.waypoints_stale && cleared == 1 &&
               w.waypoint_tile[0] == 9 && w.waypoint_capacity == 10);
        aoe_movement_waypoint_clear_ror(&w, 0, count_clear, &cleared);
        assert(!w.waypoint_count && cleared == 1);
        /* The move action's record step appends once, then marks stale so
           the next append starts a fresh list. */
        {
            uint8_t t1[3] = {4, 5, 0}, t2[3] = {6, 7, 0};
            assert(aoe_movement_record_waypoint_ror(&w, t1) &&
                   w.waypoint_count == 1 && w.waypoints_stale);
            assert(aoe_movement_record_waypoint_ror(&w, t2) &&
                   w.waypoint_count == 1 && w.waypoint_tile[0] == 4);
            assert(aoe_movement_waypoint_append_ror(&w, t2, 0, NULL, NULL) &&
                   w.waypoint_count == 1 && w.waypoint_tile[0] == 6);
        }
        aoe_movement_waypoint_free_ror(&w);
    }
    test_reach_via();
    return 0;
}
