#ifndef VITA_AOE_SPRITE_ANIMATION_H
#define VITA_AOE_SPRITE_ANIMATION_H

#include <stdint.h>

/* Runtime graphic fields used by base 0x004e8ed0 / RoR 0x00416540
   (patched RoR 1.0a 0x00415eb0, the animated sprite's +0x0c update).
   These correspond to graphic +0x5e, +0x64, +0x68, +0x6c, +0x70. */
typedef struct {
    int16_t frame_count;
    float speed_multiplier;
    float frame_duration;
    float replay_delay;
    uint8_t sequence_flags;
} AoeSpriteAnimationGraphic;

/* Semantic form of animated sprite fields +0x14 through +0x24. */
typedef struct {
    int16_t frame;
    int16_t previous_frame;
    float period;
    float elapsed;
    uint8_t frame_changed;
    uint8_t looped;
    uint8_t run_state; /* 0 stopped, 1 playing, 2 replay delay */
} AoeSpriteAnimationState;

/* Original update requires a non-null graphic and positive frame count.
   time_delta and speed are the two arguments passed by the sprite manager.
   Returns 0 without mutation for invalid inputs. */
int aoe_sprite_animation_tick(AoeSpriteAnimationState *state,
                              const AoeSpriteAnimationGraphic *graphic,
                              float time_delta, float speed);

#endif
