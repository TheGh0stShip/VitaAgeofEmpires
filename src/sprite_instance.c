#include "sprite_instance.h"

#include <stdlib.h>

#include <limits.h>
#include <math.h>

int aoe_sprite_instance_init(AoeSpriteInstance *instance,
                             const DatGraphic *graphic, AoeRng *rng,
                             int32_t offset_x, int32_t offset_y) {
    if (!instance || !graphic || !graphic->present)
        return 0;
    AoeSpriteInstance next = {0};
    next.graphic = graphic;
    next.offset_x = offset_x;
    next.offset_y = offset_y;
    next.kind = (graphic->sequence_type & 1) ? 2 : 1;
    if (next.kind == 2) {
        if (graphic->frame_count == 0 || graphic->frame_count > INT16_MAX ||
            !isfinite(graphic->frame_duration) ||
            !isfinite(graphic->replay_delay))
            return 0;
        next.animation.period = graphic->frame_duration;
        if (next.animation.period < 0.001f)
            next.animation.period = 0.001f;
        if (graphic->sequence_type & 4) {
            if (!rng)
                return 0;
            if (graphic->frame_duration == 0.0f &&
                graphic->frame_count == 1 &&
                graphic->replay_delay == 0.0f) {
                /* The original still draws RNG, then divides 0 by 0 for
                   this real DAT edge case (base graphic 834). A stationary
                   single frame is the defined portable outcome. */
                (void)aoe_rng_next(rng);
                next.animation.run_state = 1;
                *instance = next;
                return 1;
            }
            if (graphic->frame_duration <= 0.0f)
                return 0;
            float span = (float)graphic->frame_count *
                         graphic->frame_duration + graphic->replay_delay;
            float random_time = (float)aoe_rng_next(rng) * span *
                                (1.0f / 32768.0f);
            float frame_number = random_time / graphic->frame_duration;
            if (!isfinite(frame_number) || frame_number > INT16_MAX ||
                frame_number < INT16_MIN)
                return 0;
            next.animation.frame = (int16_t)frame_number;
            if (next.animation.frame >= graphic->frame_count) {
                next.animation.frame = (int16_t)(graphic->frame_count - 1);
                next.animation.period = random_time -
                    (float)next.animation.frame * graphic->frame_duration;
            }
            next.animation.previous_frame = next.animation.frame;
        }
        next.animation.run_state = 1;
    }
    *instance = next;
    return 1;
}

int aoe_sprite_instance_tick(AoeSpriteInstance *instance,
                             float time_delta, float speed) {
    if (!instance || !instance->graphic)
        return 0;
    if (instance->kind == 1)
        return 1;
    if (instance->kind != 2)
        return 0;
    AoeSpriteAnimationGraphic graphic = {
        .frame_count = (int16_t)instance->graphic->frame_count,
        .speed_multiplier = instance->graphic->speed_multiplier,
        .frame_duration = instance->graphic->frame_duration,
        .replay_delay = instance->graphic->replay_delay,
        .sequence_flags = instance->graphic->sequence_type,
    };
    return aoe_sprite_animation_tick(&instance->animation, &graphic,
                                     time_delta, speed);
}

int aoe_sprite_instance_frame(const AoeSpriteInstance *instance,
                              uint16_t angle, AoeGraphicFrame *out) {
    if (!instance || !instance->graphic ||
        (instance->kind != 1 && instance->kind != 2))
        return 0;
    int16_t frame = instance->kind == 2 ? instance->animation.frame : 0;
    return aoe_graphic_select_frame(instance->graphic, angle, frame, out);
}

int aoe_sprite_instance_reached_ror(const AoeSpriteInstance *instance,
                                    int16_t frame) {
    const AoeSpriteAnimationState *a;
    if (!instance || instance->kind != 2) return 0;
    a = &instance->animation;
    if (!a->frame_changed) return 0;
    if (a->looped) return 1;
    return a->frame >= frame && a->previous_frame < frame;
}

int aoe_sprite_instance_looped_ror(const AoeSpriteInstance *instance) {
    return instance && instance->kind == 2 && instance->animation.looped;
}

void aoe_sprite_instance_set_offset_ror(AoeSpriteInstance *instance,
                                        int32_t x, int32_t y) {
    if (!instance) return;
    instance->offset_x = x;
    instance->offset_y = y;
}

int16_t aoe_sprite_instance_current_frame_ror(const AoeSpriteInstance *instance) {
    return instance && instance->kind == 2 ? instance->animation.frame : 0;
}

int aoe_sprite_instance_playing_ror(const AoeSpriteInstance *instance) {
    return instance && instance->kind == 2 && instance->graphic &&
           instance->animation.run_state &&
           (instance->graphic->sequence_type & 1);
}

void aoe_sprite_instance_set_frame_ror(AoeSpriteInstance *instance,
                                       int16_t frame) {
    if (instance && instance->kind == 2) instance->animation.frame = frame;
}

void aoe_sprite_instance_restart_ror(AoeSpriteInstance *instance) {
    if (!instance || instance->kind != 2) return;
    instance->animation.frame_changed = 1;
    instance->animation.previous_frame = instance->animation.frame;
    instance->animation.frame = 0;
}

AoeSpriteInstance *aoe_sprite_instance_delete_ror(AoeSpriteInstance *instance, uint8_t flags) {
    if (instance && (flags & 1)) free(instance);
    return instance;
}
