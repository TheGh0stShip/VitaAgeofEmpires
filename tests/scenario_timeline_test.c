#include "scenario_timeline.h"

#include <assert.h>
#include <string.h>

typedef struct {
    int live;
    int stale;
    int created;
    char order[32];
    unsigned used;
} Trace;

static void mark(Trace *trace, char event) {
    trace->order[trace->used++] = event;
    trace->order[trace->used] = '\0';
}

static uint8_t object_class(void *user, void *object) {
    Trace *trace = user;
    return object == &trace->stale ? 8 : 7;
}

static void action_pair(void *user, void *first, void *second) {
    Trace *trace = user;
    assert(first == &trace->created && second == &trace->live);
    mark(trace, 'P');
}

static int spawn(void *user, uint8_t player_index, int16_t template_index,
                 float a, float b, void **created) {
    Trace *trace = user;
    assert(player_index == 2 && template_index == 6);
    assert(a == 3.0f && b == 4.0f);
    *created = &trace->created;
    mark(trace, 'S');
    return 1;
}

static void action_position(void *user, void *object,
                            float a, float b, float c) {
    Trace *trace = user;
    assert(object == &trace->created);
    assert(a == 5.0f && b == 6.0f && c == 7.0f);
    mark(trace, 'L');
}

static void action_simple(void *user, void *object) {
    Trace *trace = user;
    assert(object == &trace->created);
    mark(trace, 'F');
}

static AoeScenarioTimelineOps ops_for(Trace *trace) {
    AoeScenarioTimelineOps ops = {0};
    ops.user = trace;
    ops.object_class = object_class;
    ops.action_pair = action_pair;
    ops.spawn = spawn;
    ops.action_position = action_position;
    ops.action_simple = action_simple;
    return ops;
}

static void test_due_actions_and_references(void) {
    Trace trace = {0};
    AoeScenarioTimelineOps ops = ops_for(&trace);
    AoeScenarioAction actions[5] = {{0}};
    AoeScenarioTimeline timeline = {0};
    assert(sizeof(actions[0]) >= 48);
    for (int i = 0; i < 4; ++i) actions[i].next = &actions[i + 1];
    actions[0].time_s = 0.1f;
    actions[0].kind = 1;
    actions[0].player_index = 2;
    actions[0].template_index = 6;
    actions[0].arg_a = 3.0f;
    actions[0].arg_b = 4.0f;
    actions[0].target_a_id = 17;
    actions[1].time_s = 0.2f;
    actions[1].kind = 0;
    actions[1].target_a_id = 17;
    actions[1].target_a = &trace.stale;
    actions[1].target_b = &trace.live;
    actions[2].time_s = 0.3f;
    actions[2].kind = 2;
    actions[2].target_a_id = 17;
    actions[2].arg_a = 5.0f;
    actions[2].arg_b = 6.0f;
    actions[2].arg_c = 7.0f;
    actions[3].time_s = 0.4f;
    actions[3].kind = 4;
    actions[3].target_a_id = 17;
    actions[4].time_s = 0.5f;
    actions[4].kind = 3; /* Original switch has no action for kind 3. */
    actions[4].target_a = &trace.stale;
    timeline.attached = 1;
    timeline.head = &actions[0];
    timeline.last_time_s = 0.0f;

    assert(aoe_scenario_timeline_step(&timeline, 250, &ops));
    assert(strcmp(trace.order, "SP") == 0);
    assert(actions[1].target_a == &trace.created);
    assert(actions[2].target_a == &trace.created);
    assert(actions[3].target_a == &trace.created);
    assert(actions[4].target_a == 0);
    assert(timeline.last_time_s == (float)250 * 0.001f);
    assert(aoe_scenario_timeline_step(&timeline, 450, &ops));
    assert(strcmp(trace.order, "SPLF") == 0);
    assert(aoe_scenario_timeline_step(&timeline, 550, &ops));
    assert(strcmp(trace.order, "SPLF") == 0);
}

static void test_inactive_and_missing_bindings(void) {
    Trace trace = {0};
    AoeScenarioTimelineOps ops = ops_for(&trace);
    AoeScenarioTimeline timeline = {0};
    timeline.last_time_s = 1.0f;
    assert(aoe_scenario_timeline_step(&timeline, 2000, &ops));
    assert(timeline.last_time_s == 1.0f);
    ops.spawn = 0;
    timeline.attached = 1;
    assert(!aoe_scenario_timeline_step(&timeline, 2000, &ops));
    assert(timeline.last_time_s == 1.0f);
}

static void test_unsigned_time_conversion(void) {
    Trace trace = {0};
    AoeScenarioTimelineOps ops = ops_for(&trace);
    AoeScenarioTimeline timeline = {0};
    timeline.attached = 1;
    assert(aoe_scenario_timeline_step(&timeline, 16777217u, &ops));
    assert(timeline.last_time_s == 16777.21875f);
}

int main(void) {
    test_due_actions_and_references();
    test_inactive_and_missing_bindings();
    test_unsigned_time_conversion();
    return 0;
}
