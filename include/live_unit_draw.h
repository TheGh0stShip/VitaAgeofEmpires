#ifndef VITA_AOE_LIVE_UNIT_DRAW_H
#define VITA_AOE_LIVE_UNIT_DRAW_H

#include "live_unit_core.h"
#include "live_unit_placement.h"
#include "sprite_runtime.h"

/* Inputs not yet owned by AoeLiveUnitCore correspond to original live-unit
   +0x18 (sprite manager), +0x8c (type-25 controller), +0xa4 (type-25 mask),
   owner color, template +0xa8, and active screen fields. renderer and
   tile_screen_x/y are the first three arguments forwarded by tile traversal
   0x00451fd0 to this object's vtable +0x0c. */
typedef struct {
    const AoeLiveUnitCore *unit;
    const AoeLiveUnitPlacement *placement;
    const AoeSpriteRuntime *sprites;
    const void *graphic; /* final argument selected by tile traversal */
    const AoeLiveUnitCore *type25_controller;
    void *renderer;
    int32_t tile_screen_x;
    int32_t tile_screen_y;
    uint32_t type25_hidden_players;
    uint8_t active_player;
    uint8_t owner_color;
    uint8_t template_draw_flag;
    uint8_t screen_mode;
    uint8_t screen_submode;
} AoeLiveUnitDrawRorInput;

typedef struct {
    void *context;
    /* Original disc 0x004a8d50 (patched 0x004a7520) receives the unchanged renderer/tile anchor and
       precedes sprite-manager vtable +0x0c. */
    void (*overlay)(void *context, void *renderer,
                    const AoeLiveUnitCore *unit,
                    int32_t tile_screen_x, int32_t tile_screen_y);
    /* frame is live byte +0x35. Bounds are the low-word sums of tile anchor
       and signed unit +0x28, +0x2a, +0x2c, +0x2e, exactly as recovered from
       RoR 0x004a8a60. */
    int (*sprite)(void *context, void *renderer,
                  const AoeLiveUnitCore *unit,
                  const AoeSpriteRuntime *sprites, const void *graphic,
                  uint8_t owner_color, uint8_t frame,
                  int32_t draw_id, int16_t left, int16_t top,
                  int16_t right, int16_t bottom);
} AoeLiveUnitDrawRorOps;

/* RoR live-unit vtable +0x0c = 0x004a8a60 in all nine recovered classes
   (patched 1.0a 0x004a7260, the same code).
   Returns 1 for a completed dispatch (including the original no-draw cases),
   0 if required rendering methods are unavailable. Coordinate additions use
   the original low-word wrap followed by signed-16 interpretation. */
int aoe_live_unit_draw_ror(const AoeLiveUnitDrawRorInput *input,
                            const AoeLiveUnitDrawRorOps *ops,
                            int *did_draw);

/* Patched RoR 1.0a live-unit draw slots (vtable entries 0x00542758,
   0x0054275c, 0x00542760 of the base class). Both sprite slots draw nothing
   unless the unit is placed (+0x1c) and its state (+0x48) is below 7; the
   colour argument is the owner's +0x10c. */
typedef struct {
    void *context;
    /* this object's vtable +0x1c / +0x18, run around the outline draw */
    void (*before)(void *context, void *renderer, int32_t x, int32_t y);
    void (*after)(void *context, void *renderer, int32_t x, int32_t y);
    /* sprite manager (+0x18) vtable +0x10 (outline) / +0x14 (shadow);
       `extra` is only passed by the shadow draw */
    void (*outline)(void *context, uint8_t frame, int16_t x, int16_t y,
                    uint32_t color, void *renderer);
    void (*shadow)(void *context, uint8_t frame, int16_t x, int16_t y,
                   uint32_t color, void *renderer, int32_t extra);
    /* draw-frame variants: own vtable +0x20, the GDI box frame, the full
       frame (both untranslated callees) */
    void (*frame_normal)(void *context, void *renderer, int32_t x, int32_t y);
    void (*frame_box)(void *context, void *renderer, int32_t x, int32_t y);
    void (*frame_full)(void *context, void *renderer, int32_t x, int32_t y);
} AoeLiveUnitDrawSlotOps;

/* 0x004a7370: sprite-manager shadow draw at tile anchor + (+0x2c, +0x2e).
   Returns 1 if the draw was issued. */
int aoe_live_unit_draw_shadow_ror(const AoeLiveUnitCore *unit, int placed,
                                  uint32_t owner_color, void *renderer,
                                  int32_t x, int32_t y, int32_t extra,
                                  const AoeLiveUnitDrawSlotOps *ops);
/* 0x004a73c0: outline draw at anchor + (+0x28, +0x2a); when the unit's
   +0x36 flag is set or the game screen mode (+0x50) is 3 it is bracketed
   by the before/after slots. Returns 1 if the draw was issued. */
int aoe_live_unit_draw_outline_ror(const AoeLiveUnitCore *unit, int placed,
                                   uint32_t owner_color, uint8_t screen_mode,
                                   void *renderer, int32_t x, int32_t y,
                                   const AoeLiveUnitDrawSlotOps *ops);
/* 0x004a7450: draw-frame dispatch. A game +0x1b4 of 1 or screen mode 1
   uses the full frame; mode 0 the class's own +0x20 slot;
   modes 2 and 3 the box frame; other modes draw nothing.
   Returns which: 0 none, 1 normal, 2 box, 3 full. */
int aoe_live_unit_draw_frame_dispatch_ror(int32_t game_full_frame,
                                          uint8_t screen_mode, void *renderer,
                                          int32_t x, int32_t y,
                                          const AoeLiveUnitDrawSlotOps *ops);

typedef struct {
    void *context;
    /* sprite manager (+0x18) vtable +0x28(graphic, frame) before asking */
    void (*prepare)(void *context, const void *graphic, uint8_t frame);
    /* sprite list 0x00415980: the graphic's first-sprite bounds; 0 none */
    int (*bounds)(void *context, const void *graphic, uint8_t frame,
                  int16_t *x0, int16_t *y0, int16_t *x1, int16_t *y1);
    /* renderer 0x00439ad0 (outline) / 0x00439c90 (filled), 8-bit colour */
    void (*rect)(void *context, void *renderer, int32_t x0, int32_t y0,
                 int32_t x1, int32_t y1, uint8_t color);
    void (*fill)(void *context, void *renderer, int32_t x0, int32_t y0,
                 int32_t x1, int32_t y1, uint8_t color);
    /* 16-bit path: 0x00516510 on the view's draw rows (0x007c0460) with
       the id 0x007c0444 (aoe_draw_rows_add_ror(kind, row, coords, depth,
       field_1c, id)). */
    void (*add_node)(void *context, int32_t kind, int32_t row,
                     const int16_t coords[8], int32_t depth,
                     int32_t field_1c);
} AoeLiveUnitSelectionOps;

/* 0x004aa8e0: the sprite bounds of the unit's current graphic (+0x10) at
   its frame (+0x35); 0 without a graphic, sprite manager or sprite. */
int aoe_live_unit_sprite_bounds_ror(const void *graphic, uint8_t frame,
                                    const AoeLiveUnitSelectionOps *ops,
                                    int16_t *x0, int16_t *y0, int16_t *x1,
                                    int16_t *y1);
/* 0x004a8e00 (8-bit selection box, vtable entry 0x00542768): an 0xff
   outline one pixel outside the sprite bounds shifted by the anchor and
   (+0x28, +0x2a); for state <= 2 with template HP > 0 a 26 x 5 health bar
   above it: 0xff frame, 0x55 background and a 0x25 fill of
   max(ftol(hp), 0) * 25 / max_hp pixels. Returns 1 if drawn. */
int aoe_live_unit_draw_selection_ror(const AoeLiveUnitCore *unit,
                                     const void *graphic, void *renderer,
                                     int32_t x, int32_t y,
                                     const AoeLiveUnitSelectionOps *ops);

/* 0x004a75c0 (16-bit variant of 0x004a8e00; its first argument unused):
   the same box queued as draw-row nodes. Outline (left, top, right,
   bottom) one pixel outside the shifted bounds: kind 4, row = bottom,
   coords {l, t, r, b, 0, 0, 0, 0}, depth 10, 0xff. For state <= 2 with
   template HP > 0 the bar: kind 2, row = bottom, coords {l, t - 6,
   l + 26, t - 1, f, 0, f + 1, 0} with f = max(ftol(hp), 0) * 25 / max_hp
   + l, at most l + 25; depth 0x14, 0xff. Returns 1 if queued. */
int aoe_live_unit_queue_selection_ror(const AoeLiveUnitCore *unit,
                                      const void *graphic, int32_t x,
                                      int32_t y,
                                      const AoeLiveUnitSelectionOps *ops);

/* 0x004ab0c0 (live-unit slot; the main view's box test): whether the
   unit's sprite box (bounds + (x, y) + (+0x28, +0x2a), 16-bit) and the
   query rectangle (l, t)-(r, b) touch. True when either box corner of
   one lies inside the other (box top-left / bottom-right inside the
   query, query top-left / bottom-right inside the box, all inclusive),
   or one strictly spans the other on an axis with an edge strictly
   inside on the other axis. 0 without a graphic or bounds. */
int aoe_live_unit_box_touches_ror(const AoeLiveUnitCore *unit,
                                  const void *graphic, int32_t x, int32_t y,
                                  int16_t l, int16_t t, int16_t r, int16_t b,
                                  const AoeLiveUnitSelectionOps *ops);

/* The GDI "3D square" frame halves of a unit (patched RoR 1.0a). */
typedef struct {
    const AoeLiveUnitCore *unit;  /* +0x04 id, +0x28/+0x2a, +0x30 HP, template */
    float outline_x;              /* template +0xac */
    float outline_y;              /* template +0xb0 */
    int32_t unit_78;              /* unit +0x78, compared with its +0x04 */
    int32_t logging_on;           /* global 0x00580dd0 */
} AoeLiveUnitFrame3dInput;

typedef struct {
    void *context;
    /* the game's (0x00580e38) vtable +0x2c view; 0 when none */
    int (*view)(void *context);
    /* renderer 0x004387c0 unlock / 0x00438730(name, 1) relock, each with
       the caller's name */
    void (*unlock)(void *context, void *renderer, const char *name);
    void (*lock)(void *context, void *renderer, const char *name);
    /* renderer 0x00438800 device context (0: none) / 0x00438850 release */
    int (*get_dc)(void *context, void *renderer, const char *name);
    void (*release_dc)(void *context, void *renderer, const char *name);
    /* clip to the view's +0x88 region and select stock pen 7 (black) or
       6 (white); `end` clears the clip */
    void (*begin)(void *context, void *renderer, int black);
    void (*line)(void *context, void *renderer, int32_t x0, int32_t y0,
                 int32_t x1, int32_t y1);
    void (*end)(void *context, void *renderer);
    /* 0x00448cc0: tile offset (x, y, z) to screen, around base */
    void (*project)(void *context, float x, float y, float z, int32_t base_x,
                    int32_t base_y, int16_t *out_x, int16_t *out_y);
    /* renderer 0x00439c90 filled box */
    void (*fill)(void *context, void *renderer, int32_t x0, int32_t y0,
                 int32_t x1, int32_t y1, uint8_t color);
} AoeLiveUnitFrame3dOps;

/* 0x004a7c60 "stat_obj::draw_frame_3d_square_back": with a view, unlock
   the renderer and, with a device context, draw (in the black pen when
   logging is on and the unit's +0x78 equals its non-negative ID, else
   white) the back edges at the anchor (x, y) + (+0x28, +0x2a): (ox, -oy)
   to (ox, oy) and (ox, -oy) to (-ox, -oy), o the template's outline
   size; then relock. Returns 0 without a view. */
int aoe_live_unit_draw_frame_back_ror(const AoeLiveUnitFrame3dInput *in,
                                      void *renderer, int32_t x, int32_t y,
                                      const AoeLiveUnitFrame3dOps *ops);
/* 0x004a7e40 "stat_obj::draw_frame_3d_square_front": the same for the
   front edges (-ox, -oy) to (-ox, oy) and (ox, oy) to (-ox, oy); after
   relocking, a unit with ftol(HP) > 0 and template HP > 0 gets a bar at
   the projected (ox, -oy, template height size[2]): 0x97 from x - 12 to
   x + 12 over rows y - 2..y - 1, and a 0x4a fill to x - 12 +
   hp * 24 / max_hp. Returns 0 without a view. */
int aoe_live_unit_draw_frame_front_ror(const AoeLiveUnitFrame3dInput *in,
                                       void *renderer, int32_t x, int32_t y,
                                       const AoeLiveUnitFrame3dOps *ops);

#endif
