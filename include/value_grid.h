#ifndef VITA_AOE_VALUE_GRID_H
#define VITA_AOE_VALUE_GRID_H

#include <stdint.h>

#include "aoe_stream.h"

/* Patched Rise of Rome 1.0a byte grid (0x0043d420..0x0043dfd0, vtable
   0x005443c8): width x height byte cells addressed from an origin through
   per-row pointers, with a cap that raising saturates below and that
   marks cells raising and lowering leave alone. Semantic storage; offsets
   are noted. */
typedef struct {
    int32_t width, height;       /* +0x04, +0x08 */
    int32_t origin_x, origin_y;  /* +0x0c, +0x10 */
    uint8_t *cells;              /* +0x14 */
    uint8_t **rows;              /* +0x18 */
    int32_t count;               /* +0x20: the neighbour count */
    uint8_t cap;                 /* +0x24 */
} AoeValueGridRor;

/* 0x0043d420: a grid filled with `fill`, origin (0, 0) (the original
   leaves the cap unset; the port starts it at 0). 0x0043d4e0 frees it;
   0x0043d4c0 also frees the object when asked. 0x0043d510 fills it. */
int aoe_value_grid_init_ror(AoeValueGridRor *g, int32_t w, int32_t h,
                            uint8_t fill);
void aoe_value_grid_free_ror(AoeValueGridRor *g);
AoeValueGridRor *aoe_value_grid_destroy_ror(AoeValueGridRor *g, int heap);
int aoe_value_grid_fill_ror(AoeValueGridRor *g, uint8_t value);
/* 0x0043d550 / 0x0043d5f0: width, height, origin, cap (one byte) and the
   cells row by row. The original loads the cells into the rows it has;
   the port reallocates when the size changes. */
void aoe_value_grid_save_ror(const AoeValueGridRor *g, AoeOutcomeStream *out);
int aoe_value_grid_load_ror(AoeValueGridRor *g, AoeOutcomeStream *in);

/* Cells by map position (origin-relative). 0x0043d6a0 reads one without a
   range check. 0x0043d6d0 sets one; 0x0043d720 sets a box whose corners
   must both be inside (nonzero when done). */
uint8_t aoe_value_grid_get_ror(const AoeValueGridRor *g, int32_t x, int32_t y);
int aoe_value_grid_set_ror(AoeValueGridRor *g, int32_t x, int32_t y,
                           uint8_t value);
int aoe_value_grid_set_box_ror(AoeValueGridRor *g, int32_t x0, int32_t y0,
                               int32_t x1, int32_t y1, uint8_t value);
/* 0x0043d7b0 raise a cell below the cap by `amount`, at most to cap - 1;
   0x0043d940 lower one below the cap, not under 0. 0x0043d830 /
   0x0043d9b0: the same over a box clamped into the grid (the original
   clamps rows by the width too; the port also keeps them inside). */
int aoe_value_grid_raise_ror(AoeValueGridRor *g, int32_t x, int32_t y,
                             uint8_t amount);
int aoe_value_grid_lower_ror(AoeValueGridRor *g, int32_t x, int32_t y,
                             uint8_t amount);
int aoe_value_grid_raise_box_ror(AoeValueGridRor *g, int32_t x0, int32_t y0,
                                 int32_t x1, int32_t y1, uint8_t amount);
int aoe_value_grid_lower_box_ror(AoeValueGridRor *g, int32_t x0, int32_t y0,
                                 int32_t x1, int32_t y1, uint8_t amount);
/* 0x0043dfd0: a nonzero cap. */
void aoe_value_grid_set_cap_ror(AoeValueGridRor *g, uint8_t cap);

/* Neighbour scans over the cells from..to (grid coordinates, both inside;
   0 otherwise). Neighbours run clockwise from the top-left; the even ones
   are diagonal and are looked at only with `diagonals`. 0x0043dab0: for
   each cell at or above `threshold`, count its neighbours at or above it,
   plus the ones off the grid when `off_edge` (side neighbours) or
   `off_corner` (diagonal ones) is 1; the total is kept in `count`.
   0x0043dcb0: from `count`, for each cell that reaches `threshold` with
   `bonus` added: when the cell itself reaches it, one off-grid pass
   taking one per counted off-grid neighbour (stopping once the score is
   below 1); then another pass taking one per counted off-grid neighbour
   and adding two per neighbour at or above `threshold`. */
int32_t aoe_value_grid_count_ror(AoeValueGridRor *g, const int32_t from[2],
                                 const int32_t to[2], int32_t threshold,
                                 int32_t off_edge, int32_t off_corner,
                                 int32_t diagonals);
int32_t aoe_value_grid_score_ror(const AoeValueGridRor *g,
                                 const int32_t from[2], const int32_t to[2],
                                 uint8_t threshold, uint8_t bonus,
                                 int32_t off_edge, int32_t off_corner,
                                 int32_t diagonals);

#endif
