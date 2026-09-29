#include "research_action.h"

#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct { char events[4]; int count; int16_t id; int wonder; } Trace;
static void apply(void *context,int16_t id) {
    Trace *trace=context; trace->events[trace->count++]='A'; trace->id=id;
}
static void rebuild(void *context) {
    Trace *trace=context; trace->events[trace->count++]='R';
}
static void wonder(void *context,int index) {
    Trace *trace=context; trace->events[trace->count++]='W';trace->wonder=index;
}

int main(void) {
    DatTech techs_data[3];
    DatTechs techs = {3,techs_data,0};
    AoeResearchProgressEntry entries[3] = {
        {45.0f,2},{0.0f,3},{999.0f,1}
    };
    AoeResearchProgressTable table = {3,entries,&techs};
    AoeResearchAction action = {103,0,0,&table};
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
