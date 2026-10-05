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

/* Patched RoR 1.0a accessors. Static vtable 0x00543030 / animated
   0x00543078: +0x04 0x00415ac0 sets the draw offset (both kinds); +0x1c
   frame (animated 0x00416140, static 0x00415c00: 0); +0x20 0x00416150
   playing (run state set and the sequence animates; static 0x004aa8b0:
   0); +0x24 0x00416170 sets the frame; +0x38 0x00416220 restarts (marks
   the frame changed from the current one and goes to frame 0). The
   static sprite constructor 0x00415a00 stores the owner (+0x04), +0x08 1
   and +0x0c / +0x10 0; its destructor 0x00415ab0 only resets the vtable (deleting 0x00415a20);
   the animated destructor 0x00415e30 resets its own and runs that one.
   The
   animated update +0x0c is 0x00415eb0 (aoe_sprite_animation_tick). */
void aoe_sprite_instance_set_offset_ror(AoeSpriteInstance *instance,
                                        int32_t x, int32_t y);
int16_t aoe_sprite_instance_current_frame_ror(const AoeSpriteInstance *instance);
int aoe_sprite_instance_playing_ror(const AoeSpriteInstance *instance);
void aoe_sprite_instance_set_frame_ror(AoeSpriteInstance *instance,
                                       int16_t frame);
void aoe_sprite_instance_restart_ror(AoeSpriteInstance *instance);

/* Frame selected for drawing at the supplied graphic angle. */
int aoe_sprite_instance_frame(const AoeSpriteInstance *instance,
                              uint16_t angle, AoeGraphicFrame *out);

#endif
