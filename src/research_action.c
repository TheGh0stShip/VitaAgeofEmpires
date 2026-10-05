#include "research_action.h"

#include "aoe_ftol.h"

#include <math.h>
#include <string.h>

int aoe_research_complete_ror(AoeResearchCompletionManagerRor *manager,
                              int16_t research_id) {
    AoeResearchProgressEntry *entry;
    if (!manager || !manager->entries || !manager->apply_effect ||
        !manager->rebuild || research_id < 0 || research_id >= manager->count)
        return 0;
    entry=&manager->entries[research_id];
    if (entry->state < 0 || entry->state >= 3) return 0;
    if (research_id >= 0x65 && research_id <= 0x67 &&
        !manager->wonder_complete)
        return 0;
    entry->state=3;
    manager->apply_effect(manager->context,research_id);
    manager->rebuild(manager->context);
    if (research_id >= 0x65 && research_id <= 0x67)
        manager->wonder_complete(manager->context,research_id-0x65);
    return 1;
}

int aoe_research_progress_percent_ror(const AoeResearchProgressTable *table,
                                      int16_t research_id, int16_t *percent) {
    const AoeResearchProgressEntry *entry;
    long double value;
    int64_t truncated;
    if (!table || !percent || research_id < 0 || research_id >= table->count) {
        if (percent) *percent = 0;
        return table && percent;
    }
    if (!table->entries) return 0;
    entry = &table->entries[research_id];
    if (entry->state == 3) {
        *percent = 100;
        return 1;
    }
    if (entry->state != 2) {
        *percent = 0;
        return 1;
    }
    if (!table->techs || !table->techs->techs ||
        (uint16_t)research_id >= table->techs->count ||
        table->techs->techs[research_id].research_time == 0 ||
        !isfinite(entry->progress)) return 0;
    value = (long double)entry->progress * 100.0L /
            table->techs->techs[research_id].research_time;
    if (!isfinite(value) || value >= 9223372036854775808.0L ||
        value < -9223372036854775808.0L) return 0;
    truncated = (int64_t)value;
    *percent = (int16_t)(uint16_t)truncated;
    return 1;
}

int aoe_research_action_progress_ror(const AoeResearchAction *action,
                                     int16_t *research_id, int16_t *percent) {
    if (!action || !research_id || !percent) return 0;
    *research_id = action->research_id;
    if (action->state == 1) {
        *percent = 100;
        return 1;
    }
    return aoe_research_progress_percent_ror(action->progress_table,
                                             action->research_id, percent);
}

/* ---- research manager start / progress / stop ----------------------- */

static int owner_valid(const AoeResearchOwnerRor *o) {
    return o && o->manager && o->techs && o->resources &&
           o->resources->values && o->manager->entries &&
           (uint32_t)o->manager->count <= o->techs->count;
}
static int resource_index_ok(const AoeResearchOwnerRor *o, int16_t type) {
    return type >= 0 && type < o->resources->count;
}
static int in_range(const AoeResearchOwnerRor *o, int16_t id) {
    return id >= 0 && id < o->manager->count;
}

int aoe_research_can_start_ror(const AoeResearchOwnerRor *o, int16_t id,
                               int16_t *reason) {
    const DatTech *tech;
    float *res;
    float reserve;
    int use_reserve;
    if (!owner_valid(o) || o->resources->count <= 29) return -1;
    res = o->resources->values;
    if (!in_range(o, id) || o->manager->entries[id].state != 1) {
        if (reason) *reason = -1;
        return 0;
    }
    tech = &o->techs->techs[id];
    /* fld res[29]; fcomp 1.0 */
    use_reserve = !((double)res[29] < 1.0);
    reserve = use_reserve ? res[3] : 0.0f;
    for (int i = 0; i < 3; ++i) {
        int16_t type = tech->costs[i].resource_type;
        float cost;
        if (type < 0 || !(uint8_t)tech->costs[i].paid) continue;
        if (!resource_index_ok(o, type)) return -1;
        cost = (float)tech->costs[i].amount;
        if (use_reserve && type == 3) {
            reserve = reserve - cost;
            if (reserve < 0.0f) {
                if (reason) *reason = type;
                return 0;
            }
        }
        /* fld res; fcomp cost / fld reserve; fadd res; fcomp cost (the sum
           stays on the x87 stack) */
        if ((double)res[type] < (double)cost) {
            if ((double)reserve + (double)res[type] < (double)cost) {
                if (reason) *reason = type;
                return 0;
            }
            reserve = (float)((double)reserve -
                              ((double)cost - (double)res[type]));
        }
    }
    return 1;
}

int aoe_research_start_ror(const AoeResearchOwnerRor *o, int16_t id,
                           int pay, int16_t *reason) {
    const DatTech *tech;
    float *res;
    int ok;
    if (!owner_valid(o)) return -1;
    if (!in_range(o, id)) {
        if (reason) *reason = -1;
        return 0;
    }
    ok = aoe_research_can_start_ror(o, id, reason);
    if (ok < 0) return -1;
    if (!ok) {
        if (reason) *reason = -1;
        return 0;
    }
    tech = &o->techs->techs[id];
    res = o->resources->values;
    if (pay) {
        for (int i = 0; i < 3; ++i) {
            int16_t type = tech->costs[i].resource_type;
            float cost;
            if (type < 0 || !(uint8_t)tech->costs[i].paid) continue;
            cost = (float)tech->costs[i].amount;
            if ((double)res[type] < (double)cost) {
                /* res[3] = res[3] - (cost - res[type]); res[type] = 0 */
                res[3] = (float)((double)res[3] -
                                 ((double)cost - (double)res[type]));
                res[type] = 0.0f;
            } else {
                res[type] = res[type] - cost;
            }
        }
    }
    o->manager->entries[id].state = 2;
    return 1;
}

int aoe_research_add_progress_ror(const AoeResearchOwnerRor *o, int16_t id,
                                  float amount, int *researching) {
    AoeResearchProgressEntry *e;
    if (!owner_valid(o) || !researching) return 0;
    *researching = 0;
    if (!in_range(o, id) || o->manager->entries[id].state != 2) return 1;
    e = &o->manager->entries[id];
    e->progress = amount + e->progress;
    /* fild research_time; fcomp progress; test ah,0x41 */
    if ((double)o->techs->techs[id].research_time > (double)e->progress) {
        *researching = 1;
        return 1;
    }
    (void)aoe_research_complete_ror(o->manager, id);
    return 1;
}

int aoe_research_stop_ror(const AoeResearchOwnerRor *o, int16_t id,
                          int refund) {
    const DatTech *tech;
    if (!owner_valid(o)) return -1;
    if (!in_range(o, id) || o->manager->entries[id].state != 2) return 0;
    tech = &o->techs->techs[id];
    if (refund) {
        for (int i = 0; i < 3; ++i) {
            int16_t type = tech->costs[i].resource_type;
            if (type < 0 || !(uint8_t)tech->costs[i].paid) continue;
            if (!resource_index_ok(o, type)) return -1;
            o->resources->values[type] =
                (float)tech->costs[i].amount + o->resources->values[type];
        }
    }
    o->manager->entries[id].state = 1;
    o->manager->entries[id].progress = 0.0f;
    return 1;
}

int aoe_research_reset_ror(AoeResearchCompletionManagerRor *m, int16_t id) {
    if (!m || !m->entries || id < 0 || id >= m->count ||
        m->entries[id].state < 1)
        return 0;
    m->entries[id].state = 0;
    if (m->rebuild) m->rebuild(m->context);
    return 1;
}

/* ---- research action ------------------------------------------------ */

enum { RESEARCH_SIGNAL_STOPPED = 0x1f9, RESEARCH_SIGNAL_DONE = 0x1fa };

static int ops_valid(const AoeResearchActionOps *ops) {
    return ops && ops->notify && ops->failed && ops->completed &&
           ops->signal && owner_valid(&ops->owner);
}

void aoe_research_action_init_ror(AoeResearchAction *a, int16_t id,
                                  int32_t request) {
    if (!a) return;
    memset(a, 0, sizeof(*a));
    a->action_type = 0x67;
    a->state = 0;
    a->research_id = id;
    a->request_id = request;
    a->building_id = -1;
    a->owner_index = -1;
}

int aoe_research_action_activate_ror(AoeResearchAction *a, int active,
                                     const AoeResearchActionOps *ops) {
    int16_t reason = 0;
    int started;
    if (!a || !ops_valid(ops)) return 0;
    if (!active || a->research_id < 0) return 1;
    started = aoe_research_start_ror(&ops->owner, a->research_id, 1, &reason);
    if (started < 0) return 0;
    if (started == 1) {
        a->state = 6;
        return 1;
    }
    ops->failed(ops->context, a, reason);
    ops->signal(ops->context, a, RESEARCH_SIGNAL_STOPPED);
    a->state = 1;
    return 1;
}

int aoe_research_action_update_ror(AoeResearchAction *a,
                                   const AoeResearchActionOps *ops,
                                   uint8_t *result) {
    float amount;
    int researching = 0;
    if (!a || !result || !ops_valid(ops)) return 0;
    *result = 0;
    if (a->state == 1) {
        *result = 1;
        return 1;
    }
    if (a->state != 6) return 1;
    /* fld tick; fmul work_rate; fstp float */
    amount = ops->fast_research ? 1000.0f : ops->tick * a->work_rate;
    if (!aoe_research_add_progress_ror(&ops->owner, a->research_id, amount,
                                       &researching))
        return 0;
    if (researching) return 1;
    a->state = 1;
    ops->notify(ops->context, 0x64, a->owner_index, a->research_id,
                aoe_ftol32(a->building_x), aoe_ftol32(a->building_y));
    ops->completed(ops->context, a);
    ops->signal(ops->context, a, RESEARCH_SIGNAL_DONE);
    *result = 1;
    return 1;
}

static void stopped(AoeResearchAction *a, const AoeResearchActionOps *ops) {
    ops->notify(ops->context, 0x65, a->owner_index, a->research_id, 0, 0);
    ops->failed(ops->context, a, -1);
    ops->signal(ops->context, a, RESEARCH_SIGNAL_STOPPED);
}

int aoe_research_action_cancel_ror(AoeResearchAction *a,
                                   const AoeResearchActionOps *ops) {
    if (!a || !ops_valid(ops)) return 0;
    a->state = 1;
    if (aoe_research_stop_ror(&ops->owner, a->research_id, 1) < 0) return 0;
    stopped(a, ops);
    return 1;
}

int aoe_research_action_destroy_ror(AoeResearchAction *a,
                                    const AoeResearchActionOps *ops) {
    if (!a || !ops_valid(ops)) return 0;
    if (a->state == 1) return 1;
    if (aoe_research_stop_ror(&ops->owner, a->research_id, 0) < 0) return 0;
    if (!ops->quiet_teardown) stopped(a, ops);
    return 1;
}
