#include "command_drain.h"

#include <assert.h>
#include <string.h>

typedef struct {
    int commands[3];
    unsigned next_index;
    char order[16];
    unsigned used;
} Trace;

static void mark(Trace *trace, char event) {
    trace->order[trace->used++] = event;
    trace->order[trace->used] = '\0';
}

static void *next(void *user) {
    Trace *trace = user;
    mark(trace, 'N');
    if (trace->next_index == 3) return 0;
    return &trace->commands[trace->next_index++];
}

static void dispatch(void *user, void *command) {
    Trace *trace = user;
    assert(command == &trace->commands[trace->next_index - 1]);
    mark(trace, 'D');
}

static void release(void *user, void *command) {
    Trace *trace = user;
    assert(command == &trace->commands[trace->next_index - 1]);
    mark(trace, 'R');
}

int main(void) {
    Trace trace = {0};
    AoeCommandDrainOps ops = {0};
    assert(!aoe_command_drain(0));
    ops.user = &trace;
    ops.next = next;
    ops.dispatch = dispatch;
    ops.release = release;
    assert(aoe_command_drain(&ops));
    assert(trace.used == 0);
    ops.active = 1;
    assert(aoe_command_drain(&ops));
    assert(trace.next_index == 3);
    assert(strcmp(trace.order, "NDRNDRNDRN") == 0);
    ops.release = 0;
    assert(!aoe_command_drain(&ops));
    assert(strcmp(trace.order, "NDRNDRNDRN") == 0);
    return 0;
}
