#include "live_unit_core.h"

#include <assert.h>

typedef struct {
    int calls;
    AoeLiveUnitCore *unit;
    void *old_node;
    void *new_node;
    int action_calls, expect_action;
} RegistrationTrace;

static void clear_active_action(void *context, AoeLiveUnitCore *unit) {
    RegistrationTrace *trace = context;
    assert(unit == trace->unit && (unit->action_flags_36 & 1));
    assert(trace->calls == 0 && trace->action_calls == 0);
    trace->action_calls = 1;
    unit->action_flags_36 &= (uint8_t)~1u;
}

static void remove_registration(void *context, AoeLiveUnitCore *unit,
                                uint8_t was_positive, uint8_t type_25,
                                void *node) {
    RegistrationTrace *trace = context;
    assert(unit == trace->unit && was_positive == 1 && type_25 == 1);
    assert(node == trace->old_node && unit->positive_hit_points == 1);
    assert(trace->calls == 0);
    if (trace->expect_action) assert(trace->action_calls == 1);
    trace->calls = 1;
}

static void *register_unit(void *context, AoeLiveUnitCore *unit,
                           uint8_t is_positive, uint8_t type_25) {
    RegistrationTrace *trace = context;
    assert(unit == trace->unit && is_positive == 0 && type_25 == 1);
    assert(unit->positive_hit_points == 0 && trace->calls == 1);
    trace->calls = 2;
    return trace->new_node;
}

static void test_health_registration(void) {
    int nodes[2] = {0};
    AoeLiveUnitCore core = {.positive_hit_points = 1, .type_25 = 1,
                            .hit_points = 30.0f};
    void *node = &nodes[0];
    RegistrationTrace trace = {.unit = &core, .old_node = &nodes[0],
                               .new_node = &nodes[1]};
    AoeLiveUnitRegistrationHooks hooks = {
        .context = &trace, .remove = remove_registration,
        .register_unit = register_unit
    };
    AoeLiveUnitMiddleActionHooks action = {
        .context = &trace, .clear_active_action = clear_active_action
    };
    assert(aoe_live_unit_health_flag_set(&core, 1, &node, &hooks));
    assert(trace.calls == 0 && !core.registration_pending);
    assert(!aoe_live_unit_health_flag_set(&core, 0, &node, 0));
    assert(core.positive_hit_points == 1 && node == &nodes[0]);
    assert(aoe_live_unit_health_flag_set(&core, 0, &node, &hooks));
    assert(trace.calls == 2 && node == &nodes[1]);
    assert(core.positive_hit_points == 0 && core.hit_points == 30.0f);
    assert(aoe_live_unit_health_flag_set(&core, 1, &node, 0));
    assert(core.positive_hit_points == 1 && core.registration_pending == 1);
    assert(aoe_live_unit_health_flag_set(&core, 0, &node, 0));
    assert(core.positive_hit_points == 0 && core.registration_pending == 0);
    assert(node == &nodes[1] && trace.calls == 2);
    assert(aoe_live_unit_health_flag_set(&core, 1, &node, 0));
    assert(aoe_live_unit_root_middle_state(&core, &node, 0, 0));
    assert(core.hit_points == 0.0f && core.positive_hit_points == 0);
    assert(core.registration_pending == 0 && trace.calls == 2);
    core.positive_hit_points = 1;
    core.hit_points = 40.0f;
    trace.calls = 0;
    trace.old_node = &nodes[1];
    trace.new_node = &nodes[0];
    assert(aoe_live_unit_root_middle_state(&core, &node, &hooks, 0));
    assert(core.hit_points == 0.0f && core.positive_hit_points == 0);
    assert(trace.calls == 2 && node == &nodes[0]);
    core.positive_hit_points = 1;
    core.hit_points = 20.0f;
    core.action_flags_36 = 1;
    trace.calls = 0;
    trace.action_calls = 0;
    trace.expect_action = 1;
    trace.old_node = &nodes[0];
    trace.new_node = &nodes[1];
    assert(!aoe_live_unit_root_middle_state(&core, &node, &hooks, 0));
    assert(core.hit_points == 20.0f && trace.calls == 0);
    assert(aoe_live_unit_root_middle_state(&core, &node, &hooks, &action));
    assert(core.hit_points == 0.0f && core.action_flags_36 == 0);
    assert(trace.action_calls == 1 && trace.calls == 2 && node == &nodes[1]);
    assert(!aoe_live_unit_health_flag_set(&core, 2, &node, &hooks));
}

static void run_edition(AoePlayerEdition edition) {
    AoeOwnedVisibilityRegistry registry;
    DatUnit unit = {.present = 1, .type = 90, .hit_points = 50,
                    .negative_id_mode = 1};
    float resource_values[2] = {10.0f, 20.0f};
    AoePlayerResources resources = {.count = 2, .values = resource_values};
    AoeLiveUnitCore core = {0};
    unit.storage[0] = (DatUnitStorage){.resource_type = 0,
                                        .amount = 3.0f, .paid = 1};
    unit.storage[1] = (DatUnitStorage){.resource_type = 1,
                                        .amount = 7.5f, .paid = 0};
    unit.storage[2] = (DatUnitStorage){.resource_type = -1,
                                        .amount = 0.0f, .paid = 0};
    assert(aoe_owned_visibility_registry_init(&registry));
    assert(aoe_live_unit_core_begin(&unit, &resources, edition, &registry,
                                    4.5f, 9.0f, 1.0f, &core));
    assert(core.unit_template == &unit && core.unit_id == -1);
    assert(core.position.x38 == 4.5f && core.position.y3c == 9.0f &&
           core.position.z40 == 1.0f);
    assert(core.hit_points == 50.0f && core.positive_hit_points == 1);
    assert(core.type_25 == 0 && core.state == 2 && core.class_code == 10);
    assert(core.storage.carried_resource_type == 1 &&
           core.storage.carried_amount == 7.5f);
    assert(resource_values[0] == 13.0f && resource_values[1] == 20.0f);
    {
        ScnTile tiles[9] = {0};
        for (unsigned i = 0; i < 9; ++i) tiles[i].elevation = 1;
        tiles[1].elevation = 2; /* Center tile has shape 6. */
        core.position.x38 = 1.25f;
        core.position.y3c = 1.5f;
        unit.elevation_mode = 0;
        assert(aoe_live_unit_core_apply_surface(&core, tiles, 3, 3));
        assert(core.position.z40 == 1.5f);
        unit.elevation_mode = 1;
        core.position.z40 = 3.0f;
        assert(aoe_live_unit_core_apply_surface(&core, tiles, 3, 3));
        assert(core.position.z40 == 3.0f);
        core.position.z40 = 0.0f;
        assert(aoe_live_unit_core_apply_surface(&core, tiles, 3, 3));
        assert(core.position.z40 == 1.5f);
        unit.elevation_mode = 2;
        core.position.z40 = 0.0f;
        assert(aoe_live_unit_core_apply_surface(&core, tiles, 3, 3));
        assert(core.position.z40 == 0.0f);
    }
    unit.type = 25;
    unit.hit_points = 0;
    unit.negative_id_mode = 0;
    assert(aoe_live_unit_core_begin(&unit, &resources, edition, &registry,
                                    -3.0f, 2.0f, 0.0f, &core));
    assert(core.unit_id == 0 && core.hit_points == 0.0f);
    assert(core.positive_hit_points == 0 && core.type_25 == 1);
    {
        AoeLiveUnitCore before = core;
        float before_resource = resource_values[0];
        unit.present = 0;
        assert(!aoe_live_unit_core_begin(&unit, &resources, edition, &registry,
                                         99.0f, 99.0f, 99.0f, &core));
        assert(core.unit_id == before.unit_id &&
               core.position.x38 == before.position.x38 &&
               resource_values[0] == before_resource);
        unit.present = 1;
        assert(!aoe_live_unit_core_begin(&unit, &resources,
                                         (AoePlayerEdition)9, &registry,
                                         99.0f, 99.0f, 99.0f, &core));
        assert(core.unit_id == before.unit_id &&
               resource_values[0] == before_resource);
    }
    aoe_owned_visibility_registry_destroy(&registry);
}

int main(void) {
    run_edition(AOE_PLAYER_BASE);
    run_edition(AOE_PLAYER_RISE_OF_ROME);
    test_health_registration();
    return 0;
}
