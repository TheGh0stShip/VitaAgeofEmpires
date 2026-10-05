#include "world_tick.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    AoeWorldTickState *state;
    char order[64];
    unsigned used;
    uint16_t player_random[AOE_WORLD_MAX_PLAYERS];
    uint16_t followup_random[AOE_WORLD_MAX_PLAYERS * 2];
    unsigned player_calls;
    unsigned followup_calls;
    unsigned event_calls;
    int16_t expected_event_index;
    int pending_on_followup;
    AoeWorldEdition edition;
    AoeVisibilityUnitRef *registry_units[AOE_WORLD_MAX_PLAYERS + 1];
    AoeVisibilityRegistry registry;
    uint32_t synced_seed;
} Trace;

typedef struct {
    AoePlayerUpdateState state;
    AoeOutcomeManager outcome;
    AoeOutcomePlayerRef owner;
    AoeObjectUpdateNode primary_node;
    AoeObjectUpdateNode followup_node;
    unsigned visits;
    AoeVisibilityUnitRef visible_unit;
    uint32_t live_flags;
    AoeVisibilityWatchEntry visible_entry;
} TestPlayer;

static void mark(Trace *trace, char event) {
    assert(trace->used + 1 < sizeof(trace->order));
    trace->order[trace->used++] = event;
    trace->order[trace->used] = '\0';
}

static int timer_a(void *user, void *selected, uint32_t now, AoeRng *rng) {
    Trace *trace = user;
    assert(now == 500 || now == 700);
    assert(rng && selected);
    assert(trace->state && trace->state->clock.game_time_ms == 0);
    mark(trace, '1');
    return 1;
}

static int timer_b(void *user, void *selected, uint32_t now, AoeRng *rng) {
    assert(now == 500 || now == 700);
    assert(rng && selected);
    mark(user, '2');
    return 1;
}

static void *next_command(void *user) {
    mark(user, 'A');
    return 0;
}

static void dispatch_command(void *user, void *command) {
    assert(user && command);
}

static void release_command(void *user, void *command) {
    assert(user && command);
}

static uint8_t timeline_class(void *user, void *object) {
    assert(user && object);
    return 0;
}

static void timeline_pair(void *user, void *first, void *second) {
    assert(user && first && second);
}

static int timeline_spawn(void *user, uint8_t player_index,
                          int16_t template_index, float a, float b,
                          void **created) {
    assert(user && created);
    (void)player_index;
    (void)template_index;
    (void)a;
    (void)b;
    return 0;
}

static void timeline_position(void *user, void *object,
                              float a, float b, float c) {
    assert(user && object);
    (void)a;
    (void)b;
    (void)c;
}

static void timeline_simple(void *user, void *object) {
    assert(object);
    mark(user, 'B');
}

static int player_pending(void *user) {
    Trace *trace = user;
    assert(trace && trace->state);
    return trace->pending_on_followup &&
           trace->player_calls == (unsigned)trace->state->player_count;
}

static void player_pump(void *user) {
    mark(user, 'P');
}

static int player_predicate(void *user, void *object) {
    Trace *trace = user;
    TestPlayer *player = object;
    assert(object && trace->state);
    player->visits++;
    if (player->visits == 1) {
        mark(trace, 'M');
        trace->player_random[trace->player_calls++] =
            aoe_rng_next(&trace->state->rng);
    } else {
        assert(player->visits == 2);
        mark(trace, 'X');
        trace->followup_random[trace->followup_calls++] =
            aoe_rng_next(&trace->state->rng);
    }
    return 1;
}

static void player_action(void *user, void *object) {
    TestPlayer *player = object;
    assert(user && player);
    if (player->visits == 1) {
        player->state.queue58.field_04 = 3;
        player->state.queue58.field_08 = 5;
    } else {
        assert(player->state.queue58.field_04 == 0 &&
               player->state.queue58.field_08 == 0);
    }
}

static void player_event(void *user, int code, int16_t index) {
    Trace *trace = user;
    assert(trace && code == 6 && index == trace->expected_event_index);
    trace->event_calls++;
    mark(trace, 'E');
}

static void player_map(void *user, AoePlayerUpdateState *player) {
    assert(user && player);
    assert(!"no map update expected");
}

static void pump(void *user, AoeRng *rng) {
    assert(rng);
    mark(user, 'P');
}

static void visibility_notify(void *user, int player_index,
                              const AoeVisibilityUnitRef *unit) {
    Trace *trace = user;
    assert(trace && unit && player_index == 1);
    mark(trace, trace->edition == AOE_WORLD_BASE ? 'Z' : 'Y');
    trace->followup_random[trace->followup_calls++] =
        aoe_rng_next(&trace->state->rng);
}

static void finalize(void *user, AoeRng *rng) {
    assert(rng);
    mark(user, 'F');
}

static void sync_seed(void *user, uint32_t seed) {
    Trace *trace = user;
    trace->synced_seed = seed;
    mark(trace, 'S');
}

static AoeWorldTickOps ops_for(Trace *trace) {
    AoeWorldTickOps ops = {0};
    ops.user = trace;
    ops.timers.user = trace;
    ops.timers.action_a = timer_a;
    ops.timers.action_b = timer_b;
    ops.commands.user = trace;
    ops.commands.active = 1;
    ops.commands.next = next_command;
    ops.commands.dispatch = dispatch_command;
    ops.commands.release = release_command;
    ops.timeline.user = trace;
    ops.timeline.object_class = timeline_class;
    ops.timeline.action_pair = timeline_pair;
    ops.timeline.spawn = timeline_spawn;
    ops.timeline.action_position = timeline_position;
    ops.timeline.action_simple = timeline_simple;
    ops.player.user = trace;
    ops.player.emit_state_event = player_event;
    ops.player.map_update = player_map;
    ops.player.object_ops.user = trace;
    ops.player.object_ops.cooperative_pending = player_pending;
    ops.player.object_ops.cooperative_pump = player_pump;
    ops.player.object_ops.predicate = player_predicate;
    ops.player.object_ops.action = player_action;
    ops.cooperative_pump = pump;
    ops.visibility.user = trace;
    ops.visibility.notify_lost = visibility_notify;
    ops.world_finalize = finalize;
    ops.seed_sync = sync_seed;
    return ops;
}

static AoeWorldTickState initial_state(void) {
    static int timer_slot;
    static void *timer_slots[1] = {&timer_slot};
    static const AoeVisibilityRegistry empty_registry = {0};
    AoeWorldTickState state = {0};
    state.clock.speed = 1.0f;
    state.timers.count_a = 1;
    state.timers.count_b = 1;
    state.timers.slots_a = timer_slots;
    state.timers.slots_a_capacity = 1;
    state.timers.slots_b = timer_slots;
    state.timers.slots_b_capacity = 1;
    state.saved_seed = 1;
    state.registry = &empty_registry;
    return state;
}

static void init_player(TestPlayer *player, int32_t unit_id) {
    memset(player, 0, sizeof(*player));
    player->outcome.owner = &player->owner;
    player->state.outcome_manager = &player->outcome;
    player->primary_node.object = player;
    player->state.object_list28.head = &player->primary_node;
    player->state.object_list28.count = 1;
    player->followup_node.object = player;
    player->state.object_list30.head = &player->followup_node;
    player->state.object_list30.count = 1;
    player->visible_entry.unit = &player->visible_unit;
    player->visible_entry.live_flags = &player->live_flags;
    player->visible_entry.previous_flags = 2;
    player->visible_entry.unit_id = unit_id;
    player->state.visibility_watch40.entries = &player->visible_entry;
    player->state.visibility_watch40.count = 1;
    player->state.visibility_watch40.capacity = 1;
}

static void bind_registry(AoeWorldTickState *state, Trace *trace) {
    for (int i = 0; i < state->player_count; ++i) {
        AoeVisibilityWatchEntry *entry =
            &state->players[i]->visibility_watch40.entries[0];
        assert(entry->unit_id > 0 && entry->unit_id <= AOE_WORLD_MAX_PLAYERS);
        trace->registry_units[entry->unit_id] = entry->unit;
    }
    trace->registry.positive = trace->registry_units;
    trace->registry.positive_count = AOE_WORLD_MAX_PLAYERS + 1;
    trace->registry.positive_capacity = AOE_WORLD_MAX_PLAYERS + 1;
    state->registry = &trace->registry;
}

static void test_ror_first_local_frame(void) {
    Trace trace = {0};
    AoeWorldTickState state = initial_state();
    AoeWorldTickOps ops = ops_for(&trace);
    AoeWorldClockResult result = {0};
    AoeRng expected;
    TestPlayer player;
    AoeScenarioAction action = {0};
    AoeOwnedVisibilityRegistry owned_registry;
    AoeVisibilityUnitRef *replaced = 0;
    int needs_reassign = 0;
    assert(sizeof(action) >= 48);
    init_player(&player, 1);
    trace.edition = AOE_WORLD_RISE_OF_ROME;
    trace.state = &state;
    state.players[0] = &player.state;
    state.player_count = 1;
    assert(aoe_owned_visibility_registry_init(&owned_registry));
    assert(aoe_owned_visibility_registry_insert(
        &owned_registry, 1, &player.visible_unit, &replaced,
        &needs_reassign));
    assert(replaced == 0 && !needs_reassign);
    state.registry = &owned_registry.view;
    /* Growth after binding must not leave world_tick with stale arrays. */
    assert(aoe_owned_visibility_registry_insert(
        &owned_registry, 20000, &player.visible_unit, &replaced,
        &needs_reassign));
    assert(replaced == 0 && !needs_reassign);
    assert(state.registry == &owned_registry.view);
    action.kind = 4;
    action.target_a = &player.state;
    state.timeline.attached = 1;
    state.timeline.head = &action;
    state.timeline.last_time_s = -1.0f;

    assert(aoe_world_tick_core(&state, AOE_WORLD_RISE_OF_ROME, -1, 500,
                               1, &ops, &result));
    assert(result.gate_accepted && result.first_local_frame);
    assert(result.run_simulation && result.input_delta_ms == 0);
    assert(strcmp(trace.order, "12ABMPXYFS") == 0);
    assert(state.update_serial == 1 && state.player_elapsed_ms[0] == 0);
    assert(state.next_elapsed_reset == 0);
    assert(state.timeline.last_time_s == 0.0f);
    assert(trace.player_calls == 1 && trace.player_random[0] == 41);
    assert(trace.followup_calls == 2);
    assert(trace.followup_random[0] == 6334);
    assert(trace.followup_random[1] == 26500);
    aoe_rng_seed(&expected, 18467); /* Output after the primary pass. */
    assert(state.saved_seed == aoe_rng_next(&expected));
    assert(state.rng.state == expected.state);
    assert(trace.synced_seed == state.saved_seed);
    aoe_owned_visibility_registry_destroy(&owned_registry);
}

static void test_base_two_players(void) {
    Trace trace = {0};
    AoeWorldTickState state = initial_state();
    AoeWorldTickOps ops = ops_for(&trace);
    AoeWorldClockResult result = {0};
    AoeRng expected;
    TestPlayer players[2];
    AoeScenarioAction action = {0};
    init_player(&players[0], 1);
    init_player(&players[1], 2);
    trace.state = &state;
    state.players[0] = &players[0].state;
    state.players[1] = &players[1].state;
    state.player_count = 2;
    bind_registry(&state, &trace);
    action.kind = 4;
    action.target_a = &players[0].state;
    state.timeline.attached = 1;
    state.timeline.head = &action;
    state.timeline.last_time_s = -1.0f;

    assert(aoe_world_tick_core(&state, AOE_WORLD_BASE, 40, 700,
                               0, &ops, &result));
    assert(result.run_simulation && result.input_delta_ms == 40);
    assert(strcmp(trace.order, "12ABMMXZXZFS") == 0);
    assert(trace.player_calls == 2);
    assert(trace.player_random[0] == 41);
    assert(trace.player_random[1] == 18467);
    assert(trace.followup_calls == 4);
    assert(trace.followup_random[0] == 26500);
    assert(trace.followup_random[1] == 19169);
    assert(trace.followup_random[2] == 15724);
    assert(trace.followup_random[3] == 11478);
    assert(state.player_elapsed_ms[0] == 0);
    assert(state.player_elapsed_ms[1] == 40);
    assert(state.next_elapsed_reset == 1 && state.update_serial == 1);
    assert(state.timeline.last_time_s == (float)40 * 0.001f);
    aoe_rng_seed(&expected, 6334); /* Output after both primary passes. */
    assert(state.saved_seed == aoe_rng_next(&expected));
    assert(state.rng.state == expected.state);
    assert(trace.synced_seed == state.saved_seed);
}

static void test_gate_and_no_simulation(void) {
    Trace trace = {0};
    AoeWorldTickState state = initial_state();
    AoeWorldTickOps ops = ops_for(&trace);
    AoeWorldClockResult result = {0};
    trace.state = &state;
    state.clock.delta_game_ms = 9;

    assert(aoe_world_tick_core(&state, AOE_WORLD_BASE, 0, 500,
                               0, &ops, &result));
    assert(!result.gate_accepted && !result.run_simulation);
    assert(state.clock.delta_game_ms == 0 && state.saved_seed == 1);
    assert(trace.used == 0 && state.update_serial == 0);

    state.clock.speed = 0.0f;
    assert(aoe_world_tick_core(&state, AOE_WORLD_BASE, 40, 500,
                               0, &ops, &result));
    assert(result.gate_accepted && !result.run_simulation);
    assert(strcmp(trace.order, "12S") == 0);
    assert(state.saved_seed == 41 && trace.synced_seed == 41);
    assert(state.update_serial == 0);
}

static void test_world_player_outcome_transition(void) {
    Trace trace = {0};
    AoeWorldTickState state = initial_state();
    AoeWorldTickOps ops = ops_for(&trace);
    AoeWorldClockResult result = {0};
    TestPlayer player;
    init_player(&player, 1);
    trace.edition = AOE_WORLD_RISE_OF_ROME;
    trace.state = &state;
    trace.expected_event_index = 3;
    player.owner.index4a = 3;
    player.outcome.outcome = 1; /* No conditions retains prior result. */
    state.players[0] = &player.state;
    state.player_count = 1;
    bind_registry(&state, &trace);
    assert(aoe_world_tick_core(&state, AOE_WORLD_RISE_OF_ROME, 40, 700,
                               0, &ops, &result));
    assert(strcmp(trace.order, "12AMEXYFS") == 0);
    assert(trace.event_calls == 1 && player.owner.state80 == 2);
    assert(player.state.queue58.field_04 == 0 &&
           player.state.queue58.field_08 == 0);
}

static void test_base_followup_pumps_per_player(void) {
    Trace trace = {.pending_on_followup = 1};
    AoeWorldTickState state = initial_state();
    AoeWorldTickOps ops = ops_for(&trace);
    AoeWorldClockResult result = {0};
    TestPlayer players[2];
    init_player(&players[0], 1);
    init_player(&players[1], 2);
    trace.state = &state;
    state.players[0] = &players[0].state;
    state.players[1] = &players[1].state;
    state.player_count = 2;
    bind_registry(&state, &trace);
    assert(aoe_world_tick_core(&state, AOE_WORLD_BASE, 40, 700,
                               0, &ops, &result));
    assert(strcmp(trace.order, "12AMMPXZPXZFS") == 0);
    assert(players[0].visits == 2 && players[1].visits == 2);
}

static void test_missing_behavior_fails_closed(void) {
    Trace trace = {0};
    AoeWorldTickState state = initial_state();
    /* memcpy, not assignment: assignment leaves padding unspecified. */
    AoeWorldTickState original;
    memcpy(&original, &state, sizeof(original));
    AoeWorldTickOps ops = ops_for(&trace);
    AoeWorldClockResult result = {0};
    ops.player.object_ops.predicate = 0;
    assert(!aoe_world_tick_core(&state, AOE_WORLD_BASE, 40, 700,
                                0, &ops, &result));
    assert(memcmp(&state, &original, sizeof(state)) == 0);
    assert(trace.used == 0);
    ops.player.object_ops.predicate = player_predicate;
    state.player_count = AOE_WORLD_MAX_PLAYERS + 1;
    memcpy(&original, &state, sizeof(original));
    assert(!aoe_world_tick_core(&state, AOE_WORLD_BASE, 40, 700,
                                0, &ops, &result));
    assert(memcmp(&state, &original, sizeof(state)) == 0);

    state = initial_state();
    AoePlayerUpdateState invalid_player = {0};
    state.players[0] = &invalid_player;
    state.player_count = 1;
    memcpy(&original, &state, sizeof(original));
    assert(!aoe_world_tick_core(&state, AOE_WORLD_BASE, 40, 700,
                                0, &ops, &result));
    assert(memcmp(&state, &original, sizeof(state)) == 0);
    assert(trace.used == 0);

    state = initial_state();
    AoeVisibilityRegistry invalid_registry = {.positive_count = 1};
    state.registry = &invalid_registry;
    memcpy(&original, &state, sizeof(original));
    assert(!aoe_world_tick_core(&state, AOE_WORLD_BASE, 40, 700,
                                0, &ops, &result));
    assert(memcmp(&state, &original, sizeof(state)) == 0);
    assert(trace.used == 0);
}

int main(void) {
    test_ror_first_local_frame();
    test_base_two_players();
    test_gate_and_no_simulation();
    test_world_player_outcome_transition();
    test_base_followup_pumps_per_player();
    test_missing_behavior_fails_closed();
    return 0;
}
