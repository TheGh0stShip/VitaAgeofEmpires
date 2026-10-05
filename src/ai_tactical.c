#include "ai_tactical.h"
#include "value_grid.h"
#include "aoe_ftol.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"
#include "aoe_stream.h"

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
