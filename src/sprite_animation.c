#include "sprite_animation.h"

#include <limits.h>
#include <math.h>

int aoe_sprite_animation_tick(AoeSpriteAnimationState *state,
                              const AoeSpriteAnimationGraphic *graphic,
                              float time_delta, float speed) {
    if (!state || !graphic || graphic->frame_count <= 0 ||
        !isfinite(time_delta) || !isfinite(speed) ||
        !isfinite(state->period) || !isfinite(state->elapsed) ||
        state->period == 0.0f)
        return 0;

    AoeSpriteAnimationState next = *state;
    next.frame_changed = 0;
    next.looped = 0;
    if (!next.run_state) {
        *state = next;
        return 1;
    }
    if (next.frame >= graphic->frame_count)
        next.frame = (int16_t)(graphic->frame_count - 1);
    if (!(graphic->sequence_flags & 1)) {
        *state = next;
        return 1;
    }

    next.elapsed += time_delta;
    if (next.elapsed < next.period) {
        *state = next;
        return 1;
    }

    next.frame_changed = 1;
    next.previous_frame = next.frame;
    float quotient = next.elapsed / next.period;
    if (!isfinite(quotient) || quotient > INT16_MAX || quotient < INT16_MIN)
        return 0; /* Avoid undefined C conversion outside the original domain. */
    int16_t steps = (int16_t)quotient; /* x87 conversion truncates toward zero. */
    next.elapsed -= (float)steps * next.period;

    if (speed > 0.0f || graphic->speed_multiplier == 0.0f) {
        next.frame = (int16_t)(next.frame + (steps & 0xff));
        if (next.frame >= graphic->frame_count)
            next.looped = 1;
    }
    if (graphic->speed_multiplier == 0.0f || speed == 0.0f)
        next.period = graphic->frame_duration;
    else
        next.period = graphic->frame_duration * graphic->speed_multiplier / speed;
    if (next.period < 0.001f)
        next.period = 0.001f;

    if (next.looped) {
        if (graphic->replay_delay > 0.0f && next.run_state != 2) {
            next.period = graphic->replay_delay -
                (float)(next.frame - graphic->frame_count) *
                graphic->frame_duration;
            next.run_state = 2;
            next.looped = 0;
            next.frame = (int16_t)(graphic->frame_count - 1);
        } else {
            next.run_state = 1;
            while (next.frame >= graphic->frame_count)
                next.frame = (int16_t)(next.frame - graphic->frame_count);
        }
    }
    if (next.looped && (graphic->sequence_flags & 8)) {
        next.run_state = 0;
        next.frame = (int16_t)(graphic->frame_count - 1);
    }
    *state = next;
    return 1;
}
