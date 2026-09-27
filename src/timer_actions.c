#include "timer_actions.h"

#include <limits.h>

int aoe_timed_event_timer_step(AoeTimedEventTimer *timer, uint32_t now_ms,
                               const AoeTimedEventOps *ops) {
    if (!timer || !ops || !ops->expire_target || timer->count < 0 ||
        (size_t)timer->count > timer->capacity ||
        (timer->count && !timer->records)) return 0;
    if (timer->count == 0 || timer->threshold_ms == 0) return 1;
    for (int i = 0; i < timer->count; ++i) {
        if (!timer->records || (size_t)i >= timer->capacity) return 0;
        AoeTimedEvent *record = &timer->records[i];
        if (record->status == 1 &&
            timer->threshold_ms < now_ms - record->last_ms) {
            if (!record->target) return 0;
            if (!ops->expire_target(ops->user, record->target)) return 0;
            /* The PC code reloads the array pointer after the callback. */
            if (!timer->records || (size_t)i >= timer->capacity) return 0;
            timer->records[i].status = 2;
        }
    }
    return 1;
}

int aoe_lifetime_timer_step(AoeLifetimeTimer *timer, uint32_t now_ms,
                            const AoeLifetimeTimerOps *ops) {
    uint32_t elapsed;
    void *handle;
    if (!timer || !ops || !ops->cleanup_handle || !ops->free_handle)
        return 0;
    if (!timer->active || !timer->handle) return 1;
    if (timer->accumulated_ms == 0) {
        timer->previous_ms = now_ms;
        timer->accumulated_ms = 1;
        return 1;
    }
    elapsed = (now_ms - timer->previous_ms) + timer->accumulated_ms;
    timer->previous_ms = now_ms;
    timer->accumulated_ms = elapsed;
    if (elapsed >= 180000 && elapsed <= INT32_MAX) {
        handle = timer->handle;
        ops->cleanup_handle(ops->user, handle);
        ops->free_handle(ops->user, handle);
        timer->handle = 0;
    }
    return 1;
}

int aoe_original_timer_action_a(void *user, void *selected,
                                uint32_t now_ms, AoeRng *rng) {
    AoeOriginalTimerActions *actions = user;
    if (!actions || !rng) return 0;
    return aoe_timed_event_timer_step(selected, now_ms, &actions->events);
}

int aoe_original_timer_action_b(void *user, void *selected,
                                uint32_t now_ms, AoeRng *rng) {
    AoeOriginalTimerActions *actions = user;
    if (!actions || !rng) return 0;
    return aoe_lifetime_timer_step(selected, now_ms, &actions->lifetime);
}
