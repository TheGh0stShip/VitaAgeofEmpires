#include "research_action.h"

#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct { char events[16]; int count; int16_t id; int wonder; } Trace;
static void apply(void *context,int16_t id) {
    Trace *trace=context; trace->events[trace->count++]='A'; trace->id=id;
}
static void rebuild(void *context) {
    Trace *trace=context; trace->events[trace->count++]='R';
}
static void wonder(void *context,int index) {
    Trace *trace=context; trace->events[trace->count++]='W';trace->wonder=index;
}

typedef struct {
    char log[16]; int n;
    int16_t reason; uint32_t event; int32_t x, y; uint32_t codes[4];
} ActionTrace;
static void a_notify(void *c, uint32_t e, int16_t o, int16_t id, int32_t x,
                     int32_t y) {
    ActionTrace *t = c; (void)o; (void)id;
    t->log[t->n++] = 'N'; t->event = e; t->x = x; t->y = y;
}
static void a_failed(void *c, const AoeResearchAction *a, int16_t r) {
    ActionTrace *t = c; (void)a;
    t->log[t->n++] = 'F'; t->reason = r;
}
static void a_completed(void *c, const AoeResearchAction *a) {
    ActionTrace *t = c; (void)a;
    t->log[t->n++] = 'C';
}
static void a_signal(void *c, const AoeResearchAction *a, uint32_t code) {
    ActionTrace *t = c; (void)a;
    t->codes[t->n % 4] = code;
    t->log[t->n++] = 'S';
}

/* 0x004eaf4c / 0x004eb100 / 0x004eab23 / 0x004eb27b and the type-0x67
   action (0x004b6200 / 0x004b6290 / 0x004b6390 / 0x004b60f0). */
static void manager_and_action(void) {
    DatTech data[2];
    DatTechs techs = {2, data, 0};
    AoeResearchProgressEntry entries[2] = {{0.0f, 1}, {0.0f, 0}};
    Trace trace = {{0}, 0, -1, -1};
    AoeResearchCompletionManagerRor m = {2, entries, &trace, apply, rebuild,
                                         wonder};
    float values[40] = {0};
    AoePlayerResources res = {40, values};
    AoeResearchOwnerRor owner = {&m, &techs, &res};
    int16_t reason = 7;
    int researching = -1;
    memset(data, 0, sizeof(data));
    /* 50 food + 40 gold, an unpaid wood cost; 60 seconds. */
    data[0].costs[0] = (DatTechCost){0, 50, 1};
    data[0].costs[1] = (DatTechCost){3, 40, 1};
    data[0].costs[2] = (DatTechCost){1, 500, 0};
    data[0].research_time = 60;
    data[1].costs[0] = (DatTechCost){-1, 0, 0};
    data[1].costs[1] = (DatTechCost){-1, 0, 0};
    data[1].costs[2] = (DatTechCost){-1, 0, 0};

    /* Unaffordable: the failing cost's type, then start reports -1. */
    values[0] = 49.0f; values[3] = 100.0f;
    assert(aoe_research_can_start_ror(&owner, 0, &reason) == 0 && reason == 0);
    assert(aoe_research_start_ror(&owner, 0, 1, &reason) == 0 &&
           reason == -1 && entries[0].state == 1);
    /* Not available (state 0) or out of range: -1. */
    reason = 7;
    assert(aoe_research_can_start_ror(&owner, 1, &reason) == 0 &&
           reason == -1);
    reason = 7;
    assert(aoe_research_can_start_ror(&owner, 2, &reason) == 0 &&
           reason == -1);

    /* Resource 29 >= 1: gold is a reserve covering the food shortfall;
       start takes the difference from gold and zeroes food. */
    values[29] = 1.0f;
    assert(aoe_research_can_start_ror(&owner, 0, &reason) == 1);
    assert(aoe_research_start_ror(&owner, 0, 1, &reason) == 1);
    assert(entries[0].state == 2 && values[0] == 0.0f &&
           values[3] == 100.0f - 1.0f - 40.0f && values[1] == 0.0f);
    /* Reserve exhausted: the gold cost itself fails first. */
    entries[0].state = 1; values[0] = 50.0f; values[3] = 39.0f;
    assert(aoe_research_can_start_ror(&owner, 0, &reason) == 0 && reason == 3);
    values[29] = 0.0f; values[3] = 40.0f;
    assert(aoe_research_can_start_ror(&owner, 0, &reason) == 1);

    /* Stop refunds paid costs only for a researching tech. */
    assert(aoe_research_start_ror(&owner, 0, 1, &reason) == 1);
    assert(values[0] == 0.0f && values[3] == 0.0f);
    entries[0].progress = 12.0f;
    assert(aoe_research_stop_ror(&owner, 0, 1) == 1);
    assert(values[0] == 50.0f && values[3] == 40.0f && entries[0].state == 1 &&
           entries[0].progress == 0.0f);
    assert(aoe_research_stop_ror(&owner, 0, 1) == 0 && values[0] == 50.0f);

    /* Progress continues while time > progress, completes at equality. */
    assert(aoe_research_start_ror(&owner, 0, 0, &reason) == 1 &&
           values[0] == 50.0f);
    assert(aoe_research_add_progress_ror(&owner, 0, 59.5f, &researching) &&
           researching == 1);
    assert(aoe_research_add_progress_ror(&owner, 0, 0.5f, &researching) &&
           researching == 0 && entries[0].state == 3 &&
           !memcmp(trace.events, "AR", 2));
    assert(aoe_research_add_progress_ror(&owner, 0, 1.0f, &researching) &&
           researching == 0 && entries[0].progress == 60.0f);

    /* Reset: done drops to 0 and rebuilds; state 0 is left alone. */
    trace.count = 0;
    assert(aoe_research_reset_ror(&m, 0) == 1 && entries[0].state == 0 &&
           trace.count == 1 && trace.events[0] == 'R');
    assert(aoe_research_reset_ror(&m, 0) == 0);

    /* The action: activation, update, completion. */
    {
        ActionTrace t;
        AoeResearchActionOps ops = {&t, owner, a_notify, a_failed,
                                    a_completed, a_signal, 0.05f, 0, 0};
        AoeResearchAction a;
        uint8_t r = 9;
        trace.count = 0;
        memset(&t, 0, sizeof(t));
        entries[0] = (AoeResearchProgressEntry){0.0f, 1};
        values[0] = 50.0f; values[3] = 40.0f;
        aoe_research_action_init_ror(&a, 0, 77);
        assert(a.action_type == 0x67 && a.state == 0 && a.request_id == 77);
        a.owner_index = 2; a.building_x = 10.9f; a.building_y = -3.5f;
        a.work_rate = 20.0f; /* one tick = 1.0 progress */
        assert(aoe_research_action_activate_ror(&a, 0, &ops) &&
               a.state == 0 && t.n == 0);
        assert(aoe_research_action_activate_ror(&a, 1, &ops) &&
               a.state == 6 && entries[0].state == 2 && values[0] == 0.0f);
        for (int i = 0; i < 59; ++i) {
            assert(aoe_research_action_update_ror(&a, &ops, &r) && r == 0);
        }
        assert(t.n == 0 && entries[0].progress == 59.0f);
        assert(aoe_research_action_update_ror(&a, &ops, &r) && r == 1);
        assert(a.state == 1 && entries[0].state == 3);
        assert(!memcmp(t.log, "NCS", 3) && t.event == 0x64 && t.x == 10 &&
               t.y == -3 && t.codes[2] == 0x1fa);
        assert(aoe_research_action_update_ror(&a, &ops, &r) && r == 1 &&
               t.n == 3);
        /* Destroying a state-1 action does nothing. */
        assert(aoe_research_action_destroy_ror(&a, &ops) && t.n == 3);

        /* Activation failure: owner +0x118(-1), signal 0x1f9, state 1. */
        memset(&t, 0, sizeof(t));
        entries[0] = (AoeResearchProgressEntry){0.0f, 1};
        values[0] = 0.0f; values[3] = 0.0f;
        aoe_research_action_init_ror(&a, 0, -1);
        assert(aoe_research_action_activate_ror(&a, 1, &ops) &&
               a.state == 1 && !memcmp(t.log, "FS", 2) && t.reason == -1 &&
               t.codes[1] == 0x1f9 && entries[0].state == 1);

        /* Cancel refunds; destroy keeps the resources spent. */
        memset(&t, 0, sizeof(t));
        values[0] = 50.0f; values[3] = 40.0f;
        aoe_research_action_init_ror(&a, 0, -1);
        assert(aoe_research_action_activate_ror(&a, 1, &ops) && a.state == 6);
        assert(aoe_research_action_cancel_ror(&a, &ops) && a.state == 1);
        assert(values[0] == 50.0f && values[3] == 40.0f &&
               entries[0].state == 1 && !memcmp(t.log, "NFS", 3) &&
               t.event == 0x65 && t.reason == -1);
        memset(&t, 0, sizeof(t));
        aoe_research_action_init_ror(&a, 0, -1);
        assert(aoe_research_action_activate_ror(&a, 1, &ops) && a.state == 6);
        ops.quiet_teardown = 1;
        assert(aoe_research_action_destroy_ror(&a, &ops) && t.n == 0);
        assert(values[0] == 0.0f && values[3] == 0.0f &&
               entries[0].state == 1);
        /* Fast research: 1000 per update. */
        values[0] = 50.0f; values[3] = 40.0f;
        aoe_research_action_init_ror(&a, 0, -1);
        ops.fast_research = 1;
        assert(aoe_research_action_activate_ror(&a, 1, &ops));
        assert(aoe_research_action_update_ror(&a, &ops, &r) && r == 1 &&
               entries[0].state == 3);
        assert(trace.count <= (int)sizeof(trace.events));
    }
}

int main(void) {
    manager_and_action();
    DatTech techs_data[3];
    DatTechs techs = {3,techs_data,0};
    AoeResearchProgressEntry entries[3] = {
        {45.0f,2},{0.0f,3},{999.0f,1}
    };
    AoeResearchProgressTable table = {3,entries,&techs};
    AoeResearchAction action = {103,0,0,&table,-1,-1,-1,0,0,0};
    int16_t id = -1, percent = -1;
    memset(techs_data,0,sizeof(techs_data));
    techs_data[0].research_time = 120;
    techs_data[1].research_time = 1;
    techs_data[2].research_time = 1;

    assert(aoe_research_action_progress_ror(&action,&id,&percent));
    assert(id == 0 && percent == 37);
    action.research_id = 1;
    assert(aoe_research_action_progress_ror(&action,&id,&percent));
    assert(id == 1 && percent == 100);
    action.research_id = 2;
    assert(aoe_research_action_progress_ror(&action,&id,&percent));
    assert(id == 2 && percent == 0);

    action.research_id = -1;
    assert(aoe_research_action_progress_ror(&action,&id,&percent));
    assert(id == -1 && percent == 0);
    action.research_id = 3;
    assert(aoe_research_action_progress_ror(&action,&id,&percent));
    assert(id == 3 && percent == 0);

    action.state = 1; action.research_id = 99; action.progress_table = NULL;
    assert(aoe_research_action_progress_ror(&action,&id,&percent));
    assert(id == 99 && percent == 100);

    action.state = 0; action.research_id = 0; action.progress_table = &table;
    entries[0].progress = 32768.0f; /* 32768%, observed low signed word */
    techs_data[0].research_time = 100;
    assert(aoe_research_action_progress_ror(&action,&id,&percent));
    assert(percent == INT16_MIN);
    entries[0].progress = NAN; percent = 55;
    assert(!aoe_research_action_progress_ror(&action,&id,&percent));
    assert(id == 0 && percent == 55);
    entries[0].progress = 1.0f; techs_data[0].research_time = 0;
    assert(!aoe_research_action_progress_ror(&action,&id,&percent));

    AoeResearchProgressEntry completion_entries[104]={{0}};
    Trace trace={{0},0,-1,-1};
    AoeResearchCompletionManagerRor completion={
        104,completion_entries,&trace,apply,rebuild,wonder};
    completion_entries[100].state=2;
    assert(aoe_research_complete_ror(&completion,100));
    assert(completion_entries[100].state==3 && trace.count==2 &&
           !memcmp(trace.events,"AR",2) && trace.id==100);
    completion_entries[102].state=0; trace=(Trace){{0},0,-1,-1};
    assert(aoe_research_complete_ror(&completion,102));
    assert(completion_entries[102].state==3 && trace.count==3 &&
           !memcmp(trace.events,"ARW",3) && trace.wonder==1);
    assert(!aoe_research_complete_ror(&completion,102) && trace.count==3);
    completion_entries[103].state=-1;
    assert(!aoe_research_complete_ror(&completion,103));
    return 0;
}
