#include "world_clock.h"

#include <assert.h>
#include <stdint.h>

int main(void) {
    AoeWorldClock clock = {0};
    AoeWorldClockResult result;
    clock.speed = 1.0f;

    assert(aoe_world_clock_step(&clock, 0, 500, &result));
    assert(!result.gate_accepted && !result.run_simulation);
    assert(clock.game_time_ms == 0 && clock.last_wall_ms == 0);

    assert(aoe_world_clock_step(&clock, -1, 500, &result));
    assert(result.gate_accepted && result.first_local_frame);
    assert(result.input_delta_ms == 0 && result.run_simulation);
    assert(clock.last_wall_ms == 500 && clock.delta_game_ms == 0);
    assert(clock.delta_seconds == 0.0f);

    assert(aoe_world_clock_step(&clock, -1, 650, &result));
    assert(result.input_delta_ms == 100 && !result.first_local_frame);
    assert(clock.game_time_ms == 100 && clock.delta_game_ms == 100);
    assert(clock.delta_seconds > 0.099f && clock.delta_seconds < 0.101f);

    clock.pause_once = 1;
    assert(aoe_world_clock_step(&clock, -1, 700, &result));
    assert(!result.run_simulation && !clock.pause_once);
    assert(clock.game_time_ms == 100 && clock.last_wall_ms == 700);
    assert(clock.delta_seconds > 0.099f && clock.delta_seconds < 0.101f);

    clock.speed = 1.5f;
    assert(aoe_world_clock_step(&clock, 40, 705, &result));
    assert(result.input_delta_ms == 40 && result.run_simulation);
    assert(clock.previous_game_time_ms == 100 && clock.game_time_ms == 160);

    clock.pause_once = 1;
    assert(aoe_world_clock_step(&clock, 20, 710, &result));
    assert(clock.game_time_ms == 190 && !clock.pause_once);

    clock.speed = 1.0f;
    clock.last_wall_ms = UINT32_MAX - 4;
    assert(aoe_world_clock_step(&clock, -1, 5, &result));
    assert(result.input_delta_ms == 10 && clock.game_time_ms == 200);

    clock.speed = -1.0f;
    assert(aoe_world_clock_step(&clock, 5, 6, &result));
    assert(clock.delta_game_ms == UINT32_MAX - 4);
    clock.speed = 1.25f;
    assert(aoe_world_clock_step(&clock, 1, 7, &result));
    assert(clock.game_time_ms == 196); /* __ftol truncates 1.25 toward zero. */
    return 0;
}
