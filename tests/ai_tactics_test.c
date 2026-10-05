/* Patched RoR 1.0a tactical AI pieces 0x0040d150..0x0040ede0. */
#include "ai_tactics.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int unit(void *c, int32_t id, uint8_t *state, float *hp) {
    (void)c;
    if (id == 3) return 0;
    *state = id == 4 ? 3 : 2;
    *hp = (float)id + 0.9f;
    return 1;
}

int main(void) {
    static AoeAiGroupRor group;
    static AoeAiMembersRor m, m2;
    AoeAiCountersRor c;
    AoeAiPlayListRor plays;
    AoeAiTradeRor trade;
    AoeAiModuleRor strat;
    int32_t ids[50];
    uint8_t buffer[1024];
    AoeOutcomeStream out = {buffer, sizeof(buffer), 0, 0}, in;

    aoe_ai_group_init_ror(&group);
    aoe_ai_action_set_ror(&group.rules[2].actions[3], 1, 4, 0, 0, 0);
    assert(aoe_ai_group_find_action_ror(&group, 1, 4, 2) == 3);
    assert(aoe_ai_group_find_action_ror(&group, 1, 4, 10) == -1);
    assert(aoe_ai_group_find_action_ror(&group, 5, 4, 2) == -1);
    assert(aoe_ai_attack_kind_ror("Annoy") == 2 &&
           aoe_ai_attack_kind_ror("Recon") == 4 &&
           aoe_ai_attack_kind_ror("x") == 0);
    assert(aoe_ai_speed_kind_ror("Stationary") == 1 &&
           aoe_ai_speed_kind_ror("Any") == 5 && aoe_ai_any_kind_ror("Fast") == 0);

    for (int i = 0; i < AOE_AI_MEMBERS; ++i) m.slots[i].id = -1;
    assert(aoe_ai_members_add_ror(&m, 3, 1, 0) && aoe_ai_members_add_ror(&m, 4, 1, 2) &&
           aoe_ai_members_add_ror(&m, 7, 1, 2) && aoe_ai_members_add_ror(&m, 9, 0, 2));
    assert(aoe_ai_members_count_ror(&m) == 4 &&
           aoe_ai_members_count_category_ror(&m, 1) == 3 &&
           aoe_ai_members_count_role_ror(&m, 1, 2) == 2 &&
           aoe_ai_members_category_ror(&m, 9) == 0 &&
           aoe_ai_members_category_ror(&m, 8) == 0xff);
    m.slots[1].pad[0] = 0xaa;
    m.totals[4] = 12;
    m.field_1b8 = 5;
    assert(aoe_ai_members_save_ror(&m, &out) && out.pos == 20 + 20 + 1 + 400 + 8);
    in = (AoeOutcomeStream){buffer, out.pos, 0, 0};
    assert(aoe_ai_members_load_ror(&m2, &in) && !memcmp(&m2.slots, &m.slots, sizeof(m.slots)) &&
           m2.totals[4] == 12 && m2.field_1b8 == 5);
    aoe_ai_members_purge_ror(&m, unit, NULL);
    assert(aoe_ai_members_count_ror(&m) == 2 && m.slots[0].category == 0xff);
    assert(aoe_ai_members_ids_ror(&m, ids) == 2 && ids[0] == 7 && ids[1] == 9);
    aoe_ai_members_sum_hp_ror(&m, 1, unit, NULL);
    assert(aoe_ai_members_total_ror(&m, 1) == 7 &&
           aoe_ai_members_total_ror(&m, 5) == -1);
    for (int i = 0; i < AOE_AI_MEMBERS; ++i) aoe_ai_members_add_ror(&m, 100 + i, 0, 0);
    assert(!aoe_ai_members_add_ror(&m, 1, 0, 0));

    aoe_ai_counters_init_ror(&c, 3);
    assert(c.values[2] == 0 && c.values[3] == -1 && c.second[0] == -1);
    aoe_ai_counters_add_ror(&c, 1, 5);
    aoe_ai_counters_set_ror(&c, 3, 9); /* ignored */
    assert(aoe_ai_counters_get_ror(&c, 1) == 5 && aoe_ai_counters_get_ror(&c, 3) == -1 &&
           aoe_ai_counters_count_ror(&c) == 3);
    aoe_ai_counters_fill_ror(&c, 2);
    assert(c.values[7] == 2);

    aoe_ai_play_list_init_ror(&plays);
    assert(!aoe_ai_play_list_get_ror(&plays, 0));
    plays.plays = calloc(2, sizeof(AoeAiPlayRor));
    plays.count = 2;
    assert(aoe_ai_play_list_get_ror(&plays, 1) == &plays.plays[1] &&
           !aoe_ai_play_list_get_ror(&plays, 2));
    aoe_ai_play_list_release_ror(&plays);
    assert(!plays.plays && !plays.count);

    aoe_ai_strategy_init_ror(&strat, 2, NULL);
    assert(strat.id == 0x3f0 && !strcmp(strat.name, "Strategy AI"));
    aoe_ai_strategy_release_ror(&strat);
    aoe_ai_tac_init_ror(&strat, 2, NULL);
    assert(strat.id == 0x3f1 && !strcmp(strat.name, "TacAI"));
    aoe_ai_tac_release_ror(&strat);
    aoe_ai_trade_load_ror(&trade, 4, NULL);
    assert(trade.module.id == 0x3f2 && trade.module.player == 4 && !trade.field_f0);
    aoe_ai_trade_release_ror(&trade);
    assert(aoe_ai_unit_default_c0_ror() == -1);
    return 0;
}
