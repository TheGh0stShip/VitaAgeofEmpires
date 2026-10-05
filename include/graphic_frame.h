#ifndef VITA_AOE_GRAPHIC_FRAME_H
#define VITA_AOE_GRAPHIC_FRAME_H

#include <stdint.h>

#include "dat.h"

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

#endif
