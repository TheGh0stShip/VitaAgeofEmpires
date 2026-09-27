#include "object_update_list.h"

#include <assert.h>
#include <string.h>

typedef struct {
    int id;
    AoeObjectUpdateNode *node;
} TestObject;

typedef struct {
    int checked[24];
    int acted[24];
    int checks;
    int actions;
    int pumps;
    int pending;
    int disable_after_first_pump;
    char events[72];
    int event_count;
} Trace;

static void event(Trace *trace, char code) {
    assert(trace->event_count + 1 < (int)sizeof(trace->events));
    trace->events[trace->event_count++] = code;
    trace->events[trace->event_count] = '\0';
}

static int predicate(void *user, void *object) {
    Trace *trace = user;
    TestObject *item = object;
    trace->checked[trace->checks++] = item->id;
    event(trace, 'C');
    return (item->id & 1) == 0;
}

static void action(void *user, void *object) {
    Trace *trace = user;
    TestObject *item = object;
    trace->acted[trace->actions++] = item->id;
    event(trace, 'A');
    if (item->id == 2) item->node->next = 0;
}

static void pump(void *user) {
    Trace *trace = user;
    assert(trace->checks == 10 || trace->checks == 20);
    trace->pumps++;
    if (trace->disable_after_first_pump) trace->pending = 0;
    event(trace, 'P');
}

static int pending(void *user) {
    return ((Trace *)user)->pending;
}

int main(void) {
    AoeObjectUpdateNode nodes[23] = {{0}};
    TestObject objects[23] = {{0}};
    Trace trace = {0};
    AoeObjectUpdateOps ops = {&trace, predicate, action, pending, pump};
    trace.pending = 1;
    for (int i = 0; i < 23; ++i) {
        nodes[i].next = i + 1 < 23 ? &nodes[i + 1] : 0;
        objects[i].id = i;
        objects[i].node = &nodes[i];
        nodes[i].object = (i == 4 || i == 11) ? 0 : &objects[i];
    }
    assert(aoe_object_update_list(nodes, &ops));
    assert(trace.checks == 21 && trace.actions == 11 && trace.pumps == 2);
    for (int i = 0, seen = 0; i < 23; ++i) {
        if (i == 4 || i == 11) continue;
        assert(trace.checked[seen++] == i);
    }
    assert(trace.acted[0] == 0 && trace.acted[1] == 2);
    assert(trace.acted[10] == 22);
    assert(nodes[2].next == 0); /* The loop used its cached next. */

    memset(&trace, 0, sizeof(trace));
    nodes[2].next = &nodes[3];
    assert(aoe_object_update_list(nodes, &ops));
    assert(trace.checks == 21 && trace.actions == 11 && trace.pumps == 0);

    memset(&trace, 0, sizeof(trace));
    nodes[2].next = &nodes[3];
    trace.pending = 1;
    trace.disable_after_first_pump = 1;
    assert(aoe_object_update_list(nodes, &ops));
    assert(trace.checks == 21 && trace.pumps == 1);

    memset(&trace, 0, sizeof(trace));
    ops.cooperative_pump = 0;
    assert(!aoe_object_update_list(nodes, &ops));
    assert(trace.checks == 0);
    ops.cooperative_pump = pump;
    ops.predicate = 0;
    assert(!aoe_object_update_list(nodes, &ops));
    assert(trace.checks == 0);
    return 0;
}
