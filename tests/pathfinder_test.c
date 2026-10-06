#include "pathfinder.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int cleared;
static float unit_speed = 1.0f;
static float speed(void *c, void *u) { (void)c; (void)u; return unit_speed; }
static void clear_path(void *c, void *u) { (void)c; (void)u; ++cleared; }

static void test_collect_nearby(void) {
    AoePathfinderRor path;
    int32_t ids[5] = {7, 8, 7, 9, 42};
    AoePathIdListRor out = {NULL, 0, 0};

    memset(&path, 0, sizeof(path));
    path.nearby_ids = ids;
    path.nearby_count = 4;
    path.nearby_capacity = 5;
    /* Index 4 (one past the count) is read too: a stale ID. */
    assert(aoe_path_collect_nearby_ror(&path, &out));
    assert(out.count == 4 && out.capacity == 4);
    assert(out.ids[0] == 42 && out.ids[1] == 9 && out.ids[2] == 7 &&
           out.ids[3] == 8);
    /* Beyond the capacity it is skipped; existing IDs are not repeated. */
    path.nearby_capacity = 4;
    path.nearby_count = 4;
    ids[0] = 5;
    assert(aoe_path_collect_nearby_ror(&path, &out));
    assert(out.count == 5 && out.ids[4] == 5);
    free(out.ids);
}

static void release_test(void) {
    AoePathfinderRor p;
    memset(&p, 0, sizeof p);
    p.nearby_ids = malloc(16);
    p.nearby_count = 2;
    p.nearby_capacity = 4;
    p.list_11dc64 = malloc(8);
    p.list_11dc64_count = 1;
    aoe_path_release_ror(&p);
    assert(!p.nearby_ids && !p.nearby_count && !p.nearby_capacity && !p.list_11dc64 &&
           !p.list_11dc64_count);
}

int main(void) {
    release_test();
    test_collect_nearby();
    int32_t table[256];
    AoePathfinderRor path;
    AoePathPrecheckOps ops = {0, speed, clear_path};
    float distance = -1.0f;
    int unit;

    /* Expected values computed independently from the float32 constant. */
    aoe_path_distance_table_ror(table);
    assert(table[0] == 0 && table[1] == 23 && table[2] == 45 &&
           table[3] == 68 && table[7] == 158 && table[100] == 2263 &&
           table[254] == 5747 && table[255] == 5770);
    /* Octile: 3 diagonal + 4 straight = 68 + 64; symmetric. */
    assert(aoe_path_octile_ror(table, 10, 10, 13, 17) == 68 + 64);
    assert(aoe_path_octile_ror(table, 13, 17, 10, 10) == 68 + 64);
    assert(aoe_path_octile_ror(table, 5, 5, 5, 5) == 0);

    memset(&path, 0, sizeof(path));
    aoe_path_set_map_ror(&path, 1, 64, 32, 0, 0);
    assert(path.width == 64 && path.height == 32 && path.generation == 0xf0);
    path.penalty = malloc(AOE_PATH_GRID_CELLS_ROR);
    path.occupancy = malloc(AOE_PATH_OCCUPANCY_BYTES_ROR +
                            AOE_PATH_OCCUPANCY_PAD_ROR);
    assert(aoe_path_fill_penalty_ror(&path, 0xab) &&
           path.penalty[0] == 0xab &&
           path.penalty[AOE_PATH_GRID_CELLS_ROR - 1] == 0xab);
    assert(aoe_path_set_penalty_ror(&path, 2, 3, 7) &&
           path.penalty[2 * 255 + 3] == 7);
    assert(!aoe_path_set_penalty_ror(&path, 255, 0, 1));
    path.occupancy[100] = 9;
    assert(aoe_path_clear_occupancy_ror(&path) && path.occupancy[100] == 0);

    /* Width-only bounds: y up to the width passes even beyond height 32. */
    assert(aoe_path_precheck_ror(&path, table, 1, 60, 5, 63, &unit, 0.0f, 0,
                                 &distance, &ops) ==
           AOE_PATH_PRECHECK_SEARCH_ROR);
    assert(aoe_path_precheck_ror(&path, table, 1, 65, 5, 6, &unit, 0.0f, 1,
                                 &distance, &ops) ==
               AOE_PATH_PRECHECK_FAIL_ROR && cleared == 1);
    /* In range: 3 tiles straight = 48 < 4 * 16 = 64. */
    assert(aoe_path_precheck_ror(&path, table, 1, 1, 4, 1, &unit, 4.0f, 1,
                                 &distance, &ops) ==
               AOE_PATH_PRECHECK_IN_RANGE_ROR &&
           distance == 0.0f && cleared == 2);
    /* Exactly on the boundary is not in range (strict). */
    assert(aoe_path_precheck_ror(&path, table, 1, 1, 5, 1, &unit, 4.0f, 0,
                                 &distance, &ops) ==
           AOE_PATH_PRECHECK_SEARCH_ROR);
    /* Range <= 1 skips the range and speed checks entirely. */
    unit_speed = 0.0f;
    assert(aoe_path_precheck_ror(&path, table, 1, 1, 30, 1, &unit, 1.0f, 0,
                                 &distance, &ops) ==
           AOE_PATH_PRECHECK_SEARCH_ROR);
    /* A stopped unit (speed 0 or NaN) fails with distance 0. */
    distance = -1.0f;
    assert(aoe_path_precheck_ror(&path, table, 1, 1, 30, 1, &unit, 2.0f, 0,
                                 &distance, &ops) ==
               AOE_PATH_PRECHECK_FAIL_ROR && distance == 0.0f);
    unit_speed = NAN;
    assert(aoe_path_precheck_ror(&path, table, 1, 1, 30, 1, &unit, 2.0f, 0,
                                 &distance, &ops) ==
           AOE_PATH_PRECHECK_FAIL_ROR);
    free(path.penalty);
    free(path.occupancy);
    return 0;
}
