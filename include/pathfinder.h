#ifndef VITA_AOE_PATHFINDER_H
#define VITA_AOE_PATHFINDER_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a tile pathfinder (vtable 0x00544cf0,
   constructor 0x00458570, global instance 0x006a1cc0 built by 0x004584e0
   (0xff, 0xff, 0, 0, 0) and a second at 0x00583bc8 by 0x00458530 (third
   argument 1)). Grids are fixed at
   255 x 255 cells and indexed [first * 255 + second], as 0x00458800 does.
   This header covers the exact foundation; the search body 0x00458930 is
   translated incrementally. */
enum { AOE_PATH_GRID_SIDE_ROR = 255,
       AOE_PATH_GRID_CELLS_ROR = 255 * 255 };

/* Quarter-tile occupancy: 1020 rows (quarter x) of 255 bytes (tile y),
   two bits per quarter y. Reads take overlapping dwords, so storage needs
   AOE_PATH_OCCUPANCY_PAD_ROR bytes of padding after the last row. */
enum { AOE_PATH_OCCUPANCY_ROWS_ROR = 1020,
       AOE_PATH_OCCUPANCY_BYTES_ROR = 1020 * 255,
       AOE_PATH_OCCUPANCY_PAD_ROR = 8 };

typedef struct {
    uint8_t x, y;
    int32_t cost;
} AoePathHeapEntryRor;

typedef struct {
    int32_t width, height;      /* +0x04/+0x08 */
    uint8_t generation;         /* +0x11dcd8, starts at 0xf0 */
    uint8_t *state;             /* +0x0c: generation | arrival direction */
    int32_t *cost;              /* +0xfe10: f-costs */
    AoePathHeapEntryRor *heap;  /* +0x4f614, 1-based; [0] is the sentinel */
    uint32_t heap_capacity;
    uint8_t *occupancy;         /* +0xce624 (see above) */
    uint8_t *penalty;           /* +0x10de28: extra cost per cell (x16) */
    /* +0x11dce4/+0x11dce8/+0x11dcf0: units met along the last path. */
    int32_t *nearby_ids;
    uint32_t nearby_count, nearby_capacity;
    /* Search outputs. */
    int32_t heap_size;          /* +0x11dc2c */
    int32_t expanded;           /* +0x11dc38 */
    uint8_t best_x, best_y;     /* +0x11dc30/+0x11dc31 */
    int32_t best_distance;      /* +0x11dc34 */
    int32_t closest;            /* +0x11dcc8 */
    float offset_x, offset_y;   /* +0x11dc54/+0x11dc58 */
    int32_t target_rect[4];     /* +0x11dc84..+0x11dc90, quarter tiles */
    /* +0x11dc4c: the last search's target ID (-1 when it did not resolve),
       left for later fit checks. */
    int32_t target_id;
    int32_t searches;           /* +0x11dcc4 (0x0045a4e0 counts one) */
    /* +0x11dc64: a second ID list (count, capacity) the translated parts
       do not fill; the destructor frees it. */
    int32_t *list_11dc64;
    uint32_t list_11dc64_count, list_11dc64_capacity;
} AoePathfinderRor;

/* 0x004586f0, the destructor: the nearby IDs (+0x11dce4) and the
   +0x11dc64 list freed and emptied. The two global instances register it
   with atexit (0x00458500 for 0x006a1cc0, 0x00458550 for 0x00583bc8). */
void aoe_path_release_ror(AoePathfinderRor *path);

/* 0x004587e0: the penalty byte of cell [first * 255 + second] (0 without
   storage or outside the grid; the original reads unchecked). */
uint8_t aoe_path_penalty_ror(const AoePathfinderRor *path, int32_t first,
                             int32_t second);
/* 0x0045a4e0: one more search counted. */
void aoe_path_count_search_ror(AoePathfinderRor *path);

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
/* 0x00458820 (penalty fill) / 0x00458850 (occupancy clear) / 0x00458800
   (one penalty cell). Return 0 for missing storage or out-of-grid cells
   (the original writes unchecked). */
int aoe_path_fill_penalty_ror(AoePathfinderRor *path, uint8_t value);
int aoe_path_clear_occupancy_ror(AoePathfinderRor *path);
int aoe_path_set_penalty_ror(AoePathfinderRor *path, int32_t first,
                             int32_t second, uint8_t value);

/* 0x00458870 / 0x004588d0: the 2-bit occupancy fields are saturating
   counts. Add raises quarter cell (qx, qy) by one up to 3; remove lowers
   it by one when positive. Return 0 for missing storage or cells outside
   the bitmap (the original writes unchecked). */
int aoe_path_occupancy_add_ror(AoePathfinderRor *path, int32_t qx,
                               int32_t qy);
int aoe_path_occupancy_remove_ror(AoePathfinderRor *path, int32_t qx,
                                  int32_t qy);

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

/* A growable ID list: data, count (+4) and capacity (+0xc). */
typedef struct {
    int32_t *ids;
    int32_t count;
    int32_t capacity;
} AoePathIdListRor;

/* 0x0045ade0: append to `out` (each ID once, growing it one slot at a
   time) the pathfinder's met unit IDs from index nearby_count down to 0.
   The original starts one past the last ID: when that slot lies beyond
   the array it grows the array (leaving the slot uninitialised) and
   appends whatever the slot holds. Here a slot inside the capacity is
   read as in the original (a stale ID), while one beyond it is skipped
   (host-only guard). Returns 0 on allocation failure. */
int aoe_path_collect_nearby_ror(const AoePathfinderRor *path,
                                AoePathIdListRor *out);

#endif
