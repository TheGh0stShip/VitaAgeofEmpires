#include "building_update.h"
#include "research_action.h"
#include "train_action.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>

typedef struct {
    AoeBuildingUpdate b;
    AoeActionList list;
    AoeActionList live_list;
    AoeActionNode live_node;
    AoeTrainAction live_action;
    AoeBuildingQueueEntry entries[3];
    AoeBuildingProductionOps ops;
    AoeActionTickOps tick;
    float resources[33];
    uint32_t count;
    int events[40], n, null_create;
    uint8_t action_result;
    int action;
} Fixture;
static void event(Fixture *f,int e) { assert(f->n<40); f->events[f->n++]=e; }
static void *create(void *p,AoeBuildingUpdate *b,int16_t id) {
    Fixture *f=p;assert(b->queue_status==-1 && id==10);event(f,3);
    return f->null_create?NULL:&f->action;
}
static uint8_t update(void *p,void *action) {
    Fixture *f=p;assert(action==&f->action);event(f,5);return f->action_result;
}
static void waiting(void *p,void *a) { (void)p;(void)a;assert(0); }
static void notify(void *p,uint32_t e,int16_t id,uint32_t reason) {
    Fixture *f=p;assert(id==7);
    if(e==125) {
        assert(f->b.queue_status==4 && f->count==0); /* notify before INC */
        assert(reason==32 || reason==4);event(f,1000+reason);
    } else { assert(e==124 && reason==0);event(f,7); }
}
static void refund_one(void *p,int16_t id) {
    Fixture *f=p;assert(id==10 || id==20 || id==30 || id==99);event(f,8);
}
static void cancel_action(void *p,void *action) {
    Fixture *f=p;assert(action==&f->action && f->list.head);event(f,9);
}
static void destroy(void *p,void *a) {
    Fixture *f=p;assert(a==&f->action && !f->list.head);event(f,6);
}
static void action_state(void *p,void *a,uint8_t state) {
    Fixture *f=p;assert(a==&f->action && state==1 && f->list.head);event(f,4);
}
static void init(Fixture *f) {
    memset(f,0,sizeof(*f));
    f->tick=(AoeActionTickOps){f,update,waiting};
    f->ops=(AoeBuildingProductionOps){f,refund_one,cancel_action,create,
                                      &f->tick,notify};
    f->list=(AoeActionList){&f->b.state,NULL,{f,destroy,action_state}};
    f->entries[0]=(AoeBuildingQueueEntry){10,1};
    f->entries[1]=(AoeBuildingQueueEntry){20,2};
    f->entries[2]=(AoeBuildingQueueEntry){30,3};
    f->b.state=2; f->b.queue_status=-1; f->b.owner_id=7;
    f->b.queue_enabled=1; f->b.queue_count=3;
    f->b.queue_entries=f->entries; f->b.queue_capacity=3;
    f->b.queue_actions=&f->list;
    f->live_list=(AoeActionList){&f->b.state,NULL,{0}};
    f->b.live_actions=&f->live_list;
    f->resources[11]=10; f->resources[32]=50; f->resources[4]=5;
    f->b.owner_resources=(AoePlayerResources){33,f->resources};
}
static int run(Fixture *f) { return aoe_building_production_ror(&f->b,&f->count,&f->ops); }
static uint8_t parent_update(void *p,AoeBuildingUpdate *b) {
    Fixture *f=p;assert(b==&f->b && b->history_f0==123);
    b->current_1b0=456;return 0x71;
}
static void expect(Fixture *f,const int *events,int n) {
    assert(f->n==n);for(int i=0;i<n;++i)assert(f->events[i]==events[i]); f->n=0;
}
int main(void) {
    Fixture f;
    for(int result=0;result<=4;++result) {
        init(&f);f.action_result=result;assert(run(&f));
        const int finished[]={3,4,5,6,7}, pending[]={3,4,5};
        if(result>=1 && result<=3) {
            expect(&f,finished,5);assert(f.b.queue_count==2 && f.b.queue_revision==1);
            assert(f.entries[0].unit_id==20 && f.entries[1].unit_id==30);
            assert(f.entries[2].unit_id==30); /* stale tail retained */
        } else {
            expect(&f,pending,3);assert(f.b.queue_count==3 && f.list.head);
            assert(aoe_action_list_clear(&f.list));
        }
    }
    init(&f); f.b.queue_enabled=0;f.b.queue_status=4;f.count=1;
    f.b.live_actions=NULL; /* original returns before querying +0x184 */
    assert(run(&f) && f.count==0 && f.b.queue_status==-1 && !f.n);
    init(&f);f.b.queue_count=0;f.b.queue_status=9;assert(run(&f));
    assert(f.b.queue_status==9 && !f.n);
    {
        int16_t type=9,id=9,progress=9,unknown=77;
        init(&f);
        assert(!aoe_building_active_action_query_ror(
            &f.b,&type,&id,&progress));
        assert(type==0 && id==-1 && progress==0);
        f.live_node=(AoeActionNode){&unknown,NULL};
        f.live_list.head=&f.live_node;
        assert(!aoe_building_active_action_query_ror(
            &f.b,&type,&id,&progress));
        assert(type==77 && id==-1 && progress==0);
    }
    for(int code=102;code<=103;++code) {
        init(&f);
        if(code==102) {
            f.live_action=(AoeTrainAction){.action_type=102,.state=1};
            f.live_node=(AoeActionNode){&f.live_action,NULL};
        } else {
            static AoeResearchAction research;
            research=(AoeResearchAction){.action_type=103,.state=1};
            f.live_node=(AoeActionNode){&research,NULL};
        }
        f.live_list.head=&f.live_node;assert(run(&f));
        const int cancelled[]={8,7,8,7,8,7,8,7,8,7,8,7};
        expect(&f,cancelled,12);
        assert(f.b.queue_count==0 && !f.list.head);
    }
    for(int room=0;room<2;++room) {
        init(&f);if(room)f.resources[4]=0;else f.resources[11]=50;
        assert(run(&f) && f.count==1 && f.b.queue_status==4);
        assert(f.events[0]==(room?1004:1032));f.n=0;
        assert(run(&f) && f.count==1 && f.n==0); /* no repeated warning */
        f.resources[11]=10;f.resources[4]=5;f.n=0;
        f.action_result=1;assert(run(&f));assert(f.count==0 && f.b.queue_status==-1);
    }
    init(&f);f.null_create=1;assert(run(&f));
    const int no_alloc[]={3,7};expect(&f,no_alloc,2);assert(!f.list.head);
    assert(f.b.queue_count==2); /* empty list returns 1, then consumes queue */
    init(&f);assert(aoe_action_list_push(&f.list,&f.action)); f.n=0;
    f.resources[11]=50;f.resources[4]=0;f.action_result=3;
    assert(run(&f));const int existing[]={5,6,7};expect(&f,existing,3);
    /* Existing actions bypass resource gates, including full population. */
    init(&f);f.resources[4]=NAN;assert(!run(&f) && !f.list.head);
    init(&f);f.entries[0].quantity=2;f.b.queue_revision=255;
    assert(aoe_building_queue_consume_ror(&f.b,&f.ops));
    assert(f.entries[0].quantity==1 && f.b.queue_count==3 && f.b.queue_revision==0);
    f.entries[0].quantity=INT16_MIN;
    assert(aoe_building_queue_consume_ror(&f.b,&f.ops));
    assert(f.entries[0].quantity==INT16_MAX && f.b.queue_count==3);
    f.b.queue_count=0; f.n=0;
    assert(aoe_building_queue_consume_ror(&f.b,&f.ops) && !f.n);
    f.b.queue_count=4;assert(!aoe_building_queue_consume_ror(&f.b,&f.ops));
    {
        AoeBuildingQueueCancelOps cancel_ops={&f,refund_one,cancel_action,notify};
        init(&f); f.entries[0]=(AoeBuildingQueueEntry){10,2};
        f.entries[1]=(AoeBuildingQueueEntry){20,3};
        f.entries[2]=(AoeBuildingQueueEntry){10,4};
        f.b.queue_status=4; f.count=1; f.b.queue_revision=255;
        assert(aoe_building_queue_cancel_ror(&f.b,10,2,&f.count,&cancel_ops));
        const int tail_refund[]={8,8,7}; expect(&f,tail_refund,3);
        assert(f.entries[2].quantity==2 && f.b.queue_count==3);
        assert(f.b.queue_revision==0 && f.b.queue_status==4 && f.count==1);
        assert(aoe_building_queue_cancel_ror(&f.b,99,1,&f.count,&cancel_ops));
        const int miss[]={7}; expect(&f,miss,1);
        assert(f.b.queue_count==3 && f.b.queue_revision==0);
        assert(aoe_building_queue_cancel_ror(&f.b,10,2,&f.count,&cancel_ops));
        expect(&f,tail_refund,3); assert(f.b.queue_count==2);
        assert(f.entries[0].unit_id==10 && f.entries[1].unit_id==20);
        assert(aoe_action_list_push(&f.list,&f.action)); f.n=0;
        assert(aoe_building_queue_cancel_ror(&f.b,10,9,&f.count,&cancel_ops));
        const int head_refund[]={8,8,9,6,7}; expect(&f,head_refund,5);
        assert(f.b.queue_count==1 && f.entries[0].unit_id==20);
        assert(f.b.queue_status==-1 && f.count==0 && !f.list.head);
        f.b.queue_count=0; f.n=0;
        assert(aoe_building_queue_cancel_ror(&f.b,20,1,&f.count,&cancel_ops));
        assert(!f.n);
        assert(!aoe_building_queue_cancel_ror(&f.b,20,-1,&f.count,&cancel_ops));
    }
    /* Exercise the real update -> production -> pop -> consume composition. */
    init(&f); f.action_result=1;
    DatUnit unit_template={.present=1,.unit_class=3};
    f.b.unit_template=&unit_template;
    AoeBuildingUpdateOps update_ops={&f,NULL,parent_update,1.0f,&f.ops,&f.count};
    uint8_t result=0;
    assert(aoe_building_update_ror(&f.b,123,&update_ops,&result));
    assert(result==0x71 && f.b.history_e4==456 && f.b.queue_count==2);
    assert(!f.list.head && f.b.queue_revision==1);
    return 0;
}
