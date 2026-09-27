#include "player_outcome.h"

#include <assert.h>
#include <string.h>

typedef struct {
    char order[16];
    unsigned used;
} Trace;

static void mark(Trace *trace, char event) {
    assert(trace->used + 1 < sizeof(trace->order));
    trace->order[trace->used++] = event;
    trace->order[trace->used] = '\0';
}

static int update_event(void *user, AoeOutcomeEvent *event) {
    assert(event->status != 1 && event->status != 3);
    mark(user, 'E');
    return 1;
}

static int update_condition(void *user, AoeOutcomeCondition *item) {
    assert(item->status != 1 && item->status != 3);
    mark(user, 'C');
    return 1;
}

int main(void) {
    Trace trace = {0};
    AoeOutcomeOps ops = {&trace, update_event, update_condition};
    AoeOutcomeManager manager = {0};
    uint8_t result = 0;
    AoeOutcomeEvent events[3] = {{0}};
    AoeOutcomeCondition conditions[4] = {{0}};

    manager.outcome = 7;
    assert(aoe_outcome_manager_update(&manager, &ops, &result));
    assert(result == 7); /* No conditions leaves prior result untouched. */

    events[0].kind = 4;
    events[0].status = 1;
    events[0].next = &events[1];
    events[1].kind = 2;
    events[1].status = 2;
    events[1].next = &events[2];
    events[2].kind = 1;
    events[2].status = 3;
    manager.events = events;
    conditions[0].group = 4;
    conditions[0].kind = 0;
    conditions[0].status = 1;
    conditions[0].next = &conditions[1];
    conditions[1].group = 9;
    conditions[1].kind = 9;
    conditions[1].status = 2;
    conditions[1].next = &conditions[2];
    conditions[2].group = 4;
    conditions[2].kind = 2;
    conditions[2].status = 2;
    conditions[2].next = &conditions[3];
    conditions[3].group = 9;
    conditions[3].kind = 10;
    conditions[3].status = 1;
    manager.conditions = conditions;
    assert(aoe_outcome_manager_update(&manager, &ops, &result));
    assert(strcmp(trace.order, "ECC") == 0);
    assert(result == 1);
    for (int i = 0; i < 4; ++i) assert(conditions[i].visited == 1);

    memset(&trace, 0, sizeof(trace));
    conditions[0].status = 2;
    conditions[2].status = 3;
    conditions[1].status = 2;
    conditions[3].status = 3;
    assert(aoe_outcome_manager_update(&manager, &ops, &result));
    assert(result == 2); /* First group contains only types 2 and 3. */

    AoeOutcomeCondition cumulative[2] = {{0}};
    cumulative[0].group = 1;
    cumulative[0].status = 3;
    cumulative[0].next = &cumulative[1];
    cumulative[1].group = 2;
    cumulative[1].status = 2;
    manager.conditions = cumulative;
    assert(aoe_outcome_recompute(&manager));
    assert(manager.outcome == 0); /* Type-3 count persists across groups. */

    manager.events = 0;
    manager.conditions = &cumulative[1];
    assert(aoe_outcome_recompute(&manager));
    assert(manager.outcome == 2);
    cumulative[1].next = &cumulative[1];
    assert(!aoe_outcome_recompute(&manager));
    return 0;
}
