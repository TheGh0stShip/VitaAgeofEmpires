#include "action_list.h"
#include "building_update.h"
#include <assert.h>
#include <stddef.h>

typedef struct {
    AoeActionList *list;
    AoeBuildingUpdate *building;
    int events[32], used;
    int change_owner;
    void *insert_during_destroy;
    uint8_t tick_result;
    int tick_mutation;
} Trace;
static void destroy(void *context, void *action) {
    Trace *t=context;
    int id=*(int *)action;
    /* The removed node is already absent while its action is destroyed. */
    assert(!t->list->head || t->list->head->action != action);
    t->events[t->used++]=id*10+9;
    if (t->building) {
        assert(t->building->queue_count==0 && t->building->queue_status==-1);
        assert(t->building->queue_revision==255); /* increment after clear */
    }
    if (t->change_owner) *(uint8_t *)t->list->owner_state=3;
    if (t->insert_during_destroy) {
        void *insert=t->insert_during_destroy;
        t->insert_during_destroy=NULL;
        assert(aoe_action_list_push(t->list,insert));
    }
}
static void set_state(void *context, void *action, uint8_t state) {
    Trace *t=context;
    AoeActionNode *n=t->list->head;
    while (n && n->action!=action) n=n->next;
    assert(n); /* new action is linked before state 1 */
    t->events[t->used++]=*(int *)action*10+state;
}
static void expect(Trace *t, const int *events, size_t n) {
    assert(t->used==(int)n);
    for (size_t i=0;i<n;++i) assert(t->events[i]==events[i]);
    t->used=0;
}
static uint8_t tick_head(void *context,void *action) {
    Trace *t=context;t->events[t->used++]=*(int *)action*10+7;
    if(t->tick_mutation==1) assert(aoe_action_list_pop(t->list));
    if(t->tick_mutation==2) assert(aoe_action_list_clear(t->list));
    return t->tick_result;
}
static void tick_waiting(void *context,void *action) {
    Trace *t=context;t->events[t->used++]=*(int *)action*10+8;
}
static void add_id(void *c, void *a) { *(int *)c = *(int *)c * 10 + *(int *)a; }

static void count_clean(void *c) { ++*(int *)c; }

int main(void) {
    uint8_t state=2;
    int a=1,b=2,c=3;
    Trace t={0};
    AoeActionList list={&state,NULL,{&t,destroy,set_state}};
    t.list=&list;
    assert(aoe_action_list_append(&list,&a));
    assert(aoe_action_list_append(&list,&b));
    assert(aoe_action_list_push(&list,&c));
    assert(list.head->action==&c && list.head->next->action==&a);
    {
        int sum=0;
        assert(aoe_action_list_head_ror(&list)==&c);
        aoe_action_list_each_ror(&list,add_id,&sum);
        assert(sum==312);
    }
    assert(aoe_action_list_clear(&list));
    const int normal[]={11,21,31,39,10,19,20,29};
    expect(&t,normal,sizeof(normal)/sizeof(*normal));
    assert(!list.head && aoe_action_list_pop(&list));
    assert(aoe_action_list_head_ror(&list)==NULL);
    assert(!aoe_action_list_push(&list,NULL));
    state=3;
    assert(aoe_action_list_push(&list,&a));
    assert(aoe_action_list_append(&list,&b));
    assert(aoe_action_list_push(&list,NULL));
    const int reject[]={19,29}; expect(&t,reject,2);
    assert(!list.head);
    state=2;
    assert(aoe_action_list_append(&list,&a));
    assert(aoe_action_list_append(&list,&b));
    t.change_owner=1;
    assert(aoe_action_list_clear(&list));
    const int changed[]={11,21,19,29}; expect(&t,changed,4);
    t.change_owner=0;
    state=2;
    assert(aoe_action_list_append(&list,&a));
    assert(aoe_action_list_append(&list,&b));
    t.insert_during_destroy=&c;
    assert(aoe_action_list_clear(&list));
    const int reentrant[]={11,21,19,31,30,39,20,29};
    expect(&t,reentrant,8);
    AoeActionTickOps tick={&t,tick_head,tick_waiting};
    uint8_t result=99;
    assert(aoe_action_list_update_ror(&list,&tick,&result) && result==1 && !t.used);
    for(int mode=0;mode<3;++mode) {
        t.tick_result=mode==0?1:mode==1?2:255;
        assert(aoe_action_list_append(&list,&a));
        assert(aoe_action_list_append(&list,&b));
        assert(aoe_action_list_append(&list,&c));t.used=0;
        assert(aoe_action_list_update_ror(&list,&tick,&result));
        assert(result==t.tick_result);
        if(mode==0) {
            const int sequence[]={17,28,38,19,20};expect(&t,sequence,5);
            /* Production caller pops again for result 1. */
            assert(aoe_action_list_pop(&list));
            const int second[]={29,30};expect(&t,second,2);
            assert(list.head->action==&c);
        } else { const int sequence[]={17,28,38};expect(&t,sequence,3); }
        assert(aoe_action_list_clear(&list));t.used=0;
    }
    assert(aoe_action_list_append(&list,&a));
    assert(aoe_action_list_append(&list,&b));
    assert(aoe_action_list_append(&list,&c));t.used=0;
    t.tick_mutation=1;t.tick_result=1;
    assert(aoe_action_list_update_ror(&list,&tick,&result));
    const int replaced[]={17,19,20,38,29,30};expect(&t,replaced,6);
    assert(list.head->action==&c);assert(aoe_action_list_clear(&list));t.used=0;
    assert(aoe_action_list_append(&list,&a));t.used=0;
    t.tick_mutation=2;t.tick_result=3;
    assert(aoe_action_list_update_ror(&list,&tick,&result));
    const int gone[]={17,19};expect(&t,gone,2);
    assert(!list.head && result==3);
    tick.update_waiting=NULL;result=99;
    assert(!aoe_action_list_update_ror(&list,&tick,&result) && result==99);
    AoeBuildingUpdate building={0};
    building.state=2; list.owner_state=&building.state;
    building.queue_actions=&list;
    assert(aoe_action_list_append(&list,&a));
    assert(aoe_action_list_append(&list,&b));
    t.used=0; t.building=&building;
    building.state=3; building.queue_status=4;
    building.queue_count=8; building.queue_revision=255;
    uint32_t count=0;
    assert(!aoe_building_terminal_cleanup_ror(&building,NULL));
    assert(building.queue_count==8 && list.head && !t.used);
    assert(aoe_building_terminal_cleanup_ror(&building,&count));
    assert(count==UINT32_MAX && building.queue_revision==0 && !list.head);
    const int cleared[]={19,29}; expect(&t,cleared,2);
    assert(aoe_building_terminal_cleanup_ror(&building,NULL));
    assert(building.queue_revision==1 && count==UINT32_MAX);
    building.queue_status=7; building.queue_count=5;
    assert(aoe_building_terminal_cleanup_ror(&building,NULL));
    assert(building.queue_status==-1 && building.queue_count==0);
    list.ops.destroy=NULL;
    assert(!aoe_action_list_clear(&list));
    assert(!aoe_building_terminal_cleanup_ror(&building,&count));
    {
        int x=7, y=8, cleaned=0;
        t.used=0;
        t.building=NULL;
        state=2;
        list.owner_state=&state;
        list.ops.destroy=destroy;
        assert(aoe_unit_push_action_ror(&list,NULL) && !list.head);
        assert(aoe_unit_append_action_ror(&list,&x) && aoe_unit_push_action_ror(&list,&y));
        assert(list.head->action==&y && list.head->next->action==&x);
        assert(aoe_unit_append_action_ror(&list,NULL));
        aoe_unit_cleanup_actions_ror(&list,count_clean,&cleaned);
        assert(!list.head && cleaned==1);
        aoe_unit_cleanup_actions_ror(NULL,count_clean,&cleaned);
        assert(cleaned==2);
    }
    return 0;
}
