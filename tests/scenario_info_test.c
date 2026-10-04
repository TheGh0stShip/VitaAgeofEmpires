/* Patched RoR 1.0a scenario-info accessors 0x00506e40..0x00507cc0, the
   player apply 0x005076c0 and the condition installer 0x00507800. */
#include "scenario_info.h"

#include <assert.h>
#include <string.h>

typedef struct {
    float resources[4][4];
    uint8_t stances[4][4];
    uint8_t allied[4];
    AoeOutcomePlayerRef players[4];
    AoeOutcomeManager managers[4];
    AoeOutcomeUnitRef units[3];
} World;

static void set_resource(void *c, int32_t p, int16_t a, float v) {
    ((World *)c)->resources[p][a] = v;
}
static void set_stance(void *c, int32_t p, int32_t o, uint8_t s) {
    ((World *)c)->stances[p][o] = s;
}
static void set_allied(void *c, int32_t p, uint8_t v) {
    ((World *)c)->allied[p] = v;
}
static AoeOutcomePlayerRef *player(void *c, int32_t p) {
    return &((World *)c)->players[p];
}
static AoeOutcomeManager *manager(void *c, int32_t p) {
    return &((World *)c)->managers[p];
}
static AoeOutcomeUnitRef *unit(void *c, int32_t id) {
    World *w = c;
    for (int i = 0; i < 3; ++i)
        if (w->units[i].id04 == id) return &w->units[i];
    return NULL;
}

static int count_kind(const AoeOutcomeManager *m, int kind, int group) {
    int n = 0;
    for (const AoeOutcomeCondition *c = m->conditions; c; c = c->next)
        n += c->kind == kind && c->group == group;
    return n;
}

int main(void) {
    static AoeScenarioInfoRor info;
    static World w;
    AoeScenarioInstallOps ops = {
        &w, {&w, set_resource, set_stance, set_allied}, player, manager,
        unit};
    float x0, y0, x1, y1;

    /* Initialisers and clamps. */
    aoe_scenario_enable_all_techs_ror(&info);
    aoe_scenario_reset_diplomacy_ror(&info);
    aoe_scenario_reset_goals_ror(&info);
    assert(info.tech_toggles[15][19] == 1 && info.diplomacy[15][15] == 3);
    assert(aoe_scenario_goal_ror(&info, AOE_SCENARIO_GOAL_CONQUEST) == 1);
    aoe_scenario_set_all_custom_required_ror(&info, 7);
    assert(aoe_scenario_all_custom_required_ror(&info) == 1);
    aoe_scenario_set_all_custom_required_ror(&info, -2);
    assert(info.all_custom_required == 0);
    aoe_scenario_select_player_ror(&info, 40);
    assert(info.selected == 15);
    aoe_scenario_select_player_ror(&info, 16);
    assert(info.selected == 16 && !aoe_scenario_condition_ror(&info, 0));
    aoe_scenario_select_player_ror(&info, -3);
    assert(info.selected == 0);

    /* Victory type: anything but custom sets conquest. */
    info.conquest = 0;
    aoe_scenario_set_victory_type_ror(&info, 4);
    assert(info.conquest == 0);
    aoe_scenario_set_victory_type_ror(&info, 2);
    assert(info.conquest == 1 && aoe_scenario_victory_type_ror(&info) == 2);
    aoe_scenario_set_goal_ror(&info, AOE_SCENARIO_GOAL_CONQUEST, 0);
    assert(info.conquest == 0 && info.custom_conquest == 0);

    /* Diplomacy stances 0, 1, 3 only. */
    aoe_scenario_set_diplomacy_ror(&info, 0, 1, 2);
    assert(aoe_scenario_diplomacy_ror(&info, 0, 1) == 3);
    aoe_scenario_set_diplomacy_ror(&info, 0, 1, 0);
    assert(aoe_scenario_diplomacy_ror(&info, 0, 1) == 0);
    assert(aoe_scenario_diplomacy_ror(&info, 16, 0) == -1);

    /* Records follow the selected slot. */
    aoe_scenario_select_player_ror(&info, 1);
    aoe_scenario_set_condition_area_ror(&info, 3, 1, 2, 3, 4);
    assert(aoe_scenario_condition_area_ror(&info, 3, &x0, &y0, &x1, &y1));
    assert(x0 == 1 && y0 == 2 && x1 == 3 && y1 == 4);
    assert(info.conditions[1][3].area[2] == 3);
    aoe_scenario_clear_condition_ror(&info, 3);
    assert(info.conditions[1][3].area[2] == 0);
    w.units[0] = (AoeOutcomeUnitRef){.id04 = 0, .owner0c = &w.players[2]};
    w.units[1] = (AoeOutcomeUnitRef){.id04 = 9, .owner0c = &w.players[2]};
    w.units[2] = (AoeOutcomeUnitRef){.id04 = 4, .owner0c = &w.players[3]};
    aoe_scenario_set_condition_unit_ror(&info, 0, &w.units[0]);
    assert(info.conditions[1][0].unit_ref == 1);
    aoe_scenario_set_condition_unit_ror(&info, 0, NULL);
    assert(info.conditions[1][0].unit_ref == 1);
    assert(aoe_scenario_condition_unit_ror(&info, 0, unit, &w) == &w.units[0]);
    assert(!aoe_scenario_condition_target_ror(&info, 0, unit, &w));
    assert(!aoe_scenario_resolve_ror(0, unit, &w));

    /* Apply to three players (gaia + 2 ... player_count 4). */
    info.player_count = 4;
    aoe_scenario_set_start_resource_ror(&info, 0, 0, 100); /* gold */
    aoe_scenario_set_start_resource_ror(&info, 0, 1, 200); /* wood */
    aoe_scenario_set_start_resource_ror(&info, 0, 2, 300); /* food */
    aoe_scenario_set_start_resource_ror(&info, 0, 3, 150); /* stone */
    info.diplomacy[0][1] = 0;  /* player 1 toward player 2 */
    info.diplomacy[0][2] = 1;  /* toward 3 */
    info.diplomacy[0][0] = 9;  /* itself, skipped */
    aoe_scenario_set_allied_victory_ror(&info, 0, 1);
    assert(aoe_scenario_apply_players_ror(&info, &ops.apply));
    assert(w.resources[1][0] == 300 && w.resources[1][1] == 200 &&
           w.resources[1][2] == 150 && w.resources[1][3] == 100);
    assert(w.stances[1][2] == 0 && w.stances[1][3] == 1 &&
           w.stances[1][1] == 0 && w.allied[1] == 1);
    assert(w.stances[3][1] == 3 && w.stances[3][3] == 0);

    /* Custom victory, separate groups, plus per-slot records. */
    info.victory_type = 4;
    info.explore = 60;
    info.ruins = 5;
    info.discoveries = 2;
    info.conditions[0][0] = (AoeScenarioConditionRor){.type = 4,
                                                      .subtype = 0,
                                                      .amount = 500};
    info.conditions[0][1] = (AoeScenarioConditionRor){.type = 1, .player = 3};
    info.conditions[0][2] = (AoeScenarioConditionRor){.type = 3,
                                                      .unit_ref = 10,
                                                      .target_ref = 5};
    info.conditions[0][3] = (AoeScenarioConditionRor){
        .type = 2, .object_class = 83, .amount = 4,
        .area = {1, 1, 0, 9}};
    info.conditions[1][5] = (AoeScenarioConditionRor){.type = 5,
                                                      .unit_ref = 1};
    info.conditions[2][0] = (AoeScenarioConditionRor){.type = 1,
                                                      .object_class = 7,
                                                      .all_flag = 1};
    aoe_outcome_add_score_ror(&w.managers[1], 3, 50, 0); /* cleared */
    assert(aoe_scenario_install_ror(&info, 0, &ops));
    assert(count_kind(&w.managers[1], 12, 3) == 0);
    for (int p = 1; p < 4; ++p) {
        assert(count_kind(&w.managers[p], 7, 8) == 1);
        assert(count_kind(&w.managers[p], 6, 9) == 1);
        assert(count_kind(&w.managers[p], 6, 11) == 1);
        assert(count_kind(&w.managers[p], 6, 10) == 0);
    }
    assert(w.managers[1].condition_count == 3 + 4);
    assert(count_kind(&w.managers[1], 6, 1) == 1);
    assert(count_kind(&w.managers[1], 10, 1) == 1);
    assert(count_kind(&w.managers[1], 5, 1) == 1);
    assert(count_kind(&w.managers[1], 1, 1) == 1); /* x1 0: no area */
    for (AoeOutcomeCondition *c = w.managers[1].conditions; c; c = c->next) {
        if (c->kind == 6 && c->group == 1)
            assert(c->resource_index_1c == 3 && c->resource_target_20 == 500);
        if (c->kind == 10) assert(c->player_08 == &w.players[3]);
        if (c->kind == 5)
            assert(c->unit_24 == &w.units[1] && c->unit_28 == &w.units[2] &&
                   c->player_08 == &w.players[3]);
        if (c->kind == 6 && c->group == 11) assert(!c->include_allies_2d);
    }
    assert(count_kind(&w.managers[2], 0, 1) == 1);
    assert(count_kind(&w.managers[3], 9, 1) == 1);

    /* All required: every custom goal in group 8. */
    aoe_scenario_set_all_custom_required_ror(&info, 1);
    assert(aoe_scenario_install_ror(&info, 1, &ops));
    assert(count_kind(&w.managers[2], 6, 8) == 2 &&
           count_kind(&w.managers[2], 7, 8) == 1);
    for (int p = 0; p < 4; ++p) aoe_outcome_manager_release_ror(&w.managers[p]);
    return 0;
}
