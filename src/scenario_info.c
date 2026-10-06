#include "scenario_info.h"

#include <stdlib.h>
#include <string.h>

static int slot_ok(int32_t slot) {
    return slot >= 0 && slot < AOE_SCENARIO_PLAYERS;
}

void aoe_scenario_enable_all_techs_ror(AoeScenarioInfoRor *info) {
    if (!info) return;
    for (int p = 0; p < AOE_SCENARIO_PLAYERS; ++p)
        for (int t = 0; t < 20; ++t) info->tech_toggles[p][t] = 1;
}

void aoe_scenario_reset_diplomacy_ror(AoeScenarioInfoRor *info) {
    if (!info) return;
    for (int p = 0; p < AOE_SCENARIO_PLAYERS; ++p)
        for (int q = 0; q < AOE_SCENARIO_PLAYERS; ++q)
            info->diplomacy[p][q] = 3;
}

void aoe_scenario_reset_goals_ror(AoeScenarioInfoRor *info) {
    if (!info) return;
    info->custom_conquest = 1;
    info->ruins = info->artifacts = info->discoveries = 0;
    info->explore = info->gold = 0;
    memset(info->conditions, 0, sizeof(info->conditions));
}

void aoe_scenario_set_all_custom_required_ror(AoeScenarioInfoRor *info,
                                              int32_t value) {
    if (info) info->all_custom_required = value < 0 ? 0 : value > 1 ? 1 : value;
}

int32_t aoe_scenario_all_custom_required_ror(const AoeScenarioInfoRor *info) {
    return info ? info->all_custom_required : 0;
}

void aoe_scenario_set_victory_type_ror(AoeScenarioInfoRor *info,
                                       int32_t type) {
    if (!info) return;
    info->victory_type = type;
    if (type != 4) info->conquest = 1;
}

int32_t aoe_scenario_victory_type_ror(const AoeScenarioInfoRor *info) {
    return info ? info->victory_type : 0;
}

void aoe_scenario_set_score_ror(AoeScenarioInfoRor *info, int32_t score) {
    if (info) info->score = score;
}

int32_t aoe_scenario_score_ror(const AoeScenarioInfoRor *info) {
    return info ? info->score : 0;
}

void aoe_scenario_set_time_limit_ror(AoeScenarioInfoRor *info, int32_t time) {
    if (info) info->time_limit = time;
}

int32_t aoe_scenario_time_limit_ror(const AoeScenarioInfoRor *info) {
    return info ? info->time_limit : 0;
}

void aoe_scenario_select_player_ror(AoeScenarioInfoRor *info, int32_t slot) {
    if (!info) return;
    info->selected = slot < 0 ? 0 : slot > 16 ? 15 : slot;
}

void aoe_scenario_set_diplomacy_ror(AoeScenarioInfoRor *info, int32_t from,
                                    int32_t to, int32_t stance) {
    if (!info || (stance != 1 && stance != 3 && stance != 0) ||
        !slot_ok(from) || !slot_ok(to)) return;
    info->diplomacy[from][to] = stance;
}

int32_t aoe_scenario_diplomacy_ror(const AoeScenarioInfoRor *info,
                                   int32_t from, int32_t to) {
    if (!info || !slot_ok(from) || !slot_ok(to)) return -1;
    return info->diplomacy[from][to];
}

void aoe_scenario_set_start_resource_ror(AoeScenarioInfoRor *info,
                                         int32_t slot, int field,
                                         int32_t value) {
    if (info && slot_ok(slot) && field >= 0 && field < 4)
        info->start_resources[slot][field] = value;
}

int32_t aoe_scenario_start_resource_ror(const AoeScenarioInfoRor *info,
                                        int32_t slot, int field) {
    if (!info || !slot_ok(slot) || field < 0 || field >= 4) return 0;
    return info->start_resources[slot][field];
}

static int32_t *goal(AoeScenarioInfoRor *info, int which) {
    switch (which) {
    case AOE_SCENARIO_GOAL_CONQUEST: return &info->custom_conquest;
    case AOE_SCENARIO_GOAL_RUINS: return &info->ruins;
    case AOE_SCENARIO_GOAL_ARTIFACTS: return &info->artifacts;
    case AOE_SCENARIO_GOAL_DISCOVERIES: return &info->discoveries;
    case AOE_SCENARIO_GOAL_EXPLORE: return &info->explore;
    case AOE_SCENARIO_GOAL_GOLD: return &info->gold;
    default: return NULL;
    }
}

void aoe_scenario_set_goal_ror(AoeScenarioInfoRor *info, int which,
                               int32_t value) {
    int32_t *field = info ? goal(info, which) : NULL;
    if (!field) return;
    if (which == AOE_SCENARIO_GOAL_CONQUEST) {
        value = value != 0;
        info->conquest = (uint8_t)value;
    }
    *field = value;
}

int32_t aoe_scenario_goal_ror(const AoeScenarioInfoRor *info, int which) {
    int32_t *field = info ? goal((AoeScenarioInfoRor *)info, which) : NULL;
    return field ? *field : 0;
}

AoeScenarioConditionRor *aoe_scenario_condition_ror(AoeScenarioInfoRor *info,
                                                    int32_t index) {
    if (!info || !slot_ok(info->selected) || index < 0 ||
        index >= AOE_SCENARIO_CONDITIONS) return NULL;
    return &info->conditions[info->selected][index];
}

void aoe_scenario_clear_condition_ror(AoeScenarioInfoRor *info,
                                      int32_t index) {
    AoeScenarioConditionRor *c = aoe_scenario_condition_ror(info, index);
    if (c) memset(c, 0, sizeof(*c));
}

void aoe_scenario_set_condition_area_ror(AoeScenarioInfoRor *info,
                                         int32_t index, float x0, float y0,
                                         float x1, float y1) {
    AoeScenarioConditionRor *c = aoe_scenario_condition_ror(info, index);
    if (!c) return;
    c->area[0] = x0;
    c->area[1] = y0;
    c->area[2] = x1;
    c->area[3] = y1;
}

int aoe_scenario_condition_area_ror(AoeScenarioInfoRor *info, int32_t index,
                                    float *x0, float *y0, float *x1,
                                    float *y1) {
    AoeScenarioConditionRor *c = aoe_scenario_condition_ror(info, index);
    if (!c || !x0 || !y0 || !x1 || !y1) return 0;
    *x0 = c->area[0];
    *y0 = c->area[1];
    *x1 = c->area[2];
    *y1 = c->area[3];
    return 1;
}

void aoe_scenario_set_condition_unit_ror(AoeScenarioInfoRor *info,
                                         int32_t index,
                                         const AoeOutcomeUnitRef *unit) {
    AoeScenarioConditionRor *c = aoe_scenario_condition_ror(info, index);
    if (c && unit) c->unit_ref = unit->id04 + 1;
}

void aoe_scenario_set_condition_target_ror(AoeScenarioInfoRor *info,
                                           int32_t index,
                                           const AoeOutcomeUnitRef *unit) {
    AoeScenarioConditionRor *c = aoe_scenario_condition_ror(info, index);
    if (c && unit) c->target_ref = unit->id04 + 1;
}

AoeOutcomeUnitRef *aoe_scenario_resolve_ror(int32_t ref,
                                            AoeScenarioUnitFn units,
                                            void *context) {
    return ref > 0 && units ? units(context, ref - 1) : NULL;
}

AoeOutcomeUnitRef *aoe_scenario_condition_unit_ror(AoeScenarioInfoRor *info,
                                                   int32_t index,
                                                   AoeScenarioUnitFn units,
                                                   void *context) {
    AoeScenarioConditionRor *c = aoe_scenario_condition_ror(info, index);
    return c ? aoe_scenario_resolve_ror(c->unit_ref, units, context) : NULL;
}

AoeOutcomeUnitRef *aoe_scenario_condition_target_ror(AoeScenarioInfoRor *info,
                                                     int32_t index,
                                                     AoeScenarioUnitFn units,
                                                     void *context) {
    AoeScenarioConditionRor *c = aoe_scenario_condition_ror(info, index);
    return c ? aoe_scenario_resolve_ror(c->target_ref, units, context) : NULL;
}

void aoe_scenario_set_allied_victory_ror(AoeScenarioInfoRor *info,
                                         int32_t slot, int32_t value) {
    if (info && slot_ok(slot)) info->allied_victory[slot] = value;
}

int32_t aoe_scenario_allied_victory_ror(const AoeScenarioInfoRor *info,
                                        int32_t slot) {
    return info && slot_ok(slot) ? info->allied_victory[slot] : 0;
}

void aoe_scenario_set_tech_ror(AoeScenarioInfoRor *info, int32_t slot,
                               int32_t tech, int32_t value) {
    if (info && slot_ok(slot) && tech >= 0 && tech < 20)
        info->tech_toggles[slot][tech] = value;
}

int32_t aoe_scenario_tech_ror(const AoeScenarioInfoRor *info, int32_t slot,
                              int32_t tech) {
    if (!info || !slot_ok(slot) || tech < 0 || tech >= 20) return 0;
    return info->tech_toggles[slot][tech];
}

void aoe_scenario_set_option_ror(AoeScenarioInfoRor *info, int32_t index,
                                 int32_t value) {
    if (info && index >= 0 && index < 3) info->options[index] = value;
}

void aoe_scenario_set_starting_age_ror(AoeScenarioInfoRor *info,
                                       int32_t slot, int32_t age) {
    if (info && slot_ok(slot)) info->starting_age[slot] = age;
}

int32_t aoe_scenario_starting_age_ror(const AoeScenarioInfoRor *info,
                                      int32_t slot) {
    return info && slot_ok(slot) ? info->starting_age[slot] : 0;
}

int aoe_scenario_apply_players_ror(const AoeScenarioInfoRor *info,
                                   const AoeScenarioApplyOps *ops) {
    static const struct { int16_t attribute; int field; } resources[4] = {
        {0, 2}, {2, 3}, {3, 0}, {1, 1}};
    if (!info || !ops || !ops->set_resource || !ops->set_stance ||
        !ops->set_allied_victory) return 0;
    for (int32_t i = 0; i + 1 < info->player_count && i < AOE_SCENARIO_PLAYERS;
         ++i) {
        int32_t player = i + 1;
        for (int r = 0; r < 4; ++r)
            ops->set_resource(ops->context, player, resources[r].attribute,
                              (float)info->start_resources[i]
                                                          [resources[r].field]);
        for (int32_t j = 1; j < info->player_count; ++j) {
            if (j == player || j - 1 >= AOE_SCENARIO_PLAYERS) continue;
            ops->set_stance(ops->context, player, j,
                            (uint8_t)info->diplomacy[i][j - 1]);
        }
        ops->set_allied_victory(ops->context, player,
                                (uint8_t)info->allied_victory[i]);
    }
    return 1;
}

/* Custom-victory goals of one player (0x00507800, 0x0050786a/0x00507912). */
static void custom_goals(const AoeScenarioInfoRor *info, AoeOutcomeManager *m,
                         int all_required) {
    uint8_t g = all_required ? 8 : 0;
    if (info->explore) aoe_outcome_add_explore_ror(m, 8, info->explore, 1);
    if (info->ruins)
        aoe_outcome_add_attribute_ror(m, g ? g : 9, 14, info->ruins, 1);
    if (info->artifacts)
        aoe_outcome_add_attribute_ror(m, g ? g : 10, 7, info->artifacts, 1);
    if (info->discoveries)
        aoe_outcome_add_attribute_ror(m, g ? g : 11, 13, info->discoveries, 0);
    if (info->gold) aoe_outcome_add_attribute_ror(m, g ? g : 12, 3, info->gold, 1);
}

static void install_record(const AoeScenarioConditionRor *r,
                           AoeOutcomeManager *m,
                           const AoeScenarioInstallOps *ops) {
    static const int32_t resource_attributes[6] = {3, 1, 2, 0, 11, 6};
    AoeOutcomeUnitRef *unit, *target;
    switch (r->type) {
    case 4:
        if (r->subtype == 6)
            aoe_outcome_add_explore_ror(m, 1, r->amount, 1);
        else if (r->subtype >= 0 && r->subtype <= 5)
            aoe_outcome_add_attribute_ror(
                m, 1, resource_attributes[r->subtype], r->amount, 0);
        break;
    case 6:
        aoe_outcome_add_attribute_ror(m, 1, r->subtype, r->amount, 0);
        break;
    case 5:
        unit = aoe_scenario_resolve_ror(r->unit_ref, ops->units, ops->context);
        aoe_outcome_add_destroy_unit_ror(m, 1, unit, 1);
        break;
    case 3:
        target = aoe_scenario_resolve_ror(r->target_ref, ops->units,
                                          ops->context);
        unit = aoe_scenario_resolve_ror(r->unit_ref, ops->units, ops->context);
        if (target)
            aoe_outcome_add_bring_to_unit_ror(m, 1, unit, target);
        else
            aoe_outcome_add_bring_to_area_ror(m, 1, unit, r->area[0],
                                              r->area[1], r->area[2],
                                              r->area[3]);
        break;
    case 2:
        if (r->area[2] <= 0.0f)
            aoe_outcome_add_create_ror(m, 1, r->object_class, r->amount, 0);
        else
            aoe_outcome_add_create_in_area_ror(m, 1, r->object_class,
                                               r->amount, r->area[0],
                                               r->area[1], r->area[2],
                                               r->area[3], 0);
        break;
    case 7:
        aoe_outcome_add_kind100_ror(m, 1, r->subtype);
        break;
    case 1:
        if (r->object_class && r->all_flag) {
            aoe_outcome_add_destroy_all_ror(m, 1, r->object_class, NULL);
        } else if (r->player) {
            AoeOutcomePlayerRef *p = ops->player(ops->context, r->player);
            if (r->object_class)
                aoe_outcome_add_destroy_count_ror(m, 1, r->object_class,
                                                  r->amount, p);
            else
                aoe_outcome_add_defeat_player_ror(m, 1, p);
        } else if ((unit = aoe_scenario_resolve_ror(r->unit_ref, ops->units,
                                                    ops->context))) {
            aoe_outcome_add_capture_ror(m, 1, unit);
        }
        break;
    default:
        break;
    }
}

int aoe_scenario_install_ror(const AoeScenarioInfoRor *info, int apply,
                             const AoeScenarioInstallOps *ops) {
    if (!info || !ops || !ops->player || !ops->manager || !ops->units)
        return 0;
    if (apply && !aoe_scenario_apply_players_ror(info, &ops->apply)) return 0;
    for (int32_t p = 1; p < info->player_count; ++p)
        aoe_outcome_conditions_clear_ror(ops->manager(ops->context, p));
    if (info->victory_type == 4) {
        for (int32_t p = 1; p < info->player_count; ++p)
            custom_goals(info, ops->manager(ops->context, p),
                         info->all_custom_required != 0);
    }
    for (int32_t s = 0; s + 1 < info->player_count && s < AOE_SCENARIO_PLAYERS;
         ++s) {
        AoeOutcomeManager *m = ops->manager(ops->context, s + 1);
        for (int i = 0; i < AOE_SCENARIO_CONDITIONS; ++i)
            if (info->conditions[s][i].type)
                install_record(&info->conditions[s][i], m, ops);
    }
    return 1;
}

static int32_t form_atoi(const char *text) { return text ? atoi(text) : 0; }

static void store_required(AoeScenarioInfoRor *info,
                           const AoeScenarioGoalsFormRor *g) {
    aoe_scenario_set_all_custom_required_ror(info, g->all_required != 0);
    aoe_scenario_set_goal_ror(info, AOE_SCENARIO_GOAL_CONQUEST,
                              !(g->conquest_off == 0 && g->conquest_alt));
}

void aoe_scenario_store_goals_ror(AoeScenarioInfoRor *info,
                                  const AoeScenarioGoalsFormRor *g) {
    static const int goals[5] = {
        AOE_SCENARIO_GOAL_EXPLORE, AOE_SCENARIO_GOAL_RUINS,
        AOE_SCENARIO_GOAL_ARTIFACTS, AOE_SCENARIO_GOAL_DISCOVERIES,
        AOE_SCENARIO_GOAL_GOLD};
    int32_t type = 4;
    if (!info || !g) return;
    if (g->tool_victory >= 1 && g->tool_victory <= 4)
        type = g->tool_victory - 1;
    aoe_scenario_set_victory_type_ror(info, type);
    for (type = 0; type < 4 && !g->radio[type]; type++) {
    }
    aoe_scenario_set_victory_type_ror(info, type);
    aoe_scenario_set_score_ror(info, g->score);
    aoe_scenario_set_time_limit_ror(info, g->time_limit);
    store_required(info, g);
    for (int i = 0; i < 5; i++) {
        int32_t v = form_atoi(g->goal_text[i]);
        if (!g->goal_check[i]) v = 0;
        else if (i == 0) v = v < 1 ? 1 : v > 100 ? 100 : v;
        aoe_scenario_set_goal_ror(info, goals[i], v);
    }
}

void aoe_scenario_store_condition_ror(AoeScenarioInfoRor *info,
                                      int32_t index,
                                      const AoeScenarioGoalsFormRor *goals,
                                      const AoeScenarioConditionFormRor *f,
                                      float area_out[4]) {
    static const int32_t sub[5] = {0, 3, 1, 2, 4};
    AoeScenarioConditionRor *c;
    float a[4];
    int32_t amount, player;
    if (!info || !goals) return;
    store_required(info, goals);
    if (index < 0 || !f) return;
    amount = form_atoi(f->amount_text);
    player = f->player_line + 1;
    for (int i = 0; i < 4; i++) a[i] = (float)f->area[i];
    if (area_out) memcpy(area_out, a, sizeof a);
    aoe_scenario_clear_condition_ror(info, index);
    c = aoe_scenario_condition_ror(info, index);
    if (!c) return;
    switch (f->kind) {
    case 1:
        if (!f->unit || !f->target) return;
        c->type = 3;
        aoe_scenario_set_condition_unit_ror(info, index, f->unit);
        aoe_scenario_set_condition_target_ror(info, index, f->target);
        return;
    case 2:
        if (!f->unit) return;
        c->type = 3;
        aoe_scenario_set_condition_unit_ror(info, index, f->unit);
        aoe_scenario_set_condition_area_ror(info, index, a[0], a[1], a[2],
                                            a[3]);
        return;
    case 3:
    case 4:
        c->type = 2;
        c->object_class = f->object_class;
        c->amount = amount;
        if (f->kind == 4)
            aoe_scenario_set_condition_area_ror(info, index, a[0], a[1],
                                                a[2], a[3]);
        return;
    case 5:
        if (player < 1 || player > 8 || !amount) return;
        c->type = 1;
        c->player = player;
        c->object_class = f->object_class;
        c->amount = amount;
        return;
    case 6:
    case 9:
        if (!f->unit) return;
        c->type = f->kind == 6 ? 1 : 5;
        aoe_scenario_set_condition_unit_ror(info, index, f->unit);
        return;
    case 7:
    case 8:
        if (player < 1 || player > 8) return;
        c->type = 1;
        c->player = player;
        if (f->kind == 7) {
            c->object_class = f->object_class;
            c->all_flag = 1;
        }
        return;
    case 10: case 11: case 12: case 13: case 14:
        c->type = 4;
        c->subtype = sub[f->kind - 10];
        c->amount = amount;
        return;
    case 15:
        c->type = 4;
        c->subtype = 5;
        c->amount = f->tech_line + 1;
        return;
    case 16:
        c->type = 4;
        c->subtype = 6;
        c->amount = amount;
        return;
    case 17:
        c->type = 6;
        c->subtype = f->subtype_id;
        c->amount = amount;
        return;
    case 18:
        c->type = 7;
        c->subtype = f->relic_id;
        return;
    default:
        return;
    }
}

int32_t aoe_scenario_condition_kind_ror(const AoeScenarioConditionRor *c) {
    static const int32_t by_sub[7] = {10, 12, 13, 11, 14, 15, 16};
    if (!c) return -1;
    switch (c->type) {
    case 0: return 0;
    case 1:
        if (c->amount) return 5;
        if (c->unit_ref) return 6;
        return c->all_flag ? 7 : 8;
    case 2: return c->area[2] <= 0.0f ? 3 : 4;
    case 3: return c->target_ref ? 1 : 2;
    case 4:
        return c->subtype >= 0 && c->subtype <= 6 ? by_sub[c->subtype] : -1;
    case 5: return c->unit_ref ? 9 : -1;
    case 6: return 17;
    case 7: return 18;
    default: return -1;
    }
}

int32_t aoe_scenario_victory_tool_ror(const AoeScenarioInfoRor *info) {
    int32_t type = aoe_scenario_victory_type_ror(info);
    return type >= 0 && type <= 3 ? type + 1 : 5;
}

void aoe_scenario_store_diplomacy_page_ror(AoeScenarioInfoRor *info,
                                           int32_t slot,
                                           const int32_t ally[8],
                                           const int32_t neutral[8],
                                           const int32_t enemy[8],
                                           int32_t allied_victory) {
    if (!info || slot < 0) return;
    for (int32_t t = 0; t < 8; t++) {
        if (ally[t] == 1)
            aoe_scenario_set_diplomacy_ror(info, slot, t, 0);
        else if (neutral[t] == 1)
            aoe_scenario_set_diplomacy_ror(info, slot, t, 1);
        else if (enemy[t] == 1)
            aoe_scenario_set_diplomacy_ror(info, slot, t, 3);
    }
    aoe_scenario_set_allied_victory_ror(info, slot, allied_victory);
}

int aoe_scenario_diplomacy_page_ror(const AoeScenarioInfoRor *info,
                                    int32_t slot, int32_t radio[8],
                                    int32_t allied_buttons[8]) {
    if (!info || slot < 0) return 0;
    for (int32_t t = 0; t < 8; t++) {
        int32_t s = aoe_scenario_diplomacy_ror(info, slot, t);
        radio[t] = s == 0 ? 0 : s == 1 ? 1 : s == 3 ? 2 : -1;
        allied_buttons[t] = 0;
    }
    if (slot < 8)
        allied_buttons[slot] = aoe_scenario_allied_victory_ror(info, slot);
    return 1;
}

void aoe_scenario_apply_techs_ror(const AoeScenarioInfoRor *info,
                                  const AoeScenarioTechOpsRor *ops) {
    static const int16_t techs[16] = {0x58, 0x59, 0x5a, 0x5b, 0x5e, 0x5f, 0x61, 0x62,
                                      0x5d, 0x60, 0x5c, 0x65, 0x66, 0x67, 0x73, 0x74};
    int32_t p;
    int k;
    if (!ops->has_world) return;
    for (p = 1; p < info->player_count && p <= AOE_SCENARIO_PLAYERS; ++p) {
        if (info->options[0]) ops->apply_effect(ops->context, p, 100);
        for (k = 0; k < 16; ++k)
            ops->set_tech(ops->context, p, techs[k], (uint8_t)info->tech_toggles[p - 1][k]);
    }
}

static void save_i32(const AoeScenarioSaveOpsRor *ops, int32_t v) {
    uint8_t b[4];
    uint32_t u = (uint32_t)v;
    b[0] = (uint8_t)u;
    b[1] = (uint8_t)(u >> 8);
    b[2] = (uint8_t)(u >> 16);
    b[3] = (uint8_t)(u >> 24);
    ops->write(ops->context, b, 4);
}

static void save_f32(const AoeScenarioSaveOpsRor *ops, float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    save_i32(ops, (int32_t)u);
}

static void save_i32s(const AoeScenarioSaveOpsRor *ops, const int32_t *v, int n) {
    int i;
    for (i = 0; i < n; ++i) save_i32(ops, v[i]);
}

void aoe_scenario_info_save_ror(const AoeScenarioInfoRor *info,
                                const AoeScenarioTechOpsRor *techs,
                                const AoeScenarioSaveOpsRor *ops) {
    int p, c, i;
    if (techs) aoe_scenario_apply_techs_ror(info, techs);
    if (ops->base_save) ops->base_save(ops->context);
    save_i32s(ops, &info->start_resources[0][0], AOE_SCENARIO_PLAYERS * 4);
    save_i32(ops, -99);
    save_i32(ops, info->custom_conquest);
    save_i32(ops, info->ruins);
    save_i32(ops, info->artifacts);
    save_i32(ops, info->discoveries);
    save_i32(ops, info->explore);
    save_i32(ops, info->gold);
    save_i32(ops, info->all_custom_required);
    save_i32(ops, info->victory_type);
    save_i32(ops, info->score);
    save_i32(ops, info->time_limit);
    save_i32s(ops, &info->diplomacy[0][0], AOE_SCENARIO_PLAYERS * AOE_SCENARIO_PLAYERS);
    for (p = 0; p < AOE_SCENARIO_PLAYERS; ++p)
        for (c = 0; c < AOE_SCENARIO_CONDITIONS; ++c) {
            const AoeScenarioConditionRor *r = &info->conditions[p][c];
            save_i32(ops, r->object_class);
            save_i32(ops, r->all_flag);
            save_i32(ops, r->player);
            save_i32(ops, r->field_0c);
            for (i = 0; i < 4; ++i) save_f32(ops, r->area[i]);
            save_i32(ops, r->type);
            save_i32(ops, r->amount);
            save_i32(ops, r->subtype);
            save_i32(ops, r->unit_ref);
            save_i32(ops, r->target_ref);
            save_i32s(ops, r->field_34, 2);
        }
    save_i32(ops, -99);
    save_i32s(ops, info->allied_victory, AOE_SCENARIO_PLAYERS);
    save_i32s(ops, &info->tech_toggles[0][0], AOE_SCENARIO_PLAYERS * 20);
    save_i32s(ops, info->options, 3);
    save_i32s(ops, info->starting_age, AOE_SCENARIO_PLAYERS);
    save_i32(ops, -99);
}
