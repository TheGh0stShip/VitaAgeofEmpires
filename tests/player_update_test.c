#include "player_update.h"

#include <assert.h>
#include <string.h>

typedef struct {
    char events[32];
    unsigned used;
    int state_calls;
    int map_calls;
    int16_t expected_index;
    AoePlayerUpdateState *player;
    AoePlayerGroupMember *kept;
    AoePlayerGroupMember *removed;
} Trace;

static void mark(Trace *trace, char event) {
    assert(trace->used + 1 < sizeof(trace->events));
    trace->events[trace->used++] = event;
    trace->events[trace->used] = '\0';
}

static int pending(void *user) {
    mark(user, 'Q');
    return 1;
}

static void pump(void *user) {
    mark(user, 'P');
}

static int predicate(void *user, void *object) {
    mark(user, 'C');
    return *(int *)object == 2;
}

static void action(void *user, void *object) {
    assert(*(int *)object == 2);
    mark(user, 'A');
}

static void assert_outcome_entry(Trace *trace) {
    AoePlayerUpdateState *player = trace->player;
    assert(player->field_10 == 0);
    assert(player->group.count == 1);
    assert(player->group.slots[0] == trace->kept);
    assert(trace->removed->flag36 == 0);
}

static void emit_state_event(void *user, int code, int16_t player_index) {
    Trace *trace = user;
    assert(code == 6 && player_index == trace->expected_index);
    if (trace->player) assert_outcome_entry(trace);
    trace->state_calls++;
    mark(trace, 'S');
}

static void map_update(void *user, AoePlayerUpdateState *player) {
    Trace *trace = user;
    assert(player->flag_20 == 0);
    assert_outcome_entry(trace);
    trace->map_calls++;
    mark(trace, 'M');
}

static void run_edition(AoePlayerGroupEdition edition, int outcome_value,
                        int map_flag, const char *expected) {
    Trace trace = {0};
    AoePlayerUpdateState player = {0};
    AoePlayerGroupMember removed = {7, 0, 1};
    AoePlayerGroupMember kept = {1, 0, 1};
    AoeObjectUpdateNode nodes[2] = {{0}};
    AoeOutcomeManager outcome_manager = {0};
    AoeOutcomePlayerRef owner_ref = {.index4a = 1};
    AoeOutcomeEvent outcome_event = {.kind = 0, .divisor_08 = 10,
                                     .weight_0c = 2};
    AoeOutcomeCondition outcome_condition = {0};
    AoePlayerUpdateOps ops = {0};
    int objects[2] = {1, 2};
    float resource = 100.0f;
    owner_ref.resources50 = &resource;
    owner_ref.resource_capacity = 1;
    nodes[0].object = &objects[0];
    nodes[0].next = &nodes[1];
    nodes[1].object = &objects[1];
    player.object_list28.head = nodes;
    player.object_list28.count = 2;
    player.field_10 = 123;
    player.flag_20 = 1;
    player.map_update_flag = (unsigned)map_flag;
    player.group.count = 2;
    player.group.slots[0] = &removed;
    player.group.slots[1] = &kept;
    player.group.leader = &removed;
    outcome_condition.status = outcome_value ? 1 : 0;
    outcome_condition.kind = 10;
    outcome_condition.player_08 = &owner_ref;
    outcome_manager.conditions = &outcome_condition;
    outcome_manager.events = outcome_value ? &outcome_event : 0;
    outcome_manager.owner = &owner_ref;
    player.outcome_manager = &outcome_manager;
    trace.player = &player;
    trace.kept = &kept;
    trace.removed = &removed;
    trace.expected_index = 1;
    ops.object_ops.user = &trace;
    ops.object_ops.predicate = predicate;
    ops.object_ops.action = action;
    ops.object_ops.cooperative_pending = pending;
    ops.object_ops.cooperative_pump = pump;
    ops.user = &trace;
    ops.emit_state_event = emit_state_event;
    ops.map_update = map_update;

    assert(aoe_player_update_step(&player, edition, &ops));
    assert(strcmp(trace.events, expected) == 0);
    assert(trace.state_calls == (outcome_value == 1));
    assert(outcome_manager.score_sum_1c == (outcome_value == 1 ? 20u : 0u));
    assert(owner_ref.state80 == (outcome_value == 1 ? 2 : 0));
    assert(trace.map_calls == map_flag);
    assert(player.flag_20 == 0);
    assert(player.group.leader == &kept);
}

int main(void) {
    run_edition(AOE_GROUP_RISE_OF_ROME, 1, 1, "QPCCASM");
    run_edition(AOE_GROUP_BASE, 0, 0, "QPCCA");
    AoePlayerUpdateState player = {0};
    assert(!aoe_player_update_step(&player, AOE_GROUP_BASE, 0));
    AoeOutcomePlayerRef ref = {.index4a = 3};
    Trace trace = {.expected_index = 3};
    AoePlayerUpdateOps ops = {.user = &trace,
                              .emit_state_event = emit_state_event};
    assert(aoe_player_state_set(&ref, 1, &ops));
    assert(ref.state80 == 1 && trace.state_calls == 0);
    ref.state_lock81 = 1;
    assert(aoe_player_state_set(&ref, 0, &ops));
    assert(ref.state80 == 1);
    assert(aoe_player_state_set(&ref, 2, &ops));
    assert(ref.state80 == 2 && trace.state_calls == 0);
    ref.state_lock81 = 0;
    assert(aoe_player_state_set(&ref, 0, &ops));
    ops.emit_state_event = 0;
    assert(!aoe_player_state_set(&ref, 2, &ops));
    assert(ref.state80 == 0);
    ops.emit_state_event = emit_state_event;
    assert(aoe_player_state_set(&ref, 2, &ops));
    assert(ref.state80 == 2 && trace.state_calls == 1);
    return 0;
}
