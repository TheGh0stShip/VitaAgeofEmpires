#include "ai_tactical.h"
#include "value_grid.h"
#include "aoe_ftol.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"
#include "aoe_stream.h"
#include "movement.h"

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
                                          : NULL))
        return 0;
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
