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

/* Animated sprite vtable 0x00543078 +0x28 (0x00416180) / +0x2c
   (0x004161b0), reached through the sprite manager's +0x20/+0x24
   (0x004158f0 / 0x00415920) for the unit's current graphic. Reached: the
   frame changed this tick and either the animation looped or
   previous < frame <= current. Done: the looped byte. Static sprites
   (vtable 0x00543030) answer 0 to both. */
int aoe_sprite_instance_reached_ror(const AoeSpriteInstance *instance,
                                    int16_t frame);
int aoe_sprite_instance_looped_ror(const AoeSpriteInstance *instance);

/* Frame selected for drawing at the supplied graphic angle. */
int aoe_sprite_instance_frame(const AoeSpriteInstance *instance,
                              uint16_t angle, AoeGraphicFrame *out);

#endif
