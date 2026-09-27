#include "world_timers.h"

#include <assert.h>
#include <string.h>

typedef struct {
    AoeWorldTimers *timers;
    char events[12];
    unsigned used;
    int32_t a_seen[4];
    int32_t b_seen[4];
    unsigned a_calls;
    unsigned b_calls;
} Trace;

static void mark(Trace *trace, char event) {
    assert(trace->used + 1 < sizeof(trace->events));
    trace->events[trace->used++] = event;
    trace->events[trace->used] = '\0';
}

static int action_a(void *user, void *selected, uint32_t now, AoeRng *rng) {
    Trace *trace = user;
    static const uint16_t expected_rng[] = {41, 18467, 6334};
    assert(now == 500 && rng && selected);
    assert(trace->a_calls < sizeof(expected_rng) / sizeof(expected_rng[0]));
    assert(aoe_rng_next(rng) == expected_rng[trace->a_calls]);
    trace->a_seen[trace->a_calls++] = trace->timers->index_a;
    mark(trace, 'A');
    return 1;
}

static int action_b(void *user, void *selected, uint32_t now, AoeRng *rng) {
    Trace *trace = user;
    assert(now == 500 && rng && selected);
    trace->b_seen[trace->b_calls++] = trace->timers->index_b;
    mark(trace, 'B');
    return 1;
}

int main(void) {
    int object = 1;
    void *slots_a[2] = {&object, &object};
    void *slots[2] = {&object, 0};
    AoeWorldTimers timers = {0};
    AoeRng rng = {1};
    Trace trace = {0};
    AoeWorldTimerOps ops = {&trace, action_a, action_b};
    timers.count_a = 2;
    timers.count_b = 2;
    timers.index_a = 2;
    timers.index_b = 2;
    timers.slots_a = slots_a;
    timers.slots_a_capacity = 2;
    timers.slots_b = slots;
    timers.slots_b_capacity = 2;
    trace.timers = &timers;

    assert(aoe_world_timers_step(&timers, 500, &rng, &ops));
    assert(strcmp(trace.events, "AB") == 0);
    assert(trace.a_seen[0] == 0 && trace.b_seen[0] == 0);
    assert(timers.index_a == 1 && timers.index_b == 1);

    assert(aoe_world_timers_step(&timers, 500, &rng, &ops));
    assert(strcmp(trace.events, "ABA") == 0); /* Slot 1 is null. */
    assert(trace.a_seen[1] == 1);
    assert(timers.index_a == 2 && timers.index_b == 2);

    slots[0] = 0;
    assert(aoe_world_timers_step(&timers, 500, &rng, &ops));
    assert(strcmp(trace.events, "ABAA") == 0);
    assert(timers.index_a == 1 && timers.index_b == 1);

    AoeWorldTimers saved = timers;
    timers.index_b = -1;
    assert(!aoe_world_timers_valid(&timers));
    assert(!aoe_world_timers_step(&timers, 500, &rng, &ops));
    assert(strcmp(trace.events, "ABAA") == 0);
    timers = saved;
    timers.slots_b_capacity = 1;
    assert(!aoe_world_timers_valid(&timers));
    return 0;
}
