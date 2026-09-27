#include "train_action_adapter.h"
#include <assert.h>
#include <string.h>

typedef struct { int charged, failed, signalled, notified; unsigned char out[16]; size_t used; } Trace;
static int charge(void *p, AoeTrainAction *a) { Trace *t=p; assert(a->prepaid); ++t->charged; return 1; }
static void failed(void *p, AoeTrainAction *a) { (void)p; (void)a; assert(!"unexpected"); }
static void signal(void *p, AoeTrainAction *a) { (void)p; (void)a; assert(!"unexpected"); }
static void notify(void *p, AoeTrainAction *a) { ++((Trace *)p)->notified; assert(a->state == 1); }
static void cancel_signal(void *p, AoeTrainAction *a) { ++((Trace *)p)->signalled; assert(a->state == 1); }
static int base(void *p, const AoeTrainAction *a) { (void)a; ++((Trace *)p)->failed; return 1; }
static int write(void *p, const void *bytes, size_t count) { Trace *t=p; memcpy(t->out+t->used,bytes,count); t->used+=count; return 1; }
static void nested_destroy(void *p, void *a) { (void)p; (void)a; }
static void nested_state(void *p, void *a, uint8_t state) { (void)p; (void)a; (void)state; }
static int adapter_prepare(void *p, AoeBuildingUpdate *building, int16_t unit_id,
                           AoeTrainActionCreate *out) {
    Trace *trace=p;
    static DatUnit unit;
    (void)trace;
    assert(building && unit_id==7);
    *out=(AoeTrainActionCreate){.trained_template=&unit,
                                .owned_action_ops={0,nested_destroy,nested_state}};
    return 1;
}

int main(void) {
    AoeTrainAction action = {.state = 1, .unit_id = 42};
    uint8_t result = 0;
    int16_t id = 0, percent = 0;
    /* Base 004a52c0 state-1 return and 004a5700 completed query do not
       require live world/template dependencies. */
    assert(aoe_train_action_update_base(&action, 0.0f, 0, NULL, &result));
    assert(result == 1);
    assert(aoe_train_action_progress_base(&action, &id, &percent));
    assert(id == 42 && percent == 100);
    for (uint32_t value = 0; value <= UINT16_MAX / 2; ++value) {
        uint16_t random = (uint16_t)value;
        assert(aoe_train_spawn_direction_base(random, 0) ==
               aoe_train_spawn_direction_ror(random, 0));
        assert(aoe_train_spawn_direction_base(random, 1) ==
               aoe_train_spawn_direction_ror(random, 1));
    }
    {
        DatUnit unit = {0}; float values[4] = {0}; Trace trace = {0};
        AoePlayerResources resources = {4, values};
        AoeTrainActivationOps activation = {&trace, charge, failed, signal};
        AoeTrainCancelOps cancel = {&trace, notify, cancel_signal};
        AoeTrainActionSerializeOps serialization = {&trace, base, write};
        unit.type=70; unit.costs[0]=(DatUnitCost){1,9,1};
        action=(AoeTrainAction){.state=4,.unit_id=42,.progress=3.0f,
                                .request_id=-1,.prepaid=1,.trained_template=&unit,
                                .owner_resources=&resources};
        /* Base ignores the RoR-only prepaid representation and charges. */
        assert(aoe_train_action_activate_base(&action,1,&activation));
        assert(action.state==6 && trace.charged==1);
        assert(aoe_train_action_cancel_base(&action,&cancel));
        assert(values[1]==9 && trace.notified==1 && trace.signalled==1);
        assert(aoe_train_action_serialize_base(&action,&serialization));
        assert(trace.failed==1 && trace.used==10);
    }
    {
        AoeActionListOps nested = {0, nested_destroy, nested_state};
        AoeTrainActionCreate create = {.unit_id=7,.request_id=99,.prepaid=1,
                                       .owned_action_ops=nested};
        AoeTrainAction *created=aoe_train_action_create_base(&create);
        assert(created && created->unit_id==7 && created->request_id==99 &&
               created->prepaid==0);
        /* No targets means NULL target hooks are sufficient teardown. */
        assert(aoe_train_action_destroy_ror(created,NULL));
    }
    {
        Trace trace={0}; AoeBuildingUpdate building={0};
        AoeTrainActionAdapter adapter={.context=&trace,.prepare=adapter_prepare,
                                       .edition=AOE_TRAIN_ADAPTER_BASE};
        AoeTrainAction *created=aoe_train_action_adapter_create(&adapter,&building,7);
        assert(created && created->prepaid==0);
        aoe_train_action_adapter_destroy(&adapter,created);
        assert(aoe_train_action_adapter_ok(&adapter));
    }
    return 0;
}
