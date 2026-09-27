#include "timer_actions.h"
#include "world_timers.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char events[16];
    unsigned used;
    void *targets[4];
    unsigned expiries;
    void *handle;
} Trace;

static void mark(Trace *trace, char event) {
    assert(trace->used + 1 < sizeof(trace->events));
    trace->events[trace->used++] = event;
    trace->events[trace->used] = '\0';
}

static int expire(void *user, void *target) {
    Trace *trace = user;
    trace->targets[trace->expiries++] = target;
    mark(trace, 'E');
    return 1;
}

static void cleanup(void *user, void *handle) {
    Trace *trace = user;
    assert(handle == trace->handle);
    mark(trace, 'C');
}

static void free_handle(void *user, void *handle) {
    Trace *trace = user;
    assert(handle == trace->handle);
    mark(trace, 'F');
}

static void test_timed_events(void) {
    int targets[3] = {1, 2, 3};
    AoeTimedEvent records[3] = {
        {&targets[0], 1, 100},
        {&targets[1], 1, 99},
        {&targets[2], 0, 0}
    };
    AoeTimedEventTimer timer = {3, records, 3, 100};
    Trace trace = {0};
    AoeTimedEventOps ops = {&trace, expire};
    assert(aoe_timed_event_timer_step(&timer, 200, &ops));
    assert(trace.expiries == 1 && trace.targets[0] == &targets[1]);
    assert(records[0].status == 1 && records[1].status == 2);
    assert(records[2].status == 0);
    records[0].last_ms = UINT32_MAX - 100;
    assert(aoe_timed_event_timer_step(&timer, 5, &ops));
    assert(trace.expiries == 2 && trace.targets[1] == &targets[0]);
    assert(records[0].status == 2);
    assert(strcmp(trace.events, "EE") == 0);
    timer.count = 4;
    assert(!aoe_timed_event_timer_step(&timer, 5, &ops));
    assert(trace.expiries == 2);
}

static void test_lifetime(void) {
    int handle = 1;
    AoeLifetimeTimer timer = {1, &handle, 0, 0};
    Trace trace = {0};
    AoeLifetimeTimerOps ops = {&trace, cleanup, free_handle};
    trace.handle = &handle;
    assert(aoe_lifetime_timer_step(&timer, 1000, &ops));
    assert(timer.previous_ms == 1000 && timer.accumulated_ms == 1);
    assert(trace.used == 0);
    assert(aoe_lifetime_timer_step(&timer, 180998, &ops));
    assert(timer.accumulated_ms == 179999 && trace.used == 0);
    assert(aoe_lifetime_timer_step(&timer, 180999, &ops));
    assert(timer.accumulated_ms == 180000 && timer.handle == 0);
    assert(strcmp(trace.events, "CF") == 0);
    assert(aoe_lifetime_timer_step(&timer, 181000, &ops));
    assert(strcmp(trace.events, "CF") == 0);

    timer.handle = &handle;
    timer.accumulated_ms = UINT32_MAX;
    timer.previous_ms = 2000;
    assert(aoe_lifetime_timer_step(&timer, 2000, &ops));
    assert(timer.handle == &handle); /* Signed PC comparison sees -1. */
    assert(strcmp(trace.events, "CF") == 0);
}

static void test_selector_binding(void) {
    int target = 1;
    int handle = 2;
    AoeTimedEvent record = {&target, 1, 0};
    AoeTimedEventTimer event_timer = {1, &record, 1, 100};
    AoeLifetimeTimer lifetime = {1, &handle, 199, 179999};
    void *slot_a[1] = {&event_timer};
    void *slot_b[1] = {&lifetime};
    AoeWorldTimers timers = {0};
    AoeRng rng = {1};
    Trace trace = {0};
    AoeOriginalTimerActions actions = {0};
    AoeWorldTimerOps ops = {&actions, aoe_original_timer_action_a,
                            aoe_original_timer_action_b};
    trace.handle = &handle;
    actions.events.user = &trace;
    actions.events.expire_target = expire;
    actions.lifetime.user = &trace;
    actions.lifetime.cleanup_handle = cleanup;
    actions.lifetime.free_handle = free_handle;
    timers.count_a = 1;
    timers.count_b = 1;
    timers.slots_a = slot_a;
    timers.slots_a_capacity = 1;
    timers.slots_b = slot_b;
    timers.slots_b_capacity = 1;
    assert(aoe_world_timers_step(&timers, 200, &rng, &ops));
    assert(strcmp(trace.events, "ECF") == 0);
    assert(trace.targets[0] == &target);
    assert(record.status == 2 && lifetime.handle == 0);
    assert(timers.index_a == 1 && timers.index_b == 1);
}

int main(void) {
    test_timed_events();
    test_lifetime();
    test_selector_binding();
    return 0;
}
