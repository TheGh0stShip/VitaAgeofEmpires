#ifndef VITA_AOE_LIVE_UNIT_BOX_H
#define VITA_AOE_LIVE_UNIT_BOX_H

#include <stdint.h>

#include "live_unit_core.h"
#include "live_unit_draw.h"

/* The draw-list selection box of a live unit (patched RoR 1.0a), queued
   through the draw list (0x7c0460) shape add (patched 0x516510) and
   sprite add (patched 0x516390) rather than drawn on the renderer. */
typedef struct {
    const AoeLiveUnitCore *unit;  /* +0x04 id, +0x28/+0x2a, +0x30 HP, +0x36 */
    uint8_t group;                /* unit +0x37: control group 1..9 */
    /* The box half-sizes and height: template +0x30/+0x34/+0x38 in screen
       mode 3 (game +0x50), else the outline +0xac/+0xb0/+0xb4. */
    float size[3];
    uint8_t screen_mode;          /* game +0x50 */
    int32_t game_1b0;             /* game +0x1b0 */
    uint8_t template_a8;          /* bit 0: own colour, bit 1: no HP bar */
    uint8_t template_a9;          /* the own colour */
    int16_t max_hp;               /* template +0x26 */
    int32_t unit_78;              /* unit +0x78, compared with its +0x04 */
    int32_t logging_on;           /* global 0x00580dd0 */
    /* The owner's number (+0x4a) is the world's (+0x3c) viewed one (+0x7c). */
    int owner_viewed;
    int32_t show_ids;             /* game +0x9d4 */
} AoeLiveUnitBoxInput;

typedef struct {
    void *context;
    /* 0x00448cc0: tile offset (x, y, z) to screen, around base */
    void (*project)(void *context, float x, float y, float z, int32_t base_x,
                    int32_t base_y, int16_t *out_x, int16_t *out_y);
    /* draw-list shape: kind (1 box outline, 2 bar, 4 small box), layer,
       eight coordinates/arguments, priority and colour */
    void (*shape)(void *context, int32_t kind, int16_t layer,
                  const int32_t v[8], int32_t priority, uint8_t colour);
    /* draw-list sprite: frame `frame` of the number shape (game record
       0x00419a00(0)) at (x, y); 0 when that shape is missing */
    int (*number)(void *context, int32_t frame, int16_t layer, int32_t x,
                  int32_t y, int32_t priority);
} AoeLiveUnitBoxOps;

/* 0x004a76f0 colour rule: 0 when logging is on and the unit's +0x78
   equals its non-negative ID; else without flag bit 0 0x74 in screen mode
   3, or the template's own colour (+0xa9) when its +0xa8 bit 0 is set and
   game +0x1b0 is 7, else 0xff; flag bit 1 then gives 0x4a, bit 2 0x97. */
uint8_t aoe_live_unit_box_colour_ror(const AoeLiveUnitBoxInput *in);

/* 0x004a76f0: the box (kind 1, priority 10) through the projected corners
   (sx, -sy), (sx, sy), (-sx, sy), (-sx, -sy) around the anchor plus
   (+0x28, +0x2a), layered by that y; a viewed player's control group
   number (frame = group) at the last corner - (6, 8); with flag bit 0,
   no template bit 1 and ftol(HP) > 0 a 24-pixel bar (kind 2) at the
   projection of (sx, -sy, sz); with game +0x9d4 the unit ID as number
   sprites (zero digits as kind 4 boxes, a minus as a kind 4 line) 7
   pixels apart after the bar's right end (or the second corner without
   a bar). */
void aoe_live_unit_draw_box_ror(const AoeLiveUnitBoxInput *in, int32_t x,
                                int32_t y, const AoeLiveUnitBoxOps *ops);

/* 0x004a80e0: the GDI bracket corners of a unit's outline box (template
   +0xac/+0xb0/+0xb4 = ox, oy, oz). With a view: unlock the renderer and,
   with a device context, in the black pen under the logging rule of the
   3D square (else white) draw six strokes around the anchor plus (+0x28,
   +0x2a): at (ox, -oy) up from 0 to oz/4 and from 3oz/4 to oz; along x
   from (ox, -oy) to (ox/4, -oy) and from (-ox, -oy) to (-ox/4, -oy);
   along y from (ox, -oy) to (ox, -oy/4) and from (ox, oy) to (ox, oy/4);
   then relock. Returns 0 without a view. */
int aoe_live_unit_draw_brackets_ror(const AoeLiveUnitFrame3dInput *in,
                                    float outline_z, void *renderer,
                                    int32_t x, int32_t y,
                                    const AoeLiveUnitFrame3dOps *ops);

/* 0x004a84c0: the same pen, view and locking as 0x004a80e0 with the
   anchor 16 pixels higher, and eighteen strokes: at height oz the eight
   quarter-length corner ticks of the top face; at height 0 four of the
   bottom face ((-ox, -oy) and (-ox, oy) along y, (ox, oy) and (-ox, oy)
   along x); and quarter-height ticks up from 0 and down from oz at the
   (-ox, oy), (-ox, -oy) and (ox, oy) edges. */
int aoe_live_unit_draw_brackets_full_ror(const AoeLiveUnitFrame3dInput *in,
                                         float outline_z, void *renderer,
                                         int32_t x, int32_t y,
                                         const AoeLiveUnitFrame3dOps *ops);

#endif
