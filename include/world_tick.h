#ifndef VITA_AOE_WORLD_TICK_H
#define VITA_AOE_WORLD_TICK_H

#include "aoe_rng.h"
#include "command_drain.h"
#include "player_update.h"
#include "scenario_timeline.h"
#include "world_clock.h"
#include "world_timers.h"

#include <stdint.h>

#define AOE_WORLD_MAX_PLAYERS 9

typedef enum {
    AOE_WORLD_BASE = 0,
    AOE_WORLD_RISE_OF_ROME = 1
} AoeWorldEdition;

/* Portable state corresponding to fields touched by original World::update.
   Live object methods, registry population, and finalization remain
   unported. */
typedef struct {
    AoeWorldClock clock;
    AoeWorldTimers timers;
    AoeRng rng;
    AoeScenarioTimeline timeline;
    /* Reference the live registry view; owned-table growth can move arrays. */
    const AoeVisibilityRegistry *registry;
    uint32_t saved_seed;
    AoePlayerUpdateState *players[AOE_WORLD_MAX_PLAYERS];
    uint32_t player_elapsed_ms[AOE_WORLD_MAX_PLAYERS];
    int16_t player_count;
    int16_t next_elapsed_reset;
    uint32_t update_serial;
} AoeWorldTickState;

/* Required original-game dependencies. No default/diagnostic player logic
   is supplied. These hooks must be bound to real ported routines before use
   in a game; unit tests verify only the translated behaviors they cover. */
typedef struct {
    void *user;
    AoeWorldTimerOps timers;
    AoeCommandDrainOps commands;
    AoeScenarioTimelineOps timeline;
    AoePlayerUpdateOps player;
    AoeVisibilityWatchOps visibility;
    void (*cooperative_pump)(void *, AoeRng *); /* RoR, when pending. */
    void (*world_finalize)(void *, AoeRng *);
    void (*seed_sync)(void *, uint32_t);
} AoeWorldTickOps;

/* Core ordering from RoR 0x00520380 and base 0x00503390, starting after
   their external command/network gate and omitting optional AI log/game-over
   reporting. Preflight hook/bounds failures leave state unchanged; a later
   failure inside a translated dependency may leave a partial tick.
   gate_result uses world_clock.h semantics. */
int aoe_world_tick_core(AoeWorldTickState *state, AoeWorldEdition edition,
                        int32_t gate_result, uint32_t now_ms,
                        int cooperative_pending, const AoeWorldTickOps *ops,
                        AoeWorldClockResult *result);

#endif
