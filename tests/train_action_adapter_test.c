#include "train_action_adapter.h"
#include <assert.h>
#include <string.h>

typedef struct {
    int prepared, created, completed, destroyed, queue_notifications;
    AoeRng rng;
    DatUnit building_template, trained_template;
    float resources[33];
} Trace;

static void nested_destroy(void *context, void *action) {
    (void)context;
    (void)action;
}

static void nested_state(void *context, void *action, uint8_t state) {
    (void)context;
    (void)action;
    (void)state;
}

static int prepare(void *context, AoeBuildingUpdate *building, int16_t unit_id,
                   AoeTrainActionCreate *out) {
    Trace *trace = context;
    assert(building->owner_id == 2 && unit_id == 83);
    ++trace->prepared;
    *out = (AoeTrainActionCreate){
        .building_template = &trace->building_template,
        .trained_template = &trace->trained_template,
        .owner_resources = &building->owner_resources,
        .owner_id = building->owner_id,
        .building_id = 77,
        .building_x = 10.0f,
        .building_y = 20.0f,
        .owned_action_ops = {trace, nested_destroy, nested_state},
    };
    return 1;
}

static int blocked(void *context, AoeTrainAction *action, float x, float y) {
    (void)action;
    (void)x;
    (void)y;
    return 0;
}

static int create_unit(void *context, AoeTrainAction *action, float x, float y,
                       int32_t *created_unit_id) {
    Trace *trace = context;
    assert(action->state == 1 && x == action->candidate_x && y == action->candidate_y);
    ++trace->created;
    *created_unit_id = 321;
    return 1;
}

static void train_notify(void *context, uint32_t event, int16_t owner,
                         int16_t unit, int32_t x, int32_t y) {
    Trace *trace = context;
    assert(event == 102 && owner == 2 && unit == 83 && x == 10 && y == 20);
    (void)trace;
}

static void train_failed(void *context, AoeTrainAction *action) {
    (void)context;
    (void)action;
    assert(!"a prepaid, valid training action must not fail");
}

static void train_completed(void *context, AoeTrainAction *action, int32_t id) {
    Trace *trace = context;
    assert(action->building_id == 77 && id == 321);
    ++trace->completed;
}

static void train_signal(void *context, AoeTrainAction *action, uint32_t code) {
    (void)context;
    assert(action->state == 1 && code == 506);
}

static void cancel(void *context, AoeBuildingUpdate *building, int16_t unit,
                   int16_t quantity) {
    (void)context;
    (void)building;
    (void)unit;
    (void)quantity;
    assert(!"cancellation is not expected");
}

static void *bridge_create(void *context, AoeBuildingUpdate *building, int16_t unit_id) {
    return aoe_train_action_adapter_create(context, building, unit_id);
}

static void production_notify(void *context, uint32_t event, int16_t owner,
                              uint32_t reason) {
    AoeTrainActionAdapter *adapter = context;
    Trace *trace = adapter->context;
    assert(event == 124 && owner == 2 && reason == 0);
    ++trace->queue_notifications;
}

int main(void) {
    Trace trace;
    AoeBuildingQueueEntry queue[] = {{83, 1}};
    AoeBuildingUpdate building;
    AoeTrainSpawnOps spawn;
    AoeTrainActionOps update;
    AoeTrainActivationOps activation;
    AoeTrainActionAdapter adapter;
    AoeActionListOps list_ops;
    AoeActionTickOps tick_ops;
    AoeActionList actions;
    AoeBuildingProductionOps production;
    uint32_t blocked_count = 0;

    memset(&trace, 0, sizeof(trace));
    trace.building_template.work_rate = 1.0f;
    trace.building_template.size[0] = trace.building_template.size[1] = 1.0f;
    trace.trained_template.train_time = 2.0f;
    trace.trained_template.size[0] = trace.trained_template.size[1] = 0.5f;
    trace.resources[32] = 10.0f;
    trace.resources[4] = 1.0f;
    aoe_rng_seed(&trace.rng, 1);

    spawn = (AoeTrainSpawnOps){&trace, blocked, create_unit};
    update = (AoeTrainActionOps){&trace, &trace.rng, &spawn, train_notify,
                                  train_failed, train_completed, train_signal};
    activation = (AoeTrainActivationOps){&trace, NULL, train_failed, NULL};
    adapter = (AoeTrainActionAdapter){.context = &trace, .prepare = prepare,
                                      .activation = &activation, .update = &update};
    aoe_train_action_adapter_set_timing(&adapter, 2.0f, 0);
    aoe_train_action_adapter_list_ops(&adapter, &list_ops);
    aoe_train_action_adapter_tick_ops(&adapter, &tick_ops);

    building = (AoeBuildingUpdate){
        .state = 0, .queue_count = 1, .queue_status = -1, .queue_actions = &actions,
        .live_actions = &actions,
        .queue_enabled = 1, .queue_entries = queue, .queue_capacity = 1,
        .owner_resources = {33, trace.resources}, .owner_id = 2,
    };
    actions = (AoeActionList){&building.state, NULL, list_ops};
    production = (AoeBuildingProductionOps){&adapter, cancel, bridge_create,
                                             &tick_ops, production_notify};

    assert(aoe_building_production_ror(&building, &blocked_count, &production));
    assert(aoe_train_action_adapter_ok(&adapter));
    assert(!actions.head && building.queue_count == 0 && building.queue_revision == 1);
    assert(trace.prepared == 1 && trace.created == 1 && trace.completed == 1);
    assert(trace.queue_notifications == 1 && blocked_count == 0);
    return 0;
}
