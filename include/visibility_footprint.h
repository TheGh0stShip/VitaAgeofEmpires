#ifndef VITA_AOE_VISIBILITY_FOOTPRINT_H
#define VITA_AOE_VISIBILITY_FOOTPRINT_H

#include <stdint.h>

/* Semantic fields of the RoR player visibility map used by disc 0x0051a2b0 (patched 0x00517860)
   (enter) and disc 0x0051a490 (patched 0x00517a40) (leave). Arrays are runtime row[y][x] order. */
typedef struct {
    uint32_t width, height;
    int8_t *counts;             /* player-specific byte grid */
    uint32_t *tile_masks;       /* shared runtime tile visibility masks */
    uint8_t *view_codes;        /* optional active-player tile +9 bytes */
    uint32_t visible_mask;      /* map +0x24 */
    uint32_t clear_visible_mask;/* map +0x28 */
    uint32_t explored_mask;     /* map +0x2c */
    uint32_t explored_count;    /* map +0x1c */
    uint8_t suspended;          /* global 0x007d74c8 */
} AoeVisibilityFootprintGrid;

typedef struct {
    void *owner_context;
    void (*record_owner)(void *context, int32_t x, int32_t y);
    void *active_context;
    /* When the original active-player relation entry equals 1, RoR
       0x0051a2b0 also appends to that player's distinct +0x6c list. */
    void (*record_active)(void *context, int32_t x, int32_t y);
} AoeVisibilityFootprintOps;

/* Exact row half-width from the executable's PTR_DAT_00562790 table, with
   radius clamped to 16. `row` is a signed offset from the center. */
int aoe_visibility_footprint_half_width(int radius, int row, int *half_width);

/* The radius's row table (patched 0x0055f76c[radius], radius clamped to
   0..16 as 0x00517820 does): its half-widths, rows 0..radius (negative
   rows mirror). Writes the clamped radius when `clamped` is non-null. */
const uint8_t *aoe_visibility_footprint_rows(int radius, int *clamped);

/* 0x004aa9e0 (live-unit slot in every unit vtable): the unit's sight
   footprint table, 0x00517820(trunc(template +0x28 line of sight), 0);
   NULL without an owner (+0x0c). */
const uint8_t *aoe_unit_sight_footprint_rows_ror(int has_owner,
                                                 float line_of_sight,
                                                 int *clamped);

/* Direct circle-table branch of RoR 0x0051a2b0/0x0051a490. The original
   unit hooks truncate unit X/Y and line of sight toward zero before calling
   these functions. `double_explore` is the original map/player relation
   predicate; callers must obtain it from actual player state. Owner and
   active-player append destinations remain distinct. */
int aoe_visibility_footprint_enter(AoeVisibilityFootprintGrid *grid,
                                    int32_t center_x, int32_t center_y,
                                    int radius, int double_explore,
                                    const AoeVisibilityFootprintOps *ops,
                                    uint32_t *new_cells);
int aoe_visibility_footprint_leave(AoeVisibilityFootprintGrid *grid,
                                    int32_t center_x, int32_t center_y,
                                    int radius);

/* RoR disc 0x0051a5a0/0x0051a770 (patched 0x00517b50/0x00517d10) alternate rectangular branch. The inclusive
   bounds are formed by the unit hook from X/Y +/- template size and clipped
   to the map. Cell count/mask/discovery behavior matches the circle branch. */
int aoe_visibility_rectangle_enter(AoeVisibilityFootprintGrid *grid,
                                    int32_t x0, int32_t y0,
                                    int32_t x1, int32_t y1,
                                    int double_explore,
                                    const AoeVisibilityFootprintOps *ops,
                                    uint32_t *new_cells);
int aoe_visibility_rectangle_leave(AoeVisibilityFootprintGrid *grid,
                                    int32_t x0, int32_t y0,
                                    int32_t x1, int32_t y1);

#endif
