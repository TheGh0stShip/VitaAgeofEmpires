#include "train_action.h"
#include <assert.h>

typedef struct { int notifications, signals; } Trace;

static void notify_cancel(void *context, AoeTrainAction *action) {
    Trace *trace = context;
    assert(action->owner_id == 3 && action->unit_id == 88);
    ++trace->notifications;
}

static void signal_cancel(void *context, AoeTrainAction *action) {
    Trace *trace = context;
    assert(action->state == 1 && action->building_id == 44);
    ++trace->signals;
}

int main(void) {
    float values[8] = {0};
    DatUnit unit = {0};
    AoePlayerResources resources = {8, values};
    AoeTrainAction action = {0};
    Trace trace = {0};
    AoeTrainCancelOps ops = {&trace, notify_cancel, signal_cancel};

    unit.type = 70;
    unit.costs[0] = (DatUnitCost){1, 25, 1};
    unit.costs[1] = (DatUnitCost){2, 7, 0};
    action.state = 6;
    action.unit_id = 88;
    action.owner_id = 3;
    action.building_id = 44;
    action.trained_template = &unit;
    action.owner_resources = &resources;
    action.prepaid = 0;
    assert(aoe_train_action_cancel_ror(&action, &ops));
    assert(action.state == 1 && values[1] == 25 && values[2] == 0);
    assert(trace.notifications == 1 && trace.signals == 1);

    /* State 1 skips only state/refund; it still emits both cancellation calls. */
    assert(aoe_train_action_cancel_ror(&action, &ops));
    assert(values[1] == 25 && trace.notifications == 2 && trace.signals == 2);

    action.state = 6;
    action.prepaid = 1;
    assert(aoe_train_action_cancel_ror(&action, &ops));
    assert(values[1] == 25 && trace.notifications == 3 && trace.signals == 3);
    assert(!aoe_train_action_cancel_ror(NULL, &ops));
    assert(!aoe_train_action_cancel_ror(&action, NULL));
    return 0;
}
