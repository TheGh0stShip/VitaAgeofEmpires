#include "world_timers.h"

int aoe_world_timers_valid(const AoeWorldTimers *timers) {
    int32_t selected_a, selected_b;
    if (!timers || !timers->slots_a || !timers->slots_a_capacity ||
        !timers->slots_b || !timers->slots_b_capacity) return 0;
    selected_a = timers->index_a;
    selected_b = timers->index_b;
    if ((int32_t)timers->count_a <= selected_a) selected_a = 0;
    if ((int32_t)timers->count_b <= selected_b) selected_b = 0;
    return selected_a >= 0 && (size_t)selected_a < timers->slots_a_capacity &&
           timers->slots_a[selected_a] &&
           selected_b >= 0 && (size_t)selected_b < timers->slots_b_capacity;
}

int aoe_world_timers_step(AoeWorldTimers *timers, uint32_t now_ms,
                          AoeRng *rng, const AoeWorldTimerOps *ops) {
    if (!rng || !ops || !ops->action_a || !ops->action_b ||
        !aoe_world_timers_valid(timers)) return 0;

    if ((int32_t)timers->count_a <= timers->index_a)
        timers->index_a = 0;
    if (!ops->action_a(ops->user, timers->slots_a[timers->index_a], now_ms, rng))
        return 0;
    timers->index_a++;

    if ((int32_t)timers->count_b <= timers->index_b)
        timers->index_b = 0;
    /* Timer A's action may have changed the second array or index. */
    if (!timers->slots_b || timers->index_b < 0 ||
        (size_t)timers->index_b >= timers->slots_b_capacity) return 0;
    if (timers->slots_b[timers->index_b] &&
        !ops->action_b(ops->user, timers->slots_b[timers->index_b], now_ms, rng))
        return 0;
    timers->index_b++;
    return 1;
}
