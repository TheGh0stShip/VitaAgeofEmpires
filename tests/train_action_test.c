#include "train_action.h"
#include <assert.h>
#include <math.h>

typedef struct {
    int events[16],n,success;
    uint8_t target_states[3];
    int spawn_calls,accept_at,create_calls,create_ok;
    float first_x,first_y,ring2_x,ring2_y;
} Trace;
typedef struct { int32_t id; } Target;
static int charge(void *p,AoeTrainAction *a) {
    Trace *t=p;assert(a->state==4);t->events[t->n++]=7;return t->success;
}
static void notify(void *p,uint32_t e,int16_t owner,int16_t unit,int32_t x,int32_t y) {
    Trace *t=p;assert(owner==2 && unit==83);t->events[t->n++]=3;
    if(t->success) assert(e==102 && x==10 && y==-3);
    else assert(e==104 && x==0 && y==0);
}
static void failed(void *p,AoeTrainAction *a) {
    Trace *t=p;assert(a->request_id==-1);t->events[t->n++]=4;
}
static void completed(void *p,AoeTrainAction *a,int32_t id) {
    Trace *t=p;assert(a->building_id==55 && id==4321);t->events[t->n++]=5;
}
static void signal(void *p,AoeTrainAction *a,uint32_t code) {
    Trace *t=p;assert(a->state==1 && code==(t->success?506u:505u));
    t->events[t->n++]=6;
}
static void signal_failure(void *p,AoeTrainAction *a) {
    Trace *t=p;assert(a->state==4);t->events[t->n++]=6;
}
static uint8_t target_state(void *p,const void *target) {
    Trace *t=p;int id=*(const int *)target;t->events[t->n++]=10+id;
    return t->target_states[id];
}
static int32_t target_id(void *p,const void *target) {
    Trace *t=p;const Target *v=target;t->events[t->n++]=40;return v->id;
}
static void detach_target(void *p,void *target,AoeTrainAction *a) {
    Trace *t=p;assert(target==&t->target_states[0]);t->events[t->n++]=41;
    if(t->success==2)a->secondary_target=NULL;
    if(t->success==3)a->primary_target=(void *)(uintptr_t)t->target_states[1];
}
static void attach_target(void *p,void *target,AoeTrainAction *a,int16_t type) {
    Trace *t=p;Target *v=target;assert(type==102);
    assert((a->primary_target==target && a->primary_target_id==v->id) ||
           (a->secondary_target==target && a->secondary_target_id==v->id));
    t->events[t->n++]=42;
}
static void nested_destroy(void *p,void *action) {
    Trace *t=p;assert(*(int *)action==9);t->events[t->n++]=50;
}
static void nested_state(void *p,void *action,uint8_t state) {
    Trace *t=p;assert(*(int *)action==9 && state==1);t->events[t->n++]=51;
}
static int spawn_blocked(void *p,AoeTrainAction *a,float x,float y) {
    Trace *t=p;assert(a->candidate_x==x && a->candidate_y==y);
    ++t->spawn_calls;
    if(t->spawn_calls==1){t->first_x=x;t->first_y=y;}
    if(t->spawn_calls==61){t->ring2_x=x;t->ring2_y=y;}
    return t->accept_at<=0 || t->spawn_calls!=t->accept_at;
}
static int spawn_create(void *p,AoeTrainAction *a,float x,float y,int32_t *id) {
    Trace *t=p;assert(t->spawn_calls==t->accept_at);
    assert(x==a->candidate_x && y==a->candidate_y);++t->create_calls;
    *id=4321;return t->create_ok;
}
int main(void) {
    DatUnit building={0},unit={0};building.work_rate=2;unit.train_time=5;
    Trace t={0};
    AoeTrainSpawnOps spawn_ops={&t,spawn_blocked,spawn_create};
    AoeRng runtime_rng;aoe_rng_seed(&runtime_rng,1);
    AoeTrainActionOps ops={&t,&runtime_rng,&spawn_ops,notify,failed,completed,signal};
    float resources[8]={0};AoePlayerResources owner={8,resources};
    unit.type=70;
    unit.costs[0]=(DatUnitCost){1,50,1};
    unit.costs[1]=(DatUnitCost){2,30,256}; /* low paid byte is zero */
    unit.costs[2]=(DatUnitCost){3,-5,-1};
    AoeTrainAction a={.action_type=102,.state=6,.unit_id=83,.progress=0,
        .request_id=-1,.prepaid=0,.building_template=&building,
        .trained_template=&unit,.owner_resources=&owner,.owner_id=2,
        .building_id=55,.building_x=10.75f,.building_y=-3.75f};
    uint8_t result=99;
    assert(aoe_train_action_update_ror(&a,2,0,&ops,&result));
    assert(result==0 && a.progress==4 && a.state==6 && !t.n);
    assert(aoe_train_action_update_ror(&a,.5f,0,&ops,&result));
    assert(result==1 && a.progress==5 && a.state==1 && t.n==3);
    const int fail_order[]={3,4,6};
    assert(t.n==3);for(int i=0;i<3;++i) assert(t.events[i]==fail_order[i]);
    assert(resources[1]==50 && resources[2]==0 && resources[3]==-5);
    assert(aoe_train_action_update_ror(&a,NAN,0,NULL,&result) && result==1);
    assert(t.n==3); /* finished state does not refund/spawn twice */
    t=(Trace){0};t.success=1;t.accept_at=1;t.create_ok=1;
    aoe_rng_seed(&runtime_rng,1);a.state=6;a.progress=4;
    assert(aoe_train_action_update_ror(&a,1,0,&ops,&result));
    const int success_order[]={3,5,6};assert(t.n==3);
    for(int i=0;i<3;++i) assert(t.events[i]==success_order[i]);
    t.n=0;a.state=6;a.progress=9999;unit.train_time=1500;
    assert(aoe_train_action_update_ror(&a,NAN,1,&ops,&result));
    assert(a.progress==1000 && result==0 && !t.n);
    unit.train_time=1000;
    t.spawn_calls=0;t.create_calls=0;
    assert(aoe_train_action_update_ror(&a,0,1,&ops,&result));
    assert(result==1 && t.n==3);
    a.state=5;t.n=0;assert(aoe_train_action_update_ror(&a,NAN,0,NULL,&result));
    assert(result==0 && !t.n);
    a.state=6;a.progress=4;result=99;
    assert(!aoe_train_action_update_ror(&a,NAN,0,&ops,&result));
    assert(a.progress==4 && a.state==6 && result==99);
    ops.spawn=NULL;assert(!aoe_train_action_update_ror(&a,1,0,&ops,&result));
    ops.spawn=&spawn_ops;

    AoeTrainActivationOps activate={&t,charge,failed,signal_failure};
    t=(Trace){0};a.state=4;a.prepaid=0;a.unit_id=83;a.trained_template=&unit;
    assert(aoe_train_action_activate_ror(&a,0,NULL));
    assert(a.state==4 && !t.n);
    a.unit_id=-2;
    assert(aoe_train_action_activate_ror(&a,1,&activate));
    assert(a.state==4 && t.n==1 && t.events[0]==4);
    t.n=0;a.unit_id=83;a.trained_template=NULL;
    assert(aoe_train_action_activate_ror(&a,1,&activate));
    assert(a.state==4 && t.n==1 && t.events[0]==4);
    t.n=0;a.trained_template=&unit;a.prepaid=1;
    assert(aoe_train_action_activate_ror(&a,1,&activate));
    assert(a.state==6 && !t.n);
    a.state=4;a.prepaid=0;t.success=1;
    assert(aoe_train_action_activate_ror(&a,1,&activate));
    assert(a.state==6 && t.n==1 && t.events[0]==7);
    a.state=4;t=(Trace){0};
    assert(aoe_train_action_activate_ror(&a,1,&activate));
    const int activation_failure[]={7,4,6};assert(t.n==3);
    for(int i=0;i<3;++i)assert(t.events[i]==activation_failure[i]);
    assert(a.state==1);
    a.state=4;activate.charge=NULL;
    assert(!aoe_train_action_activate_ror(&a,1,&activate) && a.state==4);
    resources[1]=resources[2]=resources[3]=0;
    unit.type=69;assert(aoe_train_action_refund_ror(&a));
    assert(resources[1]==0);
    unit.type=70;assert(aoe_train_action_refund_ror(&a));
    assert(resources[1]==50 && resources[2]==0 && resources[3]==-5);
    owner.count=2;resources[1]=0;
    assert(!aoe_train_action_refund_ror(&a) && resources[1]==0);

    Target target1={1},target2={2};uint8_t waiting_result=0;
    void *positive[100]={0};
    positive[1]=positive[2]=positive[41]=&t.target_states[0];
    AoeLiveUnitRegistry registry={positive,100,NULL,0};
    a.unit_registry=&registry;
    AoeTrainTargetOps target_ops={&t,target_id,detach_target,attach_target};
    AoeTrainWaitingOps waiting={&t,target_state,&target_ops};
    t=(Trace){0};
    assert(aoe_train_action_waiting_ror(&a,NULL,&waiting_result));
    assert(waiting_result==1 && !t.n);
    a.primary_target=&target1;a.primary_target_id=1;
    a.secondary_target=&target2;a.secondary_target_id=2;
    t.target_states[1]=6;t.target_states[2]=7;
    assert(aoe_train_action_waiting_ror(&a,&waiting,&waiting_result));
    const int one_dead[]={11,12,41};assert(t.n==3);
    for(int i=0;i<3;++i)assert(t.events[i]==one_dead[i]);
    assert(a.primary_target==&target1 && !a.secondary_target);
    t.n=0;a.secondary_target=&target2;a.secondary_target_id=2;
    t.target_states[1]=7;t.success=2;
    assert(aoe_train_action_waiting_ror(&a,&waiting,&waiting_result));
    const int reread[]={11,41};assert(t.n==2);
    for(int i=0;i<2;++i)assert(t.events[i]==reread[i]);
    assert(!a.primary_target && !a.secondary_target);
    a.primary_target=&target1;a.primary_target_id=1;
    target_ops.detach=NULL;waiting_result=0;
    assert(!aoe_train_action_waiting_ror(&a,&waiting,&waiting_result));
    assert(a.primary_target==&target1 && waiting_result==0);

    Target old={41},new_target={77};target_ops.detach=detach_target;
    t=(Trace){0};a.action_type=102;a.primary_target=&old;a.primary_target_id=41;
    assert(aoe_train_action_set_primary_ror(&a,&new_target,&target_ops));
    const int replace[]={41,40,42};assert(t.n==3);
    for(int i=0;i<3;++i)assert(t.events[i]==replace[i]);
    assert(a.primary_target==&new_target && a.primary_target_id==77);
    t.n=0;assert(aoe_train_action_set_primary_ror(&a,&new_target,NULL) && !t.n);
    assert(aoe_train_action_set_primary_ror(&a,NULL,&target_ops));
    assert(!a.primary_target && a.primary_target_id==-1);
    /* Registry miss skips detach but still replaces and attaches. */
    t.n=0;a.secondary_target=&old;a.secondary_target_id=99;
    assert(aoe_train_action_set_secondary_ror(&a,&new_target,&target_ops));
    const int miss[]={40,42};assert(t.n==2);
    for(int i=0;i<2;++i)assert(t.events[i]==miss[i]);
    /* A detach callback can satisfy the second pointer comparison. */
    t=(Trace){0};t.success=3;t.target_states[1]=(uint8_t)(uintptr_t)&new_target;
    /* Use a small sentinel pointer that round-trips through the test byte. */
    void *sentinel=(void *)(uintptr_t)t.target_states[1];
    a.primary_target=&old;a.primary_target_id=41;
    assert(aoe_train_action_set_primary_ror(&a,sentinel,&target_ops));
    assert(t.n==1 && t.events[0]==41);
    assert(a.primary_target==sentinel);
    a.primary_target=&old;a.primary_target_id=41;target_ops.detach=NULL;
    assert(!aoe_train_action_set_primary_ror(&a,NULL,&target_ops));
    assert(a.primary_target==&old && a.primary_target_id==41);

    target_ops.detach=detach_target;t=(Trace){0};
    AoeTrainActionCreate create={
        .unit_id=83,.request_id=-1,.prepaid=1,
        .building_template=&building,.trained_template=&unit,
        .owner_resources=&owner,.owner_id=2,.building_id=55,
        .building_x=10.75f,.building_y=-3.75f,
        .owned_action_ops={&t,nested_destroy,nested_state},
        .unit_registry=&registry
    };
    AoeTrainAction *made=aoe_train_action_create_ror(&create);
    assert(made && made->state==0 && made->action_type==102);
    assert(made->unit_id==83 && made->request_id==-1 && made->prepaid==1);
    assert(made->progress==0 && !made->primary_target && !made->secondary_target);
    assert(made->primary_target_id==-1 && made->secondary_target_id==-1);
    assert(made->owned_actions && made->owned_actions->owner_state==&made->state);
    int nested=9;assert(aoe_action_list_push(made->owned_actions,&nested));
    assert(t.n==1 && t.events[0]==51);t.n=0;
    made->primary_target=&old;made->primary_target_id=41;
    made->secondary_target=&old;made->secondary_target_id=41;
    assert(aoe_train_action_destroy_ror(made,&target_ops));
    const int teardown[]={41,41,50};assert(t.n==3);
    for(int i=0;i<3;++i)assert(t.events[i]==teardown[i]);
    assert(aoe_train_action_destroy_ror(NULL,NULL));
    create.owned_action_ops.destroy=NULL;
    assert(!aoe_train_action_create_ror(&create));

    building.size[0]=2.0f;building.size[1]=3.0f;
    unit.size[0]=.5f;unit.size[1]=.25f;
    AoeRng rng;int32_t created=0;uint8_t found=0;
    t=(Trace){0};t.accept_at=1;t.create_ok=1;aoe_rng_seed(&rng,1);
    assert(aoe_train_action_spawn_ror(&a,&rng,&spawn_ops,&created,&found));
    assert(found==1 && created==4321 && t.spawn_calls==1 && t.create_calls==1);
    assert(t.first_x==11.069198608398438f && t.first_y==-0.49000000953674316f);
    AoeRng expected;aoe_rng_seed(&expected,1);aoe_rng_next(&expected);aoe_rng_next(&expected);
    assert(rng.state==expected.state);
    t=(Trace){0};aoe_rng_seed(&rng,1);found=1;created=99;
    assert(aoe_train_action_spawn_ror(&a,&rng,&spawn_ops,&created,&found));
    assert(found==0 && created==99 && t.spawn_calls==180 && !t.create_calls);
    assert(t.ring2_x!=t.first_x || t.ring2_y!=t.first_y);
    t=(Trace){0};t.accept_at=3;t.create_ok=0;aoe_rng_seed(&rng,1);found=0;
    assert(!aoe_train_action_spawn_ror(&a,&rng,&spawn_ops,&created,&found));
    assert(found==0 && t.spawn_calls==3 && t.create_calls==1);
    spawn_ops.blocked=NULL;
    assert(!aoe_train_action_spawn_ror(&a,&rng,&spawn_ops,&created,&found));
    return 0;
}
