#ifndef VITA_AOE_GRAPHIC_EFFECTS_H
#define VITA_AOE_GRAPHIC_EFFECTS_H

#include "slp.h"

typedef struct {
    const Palette *palette;
    uint8_t player_color;
} AoeGraphicPlayerEffects;

/* RoR 0x004a6dd0 -> 0x004a1460/0x004a2570 -> 0x005459a0:
   player-color commands OR their source index with the repeated byte set by
   0x00545760, namely (player_color + 1) * 16 truncated to eight bits.
   A negative DAT graphic player-color field means the caller must supply the
   unit/player color instead. These helpers take that resolved zero-based ID. */
uint8_t aoe_graphic_player_palette_index(uint8_t source_index,
                                          uint8_t player_color);

/* True only when every pixel can be rendered by the player-color RGBA path
   without switching to an indexed destination for shadows or outlines. */
int aoe_graphic_player_only(const SlpFrame *frame);

/* Callback for slp_blit_graphic_rgba after player_only validation. */
void aoe_graphic_apply_player(void *context, const SlpPixel *pixel,
                              uint8_t rgba[4]);

#endif
