#ifndef VITA_AOE_WORLD_TIMERS_H
#define VITA_AOE_WORLD_TIMERS_H

#include "aoe_rng.h"

#include <stddef.h>
#include <stdint.h>

/* Semantic world fields at +0x20/+0x24 (indices), +0x2c/+0x34 (signed
   counts), and +0x30/+0x38 (selected-object arrays). Not the 32-bit ABI. */
typedef struct {
    int32_t index_a;
    int32_t index_b;
    int16_t count_a;
    int16_t count_b;
    void *const *slots_a;
    size_t slots_a_capacity;
    void *const *slots_b;
    size_t slots_b_capacity;
} AoeWorldTimers;

typedef struct {
    void *user;
    /* Original action bodies remain edition-specific and unported. */
    int (*action_a)(void *user, void *selected, uint32_t now_ms, AoeRng *rng);
    int (*action_b)(void *user, void *selected, uint32_t now_ms, AoeRng *rng);
} AoeWorldTimerOps;

/* RoR 0x00520300/0x00520340; base 0x00503320/0x00503350.
   Bounds guard both pointer arrays, which the PC code accessed unchecked.
   Timer A requires a selected object; timer B skips null entries. */
int aoe_world_timers_valid(const AoeWorldTimers *timers);
int aoe_world_timers_step(AoeWorldTimers *timers, uint32_t now_ms,
                          AoeRng *rng, const AoeWorldTimerOps *ops);

#endif
