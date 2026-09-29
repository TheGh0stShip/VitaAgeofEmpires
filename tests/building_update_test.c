#include "building_update.h"
#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct { int calls, complete, terminal, change_state; } Trace;
static DatUnit normal_template = {.present=1,.unit_class=3};
static void set_state(void *p, AoeBuildingUpdate *b, uint8_t s) {
    Trace *t = p;
    assert(t->calls++ == 0 && s == 2);
    assert(b->history_ec == 3); /* before history shift */
    b->state = s;
}
static uint8_t parent(void *p, AoeBuildingUpdate *b) {
    Trace *t = p;
    assert(t->calls++ == t->complete);
    assert(b->history_ec == 2 && b->history_e8 == 1);
    assert(b->history_f8 == 5 && b->history_f4 == 4 && b->history_f0 == 99);
    if (t->change_state) b->state = 3;
    b->current_1b0 = 42;
    return 0xa7;
}
static void cancel(void *p,AoeBuildingUpdate *b,int16_t id,int16_t q) {
    (void)p;(void)b;(void)id;(void)q;assert(0);
}
static void *create(void *p,AoeBuildingUpdate *b,int16_t id) {
    (void)p;(void)b;(void)id;assert(0);return 0;
}
static uint8_t update(void *p,void *b) {
    (void)p;(void)b;assert(0);return 0;
}
static void waiting(void *p,void *b) { (void)p;(void)b;assert(0); }
static void notify(void *p,uint32_t e,int16_t id,uint32_t r) {
    (void)p;(void)e;(void)id;(void)r;assert(0);
}
static const AoeActionTickOps tick={0,update,waiting};
static const AoeBuildingProductionOps production={0,cancel,create,&tick,notify};
static void destroy(void *p, void *a) { (void)p; (void)a; assert(0); }
static void action_state(void *p, void *a, uint8_t s) {
    (void)p; (void)a; (void)s; assert(0);
}
static void attach(AoeBuildingUpdate *b, AoeActionList *list) {
    *list = (AoeActionList){&b->state, 0, {0, destroy, action_state}};
    b->queue_actions = list;
    b->live_actions = list; /* empty during every production query in this test */
}
static AoeBuildingUpdate initial(void) {
    AoeBuildingUpdate b = {0};
    b.unit_template = &normal_template;
    b.graphic_angles = 3; b.build_time = 8000;
    b.history_e4=1; b.history_e8=2; b.history_ec=3;
    b.history_f0=4; b.history_f4=5; b.history_f8=6;
    return b;
}
int main(void) {
    const float times[] = {0, 2666, 2667, 5333, 5334, 7999, 8000, 16000};
    const unsigned angles[] = {0,0,1,1,2,2,0,0};
    for (unsigned i=0; i<sizeof(times)/sizeof(times[0]); ++i) {
        AoeBuildingUpdate b = initial();
        Trace t = {0}; uint8_t result=0;
        uint32_t count=0; AoeActionList list;
        AoeBuildingUpdateOps ops = {&t,set_state,parent,1.0f,&production,&count};
        attach(&b,&list);
        b.elapsed_build_time = times[i]; t.complete = times[i]>=8000;
        assert(aoe_building_update_ror(&b,99,&ops,&result));
        assert(b.angle==angles[i] && b.state==(t.complete?2:0));
        assert(result==0xa7 && b.history_e4==42);
        assert(t.calls==t.complete+1 && b.elapsed_build_time==times[i]);
    }
    {
        AoeBuildingUpdate b = initial(); Trace t = {0}; uint8_t result=0;
        uint32_t count=1; AoeActionList list;
        AoeBuildingUpdateOps ops = {&t,set_state,parent,1.0f,&production,&count};
        attach(&b,&list); b.queue_status=4; b.queue_count=7;
        t.terminal=1; t.change_state=1;
        assert(aoe_building_update_ror(&b,99,&ops,&result));
        assert(b.history_e4==42 && result==0xa7 && t.calls==1);
        assert(b.queue_count==0 && b.queue_status==-1 && count==0);
        assert(b.queue_revision==1);
        b=initial(); t=(Trace){0}; b.state=2; b.elapsed_build_time=NAN;
        attach(&b,&list);
        assert(aoe_building_update_ror(&b,99,&ops,&result));
        b=initial(); t=(Trace){0}; b.elapsed_build_time=NAN;
        attach(&b,&list);
        AoeBuildingUpdate before=b;
        assert(!aoe_building_update_ror(&b,99,&ops,&result));
        assert(!memcmp(&b,&before,sizeof(b)) && !t.calls);
        b=initial(); attach(&b,&list);
        b.elapsed_build_time=8000; before=b; ops.set_state=0;
        assert(!aoe_building_update_ror(&b,99,&ops,&result));
        assert(!memcmp(&b,&before,sizeof(b)) && !t.calls);
        /* Original AL narrowing occurs before comparing against angles. */
        ops.set_state=set_state; b=initial(); b.graphic_angles=256;
        attach(&b,&list);
        b.build_time=1; b.elapsed_build_time=1; b.angle=9; t.complete=1;
        assert(aoe_building_update_ror(&b,99,&ops,&result));
        assert(b.angle==0);
    }
    {
        DatUnit generator = {.present=1,.unit_class=18};
        float resources[36] = {0};
        AoeBuildingUpdate b = initial(); Trace t = {0}; uint8_t result = 0;
        uint32_t count = 0; AoeActionList list;
        AoeBuildingUpdateOps ops = {&t,set_state,parent,0.5f,&production,&count};
        attach(&b,&list);
        b.unit_template = &generator;
        b.state = 2;
        b.owner_resources = (AoePlayerResources){36,resources};
        resources[35] = 12.0f;
        b.carried_amount = 91.0f;
        b.carried_resource_type = -1;
        assert(aoe_building_update_ror(&b,99,&ops,&result));
        assert(b.carried_amount == 97.0f && b.carried_resource_type == 34);
        assert(result == 0xa7 && t.calls == 1);

        b = initial(); t = (Trace){0}; attach(&b,&list);
        b.unit_template = &generator; b.state = 2;
        b.owner_resources = (AoePlayerResources){36,resources};
        b.carried_amount = 99.0f;
        assert(aoe_building_update_ror(&b,99,&ops,&result));
        assert(b.carried_amount == 100.0f && b.carried_resource_type == 34);

        b = initial(); t = (Trace){0}; attach(&b,&list);
        b.unit_template = &generator; b.state = 2;
        b.owner_resources = (AoePlayerResources){35,resources};
        AoeBuildingUpdate before = b;
        assert(!aoe_building_update_ror(&b,99,&ops,&result));
        assert(!memcmp(&b,&before,sizeof(b)) && !t.calls);

        b = initial(); t = (Trace){0}; attach(&b,&list);
        b.unit_template = &generator; b.state = 1;
        b.carried_amount = NAN;
        assert(aoe_building_update_ror(&b,99,&ops,&result));
        assert(isnan(b.carried_amount) && b.carried_resource_type == 0);
    }
    {
        AoeBuildingUpdate b = initial();
        int16_t percent = -1;
        b.build_time = 8000;
        b.elapsed_build_time = 2666.9f;
        assert(aoe_building_construction_percent_ror(&b,&percent));
        assert(percent == 33);
        b.elapsed_build_time = 8000.0f;
        assert(aoe_building_construction_percent_ror(&b,&percent));
        assert(percent == 100);
        b.elapsed_build_time = 2621440.0f; /* 32768%, low signed word */
        assert(aoe_building_construction_percent_ror(&b,&percent));
        assert(percent == INT16_MIN);
        b.state = 2; b.elapsed_build_time = NAN;
        assert(aoe_building_construction_percent_ror(&b,&percent));
        assert(percent == 100);
        b.state = 0; b.build_time = 0;
        assert(aoe_building_construction_percent_ror(&b,&percent));
        assert(percent == 100);
        b.build_time = 8000; b.elapsed_build_time = NAN; percent = 77;
        assert(!aoe_building_construction_percent_ror(&b,&percent));
        assert(percent == 77);
    }
    return 0;
}
