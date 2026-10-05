#include "ai_tactical.h"
#include "value_grid.h"
#include "aoe_ftol.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"
#include "aoe_stream.h"
#include "movement.h"
#include "unit_class.h"

static void fill(int32_t *v, int n, int32_t value) {
    int i;
    for (i = 0; i < n; ++i) v[i] = value;
}

/* Fields both constructors set. */
static void common_fields(AoeAiTacticalRor *ai) {
    ai->field_f0 = 0;
    aoe_ai_counters_init_ror(&ai->counters, 4);
    ai->field_e18 = ai->state = ai->field_e20 = 0;
    ai->field_e44 = ai->field_e48 = 0;
    ai->field_f7c = ai->field_f80 = ai->field_f84 = -1;
    ai->field_f88 = ai->field_f8c = ai->field_f90 = 0;
    ai->field_f94 = ai->field_f98 = 0;
    ai->field_f9c = ai->field_fa0 = ai->field_fa4 = ai->field_fa8 = 0;
    ai->field_ffc = ai->field_1000 = -1;
    ai->field_1004 = ai->field_1008 = 0;
}

void aoe_ai_tactical_init_ror(AoeAiTacticalRor *ai, int32_t player,
                              AoeAiLogRor *log) {
    int i;
    if (!ai) return;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_tac_init_ror(&ai->module, player, log);
    common_fields(ai);
    fill(ai->field_d88, 4, -1);
    fill(ai->sn, AOE_AI_SN_COUNT, -1);
    for (i = 0; i < AOE_AI_TAC_GATHERERS; ++i) fill(ai->gatherers[i], 5, -1);
}

void aoe_ai_tactical_release_ror(AoeAiTacticalRor *ai) {
    int i;
    if (!ai) return;
    aoe_ai_tactical_delete_groups_ror(ai, -1);
    free(ai->groups);
    ai->groups = NULL;
    ai->groups_len = ai->groups_cap = 0;
    aoe_ai_counters_release_ror(&ai->counters);
    aoe_ai_int_list_release_ror(&ai->list_d58);
    aoe_ai_int_list_release_ror(&ai->list_d48);
    aoe_ai_int_list_release_ror(&ai->list_d38);
    aoe_ai_int_list_release_ror(&ai->list_d28);
    for (i = AOE_AI_TAC_LISTS - 1; i >= 0; --i)
        aoe_ai_int_list_release_ror(&ai->lists[i]);
    aoe_ai_tac_release_ror(&ai->module);
}

/* ---- Save and load ---- */

static void put_list(AoeOutcomeStream *out, const AoeAiIntListRor *list) {
    int32_t i;
    aoe_stream_put_i32(out, list->count);
    for (i = 0; i < list->count; ++i) aoe_stream_put_i32(out, list->data[i]);
}

static void get_list(AoeOutcomeStream *in, AoeAiIntListRor *list) {
    int32_t i, n = aoe_stream_i32(in);
    for (i = 0; i < n && !in->failed; ++i)
        aoe_ai_int_list_add_unique_ror(list, aoe_stream_i32(in));
}

static void put_ints(AoeOutcomeStream *out, const int32_t *v, int n) {
    int i;
    for (i = 0; i < n; ++i) aoe_stream_put_i32(out, v[i]);
}

static void get_ints(AoeOutcomeStream *in, int32_t *v, int n) {
    int i;
    for (i = 0; i < n; ++i) v[i] = aoe_stream_i32(in);
}

int aoe_ai_tactical_save_ror(const AoeAiTacticalRor *ai,
                             AoeOutcomeStream *out) {
    int i;
    if (!ai || !out) return 0;
    put_list(out, &ai->lists[0]);
    put_list(out, &ai->lists[1]);
    aoe_stream_put_i32(out, ai->field_9cc);
    aoe_stream_put_i32(out, ai->field_9d0);
    for (i = 0; i < AOE_AI_TAC_GATHERERS; ++i) put_ints(out, ai->gatherers[i], 6);
    for (i = 2; i < AOE_AI_TAC_LISTS; ++i) put_list(out, &ai->lists[i]);
    aoe_stream_put_i32(out, AOE_AI_SN_COUNT);
    put_ints(out, ai->sn, AOE_AI_SN_COUNT);
    put_list(out, &ai->list_d28);
    put_list(out, &ai->list_d38);
    put_list(out, &ai->list_d48);
    put_list(out, &ai->list_d58);
    put_ints(out, ai->field_d68, 4);
    put_ints(out, ai->field_d78, 4);
    put_ints(out, ai->field_d88, 4);
    put_ints(out, ai->field_d98, 4);
    put_ints(out, ai->field_e24, 4);
    put_ints(out, ai->field_e34, 4);
    for (i = 0; i < 4; ++i)
        aoe_stream_put_i32(out, aoe_ai_counters_get_ror(&ai->counters, i));
    aoe_stream_put_i32(out, ai->next_group_id);
    aoe_stream_put_i32(out, ai->group_count);
    for (i = 0; i < ai->groups_len; ++i)
        aoe_ai_unit_group_save_ror(ai->groups[i], out);
    aoe_stream_put_i32(out, ai->field_d0c);
    aoe_stream_put_i32(out, ai->field_d10);
    aoe_stream_put_i32(out, ai->field_d14);
    aoe_stream_put_i32(out, ai->field_d18);
    aoe_stream_put_i32(out, ai->last_update);
    aoe_stream_put_i32(out, ai->field_d20);
    aoe_stream_put_i32(out, ai->field_d24);
    aoe_stream_put_i32(out, ai->field_e18);
    aoe_stream_put_i32(out, ai->state);
    aoe_stream_put_i32(out, ai->field_e44);
    aoe_stream_put_i32(out, ai->field_e48);
    put_ints(out, ai->block_e4c, 76);
    aoe_stream_put_i32(out, ai->field_f7c);
    aoe_stream_put_i32(out, ai->field_f80);
    aoe_stream_put_i32(out, ai->field_f84);
    aoe_stream_put_i32(out, ai->field_f88);
    aoe_stream_put_i32(out, ai->field_f8c);
    aoe_stream_put_i32(out, ai->field_f90);
    aoe_stream_put_i32(out, ai->field_f94);
    aoe_stream_put_i32(out, ai->field_f98);
    aoe_stream_put_i32(out, ai->field_f9c);
    aoe_stream_put_i32(out, ai->field_fa4);
    aoe_stream_put_i32(out, ai->field_fa0);
    aoe_stream_put_i32(out, ai->field_fa8);
    put_ints(out, ai->block_fac, 11);
    put_ints(out, ai->block_fd8, 9);
    return !out->failed;
}

int aoe_ai_tactical_load_ror(AoeAiTacticalRor *ai, int32_t player,
                             float version, AoeOutcomeStream *in,
                             AoeAiLogRor *log) {
    int32_t n;
    int i;
    if (!ai || !in) return 0;
    memset(ai, 0, sizeof(*ai));
    aoe_ai_tac_init_ror(&ai->module, player, log);
    common_fields(ai);
    get_list(in, &ai->lists[0]);
    get_list(in, &ai->lists[1]);
    ai->field_9cc = aoe_stream_i32(in);
    ai->field_9d0 = aoe_stream_i32(in);
    if (version < 7.19f) {
        int width = version < 7.17f ? 4 : 5;
        for (i = 0; i < AOE_AI_TAC_GATHERERS; ++i) {
            get_ints(in, ai->gatherers[i], width);
            if (width == 4) ai->gatherers[i][4] = -1;
            ai->gatherers[i][5] = 0;
        }
    } else {
        for (i = 0; i < AOE_AI_TAC_GATHERERS; ++i) get_ints(in, ai->gatherers[i], 6);
    }
    get_list(in, &ai->lists[2]);
    if (version >= 2.3f) get_list(in, &ai->lists[3]);
    for (i = 4; i < 9; ++i) get_list(in, &ai->lists[i]);
    if (version >= 1.4f) get_list(in, &ai->lists[9]);
    n = aoe_stream_i32(in);
    if (n < 0 || n > AOE_AI_SN_COUNT) {
        in->failed = 1;
        return 0;
    }
    get_ints(in, ai->sn, n);
    get_list(in, &ai->list_d28);
    get_list(in, &ai->list_d38);
    get_list(in, &ai->list_d48);
    get_list(in, &ai->list_d58);
    get_ints(in, ai->field_d68, 4);
    get_ints(in, ai->field_d78, 4);
    get_ints(in, ai->field_d88, 4);
    get_ints(in, ai->field_d98, 4);
    get_ints(in, ai->field_e24, 4);
    get_ints(in, ai->field_e34, 4);
    for (i = 0; i < 4; ++i)
        aoe_ai_counters_set_ror(&ai->counters, i, aoe_stream_i32(in));
    ai->next_group_id = aoe_stream_i32(in);
    ai->group_count = aoe_stream_i32(in);
    for (i = 0; i < ai->group_count && !in->failed; ++i) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_new_group_ror(ai, 0);
        if (g) aoe_ai_unit_group_load_ror(g, version, in);
    }
    ai->field_d0c = aoe_stream_i32(in);
    if (version >= 2.4f) ai->field_d10 = aoe_stream_i32(in);
    ai->field_d14 = aoe_stream_i32(in);
    ai->field_d18 = aoe_stream_i32(in);
    ai->last_update = aoe_stream_i32(in);
    ai->field_d20 = aoe_stream_i32(in);
    ai->field_d24 = aoe_stream_i32(in);
    if (version >= 2.2f) ai->field_e18 = aoe_stream_i32(in);
    ai->state = aoe_stream_i32(in);
    if (version >= 6.1f) {
        ai->field_e44 = aoe_stream_i32(in);
        ai->field_e48 = aoe_stream_i32(in);
    }
    if (version >= 6.2f) get_ints(in, ai->block_e4c, 76);
    ai->block_e4c[69] = 0;
    if (version >= 6.3f) ai->field_f7c = aoe_stream_i32(in);
    if (version >= 6.4f) {
        ai->field_f80 = aoe_stream_i32(in);
        ai->field_f84 = aoe_stream_i32(in);
    }
    if (version >= 6.9f) {
        ai->field_f88 = aoe_stream_i32(in);
        ai->field_f8c = aoe_stream_i32(in);
    }
    if (version >= 6.95f) {
        ai->field_f90 = aoe_stream_i32(in);
        ai->field_f94 = aoe_stream_i32(in);
        ai->field_f98 = aoe_stream_i32(in);
    }
    if (version >= 7.18f) ai->field_f9c = aoe_stream_i32(in);
    if (version >= 6.96f) ai->field_fa4 = aoe_stream_i32(in);
    if (version >= 6.98f) ai->field_fa0 = aoe_stream_i32(in);
    if (version >= 6.993f) ai->field_fa8 = aoe_stream_i32(in);
    if (version >= 7.04f) get_ints(in, ai->block_fac, 11);
    ai->block_fac[10] = 0;
    if (version >= 7.15f) get_ints(in, ai->block_fd8, 9);
    return !in->failed;
}

/* ---- Small members ---- */

const char *aoe_ai_tactical_state_name_ror(const AoeAiTacticalRor *ai) {
    static const char *const names[] = {
        "Setup SolGroups", "Eval CivDist", "Task Civs", "Setup BoatGroups",
        "Fill BoatGroups", "Task Boats", "Fill SolGroups", "Task IdleSol",
        "Task ActSol", "Playtasking", "Task UngrpSol", "Research", "Train",
        "BldLst Change", "Help Build", "Repair Bldg", "Repair Wall", "Build",
        "Open Builds", "Open Tasks", "Food dropsite", "BldLst Insertions"};
    if (!ai || ai->state < 0 ||
        ai->state >= (int32_t)(sizeof names / sizeof names[0]))
        return "Invalid";
    return names[ai->state];
}

int32_t aoe_ai_tactical_get_ror(const AoeAiTacticalRor *ai, int which) {
    switch (which) {
    case AOE_AI_TAC_LIST0_COUNT: return ai->lists[0].count;
    case AOE_AI_TAC_LIST1_COUNT: return ai->lists[1].count;
    case AOE_AI_TAC_FIELD_9CC: return ai->field_9cc;
    case AOE_AI_TAC_LIST1_SPARE: return ai->lists[1].field_8;
    case AOE_AI_TAC_FIELD_9D0: return ai->field_9d0;
    case AOE_AI_TAC_LIST2_COUNT: return ai->lists[2].count;
    case AOE_AI_TAC_LIST4_COUNT: return ai->lists[4].count;
    default: return 0;
    }
}

int32_t aoe_ai_tactical_d88_ror(const AoeAiTacticalRor *ai, int32_t i) {
    return i >= 0 && i < 4 ? ai->field_d88[i] : -1;
}

int32_t aoe_ai_tactical_d88_counter_ror(const AoeAiTacticalRor *ai,
                                        int32_t i) {
    if (i < 0 || i >= 4) return -1;
    return aoe_ai_counters_get_ror(&ai->counters, ai->field_d88[i]);
}

int32_t aoe_ai_tactical_sn_ror(const AoeAiTacticalRor *ai, int32_t i) {
    return ai && i >= 0 && i < AOE_AI_SN_COUNT ? ai->sn[i] : -1;
}

void aoe_ai_tactical_set_sn_ror(AoeAiTacticalRor *ai, int32_t i,
                                int32_t value, AoeAiInfoSetFn info_204,
                                void *context) {
    if (!ai || i < 0 || i >= AOE_AI_SN_COUNT) return;
    ai->sn[i] = value;
    if ((i >= 117 && i <= 120) || (i >= 156 && i <= 159)) {
        if (value < 0)
            ai->sn[i] = 0;
        else if (value > 100)
            ai->sn[i] = 100;
    }
    if (i == 105 && info_204) info_204(context, value);
}

/* ---- Groups ---- */

AoeAiUnitGroupRor *aoe_ai_tactical_new_group_ror(AoeAiTacticalRor *ai,
                                                 int32_t numbered) {
    AoeAiUnitGroupRor *g;
    if (!ai) return NULL;
    if (ai->groups_len == ai->groups_cap) {
        int32_t cap = ai->groups_cap ? ai->groups_cap * 2 : 8;
        AoeAiUnitGroupRor **grown =
            (AoeAiUnitGroupRor **)realloc(ai->groups, (size_t)cap * sizeof(*grown));
        if (!grown) return NULL;
        ai->groups = grown;
        ai->groups_cap = cap;
    }
    g = (AoeAiUnitGroupRor *)malloc(sizeof(*g));
    if (!g) return NULL;
    aoe_ai_unit_group_init_ror(g);
    ai->groups[ai->groups_len++] = g;
    if (numbered == 1) {
        g->id = ai->next_group_id++;
        ++ai->group_count;
    }
    return g;
}

static void drop_group(AoeAiTacticalRor *ai, int32_t i) {
    free(ai->groups[i]);
    memmove(ai->groups + i, ai->groups + i + 1,
            (size_t)(ai->groups_len - i - 1) * sizeof(*ai->groups));
    --ai->groups_len;
    --ai->group_count;
}

int aoe_ai_tactical_delete_group_ror(AoeAiTacticalRor *ai, int32_t id) {
    int32_t i;
    if (!ai) return 0;
    for (i = 0; i < ai->groups_len; ++i) {
        if (ai->groups[i]->id != id) continue;
        drop_group(ai, i);
        return 1;
    }
    return 0;
}

int aoe_ai_tactical_delete_groups_ror(AoeAiTacticalRor *ai, int32_t task) {
    int32_t i = 0;
    if (!ai) return 1;
    while (i < ai->groups_len) {
        if (task == -1 || ai->groups[i]->task == task)
            drop_group(ai, i);
        else
            ++i;
    }
    return 1;
}

AoeAiUnitGroupRor *aoe_ai_tactical_find_group_ror(const AoeAiTacticalRor *ai,
                                                  int32_t id, int32_t task,
                                                  int32_t field_180,
                                                  int32_t field_184) {
    int32_t i;
    if (!ai) return NULL;
    for (i = 0; i < ai->groups_len; ++i) {
        const AoeAiUnitGroupRor *g = ai->groups[i];
        if ((id == -1 || g->id == id) && (task == -1 || g->task == task) &&
            (field_180 == -1 || g->field_180 == field_180) &&
            (field_184 == -1 || g->field_184 == field_184))
            return ai->groups[i];
    }
    return NULL;
}

int32_t aoe_ai_tactical_count_groups_ror(const AoeAiTacticalRor *ai,
                                         int32_t task, int32_t field_180) {
    int32_t i, n = 0;
    if (!ai) return 0;
    for (i = 0; i < ai->groups_len; ++i)
        if ((task == -1 || ai->groups[i]->task == task) &&
            (field_180 == -1 || ai->groups[i]->field_180 == field_180))
            ++n;
    return n;
}

int32_t aoe_ai_tactical_grouped_units_ror(const AoeAiTacticalRor *ai,
                                          int32_t task) {
    int32_t i, n = 0;
    if (!ai) return 0;
    for (i = 0; i < ai->groups_len; ++i)
        if (task == -1 || ai->groups[i]->task == task)
            n += aoe_ai_unit_group_count_ror(ai->groups[i]);
    return n;
}

int32_t aoe_ai_tactical_grouped_66_ror(const AoeAiTacticalRor *ai) {
    return aoe_ai_tactical_grouped_units_ror(ai, 0x66);
}

AoeAiUnitGroupRor *aoe_ai_tactical_group_of_ror(const AoeAiTacticalRor *ai,
                                                int32_t unit) {
    int32_t i;
    if (!ai) return NULL;
    for (i = 0; i < ai->groups_len; ++i)
        if (aoe_ai_unit_group_contains_ror(ai->groups[i], unit) == 1)
            return ai->groups[i];
    return NULL;
}

int aoe_ai_tactical_ungroup_ror(AoeAiTacticalRor *ai, int32_t unit,
                                const AoeAiUnitGroupOpsRor *ops) {
    AoeAiUnitGroupRor *g = aoe_ai_tactical_group_of_ror(ai, unit);
    int removed;
    if (!g) return 0;
    removed = aoe_ai_unit_group_remove_ror(g, unit, ops);
    if (aoe_ai_unit_group_count_ror(g) == 0)
        aoe_ai_tactical_delete_group_ror(ai, g->id);
    return removed;
}

int32_t aoe_ai_tactical_free_target_ror(const AoeAiTacticalRor *ai,
                                        const AoeAiStrategyRor *strategy) {
    int32_t id;
    if (!strategy || aoe_ai_strategy_ai_victory_kind_ror(strategy) != 0)
        return -1;
    id = aoe_ai_strategy_ai_target_id_ror(strategy);
    return aoe_ai_tactical_find_group_ror(ai, -1, -1, -1, id) ? -1 : id;
}

int32_t aoe_ai_tactical_free_second_ror(const AoeAiTacticalRor *ai,
                                        const AoeAiStrategyRor *strategy) {
    int32_t id;
    if (!strategy || aoe_ai_strategy_ai_victory_kind_ror(strategy) != 4)
        return -1;
    id = aoe_ai_strategy_ai_second_id_ror(strategy);
    return aoe_ai_tactical_find_group_ror(ai, -1, -1, -1, id) ? -1 : id;
}

int32_t aoe_ai_tactical_d24_limit_ror(const AoeAiTacticalRor *ai,
                                      int32_t kind) {
    if (ai->sn[71] == 1)
        return ai->sn[48] <= ai->field_d24 ? ai->sn[48] : ai->field_d24;
    return kind == 0x2e ? ai->field_d24 : ai->sn[48];
}

void aoe_ai_tactical_roll_d24_ror(AoeAiTacticalRor *ai, AoeAiRandFn rand_fn,
                                  void *context) {
    int32_t n;
    if (!ai) return;
    n = ai->sn[102];
    if (n < 1) {
        ai->field_d24 = ai->sn[46];
    } else {
        int32_t r = rand_fn ? rand_fn(context) : 0;
        ai->field_d24 = r % n - n / 2 + ai->sn[46];
    }
    if (ai->field_d24 < 0) ai->field_d24 = 0;
}

/* ---- Members working through the computer player ---- */

int32_t aoe_ai_tactical_effort_ror(const AoeAiTacticalRor *ai, float f) {
    int32_t v = aoe_ftol32((double)ai->sn[34] / (double)f);
    return v > 99 ? 99 : v;
}

int32_t aoe_ai_tactical_d68_ror(const AoeAiTacticalRor *ai, int32_t i) {
    return i >= 0 && i < 4 ? ai->field_d68[i] : -1;
}

int32_t aoe_ai_tactical_mapped_ror(const AoeAiTacticalRor *ai, int which,
                                   int32_t i) {
    int32_t j;
    if (i < 0 || i >= 4) return -1;
    j = ai->field_d88[i];
    if (j < 0 || j >= 4) return -1;
    switch (which) {
    case AOE_AI_TAC_MAP_D98: return ai->field_d98[j];
    case AOE_AI_TAC_MAP_D68: return ai->field_d68[j];
    case AOE_AI_TAC_MAP_D78: return ai->field_d78[j];
    default: return -1;
    }
}

int32_t aoe_ai_tactical_counter_share_ror(const AoeAiTacticalRor *ai,
                                          int32_t i) {
    if (ai->field_d78[i] == 0) return ai->sn[148];
    return aoe_ai_counters_get_ror(&ai->counters, i) / ai->field_d78[i];
}

void aoe_ai_tactical_arm_e18_ror(AoeAiTacticalRor *ai, int32_t value) {
    if (ai->field_e18 != 1 && (ai->sn[134] == 0 || ai->sn[134] == value))
        ai->field_e18 = 1;
}

int aoe_ai_tactical_age_is_ror(const AoeAiTacticalOpsRor *ops, int32_t age) {
    return aoe_ftol32(ops && ops->resource ? ops->resource(ops->context, 6)
                                           : 0.0f) == age;
}

/* 0x004e1a80: see the header. */
void aoe_ai_tactical_gatherer_gain_ror(AoeAiTacticalRor *ai, int32_t unit,
                                       int32_t kind, int32_t amount,
                                       const AoeAiTacticalOpsRor *ops) {
    int32_t *g;
    if (kind >= 0 && kind < 4 && ai->field_d98[kind] < 0 && ops) {
        int32_t stock = aoe_ftol32(ops->resource
                                       ? ops->resource(ops->context, kind)
                                       : 0.0f);
        if (stock + ai->field_d98[kind] > 0 && ops->rebalance)
            ops->rebalance(ops->context);
    }
    g = aoe_ai_tactical_gatherer_ror(ai, unit);
    if (g && g[1] == kind) g[3] += amount;
}

/* 0x004e1ae0: see the header. */
int aoe_ai_tactical_release_object_ror(const AoeAiTacticalOpsRor *ops,
                                       int32_t id) {
    AoeAiWorldObjectRor obj;
    if (!ops || !ops->object || !ops->object(ops->context, id, &obj))
        return 0;
    return ops->release_object && ops->release_object(ops->context, id) == 1;
}

int32_t *aoe_ai_tactical_gatherer_ror(AoeAiTacticalRor *ai, int32_t unit) {
    int i;
    for (i = 0; i < AOE_AI_TAC_GATHERERS; ++i)
        if (ai->gatherers[i][0] == unit) return ai->gatherers[i];
    return NULL;
}

int aoe_ai_tactical_has_gatherer_ror(const AoeAiTacticalRor *ai,
                                     int32_t unit) {
    int i;
    for (i = 0; i < AOE_AI_TAC_GATHERERS; ++i)
        if (ai->gatherers[i][0] == unit) return 1;
    return 0;
}

int32_t aoe_ai_tactical_count_gatherers_ror(const AoeAiTacticalRor *ai,
                                            int32_t value) {
    int32_t i, n = 0;
    for (i = 0; i < ai->field_9cc && i < AOE_AI_TAC_GATHERERS; ++i)
        if (ai->gatherers[i][4] == value) ++n;
    return n;
}

int32_t *aoe_ai_tactical_add_gatherer_ror(AoeAiTacticalRor *ai, int32_t unit) {
    int i, free_slot = -1;
    if (unit == -1) {
        aoe_ai_log_ror(&ai->module, "ERROR adding gatherer id -1.");
        return NULL;
    }
    for (i = 0; i < AOE_AI_TAC_GATHERERS; ++i) {
        if (ai->gatherers[i][0] == -1 && free_slot == -1) {
            free_slot = i;
            continue;
        }
        if (ai->gatherers[i][0] == unit) return ai->gatherers[i];
    }
    if (free_slot == -1) return NULL;
    ai->gatherers[free_slot][0] = unit;
    ai->gatherers[free_slot][4] = -1;
    ai->gatherers[free_slot][5] = 0;
    ++ai->field_9cc;
    return ai->gatherers[free_slot];
}

int aoe_ai_tactical_remove_gatherer_ror(AoeAiTacticalRor *ai, int32_t unit) {
    int i, k;
    if (unit == -1) {
        aoe_ai_log_ror(&ai->module, "ERROR removing gatherer id -1.");
        return 0;
    }
    for (i = 0; i < AOE_AI_TAC_GATHERERS; ++i) {
        if (ai->gatherers[i][0] != unit) continue;
        for (k = 0; k < 5; ++k) ai->gatherers[i][k] = -1;
        --ai->field_9cc;
        for (; i < AOE_AI_TAC_GATHERERS - 1 && ai->gatherers[i + 1][0] != -1; ++i)
            for (k = 0; k < 5; ++k) {
                ai->gatherers[i][k] = ai->gatherers[i + 1][k];
                ai->gatherers[i + 1][k] = -1;
            }
        return 1;
    }
    return 0;
}

void aoe_ai_tactical_drop_unit_ror(AoeAiTacticalRor *ai, int32_t unit,
                                   const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor o;
    int32_t task;
    aoe_ai_int_list_remove_ror(&ai->lists[1], unit);
    aoe_ai_tactical_remove_gatherer_ror(ai, unit);
    if (!ops || !ops->object || !ops->object(ops->context, unit, &o)) return;
    if (!ops->unit_task || !ops->unit_task(ops->context, unit, &task)) return;
    if (task == 0x261 || task == 0x265) {
        int32_t *g = aoe_ai_tactical_gatherer_ror(ai, unit);
        /* The original indexes +0xd68 with the kind unchecked. */
        if (g && g[1] >= 0 && g[1] < 4) --ai->field_d68[g[1]];
    }
}

void aoe_ai_tactical_detask_ror(AoeAiTacticalRor *ai, int32_t unit,
                                const AoeAiTacticalOpsRor *ops) {
    if (unit == -1) {
        aoe_ai_log_ror(&ai->module, "ERROR DETASKING Unit #%d!", -1);
        return;
    }
    aoe_ai_tactical_drop_unit_ror(ai, unit, ops);
    aoe_ai_tactical_order_stop_ror(ops, unit, 100);
}

static int send_order(const AoeAiTacticalOpsRor *ops, int32_t unit,
                      int32_t action, int32_t target, int32_t target2,
                      float a, float b, float c, float d, int32_t value) {
    AoeAiTaskOrderRor o;
    if (!ops || !ops->order) return 1;
    o.player = ops->self;
    o.unit = unit;
    o.action = action;
    o.target = target;
    o.target2 = target2;
    o.a = a;
    o.b = b;
    o.c = c;
    o.d = d;
    o.flag = 1;
    o.zero = 0;
    o.value = value;
    ops->order(ops->context, &o);
    return 1;
}

int aoe_ai_tactical_order_stop_ror(const AoeAiTacticalOpsRor *ops,
                                   int32_t unit, int32_t value) {
    return send_order(ops, unit, 0x2c2, -1, -1, -1.0f, -1.0f, -1.0f, -1.0f,
                      value);
}

int aoe_ai_tactical_order_move_ror(const AoeAiTacticalOpsRor *ops,
                                   int32_t unit, float x, float y,
                                   int32_t value) {
    return send_order(ops, unit, 0x2c6, -1, -1, x, y, 1.0f, 1.0f, value);
}

int aoe_ai_tactical_order_2d4_ror(const AoeAiTacticalOpsRor *ops,
                                  int32_t unit, int32_t target, float a,
                                  float b, float c, float d, int32_t value) {
    return send_order(ops, unit, 0x2d4, target, -1, a, b, c, d, value);
}

void aoe_ai_tactical_player_44_ror(const AoeAiTacticalOpsRor *ops,
                                   int32_t id, uint8_t a, uint8_t b) {
    uint8_t bytes[3];
    bytes[0] = a;
    bytes[1] = b;
    bytes[2] = 1;
    if (ops && ops->player_44) ops->player_44(ops->context, id, bytes);
}

int aoe_ai_tactical_priest_or_siege_ror(const AoeAiTacticalOpsRor *ops,
                                        int32_t id) {
    AoeAiWorldObjectRor o;
    if (!ops || !ops->object || !ops->object(ops->context, id, &o)) return 0;
    return o.type_class == 0x12 || o.type_class == 0xd;
}

int aoe_ai_tactical_unit_has_task_ror(const AoeAiTacticalOpsRor *ops,
                                      int32_t id, int32_t task) {
    AoeAiWorldObjectRor o;
    int32_t t;
    if (!ops || !ops->world_object || !ops->world_object(ops->context, id, &o))
        return 0;
    return ops->unit_task && ops->unit_task(ops->context, id, &t) && t == task;
}

int aoe_ai_tactical_busy_103_ror(const AoeAiTacticalOpsRor *ops, int32_t id,
                                 int listed) {
    AoeAiWorldObjectRor o;
    int32_t t;
    int found;
    if (!ops) return 0;
    found = listed ? ops->object && ops->object(ops->context, id, &o)
                   : ops->world_object && ops->world_object(ops->context, id, &o);
    if (!found || !ops->unit_task || !ops->unit_task(ops->context, id, &t))
        return 0;
    return o.type_id == 0x103 && (t == 0x261 || t == 0x262);
}

int32_t aoe_ai_tactical_count_busy_103_ror(const AoeAiTacticalRor *ai,
                                           const AoeAiTacticalOpsRor *ops) {
    int32_t i, n = 0;
    for (i = 0; i < ai->lists[0].count; ++i)
        if (aoe_ai_tactical_busy_103_ror(ops, ai->lists[0].data[i], 1)) ++n;
    return n;
}

int32_t aoe_ai_tactical_count_groups_8_ror(const AoeAiTacticalRor *ai) {
    int32_t i, n = 0;
    for (i = 0; i < ai->groups_len; ++i) {
        const AoeAiUnitGroupRor *g = ai->groups[i];
        if (g->task >= 0x67 && g->task <= 0x6c && g->field_180 == 8) ++n;
    }
    return n;
}

int aoe_ai_tactical_cap_groups_ror(AoeAiTacticalRor *ai, int32_t player,
                                   const AoeAiTacticalOpsRor *ops) {
    int32_t i;
    if (ai->field_e18 == 0) return 0;
    if (ops && ops->set_dislike) ops->set_dislike(ops->context, player, 100);
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        if (g->task != 100 && g->task != 0x67) continue;
        if (aoe_ai_unit_group_count_ror(g) < g->field_160)
            g->field_160 = aoe_ai_unit_group_count_ror(g);
    }
    return 1;
}

void aoe_ai_tactical_settle_groups_ror(AoeAiTacticalRor *ai,
                                       const AoeAiTacticalOpsRor *ops) {
    int32_t i;
    if (ai->field_e18 == 0) return;
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        int32_t excess, now;
        if (g->task != 100 || g->type == 1) continue;
        excess = g->field_160 - aoe_ai_unit_group_count_ror(g);
        if (excess <= 0) continue;
        if (excess > 5) excess = 5;
        now = ops && ops->game_time ? (int32_t)ops->game_time(ops->context) : 0;
        if (excess <= (int32_t)((uint32_t)now - (uint32_t)g->added_time) / 60000)
            g->field_160 = aoe_ai_unit_group_count_ror(g);
    }
}

int32_t aoe_ai_tactical_pick_leader_ror(const AoeAiTacticalRor *ai,
                                        const AoeAiUnitGroupRor *group,
                                        const AoeAiTacticalOpsRor *ops) {
    int32_t i, best = -1, leader = -1, mode = ai->sn[75];
    if (!group || !ops || !ops->object) return -1;
    for (i = 0; i < aoe_ai_unit_group_count_ror(group); ++i) {
        int32_t id = aoe_ai_unit_group_member_ror(group, i), v;
        AoeAiWorldObjectRor o;
        if (!ops->object(ops->context, id, &o) || o.type_id == 0x7d) continue;
        v = aoe_ftol32(mode == 2 ? (ops->range ? ops->range(ops->context, id) : 0.0)
                                 : (double)o.hp);
        if (mode == 1 || mode == 3) {
            if (best != -1 && !(v < best)) continue;
        } else if (best != -1 && !(best < v)) {
            continue;
        }
        leader = id;
        best = v;
    }
    return leader;
}

int32_t aoe_ai_tactical_pick_leader_ids_ror(const AoeAiTacticalRor *ai,
                                            const AoeAiIntListRor *ids,
                                            const AoeAiTacticalOpsRor *ops) {
    int32_t i, best = -1, leader = -1, mode = ai->sn[75];
    if (!ids || !ops || !ops->object) return -1;
    for (i = 0; i < ids->count; ++i) {
        int32_t id = ids->data[i], v;
        AoeAiWorldObjectRor o;
        if (!ops->object(ops->context, id, &o) || o.type_id == 0x7d) continue;
        v = aoe_ftol32(mode == 2 ? (ops->range ? ops->range(ops->context, id) : 0.0)
                                 : (double)o.hp);
        if (mode == 1 || mode == 3) {
            if (best != -1 && !(v < best)) continue;
        } else if (best != -1 && !(best < v)) {
            continue;
        }
        leader = id;
        best = v;
    }
    return leader;
}

int aoe_ai_tactical_order_2bd_ror(AoeAiTacticalRor *ai, int32_t unit,
                                  int32_t target, float d, int32_t value,
                                  const AoeAiTacticalOpsRor *ops) {
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    return send_order(ops, unit, 0x2bd, target, -1, -1.0f, -1.0f, -1.0f, d,
                      value);
}

int aoe_ai_tactical_order_2cd_ror(AoeAiTacticalRor *ai, int32_t unit,
                                  int32_t target,
                                  const AoeAiTacticalOpsRor *ops) {
    if (unit == -1) return 0;
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    return send_order(ops, unit, 0x2cd, target, ops ? ops->self : 0, -1.0f,
                      -1.0f, -1.0f, 1.0f, 100);
}

void aoe_ai_tactical_share_with_allies_ror(AoeAiTacticalRor *ai, int32_t a,
                                           int32_t b, int32_t c,
                                           const AoeAiTacticalOpsRor *ops) {
    int32_t now, p, count = 0;
    uint8_t stats;
    if (ai->sn[196] != 1 || !ops || !ops->game_time || !ops->player_field_4 ||
        !ops->stats_player)
        return;
    now = (int32_t)ops->game_time(ops->context);
    if ((now - ai->field_fa0) / 1000 < ai->sn[197]) return;
    ai->field_fa0 = now;
    stats = ops->stats_player(ops->context);
    if (stats == 0xff) {
        for (p = 1; p < ops->player_count; ++p)
            if (ops->player_field_4(ops->context, p) == 1) ++count;
        if (count == 1) {
            stats = (uint8_t)ops->self;
            if (ops->set_stats_player) ops->set_stats_player(ops->context, stats);
        } else if (count > 0) {
            /* The original divides by the count unchecked. */
            int32_t pick = (ops->rand ? ops->rand(ops->context) : 0) % count + 1;
            for (p = 1; p < ops->player_count; ++p) {
                if (ops->player_field_4(ops->context, p) == 1 && --pick == 0) {
                    stats = (uint8_t)p;
                    if (ops->set_stats_player)
                        ops->set_stats_player(ops->context, stats);
                }
            }
        }
    }
    for (p = 1; p < ops->player_count; ++p) {
        if (p == ops->self || !ops->player_field_4(ops->context, p)) continue;
        if (!ops->is_ally || !ops->is_ally(ops->context, p)) continue;
        if (!ops->names_us_ally || !ops->names_us_ally(ops->context, p)) continue;
        if ((ops->self < p && p != stats) || ops->self == stats) {
            if (ops->player_54)
                ops->player_54(ops->context, p, ops->self, 2, a, b, c);
        }
    }
}

int aoe_ai_tactical_explored_enough_ror(const AoeAiTacticalRor *ai,
                                        const AoeAiTacticalOpsRor *ops) {
    double share;
    if (ai->sn[167] < 1) return 1;
    share = ops && ops->explored_ratio ? ops->explored_ratio(ops->context) : 0.0;
    return ai->sn[167] <= aoe_ftol32(share * (double)100.0f);
}

/* The resource AI's stock (0x004c8c60 over four resources). */
static int32_t stock(const AoeAiTacticalOpsRor *ops, int32_t k) {
    if (k < 0 || k >= 4 || !ops->resource) return -1;
    return aoe_ftol32(ops->resource(ops->context, k));
}

int32_t aoe_ai_tactical_need_granary_ror(const AoeAiTacticalRor *ai,
                                         int32_t *total, int32_t *unbuilt,
                                         int32_t k,
                                         const AoeAiTacticalOpsRor *ops) {
    int32_t age, v;
    if (!ops || k < 0 || k >= 4) return 0;
    *unbuilt = ops->build_unbuilt ? ops->build_unbuilt(ops->context, 0x67, 0) : 0;
    if (*unbuilt > 0) return 0;
    if (ai->sn[190 + k] <= stock(ops, k)) return 0;
    if (ai->sn[150] != -1) {
        age = aoe_ftol32(ops->resource ? ops->resource(ops->context, 6) : 0.0f);
        if (age == 0) age = 1;
        *total = ops->build_items ? ops->build_items(ops->context, 0x67, 0) : 0;
        v = ai->sn[150] * age - *total;
        return v < 0 ? 0 : v;
    }
    *total = ops->build_items ? ops->build_items(ops->context, 0x67, 0) : 0;
    return ops->target_far ? ops->target_far(ops->context, k) : 0;
}

int32_t aoe_ai_tactical_need_pit_ror(const AoeAiTacticalRor *ai,
                                     int32_t *total, int32_t *unbuilt,
                                     const AoeAiTacticalOpsRor *ops) {
    if (!ops) return 0;
    *unbuilt = ops->build_unbuilt ? ops->build_unbuilt(ops->context, 0x44, 0) : 0;
    if (*unbuilt > 0) return 0;
    if (ai->sn[190] <= stock(ops, 0)) return 0;
    if (ai->sn[151] != -1) {
        *total = ops->build_items ? ops->build_items(ops->context, 0x44, 0) : 0;
        if (ai->sn[151] <= *total) return 0;
    }
    *total = ops->build_items ? ops->build_items(ops->context, 0x44, 0) : 0;
    return ops->food_far ? ops->food_far(ops->context) : 0;
}

/* ---- Candidate collection ---- */

int32_t aoe_ai_tactical_collect_ungrouped_ror(AoeAiTacticalRor *ai,
                                              int32_t list) {
    const AoeAiIntListRor *from;
    int32_t i, n = 0;
    if (!ai || list < 0 || list >= AOE_AI_TAC_LISTS) return 0;
    from = &ai->lists[list];
    ai->list_d48.count = 0;
    for (i = 0; i < from->count; ++i) {
        if (aoe_ai_tactical_group_of_ror(ai, from->data[i])) continue;
        aoe_ai_int_list_add_unique_ror(&ai->list_d48, from->data[i]);
        ++n;
    }
    return n;
}

int32_t aoe_ai_tactical_collect_surplus_ror(AoeAiTacticalRor *ai,
                                            int boats) {
    int32_t g, n = 0;
    if (!ai) return 0;
    ai->list_d48.count = 0;
    for (g = 0; g < ai->groups_len; ++g) {
        const AoeAiUnitGroupRor *grp = ai->groups[g];
        int32_t left, i;
        int fits = boats ? grp->task >= 0x67 && grp->task <= 0x6c
                         : grp->task == 100 || grp->task == 0x65 ||
                               grp->task == 0x66;
        if (!fits || (grp->field_180 != 1 && grp->field_180 != 0)) continue;
        if (!(grp->field_160 < aoe_ai_unit_group_count_ror(grp))) continue;
        left = aoe_ai_unit_group_count_ror(grp);
        for (i = 0; grp->field_160 < left; ++i) {
            int32_t m;
            if (aoe_ai_unit_group_count_ror(grp) <= i) break;
            m = aoe_ai_unit_group_member_ror(grp, i);
            if (m == grp->commander) continue;
            --left;
            aoe_ai_int_list_add_unique_ror(&ai->list_d48, m);
            ++n;
        }
    }
    return n;
}

/* ---- Picking units and spots ---- */

int32_t aoe_ai_tactical_pick_unit_ror(const AoeAiTacticalRor *ai,
                                      const int32_t tile[2], int32_t nearest,
                                      int32_t skip0, int32_t skip1,
                                      int32_t skip2, int32_t allow_103,
                                      const AoeAiTacticalOpsRor *ops) {
    int32_t i, best = -1;
    float best_d = 0.0f;
    if (!ai || !ops || !ops->world_object || !ops->unit_action) return -1;
    for (i = 0; i < ai->lists[0].count; ++i) {
        int32_t id = ai->lists[0].data[i], action;
        AoeAiWorldObjectRor o;
        double d;
        if (aoe_ai_int_list_contains_ror(&ai->list_d58, id)) continue;
        if (!ops->world_object(ops->context, id, &o)) continue;
        if (!ops->unit_action(ops->context, id, &action)) continue;
        if (!allow_103 && o.type_id == 0x103) continue;
        if (action == skip0 || action == skip1 || action == skip2) continue;
        d = ops->distance ? ops->distance(ops->context, id, (float)tile[0],
                                          (float)tile[1], o.z)
                          : 0.0;
        if (best == -1 || (nearest == 1 && d < (double)best_d) ||
            (nearest == 0 && (double)best_d < d)) {
            best = o.id;
            best_d = (float)d;
        }
    }
    return best;
}

int aoe_ai_tactical_explore_spot_ror(AoeAiTacticalRor *ai, int32_t unit,
                                     int32_t out[2],
                                     const AoeAiTacticalOpsRor *ops,
                                     AoeAiInfoRor *info,
                                     const AoeAiInfoOpsRor *info_ops) {
    static const int8_t dx[8] = {-10, 0, 10, 10, 10, 0, -10, -10};
    static const int8_t dy[8] = {-10, -10, -10, 0, 10, 10, 10, 0};
    AoeAiWorldObjectRor u, center;
    int32_t k, best_x = -1, best_y = -1, center_id;
    float best = -1.0f;
    uint8_t zone;
    (void)ai;
    if (!ops || !ops->object || !ops->object(ops->context, unit, &u) ||
        !info || !info_ops || !info_ops->zone || !info_ops->tile_zone)
        return 0;
    zone = info_ops->zone(info_ops->context, unit);
    center_id = ops->first_center ? ops->first_center(ops->context) : -1;
    if (center_id != -1 && ops->object(ops->context, center_id, &center)) {
        for (k = 0; k < 8; ++k) {
            int32_t x = aoe_ftol32(center.x) + dx[k];
            int32_t y = aoe_ftol32(center.y) + dy[k];
            double d;
            if (x < 0 || y < 0) continue;
            if (x >= aoe_ai_info_map_width_ror(info, info_ops)) continue;
            if (y >= aoe_ai_info_map_height_ror(info, info_ops)) continue;
            if (aoe_ai_info_explored_ror(info, x, y, info_ops) == 1) continue;
            if (info_ops->tile_zone(info_ops->context, unit, x, y) != zone) continue;
            d = ops->distance ? ops->distance(ops->context, unit, (float)x,
                                              (float)y, u.z)
                              : 0.0;
            if (!(best != -1.0f) || !(d >= (double)best)) {
                best_x = x;
                best_y = y;
                best = (float)d;
            }
        }
        if (best != -1.0f) {
            out[0] = best_x;
            out[1] = best_y;
            aoe_ai_info_forget_tile_ror(info, best_x, best_y, info_ops);
            return 1;
        }
    }
    return aoe_ai_info_explore_toward_ror(info, unit, out, info_ops) != 0;
}

static int32_t square_min2(int32_t v) { return v < 2 ? 2 : v * v; }

static void tac_box(const AoeAiInfoRor *info, AoeValueGridRor *g, float x, float y, int32_t r,
                    uint8_t value, int lower) {
    int32_t a[2] = {aoe_ftol32(x - (float)r), aoe_ftol32(y - (float)r)};
    int32_t b[2] = {aoe_ftol32((float)r + x), aoe_ftol32((float)r + y)};
    aoe_ai_info_clamp_ror(info, a);
    aoe_ai_info_clamp_ror(info, b);
    if (lower) aoe_value_grid_lower_box_ror(g, a[0], a[1], b[0], b[1], value);
    else aoe_value_grid_set_box_ror(g, a[0], a[1], b[0], b[1], value);
}

void aoe_ai_tactical_block_sites_ror(const AoeAiTacticalRor *ai, const AoeAiInfoRor *info,
                                     AoeValueGridRor *g, const AoeAiInfoOpsRor *ops) {
    int32_t i;
    for (i = 0; i < ai->groups_len; ++i) {
        const AoeAiUnitGroupRor *grp = ai->groups[i];
        int32_t r;
        if (!grp || grp->loc_19c[0] == -1.0f) continue;
        r = square_min2(grp->field_160);
        tac_box(info, g, grp->loc_19c[0], grp->loc_19c[1], r / 2, 0xff, 0);
        tac_box(info, g, grp->loc_19c[0], grp->loc_19c[1], r, 10, 1);
    }
    for (i = 0; i < ai->lists[3].count; ++i) {
        AoeAiWorldObjectRor o;
        int32_t c[2], d[2];
        memset(&o, 0, sizeof o);
        if (!ops || !ops->object || !ops->object(ops->context, ai->lists[3].data[i], &o)) continue;
        c[0] = aoe_ftol32(o.x), c[1] = aoe_ftol32(o.y);
        aoe_ai_info_clamp_ror(info, c);
        d[0] = aoe_ftol32(o.x), d[1] = aoe_ftol32(o.y);
        aoe_ai_info_clamp_ror(info, d);
        aoe_value_grid_set_box_ror(g, c[0], c[1], d[0], d[1], 0xff);
        tac_box(info, g, o.x, o.y, square_min2(1), 10, 1);
    }
}

AoeAiUnitGroupRor *aoe_ai_tactical_nearest_group_ror(
    const AoeAiTacticalRor *ai, int32_t task, int32_t field_180, int32_t max,
    const int32_t pt[2], int32_t zone, const AoeAiTacticalOpsRor *ops) {
    AoeAiUnitGroupRor *best = NULL;
    int32_t best_d = -1, i, j;
    if (!ai) return NULL;
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        int32_t dx, dy, d;
        if (task != -1 && g->task != task) continue;
        if (field_180 != -1 && g->field_180 != field_180) continue;
        if (task == 0x6a) {
            for (j = 0; j < ai->groups_len; ++j)
                if (ai->groups[j]->field_1c4 == g->id) break;
            if (j < ai->groups_len) continue;
        }
        if (zone != -1) {
            uint8_t z;
            if (!ops || !ops->object_zone ||
                !ops->object_zone(ops->context, g->commander, &z) || z != zone)
                continue;
        }
        dy = pt[1] - aoe_ftol32(g->loc_19c[1]);
        dx = pt[0] - aoe_ftol32(g->loc_19c[0]);
        d = aoe_ftol32(sqrt((double)(dy * dy + dx * dx)));
        if ((d < max && best_d == -1) || max == -1 || d < best_d) {
            best = g;
            best_d = d;
        }
    }
    return best;
}

int aoe_ai_tactical_step_around_ror(const AoeAiTacticalOpsRor *ops, int32_t unit,
                                    float x0, float y0, float x1, float y1) {
    AoeAiWorldObjectRor u;
    float x = -1.0f, y = -1.0f, best = -1.0f, bx = -1.0f, by = -1.0f;
    int i;
    if (!ops || !aoe_ai_tactical_unit_has_task_ror(ops, unit, -1) || !ops->object ||
        !ops->object(ops->context, unit, &u))
        return 0;
    for (i = 0; i < 8; ++i) {
        switch (i) {
        case 0: x = x0 - 2.0f; y = y0 - 2.0f; break;
        case 1: case 5: x = (x0 + x1) * 0.5f; break;
        case 2: x = x1 + 2.0f; break;
        case 3: case 7: y = (y0 + y1) * 0.5f; break;
        case 4: y = y1 + 2.0f; break;
        default: x = x0 - 2.0f; break;
        }
        if (ops->free_tile && ops->free_tile(ops->context, unit, x, y)) {
            double d = ops->distance ? ops->distance(ops->context, unit, x, y, u.z) : 0.0;
            if (best == -1.0f || d < (double)best) {
                best = (float)d;
                bx = x;
                by = y;
            }
        }
    }
    if (best == -1.0f) return 0;
    aoe_ai_tactical_order_move_ror(ops, unit, bx, by, 100);
    return 1;
}

void aoe_ai_tactical_unit_added_ror(AoeAiTacticalRor *ai,
                                    const AoeAiWorldObjectRor *unit,
                                    const AoeAiTacticalOpsRor *ops) {
    int16_t c;
    if (!ai || !unit) return;
    c = unit->type_class;
    switch (c) {
    case 4:
        aoe_ai_int_list_add_unique_ror(&ai->lists[0], unit->id);
        return;
    case 0: case 0x18: case 6: case 0x19: case 0x1a: case 0xc: case 0x23:
    case 0x17: case 0x24: case 0x1c: case 0x12: case 0xd: case 0x27:
        aoe_ai_int_list_add_unique_ror(&ai->lists[2], unit->id);
        return;
    case 2: case 0x14: case 0x16: case 0x15:
        aoe_ai_int_list_add_unique_ror(&ai->lists[4], unit->id);
        if (c == 2) aoe_ai_int_list_add_unique_ror(&ai->lists[7], unit->id);
        if (c == 0x14) aoe_ai_int_list_add_unique_ror(&ai->lists[8], unit->id);
        if (c == 0x16) aoe_ai_int_list_add_unique_ror(&ai->lists[5], unit->id);
        if (c == 0x15) aoe_ai_int_list_add_unique_ror(&ai->lists[6], unit->id);
        return;
    default:
        if (unit->type_id == 0x9f && ops && ops->world_100 &&
            ops->world_100(ops->context) < 4)
            aoe_ai_int_list_add_unique_ror(&ai->lists[9], unit->id);
        return;
    }
}

void aoe_ai_tactical_unit_removed_ror(AoeAiTacticalRor *ai, int32_t unit,
                                      const AoeAiTacticalOpsRor *ops,
                                      const AoeAiUnitGroupOpsRor *group_ops) {
    if (!ai) return;
    aoe_ai_tactical_drop_unit_ror(ai, unit, ops);
    if (aoe_ai_int_list_contains_ror(&ai->lists[0], unit)) {
        aoe_ai_int_list_remove_ror(&ai->lists[0], unit);
    } else if (aoe_ai_int_list_contains_ror(&ai->lists[2], unit)) {
        aoe_ai_int_list_remove_ror(&ai->lists[2], unit);
    } else if (aoe_ai_int_list_contains_ror(&ai->lists[4], unit)) {
        int k;
        aoe_ai_int_list_remove_ror(&ai->lists[4], unit);
        for (k = 5; k <= 8; ++k)
            if (aoe_ai_int_list_contains_ror(&ai->lists[k], unit)) {
                aoe_ai_int_list_remove_ror(&ai->lists[k], unit);
                break;
            }
    } else if (aoe_ai_int_list_contains_ror(&ai->lists[9], unit)) {
        aoe_ai_int_list_remove_ror(&ai->lists[9], unit);
    }
    aoe_ai_tactical_ungroup_ror(ai, unit, group_ops);
}

/* One unit of the path collections: on +0xd48 (counted) when it exists
   with a unit AI, is not on +0xd58 and its AI's task and +0x2c match. */
static int32_t collect_one(AoeAiTacticalRor *ai, int32_t id, int32_t n, int32_t mode,
                           const AoeAiTacticalOpsRor *ops, int16_t *type_class) {
    int32_t task, count = 0;
    AoeAiWorldObjectRor o;
    int has_task, take;
    if (!ops || !ops->world_object || !ops->world_object(ops->context, id, &o) ||
        !ops->unit_task || !ops->unit_task(ops->context, id, &task))
        return 0;
    if (aoe_ai_int_list_contains_ror(&ai->list_d58, id)) return 0;
    if (type_class && *type_class != 0 && o.type_class != 0x12 && o.type_class != 0xd)
        return 0;
    has_task = task != -1;
    if (has_task && ops->unit_2c) ops->unit_2c(ops->context, id, &count);
    if (mode < 0) take = !has_task || count < n;
    else if (mode < 1) take = has_task && count == n;
    else take = has_task && n < count;
    if (!take) return 0;
    aoe_ai_int_list_add_unique_ror(&ai->list_d48, id);
    return 1;
}

int32_t aoe_ai_tactical_collect_by_order_ror(AoeAiTacticalRor *ai, int32_t n,
                                            int32_t mode, int32_t reset,
                                            const AoeAiTacticalOpsRor *ops) {
    int32_t i, taken = 0, *v;
    if (!ai) return 0;
    if (reset == 1) ai->list_d48.count = 0;
    for (i = 0; i < ai->lists[2].count; ++i)
        if ((v = aoe_ai_int_list_at_ror(&ai->lists[2], i)))
            taken += collect_one(ai, *v, n, mode, ops, NULL);
    return taken;
}

int32_t aoe_ai_tactical_collect_support_ror(AoeAiTacticalRor *ai, int32_t n,
                                            int32_t mode,
                                            const AoeAiTacticalOpsRor *ops) {
    int32_t i, taken = 0, *v;
    int16_t support = 1;
    if (!ai) return 0;
    ai->list_d48.count = 0;
    for (i = 0; i < ai->lists[5].count; ++i)
        if ((v = aoe_ai_int_list_at_ror(&ai->lists[5], i)))
            taken += collect_one(ai, *v, n, mode, ops, NULL);
    for (i = 0; i < ai->lists[2].count; ++i)
        if ((v = aoe_ai_int_list_at_ror(&ai->lists[2], i)))
            taken += collect_one(ai, *v, n, mode, ops, &support);
    return taken;
}

/* No AI task and no action. */
static int villager_idle(const AoeAiTacticalOpsRor *ops, int32_t id) {
    int32_t task, action;
    if (!ops->unit_task || !ops->unit_task(ops->context, id, &task) || task != -1) return 0;
    return ops->unit_action && ops->unit_action(ops->context, id, &action) && action == -1;
}

int32_t aoe_ai_tactical_idle_villager_ror(AoeAiTacticalRor *ai, int32_t mode,
                                          const AoeAiTacticalOpsRor *ops) {
    int32_t pass, i, *v, task;
    AoeAiWorldObjectRor o;
    if (!ai || !ops || !ops->world_object || !ops->unit_task) return -1;
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < ai->lists[0].count; ++i) {
            int32_t id;
            if (!(v = aoe_ai_int_list_at_ror(&ai->lists[0], i))) continue;
            id = *v;
            if (aoe_ai_int_list_contains_ror(&ai->list_d58, id)) continue;
            if (!ops->world_object(ops->context, id, &o) ||
                !ops->unit_task(ops->context, id, &task))
                continue;
            if (pass == 0 || mode != 0) {
                if ((pass == 0 || mode != 3) && aoe_ai_tactical_has_gatherer_ror(ai, id))
                    continue;
                if ((pass == 0 || mode != 1) && aoe_ai_int_list_contains_ror(&ai->lists[1], id))
                    continue;
            }
            if (villager_idle(ops, id)) return o.id;
        }
    }
    return -1;
}

/* The villager filter of 0x004d90b0 / 0x004d93a0 for one entry; the
   object goes to `o`. */
static int villager_ok(AoeAiTacticalRor *ai, int32_t id, int pass, int32_t mode,
                       const AoeAiTacticalOpsRor *ops, AoeAiWorldObjectRor *o) {
    int32_t task;
    if (aoe_ai_int_list_contains_ror(&ai->list_d58, id)) return 0;
    if (!ops->world_object(ops->context, id, o) || !ops->unit_task(ops->context, id, &task))
        return 0;
    if (pass == 0 || mode != 0) {
        if ((pass == 0 || mode != 3) && aoe_ai_tactical_has_gatherer_ror(ai, id)) return 0;
        if ((pass == 0 || mode != 1) && aoe_ai_int_list_contains_ror(&ai->lists[1], id))
            return 0;
    }
    return villager_idle(ops, id);
}

int32_t aoe_ai_tactical_villager_near_ror(AoeAiTacticalRor *ai, int32_t mode,
                                          const int32_t pt[2], int32_t nearest,
                                          const AoeAiTacticalOpsRor *ops) {
    int32_t i, *v, best = -1;
    float best_d = 0.0f;
    AoeAiWorldObjectRor o;
    if (!ai || !ops || !ops->world_object || !ops->unit_task) return -1;
    for (i = 0; i < ai->lists[0].count; ++i)
        if ((v = aoe_ai_int_list_at_ror(&ai->lists[0], i)) &&
            villager_ok(ai, *v, 0, mode, ops, &o))
            return o.id;
    for (i = 0; i < ai->lists[0].count; ++i) {
        double d;
        if (!(v = aoe_ai_int_list_at_ror(&ai->lists[0], i)) ||
            !villager_ok(ai, *v, 1, mode, ops, &o))
            continue;
        d = ops->distance ? ops->distance(ops->context, o.id, (float)pt[0], (float)pt[1], o.z)
                          : 0.0;
        if (best == -1 || (nearest == 1 && d < (double)best_d) ||
            (nearest == 0 && (double)best_d < d)) {
            best = o.id;
            best_d = (float)d;
        }
    }
    return best;
}

void aoe_ai_tactical_gather_order_ror(AoeAiTacticalRor *ai,
                                      const AoeAiTacticalOpsRor *ops) {
    int32_t picked[4] = {0}, order[4], value[4], round, i;
    if (!ai || !ops) return;
    if (ops->build_costs) ops->build_costs(ops->context, &ai->counters, ai->sn[147]);
    for (i = 0; i < 4; ++i)
        ai->field_d98[i] = (ops->resource_amount ? ops->resource_amount(ops->context, i) : 0) -
                           aoe_ai_counters_get_ror(&ai->counters, i);
    for (round = 0; round < 4; ++round) {
        int32_t best = -1, best_v = 0x7fffffff;
        for (i = 0; i < 4; ++i) {
            int32_t have = ops->resource_amount ? ops->resource_amount(ops->context, i) : 0;
            if (!(have < ai->sn[190 + i])) continue;
            if (aoe_ai_counters_get_ror(&ai->counters, i) > 0 &&
                (best == -1 || ai->field_d98[i] < best_v) && !picked[i]) {
                best_v = ai->field_d98[i];
                best = i;
            }
        }
        if (best == -1)
            for (i = 0; i < 4; ++i)
                if ((best == -1 || ai->field_d98[i] < best_v) && !picked[i]) {
                    best_v = ai->field_d98[i];
                    best = i;
                }
        picked[best] = 1;
        order[round] = best;
        value[round] = best_v;
    }
    for (i = 0; i < 4; ++i) {
        ai->field_d88[i] = order[i];
        ai->field_d98[order[i]] = value[i];
    }
}

/* Gatherer entries of another kind than 0, -1 (and `keep`) get +8 = 5. */
static void park_gatherers(AoeAiTacticalRor *ai, int32_t keep) {
    int32_t i;
    for (i = 0; i < ai->field_9cc && i < AOE_AI_TAC_GATHERERS; ++i) {
        int32_t k = ai->gatherers[i][1];
        if (k != 0 && k != keep && k != -1) ai->gatherers[i][2] = 5;
    }
}

void aoe_ai_tactical_plan_gatherers_ror(AoeAiTacticalRor *ai,
                                        const AoeAiTacticalOpsRor *ops) {
    int32_t i, total, deficit = 0, short_n = 0, n, left;
    int split = 0;
    if (!ai || !ops) return;
    for (i = 0; i < 4; ++i) ai->field_d68[i] = 0;
    for (i = 0; i < ai->field_9cc && i < AOE_AI_TAC_GATHERERS; ++i) {
        int32_t *e = ai->gatherers[i], kind = e[1], task, target;
        AoeAiWorldObjectRor o;
        if (!ops->object || !ops->object(ops->context, e[0], &o) || !ops->unit_task ||
            !ops->unit_task(ops->context, e[0], &task))
            continue;
        if (ops->unit_ai_target && ops->unit_ai_target(ops->context, e[0], &target))
            e[4] = target;
        if (!(task == 0x261 && o.type_id == 0x103 && kind == 0)) {
            if (task == 0x261 && o.type_id == 0x103 &&
                !aoe_ai_int_list_contains_ror(&ai->list_d58, o.id))
                e[1] = 0;
            if (kind < 0) continue;
        }
        if (kind >= 4) continue;
        if (task == 0x261 || task == 0x265 ||
            aoe_ai_int_list_contains_ror(&ai->list_d58, e[0]))
            ++ai->field_d68[kind];
    }
    for (i = 0; i < 4; ++i) ai->field_d78[i] = 0;
    if (aoe_ai_tactical_age_is_ror(ops, 1)) {
        if (ai->sn[181] == 1) {
            int32_t far = ops->info_best_distance ? ops->info_best_distance(ops->context, 0) : -1;
            int32_t wood = ops->resource_amount ? ops->resource_amount(ops->context, 1) : 0;
            split = 1;
            if (ops->age_open && ops->age_open(ops->context, 2) && far != -1 &&
                (wood > 0x78 || ai->field_f8c == 1)) {
                ai->field_d78[0] = ai->field_9d0;
                park_gatherers(ai, 0);
                ai->field_fa4 = 1;
            } else {
                ai->field_fa4 = 0;
                ai->field_d78[1] = ai->field_9d0;
            }
        } else {
            ai->field_fa4 = 0;
        }
    } else if (aoe_ai_tactical_age_is_ror(ops, 2)) {
        if (ai->sn[182] == 1) {
            int32_t wood = ops->resource_amount ? ops->resource_amount(ops->context, 1) : 0;
            split = 1;
            if (ops->age_open && ops->age_open(ops->context, 3) &&
                (wood > 0xf0 ||
                 ((ops->resource_amount ? ops->resource_amount(ops->context, 1) : 0) > 0x78 &&
                  (ops->info_best_distance ? ops->info_best_distance(ops->context, 0) : -1) <
                      0x14))) {
                ai->field_d78[0] = ai->field_9d0;
                park_gatherers(ai, 0);
                ai->field_fa4 = 1;
            } else {
                ai->field_fa4 = 0;
                ai->field_d78[1] = ai->field_9d0;
            }
        } else {
            ai->field_fa4 = 0;
        }
    } else if (aoe_ai_tactical_age_is_ror(ops, 3)) {
        if (ai->sn[183] == 1) {
            split = 1;
            if (!ops->age_open || !ops->age_open(ops->context, 4)) {
                int32_t v = ai->field_9d0 / 3;
                ai->field_fa4 = 0;
                ai->field_d78[0] = ai->field_d78[3] = ai->field_d78[1] = v;
            } else {
                int32_t v = ai->field_9d0 / 2;
                ai->field_d78[0] = ai->field_d78[3] = v;
                park_gatherers(ai, 3);
                ai->field_fa4 = 1;
            }
        } else {
            ai->field_fa4 = 0;
        }
    }
    if (!split) {
        n = ai->field_9d0;
        ai->field_d78[2] = (ai->sn[158] + ai->sn[119]) * n / 100;
        ai->field_d78[3] = (ai->sn[159] + ai->sn[118]) * n / 100;
        ai->field_d78[1] = (ai->sn[157] + ai->sn[120]) * n / 100;
        ai->field_d78[0] = (ai->sn[156] + ai->sn[117]) * n / 100;
    }
    total = ai->field_d78[0] + ai->field_d78[1] + ai->field_d78[2] + ai->field_d78[3];
    n = ai->field_9d0;
    for (i = 0; i < 4; ++i)
        if (ai->field_d98[i] < 0) {
            deficit -= ai->field_d98[i];
            ++short_n;
        }
    for (i = 0; i < short_n; ++i) {
        int32_t r = ai->field_d88[i];
        if (r < 0 || r > 3) continue;
        ai->field_d78[r] -= ai->field_d98[r] * (n - total) / deficit;
    }
    total = ai->field_d78[0] + ai->field_d78[1] + ai->field_d78[2] + ai->field_d78[3];
    left = ai->field_9d0 - total;
    if (left > 0) {
        ai->field_d78[0] += left / 2 + (left % 2 == 1);
        ai->field_d78[1] += left / 2;
    }
}

int aoe_ai_tactical_task_unit_ror(AoeAiTacticalRor *ai, int32_t unit, int32_t a,
                                  int32_t b, int32_t c, int32_t d,
                                  const float *waypoints, int32_t count,
                                  const AoeAiTacticalOpsRor *ops) {
    int32_t i;
    if (!ai) return 1;
    if (waypoints)
        for (i = 0; i < count; ++i)
            aoe_ai_tactical_player_44_ror(ops, unit, (uint8_t)aoe_ftol32(waypoints[4 * i]),
                                          (uint8_t)aoe_ftol32(waypoints[4 * i + 1]));
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    if (ops && ops->player_4c) ops->player_4c(ops->context, unit, a, b, c, d);
    return 1;
}

static void free_site(const AoeAiTacticalOpsRor *ops, AoeAiConItemRor *site) {
    if (site && ops->con_free) ops->con_free(ops->context, site);
}

int aoe_ai_tactical_start_build_ror(AoeAiTacticalRor *ai, int32_t builder,
                                    AoeAiBuildItemRor *item, AoeAiConItemRor *site,
                                    int32_t arg, const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor b;
    int32_t task, sx, sy;
    if (!ai || !ops) return 0;
    if (!item && !(item = ops->build_next ? ops->build_next(ops->context) : NULL)) return 0;
    if (!site && !(site = ops->con_choose ? ops->con_choose(ops->context, item, builder,
                                                            ai->block_e4c, arg)
                                          : NULL)) {
        /* A suspended search resumes with this item (the original's
           +0xf78). */
        if (ai->block_e4c[69] == 1) ai->search_item = item;
        return 0;
    }
    if (builder == -1) {
        int32_t tile[2];
        tile[0] = aoe_ftol32(aoe_ai_plan_item_float_ror(&site->plan, AOE_AI_ITEM_X));
        tile[1] = aoe_ftol32(aoe_ai_plan_item_float_ror(&site->plan, AOE_AI_ITEM_Y));
        builder = aoe_ai_tactical_pick_unit_ror(ai, tile, 1, 0x2be, 0x2ce, 0x2bc, 0, ops);
    }
    if (!ops->world_object || !ops->world_object(ops->context, builder, &b) ||
        !ops->unit_task || !ops->unit_task(ops->context, builder, &task)) {
        free_site(ops, site);
        return 0;
    }
    sx = aoe_ftol32(aoe_ai_plan_item_float_ror(&site->plan, AOE_AI_ITEM_X));
    sy = aoe_ftol32(aoe_ai_plan_item_float_ror(&site->plan, AOE_AI_ITEM_Y));
    if (!ops->can_reach ||
        !ops->can_reach(ops->context, builder, sx, sy, aoe_ftol32(b.z), 2.0f, -1)) {
        free_site(ops, site);
        return 0;
    }
    aoe_ai_build_item_put_ror(item, AOE_AI_BUILD_IN_PROGRESS, 1);
    aoe_ai_build_item_put_ror(item, AOE_AI_BUILD_BUILT, 0);
    aoe_ai_int_list_remove_ror(&ai->lists[1], builder);
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, builder);
    if (aoe_ai_plan_item_type_ror(&site->plan) == 0x114) ai->field_e44 = 1;
    if (ops->build_order)
        ops->build_order(ops->context, builder, aoe_ai_plan_item_type_ror(&site->plan),
                         aoe_ai_plan_item_float_ror(&site->plan, AOE_AI_ITEM_X),
                         aoe_ai_plan_item_float_ror(&site->plan, AOE_AI_ITEM_Y),
                         aoe_ai_plan_item_unique_id_ror(&item->plan));
    free_site(ops, site);
    return 1;
}

static int order_on(AoeAiTacticalRor *ai, int32_t unit, const AoeAiWorldObjectRor *target,
                    int32_t action, int32_t flag, const AoeAiTacticalOpsRor *ops) {
    AoeAiTaskOrderRor o;
    double d;
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    d = ops->object_distance ? ops->object_distance(ops->context, unit, target->id) : 0.0;
    o.player = ops->self;
    o.unit = unit;
    o.action = action;
    o.target = target->id;
    o.target2 = ops->self;
    o.a = target->x;
    o.b = target->y;
    o.c = target->z;
    o.d = 1.0f;
    o.flag = 1;
    o.zero = flag;
    o.value = aoe_ai_tactical_effort_ror(ai, (float)d);
    if (ops->order) ops->order(ops->context, &o);
    return 1;
}

int aoe_ai_tactical_order_help_build_ror(AoeAiTacticalRor *ai, int32_t unit,
                                         const AoeAiWorldObjectRor *target,
                                         const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor u;
    if (!ai || !ops || !ops->world_object || !ops->world_object(ops->context, unit, &u))
        return 0;
    if (!target) return 0;
    if (!ops->can_reach ||
        !ops->can_reach(ops->context, unit, aoe_ftol32(target->x), aoe_ftol32(target->y),
                        aoe_ftol32(target->z), 1.0f, target->id)) {
        if (target->type_id != 0x114 && ops->delete_object)
            ops->delete_object(ops->context, target->id);
        return 0;
    }
    return order_on(ai, unit, target, 0x2be, 1, ops);
}

int aoe_ai_tactical_order_repair_ror(AoeAiTacticalRor *ai, int32_t unit,
                                     const AoeAiWorldObjectRor *target,
                                     const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor u;
    if (!ai || !ops || !ops->world_object || !ops->world_object(ops->context, unit, &u) ||
        !target)
        return 0;
    if (!ops->can_engage || !ops->can_engage(ops->context, unit, target->id, 1.0f)) return 0;
    return order_on(ai, unit, target, 0x2ce, 0, ops);
}

int aoe_ai_tactical_order_explore_ror(AoeAiTacticalRor *ai, int32_t unit, float x,
                                      float y, int32_t explorer,
                                      const AoeAiTacticalOpsRor *ops) {
    AoeAiTaskOrderRor o;
    if (!ai) return 1;
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    if (explorer == 1) {
        aoe_ai_int_list_add_unique_ror(&ai->lists[1], unit);
        aoe_ai_tactical_remove_gatherer_ror(ai, unit);
    }
    if (!ops || !ops->order) return 1;
    o.player = ops->self;
    o.unit = unit;
    o.action = 0x2c1;
    o.target = o.target2 = -1;
    o.a = x;
    o.b = y;
    o.c = -1.0f;
    o.d = 1.0f;
    o.flag = 1;
    o.zero = 0;
    o.value = 1;
    ops->order(ops->context, &o);
    return 1;
}

int aoe_ai_tactical_order_to_object_ror(AoeAiTacticalRor *ai, int32_t unit,
                                        int32_t target,
                                        const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor u, t;
    if (!ai || !ops || !ops->object || !ops->object(ops->context, unit, &u) || unit == -1 ||
        !ops->object(ops->context, target, &t))
        return 0;
    if (!ops->can_reach ||
        !ops->can_reach(ops->context, unit, aoe_ftol32(t.x), aoe_ftol32(t.y), aoe_ftol32(t.z),
                        2.0f, target))
        return 0;
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    if (ops->player_124) ops->player_124(ops->context, unit, target, t.owner, t.x, t.y);
    return 1;
}

int aoe_ai_tactical_order_point_ror(AoeAiTacticalRor *ai, int32_t unit, int32_t target,
                                    float x, float y, const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor u;
    if (!ai || !ops || !ops->object || !ops->object(ops->context, unit, &u) || unit == -1)
        return 0;
    if (!ops->can_reach ||
        !ops->can_reach(ops->context, unit, aoe_ftol32(x), aoe_ftol32(y), 1, 1.0f, -1))
        return 0;
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    if (ops->player_124) ops->player_124(ops->context, unit, target, 0, x, y);
    return 1;
}

int aoe_ai_tactical_order_attack_ror(AoeAiTacticalRor *ai, int32_t unit, float x, float y,
                                     int32_t target, int32_t target2,
                                     const float *waypoints, int32_t count,
                                     int32_t kind, const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor u, t;
    int32_t task, i;
    if (!ai || !ops || unit == -1 || !ops->world_object ||
        !ops->world_object(ops->context, unit, &u) || !ops->unit_task ||
        !ops->unit_task(ops->context, unit, &task))
        return 0;
    if (waypoints)
        for (i = 0; i < count; ++i)
            aoe_ai_tactical_player_44_ror(ops, unit, (uint8_t)aoe_ftol32(waypoints[4 * i]),
                                          (uint8_t)aoe_ftol32(waypoints[4 * i + 1]));
    if (target == -1) {
        double d = ops->distance ? ops->distance(ops->context, unit, x, y, 1.0f) : 0.0;
        aoe_ai_tactical_order_move_ror(ops, unit, x, y, aoe_ai_tactical_effort_ror(ai, (float)d));
    } else {
        AoeAiTaskOrderRor o;
        double d;
        if (!ops->world_object(ops->context, target, &t)) return 0;
        d = ops->object_distance ? ops->object_distance(ops->context, unit, target) : 0.0;
        o.value = aoe_ai_tactical_effort_ror(ai, (float)d);
        o.player = ops->self;
        o.unit = unit;
        o.action = kind == 1 ? 0x2d9 : 0x2bc;
        o.target = target;
        o.target2 = target2;
        o.a = x;
        o.b = y;
        o.c = 1.0f;
        o.d = ops->range ? (float)ops->range(ops->context, unit) : 0.0f;
        o.flag = 1;
        o.zero = 0;
        if (ops->order) ops->order(ops->context, &o);
    }
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    return 1;
}

static int produce(AoeAiTacticalRor *ai, AoeAiBuildItemRor *item,
                   const AoeAiTacticalOpsRor *ops,
                   void (*send)(void *, int32_t, int32_t, int32_t)) {
    int32_t producer, task;
    if (!ai || !item || !ops || !ops->find_producer) return 0;
    producer = ops->find_producer(ops->context,
                                  aoe_ai_build_item_get_ror(item, AOE_AI_BUILD_FROM));
    if (producer == -1 || !ops->unit_task || !ops->unit_task(ops->context, producer, &task))
        return 0;
    aoe_ai_build_item_put_ror(item, AOE_AI_BUILD_IN_PROGRESS, 1);
    aoe_ai_build_item_put_ror(item, AOE_AI_BUILD_BUILT, 0);
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, producer);
    if (send)
        send(ops->context, aoe_ai_plan_item_type_ror(&item->plan), producer,
             aoe_ai_plan_item_unique_id_ror(&item->plan));
    return 1;
}

int aoe_ai_tactical_train_ror(AoeAiTacticalRor *ai, AoeAiBuildItemRor *item,
                              const AoeAiTacticalOpsRor *ops) {
    return produce(ai, item, ops, ops ? ops->train_order : NULL);
}

int aoe_ai_tactical_research_ror(AoeAiTacticalRor *ai, AoeAiBuildItemRor *item,
                                 const AoeAiTacticalOpsRor *ops) {
    return produce(ai, item, ops, ops ? ops->research_order : NULL);
}

int aoe_ai_tactical_order_2ca_ror(AoeAiTacticalRor *ai, int32_t unit, float x, float y,
                                  const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor u;
    AoeAiTaskOrderRor o;
    double d;
    if (!ai || !ops || !ops->object || !ops->object(ops->context, unit, &u) || unit == -1)
        return 0;
    d = ops->distance ? ops->distance(ops->context, unit, x, y, 1.0f) : 0.0;
    o.value = aoe_ai_tactical_effort_ror(ai, (float)d);
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    o.player = ops->self;
    o.unit = unit;
    o.action = 0x2ca;
    o.target = -1;
    o.target2 = ops->self;
    o.a = x;
    o.b = y;
    o.c = o.d = 1.0f;
    o.flag = 1;
    o.zero = 0;
    if (ops->order) ops->order(ops->context, &o);
    return 1;
}

/* Retask (or make) a soldier group. */
static void soldier_group(AoeAiUnitGroupRor *g, int32_t task, int32_t size, int full) {
    g->task = task;
    if (full) {
        g->field_184 = -1;
        g->field_188 = -1;
        g->field_180 = 1;
        g->type = 0;
    }
    g->field_160 = size;
}

static int32_t ceil_div(int32_t a, int32_t b) {
    return aoe_ftol32(ceil((double)a / (double)b));
}

void aoe_ai_tactical_setup_soldier_groups_ror(AoeAiTacticalRor *ai,
                                              const AoeAiTacticalOpsRor *ops,
                                              const AoeAiUnitGroupOpsRor *group_ops) {
    AoeAiIntListRor doomed;
    int32_t i, k, now, n100, n65, n66, w100 = 0, w65 = 0, w66 = 0, m, rest, *first;
    if (!ai || !ops) return;
    memset(&doomed, 0, sizeof doomed);
    for (i = 0; i < ai->groups_len; ++i)
        if (aoe_ai_unit_group_count_ror(ai->groups[i]) == 0)
            aoe_ai_int_list_add_unique_ror(&doomed, ai->groups[i]->id);
    now = ops->game_time ? (int32_t)ops->game_time(ops->context) : 0;
    if ((uint32_t)(now - ai->field_d0c) / 1000 > 600 &&
        (uint32_t)(now - ai->field_d10) / 1000 > 600) {
        ai->field_d10 = now;
        for (i = 0; i < ai->groups_len; ++i)
            if (ai->groups[i]->task == 100)
                aoe_ai_int_list_add_unique_ror(&doomed, ai->groups[i]->id);
    }
    for (k = 0; k < doomed.count; ++k) aoe_ai_tactical_delete_group_ror(ai, doomed.data[k]);
    first = aoe_ai_int_list_at_ror(&ai->lists[2], 0);
    if (ops->info_zone_open && !ops->info_zone_open(ops->context, first ? *first : 0)) {
        aoe_ai_tactical_delete_groups_ror(ai, 0x66);
        aoe_ai_tactical_set_sn_ror(ai, 0x2a, 0, NULL, NULL);
    }
    n100 = aoe_ai_tactical_count_groups_ror(ai, 100, -1);
    n65 = aoe_ai_tactical_count_groups_ror(ai, 0x65, -1);
    n66 = aoe_ai_tactical_count_groups_ror(ai, 0x66, -1);
    aoe_ai_tactical_collect_ungrouped_ror(ai, 9);
    while (ai->list_d48.count > 0) {
        int32_t *v = aoe_ai_int_list_at_ror(&ai->list_d48, k), id, before, centre;
        AoeAiWorldObjectRor u, c;
        if (!v) break;
        id = *v;
        before = ai->list_d48.count;
        centre = ops->first_center ? ops->first_center(ops->context) : -1;
        if (ops->object && ops->object(ops->context, id, &u) && centre != -1 &&
            ops->object(ops->context, centre, &c)) {
            AoeAiUnitGroupRor *g = aoe_ai_tactical_new_group_ror(ai, 1);
            if (!g) break;
            g->task = 0x6d;
            g->field_160 = 1;
            aoe_ai_unit_group_add_ror(g, id, group_ops);
            aoe_ai_unit_group_set_commander_ror(g, id, group_ops);
            aoe_ai_unit_group_set_location_ror(g->location, u.x, u.y, u.z);
            aoe_ai_unit_group_set_location_ror(g->loc_19c, u.x, u.y, u.z);
            aoe_ai_unit_group_set_location_ror(g->loc_1ac, c.x, c.y, c.z);
        }
        aoe_ai_int_list_remove_ror(&ai->list_d48, id);
        if (ai->list_d48.count == before) break;
    }
    m = ai->lists[2].count;
    if (ai->sn[40] == 0) {
        w100 = ceil_div(m, ai->sn[16]);
        if (w100 > ai->sn[36]) w100 = ai->sn[36];
        rest = m - ai->sn[16] * w100;
        if (rest > 0) {
            w65 = ceil_div(rest, ai->sn[25]);
            if (w65 > ai->sn[38]) w65 = ai->sn[38];
            rest = m - ai->sn[25] * w65 - ai->sn[16] * w100;
            if (rest > 0) {
                w66 = ceil_div(rest, ai->sn[43]);
                if (w66 > ai->sn[42]) w66 = ai->sn[42];
            }
        }
    } else {
        w100 = m < ai->sn[36] ? m : ai->sn[36];
        if (m - w100 > 0) {
            w65 = m - w100 < ai->sn[38] ? m - w100 : ai->sn[38];
            if (m - w65 - w100 > 0)
                w66 = m - w65 - w100 < ai->sn[42] ? m - w65 - w100 : ai->sn[42];
        }
    }
    if (ai->sn[162] == 1) w100 = 0;
    for (; n100 < w100; ++n100) {
        AoeAiUnitGroupRor *g = NULL;
        int32_t size;
        if (n65 > 0) {
            if ((g = aoe_ai_tactical_find_group_ror(ai, -1, 0x65, -1, -1))) --n65;
            else if ((g = aoe_ai_tactical_find_group_ror(ai, -1, 0x66, -1, -1))) --n66;
        }
        if (!g && !(g = aoe_ai_tactical_new_group_ror(ai, 1))) break;
        size = (ops->rand ? ops->rand(ops->context) : 0) % (ai->sn[98] + 1) + ai->sn[16];
        if (size > ai->sn[26]) size = ai->sn[26];
        soldier_group(g, 100, size, 1);
    }
    while (n100 > w100) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_find_group_ror(ai, -1, 100, 0, -1);
        if (!g) g = aoe_ai_tactical_find_group_ror(ai, -1, 100, 1, -1);
        if (!g) break;
        aoe_ai_tactical_delete_group_ror(ai, g->id);
        --n100;
    }
    for (; n65 < w65; ++n65) {
        AoeAiUnitGroupRor *g = NULL;
        if (n66 > 0 && (g = aoe_ai_tactical_find_group_ror(ai, -1, 0x66, -1, -1))) --n66;
        if (!g && !(g = aoe_ai_tactical_new_group_ror(ai, 1))) break;
        soldier_group(g, 0x65, ai->sn[25], 1);
    }
    while (n65 > w65) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_find_group_ror(ai, -1, 0x65, -1, -1);
        if (!g) break;
        aoe_ai_tactical_delete_group_ror(ai, g->id);
        --n65;
    }
    for (; n66 < w66; ++n66) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_new_group_ror(ai, 1);
        if (!g) break;
        soldier_group(g, 0x66, ai->sn[43], 0);
    }
    while (n66 > w66) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_find_group_ror(ai, -1, 0x66, -1, -1);
        if (!g) break;
        aoe_ai_tactical_delete_group_ror(ai, g->id);
        --n66;
    }
    free(doomed.data);
}

void aoe_ai_tactical_send_explorer_ror(AoeAiTacticalRor *ai, int32_t unit,
                                       int32_t explore,
                                       const AoeAiTacticalOpsRor *ops,
                                       AoeAiInfoRor *info,
                                       const AoeAiInfoOpsRor *info_ops) {
    int32_t pt[2];
    if (!ai) return;
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    if (explore == 1 &&
        aoe_ai_tactical_explore_spot_ror(ai, unit, pt, ops, info, info_ops) == 1) {
        aoe_ai_tactical_order_explore_ror(ai, unit, (float)pt[0], (float)pt[1], 1, ops);
        return;
    }
    aoe_ai_tactical_order_explore_ror(ai, unit, -1.0f, -1.0f, explore == 1, ops);
}

void aoe_ai_tactical_send_scout_ror(AoeAiTacticalRor *ai, int32_t unit, float x,
                                    float y, const AoeAiTacticalOpsRor *ops,
                                    AoeAiInfoRor *info,
                                    const AoeAiInfoOpsRor *info_ops) {
    int32_t i, pt[2];
    if (!ai) return;
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    for (i = 0; i < ai->lists[0].count; ++i)
        if (ai->lists[0].data[i] == unit) {
            if (aoe_ai_tactical_explore_spot_ror(ai, unit, pt, ops, info, info_ops) == 1)
                aoe_ai_tactical_order_explore_ror(ai, unit, (float)pt[0], (float)pt[1], 1, ops);
            else
                aoe_ai_tactical_order_explore_ror(ai, unit, x, y, 1, ops);
            return;
        }
    aoe_ai_tactical_order_explore_ror(ai, unit, x, y, 0, ops);
}

void aoe_ai_tactical_plan_villagers_ror(AoeAiTacticalRor *ai,
                                        const AoeAiTacticalOpsRor *ops) {
    int32_t explored, seen, v, *first, *x = NULL;
    if (!ai || !ops) return;
    x = &ai->lists[1].field_8;
    explored = aoe_ftol32((ops->explored_ratio ? ops->explored_ratio(ops->context) : 0.0) *
                          (double)100.0f);
    first = aoe_ai_int_list_at_ror(&ai->list_d28, 0);
    seen = ops->info_count_owned ? ops->info_count_owned(ops->context, first ? *first : 0) : 0;
    v = ai->lists[0].count;
    if (v < 3) {
        if (v == 1) {
            ai->field_9d0 = 0;
            *x = 1;
        } else {
            ai->field_9d0 = 1;
            if (ai->sn[5] != -1 && ai->sn[5] < 1) ai->field_9d0 = ai->sn[5];
            *x = v - ai->field_9d0;
            if (ai->sn[3] != -1 && *x > ai->sn[3]) *x = ai->sn[3];
        }
    } else {
        first = aoe_ai_int_list_at_ror(&ai->lists[0], 0);
        if (ops->info_zone_open && ops->info_zone_open(ops->context, first ? *first : 0) == 1) {
            int32_t g;
            *x = aoe_ftol32(ceil((double)ai->sn[0] * (double)v * (double)0.01f));
            g = aoe_ai_tactical_grouped_units_ror(ai, 0x66);
            if (*x + g > ai->sn[18]) *x = ai->sn[18] - g;
            if (ai->sn[3] != -1 && *x > ai->sn[3]) *x = ai->sn[3];
        } else {
            *x = 0;
        }
        if (explored >= ai->sn[32] && seen > ai->sn[145]) {
            if (*x > ai->sn[35]) *x = ai->sn[35];
        } else if (explored >= ai->sn[179]) {
            *x = *x / 2 > 0 ? *x / 2 : 0;
        }
        if (ai->field_fa4 == 1) *x = 0;
        ai->field_9d0 = v - *x;
        if (ai->sn[5] != -1 && ai->field_9d0 > ai->sn[5]) ai->field_9d0 = ai->sn[5];
    }
    aoe_ai_tactical_plan_gatherers_ror(ai, ops);
}

int32_t aoe_ai_tactical_enemy_seen_ror(AoeAiTacticalRor *ai,
                                       const AoeAiTacticalOpsRor *ops) {
    int32_t *first;
    if (!ai) return 0;
    first = aoe_ai_int_list_at_ror(&ai->list_d28, 0);
    return ops && ops->info_count_owned
               ? ops->info_count_owned(ops->context, first ? *first : 0)
               : 0;
}

AoeAiUnitGroupRor *aoe_ai_tactical_ready_group_ror(AoeAiTacticalRor *ai, int32_t task,
                                                   int32_t max, const int32_t pt[2],
                                                   const AoeAiUnitGroupOpsRor *group_ops) {
    AoeAiUnitGroupRor *best = NULL;
    int32_t best_d = -1, i;
    if (!ai) return NULL;
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        int32_t dx, dy, d;
        if (task != -1 && g->task != task) continue;
        if (g->field_160 > aoe_ai_unit_group_count_ror(g)) continue;
        if (!aoe_ai_unit_group_in_range_ror(g, group_ops) ||
            !aoe_ai_unit_group_settled_ror(g, group_ops, 1))
            continue;
        dy = pt[1] - aoe_ftol32(g->loc_19c[1]);
        dx = pt[0] - aoe_ftol32(g->loc_19c[0]);
        d = aoe_ftol32(sqrt((double)(dy * dy + dx * dx)));
        if ((d < max && best_d == -1) || d < best_d) {
            best = g;
            best_d = d;
        }
    }
    return best;
}

int aoe_ai_tactical_rally_group_ror(AoeAiTacticalRor *ai, AoeAiUnitGroupRor *g,
                                    const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor cmd, c;
    int32_t centre, pt[2], out[2];
    if (!ai || !g || !ops || !ops->object || !ops->object(ops->context, g->commander, &cmd))
        return 0;
    centre = ops->first_center ? ops->first_center(ops->context) : -1;
    if (centre != -1 && ops->object(ops->context, centre, &c)) {
        pt[0] = aoe_ftol32(c.x);
        pt[1] = aoe_ftol32(c.y);
        if (ops->ring_site &&
            ops->ring_site(ops->context, pt, g->field_160, ai->sn[22], ai->sn[72], cmd.id,
                           out) == 1) {
            aoe_ai_unit_group_set_all_locations_ror(g, (float)out[0], (float)out[1], cmd.z);
            return 1;
        }
    }
    aoe_ai_unit_group_set_all_locations_ror(g, cmd.x, cmd.y, cmd.z);
    return 1;
}

void aoe_ai_tactical_path_waypoints_ror(AoeAiUnitGroupRor *g, AoeUnitPathRor *path) {
    float step, acc;
    int few, i;
    if (!g || !path) return;
    step = (float)(aoe_unit_path_length_ror(path) * (double)0.2f);
    if (step < 10.0f) step = 10.0f;
    aoe_unit_path_rewind_ror(path);
    acc = (float)aoe_unit_path_segment_ror(path);
    aoe_unit_path_advance_ror(path);
    few = aoe_unit_path_count_ror(path) <= 5;
    for (i = 1; i < aoe_unit_path_count_ror(path) - 1; ++i) {
        if (!(acc < (few ? 5.0f : step))) {
            const AoePathWaypointRor *w = aoe_unit_path_current_ror(path);
            if (w) aoe_ai_unit_group_add_waypoint_ror(g, w->x, w->y);
        }
        acc = (float)(aoe_unit_path_segment_ror(path) + (double)acc);
        aoe_unit_path_advance_ror(path);
    }
}

static void purge_list(AoeAiTacticalRor *ai, int k, const AoeAiTacticalOpsRor *ops,
                       const AoeAiUnitGroupOpsRor *group_ops) {
    int32_t i;
    for (i = 0; i < ai->lists[k].count; ++i) {
        int32_t *v = aoe_ai_int_list_at_ror(&ai->lists[k], i), id, task;
        AoeAiWorldObjectRor o;
        if (!v) continue;
        id = *v;
        if (ops->world_object(ops->context, id, &o) && !(o.hp < 1.0f) &&
            ops->unit_task && ops->unit_task(ops->context, id, &task))
            continue;
        aoe_ai_tactical_unit_removed_ror(ai, id, ops, group_ops);
    }
}

void aoe_ai_tactical_upkeep_ror(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops,
                                const AoeAiUnitGroupOpsRor *group_ops) {
    int32_t i;
    if (!ai || !ops || !ops->world_object) return;
    for (i = 0; i < ai->lists[0].count; ++i) {
        int32_t *v = aoe_ai_int_list_at_ror(&ai->lists[0], i), id, task;
        AoeAiWorldObjectRor u;
        if (!v) continue;
        id = *v;
        if (!ops->world_object(ops->context, id, &u) || !ops->unit_task ||
            !ops->unit_task(ops->context, id, &task))
            continue;
        if (aoe_ai_int_list_contains_ror(&ai->list_d58, id)) continue;
        if ((ops->unit_f4 ? ops->unit_f4(ops->context, id) : 0.0) == 0.0 &&
            u.field_44 > 0.0f && !(ops->unit_130 && ops->unit_130(ops->context, id)) &&
            !(ops->unit_134 && ops->unit_134(ops->context, id) > 2)) {
            int32_t drop = ops->info_dropsite ? ops->info_dropsite(ops->context, id) : -1;
            AoeAiWorldObjectRor d;
            if (drop == -1 || !ops->world_object(ops->context, drop, &d)) {
                aoe_ai_tactical_order_stop_ror(ops, id, 100);
            } else {
                if (ops->unload_at) ops->unload_at(ops->context, id, drop, d.x, d.y);
                aoe_ai_int_list_add_unique_ror(&ai->list_d58, id);
            }
        } else {
            int32_t action, target;
            AoeAiWorldObjectRor t;
            if (ops->unit_action && ops->unit_action(ops->context, id, &action) &&
                action == 700 && ops->unit_ai_target &&
                ops->unit_ai_target(ops->context, id, &target) &&
                (!ops->object || !ops->object(ops->context, target, &t) || t.state > 2))
                aoe_ai_tactical_order_stop_ror(ops, id, 100);
        }
    }
    purge_list(ai, 0, ops, group_ops);
    purge_list(ai, 2, ops, group_ops);
    purge_list(ai, 4, ops, group_ops);
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        if (g->task != 0x6a || g->field_180 != 0xc) continue;
        if (g->field_1c4 == -1 ||
            !aoe_ai_tactical_find_group_ror(ai, g->field_1c4, -1, -1, -1)) {
            g->field_180 = 1;
            g->field_184 = g->field_188 = -1;
            aoe_ai_unit_group_set_location_ror(g->loc_18c, -1.0f, -1.0f, -1.0f);
            aoe_ai_unit_group_set_location_ror(g->loc_19c, -1.0f, -1.0f, -1.0f);
            aoe_ai_unit_group_set_location_ror(g->loc_1ac, -1.0f, -1.0f, -1.0f);
            g->type = 0;
            g->field_1c0 = -1;
            g->field_1d0 = -1;
        }
    }
}

/* One list pass of 0x004de320: each ungrouped unit gets its own group. */
static void single_unit_groups(AoeAiTacticalRor *ai, int32_t k, int32_t task,
                               int32_t size, const AoeAiTacticalOpsRor *ops,
                               const AoeAiUnitGroupOpsRor *group_ops) {
    while (ai->list_d48.count > 0) {
        int32_t *v = aoe_ai_int_list_at_ror(&ai->list_d48, k), id, before;
        AoeAiWorldObjectRor u;
        if (!v) break;
        id = *v;
        before = ai->list_d48.count;
        if (ops->object && ops->object(ops->context, id, &u)) {
            AoeAiUnitGroupRor *g = aoe_ai_tactical_new_group_ror(ai, 1);
            if (!g) break;
            g->task = task;
            g->field_160 = size;
            aoe_ai_unit_group_add_ror(g, id, group_ops);
            aoe_ai_unit_group_set_commander_ror(g, id, group_ops);
            aoe_ai_unit_group_set_location_ror(g->location, u.x, u.y, u.z);
            aoe_ai_unit_group_set_location_ror(g->loc_19c, u.x, u.y, u.z);
            aoe_ai_unit_group_set_location_ror(g->loc_1ac, u.x, u.y, u.z);
        }
        aoe_ai_int_list_remove_ror(&ai->list_d48, id);
        if (ai->list_d48.count == before) break;
    }
}

void aoe_ai_tactical_setup_boat_groups_ror(AoeAiTacticalRor *ai,
                                           const AoeAiTacticalOpsRor *ops,
                                           const AoeAiUnitGroupOpsRor *group_ops) {
    AoeAiIntListRor doomed;
    int32_t i, k, n67, n68, n69, w67 = 0, w68 = 0, w69 = 0, m, rest, *first;
    if (!ai || !ops) return;
    memset(&doomed, 0, sizeof doomed);
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        if ((g->task == 0x67 || g->task == 0x68 || g->task == 0x69) &&
            aoe_ai_unit_group_count_ror(g) == 0)
            aoe_ai_int_list_add_unique_ror(&doomed, g->id);
    }
    for (k = 0; k < doomed.count; ++k) aoe_ai_tactical_delete_group_ror(ai, doomed.data[k]);
    n67 = aoe_ai_tactical_count_groups_ror(ai, 0x67, -1);
    n68 = aoe_ai_tactical_count_groups_ror(ai, 0x68, -1);
    n69 = aoe_ai_tactical_count_groups_ror(ai, 0x69, -1);
    aoe_ai_tactical_collect_ungrouped_ror(ai, 6);
    single_unit_groups(ai, k, 0x6b, ai->sn[65] + 1, ops, group_ops);
    aoe_ai_tactical_collect_ungrouped_ror(ai, 7);
    single_unit_groups(ai, k, 0x6c, ai->sn[64] + 1, ops, group_ops);
    aoe_ai_tactical_collect_ungrouped_ror(ai, 8);
    single_unit_groups(ai, k, 0x6a, ai->sn[66] + 1, ops, group_ops);
    first = aoe_ai_int_list_at_ror(&ai->lists[4], 0);
    if (ops->info_zone_open && !ops->info_zone_open(ops->context, first ? *first : 0)) {
        aoe_ai_tactical_delete_groups_ror(ai, 0x69);
        aoe_ai_tactical_set_sn_ror(ai, 0x3d, 0, NULL, NULL);
    }
    m = ai->lists[5].count;
    if (ai->sn[40] == 0) {
        w67 = ceil_div(m, ai->sn[59]);
        if (w67 > ai->sn[58]) w67 = ai->sn[58];
        rest = m - ai->sn[59] * w67;
        if (rest > 0) {
            w68 = ceil_div(rest, ai->sn[68]);
            if (w68 > ai->sn[67]) w68 = ai->sn[67];
            rest = m - ai->sn[68] * w68 - ai->sn[59] * w67;
            if (rest > 0) {
                w69 = ceil_div(rest, ai->sn[62]);
                if (w69 > ai->sn[61]) w69 = ai->sn[61];
            }
        }
    } else {
        w67 = m < ai->sn[58] ? m : ai->sn[58];
        if (m - w67 > 0) {
            w68 = m - w67 < ai->sn[67] ? m - w67 : ai->sn[67];
            if (m - w68 - w67 > 0)
                w69 = m - w68 - w67 < ai->sn[61] ? m - w68 - w67 : ai->sn[61];
        }
    }
    for (; n67 < w67; ++n67) {
        AoeAiUnitGroupRor *g = NULL;
        if (n68 > 0) {
            if ((g = aoe_ai_tactical_find_group_ror(ai, -1, 0x68, -1, -1))) --n68;
            else if ((g = aoe_ai_tactical_find_group_ror(ai, -1, 0x69, -1, -1))) --n69;
        }
        if (!g && !(g = aoe_ai_tactical_new_group_ror(ai, 1))) break;
        soldier_group(g, 0x67, ai->sn[59], 1);
    }
    while (n67 > w67) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_find_group_ror(ai, -1, 0x67, 0, -1);
        if (!g) g = aoe_ai_tactical_find_group_ror(ai, -1, 0x67, 1, -1);
        if (!g) break;
        aoe_ai_tactical_delete_group_ror(ai, g->id);
        --n67;
    }
    for (; n68 < w68; ++n68) {
        AoeAiUnitGroupRor *g = NULL;
        if (n69 > 0 && (g = aoe_ai_tactical_find_group_ror(ai, -1, 0x69, -1, -1))) --n69;
        if (!g && !(g = aoe_ai_tactical_new_group_ror(ai, 1))) break;
        soldier_group(g, 0x68, ai->sn[68], 1);
    }
    while (n68 > w68) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_find_group_ror(ai, -1, 0x68, -1, -1);
        if (!g) break;
        aoe_ai_tactical_delete_group_ror(ai, g->id);
        --n68;
    }
    for (; n69 < w69; ++n69) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_new_group_ror(ai, 1);
        if (!g) break;
        soldier_group(g, 0x69, ai->sn[62], 0);
    }
    while (n69 > w69) {
        AoeAiUnitGroupRor *g = aoe_ai_tactical_find_group_ror(ai, -1, 0x69, -1, -1);
        if (!g) break;
        aoe_ai_tactical_delete_group_ror(ai, g->id);
        --n69;
    }
    free(doomed.data);
}

/* 0x004e13c0: see the header. */
int aoe_ai_tactical_defend_object_ror(AoeAiTacticalRor *ai, int32_t enemy, int32_t ours,
                                      const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor e, o, u;
    int32_t tile[2], unit, sent = 0, i;
    if (!ai || !ops || !ops->world_object || !ops->world_object(ops->context, enemy, &e) ||
        !ops->world_object(ops->context, ours, &o))
        return 0;
    for (i = 0; i < ai->lists[1].count && i < ai->lists[1].capacity; ++i)
        if (ai->lists[1].data[i] == o.id) return 0;
    tile[0] = aoe_ftol32(o.x);
    tile[1] = aoe_ftol32(o.y);
    unit = aoe_ai_tactical_pick_unit_ror(ai, tile, 1, 0x2bc, 0x2c9, 0x2c9, 1, ops);
    if (unit == -1) return 0;
    do {
        float r;
        if (sent >= ai->sn[100] || !ops->object || !ops->object(ops->context, unit, &u)) break;
        r = ops->range ? (float)ops->range(ops->context, unit) : 0.0f;
        if (!ops->can_reach ||
            !ops->can_reach(ops->context, unit, aoe_ftol32(e.x), aoe_ftol32(e.y),
                            aoe_ftol32(e.z), r, e.id))
            break;
        aoe_ai_tactical_order_attack_ror(ai, unit, e.x, e.y, e.id, e.owner, NULL, 0, 0, ops);
        ++sent;
        unit = aoe_ai_tactical_pick_unit_ror(ai, tile, 1, 0x2bc, 0x2c9, 0x2c9, 1, ops);
    } while (unit != -1);
    return sent > 0;
}

/* 0x004e2b70: see the header. */
static int research_in_age(AoeAiTacticalRor *ai, int32_t age, int32_t next, int32_t type,
                           int32_t food, int32_t gold, const AoeAiTacticalOpsRor *ops,
                           AoeAiBuildItemRor **item) {
    *item = NULL;
    if (!aoe_ai_tactical_age_is_ror(ops, age) || ai->sn[180 + age] != 1 || !ops->age_open ||
        !ops->age_open(ops->context, next) || !ops->resource_amount ||
        ops->resource_amount(ops->context, 0) < food ||
        (gold && ops->resource_amount(ops->context, 3) < gold))
        return 0;
    *item = ops->build_pending ? ops->build_pending(ops->context, type, 1, 4) : NULL;
    return 1;
}

void aoe_ai_tactical_research_ages_ror(AoeAiTacticalRor *ai,
                                       const AoeAiTacticalOpsRor *ops) {
    AoeAiBuildItemRor *item;
    if (!ai || !ops) return;
    /* Original quirk: when the age-1 test passes but no 0x65 item is
       pending, the age-2 test is skipped (it is only the else branch). */
    if (research_in_age(ai, 1, 2, 0x65, 500, 0, ops, &item) ||
        research_in_age(ai, 2, 3, 0x66, 800, 0, ops, &item)) {
        if (item) aoe_ai_tactical_research_ror(ai, item, ops);
    }
    if (research_in_age(ai, 3, 4, 0x67, 900, 700, ops, &item) && item)
        aoe_ai_tactical_research_ror(ai, item, ops);
}

/* 0x004e1d00: see the header. */
static void plan_costed(int32_t type, int32_t cap, int32_t position,
                        int32_t limit, const AoeAiTacticalOpsRor *ops) {
    AoeAiCountersRor costs;
    int32_t i;
    aoe_ai_counters_init_ror(&costs, 4);
    /* Original quirk: the (still zero) costs are scaled by `cap` for the
       first `cap` slots before they are filled, so nothing is scaled. */
    for (i = 0; i < cap; ++i)
        aoe_ai_counters_set_ror(&costs, i, aoe_ai_counters_get_ror(&costs, i) * cap);
    if (ops->type_costs) ops->type_costs(ops->context, type, &costs);
    if (ops->can_afford && ops->can_afford(ops->context, &costs) && ops->template_usable &&
        ops->template_usable(ops->context, type) && ops->build_all_started &&
        ops->build_all_started(ops->context, 0, 0) && ops->build_started &&
        ops->build_started(ops->context, type, 0) < limit && ops->build_add)
        ops->build_add(ops->context, type, cap, position);
    aoe_ai_counters_release_ror(&costs);
}

void aoe_ai_tactical_plan_building_ror(AoeAiTacticalRor *ai, int32_t type, int32_t cap,
                                       int32_t position, const AoeAiTacticalOpsRor *ops) {
    if (!ai || !ops) return;
    if (type == 0x32) {
        if (ai->sn[206] == 1 && aoe_ai_tactical_age_is_ror(ops, 4))
            plan_costed(0x32, cap, position, ai->sn[220], ops);
    } else if (type == 0x44) {
        int32_t total = 0, unbuilt = 0, need, dist;
        if (ai->sn[205] != 1 || !ops->info_best_distance) return;
        dist = ops->info_best_distance(ops->context, 0);
        if (dist == -1) return;
        need = aoe_ai_tactical_need_pit_ror(ai, &total, &unbuilt, ops);
        if (unbuilt > 0 || need < 1 || ai->sn[163] >= dist) return;
        if (ops->build_add) ops->build_add(ops->context, 0x44, cap, position);
    } else if (type == 0x4f) {
        if (ai->sn[207] == 1) plan_costed(0x4f, cap, position, ai->sn[222], ops);
    }
}

int aoe_ai_tactical_try_explore_ror(AoeAiTacticalRor *ai, int32_t unit, int32_t explore,
                                    const AoeAiTacticalOpsRor *ops, AoeAiInfoRor *info,
                                    const AoeAiInfoOpsRor *info_ops) {
    AoeAiWorldObjectRor u;
    int32_t task, action, v;
    if (!ai || !ops || !ops->world_object || !ops->world_object(ops->context, unit, &u) ||
        !ops->unit_task || !ops->unit_task(ops->context, unit, &task))
        return 0;
    if (task == 600 || task == 0x25a || task == 0x26a || task == 0x25d) return 1;
    if (u.field_44 > 0.0f) return 0;
    action = -1;
    if (ops->unit_action) ops->unit_action(ops->context, unit, &action);
    if (action == 0x2c6 && ops->unit_2c && ops->unit_2c(ops->context, unit, &v) && v == 100)
        return 0;
    if (action == 0x2be) return 0;
    if (ops->unit_ai_table_has && ops->unit_ai_table_has(ops->context, unit, 0x2c1) == 1)
        return 0;
    aoe_ai_tactical_send_explorer_ror(ai, unit, explore, ops, info, info_ops);
    return 1;
}

void aoe_ai_tactical_insert_builds_ror(AoeAiTacticalRor *ai,
                                       const AoeAiTacticalOpsRor *ops) {
    int skip_pit_plan = 0;
    uint32_t minutes;
    if (!ai || !ops) return;
    if (ai->sn[205] == 1) {
        if (ops->build_file_set && ops->build_file_set(ops->context) && ai->field_f88 == 0 &&
            ai->sn[169] <= (ops->info_field_560 ? ops->info_field_560(ops->context) : 0)) {
            if (ops->build_add) ops->build_add(ops->context, 0x67, 1, 0);
            ai->field_f88 = 1;
        }
        if (ai->sn[205] == 1 && ops->build_file_set && ops->build_file_set(ops->context) &&
            ai->field_f8c == 0 &&
            (ai->field_f88 == 1 ||
             (ops->resource_amount ? ops->resource_amount(ops->context, 1) : 0) >= 0xfa) &&
            (ops->info_best_distance ? ops->info_best_distance(ops->context, 0) : -1) != -1) {
            if (ops->build_add) ops->build_add(ops->context, 0x44, 1, 1);
            ai->field_f8c = 1;
            skip_pit_plan = 1;
        }
    }
    if (!skip_pit_plan && ai->field_f8c == 1) aoe_ai_tactical_plan_building_ror(ai, 0x44, 1, 0, ops);
    minutes = (ops->game_time ? ops->game_time(ops->context) : 0) / 60000;
    if (ai->sn[175] != -1 && (uint32_t)ai->sn[176] <= minutes) {
        if (ops->build_add) ops->build_add(ops->context, ai->sn[175], 1, 0);
        ai->sn[175] = -1;
    }
    aoe_ai_tactical_plan_building_ror(ai, 0x32, 1, 0, ops);
    aoe_ai_tactical_plan_building_ror(ai, 0x4f, 1, 0, ops);
}

void aoe_ai_tactical_retarget_trade_ror(AoeAiTacticalRor *ai,
                                        const AoeAiTacticalOpsRor *ops) {
    int32_t i, k;
    if (!ai || !ops || !ops->object) return;
    for (i = 0; i < ai->lists[7].count; ++i) {
        int32_t *v = aoe_ai_int_list_at_ror(&ai->lists[7], i);
        AoeAiWorldObjectRor o;
        int16_t kind;
        if (!v || !ops->object(ops->context, *v, &o)) continue;
        kind = o.field_4c;
        if (!((ops->resource_amount ? ops->resource_amount(ops->context, kind) : 0) <
              ai->sn[216]))
            continue;
        for (k = 2; k >= 0; --k)
            if (k != kind &&
                ai->sn[216] <= (ops->resource_amount ? ops->resource_amount(ops->context, k) : 0)) {
                if (ops->set_trade) ops->set_trade(ops->context, *v, k);
                break;
            }
    }
}

void aoe_ai_tactical_send_gatherers_ror(AoeAiTacticalRor *ai, int32_t first, int32_t count,
                                        int32_t target, const AoeAiTacticalOpsRor *ops) {
    AoeAiIntListRor sent;
    AoeAiWorldObjectRor t;
    int32_t n, i;
    if (!ai || !ops || !ops->object || !ops->object(ops->context, target, &t)) return;
    memset(&sent, 0, sizeof sent);
    aoe_ai_int_list_add_unique_ror(&sent, first);
    for (n = 0; n < count; ++n) {
        int32_t best = -1;
        float best_d = -1.0f;
        for (i = 0; i < ai->field_9cc && i < AOE_AI_TAC_GATHERERS; ++i) {
            int32_t *e = ai->gatherers[i], task, j;
            AoeAiWorldObjectRor u;
            double d;
            for (j = 0; j < sent.count; ++j)
                if (sent.data[j] == e[0]) break;
            if (j < sent.count) continue;
            if (!ops->object(ops->context, e[0], &u) || !ops->unit_task ||
                !ops->unit_task(ops->context, e[0], &task) || e[1] != 0)
                continue;
            d = ops->object_distance ? ops->object_distance(ops->context, e[0], target) : 0.0;
            if (best_d == -1.0f || d < (double)best_d) {
                best = e[0];
                best_d = (float)d;
            }
        }
        if (ops->player_124) ops->player_124(ops->context, best, target, 0, t.x, t.y);
        aoe_ai_int_list_add_unique_ror(&sent, best);
    }
    free(sent.data);
}

void aoe_ai_tactical_tribute_surplus_ror(AoeAiTacticalRor *ai,
                                         const AoeAiTacticalOpsRor *ops) {
    int32_t now, k, p;
    if (!ai || !ops || ai->sn[186] == -1) return;
    if (!ops->has_market || !ops->has_market(ops->context)) return;
    now = ops->game_time ? (int32_t)ops->game_time(ops->context) : 0;
    if ((now - ai->field_f9c) / 1000 < ai->sn[186]) return;
    ai->field_f9c = now;
    if (ops->stats_player && ops->stats_player(ops->context) == 0xff && ops->set_stats_player) {
        int32_t computers = 0, r;
        for (p = 1; p < ops->player_count; ++p)
            if (ops->player_field_4 && ops->player_field_4(ops->context, p) == 1) ++computers;
        if (ops->player_count > 1 && computers == 1) {
            ops->set_stats_player(ops->context, (uint8_t)ops->self);
        } else if (computers > 0) {
            r = (ops->rand ? ops->rand(ops->context) : 0) % computers + 1;
            for (p = 1; p < ops->player_count; ++p)
                if (ops->player_field_4 && ops->player_field_4(ops->context, p) == 1 && --r == 0)
                    ops->set_stats_player(ops->context, (uint8_t)p);
        }
    }
    for (k = 0; k < 4; ++k) {
        int32_t have = ops->resource_amount ? ops->resource_amount(ops->context, k) : 0, best = -1;
        float best_v = 0.0f;
        if (!(ai->sn[190 + k] < have)) continue;
        for (p = 1; p < ops->player_count; ++p) {
            float v;
            if (p == ops->self || !ops->player_field_4 || !ops->player_field_4(ops->context, p) ||
                !ops->is_ally || !ops->is_ally(ops->context, p) || !ops->names_us_ally ||
                !ops->names_us_ally(ops->context, p))
                continue;
            v = ops->player_resource ? ops->player_resource(ops->context, p, k) : 0.0f;
            if (v < (float)(ops->player_3c ? ops->player_3c(ops->context, p, 0xbe + k) : 0) &&
                (best == -1 || v < best_v)) {
                best = p;
                best_v = v;
            }
        }
        if (best != -1 && ops->tribute)
            ops->tribute(ops->context, best, k, (float)(have - ai->sn[190 + k]));
    }
}

/* The statistics player chosen when unset (as 0x004c71a0 does). */
static void pick_stats_player(const AoeAiTacticalOpsRor *ops) {
    int32_t p, computers = 0, r;
    if (!ops->stats_player || ops->stats_player(ops->context) != 0xff || !ops->set_stats_player)
        return;
    for (p = 1; p < ops->player_count; ++p)
        if (ops->player_field_4 && ops->player_field_4(ops->context, p) == 1) ++computers;
    if (ops->player_count > 1 && computers == 1) {
        ops->set_stats_player(ops->context, (uint8_t)ops->self);
        return;
    }
    if (computers <= 0) return;
    r = (ops->rand ? ops->rand(ops->context) : 0) % computers + 1;
    for (p = 1; p < ops->player_count; ++p)
        if (ops->player_field_4 && ops->player_field_4(ops->context, p) == 1 && --r == 0)
            ops->set_stats_player(ops->context, (uint8_t)p);
}

void aoe_ai_tactical_ask_allies_ror(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops) {
    int32_t now, k, p, budget;
    uint8_t stats;
    if (!ai || !ops || ai->sn[186] == -1) return;
    now = ops->game_time ? (int32_t)ops->game_time(ops->context) : 0;
    if ((now - ai->field_f98) / 1000 < ai->sn[186]) return;
    ai->field_f98 = now;
    pick_stats_player(ops);
    stats = ops->stats_player ? ops->stats_player(ops->context) : 0xff;
    if (stats != (uint8_t)ops->self) return;
    budget = ai->sn[187];
    for (k = 0; k < 4; ++k) {
        int32_t r = ai->field_d88[k];
        if (budget < 1) return;
        if (r < 0 || r > 3) continue;
        if (!((ops->resource_amount ? ops->resource_amount(ops->context, r) : 0) <
              ai->sn[190 + r]))
            continue;
        if (!(ai->field_d98[k] < 0)) continue;
        for (p = 1; p < ops->player_count; ++p) {
            int32_t amount;
            if (p == ops->self || !ops->player_field_4 || !ops->player_field_4(ops->context, p) ||
                !ops->is_ally || !ops->is_ally(ops->context, p) || !ops->names_us_ally ||
                !ops->names_us_ally(ops->context, p))
                continue;
            if (!((ops->self < p && p != stats) || ops->self == stats)) continue;
            amount = aoe_ftol32(ops->player_resource ? ops->player_resource(ops->context, p, r)
                                                     : 0.0f);
            if (amount <= 0x32) continue;
            if (amount > budget) amount = budget;
            budget -= amount;
            if (ops->player_54) ops->player_54(ops->context, p, ops->self, 1, r, amount, 0);
        }
    }
}

void aoe_ai_tactical_start_play_ror(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops,
                                    const AoeAiUnitGroupOpsRor *group_ops) {
    int32_t ids[50], cast[50], n = 0, i, play = -1, cast_n = 0, leader = -1, target, *first;
    AoeAiWorldObjectRor l;
    AoeAiUnitGroupRor *g;
    if (!ai || !ops || ai->sn[162] == 0) return;
    for (i = 0; i < ai->lists[2].count; ++i) {
        int32_t *v = aoe_ai_int_list_at_ror(&ai->lists[2], i);
        AoeAiUnitGroupRor *in;
        if (!v) continue;
        in = aoe_ai_tactical_group_of_ror(ai, *v);
        if (in && in->task == 100) continue;
        if (n < 50) ids[n++] = *v;
    }
    if (n == 0 || !ops->choose_play) return;
    first = aoe_ai_int_list_at_ror(&ai->list_d28, 0);
    target = ops->choose_play(ops->context, first ? *first : 0, ids, n, &play, cast, &cast_n,
                              &leader);
    if (target == -1 || !ops->object || !ops->object(ops->context, leader, &l)) return;
    if (!(g = aoe_ai_tactical_new_group_ror(ai, 1))) return;
    g->task = 100;
    g->field_160 = cast_n;
    for (i = 0; i < cast_n && i < 50; ++i) aoe_ai_unit_group_add_ror(g, cast[i], group_ops);
    aoe_ai_unit_group_set_commander_ror(g, leader, group_ops);
    aoe_ai_unit_group_set_location_ror(g->location, l.x, l.y, l.z);
    aoe_ai_unit_group_set_location_ror(g->loc_19c, l.x, l.y, l.z);
    aoe_ai_unit_group_set_location_ror(g->loc_1ac, l.x, l.y, l.z);
    g->field_184 = target;
    g->field_1d0 = play;
    g->field_180 = 2;
    g->type = 1;
    aoe_ai_unit_group_update_ror(g, group_ops, 2, 1, 1);
}

static int reaches(const AoeAiTacticalOpsRor *ops, int32_t unit, const AoeAiWorldObjectRor *e) {
    AoeAiWorldObjectRor u;
    float r;
    if (!ops->world_object || !ops->world_object(ops->context, unit, &u)) return 0;
    r = ops->range ? (float)ops->range(ops->context, unit) : 0.0f;
    return ops->can_reach && ops->can_reach(ops->context, unit, aoe_ftol32(e->x),
                                            aoe_ftol32(e->y), aoe_ftol32(e->z), r, e->id) == 1;
}

void aoe_ai_tactical_defend_ror(AoeAiTacticalRor *ai, int32_t enemy,
                                const AoeAiTacticalOpsRor *ops,
                                const AoeAiUnitGroupOpsRor *group_ops) {
    AoeAiWorldObjectRor e;
    AoeAiUnitGroupRor *g;
    uint32_t start;
    int32_t pt[2], sent = 0, i, v;
    if (!ai || !ops || !ops->object) return;
    start = ops->real_ms ? ops->real_ms(ops->context) : 0;
    if (!ops->object(ops->context, enemy, &e)) return;
    pt[0] = aoe_ftol32(e.x);
    pt[1] = aoe_ftol32(e.y);
    g = aoe_ai_tactical_ready_group_ror(ai, 100, ai->sn[20], pt, group_ops);
    if (!g) g = aoe_ai_tactical_ready_group_ror(ai, 0x67, ai->sn[20], pt, group_ops);
    if (g && reaches(ops, g->commander, &e)) {
        g->field_184 = enemy;
        g->field_188 = e.type_id;
        aoe_ai_unit_group_set_location_ror(g->loc_18c, e.x, e.y, e.z);
        g->field_180 = 2;
        aoe_ai_unit_group_update_ror(g, group_ops, 2, 1, 0);
        return;
    }
    for (i = 0; i < ai->lists[2].count; ++i) {
        int32_t *p = aoe_ai_int_list_at_ror(&ai->lists[2], i), id, task, target;
        AoeAiWorldObjectRor u;
        if (i > 1 && ops->real_ms && ops->real_ms(ops->context) - start > 10) return;
        if (!p) continue;
        id = *p;
        if (!ops->world_object || !ops->world_object(ops->context, id, &u) || !ops->unit_task ||
            !ops->unit_task(ops->context, id, &task))
            continue;
        if (task == 600 && ops->unit_ai_target && ops->unit_ai_target(ops->context, id, &target) &&
            target != -1) {
            AoeAiWorldObjectRor t;
            if (target == enemy) {
                ++sent;
                continue;
            }
            if (ops->world_object(ops->context, target, &t)) {
                int32_t *first = aoe_ai_int_list_at_ror(&ai->list_d28, 0);
                if (t.owner == (first ? *first : 0)) continue;
            }
        }
        if (!((ops->object_distance ? ops->object_distance(ops->context, id, enemy) : 0.0) <
              (double)25.0f))
            continue;
        if (!reaches(ops, id, &e)) continue;
        if (aoe_ai_tactical_order_attack_ror(ai, id, e.x, e.y, enemy, e.owner, NULL, 0, 0, ops) == 1)
            ++sent;
    }
    v = aoe_ai_tactical_pick_unit_ror(ai, pt, 1, 700, -1000, -1000, 1, ops);
    while (v != -1 && sent < 6) {
        AoeAiWorldObjectRor u;
        if (!ops->object(ops->context, v, &u)) return;
        if (!reaches(ops, v, &e)) return;
        aoe_ai_tactical_order_attack_ror(ai, v, e.x, e.y, enemy, e.owner, NULL, 0, 0, ops);
        ++sent;
        v = aoe_ai_tactical_pick_unit_ror(ai, pt, 1, 700, 0x2c9, 0x2c9, 1, ops);
    }
}

/* 0x004e22b0: see the header. */
void aoe_ai_tactical_upgrades_ror(AoeAiTacticalRor *ai, int32_t owner,
                                  const AoeAiTacticalOpsRor *ops, const AoeAiInfoRor *info,
                                  const AoeAiInfoOpsRor *info_ops) {
    static const int32_t items[11][2] = {
        {0x25, 0x65}, {0x4b, 0xc}, {0x5d, 0}, {0x27, 0x57}, {0x49, 0xc}, {5, 0x57},
        {0x12b, 0x65}, {4, 0x57}, {0x7d, 0x68}, {0x29, 0x57}, {0x28, 0x65}};
    AoeAiWorldObjectRor c;
    int32_t now, centre, budget, step = 0, added = 0;
    if (!ai || !ops || !ops->game_time) return;
    now = (int32_t)ops->game_time(ops->context);
    if ((now - ai->field_f94) / 1000 < 180) return;
    ai->field_f94 = now;
    centre = ops->first_of_type ? ops->first_of_type(ops->context, 0x6d) : -1;
    if (centre == -1 || !ops->object || !ops->object(ops->context, centre, &c)) return;
    budget = aoe_ai_info_count_near_ror(info, owner, aoe_ftol32(c.x), aoe_ftol32(c.y), 20,
                                        info_ops) -
             ai->lists[2].count;
    if (budget > 5) budget = 5;
    while (budget > 0) {
        if (step == 0) {
            if (ops->resource_amount && ops->resource_amount(ops->context, 2) >= 150 &&
                ai->lists[0].count > 2) {
                if (ops->build_add) ops->build_add(ops->context, 0x4f, 1, 0);
                budget -= 2;
            }
            step = 1;
        } else if (step <= 11) {
            const int32_t *it = items[step - 1];
            ++step;
            if (ops->template_usable && ops->template_usable(ops->context, it[0]) &&
                ops->first_of_type && ops->first_of_type(ops->context, it[1]) != -1) {
                AoeAiCountersRor costs;
                aoe_ai_counters_init_ror(&costs, 4);
                if (ops->type_costs) ops->type_costs(ops->context, it[0], &costs);
                if (ops->can_afford && ops->can_afford(ops->context, &costs) == 1) {
                    if (ops->build_add) ops->build_add(ops->context, it[0], 1, 0);
                    --budget;
                    added = 1;
                }
                aoe_ai_counters_release_ror(&costs);
            }
        } else {
            if (!added) return;
            step = 1;
            added = 0;
        }
    }
}

/* 0x004d3700: the dispatch at its 0x4d3883 label. */
static void run_group_dispatch(AoeAiTacticalRor *ai, AoeAiUnitGroupRor *g,
                               const AoeAiTacticalOpsRor *ops) {
    if (g->task == 0x66 || g->task == 0x69) {
        g->field_180 = 8;
    } else if (g->task == 100 || g->task == 0x67) {
        if (aoe_ai_tactical_enemy_seen_ror(ai, ops) == 0 && g->task == 0x67) {
            g->task = 0x69;
            g->field_180 = 8;
        } else {
            g->field_180 = 2;
        }
    } else if (g->task == 0x65 || g->task == 0x68) {
        g->field_180 = 4;
    } else if (g->task == 0x6d) {
        g->field_180 = 0x13;
    }
}

/* 0x004d3700: see the header. */
void aoe_ai_tactical_run_groups_ror(AoeAiTacticalRor *ai, uint32_t start, uint32_t budget,
                                    const AoeAiTacticalOpsRor *ops,
                                    const AoeAiUnitGroupOpsRor *group_ops,
                                    const AoeAiStrategyRor *strategy,
                                    const AoeAiInfoRor *info, const AoeAiInfoOpsRor *info_ops) {
    int32_t i = 0;
    if (!ai || !ops) return;
    if (ai->field_f80 != -1) {
        while (i < ai->groups_len && ai->groups[i]->id != ai->field_f80) ++i;
        if (i == ai->groups_len) i = 0;
    }
    for (; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        int ready, dispatch = 0;
        if (g->task == 0x6b || g->task == 0x6c || g->task == 0x6a ||
            (g->field_180 != 0 && g->field_180 != 1))
            continue;
        ready = g->flag_2c8 > 10 ? 1 : aoe_ai_unit_group_in_range_ror(g, group_ops);
        if (g->field_160 > aoe_ai_unit_group_count_ror(g)) {
            AoeAiWorldObjectRor commander, centre;
            int32_t centre_id = ops->first_center ? ops->first_center(ops->context) : -1;
            if (ops->object && ops->object(ops->context, g->commander, &commander) &&
                centre_id != -1 && ops->object(ops->context, centre_id, &centre) &&
                info_ops && info_ops->zone && info_ops->tile_zone &&
                info_ops->zone(info_ops->context, g->commander) !=
                    info_ops->tile_zone(info_ops->context, g->commander,
                                        aoe_ftol32(centre.x), aoe_ftol32(centre.y))) {
                g->field_160 = aoe_ai_unit_group_count_ror(g);
                dispatch = 1;
            }
        } else if (ready && aoe_ai_unit_group_settled_ror(g, group_ops, 1)) {
            dispatch = 1;
            if ((g->task == 0x66 || g->task == 0x69) &&
                !(ops->info_zone_open && ops->info_zone_open(ops->context, g->commander)))
                dispatch = 0;
            else if (g->task == 100 || g->task == 0x67) {
                int32_t kind = strategy ? aoe_ai_strategy_ai_victory_kind_ror(strategy) : -1;
                if (kind == 0) {
                    if (aoe_ai_tactical_free_target_ror(ai, strategy) == -1 && g->task == 100)
                        dispatch = 0;
                    else
                        g->field_18 = 0;
                }
                if (kind == 4) {
                    if (aoe_ai_tactical_free_second_ror(ai, strategy) == -1 && g->task == 100)
                        dispatch = 0;
                    else
                        g->field_18 = 4;
                }
            } else if (g->task == 0x65 || g->task == 0x68) {
                dispatch = info && aoe_ai_info_list_148_count_ror(info) != 0;
            }
        }
        if (dispatch)
            run_group_dispatch(ai, g, ops);
        else if (!ready)
            aoe_ai_unit_group_update_ror(g, group_ops, 9, 1, 0);
        ai->field_f80 = i + 1 < ai->groups_len ? ai->groups[i + 1]->id : -1;
        if (ops->real_ms && ops->real_ms(ops->context) - start >= budget) return;
    }
    ai->field_f80 = -1;
}

/* 0x004daec0: the building planning after a target is found. */
static void gather_plan(AoeAiTacticalRor *ai, int32_t unit, int32_t kind,
                        const AoeAiWorldObjectRor *t, const AoeAiTacticalOpsRor *ops,
                        const AoeAiInfoRor *info, const AoeAiInfoOpsRor *info_ops) {
    int32_t total = 0, unbuilt = 0, dist, limit, type;
    if (ai->sn[205] != 1) return;
    dist = aoe_ftol32(ops->object_distance ? ops->object_distance(ops->context, unit, t->id)
                                           : 0.0);
    if (kind == 0 && ai->field_f8c == 1) {
        if (ai->field_f88 != 1) return;
        if (t->type_id == 0x3b) {
            if (aoe_ai_tactical_need_pit_ror(ai, &total, &unbuilt, ops) < 1 || unbuilt != 0)
                return;
            if (dist <= ai->sn[163] || !ops->info_best_distance ||
                ops->info_best_distance(ops->context, 0) == -1 ||
                aoe_ai_info_clear_of_enemy_ror(info, aoe_ftol32(t->x), aoe_ftol32(t->y),
                                               info_ops) != 1)
                return;
            type = 0x44;
        } else {
            if (t->type_class == 0x1f || t->type_class == 5 || t->type_class == 0x21) return;
            if (aoe_ai_tactical_need_granary_ror(ai, &total, &unbuilt, 0, ops) < 1 ||
                unbuilt != 0)
                return;
            if (dist <= 15 && total != 0) return;
            type = 0x67;
        }
    } else {
        if (ai->field_f88 != 1 || ai->field_f8c != 1) return;
        if (aoe_ai_tactical_need_granary_ror(ai, &total, &unbuilt, kind, ops) < 1 ||
            unbuilt != 0)
            return;
        limit = 163 + t->field_4c;
        limit = limit >= 0 && limit < AOE_AI_SN_COUNT ? ai->sn[limit] : 0;
        if (dist <= limit && total != 0) return;
        type = 0x67;
    }
    if (ops->build_add) ops->build_add(ops->context, type, 1, -1);
}

/* 0x004daec0: see the header. */
int aoe_ai_tactical_gather_ror(AoeAiTacticalRor *ai, int32_t unit, int32_t kind,
                               int32_t fixed, const int32_t exclude[4],
                               const AoeAiTacticalOpsRor *ops, AoeAiInfoRor *info,
                               const AoeAiInfoOpsRor *info_ops) {
    AoeAiWorldObjectRor u, t;
    int32_t *entry, target = -1, drop, amount;
    float pos[2] = {0.0f, 0.0f};
    if (!ai || !ops || !ops->object || !ops->object(ops->context, unit, &u)) return 0;
    entry = aoe_ai_tactical_gatherer_ror(ai, unit);
    if (entry && entry[2] > entry[3]) kind = entry[1];
    if (kind == -1) {
        int32_t k;
        for (k = 0; k < 4; ++k) {
            int32_t want = ai->field_d88[k];
            if (exclude && want >= 0 && want < 4 && exclude[want] == 1) continue;
            drop = -1;
            target = info ? aoe_ai_info_pick_target_ror(info, want, unit, pos, &drop, NULL, 0,
                                                        info_ops)
                          : -1;
            if (target == -1) continue;
            kind = want;
            if (drop == -1) {
                if (ops->can_engage) ops->can_engage(ops->context, unit, target, 1.0f);
            } else if (ops->can_gather) {
                ops->can_gather(ops->context, unit, target, drop);
            }
            break;
        }
        if (target == -1) return 0;
    } else {
        int32_t tried[3], n = 0;
        for (;;) {
            int ok;
            drop = -1;
            target = info ? aoe_ai_info_pick_target_ror(info, kind, unit, pos, &drop, tried, n,
                                                        info_ops)
                          : -1;
            if (target == -1) return 0;
            if (drop == -1)
                ok = ops->can_engage && ops->can_engage(ops->context, unit, target, 1.0f);
            else
                ok = ops->can_gather && ops->can_gather(ops->context, unit, target, drop);
            if (ok) break;
            aoe_ai_info_set_target_wait_ror(info, kind, target, kind == 0 ? 2 : 0xf);
            tried[n++] = target;
            if (n > 2) return 0;
        }
        if (target < 0) return 0;
    }
    if (!ops->object(ops->context, target, &t)) return 0;
    gather_plan(ai, unit, kind, &t, ops, info, info_ops);
    aoe_ai_int_list_remove_ror(&ai->lists[1], unit);
    entry = aoe_ai_tactical_add_gatherer_ror(ai, unit);
    if (!entry) return 0;
    if (kind >= 0 && kind < 4) ++ai->field_d68[kind];
    aoe_ai_int_list_add_unique_ror(&ai->list_d58, unit);
    entry[1] = kind;
    amount = kind >= 0 && kind < 4 ? aoe_ai_tactical_counter_share_ror(ai, kind) : ai->sn[148];
    if (fixed == 1) {
        amount = 10;
    } else {
        if (ai->sn[203] == 1) {
            if (t.type_id == 0x3b)
                amount = aoe_ftol32(t.field_44);
            else if (kind == 1)
                amount = 0x4b;
            else if (kind == 3 || kind == 2)
                amount = 0x32;
        } else if (amount < ai->sn[148]) {
            amount = ai->sn[148];
        } else if (amount > ai->sn[149]) {
            amount = ai->sn[149];
        }
        if (ai->field_fa4 == 1) amount = 0x14;
    }
    entry[2] = amount;
    entry[3] = 0;
    if (ops->player_124) ops->player_124(ops->context, unit, target, t.owner, pos[0], pos[1]);
    if (t.type_id == 0x30) aoe_ai_tactical_send_gatherers_ror(ai, unit, ai->sn[204] - 1, target, ops);
    if (t.type_id == 0x32 && ops->player_ddc_add) ops->player_ddc_add(ops->context, t.id);
    return 1;
}

/* 0x004da330: see the header. */
int aoe_ai_tactical_regather_ror(AoeAiTacticalRor *ai, int32_t unit,
                                 const AoeAiTacticalOpsRor *ops, AoeAiInfoRor *info,
                                 const AoeAiInfoOpsRor *info_ops) {
    AoeAiWorldObjectRor o;
    int32_t tried[4] = {0, 0, 0, 0}, *e, i, k, task, v;
    uint32_t now;
    int r = 0;
    if (!ai || !ops) return 0;
    for (i = 0; i < ai->field_9cc && i < AOE_AI_TAC_GATHERERS; ++i)
        if (ai->gatherers[i][0] == unit) break;
    if (i >= ai->field_9cc || i >= AOE_AI_TAC_GATHERERS) return 0;
    e = ai->gatherers[i];
    now = ops->real_ms ? ops->real_ms(ops->context) : 0;
    if (now - (uint32_t)e[5] < 5000) return 1;
    e[5] = (int32_t)now;
    if (!ops->world_object || !ops->world_object(ops->context, unit, &o) || !ops->unit_task ||
        !ops->unit_task(ops->context, unit, &task))
        return 0;
    if (task == 600 || task == 0x25a || task == 0x26a) return 1;
    if (o.field_44 > 0.0f) return 1;
    if (aoe_ai_tactical_busy_103_ror(ops, unit, 0) == 1 &&
        (ops->resource_amount ? ops->resource_amount(ops->context, 0) : 0) < 1000 &&
        ai->lists[0].count - aoe_ai_tactical_count_busy_103_ror(ai, ops) > 3)
        return 1;
    v = -1;
    if (ops->unit_action) ops->unit_action(ops->context, unit, &v);
    if (v == 0x2c6) {
        int32_t w = -1;
        if (ops->unit_2c && ops->unit_2c(ops->context, unit, &w) && w == 100) return 1;
    }
    if (v == 0x2be) return 1;
    if (e[2] > e[3]) {
        if (task != -1) return 1;
        if (e[1] >= 0 && e[1] < 4) tried[e[1]] = 1;
        if (aoe_ai_tactical_gather_ror(ai, e[0], e[1], 0, NULL, ops, info, info_ops) == 1)
            return 1;
        e[1] = e[2] = e[3] = -1;
    }
    for (k = 0; k < 4; ++k) {
        if (tried[k] == 1 || ai->field_d68[k] >= ai->field_d78[k]) continue;
        if (e[1] == k && ops->unit_task(ops->context, unit, &task) &&
            (task == 0x265 || task == 0x261))
            return 1;
        tried[k] = 1;
        if (aoe_ai_tactical_gather_ror(ai, e[0], k, 0, NULL, ops, info, info_ops) == 1)
            return 1;
    }
    if ((aoe_ai_tactical_age_is_ror(ops, 1) && ai->sn[181] == 1) ||
        (aoe_ai_tactical_age_is_ror(ops, 2) && ai->sn[182] == 1 && tried[0] == 0)) {
        tried[0] = 1;
        r = aoe_ai_tactical_gather_ror(ai, e[0], 0, 1, NULL, ops, info, info_ops);
    } else if (aoe_ai_tactical_age_is_ror(ops, 3) && ai->sn[183] == 1) {
        if (tried[0] == 0) {
            r = aoe_ai_tactical_gather_ror(ai, e[0], 0, 1, NULL, ops, info, info_ops);
            tried[0] = 1;
        }
        if (tried[3] == 0) {
            if (r) return 1;
            tried[3] = 1;
            r = aoe_ai_tactical_gather_ror(ai, e[0], 3, 1, NULL, ops, info, info_ops);
        }
    }
    if (!r) aoe_ai_tactical_gather_ror(ai, e[0], -1, 1, tried, ops, info, info_ops);
    return 1;
}

/* 0x004d2f40: an idle villager, the farthest from our town centre. */
static int32_t round_villager(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor c;
    int32_t centre = ops->first_center ? ops->first_center(ops->context) : -1;
    if (centre != -1 && ops->object && ops->object(ops->context, centre, &c)) {
        int32_t pt[2] = {aoe_ftol32(c.x), aoe_ftol32(c.y)};
        return aoe_ai_tactical_villager_near_ror(ai, -1, pt, 0, ops);
    }
    return aoe_ai_tactical_idle_villager_ror(ai, -1, ops);
}

static int round_spent(const AoeAiTacticalOpsRor *ops, uint32_t start, uint32_t budget) {
    return ops->real_ms && ops->real_ms(ops->context) - start >= budget;
}

/* 0x004d2f40: see the header. */
int32_t aoe_ai_tactical_villager_round_ror(AoeAiTacticalRor *ai, uint32_t start, uint32_t budget,
                                           const AoeAiTacticalOpsRor *ops, AoeAiInfoRor *info,
                                           const AoeAiInfoOpsRor *info_ops) {
    AoeAiIntListRor *idle;
    int32_t done = 0, i, *slot;
    if (!ai || !ops) return 0;
    idle = &ai->lists[1];
    if (ai->lists[0].count < 1) {
        ai->field_f7c = -1;
        return 0;
    }
    if (ai->field_f7c == -1) {
        while (idle->count < idle->field_8) {
            int32_t unit = round_villager(ai, ops);
            if (unit == -1) {
                int32_t k;
                if (ai->field_9d0 >= ai->field_9cc) break;
                for (k = 3; k >= 0 && unit == -1; --k) {
                    AoeAiWorldObjectRor o;
                    int32_t id = ops->first_tasked ? ops->first_tasked(ops->context, 0x261, 0x25d) : -1;
                    if (id == -1 && ops->first_tasked)
                        id = ops->first_tasked(ops->context, 0x265, 0x25d);
                    if (id == -1 || !ops->object || !ops->object(ops->context, id, &o) ||
                        o.field_44 != 0.0f)
                        continue;
                    if (aoe_ai_int_list_contains_ror(idle, id)) continue;
                    aoe_ai_tactical_remove_gatherer_ror(ai, id);
                    unit = id;
                }
                if (unit == -1) break;
            }
            if (!aoe_ai_int_list_add_unique_ror(idle, unit)) break;
        }
        while (idle->count > idle->field_8 && idle->count > 0) {
            int32_t unit;
            slot = aoe_ai_int_list_at_ror(idle, 0);
            if (!slot) break;
            unit = *slot;
            aoe_ai_int_list_remove_ror(idle, unit);
            aoe_ai_tactical_order_stop_ror(ops, unit, 100);
        }
        while (ai->field_9cc < ai->field_9d0) {
            int32_t unit = round_villager(ai, ops);
            if (unit == -1 || !aoe_ai_tactical_add_gatherer_ror(ai, unit)) break;
        }
        slot = aoe_ai_int_list_at_ror(&ai->lists[0], 0);
        ai->field_f7c = slot ? *slot : -1;
        if (round_spent(ops, start, budget)) return 0;
    }
    for (i = 0; i < ai->lists[0].count; ++i) {
        slot = aoe_ai_int_list_at_ror(&ai->lists[0], i);
        if (slot && *slot == ai->field_f7c) break;
    }
    while (i < ai->lists[0].count) {
        int32_t unit;
        ++done;
        slot = aoe_ai_int_list_at_ror(&ai->lists[0], i);
        unit = slot ? *slot : -1;
        if (aoe_ai_int_list_contains_ror(idle, unit)) {
            aoe_ai_tactical_try_explore_ror(ai, unit, 1, ops, info, info_ops);
        } else if (!aoe_ai_tactical_regather_ror(ai, unit, ops, info, info_ops)) {
            aoe_ai_tactical_add_gatherer_ror(ai, unit);
            ++i;
            continue;
        }
        aoe_ai_tactical_plan_gatherers_ror(ai, ops);
        if (++i >= ai->lists[0].count) break;
        slot = aoe_ai_int_list_at_ror(&ai->lists[0], i);
        ai->field_f7c = slot ? *slot : -1;
        if (round_spent(ops, start, budget)) return done;
    }
    ai->field_f7c = -1;
    return done;
}

/* 0x004d6390: a group's target and state cleared. */
static void boat_reset(AoeAiUnitGroupRor *g) {
    g->type = 0;
    g->field_180 = 1;
    g->field_184 = -1;
    g->field_2cc = 0;
    g->field_188 = -1;
    aoe_ai_unit_group_set_location_ror(g->loc_18c, -1.0f, -1.0f, -1.0f);
}

static int boat_skipped(const AoeAiUnitGroupRor *g) {
    return g->task >= 100 && g->task <= 0x69;
}

static void boat_target(AoeAiUnitGroupRor *g, int32_t id, int32_t type, float x, float y,
                        float z) {
    g->field_184 = id;
    g->field_188 = type;
    aoe_ai_unit_group_set_location_ror(g->loc_18c, x, y, z);
    g->type = 1;
}

/* 0x004d6390: the +0x180 10 / 0xb steps; 1 when the original skips the
   next group. */
static int boat_step_10(AoeAiUnitGroupRor *g, const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor o;
    int32_t task;
    if (!ops->object || !ops->object(ops->context, g->field_184, &o)) {
        boat_reset(g);
        return -1;
    }
    if (ops->object(ops->context, g->commander, &o) && ops->unit_task &&
        ops->unit_task(ops->context, g->commander, &task) && task == -1) {
        boat_reset(g);
        return 1;
    }
    return 0;
}

static int boat_step_b(AoeAiUnitGroupRor *g, const AoeAiTacticalOpsRor *ops,
                       const AoeAiUnitGroupOpsRor *group_ops, AoeAiInfoRor *info,
                       const AoeAiInfoOpsRor *info_ops) {
    AoeAiWorldObjectRor o;
    AoeAiInfoObjectRor *dock;
    int32_t task, head;
    if (!ops->object || !ops->object(ops->context, g->field_184, &o)) {
        boat_reset(g);
        return 0;
    }
    if (!ops->object(ops->context, g->commander, &o) || !ops->unit_task ||
        !ops->unit_task(ops->context, g->commander, &task))
        return 0;
    if (task == -1) {
        boat_reset(g);
        return 1;
    }
    if (!ops->head_action_target || !ops->head_action_target(ops->context, g->commander, &head))
        return 0;
    dock = info ? aoe_ai_info_nearest_trade_dock_ror(info, head, info_ops) : NULL;
    if (!dock || dock->id == g->field_184) return 0;
    if (!ops->can_reach_slot ||
        !ops->can_reach_slot(ops->context, g->commander, dock->x, dock->y, dock->z, 1.0f,
                             dock->id)) {
        g->field_180 = 1;
        return 0;
    }
    boat_target(g, dock->id, dock->type_id, (float)dock->x, (float)dock->y, (float)dock->z);
    aoe_ai_unit_group_update_ror(g, group_ops, g->field_180, 1, 1);
    return 0;
}

/* 0x004d6390: see the header. */
void aoe_ai_tactical_boat_groups_ror(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops,
                                     const AoeAiUnitGroupOpsRor *group_ops,
                                     AoeAiInfoRor *info, const AoeAiInfoOpsRor *info_ops) {
    int32_t i;
    if (!ai || !ops) return;
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        int ready;
        if (boat_skipped(g) || (g->field_180 != 0 && g->field_180 != 1)) continue;
        ready = aoe_ai_unit_group_in_range_ror(g, group_ops);
        if (g->field_160 <= aoe_ai_unit_group_count_ror(g)) {
            if (!ready && aoe_ai_unit_group_count_ror(g) > 1) {
                aoe_ai_unit_group_update_ror(g, group_ops, 9, 1, 0);
                continue;
            }
            if (aoe_ai_unit_group_settled_ror(g, group_ops, 1)) {
                if (g->task == 0x6b) g->field_180 = 10;
                else if (g->task == 0x6c) g->field_180 = 0xb;
                else if (g->task == 0x6a) g->field_180 = 1;
                continue;
            }
        }
        if (!ready) aoe_ai_unit_group_update_ror(g, group_ops, 9, 1, 0);
    }
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        AoeAiWorldObjectRor c;
        int skip;
        if (boat_skipped(g) || g->field_180 == 0 || g->field_180 == 1) continue;
        if (g->type == 0) {
            if (g->task == 0x6b || g->task == 0x6c) {
                int open;
                if (!ops->object || !ops->object(ops->context, g->commander, &c)) {
                    g->field_180 = 1;
                    continue;
                }
                if (g->task == 0x6b) {
                    AoeAiWorldObjectRor t;
                    int32_t target = -1, drop = -1, tried[5], n = 0, k;
                    if (aoe_ai_tactical_count_groups_8_ror(ai) > 0 && ops->info_zone_open &&
                        ops->info_zone_open(ops->context, g->commander) == 1) {
                        for (k = 0; k < 5; ++k) {
                            float pos[2] = {0.0f, 0.0f};
                            int r;
                            target = info ? aoe_ai_info_pick_target_ror(info, 0, c.id, pos, &drop,
                                                                        tried, n, info_ops)
                                          : -1;
                            if (drop == -1)
                                r = ops->can_reach &&
                                    ops->can_reach(ops->context, c.id, aoe_ftol32(pos[0]),
                                                   aoe_ftol32(pos[1]), aoe_ftol32(c.z), 1.0f,
                                                   target);
                            else
                                r = ops->can_gather &&
                                    ops->can_gather(ops->context, c.id, target, drop);
                            if (r == 0)
                                tried[n++] = target;
                            else if (r == 1)
                                break;
                        }
                    }
                    if (target >= 0 && ops->object(ops->context, target, &t)) {
                        boat_target(g, target, t.type_id, t.x, t.y, t.z);
                        aoe_ai_unit_group_update_ror(g, group_ops, g->field_180, 1, 0);
                        continue;
                    }
                } else {
                    AoeAiInfoObjectRor *dock =
                        info ? aoe_ai_info_nearest_trade_dock_ror(info, g->commander, info_ops)
                             : NULL;
                    if (dock) {
                        if (!ops->can_reach_slot ||
                            !ops->can_reach_slot(ops->context, c.id, dock->x, dock->y, dock->z,
                                                 1.0f, dock->id)) {
                            g->field_180 = 1;
                            continue;
                        }
                        boat_target(g, dock->id, dock->type_id, (float)dock->x, (float)dock->y,
                                    (float)dock->z);
                        aoe_ai_unit_group_update_ror(g, group_ops, g->field_180, 1, 0);
                        continue;
                    }
                }
                open = ops->info_zone_open && ops->info_zone_open(ops->context, g->commander) == 1;
                if (open) {
                    g->field_180 = 8;
                    aoe_ai_unit_group_update_ror(g, group_ops, g->field_180, 1, 0);
                    g->type = 1;
                } else {
                    g->field_180 = 1;
                }
            } else {
                aoe_ai_unit_group_update_ror(g, group_ops, g->field_180, 1, 0);
            }
            continue;
        }
        if (g->field_180 == 8) {
            int found = g->task == 0x6c && info &&
                        aoe_ai_info_nearest_trade_dock_ror(info, g->commander, info_ops) != NULL;
            if (g->task == 0x6b) {
                if (aoe_ai_tactical_count_groups_8_ror(ai) > 1) {
                    float pos[2] = {0.0f, 0.0f};
                    int32_t drop = -1;
                    if (info && aoe_ai_info_pick_target_ror(info, 0, g->commander, pos, &drop, NULL,
                                                            0, info_ops) != -1)
                        found = 1;
                } else {
                    int32_t action = -1;
                    if (ops->object && ops->object(ops->context, g->commander, &c) &&
                        ops->unit_action && ops->unit_action(ops->context, g->commander, &action) &&
                        action != 0x2c1) {
                        g->field_180 = 8;
                        aoe_ai_unit_group_update_ror(g, group_ops, g->field_180, 1, 0);
                        g->type = 1;
                    }
                }
            }
            if (found) {
                if (!ops->object || !ops->object(ops->context, g->commander, &c)) continue;
                boat_reset(g);
                aoe_ai_unit_group_set_location_ror(g->loc_19c, c.x, c.y, c.z);
                aoe_ai_unit_group_update_ror(g, group_ops, g->field_180, 1, 0);
                continue;
            }
        }
        skip = 0;
        if (g->field_180 == 10) {
            int r = boat_step_10(g, ops);
            if (r == -1) continue;
            if (r == 1) {
                /* The original moves on and runs the 0xb step on the next
                   group, then passes it. */
                if (++i >= ai->groups_len) break;
                g = ai->groups[i];
            }
        }
        if (g->field_180 == 0xb) skip = boat_step_b(g, ops, group_ops, info, info_ops);
        if (skip) ++i;
    }
}

/* The index of a group (the array shifts when groups are deleted). */
static int32_t group_index(const AoeAiTacticalRor *ai, const AoeAiUnitGroupRor *g) {
    int32_t i;
    for (i = 0; i < ai->groups_len; ++i)
        if (ai->groups[i] == g) return i;
    return -1;
}

/* 0x004dee90 / 0x004dcf40: the candidate filter of the soldier version:
   the unit must be in zone `zone` (a commander's own zone; -1, when the
   group has none, matches nothing). FILL_ANY_ZONE: no filter (boats). */
enum { FILL_ANY_ZONE = -2 };
static int fill_zone_ok(int32_t zone, int32_t id, const AoeAiInfoOpsRor *info_ops) {
    if (zone == FILL_ANY_ZONE) return 1;
    return info_ops && info_ops->zone && (int32_t)info_ops->zone(info_ops->context, id) == zone;
}

/* 0x004dee90 / 0x004dcf40: +0xd48's candidates join `g`: all of them, or
   (`nearest`) the nearest to its +0x19c location one at a time until it
   is full; `move` takes each out of its group first (0x004e0930). */
static void fill_from_d48(AoeAiTacticalRor *ai, AoeAiUnitGroupRor *g, int nearest, int move,
                          int32_t zone, const AoeAiTacticalOpsRor *ops,
                          const AoeAiUnitGroupOpsRor *group_ops, const AoeAiInfoOpsRor *info_ops) {
    int32_t j, *slot;
    if (!nearest) {
        for (j = 0; j < ai->list_d48.count; ++j) {
            AoeAiWorldObjectRor o;
            slot = aoe_ai_int_list_at_ror(&ai->list_d48, j);
            if (!slot) break;
            if (zone != FILL_ANY_ZONE &&
                (!ops->object || !ops->object(ops->context, *slot, &o) ||
                 !fill_zone_ok(zone, *slot, info_ops)))
                continue;
            if (move) aoe_ai_tactical_ungroup_ror(ai, *slot, group_ops);
            slot = aoe_ai_int_list_at_ror(&ai->list_d48, j);
            if (slot) aoe_ai_unit_group_add_ror(g, *slot, group_ops);
        }
        return;
    }
    while (aoe_ai_unit_group_count_ror(g) < g->field_160) {
        int32_t best = -1;
        float best_d = -1.0f;
        if (ai->list_d48.count < 1) break;
        for (j = 0; j < ai->list_d48.count; ++j) {
            AoeAiWorldObjectRor o;
            float d;
            slot = aoe_ai_int_list_at_ror(&ai->list_d48, j);
            if (!slot || !ops->object || !ops->object(ops->context, *slot, &o) ||
                !fill_zone_ok(zone, *slot, info_ops))
                continue;
            d = (float)(ops->distance ? ops->distance(ops->context, *slot, g->loc_19c[0],
                                                      g->loc_19c[1], g->loc_19c[2])
                                      : 0.0);
            if (best_d == -1.0f || d < best_d) {
                best = *slot;
                best_d = d;
            }
        }
        if (best == -1) break;
        if (move) aoe_ai_tactical_ungroup_ror(ai, best, group_ops);
        aoe_ai_unit_group_add_ror(g, best, group_ops);
        aoe_ai_int_list_remove_ror(&ai->list_d48, best);
    }
}

/* 0x004dee90 / 0x004dcf40: a commander-less group gets one; 0 when no
   candidate is left (the caller then ends). */
static int lead_group(AoeAiTacticalRor *ai, AoeAiUnitGroupRor *g, int32_t list, int boats,
                      const AoeAiTacticalOpsRor *ops, const AoeAiUnitGroupOpsRor *group_ops) {
    int32_t found = aoe_ai_tactical_collect_ungrouped_ror(ai, list);
    if (aoe_ai_unit_group_count_ror(g) > 0) {
        aoe_ai_unit_group_set_commander_ror(g, aoe_ai_tactical_pick_leader_ror(ai, g, ops),
                                            group_ops);
    } else {
        int32_t leader;
        if (found == 0) aoe_ai_tactical_collect_surplus_ror(ai, boats);
        if (ai->list_d48.count == 0) return 0;
        leader = aoe_ai_tactical_pick_leader_ids_ror(ai, &ai->list_d48, ops);
        aoe_ai_unit_group_add_ror(g, leader, group_ops);
        aoe_ai_unit_group_set_commander_ror(g, leader, group_ops);
    }
    if (g->field_180 == 0 || g->field_180 == 1) aoe_ai_tactical_rally_group_ror(ai, g, ops);
    return 1;
}

/* 0x004dee90 / 0x004dcf40: a short group takes ungrouped units of `list`,
   then the surplus. */
static void top_up_group(AoeAiTacticalRor *ai, AoeAiUnitGroupRor *g, int32_t list, int boats,
                         int32_t zone, const AoeAiTacticalOpsRor *ops,
                         const AoeAiUnitGroupOpsRor *group_ops, const AoeAiInfoOpsRor *info_ops) {
    int32_t n;
    if (aoe_ai_unit_group_count_ror(g) >= g->field_160) return;
    n = aoe_ai_tactical_collect_ungrouped_ror(ai, list);
    fill_from_d48(ai, g, n > g->field_160 - aoe_ai_unit_group_count_ror(g), 0, zone, ops,
                  group_ops, info_ops);
    if (aoe_ai_unit_group_count_ror(g) >= g->field_160) return;
    n = aoe_ai_tactical_collect_surplus_ror(ai, boats);
    fill_from_d48(ai, g, n > g->field_160 - aoe_ai_unit_group_count_ror(g), 1, zone, ops,
                  group_ops, info_ops);
}

/* 0x004dee90: see the header. */
void aoe_ai_tactical_fill_boat_groups_ror(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops,
                                          const AoeAiUnitGroupOpsRor *group_ops) {
    int32_t i, t, j;
    if (!ai || !ops) return;
    aoe_ai_tactical_collect_ungrouped_ror(ai, 4);
    aoe_ai_tactical_collect_surplus_ror(ai, 1);
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        if (g->task < 0x67 || g->task > 0x69 || g->commander != -1) continue;
        if (!lead_group(ai, g, 4, 1, ops, group_ops)) return;
    }
    if (ai->sn[40] == 0) {
        for (t = 0x67; t <= 0x6a; ++t) {
            for (i = 0; i < ai->groups_len; ++i) {
                AoeAiUnitGroupRor *g = ai->groups[i];
                if (g->task != t) continue;
                top_up_group(ai, g, 4, 1, FILL_ANY_ZONE, ops, group_ops, NULL);
                i = group_index(ai, g);
                if (i < 0) break;
            }
        }
    }
    if (aoe_ai_tactical_collect_ungrouped_ror(ai, 4) == 0) return;
    for (j = 0; j < ai->list_d48.count; ++j) {
        AoeAiUnitGroupRor *best = NULL;
        int32_t best_v = 0, *slot;
        for (i = 0; i < ai->groups_len; ++i) {
            AoeAiUnitGroupRor *g = ai->groups[i];
            int32_t n = aoe_ai_unit_group_count_ror(g);
            if (g->task < 0x67 || g->task > 0x6c || n >= g->field_160 ||
                (g->field_180 != 1 && g->field_180 != 0))
                continue;
            if (!best || g->field_160 - n > best_v) {
                best = g;
                best_v = g->field_160 - n;
            }
        }
        if (!best) {
            for (i = 0; i < ai->groups_len; ++i) {
                AoeAiUnitGroupRor *g = ai->groups[i];
                int32_t n = aoe_ai_unit_group_count_ror(g);
                if (g->task < 0x67 || g->task > 0x6c ||
                    (g->field_180 != 1 && g->field_180 != 0))
                    continue;
                if ((g->task == 0x67 && n >= ai->sn[60]) || (g->task == 0x68 && n >= ai->sn[69]) ||
                    (g->task == 0x69 && n >= ai->sn[63]))
                    continue;
                if (!best || n - g->field_160 < best_v) {
                    best = g;
                    best_v = n - g->field_160;
                }
            }
            if (!best) return;
        }
        slot = aoe_ai_int_list_at_ror(&ai->list_d48, j);
        if (slot) aoe_ai_unit_group_add_ror(best, *slot, group_ops);
    }
}

static int soldier_task(int32_t t) { return t == 100 || t == 0x65 || t == 0x66; }

/* 0x004dcf40: a group's commander zone for the candidate filter. */
static int32_t commander_zone(const AoeAiUnitGroupRor *g, const AoeAiTacticalOpsRor *ops,
                              const AoeAiInfoOpsRor *info_ops) {
    AoeAiWorldObjectRor o;
    if (!ops->object || !ops->object(ops->context, g->commander, &o) || !info_ops ||
        !info_ops->zone)
        return -1;
    return info_ops->zone(info_ops->context, g->commander);
}

/* 0x004dcf40: see the header. */
void aoe_ai_tactical_fill_soldier_groups_ror(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops,
                                             const AoeAiUnitGroupOpsRor *group_ops,
                                             const AoeAiInfoOpsRor *info_ops) {
    int32_t i, t, j;
    if (!ai || !ops) return;
    aoe_ai_tactical_collect_ungrouped_ror(ai, 2);
    aoe_ai_tactical_collect_surplus_ror(ai, 0);
    for (i = 0; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        if (!soldier_task(g->task) || g->commander != -1) continue;
        if (!lead_group(ai, g, 2, 0, ops, group_ops)) return;
    }
    if (ai->sn[40] == 0) {
        for (t = 100; t <= 0x66; ++t) {
            for (i = 0; i < ai->groups_len; ++i) {
                AoeAiUnitGroupRor *g = ai->groups[i];
                if (g->task != t || aoe_ai_unit_group_count_ror(g) >= g->field_160 || g->type == 1)
                    continue;
                top_up_group(ai, g, 2, 0, commander_zone(g, ops, info_ops), ops, group_ops,
                             info_ops);
                i = group_index(ai, g);
                if (i < 0) break;
            }
        }
    }
    if (aoe_ai_tactical_collect_ungrouped_ror(ai, 2) == 0) return;
    for (j = 0; j < ai->list_d48.count; ++j) {
        AoeAiUnitGroupRor *best = NULL;
        AoeAiWorldObjectRor o;
        int32_t best_v = 0, zone, *slot = aoe_ai_int_list_at_ror(&ai->list_d48, j);
        if (!slot || !ops->object || !ops->object(ops->context, *slot, &o)) continue;
        zone = info_ops && info_ops->zone ? info_ops->zone(info_ops->context, *slot) : 0;
        for (i = 0; i < ai->groups_len; ++i) {
            AoeAiUnitGroupRor *g = ai->groups[i];
            int32_t n = aoe_ai_unit_group_count_ror(g);
            if (!soldier_task(g->task) || g->type == 1 || commander_zone(g, ops, info_ops) != zone ||
                n >= g->field_160 || (g->field_180 != 1 && g->field_180 != 0))
                continue;
            if (!best || g->field_160 - n > best_v) {
                best = g;
                best_v = g->field_160 - n;
            }
        }
        if (!best) {
            for (i = 0; i < ai->groups_len; ++i) {
                AoeAiUnitGroupRor *g = ai->groups[i];
                int32_t n = aoe_ai_unit_group_count_ror(g);
                if (!soldier_task(g->task) || g->type == 1 ||
                    (g->field_180 != 1 && g->field_180 != 0))
                    continue;
                if ((g->task == 100 && n >= ai->sn[26]) || (g->task == 0x65 && n >= ai->sn[28]) ||
                    (g->task == 0x66 && n >= ai->sn[44]))
                    continue;
                if (commander_zone(g, ops, info_ops) != zone) continue;
                if (!best || n - g->field_160 < best_v) {
                    best = g;
                    best_v = n - g->field_160;
                }
            }
            if (!best) return;
        }
        slot = aoe_ai_int_list_at_ror(&ai->list_d48, j);
        if (slot) aoe_ai_unit_group_add_ror(best, *slot, group_ops);
    }
}

/* 0x004d3ab0: the +0xfac block as the information AI's scan state. */
static void scan_load(const AoeAiTacticalRor *ai, AoeAiInfoGroupScanRor *s) {
    const int32_t *b = ai->block_fac;
    s->best_id = b[0];
    memcpy(&s->best_score, &b[1], 4);
    s->best_index = b[2];
    s->next = b[3];
    s->field_10 = b[4];
    s->field_14 = b[5];
    s->walls = (uint8_t)b[6];
    s->wall_id = b[7];
    memcpy(&s->wall_score, &b[8], 4);
    s->wall_index = b[9];
    s->running = b[10];
}

static void scan_store(AoeAiTacticalRor *ai, const AoeAiInfoGroupScanRor *s) {
    int32_t *b = ai->block_fac;
    b[0] = s->best_id;
    memcpy(&b[1], &s->best_score, 4);
    b[2] = s->best_index;
    b[3] = s->next;
    b[4] = s->field_10;
    b[5] = s->field_14;
    b[6] = (b[6] & ~0xff) | s->walls;
    b[7] = s->wall_id;
    memcpy(&b[8], &s->wall_score, 4);
    b[9] = s->wall_index;
    b[10] = s->running;
}

enum { GT_DONE, GT_NEXT, GT_SUSPEND };

typedef struct {
    AoeAiTacticalRor *ai;
    const AoeAiTacticalOpsRor *ops;
    const AoeAiUnitGroupOpsRor *gops;
    const AoeAiStrategyRor *strategy;
    AoeAiInfoRor *info;
    const AoeAiInfoOpsRor *iops;
    uint32_t start;
} GroupTaskCtx;

static int32_t gt_now(const GroupTaskCtx *c) {
    return c->ops->game_time ? (int32_t)c->ops->game_time(c->ops->context) : 0;
}

static uint32_t gt_secs_since(const GroupTaskCtx *c, int32_t t) {
    return (uint32_t)(gt_now(c) - t) / 1000u;
}

static double gt_range(const GroupTaskCtx *c, int32_t id) {
    return c->ops->range ? c->ops->range(c->ops->context, id) : 0.0;
}

static int gt_198(const GroupTaskCtx *c, int32_t id, int32_t x, int32_t y, int32_t z, float r,
                  int32_t target, int32_t a, int32_t b) {
    return c->ops->reach_198 ? c->ops->reach_198(c->ops->context, id, x, y, z, r, target, a, b) : 0;
}

static int gt_1a4(const GroupTaskCtx *c, int32_t id, int32_t x, int32_t y, int32_t z, float r,
                  int32_t target, int32_t a, int32_t b) {
    return c->ops->reach_1a4 ? c->ops->reach_1a4(c->ops->context, id, x, y, z, r, target, a, b) : 0;
}

static int gt_transport(const GroupTaskCtx *c, int32_t id) {
    return c->ops->wants_transport && c->ops->wants_transport(c->ops->context, id) == 1;
}

static int gt_engage(const GroupTaskCtx *c, int32_t id, int32_t target) {
    return c->ops->can_engage &&
           c->ops->can_engage(c->ops->context, id, target, (float)gt_range(c, id)) == 1;
}

/* The group's target and state cleared (+0x184, +0x188, +0x18c, +0x2cc,
   +0x180 1, type 0). */
static void gt_reset(AoeAiUnitGroupRor *g) {
    g->type = 0;
    g->field_180 = 1;
    g->field_184 = -1;
    g->field_2cc = 0;
    g->field_188 = -1;
    aoe_ai_unit_group_set_location_ror(g->loc_18c, -1.0f, -1.0f, -1.0f);
}

static void gt_target(AoeAiUnitGroupRor *g, int32_t id, int32_t type, float x, float y, float z) {
    g->field_184 = id;
    g->field_188 = type;
    aoe_ai_unit_group_set_location_ror(g->loc_18c, x, y, z);
}

/* Boarding a transport for target `id`: task 0xe, +0x180 0xd, +0x1c0 200. */
static void gt_board(const GroupTaskCtx *c, AoeAiUnitGroupRor *g, int32_t id, int32_t type,
                     float x, float y, float z) {
    aoe_ai_unit_group_update_ror(g, c->gops, 0xe, 1, 0);
    g->field_180 = 0xd;
    gt_target(g, id, type, x, y, z);
    g->type = 1;
    g->field_1c0 = 200;
}

/* The +0xd28 first player's sliced scan target; GT_SUSPEND while it runs. */
static int gt_scan(const GroupTaskCtx *c, AoeAiUnitGroupRor *g, AoeAiInfoObjectRor **out) {
    AoeAiInfoGroupScanRor scan;
    int32_t *first = aoe_ai_int_list_at_ror(&c->ai->list_d28, 0);
    scan_load(c->ai, &scan);
    *out = c->info ? aoe_ai_info_group_target_ror(c->info, (uint32_t)(first ? *first : 0), g,
                                                  &scan, c->start, c->iops)
                   : NULL;
    scan_store(c->ai, &scan);
    return c->ai->block_fac[10] == 1 ? GT_SUSPEND : GT_DONE;
}

/* 0x004d3ab0: a type-0 group finds its target. */
static int gt_type0(const GroupTaskCtx *c, AoeAiUnitGroupRor *g) {
    AoeAiTacticalRor *ai = c->ai;
    const AoeAiTacticalOpsRor *ops = c->ops;
    AoeAiInfoObjectRor *t = NULL;
    AoeAiWorldObjectRor cmd;
    int have_cmd = 0;
    int32_t found = 1;
    if (g->task == 100 || g->task == 0x67) {
        uint32_t secs;
        int32_t kind;
        if (ai->field_e18 == 0) {
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        if (ai->sn[162] != 0 && g->task != 0x67) {
            /* The original reads the target ID through a null pointer. */
            g->field_180 = 1;
            return GT_DONE;
        }
        secs = gt_secs_since(c, ai->field_d0c);
        if ((ai->field_d0c != 0 && ai->sn[47] == 2 && secs != 0 &&
             secs < (uint32_t)aoe_ai_tactical_d24_limit_ror(ai, 0x2e)) ||
            (ai->field_d0c != 0 && ai->sn[47] == 1 &&
             secs < (uint32_t)aoe_ai_tactical_d24_limit_ror(ai, 0x2e))) {
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        kind = c->strategy ? aoe_ai_strategy_ai_victory_kind_ror(c->strategy) : -1;
        if (kind == 0 || kind == 4) {
            int32_t want = kind == 0 ? aoe_ai_tactical_free_target_ror(ai, c->strategy)
                                     : aoe_ai_tactical_free_second_ror(ai, c->strategy);
            t = c->info ? aoe_ai_info_find_object_ror(c->info, want) : NULL;
            if (!t && c->info) {
                int32_t *first = aoe_ai_int_list_at_ror(&ai->list_d28, 0);
                t = aoe_ai_info_engage_target_ror(c->info, (uint32_t)(first ? *first : 0), 1, 1,
                                                  g->commander, &found, c->iops);
            }
        } else if (gt_scan(c, g, &t) == GT_SUSPEND) {
            return GT_SUSPEND;
        }
        if (!t) {
            g->field_180 = 1;
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        ++t->field_10;
        if (!ops->object || !ops->object(ops->context, g->commander, &cmd)) {
            g->field_180 = 1;
            return GT_DONE;
        }
        have_cmd = 1;
        if (gt_198(c, cmd.id, t->x, t->y, t->z, (float)gt_range(c, cmd.id), t->id, t->owner,
                   0x1b)) {
            ai->field_d0c = gt_now(c);
            if (ai->sn[71] == 1) ai->field_d14 = gt_now(c);
            gt_target(g, t->id, t->type_id, t->x, t->y, t->z);
            g->type = 1;
            aoe_ai_tactical_share_with_allies_ror(ai, t->owner, t->x, t->y, ops);
        } else {
            if (g->task == 0x67) {
                g->field_180 = 1;
                g->field_1d0 = -1;
                return GT_DONE;
            }
            if (!gt_1a4(c, cmd.id, t->x, t->y, t->z, (float)gt_range(c, cmd.id), t->id, t->owner,
                        0x1b)) {
                g->field_2cc = 0;
                g->field_180 = 1;
                g->field_1d0 = -1;
                ai->block_fac[10] = 0;
                return GT_NEXT;
            }
            if (!gt_transport(c, cmd.id)) {
                g->field_180 = 1;
                g->field_1d0 = -1;
                return GT_DONE;
            }
            gt_board(c, g, t->id, t->type_id, t->x, t->y, t->z);
            g->field_1d0 = -1;
            return GT_DONE;
        }
    } else if (g->task == 0x65 || g->task == 0x68) {
        float r2;
        t = c->info ? aoe_ai_info_free_resource_ror(c->info, g->commander, c->iops) : NULL;
        if (!t || !ops->object || !ops->object(ops->context, g->commander, &cmd)) {
            g->field_180 = 1;
            return GT_DONE;
        }
        have_cmd = 1;
        r2 = (float)(t->type_id == 0x6d ? ai->sn[22] : ai->sn[57]);
        if (gt_198(c, cmd.id, t->x, t->y, t->z, (float)gt_range(c, cmd.id), t->id, -1, -1)) {
            t->field_10 = 1;
            gt_target(g, t->id, t->type_id, t->x, t->y, t->z);
            aoe_ai_unit_group_set_location_ror(g->loc_19c, t->x, t->y, t->z);
            g->type = 1;
        } else if (gt_1a4(c, cmd.id, t->x, t->y, t->z, r2, t->id, -1, -1) &&
                   gt_transport(c, cmd.id)) {
            t->field_10 = 1;
            gt_board(c, g, t->id, t->type_id, t->x, t->y, t->z);
            return GT_DONE;
        } else {
            g->field_180 = 1;
            return GT_DONE;
        }
    } else if (g->task == 0x6d) {
        AoeAiWorldObjectRor tc;
        int32_t centre, x, y, z, b4e;
        float r;
        if (gt_secs_since(c, g->field_1cc) < 60) {
            g->field_180 = 1;
            return GT_DONE;
        }
        centre = ops->first_center ? ops->first_center(ops->context) : -1;
        if (centre == -1 || !ops->object || !ops->object(ops->context, centre, &tc) ||
            !ops->object(ops->context, g->commander, &cmd)) {
            g->field_180 = 1;
            return GT_DONE;
        }
        have_cmd = 1;
        x = aoe_ftol32(tc.x);
        y = aoe_ftol32(tc.y);
        z = aoe_ftol32(tc.z);
        r = (float)ai->sn[23];
        b4e = ops->object_4e ? ops->object_4e(ops->context, tc.id) : 0;
        if (gt_198(c, cmd.id, x, y, z, r, tc.id, -1, -1)) {
            gt_target(g, tc.id, b4e, tc.x, tc.y, tc.z);
            aoe_ai_unit_group_set_location_ror(g->loc_19c, tc.x, tc.y, tc.z);
            g->type = 1;
        } else if (gt_1a4(c, cmd.id, x, y, z, r, tc.id, -1, -1)) {
            if (!gt_transport(c, cmd.id)) {
                g->field_180 = 1;
                return GT_DONE;
            }
            gt_board(c, g, tc.id, b4e, tc.x, tc.y, tc.z);
            return GT_DONE;
        } else {
            g->field_1cc = gt_now(c);
            g->field_180 = 1;
            return GT_DONE;
        }
    }
    if (c->strategy && aoe_ai_strategy_ai_victory_kind_ror(c->strategy) == 0xb &&
        (g->task == 100 || g->task == 0x67) && ai->sn[103] == 1 && t && have_cmd) {
        aoe_ai_info_penalize_objects_ror(c->info, t->owner, t->id, c->iops);
        aoe_ai_info_penalize_areas_ror(c->info, t->owner, c->iops);
        if (ops->path_to) {
            AoeUnitPathRor *path = ops->path_to(ops->context, cmd.id, t->x, t->y, 1,
                                                (float)gt_range(c, cmd.id), t->id);
            if (path) aoe_ai_tactical_path_waypoints_ror(g, path);
        }
    }
    if (found == 0 && g->field_180 == 2)
        aoe_ai_unit_group_update_ror(g, c->gops, 0x15, 1, 0);
    else
        aoe_ai_unit_group_update_ror(g, c->gops, g->field_180, 1, 0);
    return GT_DONE;
}

/* 0x004d3ab0, +0x180 0xd: boarding the transport. */
static int gt_boarding(const GroupTaskCtx *c, AoeAiUnitGroupRor *g) {
    AoeAiTacticalRor *ai = c->ai;
    const AoeAiTacticalOpsRor *ops = c->ops;
    AoeAiUnitGroupRor *boat;
    AoeAiWorldObjectRor o;
    int32_t k, any = 0;
    if (g->field_1c0 != 200) {
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    if (g->field_1c4 == -1) {
        int32_t pt[2];
        if (!ops->object || !ops->object(ops->context, g->commander, &o)) {
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        pt[0] = aoe_ftol32(o.x);
        pt[1] = aoe_ftol32(o.y);
        boat = aoe_ai_tactical_nearest_group_ror(ai, 0x6a, 1, -1, pt, -1, ops);
        if (!boat) boat = aoe_ai_tactical_nearest_group_ror(ai, 0x6a, 0, -1, pt, -1, ops);
        if (boat) {
            g->field_1c4 = boat->id;
            g->field_1c8 = boat->task;
            boat->field_180 = 0xc;
            boat->type = 1;
            boat->field_1c4 = g->id;
        }
    } else {
        boat = aoe_ai_tactical_find_group_ror(ai, g->field_1c4, -1, -1, -1);
    }
    if (!boat) {
        g->field_1c4 = -1;
        g->field_1c8 = -1;
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    if (ops->object && ops->object(ops->context, boat->commander, &o) && ops->transport_full &&
        ops->transport_full(ops->context, boat->commander)) {
        g->field_180 = 0xf;
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    for (k = 0; k < aoe_ai_unit_group_count_ror(g); ++k) {
        int32_t m = aoe_ai_unit_group_member_ror(g, k), task;
        if (ops->object && ops->object(ops->context, m, &o) && ops->unit_task &&
            ops->unit_task(ops->context, m, &task) && task == -1) {
            any = 1;
            aoe_ai_tactical_order_2cd_ror(ai, m, boat->commander, ops);
        }
    }
    if (!any) g->field_180 = 0xf;
    ai->block_fac[10] = 0;
    return GT_NEXT;
}

/* 0x004d3ab0, +0x180 0xf: loading, then sailing. */
static int gt_loading(const GroupTaskCtx *c, AoeAiUnitGroupRor *g) {
    AoeAiTacticalRor *ai = c->ai;
    const AoeAiTacticalOpsRor *ops = c->ops;
    AoeAiUnitGroupRor *boat = aoe_ai_tactical_find_group_ror(ai, g->field_1c4, -1, -1, -1);
    AoeAiWorldObjectRor o;
    int32_t k, all_in = 1, x, y, out[2];
    int32_t zone = 0xff;
    if (!boat || !ops->object || !ops->object(ops->context, boat->commander, &o)) {
        g->field_184 = -1;
        g->field_2cc = 0;
        g->field_188 = -1;
        g->field_180 = 1;
        g->type = 0;
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    if (ops->transport_full && ops->transport_full(ops->context, boat->commander))
        aoe_ai_unit_group_prune_ror(g, c->gops);
    for (k = 0; k < aoe_ai_unit_group_count_ror(g); ++k) {
        int32_t m = aoe_ai_unit_group_member_ror(g, k), task, held, action;
        if (!ops->object(ops->context, m, &o)) {
            aoe_ai_log_ror(&ai->module, "    Unit %d is invalid.", m);
            continue;
        }
        if (!ops->unit_task || !ops->unit_task(ops->context, m, &task)) {
            aoe_ai_log_ror(&ai->module, "    Unit %d has invalid unitAI.", m);
            continue;
        }
        if (m == g->commander && c->iops && c->iops->tile_zone)
            zone = c->iops->tile_zone(c->iops->context, m, aoe_ftol32(g->loc_18c[0]),
                                      aoe_ftol32(g->loc_18c[1]));
        held = ops->container_of ? ops->container_of(ops->context, m) : -1;
        if (held != -1 && held == boat->commander) {
            if (task != -1) aoe_ai_tactical_order_stop_ror(ops, m, 100);
        } else if (held != -1) {
            aoe_ai_unit_group_remove_ror(g, m, c->gops);
        } else {
            all_in = 0;
            action = -1;
            if (ops->unit_action) ops->unit_action(ops->context, m, &action);
            if (action != 0x2bc && action != 0x2cd)
                aoe_ai_tactical_order_2cd_ror(ai, aoe_ai_unit_group_member_ror(g, k),
                                              boat->commander, ops);
        }
    }
    if (!all_in) return GT_DONE;
    x = aoe_ftol32(g->loc_18c[0]);
    y = aoe_ftol32(g->loc_18c[1]);
    if (!c->info || !aoe_ai_info_approach_tile_ror(c->info, x, y, out, 1, 1, g->field_184, 0xf,
                                                   g->commander, c->iops)) {
        out[0] = x;
        out[1] = y;
    }
    aoe_ai_tactical_order_2ca_ror(ai, boat->commander, (float)out[0] + 0.5f,
                                  (float)out[1] + 0.5f, ops);
    aoe_ai_unit_group_set_location_ror(boat->loc_18c, (float)out[0], (float)out[1], -1.0f);
    boat->field_180 = 0xc;
    boat->field_188 = zone & 0xff;
    g->field_180 = 0x10;
    aoe_ai_unit_group_update_ror(g, c->gops, 0x16, 0, 1);
    if (aoe_ai_unit_group_count_ror(boat) > 1) {
        for (k = 0; k < aoe_ai_unit_group_count_ror(boat); ++k) {
            int32_t m = aoe_ai_unit_group_member_ror(boat, k);
            if (m == boat->commander || !ops->object(ops->context, m, &o)) continue;
            aoe_ai_tactical_order_2bd_ror(ai, m, boat->commander, o.type_28 * 0.5f, 0x63, ops);
        }
    }
    return GT_DONE;
}

/* 0x004d3ab0, +0x180 0x10: unloading. GT_DONE + 1 falls through to the
   later states. */
enum { GT_FALL = 3 };
static int gt_landing(const GroupTaskCtx *c, AoeAiUnitGroupRor *g) {
    AoeAiTacticalRor *ai = c->ai;
    const AoeAiTacticalOpsRor *ops = c->ops;
    AoeAiUnitGroupRor *boat = aoe_ai_tactical_find_group_ror(ai, g->field_1c4, -1, -1, -1);
    AoeAiWorldObjectRor bc, o, cmd;
    int32_t k, all_out = 1, idle = 1, task;
    if (!boat || !ops->object || !ops->object(ops->context, boat->commander, &bc) ||
        !ops->unit_task || !ops->unit_task(ops->context, boat->commander, &task) ||
        bc.owner != ops->self) {
        g->field_184 = -1;
        g->field_2cc = 0;
        g->field_188 = -1;
        g->field_180 = 1;
        g->type = 0;
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    for (k = 0; k < aoe_ai_unit_group_count_ror(g); ++k) {
        int32_t m = aoe_ai_unit_group_member_ror(g, k);
        AoeAiWorldObjectRor t;
        if (!ops->object(ops->context, m, &o) || !ops->unit_task(ops->context, m, &task)) continue;
        if (ops->container_of && ops->container_of(ops->context, m) != -1) {
            all_out = 0;
            continue;
        }
        idle = 0;
        if (g->task == 100 && ops->object(ops->context, g->field_184, &t)) {
            int32_t cur = -1;
            if (ops->unit_ai_target) ops->unit_ai_target(ops->context, m, &cur);
            if (cur != g->field_184 && gt_engage(c, m, g->field_184))
                aoe_ai_tactical_order_attack_ror(ai, m, g->loc_18c[0], g->loc_18c[1],
                                                 g->field_184, -1, NULL, 0, 0, ops);
        } else {
            aoe_ai_tactical_order_stop_ror(ops, m, 100);
        }
    }
    if (idle == 1) {
        int32_t state = 0;
        if (ops->head_action_state && ops->head_action_state(ops->context, boat->commander, &state) &&
            state != 2)
            return GT_DONE;
        aoe_ai_tactical_order_2ca_ror(ai, boat->commander, boat->loc_18c[0] + 0.5f,
                                      boat->loc_18c[1] + 0.5f, ops);
        boat->field_180 = 0xc;
        return GT_DONE;
    }
    if (all_out == 0) {
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    {
        AoeAiUnitGroupRor *b2 = aoe_ai_tactical_find_group_ror(ai, g->field_1c4, -1, -1, -1);
        if (b2) {
            AoeAiWorldObjectRor tc;
            int32_t centre;
            b2->field_184 = -1;
            b2->field_188 = -1;
            b2->field_180 = 1;
            b2->type = 0;
            centre = ops->first_center ? ops->first_center(ops->context) : -1;
            if (centre != -1 && ops->object(ops->context, centre, &tc))
                aoe_ai_tactical_order_move_ror(ops, b2->commander, tc.x, tc.y, 0x32);
        }
    }
    if (g->task == 100) {
        if (ops->object(ops->context, g->commander, &cmd) && gt_engage(c, cmd.id, g->field_184)) {
            ai->field_d0c = gt_now(c);
            if (ai->sn[71] == 1) ai->field_d14 = gt_now(c);
            g->field_180 = 2;
            aoe_ai_unit_group_update_ror(g, c->gops, g->field_180, 1, 0);
            return GT_DONE;
        }
        g->type = 1;
        g->field_180 = 7;
        aoe_ai_unit_group_update_ror(g, c->gops, 7, 0, 1);
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    if (g->task == 0x65 || g->task == 0x6d) {
        aoe_ai_unit_group_set_location_ror(g->loc_19c, g->loc_18c[0], g->loc_18c[1],
                                           g->loc_18c[2]);
        g->type = 0;
        g->field_180 = 1;
        aoe_ai_unit_group_update_ror(g, c->gops, 1, 1, 1);
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    return GT_FALL;
}

/* 0x004d3ab0: +0x180 2 with +0x1d0 -1, the attack. */
static int gt_attack(const GroupTaskCtx *c, AoeAiUnitGroupRor *g) {
    AoeAiTacticalRor *ai = c->ai;
    const AoeAiTacticalOpsRor *ops = c->ops;
    AoeAiWorldObjectRor o, tgt, cmd;
    int32_t k;
    if (ai->sn[91] < 100) {
        for (k = 0; k < aoe_ai_unit_group_count_ror(g);) {
            int32_t m = aoe_ai_unit_group_member_ror(g, k), v, pct = 0;
            if (!ops->object || !ops->object(ops->context, m, &o)) {
                ++k;
                continue;
            }
            v = aoe_ai_unit_group_value_ror(g, k);
            if (v > 0) pct = (v - aoe_ftol32(o.hp)) * 100 / v;
            if (pct <= ai->sn[91]) {
                ++k;
                continue;
            }
            aoe_ai_tactical_order_move_ror(ops, m, g->loc_1ac[0], g->loc_1ac[1], 100);
            aoe_ai_unit_group_remove_ror(g, aoe_ai_unit_group_member_ror(g, k), c->gops);
            if (aoe_ai_unit_group_count_ror(g) == 0) break;
        }
    }
    if (ops->object && ops->object(ops->context, g->field_184, &tgt) && tgt.state <= 2) {
        if (g->field_18 == -1) {
            if (ops->is_ally && ops->is_ally(ops->context, tgt.owner)) {
                gt_reset(g);
                if (ops->object(ops->context, g->commander, &cmd))
                    aoe_ai_unit_group_set_location_ror(g->loc_19c, cmd.x, cmd.y, cmd.z);
                return GT_DONE;
            }
            if (aoe_ai_unit_group_settled_ror(g, c->gops, 0) == 1) {
                if (!ops->object(ops->context, g->commander, &cmd)) {
                    ai->block_fac[10] = 0;
                    return GT_NEXT;
                }
                if (gt_engage(c, cmd.id, g->field_184)) {
                    aoe_ai_unit_group_update_ror(g, c->gops, 0x14, 0, 1);
                    return GT_DONE;
                }
                aoe_ai_unit_group_set_location_ror(g->loc_19c, cmd.x, cmd.y, cmd.z);
                gt_reset(g);
                return GT_DONE;
            }
            aoe_ai_unit_group_update_ror(g, c->gops, 0x14, 0, 1);
            return GT_DONE;
        }
        if (g->field_18 == 4 && tgt.owner == ops->self && ops->active_unit &&
            ops->active_unit(ops->context, tgt.id)) {
            const float *p1, *p2;
            aoe_ai_unit_group_add_ror(g, tgt.id, c->gops);
            p1 = c->strategy ? aoe_ai_strategy_ai_target_point_ror(c->strategy) : NULL;
            p2 = c->strategy ? aoe_ai_strategy_ai_second_point_ror(c->strategy) : NULL;
            gt_reset(g);
            if (p1 && p2)
                aoe_ai_unit_group_set_location_ror(g->loc_19c, (p2[0] + p1[0]) * 0.5f,
                                                   (p2[1] + p1[1]) * 0.5f, (p2[2] + p1[2]) * 0.5f);
        }
        if (gt_secs_since(c, g->field_1cc) > (uint32_t)ai->sn[76]) {
            gt_reset(g);
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        return GT_FALL;
    }
    if (ai->sn[49] == 2) {
        gt_reset(g);
        aoe_ai_unit_group_update_ror(g, c->gops, g->field_180, 1, 0);
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    if (!ops->object || !ops->object(ops->context, g->commander, &cmd)) {
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    {
        AoeAiInfoObjectRor *t;
        int32_t old;
        if (gt_scan(c, g, &t) == GT_SUSPEND) return GT_SUSPEND;
        if (t && gt_198(c, cmd.id, t->x, t->y, t->z, (float)gt_range(c, cmd.id), t->id, t->owner,
                        0x1b) == 1) {
            gt_target(g, t->id, t->type_id, t->x, t->y, t->z);
            g->type = 1;
            aoe_ai_unit_group_update_ror(g, c->gops, 2, 0, 0);
            return GT_DONE;
        }
        old = g->field_188;
        gt_reset(g);
        if (ai->sn[49] == 0) {
            aoe_ai_unit_group_set_location_ror(g->loc_19c, cmd.x, cmd.y, cmd.z);
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        if (ai->sn[49] != 3 || old == 0x7e || old == 0x59) {
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        if (ai->sn[135] == 1) {
            int32_t r = ai->sn[136], y = aoe_ftol32(cmd.y), x = aoe_ftol32(cmd.x);
            aoe_ai_info_mark_rect_ror(c->info, x - r, y - r, x + r, y + r, c->iops);
        }
        g->type = 1;
        g->field_180 = 7;
        aoe_ai_unit_group_update_ror(g, c->gops, 7, 0, 1);
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
}

/* 0x004d3ab0: the type-1 states after the transport ones. */
static int gt_states(const GroupTaskCtx *c, AoeAiUnitGroupRor *g) {
    AoeAiTacticalRor *ai = c->ai;
    const AoeAiTacticalOpsRor *ops = c->ops;
    AoeAiWorldObjectRor o;
    int r;
    if (g->field_180 == 0xc) return GT_DONE;
    if (g->field_180 == 0x13) {
        uint32_t secs;
        if (aoe_ai_unit_group_in_range_ror(g, c->gops)) return GT_DONE;
        secs = gt_secs_since(c, g->field_1cc);
        if (aoe_ai_unit_group_settled_ror(g, c->gops, 1) == 1 && secs > 5) {
            g->field_1cc = gt_now(c);
            aoe_ai_unit_group_update_ror(g, c->gops, 9, 1, 0);
        }
        return GT_DONE;
    }
    if (g->field_180 == 3) {
        if (gt_secs_since(c, g->field_1cc) > (uint32_t)ai->sn[76]) {
            gt_reset(g);
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        if (aoe_ai_unit_group_in_range_ror(g, c->gops)) {
            g->field_180 = 1;
            g->type = 0;
            if (g->task == 0x65 || g->task == 0x68) {
                AoeAiInfoObjectRor *t =
                    c->info ? aoe_ai_info_find_object_ror(c->info, g->field_184) : NULL;
                if (t) t->field_10 = 0;
            }
            if ((ops->build_count ? ops->build_count(ops->context) : 0) == 0 &&
                ((g->task == 100 && aoe_ai_tactical_count_groups_ror(ai, 100, -1) > 1) ||
                 g->task != 100)) {
                if (g->task == 0x66) --ai->sn[42];
                else if (g->task == 0x65) --ai->sn[38];
                else if (g->task == 100) --ai->sn[36];
                aoe_ai_unit_group_clear_ror(g, c->gops);
                g->field_180 = 0;
            }
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
    }
    if (g->field_180 == 7) {
        AoeAiWorldObjectRor cmd, to;
        AoeAiInfoObjectRor *t;
        int have_cmd = ops->object && ops->object(ops->context, g->commander, &cmd);
        if (gt_scan(c, g, &t) == GT_SUSPEND) return GT_SUSPEND;
        if (have_cmd && t && ops->object(ops->context, t->id, &to)) {
            if (gt_198(c, cmd.id, t->x, t->y, t->z, (float)gt_range(c, cmd.id), t->id, t->owner,
                       0x1b) == 1) {
                g->field_180 = 2;
                gt_target(g, t->id, t->type_id, t->x, t->y, t->z);
                g->type = 1;
                aoe_ai_unit_group_update_ror(g, c->gops, 2, 0, 1);
                return GT_DONE;
            }
            if (gt_1a4(c, cmd.id, t->x, t->y, t->z, (float)gt_range(c, cmd.id), t->id, t->owner,
                       0x1b) == 1 &&
                gt_transport(c, cmd.id)) {
                gt_board(c, g, t->id, t->type_id, t->x, t->y, t->z);
                g->field_1d0 = -1;
                return GT_DONE;
            }
        }
        if (gt_secs_since(c, g->field_1cc) > (uint32_t)ai->sn[88] * 4u) {
            g->field_180 = 7;
            aoe_ai_unit_group_update_ror(g, c->gops, 7, 0, 1);
            return GT_DONE;
        }
    }
    if (g->field_180 == 2 && g->field_1d0 == -1) {
        r = gt_attack(c, g);
        if (r != GT_FALL) return r;
    }
    if (g->field_180 == 2 && g->field_1d0 != -1) {
        if (!ops->object || !ops->object(ops->context, g->commander, &o) || !ops->unit_task) {
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
        {
            int32_t task;
            if (!ops->unit_task(ops->context, g->commander, &task)) {
                ai->block_fac[10] = 0;
                return GT_NEXT;
            }
        }
        if (!ops->unit_ai_12c || ops->unit_ai_12c(ops->context, g->commander) == 0)
            g->field_1d0 = -1;
        ai->block_fac[10] = 0;
        return GT_NEXT;
    }
    if (g->field_180 == 4) {
        AoeAiWorldObjectRor t;
        if (aoe_ai_unit_group_settled_ror(g, c->gops, 1) == 1 &&
            !aoe_ai_unit_group_in_range_ror(g, c->gops)) {
            aoe_ai_unit_group_update_ror(g, c->gops, 9, 1, 0);
            return GT_DONE;
        }
        if (!ops->object || !ops->object(ops->context, g->field_184, &t) ||
            (c->info && aoe_ai_info_better_resource_ror(c->info, g->commander, g->field_188,
                                                        t.type_class, c->iops))) {
            if (ops->object && ops->object(ops->context, g->field_184, &t) && c->info) {
                AoeAiInfoObjectRor *f = aoe_ai_info_find_object_ror(c->info, g->field_184);
                if (f) f->field_10 = 0;
            }
            g->field_180 = 1;
            g->type = 0;
            ai->block_fac[10] = 0;
            return GT_NEXT;
        }
    }
    if (g->field_180 == 3 || g->field_180 == 0xd) return GT_NEXT;
    {
        int32_t hp = aoe_ai_unit_group_hit_points_ror(g, c->gops), a = 0, b = 0;
        if (g->field_168 != 0) a = (g->field_168 - hp) * 100 / g->field_168;
        if (g->field_16c != 0) b = (g->field_16c - aoe_ai_unit_group_count_ror(g)) * 100 /
                                   g->field_16c;
        if (a > ai->sn[30] || b > ai->sn[31]) {
            g->field_180 = 3;
            aoe_ai_unit_group_update_ror(g, c->gops, 3, 1, 0);
            return GT_DONE;
        }
    }
    return GT_NEXT;
}

/* 0x004d3ab0: see the header. */
void aoe_ai_tactical_group_tasks_ror(AoeAiTacticalRor *ai, uint32_t start, uint32_t budget,
                                     const AoeAiTacticalOpsRor *ops,
                                     const AoeAiUnitGroupOpsRor *group_ops,
                                     const AoeAiStrategyRor *strategy, AoeAiInfoRor *info,
                                     const AoeAiInfoOpsRor *info_ops) {
    GroupTaskCtx c;
    int32_t i = 0;
    if (!ai || !ops) return;
    c.ai = ai;
    c.ops = ops;
    c.gops = group_ops;
    c.strategy = strategy;
    c.info = info;
    c.iops = info_ops;
    c.start = start;
    if (ai->field_f84 != -1) {
        while (i < ai->groups_len && ai->groups[i]->id != ai->field_f84) ++i;
        if (i == ai->groups_len) i = 0;
    }
    for (; i < ai->groups_len; ++i) {
        AoeAiUnitGroupRor *g = ai->groups[i];
        int r;
        if (g->task == 0x6b || g->task == 0x6c || g->task == 0x6a || g->field_180 == 0 ||
            g->field_180 == 1)
            continue;
        if (!aoe_ai_unit_group_settled_ror(g, group_ops, 1)) g->field_1cc = gt_now(&c);
        if (g->type == 0) {
            r = gt_type0(&c, g);
        } else if (g->field_180 == 0xd) {
            r = gt_boarding(&c, g);
        } else if (g->field_180 == 0xf) {
            r = gt_loading(&c, g);
        } else {
            r = g->field_180 == 0x10 ? gt_landing(&c, g) : GT_FALL;
            if (r == GT_FALL) r = gt_states(&c, g);
        }
        if (r == GT_SUSPEND) {
            ai->field_f84 = g->id;
            return;
        }
        i = group_index(ai, g) < 0 ? i - 1 : group_index(ai, g);
        if (r == GT_DONE) ai->block_fac[10] = 0;
        if (ai->block_fac[10] != 0) return;
        ai->field_f84 = i + 1 < ai->groups_len ? ai->groups[i + 1]->id : -1;
        if (ops->real_ms && ops->real_ms(ops->context) - start >= budget) return;
    }
    ai->field_f84 = -1;
}

static int32_t gt_now_ops(const AoeAiTacticalOpsRor *ops) {
    return ops->game_time ? (int32_t)ops->game_time(ops->context) : 0;
}

/* 0x004d7880: the attacker's owner turns enemy. */
static void notify_turn_enemy(AoeAiDiplomacyRor *dip, const AoeAiDiplomacyOps *dip_ops,
                              int32_t p) {
    aoe_ai_diplomacy_set_changeable_ror(dip, dip_ops, p, 1);
    aoe_ai_diplomacy_set_ror(dip, dip_ops, p, AOE_AI_DISLIKE, 100);
    aoe_ai_diplomacy_request_ror(dip, dip_ops, p, 3);
    aoe_ai_diplomacy_set_changeable_ror(dip, dip_ops, p, 0);
}

static int notify_reach(const AoeAiTacticalOpsRor *ops, int32_t id, const AoeAiWorldObjectRor *a) {
    double r = ops->range ? ops->range(ops->context, id) : 0.0;
    return ops->can_reach && ops->can_reach(ops->context, id, aoe_ftol32(a->x), aoe_ftol32(a->y),
                                            aoe_ftol32(a->z), (float)r, a->id) == 1;
}

/* 0x004d7880: see the header. */
void aoe_ai_tactical_notify_ror(AoeAiTacticalRor *ai, int32_t unit, int32_t code, int32_t other,
                                const AoeAiTacticalOpsRor *ops,
                                const AoeAiUnitGroupOpsRor *group_ops, AoeAiInfoRor *info,
                                const AoeAiInfoOpsRor *info_ops, AoeAiDiplomacyRor *dip,
                                const AoeAiDiplomacyOps *dip_ops) {
    AoeAiWorldObjectRor ours, a, tc, cmd;
    int have_ours, have_tc = 0;
    uint32_t start;
    int32_t n, sent, k, e, centre, pt[2];
    AoeAiUnitGroupRor *g;
    if (!ai || !ops || !ops->world_object) return;
    have_ours = ops->world_object(ops->context, unit, &ours);
    if (code != 0x72 && code != 0x74 && !have_ours) return;
    start = ops->real_ms ? ops->real_ms(ops->context) : 0;
    if (code == 0x72 || code == 0x74) {
        AoeAiInfoObjectRor *t;
        int32_t limit;
        if (!info) return;
        t = code == 0x72 ? aoe_ai_info_best_9f_ror(info, (uint32_t)-1, info_ops)
                         : aoe_ai_info_best_9e_ror(info, (uint32_t)-1, info_ops);
        if (!t) return;
        n = aoe_ai_tactical_collect_by_order_ror(ai, 100, -1, 1, ops);
        limit = n > 3 ? 3 : n;
        sent = 0;
        for (k = 0; k < ai->list_d48.count && sent < limit; ++k) {
            AoeAiWorldObjectRor o;
            int32_t task, id = ai->list_d48.data[k];
            if (!ops->world_object(ops->context, id, &o) || !ops->unit_task ||
                !ops->unit_task(ops->context, id, &task) || !ops->can_engage ||
                !ops->can_engage(ops->context, id, t->id, 2.0f))
                continue;
            if (aoe_ai_tactical_order_2bd_ror(ai, id, t->id, 2.0f, 100, ops) == 1) {
                aoe_ai_tactical_ungroup_ror(ai, ai->list_d48.data[k], group_ops);
                ++sent;
            }
        }
        return;
    }
    if (code != 0x201) return;
    if (!ops->world_object(ops->context, other, &a) || a.owner == ops->self) return;
    if (a.owner == 0 && (ai->field_fa4 == 0 || (a.type_id != 0x7e && a.type_id != 0x59)) &&
        aoe_ai_tactical_defend_object_ror(ai, a.id, ours.id, ops) == 1)
        return;
    if (a.owner != 0) {
        int in0 = aoe_ai_int_list_contains_ror(&ai->lists[0], unit);
        int alarm = 1;
        if (!in0 || aoe_ai_int_list_contains_ror(&ai->lists[1], unit)) {
            int32_t action = -1, task;
            alarm = ops->unit_task && ops->unit_task(ops->context, unit, &task) &&
                    ops->unit_action && ops->unit_action(ops->context, unit, &action) &&
                    action != 0x2c1;
        }
        if (alarm) {
            int32_t d = ops->world_100 ? ops->world_100(ops->context) : 0;
            if (d == 3) ai->sn[104] -= 2;
            else if (d == 4) --ai->sn[104];
            else aoe_ai_tactical_arm_e18_ror(ai, 3);
            aoe_ai_tactical_upgrades_ror(ai, a.owner, ops, info, info_ops);
        }
    }
    e = a.owner;
    if (e >= 0 && e < 9) ++ai->block_fd8[e];
    if ((ops->player_field_4 ? ops->player_field_4(ops->context, e) : 0) == 0) {
        if (ops->is_ally && ops->is_ally(ops->context, e) == 1 && e >= 0 && e < 9 &&
            ai->block_fd8[e] >= ai->sn[221] && ai->sn[215] == 1)
            notify_turn_enemy(dip, dip_ops, e);
        else {
            aoe_ai_diplomacy_add_ror(dip, dip_ops, e, AOE_AI_LIKE, ai->sn[178]);
            aoe_ai_diplomacy_add_ror(dip, dip_ops, e, AOE_AI_DISLIKE, ai->sn[178]);
        }
    } else if (ops->is_ally && ops->is_ally(ops->context, e) == 1 && ops->player_18 &&
               ops->player_18(ops->context, e) == 1) {
        notify_turn_enemy(dip, dip_ops, e);
    }
    if (ai->sn[124] > 0 && ai->sn[128] == e && ai->sn[133] == 1 && ops->befriend)
        ops->befriend(ops->context);
    if (info)
        aoe_ai_info_add_tile_area_ror(info, 0, (uint8_t)aoe_ftol32(ours.x),
                                      (uint8_t)aoe_ftol32(ours.y), (uint8_t)a.owner,
                                      (uint8_t)ops->self, 0, 0,
                                      ops->game_time ? ops->game_time(ops->context) : 0, -1,
                                      info_ops);
    if (ai->field_d14 != 0 &&
        (uint32_t)(gt_now_ops(ops) - ai->field_d14) / 1000u <
            (uint32_t)aoe_ai_tactical_d24_limit_ror(ai, 0x30))
        return;
    pt[0] = aoe_ftol32(a.x);
    pt[1] = aoe_ftol32(a.y);
    if (aoe_class_is_boat_ror(a.type_class) == 1) {
        g = aoe_ai_tactical_ready_group_ror(ai, 0x67, ai->sn[20], pt, group_ops);
    } else {
        g = aoe_ai_tactical_ready_group_ror(ai, 100, ai->sn[20], pt, group_ops);
        if (!g) g = aoe_ai_tactical_ready_group_ror(ai, 0x67, ai->sn[20], pt, group_ops);
    }
    if (g && ops->world_object(ops->context, g->commander, &cmd) && notify_reach(ops, cmd.id, &a)) {
        g->field_184 = other;
        g->field_188 = a.type_id;
        aoe_ai_unit_group_set_location_ror(g->loc_18c, a.x, a.y, a.z);
        g->field_180 = 2;
        aoe_ai_unit_group_update_ror(g, group_ops, 2, 1, 0);
        ai->field_d14 = gt_now_ops(ops);
        if (ai->sn[71] == 1) ai->field_d0c = gt_now_ops(ops);
        return;
    }
    centre = ops->first_center ? ops->first_center(ops->context) : -1;
    if (centre != -1 && ops->object && ops->object(ops->context, centre, &tc)) have_tc = 1;
    if (have_tc &&
        aoe_ftol32(sqrt((double)(tc.x - a.x) * (tc.x - a.x) + (double)(tc.y - a.y) * (tc.y - a.y))) < 20) {
        n = aoe_ai_tactical_collect_support_ror(ai, 100, -1, ops) +
            aoe_ai_tactical_collect_by_order_ror(ai, 100, -1, 0, ops);
        if (n == 0 && aoe_class_is_boat_ror(a.type_class) == 0 && ai->sn[225] == 1 &&
            (uint32_t)gt_now_ops(ops) / 1000u < (uint32_t)ai->sn[104]) {
            for (k = 0; k < ai->lists[0].count; ++k) {
                int32_t *slot = aoe_ai_int_list_at_ror(&ai->lists[0], k);
                if (slot) aoe_ai_int_list_add_unique_ror(&ai->list_d48, *slot);
            }
            n = ai->lists[0].count;
        }
    } else {
        if (aoe_class_is_boat_ror(a.type_class) == 1)
            n = aoe_ai_tactical_collect_support_ror(ai, ai->sn[20], -1, ops);
        else
            n = aoe_ai_tactical_collect_support_ror(ai, ai->sn[20], -1, ops) +
                aoe_ai_tactical_collect_by_order_ror(ai, ai->sn[20], -1, 0, ops);
        n = ai->sn[19] * n / 100;
    }
    if (have_tc)
        aoe_ai_tactical_share_with_allies_ror(ai, a.owner, aoe_ftol32(tc.x), aoe_ftol32(tc.y), ops);
    else
        aoe_ai_tactical_share_with_allies_ror(ai, a.owner, aoe_ftol32(a.x), aoe_ftol32(a.y), ops);
    sent = 0;
    for (k = 0; k < ai->list_d48.count && sent < n; ++k) {
        AoeAiWorldObjectRor m;
        int32_t id, task, v = 0;
        if (k > 1 && ops->real_ms && ops->real_ms(ops->context) - start > 10) break;
        id = ai->list_d48.data[k];
        if (!ops->world_object(ops->context, id, &m) || !ops->unit_task ||
            !ops->unit_task(ops->context, id, &task))
            continue;
        if (task == 600) {
            int32_t t = -1;
            if (ops->unit_ai_target) ops->unit_ai_target(ops->context, id, &t);
            if (t != -1) {
                AoeAiWorldObjectRor to;
                if (t == a.id) {
                    ++sent;
                    continue;
                }
                if (ops->world_object(ops->context, t, &to)) {
                    int32_t *first = aoe_ai_int_list_at_ror(&ai->list_d28, 0);
                    if (first && to.owner == *first) continue;
                }
            }
        }
        if (ops->unit_2c) ops->unit_2c(ops->context, id, &v);
        if (id == unit) {
            if (v == 100 || !notify_reach(ops, id, &a)) continue;
            if (aoe_ai_tactical_order_attack_ror(ai, ai->list_d48.data[k], a.x, a.y, a.id, a.owner,
                                                 NULL, 0, 0, ops) == 1) {
                ai->field_d14 = gt_now_ops(ops);
                if (ai->sn[71] == 1) ai->field_d0c = gt_now_ops(ops);
                ++sent;
            }
        } else {
            float d = (float)(ops->object_distance ? ops->object_distance(ops->context, unit, id)
                                                   : 0.0);
            int32_t eff = aoe_ai_tactical_effort_ror(ai, d);
            if (!((float)ai->sn[20] > d) || v >= eff || eff <= 0 || !notify_reach(ops, id, &a))
                continue;
            if (aoe_ai_tactical_order_attack_ror(ai, ai->list_d48.data[k], a.x, a.y, a.id, a.owner,
                                                 NULL, 0, 0, ops) == 1)
                ++sent;
        }
    }
}

/* 0x004d1b70: enough explored and builders below number 4 (or a wonder
   pending / set). */
static int step_builders_free(const AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops) {
    if (!aoe_ai_tactical_explored_enough_ror(ai, ops)) return 0;
    return (ops->count_tasked ? ops->count_tasked(ops->context, 0x25a) : 0) < ai->sn[4] ||
           ai->field_e44 != 0 || ai->field_e48 != 0;
}

static void step_repair(AoeAiTacticalRor *ai, int32_t unit_class, int32_t type,
                        const AoeAiTacticalOpsRor *ops) {
    AoeAiWorldObjectRor h;
    int32_t id, v, tile[2];
    if (!step_builders_free(ai, ops)) return;
    id = ops->healthiest_damaged ? ops->healthiest_damaged(ops->context, unit_class, type) : -1;
    if (id == -1 || !ops->object || !ops->object(ops->context, id, &h) || h.state != 2) return;
    if (ops->first_tasked_on && ops->first_tasked_on(ops->context, 0x26a, id) != -1) return;
    tile[0] = aoe_ftol32(h.x);
    tile[1] = aoe_ftol32(h.y);
    v = aoe_ai_tactical_pick_unit_ror(ai, tile, 1, 0x2be, 0x2ce, 0x2bc, 0, ops);
    if (v != -1) aoe_ai_tactical_order_repair_ror(ai, v, &h, ops);
}

static int step_queue_ready(const AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops) {
    return (ops->build_count ? ops->build_count(ops->context) : 0) > 0 &&
           ai->field_d20 >= ai->sn[101];
}

static void step_timing(AoeAiTacticalRor *ai, uint32_t now, uint32_t step_start) {
    ai->field_ffc = (int32_t)(now - step_start);
    ai->field_1008 += ai->field_ffc;
    ++ai->field_1004;
    if (ai->field_1004 >= 20) {
        ai->field_1000 = ai->field_1008 / ai->field_1004;
        ai->field_1008 = 0;
        ai->field_1004 = 0;
    }
}

/* 0x004d1b70: see the header. */
int aoe_ai_tactical_step_ror(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops,
                             const AoeAiUnitGroupOpsRor *group_ops,
                             const AoeAiStrategyRor *strategy, AoeAiInfoRor *info,
                             const AoeAiInfoOpsRor *info_ops) {
    uint32_t budget, start, now = 0;
    if (!ai || !ops) return 0;
    budget = ops->world_budget ? ops->world_budget(ops->context) : 0;
    start = ops->real_ms ? ops->real_ms(ops->context) : 0;
    if (budget != 0) {
        do {
            uint32_t step_start = ops->real_ms ? ops->real_ms(ops->context) : 0;
            int advance = 1;
            switch (ai->state) {
            case 0:
                aoe_ai_tactical_setup_soldier_groups_ror(ai, ops, group_ops);
                break;
            case 1:
                if (ai->lists[0].count > 0) {
                    if (ai->field_e20 == 0) {
                        ai->field_e20 = 1;
                        aoe_ai_tactical_gather_order_ror(ai, ops);
                    }
                    aoe_ai_tactical_plan_villagers_ror(ai, ops);
                }
                break;
            case 2:
                aoe_ai_tactical_villager_round_ror(ai, start, budget, ops, info, info_ops);
                if (ai->field_f7c != -1) advance = 0;
                break;
            case 3:
                if (ai->lists[4].count > 0)
                    aoe_ai_tactical_setup_boat_groups_ror(ai, ops, group_ops);
                break;
            case 4:
                if (ai->lists[4].count > 0)
                    aoe_ai_tactical_fill_boat_groups_ror(ai, ops, group_ops);
                break;
            case 5:
                if (ai->lists[4].count > 0)
                    aoe_ai_tactical_boat_groups_ror(ai, ops, group_ops, info, info_ops);
                break;
            case 6:
                if (ai->lists[4].count > 0 || ai->lists[2].count > 0)
                    aoe_ai_tactical_fill_soldier_groups_ror(ai, ops, group_ops, info_ops);
                break;
            case 7:
                if (ai->lists[4].count > 0 || ai->lists[2].count > 0 || ai->lists[9].count > 0) {
                    aoe_ai_tactical_run_groups_ror(ai, start, budget, ops, group_ops, strategy,
                                                   info, info_ops);
                    if (ai->field_f80 != -1) advance = 0;
                }
                break;
            case 8:
                if (ai->lists[4].count > 0 || ai->lists[2].count > 0 || ai->lists[9].count > 0) {
                    aoe_ai_tactical_group_tasks_ror(ai, start, budget, ops, group_ops, strategy,
                                                    info, info_ops);
                    if (ai->field_f84 != -1) advance = 0;
                }
                break;
            case 9:
                if (ai->lists[2].count > 0) aoe_ai_tactical_start_play_ror(ai, ops, group_ops);
                break;
            case 10: {
                AoeAiWorldObjectRor tc;
                int32_t centre = ops->first_center ? ops->first_center(ops->context) : -1;
                if (centre != -1 && ops->object && ops->object(ops->context, centre, &tc) && info) {
                    int32_t enemy = aoe_ai_info_nearest_enemy_ror(info, tc.x, tc.y, 0x14, 0x4f,
                                                                  199, 0x45, 0x116, info_ops);
                    if (enemy != -1) aoe_ai_tactical_defend_ror(ai, enemy, ops, group_ops);
                }
                break;
            }
            case 11:
                ++ai->field_d20;
                if (step_queue_ready(ai, ops) && ops->build_next_of) {
                    AoeAiBuildItemRor *item = ops->build_next_of(ops->context, 1);
                    if (item) aoe_ai_tactical_research_ror(ai, item, ops);
                }
                break;
            case 12:
                if (ops->resource && ops->resource(ops->context, 11) < ops->resource(ops->context, 32) &&
                    step_queue_ready(ai, ops) && ops->build_next_of) {
                    AoeAiBuildItemRor *item = ops->build_next_of(ops->context, 2);
                    if (item) aoe_ai_tactical_train_ror(ai, item, ops);
                }
                break;
            case 13:
                if (step_queue_ready(ai, ops)) {
                    AoeAiBuildItemRor *item =
                        ops->build_next_of ? ops->build_next_of(ops->context, 3) : NULL;
                    if (item) {
                        if (ops->load_build_file) ops->load_build_file(ops->context, item);
                        aoe_ai_tactical_gather_order_ror(ai, ops);
                    }
                    ai->field_d20 = 0;
                }
                break;
            case 14:
                if (step_builders_free(ai, ops)) {
                    AoeAiWorldObjectRor f;
                    int32_t id = ops->first_unbuilt ? ops->first_unbuilt(ops->context) : -1;
                    if (id != -1 && ops->object && ops->object(ops->context, id, &f) &&
                        (ai->field_e44 != 0 || !ops->first_tasked_on ||
                         ops->first_tasked_on(ops->context, 0x25a, id) == -1)) {
                        int32_t tile[2] = {aoe_ftol32(f.x), aoe_ftol32(f.y)};
                        int32_t v = aoe_ai_tactical_pick_unit_ror(ai, tile, 1, 0x2be, 0x2ce, 0x2bc,
                                                                  0, ops);
                        if (v != -1 && aoe_ai_tactical_order_help_build_ror(ai, v, &f, ops) == 1)
                            ai->field_f90 = gt_now_ops(ops);
                    }
                }
                break;
            case 15:
                step_repair(ai, 3, ai->field_e48 == 1 ? 0x114 : -1, ops);
                break;
            case 16:
                step_repair(ai, 0x1b, -1, ops);
                break;
            case 17:
                if (aoe_ai_tactical_explored_enough_ror(ai, ops) && ai->field_e44 == 0 &&
                    ((ops->count_tasked ? ops->count_tasked(ops->context, 0x25a) : 0) < ai->sn[4] ||
                     ai->field_e48 != 0) &&
                    ops->build_next_of) {
                    AoeAiBuildItemRor *item = ops->build_next_of(ops->context, 0);
                    if (item && aoe_ai_tactical_start_build_ror(ai, -1, item, NULL, (int32_t)start,
                                                                ops) == 1)
                        ai->field_f90 = gt_now_ops(ops);
                }
                break;
            case 18:
                if (ai->block_e4c[69] == 1) {
                    AoeAiConItemRor *site =
                        ops->con_choose ? ops->con_choose(ops->context, ai->search_item,
                                                          ai->block_e4c[66], ai->block_e4c,
                                                          (int32_t)start)
                                        : NULL;
                    if (site && aoe_ai_tactical_start_build_ror(ai, ai->block_e4c[66],
                                                                ai->search_item, site,
                                                                (int32_t)start, ops) == 1)
                        ai->field_f90 = gt_now_ops(ops);
                    if (ai->block_e4c[69] == 1) advance = 0;
                }
                break;
            case 19: {
                int32_t i;
                aoe_ai_tactical_upkeep_ror(ai, ops, group_ops);
                for (i = 0; i < 4; ++i) {
                    if (ai->field_d98[i] > 0 &&
                        (ops->resource_amount ? ops->resource_amount(ops->context, i) : 0) +
                                ai->field_d98[i] < 0) {
                        aoe_ai_tactical_gather_order_ror(ai, ops);
                        break;
                    }
                }
                aoe_ai_tactical_ask_allies_ror(ai, ops);
                aoe_ai_tactical_tribute_surplus_ror(ai, ops);
                aoe_ai_tactical_research_ages_ror(ai, ops);
                aoe_ai_tactical_retarget_trade_ror(ai, ops);
                aoe_ai_tactical_settle_groups_ror(ai, ops);
                break;
            }
            case 20:
                if (info) {
                    AoeAiWorldObjectRor o;
                    int32_t t = aoe_ai_info_current_target_ror(info, 0);
                    if (!ops->object || !ops->object(ops->context, t, &o))
                        aoe_ai_info_refresh_targets_ror(info, 0, info_ops);
                }
                break;
            case 21:
                aoe_ai_tactical_insert_builds_ror(ai, ops);
                if ((gt_now_ops(ops) - ai->field_f90) / 60000 > 5) {
                    if (ops->build_retry) ops->build_retry(ops->context, ai->field_f88, ai->field_f8c);
                    ai->field_f90 = gt_now_ops(ops);
                }
                now = ops->real_ms ? ops->real_ms(ops->context) : 0;
                if (ops->world_spend) ops->world_spend(ops->context, now - start);
                step_timing(ai, now, step_start);
                ai->state = 0;
                if (ops->world_5204d0) ops->world_5204d0(ops->context, 3);
                return 1;
            default:
                break;
            }
            now = ops->real_ms ? ops->real_ms(ops->context) : 0;
            step_timing(ai, now, step_start);
            if (advance) ++ai->state;
        } while (now - start < budget);
    }
    now = ops->real_ms ? ops->real_ms(ops->context) : 0;
    if (ops->world_spend) ops->world_spend(ops->context, now - start);
    return 1;
}

/* 0x004d0980: see the header. */
int aoe_ai_tactical_update_ror(AoeAiTacticalRor *ai, const AoeAiTacticalOpsRor *ops,
                               const AoeAiUnitGroupOpsRor *group_ops,
                               const AoeAiStrategyRor *strategy, AoeAiInfoRor *info,
                               const AoeAiInfoOpsRor *info_ops, AoeAiDiplomacyRor *dip,
                               const AoeAiDiplomacyOps *dip_ops) {
    int32_t now, disliked, liked, p, count;
    if (!ai || !ops) return 0;
    now = gt_now_ops(ops);
    if ((uint32_t)ai->sn[99] * 60000u < (uint32_t)(now - ai->last_update)) {
        int32_t *dbl[3] = {&ai->sn[30], &ai->sn[31], &ai->sn[91]};
        int k;
        ai->sn[26] += ai->sn[94];
        if (ai->sn[26] < 0) ai->sn[26] = 0;
        ai->sn[16] += ai->sn[93];
        if (ai->sn[16] < 0) ai->sn[16] = 0;
        if (ai->sn[16] > ai->sn[26]) ai->sn[16] = ai->sn[26];
        if (ai->sn[26] < ai->sn[16]) ai->sn[26] = ai->sn[16];
        for (k = 0; k < 3; ++k) {
            *dbl[k] *= 2;
            if (*dbl[k] < 0) *dbl[k] = 0;
            if (*dbl[k] > 100) *dbl[k] = 100;
        }
        ai->last_update = now;
    }
    ai->list_d58.count = 0;
    if (ops->player_ddc_clear) ops->player_ddc_clear(ops->context);
    if ((uint32_t)now / 1000u >= (uint32_t)ai->sn[104]) aoe_ai_tactical_arm_e18_ror(ai, 2);
    disliked = aoe_ai_diplomacy_most_disliked_ror(dip, dip_ops, ai->sn[124], ai->sn[128],
                                                   ai->sn[188], ai->sn[195]);
    liked = aoe_ai_diplomacy_most_liked_ror(dip, dip_ops, ai->sn[124], ai->sn[128]);
    ai->list_d28.count = 0;
    aoe_ai_int_list_add_unique_ror(&ai->list_d28, disliked);
    ai->list_d38.count = 0;
    aoe_ai_int_list_add_unique_ror(&ai->list_d38, ops->self);
    if (disliked != liked) aoe_ai_int_list_add_unique_ror(&ai->list_d38, liked);
    count = ops->player_count;
    for (p = 1; p < count; ++p) {
        if (p == ops->self) continue;
        if (p == disliked) {
            if (!aoe_ai_diplomacy_is_enemy_ror(dip_ops, p) &&
                aoe_ai_diplomacy_changeable_ror(dip, dip_ops, p) &&
                aoe_ai_diplomacy_get_ror(dip, dip_ops, p, AOE_AI_DISLIKE) > 0x4a)
                aoe_ai_diplomacy_request_ror(dip, dip_ops, p, 3);
        } else if (p == liked) {
            if (!aoe_ai_diplomacy_is_ally_ror(dip_ops, p) &&
                aoe_ai_diplomacy_changeable_ror(dip, dip_ops, p) &&
                aoe_ai_diplomacy_get_ror(dip, dip_ops, p, AOE_AI_LIKE) > 0x4a)
                aoe_ai_diplomacy_request_ror(dip, dip_ops, p, 0);
        } else if (aoe_ai_diplomacy_get_ror(dip, dip_ops, p, AOE_AI_LIKE) > ai->sn[29] &&
                   aoe_ai_diplomacy_changeable_ror(dip, dip_ops, p)) {
            if (!aoe_ai_diplomacy_is_ally_ror(dip_ops, p))
                aoe_ai_diplomacy_request_ror(dip, dip_ops, p, 0);
        } else if (!aoe_ai_diplomacy_is_neutral_ror(dip_ops, p) &&
                   aoe_ai_diplomacy_changeable_ror(dip, dip_ops, p)) {
            aoe_ai_diplomacy_request_ror(dip, dip_ops, p, 1);
        }
    }
    aoe_ai_tactical_step_ror(ai, ops, group_ops, strategy, info, info_ops);
    ai->list_d58.count = 0;
    return 1;
}
