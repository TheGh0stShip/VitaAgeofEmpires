#ifndef VITA_AOE_TIMER_ACTIONS_H
#define VITA_AOE_TIMER_ACTIONS_H

#include "aoe_rng.h"

#include <stddef.h>
#include <stdint.h>

/* Original 36-byte record fields: target +0x18, status +0x1c,
   last timestamp +0x20. The portable form omits unrelated record fields. */
typedef struct {
    void *target;
    uint8_t status;
    uint32_t last_ms;
} AoeTimedEvent;

/* RoR 0x004a3a30 / base 0x0042b6a0 object fields: signed count +2,
   records pointer +4, unsigned threshold +8. */
typedef struct {
    int16_t count;
    AoeTimedEvent *records;
    size_t capacity;
    uint32_t threshold_ms;
} AoeTimedEventTimer;

typedef struct {
    void *user;
    /* RoR 0x004a49e0 / base 0x0044d370; use audio_target binding. */
    int (*expire_target)(void *user, void *target);
} AoeTimedEventOps;

/* Strict unsigned elapsed comparison; sets status 1 -> 2 after callback. */
int aoe_timed_event_timer_step(AoeTimedEventTimer *timer, uint32_t now_ms,
                               const AoeTimedEventOps *ops);

/* RoR 0x004a7c00 / base 0x004b3e30 fields: active +0x14, handle +0x1c,
   previous time +0x20, signed elapsed accumulator +0x24. */
typedef struct {
    uint8_t active;
    void *handle;
    uint32_t previous_ms;
    uint32_t accumulated_ms;
} AoeLifetimeTimer;

typedef struct {
    void *user;
    /* Original cleanup and free, in that order, on the cached handle. */
    void (*cleanup_handle)(void *user, void *handle);
    void (*free_handle)(void *user, void *handle);
} AoeLifetimeTimerOps;

/* Initializes accumulator to 1 on first active tick. Afterwards the PC
   routines compare signed 32-bit accumulated time against 180000. */
int aoe_lifetime_timer_step(AoeLifetimeTimer *timer, uint32_t now_ms,
                            const AoeLifetimeTimerOps *ops);

/* Bind the translated action bodies to AoeWorldTimerOps.action_a/b.
   Selected objects must be AoeTimedEventTimer / AoeLifetimeTimer. */
typedef struct {
    AoeTimedEventOps events;
    AoeLifetimeTimerOps lifetime;
} AoeOriginalTimerActions;

int aoe_original_timer_action_a(void *user, void *selected,
                                uint32_t now_ms, AoeRng *rng);
int aoe_original_timer_action_b(void *user, void *selected,
                                uint32_t now_ms, AoeRng *rng);

#endif
