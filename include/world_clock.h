#ifndef VITA_AOE_WORLD_CLOCK_H
#define VITA_AOE_WORLD_CLOCK_H

#include <stdint.h>

/* The shared clock portion of Rise of Rome World::update (0x00520380)
   and base Age of Empires World::update (0x00503390). Network/command
   gating and player simulation are separate original-game dependencies. */
typedef struct {
    uint32_t game_time_ms;
    uint32_t previous_game_time_ms;
    uint32_t delta_game_ms;
    float delta_seconds; /* Updated only when the original simulation pass runs. */
    uint32_t last_wall_ms;
    float speed;
    uint8_t pause_once;
} AoeWorldClock;

typedef struct {
    uint32_t input_delta_ms;
    uint8_t gate_accepted;
    uint8_t run_simulation;
    uint8_t first_local_frame;
} AoeWorldClockResult;

/* gate_result is the original command/network gate's result: zero blocks,
   -1 selects local wall-clock timing, and a positive value supplies a
   synchronized delta in milliseconds. now_ms is the 32-bit platform clock.
   Returns zero without modifying state for invalid arguments/scaling. */
int aoe_world_clock_step(AoeWorldClock *clock, int32_t gate_result,
                         uint32_t now_ms, AoeWorldClockResult *result);

#endif
