#ifndef VITA_AOE_GRAPHIC_FRAME_H
#define VITA_AOE_GRAPHIC_FRAME_H

#include <stdint.h>

#include "dat.h"
#include "shape.h"

typedef struct {
    uint32_t slp_frame;
    uint8_t mirror_horizontal;
} AoeGraphicFrame;

/* RoR 0x004a6d10 (patched RoR 1.0a 0x004a5520, the same logic) / base
   0x004b2e60. Selects the stored SLP frame for a
   graphic angle and animation frame. The original caller bounds angles;
   this portable API rejects invalid ones and does not mutate output.
   Sprite slots over it (the low 16 bits kept): 0x00415c10 a static
   sprite's frame for an angle (animation frame 0), 0x004161c0 an
   animated one's (its current frame +0x14). */
int aoe_graphic_select_frame(const DatGraphic *graphic,
                             uint16_t angle, int16_t animation_frame,
                             AoeGraphicFrame *out);

/* Patched RoR 1.0a 0x004a5430: the sounds a graphic plays as an animation
   moves from frame `from` to frame `to` at `angle` (an angle at or past
   angle_count uses angle 0; equal frames play nothing). The graphic's own
   sound (when resolved, sound_id >= 0) plays on entering a cycle: `from`
   is 0, or the frame wrapped (to < from) onto a non-zero frame. With attack
   sounds, each of the angle's three (delay, sound) pairs up to the first
   negative delay plays when its delay is crossed: from <= delay < to going
   forward, delay >= from or delay < to after a wrap. Each play is
   0x004a2370(1) on that sound; `play` receives its ID. Returns the count. */
int aoe_graphic_frame_sounds(const DatGraphic *graphic, int16_t from,
                             int16_t to, int16_t angle,
                             void (*play)(void *context, int16_t sound_id),
                             void *context);

/* Graphic pixel test (the SLP-backed per-graphic test): whether
   local point (x, y) hits `graphic` at `angle` (facing) and `frame`. */
typedef int (*AoeGraphicPixelTestFn)(void *context, const DatGraphic *graphic,
                                     int32_t x, int32_t y, int32_t angle,
                                     int32_t frame);

/* Patched RoR 1.0a 0x004a6070: a point inside the graphic's non-empty
   coordinate box (x0..x1 on a non-zero x pair, y0..y1 on a non-zero y
   pair, inclusive) hits outright. With deltas, each delta graphic is
   pixel-tested at the point less its offset, with the angle rescaled to
   that graphic (delta angle_count * angle / own angle_count); the first
   hit wins. Without deltas the graphic itself is pixel-tested.
   Unresolved delta graphics (a host-only case) are skipped. */
int aoe_graphic_hit_test_ror(const DatGraphic *graphic, int32_t x, int32_t y,
                             int32_t angle, int32_t frame,
                             AoeGraphicPixelTestFn pixel, void *context);

/* Patched RoR 1.0a 0x004aafe0 (live-unit slot): the unit's current
   graphic hit test. The unit turns the screen point into graphic space
   (point - unit +0x28/+0x2a - anchor) and uses its facing (+0x35) and the
   sprite manager's current frame (+0x18 vtable +0x04); this tests (x, y)
   and, when that misses and `tolerance` is non-zero, every offset in
   [-tolerance, tolerance) on both axes, x-major. */
int aoe_graphic_hit_test_near_ror(const DatGraphic *graphic, int16_t x,
                                  int16_t y, int16_t tolerance,
                                  int32_t angle, int32_t frame,
                                  AoeGraphicPixelTestFn pixel, void *context);

/* Patched RoR 1.0a 0x004a5a50: draw the graphic's layer-20 parts at
   (x, y) for `angle` (at or past angle_count: 0). With deltas, every
   delta whose graphic has layer 20 and whose display angle is negative or
   equal to the angle is drawn (0x004a55e0) at its offset with the angle
   rescaled as in the hit test; without deltas the graphic itself when it
   is on layer 20. Returns the number drawn. */
int aoe_graphic_draw_layer20_ror(const DatGraphic *graphic, int32_t angle,
                                 int32_t x, int32_t y,
                                 void (*draw)(void *context,
                                              const DatGraphic *graphic,
                                              int32_t angle, int32_t x,
                                              int32_t y),
                                 void *context);

/* Patched RoR 1.0a 0x004a5b50: the same walk for the parts below layer
   20 whose layer differs from `skip_layer` (the graphic itself when it has
   no deltas). Returns the number drawn (the original's return value, a
   delta count, is not used by the port). */
int aoe_graphic_draw_lower_layers_ror(const DatGraphic *graphic,
                                      int32_t angle, int32_t x, int32_t y,
                                      uint8_t skip_layer,
                                      void (*draw)(void *context,
                                                   const DatGraphic *graphic,
                                                   int32_t angle, int32_t x,
                                                   int32_t y),
                                      void *context);

/* SLP frame bounds of a graphic (the original loads the SLP on first
   use): 0 when unavailable. */
typedef int (*AoeGraphicFrameBoundsFn)(void *context, const DatGraphic *graphic,
                                       uint32_t slp_frame, int32_t *x0,
                                       int32_t *y0, int32_t *x1, int32_t *y1);

/* Patched RoR 1.0a 0x004a5f50: grow the box (*x0, *y0)-(*x1, *y1) by one
   graphic part's frame bounds. The animation frame wraps to 0 at or past
   frame_count; the stored frame comes from 0x004a5520
   (aoe_graphic_select_frame); a mirrored frame swaps and negates the x
   bounds; the part offset (dx, dy) is added. Returns 0 (box untouched)
   when the bounds are unavailable. */
int aoe_graphic_union_part_bounds_ror(const DatGraphic *graphic,
                                      int32_t angle, int32_t frame,
                                      int16_t dx, int16_t dy,
                                      AoeGraphicFrameBoundsFn bounds,
                                      void *context, int16_t *x0, int16_t *y0,
                                      int16_t *x1, int16_t *y1);

/* Patched RoR 1.0a 0x004a5d60: the graphic's screen box at `angle` /
   `frame`. A non-empty coordinate box (as in the hit test) is returned
   as is. Otherwise the box starts at 0 and grows (0x004a5f50) by every
   delta part at its offset with the angle rescaled to that part (all
   deltas; no display-angle filter), or by the graphic itself without
   deltas. Every delta is skipped while the owning graphic's selection
   mode (+0x2c) is 0. The original loads the SLP on first use; the port
   leaves that to `bounds` (aoe_graphic_bounds_slp_ror does it). Returns 0 when the box stays degenerate (both
   x or both y bounds 0). */
int aoe_graphic_bounds_ror(const DatGraphic *graphic, int32_t angle,
                           int32_t frame, AoeGraphicFrameBoundsFn bounds,
                           void *context, int16_t *x0, int16_t *y0,
                           int16_t *x1, int16_t *y1);

/* A graphic's lazily loaded shape (graphic +0x1c: an operator new(0x20)
   shape object, NULL until first use). */
typedef struct {
    AoeShapeRor shape;
    int loaded;
} AoeGraphicShapeRor;

typedef struct {
    void *context;
    /* The graphic's shape slot (its +0x1c); NULL means no slot. */
    AoeGraphicShapeRor *(*slot)(void *context, const DatGraphic *graphic);
    const AoeShapeOpsRor *shape_ops;
    uint32_t (*now)(void *context);                    /* timeGetTime */
    void (*record)(void *context, int32_t slot, uint32_t ms); /* 0x00417370 */
} AoeGraphicShapeOpsRor;

/* The load step shared by 0x004a5c60 / 0x004a5d60 / 0x004a6150: with no
   shape yet, a graphic whose slp_loadable byte (+0x14) is clear has none
   (0); otherwise "<filename>.shp" (0x00450e20) is loaded through
   0x0049f5f0 with the graphic's SLP ID, the load time recorded in
   statistic 11. Returns 1 when the graphic has a shape object. */
int aoe_graphic_shape_ensure_ror(const DatGraphic *graphic,
                                 const AoeGraphicShapeOpsRor *ops,
                                 AoeShapeRor **out);

/* Patched RoR 1.0a 0x004a5c60: load the shape on first use, then frame
   `slp_frame`'s box relative to the hotspot (0x0049fac0). 0 when the
   graphic has no shape or the frame is unusable. Its signature matches
   AoeGraphicFrameBoundsFn with `ops` (AoeGraphicShapeOpsRor *) as
   context. */
int aoe_graphic_frame_bounds_slp_ror(void *ops, const DatGraphic *graphic,
                                     uint32_t slp_frame, int32_t *x0,
                                     int32_t *y0, int32_t *x1, int32_t *y1);

/* Patched RoR 1.0a 0x004a6150: the graphic's own selection test at local
   point (x, y). Loads the shape on first use (none: miss); a frame past
   frame_count (strictly greater) becomes 0; the stored frame and mirror
   come from 0x004a5520. Selection mode 1 tests the SLP pixel
   (0x0049fb90) at (x, y), x negated when mirrored; mode 2 tests the
   frame box (0x0049fac0) inclusively, mirrored about x when mirrored;
   other modes miss. Matches AoeGraphicPixelTestFn with `ops`
   (AoeGraphicShapeOpsRor *) as context. */
int aoe_graphic_pixel_test_slp_ror(void *ops, const DatGraphic *graphic,
                                   int32_t x, int32_t y, int32_t angle,
                                   int32_t frame);

/* Patched RoR 1.0a 0x004a5d60 with its SLP steps: the owning graphic's
   shape is loaded first (no shape: 0, even with a coordinate box), then
   aoe_graphic_bounds_ror over aoe_graphic_frame_bounds_slp_ror. */
int aoe_graphic_bounds_slp_ror(const DatGraphic *graphic, int32_t angle,
                               int32_t frame,
                               const AoeGraphicShapeOpsRor *ops,
                               int16_t *x0, int16_t *y0, int16_t *x1,
                               int16_t *y1);

#endif
