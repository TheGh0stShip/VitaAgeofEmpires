/* Patched RoR 1.0a tactical AI state, numbers and unit groups. */
#include "ai_tactical.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "aoe_stream.h"

static int32_t info_value = -7;
static void info_set(void *c, int32_t v) { (void)c; info_value = v; }
static int32_t roll(void *c) { (void)c; return 17; }

int main(void) {
    static AoeAiTacticalRor ai, back;
    static uint8_t buf[1 << 16];
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;
    AoeAiUnitGroupRor *g1, *g2, *g3;
    AoeAiStrategyRor strategy;
    size_t full;

    aoe_ai_tactical_init_ror(&ai, 3, NULL);
    assert(ai.module.id == 0x3f1 && !strcmp(ai.module.name, "TacAI"));
    assert(ai.sn[0] == -1 && ai.sn[225] == -1 && ai.targets[49][4] == -1 &&
           ai.targets[49][5] == 0 && ai.field_d88[3] == -1 &&
           ai.field_f84 == -1 && ai.field_1000 == -1 && ai.counters.count == 4);
    assert(!strcmp(aoe_ai_tactical_state_name_ror(&ai), "Setup SolGroups"));
    ai.state = 21;
    assert(!strcmp(aoe_ai_tactical_state_name_ror(&ai), "BldLst Insertions"));
    ai.state = 22;
    assert(!strcmp(aoe_ai_tactical_state_name_ror(&ai), "Invalid"));
    ai.state = 9;

    /* Numbers: clamps and the information-AI hook. */
    aoe_ai_tactical_set_sn_ror(&ai, 117, 250, info_set, NULL);
    aoe_ai_tactical_set_sn_ror(&ai, 159, -3, info_set, NULL);
    aoe_ai_tactical_set_sn_ror(&ai, 121, 250, info_set, NULL);
    aoe_ai_tactical_set_sn_ror(&ai, 226, 5, info_set, NULL);
    assert(ai.sn[117] == 100 && ai.sn[159] == 0 && ai.sn[121] == 250);
    assert(info_value == -7);
    aoe_ai_tactical_set_sn_ror(&ai, 105, 12, info_set, NULL);
    assert(info_value == 12 && aoe_ai_tactical_sn_ror(&ai, 105) == 12 &&
           aoe_ai_tactical_sn_ror(&ai, 226) == -1);

    /* +0xd24 helpers. */
    ai.sn[46] = 10;
    ai.sn[102] = 0;
    aoe_ai_tactical_roll_d24_ror(&ai, roll, NULL);
    assert(ai.field_d24 == 10);
    ai.sn[102] = 6; /* 17 % 6 - 3 + 10 */
    aoe_ai_tactical_roll_d24_ror(&ai, roll, NULL);
    assert(ai.field_d24 == 12);
    ai.sn[46] = -20;
    aoe_ai_tactical_roll_d24_ror(&ai, roll, NULL);
    assert(ai.field_d24 == 0);
    ai.field_d24 = 12;
    ai.sn[48] = 9;
    ai.sn[71] = 0;
    assert(aoe_ai_tactical_d24_limit_ror(&ai, 0x2e) == 12 &&
           aoe_ai_tactical_d24_limit_ror(&ai, 1) == 9);
    ai.sn[71] = 1;
    assert(aoe_ai_tactical_d24_limit_ror(&ai, 1) == 9);
    ai.sn[48] = 15;
    assert(aoe_ai_tactical_d24_limit_ror(&ai, 1) == 12);

    /* Groups. */
    g1 = aoe_ai_tactical_new_group_ror(&ai, 1);
    g2 = aoe_ai_tactical_new_group_ror(&ai, 1);
    g3 = aoe_ai_tactical_new_group_ror(&ai, 1);
    assert(g1->id == 0 && g2->id == 1 && g3->id == 2 &&
           ai.next_group_id == 3 && ai.group_count == 3 && ai.groups_len == 3);
    g1->type = 0x66;
    g2->type = 0x66;
    g2->field_180 = 4;
    g3->type = 0x6d;
    g3->field_184 = 500;
    aoe_ai_unit_group_add_ror(g1, 70, NULL);
    aoe_ai_unit_group_add_ror(g2, 71, NULL);
    aoe_ai_unit_group_add_ror(g2, 72, NULL);
    assert(aoe_ai_tactical_find_group_ror(&ai, -1, 0x66, -1, -1) == g1);
    assert(aoe_ai_tactical_find_group_ror(&ai, -1, 0x66, 4, -1) == g2);
    assert(!aoe_ai_tactical_find_group_ror(&ai, 9, -1, -1, -1));
    assert(aoe_ai_tactical_count_groups_ror(&ai, 0x66, -1) == 2 &&
           aoe_ai_tactical_count_groups_ror(&ai, -1, 4) == 1);
    assert(aoe_ai_tactical_grouped_66_ror(&ai) == 3 &&
           aoe_ai_tactical_grouped_units_ror(&ai, -1) == 3);
    assert(aoe_ai_tactical_group_of_ror(&ai, 72) == g2 &&
           !aoe_ai_tactical_group_of_ror(&ai, 73));

    /* Victory targets that no group already holds. */
    aoe_ai_strategy_ai_init_ror(&strategy, 3, NULL);
    strategy.victory_kind = 0;
    strategy.target_id = 500;
    strategy.second_id = 501;
    assert(aoe_ai_tactical_free_target_ror(&ai, &strategy) == -1);
    strategy.target_id = 502;
    assert(aoe_ai_tactical_free_target_ror(&ai, &strategy) == 502);
    assert(aoe_ai_tactical_free_second_ror(&ai, &strategy) == -1);
    strategy.victory_kind = 4;
    assert(aoe_ai_tactical_free_second_ror(&ai, &strategy) == 501);
    aoe_ai_strategy_ai_release_ror(&strategy);

    /* Getters. */
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 5);
    aoe_ai_int_list_add_unique_ror(&ai.lists[4], 6);
    aoe_ai_int_list_add_unique_ror(&ai.lists[4], 7);
    ai.lists[1].field_8 = 33;
    ai.field_9cc = 44;
    assert(aoe_ai_tactical_get_ror(&ai, AOE_AI_TAC_LIST0_COUNT) == 1 &&
           aoe_ai_tactical_get_ror(&ai, AOE_AI_TAC_LIST4_COUNT) == 2 &&
           aoe_ai_tactical_get_ror(&ai, AOE_AI_TAC_LIST1_SPARE) == 33 &&
           aoe_ai_tactical_get_ror(&ai, AOE_AI_TAC_FIELD_9CC) == 44);
    ai.field_d88[1] = 2;
    aoe_ai_counters_set_ror(&ai.counters, 2, 99);
    assert(aoe_ai_tactical_d88_ror(&ai, 1) == 2 &&
           aoe_ai_tactical_d88_ror(&ai, 4) == -1 &&
           aoe_ai_tactical_d88_counter_ror(&ai, 1) == 99 &&
           aoe_ai_tactical_d88_counter_ror(&ai, 0) == -1);

    /* Save and load at the newest version. */
    ai.block_e4c[69] = 5;
    ai.block_e4c[3] = 6;
    ai.block_fac[10] = 7;
    ai.block_fd8[8] = 8;
    ai.field_f9c = 9;
    ai.targets[2][5] = 11;
    assert(aoe_ai_tactical_save_ror(&ai, &out));
    full = out.pos;
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_ai_tactical_load_ror(&back, 4, 7.2f, &in, NULL));
    assert(in.pos == full && back.module.player == 4);
    assert(back.lists[4].count == 2 && back.lists[4].data[1] == 7);
    assert(back.sn[117] == 100 && back.sn[105] == 12 && back.targets[2][5] == 11);
    assert(back.groups_len == 3 && back.group_count == 3 &&
           back.next_group_id == 3 && back.groups[1]->members[1] == 72 &&
           back.groups[2]->field_184 == 500);
    assert(back.block_e4c[69] == 0 && back.block_e4c[3] == 6 &&
           back.block_fac[10] == 0 && back.block_fd8[8] == 8 &&
           back.field_f9c == 9 && back.state == 9 &&
           aoe_ai_counters_get_ror(&back.counters, 2) == 99);
    aoe_ai_tactical_release_ror(&back);

    /* An older layout: no list 3 before 2.3 (an empty list adds one i32). */
    {
        static uint8_t old[4096];
        AoeOutcomeStream o = {old, sizeof old, 0, 0};
        int i;
        aoe_stream_put_i32(&o, 0);           /* list 0 */
        aoe_stream_put_i32(&o, 1);           /* list 1: one value */
        aoe_stream_put_i32(&o, 42);
        aoe_stream_put_i32(&o, 3);           /* +0x9cc */
        aoe_stream_put_i32(&o, 4);           /* +0x9d0 */
        for (i = 0; i < 50 * 4; ++i) aoe_stream_put_i32(&o, i);
        for (i = 0; i < 6; ++i) aoe_stream_put_i32(&o, 0); /* lists 2, 4..8 */
        aoe_stream_put_i32(&o, 226);
        for (i = 0; i < 226; ++i) aoe_stream_put_i32(&o, i);
        for (i = 0; i < 4; ++i) aoe_stream_put_i32(&o, 0); /* +0xd28.. */
        for (i = 0; i < 24 + 4; ++i) aoe_stream_put_i32(&o, 1);
        aoe_stream_put_i32(&o, 5);           /* +0xd04 */
        aoe_stream_put_i32(&o, 0);           /* no groups */
        for (i = 0; i < 1 + 5 + 1; ++i) aoe_stream_put_i32(&o, 100 + i);
        in = (AoeOutcomeStream){old, o.pos, 0, 0};
        assert(aoe_ai_tactical_load_ror(&back, 4, 1.0f, &in, NULL));
        assert(in.pos == o.pos && back.lists[1].data[0] == 42 &&
               back.targets[1][0] == 4 && back.targets[1][4] == -1 &&
               back.sn[225] == 225 && back.next_group_id == 5 &&
               back.field_d0c == 100 && back.field_d10 == 0 &&
               back.field_d14 == 101 && back.field_d24 == 105 &&
               back.field_e18 == 0 && back.state == 106 &&
               back.field_f7c == -1 && back.field_ffc == -1);
        aoe_ai_tactical_release_ror(&back);
        /* A number count above 226 is refused. */
        old[4 * (5 + 200 + 6)] = 227;
        in = (AoeOutcomeStream){old, o.pos, 0, 0};
        assert(!aoe_ai_tactical_load_ror(&back, 4, 1.0f, &in, NULL));
        aoe_ai_tactical_release_ror(&back);
    }

    /* Ungrouping empties and deletes g1. */
    assert(aoe_ai_tactical_ungroup_ror(&ai, 70, NULL) == 1);
    assert(ai.groups_len == 2 && ai.group_count == 2 && ai.groups[0] == g2);
    assert(aoe_ai_tactical_new_group_ror(&ai, 0)->id == -1 && ai.group_count == 2 &&
           ai.groups_len == 3);
    assert(!aoe_ai_tactical_ungroup_ror(&ai, 70, NULL));
    assert(aoe_ai_tactical_delete_group_ror(&ai, 1) && ai.groups_len == 2);
    assert(!aoe_ai_tactical_delete_group_ror(&ai, 1));
    aoe_ai_tactical_new_group_ror(&ai, 1)->type = 0x6d;
    assert(aoe_ai_tactical_delete_groups_ror(&ai, 0x6d) && ai.groups_len == 1);
    aoe_ai_tactical_release_ror(&ai);
    puts("ai tactical ok");
    return 0;
}
