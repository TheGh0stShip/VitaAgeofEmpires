#include "sprite_animation.h"

#include <assert.h>

static void test_loop_and_step_count(void) {
    AoeSpriteAnimationGraphic graphic = {4, 0.0f, 0.5f, 0.0f, 1};
    AoeSpriteAnimationState state = {.period = 0.5f, .run_state = 1};
    assert(aoe_sprite_animation_tick(&state, &graphic, 0.75f, 1.0f));
    assert(state.frame == 1 && state.previous_frame == 0);
    assert(state.elapsed == 0.25f && state.period == 0.5f);
    assert(state.frame_changed == 1 && state.looped == 0);
    assert(aoe_sprite_animation_tick(&state, &graphic, 0.125f, 1.0f));
    assert(state.frame == 1 && state.frame_changed == 0);
    assert(state.elapsed == 0.375f);
    assert(aoe_sprite_animation_tick(&state, &graphic, 1.125f, 1.0f));
    assert(state.previous_frame == 1 && state.frame == 0);
    assert(state.frame_changed == 1 && state.looped == 1);
    assert(state.run_state == 1 && state.elapsed == 0.0f);
}

static void test_replay_delay_and_one_shot(void) {
    AoeSpriteAnimationGraphic graphic = {4, 0.0f, 0.5f, 1.5f, 9};
    AoeSpriteAnimationState state = {
        .frame = 3, .period = 0.5f, .run_state = 1,
    };
    assert(aoe_sprite_animation_tick(&state, &graphic, 0.5f, 1.0f));
    assert(state.frame == 3 && state.run_state == 2);
    assert(state.period == 1.5f && state.looped == 0);
    assert(state.frame_changed == 1 && state.previous_frame == 3);
    assert(aoe_sprite_animation_tick(&state, &graphic, 1.5f, 1.0f));
    assert(state.run_state == 0 && state.frame == 3);
    assert(state.looped == 1 && state.frame_changed == 1);
    assert(aoe_sprite_animation_tick(&state, &graphic, 1.0f, 1.0f));
    assert(state.run_state == 0 && state.frame == 3);
    assert(state.frame_changed == 0 && state.looped == 0);
}

static void test_speed_and_inactive(void) {
    AoeSpriteAnimationGraphic graphic = {5, 2.0f, 0.5f, 0.0f, 1};
    AoeSpriteAnimationState state = {.period = 0.5f, .run_state = 1};
    assert(aoe_sprite_animation_tick(&state, &graphic, 0.5f, 1.0f));
    assert(state.frame == 1 && state.period == 1.0f);
    assert(aoe_sprite_animation_tick(&state, &graphic, 1.0f, 0.0f));
    assert(state.frame == 1 && state.period == 0.5f);
    assert(state.frame_changed == 1);
    graphic.sequence_flags = 0;
    state.frame = 99;
    assert(aoe_sprite_animation_tick(&state, &graphic, 1.0f, 1.0f));
    assert(state.frame == 4 && state.frame_changed == 0);
    graphic.frame_duration = 0.0f;
    graphic.reference_speed = 0.0f;
    graphic.sequence_flags = 1;
    assert(aoe_sprite_animation_tick(&state, &graphic, 0.5f, 1.0f));
    assert(state.period == 0.001f);
}

static void test_replay_overshoot(void) {
    AoeSpriteAnimationGraphic graphic = {3, 0.0f, 0.5f, 0.25f, 1};
    AoeSpriteAnimationState state = {.frame = 2, .period = 0.5f,
                                     .run_state = 1};
    assert(aoe_sprite_animation_tick(&state, &graphic, 1.5f, 1.0f));
    assert(state.run_state == 2 && state.frame == 2);
    assert(state.period == -0.75f); /* Original does not clamp delay. */
    assert(aoe_sprite_animation_tick(&state, &graphic, 0.0f, 1.0f));
    assert(state.period == 0.5f && state.run_state == 2);
}

int main(void) {
    test_loop_and_step_count();
    test_replay_delay_and_one_shot();
    test_speed_and_inactive();
    test_replay_overshoot();
    return 0;
}
