#ifndef VITA_AOE_PATHFINDER_H
#define VITA_AOE_PATHFINDER_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a tile pathfinder (vtable 0x00544cf0,
   constructor 0x00458570, global instance 0x006a1cc0). Grids are fixed at
   255 x 255 cells and indexed [first * 255 + second], as 0x00458800 does.
   This header covers the exact foundation; the search body 0x00458930 is
   translated incrementally. */
enum { AOE_PATH_GRID_SIDE_ROR = 255,
       AOE_PATH_GRID_CELLS_ROR = 255 * 255 };

typedef struct {
    int32_t width, height;      /* +0x04/+0x08 */
    uint8_t generation;         /* +0x11dcd8, starts at 0xf0 */
    uint8_t *blocked;           /* +0x10de28, AOE_PATH_GRID_CELLS_ROR bytes */
    uint32_t *visited;          /* +0xce624, AOE_PATH_GRID_CELLS_ROR dwords */
} AoePathfinderRor;

/* Global table 0x006a18c0, filled by the constructor: entry i is
   __ftol(i * 1.4142f * 16.0f + 0.5f), the diagonal cost of i steps in
   sixteenths of a tile. */
void aoe_path_distance_table_ror(int32_t table[256]);

/* Inline octile distance used throughout 0x00458930: the shorter axis
   difference (masked to a byte) through the table plus 16 per extra
   straight step. */
int32_t aoe_path_octile_ror(const int32_t table[256], int32_t x1, int32_t y1,
                            int32_t x2, int32_t y2);

/* 0x00458790: dimensions come from the map (+0x08/+0x0c) when one is
   given, otherwise from the arguments; the generation byte becomes 0xf0. */
void aoe_path_set_map_ror(AoePathfinderRor *path, int has_map,
                          int32_t map_width, int32_t map_height,
                          int32_t width, int32_t height);
/* 0x00458820 / 0x00458850 / 0x00458800. Return 0 for missing storage or
   out-of-grid cells (the original writes unchecked). */
int aoe_path_fill_blocked_ror(AoePathfinderRor *path, uint8_t value);
int aoe_path_clear_visited_ror(AoePathfinderRor *path);
int aoe_path_set_blocked_ror(AoePathfinderRor *path, int32_t first,
                             int32_t second, uint8_t value);

typedef enum {
    AOE_PATH_PRECHECK_FAIL_ROR = 0,     /* return 0 */
    AOE_PATH_PRECHECK_IN_RANGE_ROR = 1, /* return 1 with distance 0 */
    AOE_PATH_PRECHECK_SEARCH_ROR = 2    /* continue into the search */
} AoePathPrecheckRor;

typedef struct {
    void *context;
    float (*speed)(void *context, void *unit);   /* unit +0xfc */
    /* 0x00458330 on unit +0xa4, or +0xd8 when byte +0x154 is set. */
    void (*clear_unit_path)(void *context, void *unit);
} AoePathPrecheckOps;

/* Entry checks of 0x00458930(x1, y1, x2, y2, unit, range, ..., mode,
   *distance, ...). All four coordinates are checked against the width
   only (the original never compares them with the height). Out of bounds
   fails. With range > 1, a goal whose octile distance is below range * 16
   succeeds with distance 0, and a unit with speed 0 or NaN fails with
   distance 0. Mode 1 clears the unit's path on every early exit. */
AoePathPrecheckRor aoe_path_precheck_ror(const AoePathfinderRor *path,
                                         const int32_t table[256],
                                         int32_t x1, int32_t y1, int32_t x2,
                                         int32_t y2, void *unit, float range,
                                         int32_t mode, float *distance,
                                         const AoePathPrecheckOps *ops);

#endif
