#include "pathfinder_search.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { W = 24, H = 24 };
static uint8_t terrain[H][W]; /* 0 grass (passable), 1 water (blocked) */
static AoeLiveUnit unit;
static DatUnit unit_t;
static AoeUnitPathRor upath;
static const float costs[32] = {1.0f, 0.0f};
static int registers, unregisters;

static AoeLiveUnit target;
static DatUnit target_t;
static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    if (id == unit.core.unit_id) return &unit;
    return id == target.core.unit_id ? &target : NULL;
}

/* Mark quarter cell (qx, qy) occupied (2 bits per quarter y). */
static void occupy(AoePathfinderRor *p, int qx, int qy) {
    p->occupancy[qx * 255 + (qy >> 2)] |= (uint8_t)(1u << (2 * (qy & 3)));
}
static float speed(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 1.0f; }
static AoeUnitPathRor *path_of(void *c, AoeLiveUnit *u) { (void)c; (void)u; return &upath; }
static int tile(void *c, int32_t x, int32_t y, uint8_t *b5, uint8_t *b6) {
    (void)c;
    if (x < 0 || y < 0 || x >= W || y >= H) return 0;
    *b5 = terrain[y][x];
    *b6 = 0;
    return 1;
}
static int16_t overlay(void *c, uint8_t o) { (void)c; (void)o; return 1; }
static int32_t reg(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++registers; return 1; }
static void unreg(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++unregisters; }
static int centered(void *c, AoeLiveUnit *u, float x, float y) {
    (void)c; (void)u; (void)x; (void)y;
    return 1;
}
static int32_t retries(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0; }
static uint8_t busy(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0; }
static float rate(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0.0f; }
static float field130(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0.0f; }
static uint8_t flag85(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0; }
static int attached(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0; }
static void adjust(void *c, AoeLiveUnit *u, int32_t lo[2], int32_t hi[2]) {
    (void)c; (void)u; (void)lo; (void)hi;
}
static int32_t query(void *c, float r, int32_t f, int32_t x, int32_t y,
                     int32_t *count) {
    (void)c; (void)r; (void)f; (void)x; (void)y;
    *count = 0;
    return -1;
}
static int16_t owner_id(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 1; }
static int32_t owner_kind(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0; }

static AoePathfinderRor make_pathfinder(void) {
    AoePathfinderRor p;
    memset(&p, 0, sizeof(p));
    aoe_path_set_map_ror(&p, 1, W, H, 0, 0);
    p.state = malloc(AOE_PATH_GRID_CELLS_ROR);
    p.cost = calloc(AOE_PATH_GRID_CELLS_ROR, sizeof(int32_t));
    p.heap_capacity = 65026;
    p.heap = calloc(p.heap_capacity, sizeof(AoePathHeapEntryRor));
    p.occupancy = calloc(1, AOE_PATH_OCCUPANCY_BYTES_ROR +
                                AOE_PATH_OCCUPANCY_PAD_ROR);
    p.penalty = calloc(1, AOE_PATH_GRID_CELLS_ROR);
    memset(p.state, 0xff, AOE_PATH_GRID_CELLS_ROR);
    return p;
}

static void free_pathfinder(AoePathfinderRor *p) {
    free(p->state); free(p->cost); free(p->heap); free(p->occupancy);
    free(p->penalty); free(p->nearby_ids);
}

/* Every waypoint must lie on passable terrain, consecutive waypoints must
   be straight or diagonal lines, and the path must start at the start. */
static void check_path(int32_t sx, int32_t sy) {
    assert(upath.count >= 1);
    assert((int)upath.points[0].x == sx && (int)upath.points[0].y == sy);
    for (int32_t i = 0; i < upath.count; ++i) {
        int x = (int)upath.points[i].x, y = (int)upath.points[i].y;
        assert(terrain[y][x] == 0);
        if (i) {
            int px = (int)upath.points[i - 1].x, py = (int)upath.points[i - 1].y;
            int dx = abs(x - px), dy = abs(y - py);
            assert(dx == 0 || dy == 0 || dx == dy);
        }
    }
}

int main(void) {
    int32_t table[256], returned;
    float distance = -1.0f;
    AoePathfinderRor p = make_pathfinder();
    AoePathWorldOps ops = {0};
    AoePathRequestRor req;
    AoePathResultRor res;
    aoe_path_distance_table_ror(table);
    ops.resolve = resolve; ops.speed = speed; ops.unit_path = path_of;
    ops.terrain_costs = costs; ops.terrain_cost_count = 32;
    ops.free_terrain_a = ops.free_terrain_b = -1;
    ops.map_width = W; ops.map_height = H;
    ops.tile = tile; ops.overlay_terrain = overlay;
    ops.lift_obstruction = reg; ops.restore_obstruction = unreg;
    ops.centered = centered; ops.retry_count = retries;
    ops.member_busy = busy; ops.member_rate = rate;
    ops.member_field_130 = field130; ops.member_flag_85 = flag85;
    ops.attached = attached; ops.footprint_adjust = adjust;
    ops.query = query; ops.owner_id = owner_id; ops.owner_kind = owner_kind;

    unit.constructed = 1;
    unit.core.unit_id = 5;
    unit.core.unit_template = &unit_t;
    unit_t.size[0] = unit_t.size[1] = 0.2f;
    unit.core.position.x38 = 2.5f;
    unit.core.position.y3c = 2.5f;
    unit.core.position.z40 = 3.0f;

    memset(&req, 0, sizeof(req));
    req.x1 = 2; req.y1 = 2; req.x2 = 12; req.y2 = 2;
    req.unit = &unit; req.range = 0.0f; req.target_id = -1; req.mode = 1;
    req.step = 1; req.query_filter = -1; req.budget_base = 2500;
    req.budget_limited = 1;

    /* Open ground: a straight run to the goal (goal radius = 16 = 1 tile
       because step 1 > range 0). */
    res = aoe_path_find_ror(&p, table, &req, &ops, &returned, &distance);
    assert(res == AOE_PATH_RESULT_GOAL_ROR && returned == 1);
    check_path(2, 2);
    /* One tile away is exactly 16, not below the radius: it walks onto
       the goal tile, with a single straight segment (two waypoints). */
    assert(upath.count == 2 && upath.points[0].direction == 0xff &&
           upath.points[1].direction == 3 &&
           upath.points[upath.count - 1].x == 12.5f &&
           upath.points[upath.count - 1].y == 2.5f &&
           upath.points[0].z == 3.0f);
    assert(registers == 1 && unregisters == 1); /* own obstruction */
    printf("open: %d waypoints, distance %g, expanded %d\n", upath.count,
           distance, p.expanded);

    /* A water wall at x = 7 for y 0..15: the path must go around. */
    for (int y = 0; y <= 15; ++y) terrain[y][7] = 1;
    res = aoe_path_find_ror(&p, table, &req, &ops, &returned, &distance);
    assert(res == AOE_PATH_RESULT_GOAL_ROR && returned == 1);
    check_path(2, 2);
    {
        int crossed_low = 0;
        for (int32_t i = 0; i < upath.count; ++i)
            if (upath.points[i].y >= 16.0f) crossed_low = 1;
        assert(crossed_low);
    }
    printf("wall: %d waypoints, distance %g, expanded %d\n", upath.count,
           distance, p.expanded);

    /* Sealed goal: a full wall. The search ends partial and the unit heads
       for the best (closest) node found. */
    for (int y = 0; y < H; ++y) terrain[y][7] = 1;
    res = aoe_path_find_ror(&p, table, &req, &ops, &returned, &distance);
    assert(res == AOE_PATH_RESULT_PARTIAL_ROR && returned == 0);
    assert(p.best_x == 6);
    check_path(2, 2);
    printf("sealed: best (%u,%u) distance %g\n", p.best_x, p.best_y, distance);

    /* Already in range: precheck success, path cleared. */
    req.range = 20.0f;
    res = aoe_path_find_ror(&p, table, &req, &ops, &returned, &distance);
    assert(res == AOE_PATH_RESULT_IN_RANGE_ROR && returned == 1 &&
           upath.count == 0 && distance == 0.0f);

    /* Generation rollover: 31 searches reuse the state grid safely. */
    req.range = 0.0f;
    for (int y = 0; y < H; ++y) terrain[y][7] = 0;
    for (int i = 0; i < 40; ++i) {
        res = aoe_path_find_ror(&p, table, &req, &ops, &returned, &distance);
        assert(res == AOE_PATH_RESULT_GOAL_ROR);
    }

    /* Occupancy: a target building occupying quarters (40..47, 8..15)
       (tiles 10-11, 2-3). With check_occupancy the unit stops when its
       footprint touches the target rectangle (cost 0, TOUCH). */
    target.constructed = 1;
    target.core.unit_id = 9;
    target.core.unit_template = &target_t;
    target_t.size[0] = target_t.size[1] = 1.0f;
    target.core.position.x38 = 11.0f;
    target.core.position.y3c = 3.0f;
    for (int qx = 40; qx < 48; ++qx)
        for (int qy = 8; qy < 16; ++qy) occupy(&p, qx, qy);
    req.x2 = 11; req.y2 = 2; req.target_id = 9; req.check_occupancy = 1;
    res = aoe_path_find_ror(&p, table, &req, &ops, &returned, &distance);
    assert(res == AOE_PATH_RESULT_TOUCH_ROR && returned == 1);
    assert(p.target_rect[0] == 40 && p.target_rect[1] == 8 &&
           p.target_rect[2] == 48 && p.target_rect[3] == 16);
    check_path(2, 2);
    printf("touch: %d waypoints, last x %g\n", upath.count,
           upath.points[upath.count - 1].x);
    /* Without a target the same occupied cells simply block: the goal is
       inside them, so the search ends partial next to them. */
    req.target_id = -1;
    res = aoe_path_find_ror(&p, table, &req, &ops, &returned, &distance);
    assert(res == AOE_PATH_RESULT_PARTIAL_ROR && returned == 0);
    req.check_occupancy = 0;
    req.target_id = -1;
    memset(p.occupancy, 0, AOE_PATH_OCCUPANCY_BYTES_ROR);

    /* Waypoint list limit: prepend refuses beyond 1000 entries. */
    {
        AoeUnitPathRor big = {0};
        AoePathWaypointRor w = {0};
        for (int i = 0; i <= 1000; ++i)
            assert(aoe_unit_path_prepend_ror(&big, &w));
        assert(big.count == 1001 && !aoe_unit_path_prepend_ror(&big, &w));
        aoe_unit_path_free_ror(&big);
    }
    /* 0x0045a4f0 fit checks. A 0.2-tile unit at (3.5, 3.5) covers quarter
       cells [13, 14) on both axes. */
    memset(terrain, 0, sizeof(terrain));
    memset(p.occupancy, 0, AOE_PATH_OCCUPANCY_BYTES_ROR);
    p.target_id = -1;
    registers = unregisters = 0;
    assert(aoe_path_fits_ror(&p, &ops, &unit, 3.5f, 3.5f, 1) == 1 &&
           registers == 1 && unregisters == 1);
    occupy(&p, 14, 13); /* just outside the exclusive high edge */
    assert(aoe_path_fits_ror(&p, &ops, &unit, 3.5f, 3.5f, 1) == 1);
    occupy(&p, 13, 13);
    assert(aoe_path_fits_ror(&p, &ops, &unit, 3.5f, 3.5f, 1) == 0 &&
           registers == 3 && unregisters == 3);
    /* Inside the last search's target rectangle the slow sweep runs, and a
       touch still means "does not fit". */
    p.target_id = 9;
    p.target_rect[0] = p.target_rect[1] = 12;
    p.target_rect[2] = p.target_rect[3] = 15;
    assert(aoe_path_fits_ror(&p, &ops, &unit, 3.5f, 3.5f, 1) == 0);
    memset(p.occupancy, 0, AOE_PATH_OCCUPANCY_BYTES_ROR);
    assert(aoe_path_fits_ror(&p, &ops, &unit, 3.5f, 3.5f, 1) == 1);
    p.target_id = -1;
    /* Terrain is only checked when asked, with no start-tile exemption. */
    terrain[3][3] = 1;
    assert(aoe_path_fits_ror(&p, &ops, &unit, 3.5f, 3.5f, 1) == 0 &&
           registers == 5);
    assert(aoe_path_fits_ror(&p, &ops, &unit, 3.5f, 3.5f, 0) == 1);
    terrain[3][3] = 0;
    /* At x = 0.1 the low quarter edge is -1, nudged up to 0. */
    occupy(&p, 0, 13);
    assert(aoe_path_fits_ror(&p, &ops, &unit, 0.1f, 3.5f, 1) == 0);
    memset(p.occupancy, 0, AOE_PATH_OCCUPANCY_BYTES_ROR);
    assert(aoe_path_fits_ror(&p, &ops, &unit, 0.1f, 3.5f, 1) == 1);
    /* Beyond the map edge the terrain sweep fails. */
    assert(aoe_path_fits_ror(&p, &ops, &unit, 23.9f, 3.5f, 1) == 0);
    assert(aoe_path_fits_ror(&p, NULL, &unit, 3.5f, 3.5f, 1) == -1);

    /* Occupancy counts saturate at 3 and floor at 0. */
    memset(p.occupancy, 0, AOE_PATH_OCCUPANCY_BYTES_ROR);
    for (int i = 0; i < 4; ++i) assert(aoe_path_occupancy_add_ror(&p, 2, 7));
    assert(p.occupancy[2 * 255 + 1] == 0xc0);
    assert(aoe_path_occupancy_remove_ror(&p, 2, 7) &&
           p.occupancy[2 * 255 + 1] == 0x80);
    assert(aoe_path_occupancy_remove_ror(&p, 2, 7) &&
           aoe_path_occupancy_remove_ror(&p, 2, 7) &&
           aoe_path_occupancy_remove_ror(&p, 2, 7) &&
           p.occupancy[2 * 255 + 1] == 0);
    assert(!aoe_path_occupancy_add_ror(&p, 1020, 0) &&
           !aoe_path_occupancy_add_ror(&p, 0, 1020) &&
           !aoe_path_occupancy_add_ror(&p, -1, 0));

    /* Footprint lift/restore 0x004ab7f0/0x004ab6e0: a 0.2-tile unit at
       (3.5, 3.5) counts quarter cell 13 on both axes: [ftol(13.2),
       ftol(14.8)) = [13, 14). */
    {
        AoePathfinderRor second = make_pathfinder();
        AoeObstructionStateRor st = {0, 7};
        unit.core.position.x38 = unit.core.position.y3c = 3.5f;
        unit_t.size[2] = 1.0f;
        assert(aoe_unit_obstruction_lift_ror(&unit, &st, &p, &second, 0) == 0 &&
               st.previous == 7);
        assert(aoe_unit_obstruction_restore_ror(&unit, &st, &p, &second, 0) ==
                   1 && st.placed == 1 && st.previous == 0);
        assert(p.occupancy[13 * 255 + 3] == 0x04 &&
               p.occupancy[12 * 255 + 3] == 0 &&
               p.occupancy[14 * 255 + 3] == 0 &&
               second.occupancy[13 * 255 + 3] == 0);
        /* Already placed: unchanged. */
        assert(aoe_unit_obstruction_restore_ror(&unit, &st, &p, &second, 0) ==
                   0 && p.occupancy[13 * 255 + 3] == 0x04);
        /* The fit check sees the count unless the unit's own footprint is
           lifted (the test's lift callback does not touch the map). */
        assert(aoe_path_fits_ror(&p, &ops, &unit, 3.5f, 3.5f, 1) == 0);
        /* Selector 2 lifts from both maps (the second one is empty and
           stays 0); selector 1 places in the second only. */
        assert(aoe_unit_obstruction_lift_ror(&unit, &st, &p, &second, 2) == 1 &&
               st.placed == 0 && st.previous == 1 &&
               p.occupancy[13 * 255 + 3] == 0 &&
               second.occupancy[13 * 255 + 3] == 0);
        assert(aoe_unit_obstruction_restore_ror(&unit, &st, &p, &second, 1) == 1 &&
               p.occupancy[13 * 255 + 3] == 0 &&
               second.occupancy[13 * 255 + 3] == 0x04);
        /* A flat (height 0) template never counts. */
        unit_t.size[2] = 0.0f;
        st.placed = 0;
        assert(aoe_unit_obstruction_restore_ror(&unit, &st, &p, &second, 0) == 0);
        assert(aoe_unit_obstruction_restore_ror(NULL, &st, &p, &second, 0) == -1);
        free_pathfinder(&second);
    }

    aoe_unit_path_free_ror(&upath);
    free_pathfinder(&p);
    return 0;
}
