#ifndef VITA_AOE_PATHFINDER_SEARCH_H
#define VITA_AOE_PATHFINDER_SEARCH_H

#include <stdint.h>

#include "live_unit_factory.h"
#include "pathfinder.h"

/* A unit path object (+0xa4 and +0xd8 of moving units): 16-byte waypoints
   at +0x24, count +0x2c, cursor +0x30. */
typedef struct {
    float x, y, z;
    uint8_t direction;
} AoePathWaypointRor;

typedef struct {
    AoePathWaypointRor *points;
    int32_t count, cursor;
} AoeUnitPathRor;

/* 0x00458330: count and cursor become 0 (storage is kept). */
void aoe_unit_path_clear_ror(AoeUnitPathRor *path);
/* 0x004581b0: prepend one waypoint, reallocating to count + 1. Fails
   (returns 0) once count exceeds 1000, or on allocation failure. */
int aoe_unit_path_prepend_ror(AoeUnitPathRor *path,
                              const AoePathWaypointRor *point);
void aoe_unit_path_free_ror(AoeUnitPathRor *path);

/* Arguments of 0x00458930 (15 stack arguments). */
typedef struct {
    int32_t x1, y1, x2, y2;   /* start and goal tiles */
    AoeLiveUnit *unit;
    float range;
    int32_t target_id;
    int32_t mode;             /* 1 builds the path and clears it on failure */
    int32_t budget_limited;   /* +0x2c: 1 honours the node budget */
    int32_t argument_30;      /* stored at +0x11dc78, otherwise unused */
    int32_t check_occupancy;  /* +0x34: 1 tests the occupancy bitmap */
    int32_t step;             /* +0x38: neighbour stride in tiles */
    float query_radius;       /* +0x3c -> +0x11dce0 */
    int32_t query_filter;     /* +0x40 -> +0x11dcdc; -1 disables queries */
    int32_t budget_base;      /* global 0x00554738 (2500); -1 unlimited */
} AoePathRequestRor;

typedef struct {
    void *context;
    AoeLiveUnit *(*resolve)(void *context, int32_t unit_id); /* 0x005206d0 */
    float (*speed)(void *context, AoeLiveUnit *unit);         /* +0xfc */
    /* +0xa4, or +0xd8 when byte +0x154 is set. */
    AoeUnitPathRor *(*unit_path)(void *context, AoeLiveUnit *unit);
    /* World +0x54[template +0x66]: passability per terrain (> 0 passes). */
    const float *terrain_costs;
    int32_t terrain_cost_count;
    /* Unit +0x14c/+0x150: terrains passable regardless of the table. */
    int32_t free_terrain_a, free_terrain_b;
    /* Map +0x08/+0x0c and its tiles (row y, column x): byte +5 (terrain in
       the low five bits) and byte +6 (overlay in the low four bits). */
    int32_t map_width, map_height;
    int (*tile)(void *context, int32_t x, int32_t y, uint8_t *byte5,
                uint8_t *byte6);
    /* Map +0x3928 + overlay * 0x5a0 (a short terrain ID). */
    int16_t (*overlay_terrain)(void *context, uint8_t overlay);
    /* Obstruction manager +0x11dcf4: register 0x004ab7f0 (result stored at
       +0x11dc48), unregister 0x004ab6e0. */
    int32_t (*register_obstruction)(void *context, AoeLiveUnit *unit);
    void (*unregister_obstruction)(void *context, AoeLiveUnit *unit);
    int (*centered)(void *context, AoeLiveUnit *unit, float x,
                    float y);                          /* 0x0045a4f0(.., 1) */
    int32_t (*retry_count)(void *context, AoeLiveUnit *unit);  /* +0x144 */
    uint8_t (*member_busy)(void *context, AoeLiveUnit *unit);  /* +0x130() */
    float (*member_rate)(void *context, AoeLiveUnit *unit);    /* +0xf4() */
    float (*member_field_130)(void *context, AoeLiveUnit *unit);
    uint8_t (*member_flag_85)(void *context, AoeLiveUnit *unit);
    int (*attached)(void *context, AoeLiveUnit *unit);         /* +0x20 */
    void (*footprint_adjust)(void *context, AoeLiveUnit *unit,
                             int32_t low[2], int32_t high[2]); /* 0x0044ef10 */
    /* 0x00520590(radius, filter, x, y, &count, 0): a unit ID or -1. */
    int32_t (*query)(void *context, float radius, int32_t filter, int32_t x,
                     int32_t y, int32_t *count);
    int16_t (*owner_id)(void *context, AoeLiveUnit *unit);     /* +0x4a */
    int32_t (*owner_kind)(void *context, AoeLiveUnit *unit);   /* +0x04 */
} AoePathWorldOps;

typedef enum {
    AOE_PATH_RESULT_ERROR_ROR = -1,   /* host-only: dependency or bounds */
    AOE_PATH_RESULT_FAIL_ROR = 0,     /* returned 0 */
    AOE_PATH_RESULT_IN_RANGE_ROR,     /* precheck success, returned 1 */
    AOE_PATH_RESULT_GOAL_ROR,         /* popped node within the goal radius */
    AOE_PATH_RESULT_TOUCH_ROR,        /* footprint touched the target */
    AOE_PATH_RESULT_PARTIAL_ROR       /* budget/heap exhausted, returned 0 */
} AoePathResultRor;

/* The full search 0x00458930 with path builder 0x0045a970. `returned` is
   the original 0/1 result and `distance` (when non-NULL) receives the
   original float result. The debug overlay behind global 0x007bf9b8 is not
   reproduced. */
AoePathResultRor aoe_path_find_ror(AoePathfinderRor *path,
                                   const int32_t table[256],
                                   const AoePathRequestRor *request,
                                   const AoePathWorldOps *ops,
                                   int32_t *returned, float *distance);

#endif
