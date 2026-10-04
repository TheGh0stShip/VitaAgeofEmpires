#include "live_unit_state.h"

#include <assert.h>
#include <string.h>

typedef struct {
    char calls[32];
    size_t count;
    int old_node, new_node;
    float *resource;
    int base_case;
} Trace;

static void record(Trace *trace, char call) {
    assert(trace->count + 1 < sizeof(trace->calls));
    trace->calls[trace->count++] = call;
    trace->calls[trace->count] = 0;
}
static void mode_two(void *ctx, AoeLiveUnitCore *unit) {
    Trace *trace = ctx;
    assert(unit->state == 2);
    assert(*trace->resource == 17.0f);
    record(trace, 'M');
}
static void pre_terminal(void *ctx, AoeLiveUnitCore *unit) {
    Trace *trace = ctx;
    assert(unit->state == 2);
    assert(*trace->resource == (trace->base_case ? 20.0f : 17.0f));
    record(trace, 'T');
}
static void leave(void *ctx, AoeLiveUnitCore *unit, int alternate) {
    assert(unit->state == 2);
    record(ctx, alternate ? 'L' : 'l');
}
static void enter(void *ctx, AoeLiveUnitCore *unit, int alternate) {
    assert(unit->state == 2 && alternate == 1);
    record(ctx, 'I');
}
static void state_three(void *ctx, AoeLiveUnitCore *unit) {
    assert(unit->state == 3);
    record(ctx, 'A');
}
static void remove_owner(void *ctx, AoeLiveUnitCore *unit,
                         uint8_t positive, uint8_t type_25, void *node) {
    Trace *trace = ctx;
    assert((unit->state == 2 || unit->state == 7) && positive == 1 && type_25 == 0);
    assert(node == &trace->old_node);
    record(trace, 'R');
}
static void *register_owner(void *ctx, AoeLiveUnitCore *unit,
                            uint8_t positive, uint8_t type_25) {
    Trace *trace = ctx;
    assert((unit->state == 2 || unit->state == 7) && positive == 0 && type_25 == 0);
    record(trace, 'G');
    return &trace->new_node;
}
static void parent_high(void *ctx, AoeLiveUnitCore *unit) {
    assert(unit->state == 2);
    record(ctx, 'P');
}
static void clear_action(void *ctx, AoeLiveUnitCore *unit) {
    assert(unit->state == 2);
    record(ctx, 'C');
}

int main(void) {
    DatUnit definition = {.present = 1, .line_of_sight = 4.0f,
                          .state_effect_mode = 2};
    AoeLiveUnitCore unit = {.unit_template = &definition, .state = 2,
                            .positive_hit_points = 1,
                            .registration_pending = 1};
    Trace trace = {0};
    float resource_value = 20.0f;
    AoePlayerResources resources = {.count = 1, .values = &resource_value};
    void *node = &trace.old_node;
    AoeLiveUnitRootStateOps ops = {
        .context = &trace, .owner_resources = &resources,
        .edition = AOE_PLAYER_RISE_OF_ROME,
        .mode_two_effect = mode_two, .pre_terminal = pre_terminal,
        .visibility_leave = leave, .visibility_enter = enter,
        .state_three_actions = state_three, .has_map_tile = 1,
        .registration_node = &node,
        .registration = {&trace, remove_owner, register_owner}
    };
    definition.storage[0] = (DatUnitStorage){.resource_type = 0,
                                             .amount = 3.0f, .paid = 2};
    trace.resource = &resource_value;
    assert(aoe_live_unit_root_state_set_ror(&unit, 2, &ops));
    assert(trace.count == 0 && unit.state == 2);
    assert(aoe_live_unit_root_state_set_ror(&unit, 7, &ops));
    assert(strcmp(trace.calls, "MTlIL") == 0 && resource_value == 17.0f);
    assert(unit.state == 7 && unit.positive_hit_points == 0 &&
           unit.registration_pending == 0 && node == &trace.old_node);
    assert(aoe_live_unit_root_state_set_ror(&unit, 6, &ops));
    assert(unit.state == 7 && strcmp(trace.calls, "MTlIL") == 0);
    assert(aoe_live_unit_root_state_set_ror(&unit, 0, &ops));
    assert(unit.state == 0 && strcmp(trace.calls, "MTlIL") == 0);

    memset(&trace, 0, sizeof(trace));
    unit.state = 2;
    unit.positive_hit_points = 1;
    unit.registration_pending = 0;
    resource_value = 20.0f;
    trace.resource = &resource_value;
    node = &trace.old_node;
    ops.mode_two_effect = NULL;
    assert(!aoe_live_unit_root_state_set_ror(&unit, 7, &ops));
    assert(unit.state == 2 && trace.count == 0);
    ops.mode_two_effect = mode_two;
    assert(aoe_live_unit_root_state_set_ror(&unit, 7, &ops));
    assert(strcmp(trace.calls, "MTlILRG") == 0);
    assert(node == &trace.new_node && unit.positive_hit_points == 0);

    memset(&trace, 0, sizeof(trace));
    unit.state = 2;
    unit.positive_hit_points = 1;
    unit.registration_pending = 1;
    resource_value = 20.0f;
    trace.resource = &resource_value;
    ops.has_map_tile = 0;
    ops.default_action = &trace.old_node;
    ops.current_action = &trace.old_node;
    ops.has_action_list = 1;
    ops.action_count = 2;
    assert(aoe_live_unit_root_state_set_ror(&unit, 3, &ops));
    assert(strcmp(trace.calls, "MA") == 0);
    assert(unit.state == 3 && unit.positive_hit_points == 0);
    memset(&trace, 0, sizeof(trace));
    unit.state = 2;
    unit.positive_hit_points = 1;
    unit.registration_pending = 1;
    resource_value = 20.0f;
    trace.resource = &resource_value;
    ops.has_action_list = 0;
    assert(aoe_live_unit_root_state_set_ror(&unit, 6, &ops));
    assert(strcmp(trace.calls, "M") == 0);
    assert(unit.state == 6 && unit.positive_hit_points == 1);

    memset(&trace, 0, sizeof(trace));
    unit.state = 2;
    unit.positive_hit_points = 1;
    unit.registration_pending = 1;
    resource_value = 20.0f;
    trace.resource = &resource_value;
    trace.base_case = 1;
    ops.edition = AOE_PLAYER_BASE;
    ops.has_map_tile = 1;
    ops.mode_two_effect = NULL; /* Base setter has no +0xa6 branch. */
    assert(aoe_live_unit_root_state_set_base(&unit, 7, &ops));
    assert(strcmp(trace.calls, "TlIL") == 0 && resource_value == 17.0f);
    assert(unit.state == 7 && unit.positive_hit_points == 0);
    assert(aoe_live_unit_root_state_set_base(&unit, 6, &ops));
    assert(unit.state == 6 && strcmp(trace.calls, "TlIL") == 0);

    definition.type = 10;
    unit.unit_template = &definition;
    assert(aoe_live_unit_low_state_ror(&unit, NULL, 8.0f));
    definition.type = 30;
    /* 0x0044c7b0: facet = (rotation - pi/4) * angles / 2pi + 0.5, wrapped
       by +angles below zero, truncated. */
    DatGraphic animated = {.sequence_type = 2, .frame_count = 8,
                           .angle_count = 8};
    assert(aoe_live_unit_low_state_ror(&unit, &animated, 1.25f));
    assert(unit.scenario_rotation == 1.25f && unit.scenario_frame == 1);
    assert(aoe_live_unit_low_state_ror(&unit, &animated, 0.0f));
    assert(unit.scenario_frame == 7); /* -0.5 + 8 */
    /* Three-angle wall graphics: scenario rotations 0, 2pi/3, 4pi/3. */
    animated.angle_count = 3;
    assert(aoe_live_unit_low_state_ror(&unit, &animated, 0.0f) &&
           unit.scenario_frame == 0);
    assert(aoe_live_unit_low_state_ror(&unit, &animated, 2.0944f) &&
           unit.scenario_frame == 1);
    assert(aoe_live_unit_low_state_ror(&unit, &animated, 4.18879f) &&
           unit.scenario_frame == 2);
    /* No angles: facet 0. */
    animated.angle_count = 0;
    assert(aoe_live_unit_low_state_ror(&unit, &animated, 4.0f) &&
           unit.scenario_frame == 0);
    animated.angle_count = 8;
    animated.sequence_type = 0;
    assert(aoe_live_unit_low_state_ror(&unit, &animated, 7.0f));
    assert(unit.scenario_frame == 0);
    definition.type = 99;
    assert(!aoe_live_unit_low_state_ror(&unit, NULL, 0.0f));

    memset(&trace, 0, sizeof(trace));
    definition.type = 10;
    definition.state_effect_mode = 0;
    unit.state = 2;
    unit.positive_hit_points = 1;
    unit.registration_pending = 0;
    unit.action_flags_36 = 1;
    resource_value = 20.0f;
    node = &trace.old_node;
    AoeLiveUnitRootHighStateOps high = {
        .context = &trace, .parent_high_state = parent_high,
        .clear_active_action = clear_action, .state = {
            .context = &trace, .owner_resources = &resources,
            .edition = AOE_PLAYER_RISE_OF_ROME,
            .pre_terminal = pre_terminal,
            .visibility_leave = leave, .visibility_enter = enter,
            .has_map_tile = 1, .registration_node = &node,
            .registration = {&trace, remove_owner, register_owner}
        }
    };
    trace.resource = &resource_value;
    assert(aoe_live_unit_root_high_state_ror(&unit, &high));
    assert(strcmp(trace.calls, "PCRGTlIL") == 0 && unit.state == 7 &&
           unit.positive_hit_points == 0 && node == &trace.new_node);
    assert(aoe_live_unit_root_high_state_ror(&unit, &high));
    assert(strcmp(trace.calls, "PCRGTlIL") == 0);
    return 0;
}
