#ifndef VITA_AOE_SPRITE_INSTANCE_H
#define VITA_AOE_SPRITE_INSTANCE_H

#include <stdint.h>

#include "aoe_rng.h"
#include "dat.h"
#include "graphic_frame.h"
#include "sprite_animation.h"

/* Portable static/animated sprite instance corresponding to the original
   factory RoR 0x004a7b10 / base 0x004b3d10. The DAT table owns graphic. */
typedef struct {
    const DatGraphic *graphic;
    int32_t offset_x, offset_y;
    uint8_t kind; /* 1 static, 2 animated */
    AoeSpriteAnimationState animation;
} AoeSpriteInstance;

/* Constructors RoR 0x00416060/0x00416300 and base
   0x004e8900/0x004e8bf0. rng is required only when sequence flag 0x04
   randomizes the starting frame. Returns zero without mutation if the
   source graphic cannot safely be instantiated. */
int aoe_sprite_instance_init(AoeSpriteInstance *instance,
                             const DatGraphic *graphic, AoeRng *rng,
                             int32_t offset_x, int32_t offset_y);

/* Static sprites do not advance (the original virtual method is a no-op). */
int aoe_sprite_instance_tick(AoeSpriteInstance *instance,
                             float time_delta, float speed);

/* Frame selected for drawing at the supplied graphic angle. */
int aoe_sprite_instance_frame(const AoeSpriteInstance *instance,
                              uint16_t angle, AoeGraphicFrame *out);

#endif
