#ifndef VITA_AOE_DRAW_PRIMS_H
#define VITA_AOE_DRAW_PRIMS_H

#include <stdint.h>

#include "shape.h"

/* Patched Rise of Rome 1.0a solid drawing on an 8-bit surface
   (0x00439ad0..0x0043a350). Everything is clipped to the surface's clip
   box; nothing is drawn without bits. */

/* 0x00439b70: `len` pixels of row y from column x. 0x00439c00: `len`
   pixels of column x from row y down. */
void aoe_draw_hline_ror(AoeShapeSurfaceRor *s, int32_t x, int32_t y,
                        int32_t len, uint8_t color);
void aoe_draw_vline_ror(AoeShapeSurfaceRor *s, int32_t x, int32_t y,
                        int32_t len, uint8_t color);
/* 0x00439c90: a filled box (corners in either order). 0x00439ad0: its
   outline. */
void aoe_draw_fill_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                       int32_t x1, int32_t y1, uint8_t color);
void aoe_draw_rect_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                       int32_t x1, int32_t y1, uint8_t color);

/* Bevels: rings drawn inwards, each with a top/right colour and a
   bottom/left colour. 0x00439d90 one ring (a top/right, b bottom/left).
   0x00439e40 two rings (outer c[0] / c[3], inner c[1] / c[2]).
   0x00439f60 three rings (c[0] / c[5], c[1] / c[4], c[2] / c[3]).
   0x0043a100: outer c[0] / c[3] and a second top/right line c[1] that
   starts one column further left; c[2] is unused. 0x0043a1e0: like
   0x00439f60 with the third top/right line one column further left and
   no third bottom/left line (c[3] unused). */
void aoe_draw_bevel1_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                         int32_t x1, int32_t y1, uint8_t a, uint8_t b);
void aoe_draw_bevel2_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                         int32_t x1, int32_t y1, const uint8_t c[4]);
void aoe_draw_bevel3_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                         int32_t x1, int32_t y1, const uint8_t c[6]);
void aoe_draw_bevel2_flat_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                              int32_t x1, int32_t y1, const uint8_t c[4]);
void aoe_draw_bevel3_flat_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                              int32_t x1, int32_t y1, const uint8_t c[6]);

/* 0x0043a340 sets the shading table; 0x0043a350 passes every pixel of a
   box (left, top, right, bottom; clipped to the clip box and the
   surface) through it. */
void aoe_draw_set_shade_ror(AoeShapeSurfaceRor *s, const uint8_t *table);
void aoe_draw_shade_ror(AoeShapeSurfaceRor *s, int32_t left, int32_t top,
                        int32_t right, int32_t bottom);

#endif
