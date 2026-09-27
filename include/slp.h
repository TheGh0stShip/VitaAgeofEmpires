#ifndef VITA_AOE_SLP_H
#define VITA_AOE_SLP_H

#include <stddef.h>
#include <stdint.h>

#include "palette.h"

enum {
    SLP_TRANSPARENT = 0,
    SLP_COLOR = 1,
    SLP_PLAYER = 2,
    SLP_SHADOW = 3,
    SLP_OUTLINE1 = 4,
    SLP_OUTLINE2 = 5
};

typedef struct {
    uint8_t index;
    uint8_t kind;
} SlpPixel;

typedef struct {
    uint32_t width, height;
    int32_t hotspot_x, hotspot_y;
    SlpPixel *pixels;
} SlpFrame;

int slp_frame_count(const uint8_t *data, size_t size, uint32_t *count);
int slp_decode_frame(const uint8_t *data, size_t size, uint32_t frame_index,
                     SlpFrame *frame);
void slp_free_frame(SlpFrame *frame);

/* Composites terrain/border SLP_COLOR pixels using the original JASC palette into
   a top-left-addressed RGBA8888 buffer. Transparent pixels preserve the
   destination; player/shadow/outline pixels are rejected. Coordinates are
   the frame's top-left, before any caller-provided tile/hotspot placement. */
int slp_blit_terrain_rgba(const SlpFrame *frame, const Palette *palette,
                          uint8_t *rgba, size_t rgba_size,
                          uint32_t width, uint32_t height, size_t stride,
                          int32_t x, int32_t y);

/* Original graphic drawing placement: RoR 0x004a1460/0x004a2570.
   anchor is the sprite hotspot; horizontal mirroring changes the x origin
   to anchor_x + hotspot_x - width. Ordinary palette pixels are resolved
   here. A callback is required when the frame contains player-color,
   shadow, or outline pixels; it receives the existing destination RGBA and
   applies the original effect supplied by the caller. All arguments and
   pixel kinds are validated before any destination pixel changes. */
int slp_blit_graphic_rgba(const SlpFrame *frame, const Palette *palette,
                          uint8_t *rgba, size_t rgba_size,
                          uint32_t width, uint32_t height, size_t stride,
                          int32_t anchor_x, int32_t anchor_y,
                          int mirror_horizontal,
                          void (*apply_special)(void *context,
                                                const SlpPixel *pixel,
                                                uint8_t rgba[4]),
                          void *context);

/* Original paletted sprite destination. Ordinary pixels replace the index;
   player pixels OR in the selected 16-color bank (RoR 0x005459a0);
   shadow pixels transform the *existing* index through shadow_table
   (RoR SLP opcode 0xb). The caller must supply the original 256-byte table
   for faithful shadows (aoe_shadow_table_load resolves Shadow.col). Frames
   with outlines or a missing required shadow
   table are rejected before any destination change. */
int slp_blit_graphic_indexed(const SlpFrame *frame,
                             uint8_t *indices, size_t indices_size,
                             uint32_t width, uint32_t height, size_t stride,
                             int32_t anchor_x, int32_t anchor_y,
                             int mirror_horizontal, uint8_t player_color,
                             const uint8_t shadow_table[256]);

#endif
