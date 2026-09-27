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
    /* Original 0x004a8d50 receives the unchanged renderer/tile anchor and
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

/* RoR live-unit vtable +0x0c = 0x004a8a60 in all nine recovered classes.
   Returns 1 for a completed dispatch (including the original no-draw cases),
   0 if required rendering methods are unavailable. Coordinate additions use
   the original low-word wrap followed by signed-16 interpretation. */
int aoe_live_unit_draw_ror(const AoeLiveUnitDrawRorInput *input,
                            const AoeLiveUnitDrawRorOps *ops,
                            int *did_draw);

#endif
