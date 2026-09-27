#include "building_update.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>

typedef struct {
    AoeBuildingUpdate b;
    AoeActionList list;
    AoeBuildingQueueEntry entries[3];
    AoeBuildingProductionOps ops;
    AoeActionTickOps tick;
    float resources[33];
    uint32_t count;
    int events[40], n, query_result, code, null_create, stuck_cancel;
    uint8_t action_result;
    int action;
} Fixture;
static void event(Fixture *f,int e) { assert(f->n<40); f->events[f->n++]=e; }
static int query(void *p,AoeBuildingUpdate *b,int16_t *code) {
    Fixture *f=p; assert(b==&f->b);event(f,1);*code=f->code;return f->query_result;
}
static void cancel(void *p,AoeBuildingUpdate *b,int16_t id,int16_t quantity) {
    Fixture *f=p;assert(id==b->queue_entries[0].unit_id && quantity==-1);
    event(f,2); if (!f->stuck_cancel) --b->queue_count;
}
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
static void destroy(void *p,void *a) {
    Fixture *f=p;assert(a==&f->action && !f->list.head);event(f,6);
}
static void action_state(void *p,void *a,uint8_t state) {
    Fixture *f=p;assert(a==&f->action && state==1 && f->list.head);event(f,4);
}
static void init(Fixture *f) {
    memset(f,0,sizeof(*f));
    f->tick=(AoeActionTickOps){f,update,waiting};
    f->ops=(AoeBuildingProductionOps){f,query,cancel,create,&f->tick,notify};
    f->list=(AoeActionList){&f->b.state,NULL,{f,destroy,action_state}};
    f->entries[0]=(AoeBuildingQueueEntry){10,1};
    f->entries[1]=(AoeBuildingQueueEntry){20,2};
    f->entries[2]=(AoeBuildingQueueEntry){30,3};
    f->b.state=2; f->b.queue_status=-1; f->b.owner_id=7;
    f->b.queue_enabled=1; f->b.queue_count=3;
    f->b.queue_entries=f->entries; f->b.queue_capacity=3;
    f->b.queue_actions=&f->list;
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
        const int finished[]={1,3,4,5,6,7}, pending[]={1,3,4,5};
        if(result>=1 && result<=3) {
            expect(&f,finished,6);assert(f.b.queue_count==2 && f.b.queue_revision==1);
            assert(f.entries[0].unit_id==20 && f.entries[1].unit_id==30);
            assert(f.entries[2].unit_id==30); /* stale tail retained */
        } else {
            expect(&f,pending,4);assert(f.b.queue_count==3 && f.list.head);
            assert(aoe_action_list_clear(&f.list));
        }
    }
    init(&f); f.b.queue_enabled=0;f.b.queue_status=4;f.count=1;
    assert(run(&f) && f.count==0 && f.b.queue_status==-1 && !f.n);
    init(&f);f.b.queue_count=0;f.b.queue_status=9;assert(run(&f));
    assert(f.b.queue_status==9 && !f.n);
    for(int code=102;code<=103;++code) {
        init(&f);f.query_result=1;f.code=code;assert(run(&f));
        const int cancelled[]={1,2,2,2};expect(&f,cancelled,4);
        assert(f.b.queue_count==0 && !f.list.head);
    }
    init(&f);f.query_result=1;f.code=102;f.stuck_cancel=1;assert(!run(&f));
    for(int room=0;room<2;++room) {
        init(&f);if(room)f.resources[4]=0;else f.resources[11]=50;
        assert(run(&f) && f.count==1 && f.b.queue_status==4);
        assert(f.events[1]==(room?1004:1032));f.n=0;
        assert(run(&f) && f.count==1 && f.n==1); /* no repeated warning */
        f.resources[11]=10;f.resources[4]=5;f.n=0;
        f.action_result=1;assert(run(&f));assert(f.count==0 && f.b.queue_status==-1);
    }
    init(&f);f.null_create=1;assert(run(&f));
    const int no_alloc[]={1,3,7};expect(&f,no_alloc,3);assert(!f.list.head);
    assert(f.b.queue_count==2); /* empty list returns 1, then consumes queue */
    init(&f);assert(aoe_action_list_push(&f.list,&f.action)); f.n=0;
    f.resources[11]=50;f.resources[4]=0;f.action_result=3;
    assert(run(&f));const int existing[]={1,5,6,7};expect(&f,existing,4);
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
    /* Exercise the real update -> production -> pop -> consume composition. */
    init(&f); f.action_result=1;
    AoeBuildingUpdateOps update_ops={&f,NULL,parent_update,&f.ops,&f.count};
    uint8_t result=0;
    assert(aoe_building_update_ror(&f.b,123,&update_ops,&result));
    assert(result==0x71 && f.b.history_e4==456 && f.b.queue_count==2);
    assert(!f.list.head && f.b.queue_revision==1);
    return 0;
}
