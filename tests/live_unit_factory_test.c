#include "live_unit_factory.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void record_cell(void *context, int32_t x, int32_t y) {
    (void)context;
    (void)x;
    (void)y;
}

static int lookup_kind_68(void *context, int16_t *kind) {
    int *called = context;
    *called += 1;
    *kind = 0x68;
    return 1;
}

static void derived_kind_68(void *context, AoeLiveUnit *unit) {
    int *called = context;
    assert(unit->core.state == 2);
    *called += 10;
}

typedef struct {
    AoeLiveUnit *expected;
    uint8_t state;
    unsigned calls;
} TargetStateTrace;

static int set_target_state(void *context, AoeLiveUnit *unit, uint8_t state) {
    TargetStateTrace *trace = context;
    assert(unit == trace->expected);
    trace->state = state;
    trace->calls++;
    unit->core.state = state;
    return 1;
}

typedef struct {
    char events[16];
    unsigned count;
    int16_t graphic_id;
    int16_t sound_id;
} UpdateStateTrace;

typedef struct { char events[8]; unsigned count; } BuildingHighTrace;
static void building_high_event(BuildingHighTrace *t,char event) {
    assert(t->count < sizeof(t->events)); t->events[t->count++]=event;
}
static void building_pre_high(void *context,AoeLiveUnit *unit) {
    assert(unit->building_manager_link==(void *)0x1234);
    building_high_event(context,'P');
}
static int building_remove(void *context,AoeLiveUnit *unit) {
    assert(unit->core.state==7 && unit->building_manager_link==(void *)0x1234);
    building_high_event(context,'R'); return 1;
}
static int building_remove_state_three(void *context,AoeLiveUnit *unit) {
    assert(unit->core.state==2 && unit->building_manager_link==(void *)0x1234);
    building_high_event(context,'R'); return 1;
}
static int building_probe(void *context,int16_t template_id) {
    BuildingHighTrace *t=context; assert(template_id==109);
    building_high_event(t,'Q'); return 0;
}
static void building_remove_research(void *context,int16_t research_id) {
    BuildingHighTrace *t=context;
    static const int16_t expected[]={105,0x4a,0x4b,0x4c};
    unsigned index=t->count-3;
    assert(index<4 && research_id==expected[index]);
    building_high_event(t,'C');
}
static void queue_refund_unexpected(void *context,int16_t unit_id) {
    (void)context;(void)unit_id;assert(0);
}
static void queue_cancel_unexpected(void *context,void *action) {
    (void)context;(void)action;assert(0);
}
static void queue_notify_unexpected(void *context,uint32_t event,
                                    int16_t owner,uint32_t reason) {
    (void)context;(void)event;(void)owner;(void)reason;assert(0);
}
typedef struct {
    char events[16];
    unsigned count;
    AoeRng *rng;
    uint8_t expected_angle;
    uint8_t paint_fail;
} BuildingCompletionTrace;
static void completion_event(BuildingCompletionTrace *t,char event) {
    assert(t->count < sizeof(t->events)); t->events[t->count++]=event;
}
static void completion_local(void *context,AoeLiveUnit *unit,int value) {
    BuildingCompletionTrace *t=context;
    assert(value==1 && !unit->building_completed && t->rng->state!=41);
    completion_event(t,'L');
}
static void completion_pre(void *context,AoeLiveUnit *unit) {
    BuildingCompletionTrace *t=context;
    assert(unit->building_completed && t->rng->state==41);
    completion_event(t,'P');
}
static int completion_dc(void *context,AoeLiveUnit *unit) {
    BuildingCompletionTrace *t=context;
    assert(unit->building_update->angle==t->expected_angle);
    completion_event(context,'C'); return 1;
}
static int completion_d8(void *context,AoeLiveUnit *unit) {
    BuildingCompletionTrace *t=context;
    assert(unit->building_update->angle==t->expected_angle);
    completion_event(context,'D'); return 1;
}
static int completion_graphic(void *context,AoeLiveUnit *unit,
                              int16_t graphic_id) {
    BuildingCompletionTrace *t=context;
    assert(unit->building_update->angle==t->expected_angle && graphic_id==0);
    completion_event(context,'G'); return 1;
}
static void completion_adjacent(void *context,AoeLiveUnit *unit) {
    assert(unit->building_update->angle==7);
    completion_event(context,'A');
}
static void completion_disappear(void *context,AoeLiveUnit *unit) {
    assert(unit->building_completed);
    completion_event(context,'X');
}
static void completion_spawn(void *context,AoeLiveUnit *unit,int16_t unit_id,
                             float x,float y,float z) {
    assert(unit_id==42 && x==2.25f && y==2.25f && z==0.0f &&
           unit->building_completed);
    completion_event(context,'S');
}
static void completion_paint(void *context,AoeLiveUnit *unit,
                             const AoeBuildingFoundationBounds *bounds,
                             uint8_t terrain_id) {
    assert(unit->building_completed && terrain_id==9 &&
           bounds->x0==0 && bounds->y0==1 &&
           bounds->x1==3 && bounds->y1==2);
    completion_event(context,'F');
}

static int completion_paint_runtime(void *context,AoeLiveUnit *unit,
                                    const AoeBuildingFoundationBounds *bounds,
                                    uint8_t terrain_id) {
    completion_paint(context,unit,bounds,terrain_id);
    return !((BuildingCompletionTrace *)context)->paint_fail;
}
static void completion_invalidate(void *context,AoeLiveUnit *unit,
                                  int32_t x0,int32_t y0,
                                  int32_t x1,int32_t y1) {
    assert(unit->building_completed && x0==-1 && y0==0 && x1==4 && y1==3);
    completion_event(context,'I');
}
static void completion_research(void *context,AoeLiveUnit *unit,
                                int16_t research_id) {
    assert(unit->building_completed && research_id==105);
    completion_event(context,'R');
}
static void completion_notify(void *context,uint32_t event,int16_t owner_id,
                              int16_t unit_id,int32_t x,int32_t y) {
    assert(event==0x69 && owner_id==3 && unit_id==109 && x==2 && y==2);
    completion_event(context,'N');
}
static void completion_owner(void *context,AoeLiveUnit *unit,
                             uint32_t completion_value) {
    assert(unit->building_completed && completion_value==0x12345678u);
    completion_event(context,'O');
}
static void completion_ui_refresh(void *context) {
    unsigned *calls=context; ++*calls;
}
static void completion_ui_trace_refresh(void *context) {
    completion_event(context,'O');
}
static void completion_research_apply(void *context,int16_t research_id) {
    assert(research_id==105); completion_event(context,'R');
}
static void completion_research_rebuild(void *context) {
    completion_event(context,'B');
}
static void completion_research_wonder(void *context,int index) {
    (void)context;(void)index;assert(0);
}
static void building_action_destroy(void *context,void *action) {
    assert(action==(void *)0x5678); building_high_event(context,'D');
}
static void building_action_state(void *context,void *action,uint8_t state) {
    (void)context;(void)action;(void)state;assert(0);
}
static void building_queue_deallocate(void *context,void *allocation) {
    building_high_event(context,'F'); free(allocation);
}

static void update_event(UpdateStateTrace *trace, char event) {
    assert(trace->count < sizeof(trace->events));
    trace->events[trace->count++] = event;
}

static int update_active(void *context, AoeLiveUnit *unit) {
    assert(unit);
    update_event(context, 'A');
    return 1;
}

static int update_set_state(void *context, AoeLiveUnit *unit, uint8_t state) {
    update_event(context, 'S');
    unit->core.state = state;
    return 1;
}

static int update_select_graphic(void *context, AoeLiveUnit *unit,
                                 int16_t graphic_id) {
    UpdateStateTrace *trace = context;
    assert(unit);
    update_event(trace, 'G');
    trace->graphic_id = graphic_id;
    return 1;
}

static void update_dying_sound(void *context, AoeLiveUnit *unit,
                               int16_t sound_id) {
    UpdateStateTrace *trace = context;
    assert(unit);
    update_event(trace, 'D');
    trace->sound_id = sound_id;
}

static int update_terminal(void *context, AoeLiveUnit *unit) {
    assert(unit);
    update_event(context, 'T');
    return 1;
}

int main(void) {
    ScnTile terrain[16] = {0};
    AoeObjectUpdateList tiles[16] = {0};
    int8_t counts[16];
    uint32_t masks[16] = {0};
    uint8_t view[16] = {0};
    int32_t relations[1] = {1};
    float resources[1] = {0};
    AoePlayerResources resource_state = {1, resources};
    AoeOwnedVisibilityRegistry world;
    AoePlayerUpdateState owner = {0};
    AoeRng rng = {.state = 1};
    DatUnit unit = {.present = 1, .type = 10, .hit_points = 25,
                    .standing_graphic = 0, .walking_graphic = -1,
                    .line_of_sight = 1.0f};
    DatGraphic graphic = {.present = 1, .frame_count = 1, .angle_count = 1,
                          .frame_duration = 1.0f};
    DatGraphics graphics = {.count = 1, .present_count = 1, .graphics = &graphic};
    DatMapGeometry geometry = {.tile_half_width = 32,
                               .tile_half_height = 16};
    AoeVisibilityFootprintGrid grid = {
        .width = 4, .height = 4, .counts = counts, .tile_masks = masks,
        .view_codes = view, .visible_mask = 1u, .clear_visible_mask = ~1u,
        .explored_mask = 0x10000u
    };
    AoeLiveUnitVisionBinding vision = {
        .owner_grid = &grid, .owner_index = 0, .active_index = 0,
        .active_relations = relations, .relation_count = 1,
        .explored_ops = {.record_owner = record_cell,
                         .record_active = record_cell}
    };
    AoeLiveUnit live = {0};
    AoeLiveUnit other = {0};
    int32_t assigned_id;
    for (unsigned i = 0; i < 16; ++i) counts[i] = -1;
    assert(aoe_owned_visibility_registry_init(&world));

    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        1.25f, 2.25f, 0.0f));
    assert(live.constructed && live.core.state == 2 &&
           live.core.unit_template == &unit);
    assert(live.core.draw_left == 16 && live.core.draw_top == 16 &&
           live.core.draw_right == 16 && live.core.draw_bottom == 16);
    assert(live.graphic.current_graphic == &graphic &&
           aoe_sprite_list_count(&live.sprites.list) == 1);
    assert(live.placement.tile == &tiles[9] && tiles[9].count == 1);
    assert(live.select_2c == 1 && live.select_30 == 0 &&
           live.owner_node == owner.object_list2c.head &&
           owner.object_list2c.count == 1);
    assert(aoe_visibility_registry_resolve(&world.view, live.core.unit_id) ==
           &live.core.position);
    /* The actual 0..2 player-spawn branch reaches the type-10 no-op. */
    assert(aoe_live_unit_scenario_dispatch_ror(
        &live, 2, 3.0f, &(AoeLiveUnitScenarioDispatchOps){0}));
    assigned_id = live.core.unit_id;

    /* Patched 0x004a8f40 stably prunes missing and state>2 dependencies. */
    assert(aoe_live_unit_construct_ror(&other, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        3.25f, 2.25f, 0.0f));
    int32_t dependency_ids[] = {
        other.core.unit_id, -1, live.core.unit_id,
        other.core.unit_id, 999999};
    int32_t repeated_dependency_ids[] = {
        live.core.unit_id, live.core.unit_id};
    assert(!aoe_live_unit_has_dependencies_ror(&live));
    assert(aoe_live_unit_set_dependencies_ror(
        &live, dependency_ids, 5, 1234, UINT32_C(0x3f800000)));
    assert(aoe_live_unit_has_dependencies_ror(&live) &&
           live.dependent_unit_count == 3 &&
           live.dependent_unit_capacity == 3 &&
           live.dependent_unit_ids[0] == other.core.unit_id &&
           live.dependent_unit_ids[1] == live.core.unit_id &&
           live.dependent_unit_ids[2] == 999999 &&
           live.dependent_source_id == 1234 &&
           live.dependent_context_bits == UINT32_C(0x3f800000));
    other.core.state = 3;
    int handled = -1;
    assert(aoe_live_unit_update_prelude_ror(
        &live, &owner, &world, &handled));
    assert(!handled && live.dependent_unit_count == 1 &&
           live.dependent_unit_ids[0] == live.core.unit_id);
    assert(aoe_live_unit_set_dependencies_ror(
        &live, repeated_dependency_ids, 2, -5, UINT32_C(0x40000000)) &&
        live.dependent_unit_count == 1 &&
        live.dependent_unit_capacity == 3 &&
        live.dependent_unit_ids[0] == live.core.unit_id);
    assert(aoe_live_unit_set_dependencies_ror(
        &live, dependency_ids, -1, -7, UINT32_C(0x40400000)) &&
        !aoe_live_unit_has_dependencies_ror(&live) &&
        live.dependent_unit_capacity == 3 &&
        live.dependent_source_id == -7 &&
        live.dependent_context_bits == UINT32_C(0x40400000));
    other.core.state = 2;
    aoe_live_unit_destroy_ror(&other, &owner, &world, &vision);

    /* Original +0x20 suppresses the normal switch; states above 2 propagate
       through the current object's class-specific virtual +0x5c setter. */
    TargetStateTrace target_trace = {.expected = &live};
    AoeLiveUnitCore target = {.state = 2};
    AoeLiveUnitUpdateTargetOps target_ops = {
        .context = &target_trace, .set_state = set_target_state
    };
    handled = -1;
    assert(aoe_live_unit_update_attached_target_ror(
        &live, &target_ops, &handled));
    assert(!handled && target_trace.calls == 0);
    live.placement.attached_unit = &target;
    assert(aoe_live_unit_update_attached_target_ror(
        &live, &target_ops, &handled));
    assert(handled && target_trace.calls == 0 && live.core.state == 2);
    target.state = 7;
    assert(!aoe_live_unit_update_attached_target_ror(&live, 0, &handled));
    assert(handled && live.core.state == 2);
    assert(aoe_live_unit_update_attached_target_ror(
        &live, &target_ops, &handled));
    assert(handled && target_trace.calls == 1 && target_trace.state == 7 &&
           live.core.state == 7);
    live.core.state = 2;
    live.placement.attached_unit = 0;

    /* Complete no-target state switch in patched 0x004a8f40. */
    UpdateStateTrace update_trace = {0};
    AoeLiveUnitUpdateStateOps update_ops = {
        .context = &update_trace,
        .active_update = update_active,
        .set_state = update_set_state,
        .select_graphic = update_select_graphic,
        .play_dying_sound = update_dying_sound,
        .terminal_cleanup = update_terminal,
        .world_delta = 1.5f
    };
    uint8_t predicate_result = 99;
    live.core.hit_points = 1.0f;
    assert(aoe_live_unit_update_normal_state_ror(
        &live, &update_ops, &predicate_result));
    assert(predicate_result == 0 && update_trace.count == 1 &&
           update_trace.events[0] == 'A' && live.core.state == 2);

    memset(&update_trace, 0, sizeof(update_trace));
    unit.dying_graphic = 7;
    unit.dying_sound = 5;
    live.core.hit_points = -1.0f;
    assert(aoe_live_unit_update_normal_state_ror(
        &live, &update_ops, &predicate_result));
    assert(live.core.state == 3 && update_trace.count == 4 &&
           memcmp(update_trace.events, "ASGD", 4) == 0 &&
           update_trace.graphic_id == 7 && update_trace.sound_id == 5);

    memset(&update_trace, 0, sizeof(update_trace));
    unit.decay_rate = 2.0f;
    live.core.storage.carried_resource_type = 0;
    live.core.storage.carried_amount = 10.0f;
    assert(aoe_live_unit_update_normal_state_ror(
        &live, &update_ops, &predicate_result));
    assert(live.core.storage.carried_amount == 7.0f &&
           update_trace.count == 0);

    unit.undead_mode = 1;
    unit.undead_graphic = 9;
    live.core.state = 5;
    live.core.storage.carried_amount = 0.0f;
    assert(aoe_live_unit_update_normal_state_ror(
        &live, &update_ops, &predicate_result));
    assert(live.core.state == 6 && update_trace.count == 2 &&
           memcmp(update_trace.events, "SG", 2) == 0 &&
           update_trace.graphic_id == 9);

    memset(&update_trace, 0, sizeof(update_trace));
    unit.undead_mode = 0;
    live.core.state = 3;
    live.core.storage.carried_resource_type = -1;
    assert(aoe_live_unit_update_normal_state_ror(
        &live, &update_ops, &predicate_result));
    assert(live.core.state == 7 && update_trace.count == 2 &&
           memcmp(update_trace.events, "ST", 2) == 0);
    assert(aoe_live_unit_update_normal_state_ror(
        &live, &update_ops, &predicate_result));
    assert(live.core.state == 8 && update_trace.count == 3 &&
           update_trace.events[2] == 'S');
    unit.negative_id_mode = 0;
    assert(aoe_live_unit_update_normal_state_ror(
        &live, &update_ops, &predicate_result) && predicate_result == 1);
    unit.negative_id_mode = 1;
    assert(aoe_live_unit_update_normal_state_ror(
        &live, &update_ops, &predicate_result) && predicate_result == 2);
    live.core.state = 2;
    live.core.hit_points = 25.0f;

    aoe_live_unit_destroy_ror(&live, &owner, &world, &vision);
    assert(!live.constructed && owner.object_list2c.count == 0 &&
           tiles[9].count == 0 &&
           !aoe_visibility_registry_resolve(&world.view, assigned_id));
    for (unsigned i = 0; i < 16; ++i) assert(counts[i] <= 0);

    /* 0x004a8200 passes +0x49 then +0x4a to player +0xdc: a zero-HP
       type-25 unit therefore selects list +0x30, not +0x2c or +0x28. */
    unit.type = 25;
    unit.hit_points = 0;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    assert(live.select_2c == 0 && live.select_30 == 1 &&
           live.owner_node == owner.object_list30.head &&
           owner.object_list30.count == 1);
    /* Type-20 base constructors zero +0x88. Its update predicate passes that
       field and world delta to sprite-manager +0x08. */
    assert(live.animation_rate == 0.0f);
    assert(aoe_live_unit_tick_sprites_ror(&live, 0.5f));
    assert(aoe_sprite_runtime_at(&live.sprites, 0)->animation.frame == 0);
    aoe_live_unit_destroy_ror(&live, &owner, &world, &vision);
    assert(owner.object_list30.count == 0);

    /* Zero-to-positive health defers list reclassification until +0x24. */
    unit.type = 10;
    unit.hit_points = 0;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    assert(live.owner_node == owner.object_list28.head &&
           aoe_live_unit_health_flag_set(
               &live.core, 1, (void **)&live.owner_node,
               &(AoeLiveUnitRegistrationHooks){0}) &&
           live.core.registration_pending && owner.object_list28.count == 1);
    handled = 0;
    assert(aoe_live_unit_update_prelude_ror(
        &live, &owner, &world, &handled));
    assert(handled && !live.core.registration_pending &&
           live.owner_node == owner.object_list2c.head &&
           owner.object_list28.count == 0 && owner.object_list2c.count == 1);
    aoe_live_unit_destroy_ror(&live, &owner, &world, &vision);
    assert(owner.object_list2c.count == 0);

    /* Effect-free root high state uses real owner and vision adapters. */
    unit.type = 10;
    unit.hit_points = 25;
    unit.state_notify_mode = 0;
    unit.state_effect_mode = 0;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    assert(owner.object_list2c.count == 1);
    assert(aoe_live_unit_scenario_high_root_ror(
        &live, &resource_state, &owner, &world, &vision));
    assert(live.core.state == 7 && !live.core.positive_hit_points &&
           owner.object_list2c.count == 0 && owner.object_list28.count == 1 &&
           live.select_2c == 0 && live.select_30 == 0);
    aoe_live_unit_destroy_ror(&live, &owner, &world, &vision);
    assert(owner.object_list28.count == 0);

    unit.state_notify_mode = 1;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    assert(!aoe_live_unit_scenario_high_root_ror(
        &live, &resource_state, &owner, &world, &vision));
    assert(live.core.state == 2 && owner.object_list2c.count == 1);
    aoe_live_unit_destroy_ror(&live, &owner, &world, &vision);

    unit.state_effect_mode = 2;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    masks[10] = 0; /* Exercise the campaign tree's zero-high-mask branch. */
    assert(aoe_live_unit_scenario_high_root_ror(
        &live, &resource_state, &owner, &world, &vision));
    assert(live.core.state == 7 && owner.visibility_watch40.count == 0 &&
           owner.visibility_watch40.capacity == 32);
    aoe_live_unit_destroy_ror(&live, &owner, &world, &vision);
    aoe_visibility_watch_destroy(&owner.visibility_watch40);

    /* A type-30 scenario record uses 0x0044ca40 then 0x0044c7b0. */
    unit.type = 30;
    unit.hit_points = 25;
    unit.state_notify_mode = 0;
    unit.state_effect_mode = 0;
    graphic.sequence_type = 3; /* angle-sequenced and animated */
    graphic.frame_count = 4;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    assert(aoe_live_unit_scenario_dispatch_ror(
        &live, 0, 258.75f, &(AoeLiveUnitScenarioDispatchOps){0}));
    assert(live.core.scenario_rotation == 258.75f &&
           live.core.scenario_frame == 2);
    live.animation_rate = 1.0f;
    assert(aoe_live_unit_tick_sprites_ror(&live, 1.0f));
    assert(aoe_sprite_runtime_at(&live.sprites, 0)->animation.frame == 1);
    aoe_live_unit_destroy_ror(&live, &owner, &world, &vision);

    /* Type-70 0x004b02a0 probes the current object then runs root middle. */
    unit.type = 70;
    int derived_calls = 0;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    live.core.registration_pending = 1;
    assert(aoe_live_unit_scenario_dispatch_ror(&live, 3, 0.0f,
        &(AoeLiveUnitScenarioDispatchOps){
            .context = &derived_calls, .derived_middle_time = 1.0f,
            .derived_middle_threshold = 1.0f,
            .derived_middle_lookup = lookup_kind_68,
            .derived_middle_kind_68 = derived_kind_68}));
    assert(derived_calls == 11 && !live.core.positive_hit_points &&
           !live.core.registration_pending);
    aoe_live_unit_destroy_ror(&live, &owner, &world, &vision);

    /* Type-80 high state composes 004ae450 instead of an opaque callback. */
    unit.type=80; unit.state_notify_mode=0; unit.state_effect_mode=0;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    BuildingHighTrace high={0};
    AoeActionNode *high_node=malloc(sizeof(*high_node));
    assert(high_node); *high_node=(AoeActionNode){(void *)0x5678,NULL};
    AoeActionList high_list={&live.core.state,high_node,
        {&high,building_action_destroy,building_action_state}};
    live.derived_action_list=&high_list;
    live.building_manager_link=(void *)0x1234;
    live.core.registration_pending=1;
    AoeLiveUnitScenarioDispatchOps high_ops={
        .context=&high,
        .building_pre_high=building_pre_high,
        .remove_building_manager=building_remove,
        .root_high={.state={.owner_resources=&resource_state,
                            .edition=AOE_PLAYER_RISE_OF_ROME}}
    };
    assert(aoe_live_unit_scenario_dispatch_ror(&live,7,0.0f,&high_ops));
    assert(high.count==3 && !memcmp(high.events,"PDR",3));
    assert(live.core.state==7 && !live.building_manager_link &&
           !live.derived_action_list->head);
    live.derived_action_list=NULL;
    aoe_live_unit_destroy_ror(&live,&owner,&world,&vision);

    /* Setter state 2->3 composes its manager, queue, root and cleanup tail. */
    unit.id=109; unit.building_research_id=105;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    AoeBuildingUpdate building={.unit_template=&unit,.state=2,
        .queue_status=-1,.queue_actions=&high_list};
    high_list.head=NULL; high_list.owner_state=&building.state;
    live.building_update=&building;
    live.building_manager_link=(void *)0x1234;
    live.core.registration_pending=1;
    high=(BuildingHighTrace){0};
    uint32_t status_four_count=0;
    AoeBuildingStateThreeOps state_three={
        .context=&high,
        .building_pre_state_three=building_pre_high,
        .remove_building_manager=building_remove_state_three,
        .completion_probe=building_probe,
        .remove_research=building_remove_research,
        .owner_status_four_count=&status_four_count,
        .queue_cancel={&high,queue_refund_unexpected,
                       queue_cancel_unexpected,queue_notify_unexpected},
        .root_state={.owner_resources=&resource_state,
                     .edition=AOE_PLAYER_RISE_OF_ROME}
    };
    assert(aoe_live_unit_building_state_three_ror(&live,&state_three));
    assert(high.count==7 && !memcmp(high.events,"PRQCCCC",7));
    assert(live.core.state==3 && building.state==3 &&
           !live.building_manager_link);
    live.building_update=NULL;
    aoe_live_unit_destroy_ror(&live,&owner,&world,&vision);

    /* Setter state 2 completion prefix preserves RNG and visual ordering. */
    unit.train_time=50; unit.building_angle=7;
    unit.building_adjacent_mode=1; unit.disappears_when_built=0;
    unit.size[0]=1.5f; unit.size[1]=0.75f;
    unit.stack_unit_id=42; unit.foundation_terrain=9;
    unit.building_research_id=105;
    assert(aoe_live_unit_construct_ror(&live, &unit, &graphics,
                                        &resource_state, &world, &owner,
                                        terrain, tiles, &geometry,
                                        4, 4, &vision, &rng,
                                        2.25f, 2.25f, 0.0f));
    AoeActionList completion_live_actions,completion_queue_actions;
    AoeActionListOps completion_list_ops={
        &high,building_action_destroy,building_action_state};
    assert(aoe_live_unit_bind_building_update_ror(
        &live,&building,&completion_live_actions,completion_list_ops,
        &completion_queue_actions,completion_list_ops,
        resource_state,3,0x12345678u));
    assert(live.building_update==&building &&
           live.derived_action_list==&completion_live_actions &&
           building.live_actions==&completion_live_actions &&
           building.queue_actions==&completion_queue_actions &&
           building.state==live.core.state && building.angle==7 &&
           building.graphic_angles==1 && building.build_time==50 &&
           building.queue_enabled && building.queue_status==-1 &&
           building.owner_id==3 &&
           live.building_completion_value==0x12345678u);
    rng.state=1;
    BuildingCompletionTrace completion={.rng=&rng,.expected_angle=7};
    AoeBuildingCompletionVisualOps completion_ops={
        .context=&completion,.rng=&rng,.signal_local_completion=1,
        .local_completion=completion_local,
        .building_pre_complete=completion_pre,
        .visibility_dc=completion_dc,.visibility_d8=completion_d8,
        .select_graphic=completion_graphic,
        .complete_adjacent=completion_adjacent,
        .disappear=completion_disappear
    };
    assert(aoe_live_unit_building_completion_visual_ror(
        &live,&completion_ops));
    assert(completion.count==6 && !memcmp(completion.events,"LPCDGA",6));
    assert(building.elapsed_build_time==50.0f && building.angle==7 &&
           live.building_completed && rng.state==41);

    unit.disappears_when_built=1; rng.state=1;
    completion=(BuildingCompletionTrace){.rng=&rng};
    completion_ops.context=&completion;
    completion_ops.signal_local_completion=0;
    live.building_completed=0;
    assert(aoe_live_unit_building_completion_visual_ror(
        &live,&completion_ops));
    assert(completion.count==2 && !memcmp(completion.events,"PX",2));
    assert(rng.state==41 && live.building_completed);

    AoeBuildingFoundationBounds foundation;
    assert(aoe_live_unit_building_foundation_bounds_ror(&live,&foundation));
    assert(foundation.x0==0 && foundation.y0==1 &&
           foundation.x1==3 && foundation.y1==2);
    unsigned ui_refreshes=0;
    uint32_t wonder_completed=0;
    AoeBuildingCompletionUiEntryRor ui_match={
        .template_id=109,.completion_key=0x12345678,
        .object_id=-1,.display_name="Town Center"};
    AoeBuildingCompletionUiEntryRor ui_miss={
        .next=&ui_match,.template_id=276,.completion_key=0x12345678,
        .object_id=-1,.display_name="Wonder"};
    AoeBuildingCompletionUiRor completion_ui={
        .head=&ui_miss,.wonder_completed=&wonder_completed,
        .selected_completion_key=-1,.refresh_context=&ui_refreshes,
        .refresh=completion_ui_refresh};
    assert(aoe_live_unit_owner_complete_ui_ror(
        &completion_ui,&live,0x12345678u));
    assert(ui_refreshes==1 && ui_match.completed && !ui_miss.completed &&
           ui_match.object_id==live.core.unit_id && !wonder_completed &&
           completion_ui.selected_completion_key==0x12345678 &&
           !strcmp(completion_ui.selected_name,"Town Center"));
    unit.id=276;
    completion_ui.head=&ui_miss;
    assert(aoe_live_unit_owner_complete_ui_ror(
        &completion_ui,&live,0x12345678u));
    assert(ui_refreshes==2 && ui_miss.completed && wonder_completed==1);
    unit.id=109;
    assert(aoe_live_unit_owner_complete_ui_ror(NULL,&live,0x12345678u));
    completion=(BuildingCompletionTrace){0};
    AoeBuildingCompletionWorldOps world_completion={
        .context=&completion,.spawn_stack_unit=completion_spawn,
        .paint_foundation=completion_paint,
        .invalidate_foundation=completion_invalidate,
        .apply_research=completion_research,
        .notify_completion=completion_notify,
        .owner_complete=completion_owner
    };
    assert(aoe_live_unit_building_completion_world_ror(
        &live,&world_completion));
    assert(completion.count==6 && !memcmp(completion.events,"SFIRNO",6));

    unit.disappears_when_built=0; rng.state=1;
    live.building_completed=0; building.angle=0;
    completion=(BuildingCompletionTrace){.rng=&rng};
    completion_ops.context=&completion;
    completion_ops.signal_local_completion=1;
    world_completion.context=&completion;
    AoeBuildingStateTwoOps state_two={
        .visual=completion_ops,.world=world_completion,
        .root_state={.owner_resources=&resource_state,
                     .edition=AOE_PLAYER_RISE_OF_ROME}
    };
    assert(aoe_live_unit_building_state_two_ror(&live,&state_two));
    assert(completion.count==12 &&
           !memcmp(completion.events,"LPCDGASFIRNO",12));
    assert(live.core.state==2 && building.state==2 &&
           live.building_completed && rng.state==41);

    /* Runtime composition binds the real vision and sprite virtuals. */
    rng.state=1; live.building_completed=0; building.angle=0;
    completion=(BuildingCompletionTrace){.rng=&rng};
    AoeBuildingCompletionVisualRuntimeOps visual_runtime={
        .context=&completion,.rng=&rng,.graphics=&graphics,.vision=&vision,
        .signal_local_completion=1,.local_completion=completion_local,
        .building_pre_complete=completion_pre,
        .complete_adjacent=completion_adjacent,
        .disappear=completion_disappear
    };
    ui_match.completed=0;
    completion_ui.head=&ui_match;
    completion_ui.refresh_context=&completion;
    completion_ui.refresh=completion_ui_trace_refresh;
    AoeResearchProgressEntry completion_research_entries[106]={{0}};
    AoeResearchCompletionManagerRor completion_research_manager={
        106,completion_research_entries,&completion,
        completion_research_apply,completion_research_rebuild,
        completion_research_wonder};
    AoeBuildingCompletionWorldRuntimeOps world_runtime={
        .context=&completion,.spawn_stack_unit=completion_spawn,
        .paint_foundation=completion_paint_runtime,
        .invalidate_foundation=completion_invalidate,
        .research=&completion_research_manager,
        .owner_ui=&completion_ui
    };
    AoeBuildingStateTwoRuntimeOps state_two_runtime={
        .visual=visual_runtime,.world=world_runtime,
        .root_state={.owner_resources=&resource_state,
                     .edition=AOE_PLAYER_RISE_OF_ROME}
    };
    assert(aoe_live_unit_building_state_two_runtime_ror(
        &live,&state_two_runtime));
    assert(completion.count==9 &&
           !memcmp(completion.events,"LPASFIRBO",9));
    assert(live.core.state==2 && building.state==2 &&
           live.building_completed && rng.state==41 &&
           completion_research_entries[105].state==3);
    completion.count=0;
    completion.paint_fail=1;
    completion_research_entries[105].state=0;
    ui_match.completed=0;
    assert(!aoe_live_unit_building_completion_world_runtime_ror(
        &live,&world_runtime));
    assert(completion.count==6 && !memcmp(completion.events,"SFIRBO",6));
    AoeActionNode *completion_live_node=malloc(sizeof(*completion_live_node));
    AoeActionNode *completion_queue_node=malloc(sizeof(*completion_queue_node));
    assert(completion_live_node && completion_queue_node);
    *completion_live_node=(AoeActionNode){(void *)0x5678,NULL};
    *completion_queue_node=(AoeActionNode){(void *)0x5678,NULL};
    completion_live_actions.head=completion_live_node;
    completion_queue_actions.head=completion_queue_node;
    building.queue_entries=malloc(sizeof(*building.queue_entries));
    assert(building.queue_entries);
    building.queue_capacity=1;
    high=(BuildingHighTrace){0};
    completion_live_actions.ops.context=&high;
    completion_queue_actions.ops.context=&high;
    assert(aoe_live_unit_unbind_building_update_ror(
        &live,&status_four_count,&high,building_queue_deallocate));
    assert(high.count==3 && !memcmp(high.events,"DFD",3) &&
           !live.building_update && !live.derived_action_list &&
           !live.building_completion_value && !building.queue_entries);
    aoe_live_unit_destroy_ror(&live,&owner,&world,&vision);
    aoe_owned_visibility_registry_destroy(&world);
    return 0;
}
