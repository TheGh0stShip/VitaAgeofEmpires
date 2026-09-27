#ifndef VITA_AOE_GRAPHIC_FRAME_H
#define VITA_AOE_GRAPHIC_FRAME_H

#include <stdint.h>

#include "dat.h"

typedef struct {
    uint32_t slp_frame;
    uint8_t mirror_horizontal;
} AoeGraphicFrame;

/* RoR 0x004a6d10 / base 0x004b2e60. Selects the stored SLP frame for a
   graphic angle and animation frame. The original caller bounds angles;
   this portable API rejects invalid ones and does not mutate output. */
int aoe_graphic_select_frame(const DatGraphic *graphic,
                             uint16_t angle, int16_t animation_frame,
                             AoeGraphicFrame *out);

#endif
