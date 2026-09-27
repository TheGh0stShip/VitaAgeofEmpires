#include "world_clock.h"

#include <limits.h>
#include <math.h>

int aoe_world_clock_step(AoeWorldClock *clock, int32_t gate_result,
                         uint32_t now_ms, AoeWorldClockResult *result) {
    AoeWorldClock next;
    AoeWorldClockResult outcome = {0};
    uint32_t input_ms;
    int32_t scaled_ms = 0;
    if (!clock || !result || gate_result < -1 || !isfinite(clock->speed))
        return 0;
    next = *clock;
    if (gate_result == 0) {
        next.delta_game_ms = 0;
        *clock = next;
        *result = outcome;
        return 1;
    }
    outcome.gate_accepted = 1;
    if (gate_result == -1) {
        outcome.first_local_frame = (uint8_t)(next.last_wall_ms == 0);
        input_ms = next.last_wall_ms ? now_ms - next.last_wall_ms : 0;
        if (input_ms > 100) input_ms = 100;
    } else {
        input_ms = (uint32_t)gate_result;
    }
    outcome.input_delta_ms = input_ms;
    next.last_wall_ms = now_ms;
    next.previous_game_time_ms = next.game_time_ms;
    /* Both executables convert unsigned milliseconds with fildll, multiply
       by the single-precision speed, and truncate through __ftol. */
    if (!next.pause_once || gate_result != -1) {
        long double scaled = (long double)input_ms * (long double)next.speed;
        if (scaled < INT32_MIN || scaled > INT32_MAX) return 0;
        scaled_ms = (int32_t)scaled;
        next.game_time_ms += (uint32_t)scaled_ms;
    }
    next.pause_once = 0;
    next.delta_game_ms = next.game_time_ms - next.previous_game_time_ms;
    outcome.run_simulation = (uint8_t)(next.delta_game_ms != 0 ||
                                        outcome.first_local_frame);
    if (outcome.run_simulation)
        next.delta_seconds = (float)next.delta_game_ms * 0.001f;
    *clock = next;
    *result = outcome;
    return 1;
}
