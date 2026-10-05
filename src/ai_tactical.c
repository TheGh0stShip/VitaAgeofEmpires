#include "ai_tactical.h"

#include <stdlib.h>
#include <string.h>

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
    for (i = 0; i < AOE_AI_TAC_TARGETS; ++i) fill(ai->targets[i], 5, -1);
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
    for (i = 0; i < AOE_AI_TAC_TARGETS; ++i) put_ints(out, ai->targets[i], 6);
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
        for (i = 0; i < AOE_AI_TAC_TARGETS; ++i) {
            get_ints(in, ai->targets[i], width);
            if (width == 4) ai->targets[i][4] = -1;
            ai->targets[i][5] = 0;
        }
    } else {
        for (i = 0; i < AOE_AI_TAC_TARGETS; ++i) get_ints(in, ai->targets[i], 6);
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

int aoe_ai_tactical_delete_groups_ror(AoeAiTacticalRor *ai, int32_t type) {
    int32_t i = 0;
    if (!ai) return 1;
    while (i < ai->groups_len) {
        if (type == -1 || ai->groups[i]->type == type)
            drop_group(ai, i);
        else
            ++i;
    }
    return 1;
}

AoeAiUnitGroupRor *aoe_ai_tactical_find_group_ror(const AoeAiTacticalRor *ai,
                                                  int32_t id, int32_t type,
                                                  int32_t field_180,
                                                  int32_t field_184) {
    int32_t i;
    if (!ai) return NULL;
    for (i = 0; i < ai->groups_len; ++i) {
        const AoeAiUnitGroupRor *g = ai->groups[i];
        if ((id == -1 || g->id == id) && (type == -1 || g->type == type) &&
            (field_180 == -1 || g->field_180 == field_180) &&
            (field_184 == -1 || g->field_184 == field_184))
            return ai->groups[i];
    }
    return NULL;
}

int32_t aoe_ai_tactical_count_groups_ror(const AoeAiTacticalRor *ai,
                                         int32_t type, int32_t field_180) {
    int32_t i, n = 0;
    if (!ai) return 0;
    for (i = 0; i < ai->groups_len; ++i)
        if ((type == -1 || ai->groups[i]->type == type) &&
            (field_180 == -1 || ai->groups[i]->field_180 == field_180))
            ++n;
    return n;
}

int32_t aoe_ai_tactical_grouped_units_ror(const AoeAiTacticalRor *ai,
                                          int32_t type) {
    int32_t i, n = 0;
    if (!ai) return 0;
    for (i = 0; i < ai->groups_len; ++i)
        if (type == -1 || ai->groups[i]->type == type)
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
