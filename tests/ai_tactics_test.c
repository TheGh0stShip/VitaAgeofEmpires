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

/* Units 100..: category = id % 10 (8 and 9 refused). */
static int32_t fill_category(void *c, int32_t id) {
    (void)c;
    return id % 10 >= 6 ? -1 : id % 10;
}

static void fill_tests(void) {
    static AoeAiGroupRor play;
    static AoeAiMembersRor m;
    const int32_t ids[] = {100, 110, 120, 101, 111, 102};
    const int32_t bad[] = {100, 108};
    aoe_ai_group_init_ror(&play);
    aoe_ai_members_init_ror(&m);
    assert(m.field_190 == -1 && m.field_1b8 == 0xfd && m.slots[49].id == -1 &&
           m.slots[0].role == 0xff && m.totals[4] == -1 && m.shortfall == 0);
    /* Two ranges: range 0 wants at least 2 of category 0 (at most 3),
       range 1 at least 1 of category 1 and 2 of category 2 (max 2 each). */
    play.range_count = 2;
    play.field_42 = 50;
    aoe_ai_range_set_min_ror(&play.ranges[0], 0, 2);
    aoe_ai_range_set_max_ror(&play.ranges[0], 0, 3);
    aoe_ai_range_set_max_ror(&play.ranges[0], 1, 0);
    aoe_ai_range_set_max_ror(&play.ranges[0], 2, 0);
    aoe_ai_range_set_min_ror(&play.ranges[1], 1, 1);
    aoe_ai_range_set_min_ror(&play.ranges[1], 2, 2);
    aoe_ai_range_set_max_ror(&play.ranges[1], 0, 0);
    aoe_ai_range_set_max_ror(&play.ranges[1], 1, 2);
    aoe_ai_range_set_max_ror(&play.ranges[1], 2, 2);
    assert(aoe_ai_members_fill_ror(&m, 1, &play, ids, 6, fill_category, NULL) == 1);
    /* 100, 110 (minimum) and 120 (room) in range 0; 101, 111 and 102 in
       range 1; one category-2 member short. */
    assert(aoe_ai_members_count_role_ror(&m, 0, 0) == 3 &&
           aoe_ai_members_count_role_ror(&m, 1, 1) == 2 &&
           aoe_ai_members_count_role_ror(&m, 1, 2) == 1 && m.shortfall == 1);
    /* A refused unit clears the result; a size of 1 stops at once. */
    aoe_ai_members_init_ror(&m);
    play.field_42 = 1;
    assert(aoe_ai_members_fill_ror(&m, 2, &play, bad, 2, fill_category, NULL) == 0);
    assert(aoe_ai_members_count_ror(&m) == 1 && m.shortfall == 4);
    /* No range 6: 0. */
    aoe_ai_members_init_ror(&m);
    play.range_count = 7;
    play.field_42 = 50;
    assert(aoe_ai_members_fill_ror(&m, 2, &play, ids, 6, fill_category, NULL) == 0);
    assert(aoe_ai_members_fill_ror(&m, 2, NULL, ids, 6, fill_category, NULL) == 0);
}

static void point_tests(void) {
    AoeAiPointRor p, o, out;
    aoe_ai_point_zero_ror(&p);
    assert(p.x == 0 && p.y == 0 && p.z == 0);
    aoe_ai_point_set_ror(&p, 12, 10, 3);
    aoe_ai_point_set_ror(&o, 10, 10, 0);
    aoe_ai_point_rotate_ror(&p, &o, &out, 0);
    assert(out.x == 12 && out.y == 10 && out.z == 3);
    /* A quarter turn (two eighths): the offset (2, 0) becomes (0, 2),
       without the origin added back. */
    aoe_ai_point_rotate_ror(&p, &o, &out, 2);
    assert(out.x == 0 && out.y == 2 && out.z == 3);
}

int main(void) {
    point_tests();
    static AoeAiGroupRor group;
    static AoeAiMembersRor m, m2;
    AoeAiCountersRor c;
    AoeAiPlayListRor plays;
    AoeAiTradeRor trade;

    fill_tests();
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
