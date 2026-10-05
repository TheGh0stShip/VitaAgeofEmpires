/* Patched RoR 1.0a tactical AI state, numbers and unit groups. */
#include "ai_tactical.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "aoe_stream.h"

static int32_t info_value = -7;
static void info_set(void *c, int32_t v) { (void)c; info_value = v; }
static int32_t roll(void *c) { (void)c; return 17; }

static AoeAiTaskOrderRor last_order;
static int orders;
static void order(void *c, const AoeAiTaskOrderRor *o) { (void)c; last_order = *o; ++orders; }
static uint8_t bytes44[3];
static int32_t id44;
static void p44(void *c, int32_t id, const uint8_t b[3]) { (void)c; id44 = id; memcpy(bytes44, b, 3); }
static int tobject(void *c, int32_t id, AoeAiWorldObjectRor *o) {
    (void)c;
    memset(o, 0, sizeof *o);
    o->id = id;
    o->hp = (float)(id % 100);
    o->type_class = id == 801 ? 0x12 : id == 802 ? 0xd : 3;
    o->type_id = id >= 900 && id < 910 ? 0x103 : id == 805 ? 0x7d : 1;
    return id > 0 && id != 899;
}
static int ttask(void *c, int32_t id, int32_t *t) {
    (void)c;
    if (id == 803) return 0;
    *t = id == 900 ? 0x261 : id == 901 ? 0x262 : id == 804 ? 0x265 : 7;
    return 1;
}
static float tres[8] = {100.0f, 5.0f, 0, 0, 0, 0, 2.6f, 0};
static float tresource(void *c, int32_t i) { (void)c; return tres[i]; }
static double tratio(void *c) { (void)c; return 0.249; }
static int32_t titems(void *c, int32_t t, int32_t k) { (void)c; (void)k; return t == 0x67 ? 3 : 1; }
static int32_t tunbuilt_n;
static int32_t tunbuilt(void *c, int32_t t, int32_t k) { (void)c; (void)t; (void)k; return tunbuilt_n; }
static int tfar(void *c, int32_t k) { (void)c; return k == 1 ? 7 : 0; }
static int tfood(void *c) { (void)c; return 9; }
static int32_t disliked = -1, dislike_v;
static void tdislike(void *c, int32_t p, int32_t v) { (void)c; disliked = p; dislike_v = v; }
static uint32_t tnow = 300000;
static uint32_t ttime(void *c) { (void)c; return tnow; }
static double trange(void *c, int32_t id) { (void)c; return (double)(id % 7); }

static void member_tests(void) {
    static AoeAiTacticalRor ai;
    AoeAiTacticalOpsRor ops;
    AoeAiUnitGroupRor *g;
    int32_t *e, total = -1, unbuilt = -1;

    memset(&ops, 0, sizeof ops);
    ops.self = 2;
    ops.object = tobject;
    ops.world_object = tobject;
    ops.unit_task = ttask;
    ops.order = order;
    ops.player_44 = p44;
    ops.resource = tresource;
    ops.explored_ratio = tratio;
    ops.build_items = titems;
    ops.build_unbuilt = tunbuilt;
    ops.target_far = tfar;
    ops.food_far = tfood;
    ops.set_dislike = tdislike;
    ops.game_time = ttime;
    ops.range = trange;
    aoe_ai_tactical_init_ror(&ai, 2, NULL);

    ai.sn[34] = 50;
    assert(aoe_ai_tactical_effort_ror(&ai, 4.0f) == 12 &&
           aoe_ai_tactical_effort_ror(&ai, 0.25f) == 99);
    ai.field_d68[2] = 6; ai.field_d78[2] = 7; ai.field_d98[2] = 8;
    ai.field_d88[1] = 2;
    assert(aoe_ai_tactical_d68_ror(&ai, 2) == 6 && aoe_ai_tactical_d68_ror(&ai, 4) == -1);
    assert(aoe_ai_tactical_mapped_ror(&ai, AOE_AI_TAC_MAP_D98, 1) == 8 &&
           aoe_ai_tactical_mapped_ror(&ai, AOE_AI_TAC_MAP_D68, 1) == 6 &&
           aoe_ai_tactical_mapped_ror(&ai, AOE_AI_TAC_MAP_D78, 1) == 7 &&
           aoe_ai_tactical_mapped_ror(&ai, AOE_AI_TAC_MAP_D78, 0) == -1);
    ai.sn[148] = 33;
    aoe_ai_counters_set_ror(&ai.counters, 2, 50);
    assert(aoe_ai_tactical_counter_share_ror(&ai, 2) == 7 &&
           aoe_ai_tactical_counter_share_ror(&ai, 0) == 33);
    ai.sn[134] = 5;
    aoe_ai_tactical_arm_e18_ror(&ai, 4);
    assert(ai.field_e18 == 0);
    aoe_ai_tactical_arm_e18_ror(&ai, 5);
    assert(ai.field_e18 == 1);
    assert(aoe_ai_tactical_age_is_ror(&ops, 2) && !aoe_ai_tactical_age_is_ror(&ops, 3));

    /* Gatherers. */
    assert(!aoe_ai_tactical_add_gatherer_ror(&ai, -1));
    e = aoe_ai_tactical_add_gatherer_ror(&ai, 10);
    assert(e == ai.gatherers[0] && e[4] == -1 && e[5] == 0 && ai.field_9cc == 1);
    assert(aoe_ai_tactical_add_gatherer_ror(&ai, 10) == e && ai.field_9cc == 1);
    aoe_ai_tactical_add_gatherer_ror(&ai, 11)[4] = 3;
    aoe_ai_tactical_add_gatherer_ror(&ai, 12)[4] = 3;
    ai.gatherers[2][5] = 99;
    assert(aoe_ai_tactical_count_gatherers_ror(&ai, 3) == 2 &&
           aoe_ai_tactical_has_gatherer_ror(&ai, 12) &&
           aoe_ai_tactical_gatherer_ror(&ai, 11) == ai.gatherers[1]);
    assert(aoe_ai_tactical_remove_gatherer_ror(&ai, 10) && ai.field_9cc == 2 &&
           ai.gatherers[0][0] == 11 && ai.gatherers[1][0] == 12 &&
           ai.gatherers[2][0] == -1 && ai.gatherers[2][5] == 99 &&
           ai.gatherers[1][5] == 0);
    assert(!aoe_ai_tactical_remove_gatherer_ror(&ai, 10) &&
           !aoe_ai_tactical_remove_gatherer_ror(&ai, -1));

    /* Dropping and detasking. */
    aoe_ai_int_list_add_unique_ror(&ai.lists[1], 804);
    aoe_ai_int_list_add_unique_ror(&ai.lists[1], 805);
    aoe_ai_tactical_add_gatherer_ror(&ai, 804)[1] = 2;
    aoe_ai_tactical_detask_ror(&ai, 804, &ops);
    assert(ai.lists[1].count == 1 && ai.lists[1].data[0] == 805 &&
           !aoe_ai_tactical_has_gatherer_ror(&ai, 804) && ai.field_d68[2] == 6);
    assert(orders == 1 && last_order.action == 0x2c2 && last_order.unit == 804 &&
           last_order.player == 2 && last_order.value == 100 && last_order.a == -1.0f &&
           last_order.flag == 1);
    aoe_ai_tactical_detask_ror(&ai, -1, &ops);
    assert(orders == 1);
    aoe_ai_tactical_order_move_ror(&ops, 5, 3.0f, 4.0f, 9);
    assert(last_order.action == 0x2c6 && last_order.b == 4.0f && last_order.c == 1.0f &&
           last_order.target == -1);
    aoe_ai_tactical_order_2d4_ror(&ops, 5, 6, 1, 2, 3, 4, 7);
    assert(last_order.action == 0x2d4 && last_order.target == 6 && last_order.d == 4.0f);
    aoe_ai_tactical_player_44_ror(&ops, 9, 4, 5);
    assert(id44 == 9 && bytes44[0] == 4 && bytes44[1] == 5 && bytes44[2] == 1);

    /* Object tests. */
    assert(aoe_ai_tactical_priest_or_siege_ror(&ops, 801) &&
           aoe_ai_tactical_priest_or_siege_ror(&ops, 802) &&
           !aoe_ai_tactical_priest_or_siege_ror(&ops, 803));
    assert(aoe_ai_tactical_unit_has_task_ror(&ops, 900, 0x261) &&
           !aoe_ai_tactical_unit_has_task_ror(&ops, 803, 0x261));
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 900);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 901);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 902);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 805);
    assert(aoe_ai_tactical_count_busy_103_ror(&ai, &ops) == 2 &&
           aoe_ai_tactical_busy_103_ror(&ops, 901, 0));

    /* Groups. */
    g = aoe_ai_tactical_new_group_ror(&ai, 1);
    g->task = 0x68; g->field_180 = 8;
    g = aoe_ai_tactical_new_group_ror(&ai, 1);
    g->task = 100; g->field_160 = 5;
    aoe_ai_unit_group_add_ror(g, 805, NULL);
    aoe_ai_unit_group_add_ror(g, 899, NULL);
    aoe_ai_unit_group_add_ror(g, 950, NULL);
    aoe_ai_unit_group_add_ror(g, 977, NULL);
    g->added_time = 200000;
    assert(aoe_ai_tactical_count_groups_8_ror(&ai) == 1);
    /* Leader by hit points (id % 100): largest by default, smallest for
       1 and 3; 805 is type 0x7d and 899 missing. */
    assert(aoe_ai_tactical_pick_leader_ror(&ai, g, &ops) == 977);
    ai.sn[75] = 1;
    assert(aoe_ai_tactical_pick_leader_ror(&ai, g, &ops) == 950);
    ai.sn[75] = 2; /* range id % 7: 950 -> 5, 977 -> 4 */
    assert(aoe_ai_tactical_pick_leader_ror(&ai, g, &ops) == 950);
    /* Settling: 5 - 4 = 1 excess, 100000 ms = 1 minute passed. */
    aoe_ai_tactical_settle_groups_ror(&ai, &ops);
    assert(g->field_160 == 4);
    g->field_160 = 9;
    aoe_ai_tactical_settle_groups_ror(&ai, &ops);
    assert(g->field_160 == 9);
    assert(aoe_ai_tactical_cap_groups_ror(&ai, 3, &ops) == 1 && g->field_160 == 4 &&
           disliked == 3 && dislike_v == 100);
    ai.field_e18 = 0;
    assert(aoe_ai_tactical_cap_groups_ror(&ai, 4, &ops) == 0 && disliked == 3);

    /* Exploration share and dropsite needs. */
    ai.sn[167] = 0;
    assert(aoe_ai_tactical_explored_enough_ror(&ai, &ops));
    ai.sn[167] = 25;
    assert(!aoe_ai_tactical_explored_enough_ror(&ai, &ops));
    ai.sn[167] = 24;
    assert(aoe_ai_tactical_explored_enough_ror(&ai, &ops));
    tunbuilt_n = 1;
    assert(aoe_ai_tactical_need_granary_ror(&ai, &total, &unbuilt, 1, &ops) == 0 &&
           unbuilt == 1);
    tunbuilt_n = 0;
    ai.sn[191] = 5; /* stock 5 reaches it */
    assert(aoe_ai_tactical_need_granary_ror(&ai, &total, &unbuilt, 1, &ops) == 0);
    ai.sn[191] = 6;
    ai.sn[150] = -1;
    assert(aoe_ai_tactical_need_granary_ror(&ai, &total, &unbuilt, 1, &ops) == 7 && total == 3);
    ai.sn[150] = 2; /* 2 * age 2 - 3 */
    assert(aoe_ai_tactical_need_granary_ror(&ai, &total, &unbuilt, 1, &ops) == 1);
    ai.sn[150] = 1;
    assert(aoe_ai_tactical_need_granary_ror(&ai, &total, &unbuilt, 1, &ops) == 0);
    ai.sn[190] = 101;
    ai.sn[151] = -1;
    assert(aoe_ai_tactical_need_pit_ror(&ai, &total, &unbuilt, &ops) == 9 && total == 1);
    ai.sn[151] = 1;
    assert(aoe_ai_tactical_need_pit_ror(&ai, &total, &unbuilt, &ops) == 0);
    ai.sn[190] = 100;
    ai.sn[151] = -1;
    assert(aoe_ai_tactical_need_pit_ror(&ai, &total, &unbuilt, &ops) == 0);
    aoe_ai_tactical_release_ror(&ai);
}

static int32_t pf4(void *c, int32_t p) { (void)c; return p == 2 || p == 4 || p == 5 ? 1 : 0; }
static int s_ally(void *c, int32_t p) { (void)c; return p != 5; }
static int s_names(void *c, int32_t p) { (void)c; return p != 5; }
static uint8_t stats_p = 0xff;
static uint8_t s_stats(void *c) { (void)c; return stats_p; }
static void s_set_stats(void *c, uint8_t p) { (void)c; stats_p = p; }
static int32_t sent_to[8], sent_n;
static void s54(void *c, int32_t p, int32_t from, int32_t k, int32_t a, int32_t b, int32_t cc) {
    (void)c; assert(from == 2 && k == 2 && a == 7 && b == 8 && cc == 9); sent_to[sent_n++] = p;
}
static int32_t s_rand(void *c) { (void)c; return 4; }

static void share_tests(void) {
    static AoeAiTacticalRor ai;
    AoeAiTacticalOpsRor ops;
    AoeAiIntListRor ids = {0};

    memset(&ops, 0, sizeof ops);
    ops.self = 2;
    ops.object = tobject;
    ops.order = order;
    ops.game_time = ttime;
    ops.player_count = 6;
    ops.player_field_4 = pf4;
    ops.is_ally = s_ally;
    ops.names_us_ally = s_names;
    ops.stats_player = s_stats;
    ops.set_stats_player = s_set_stats;
    ops.player_54 = s54;
    ops.rand = s_rand;
    ops.range = trange;
    aoe_ai_tactical_init_ror(&ai, 2, NULL);

    aoe_ai_int_list_add_unique_ror(&ids, 950);
    aoe_ai_int_list_add_unique_ror(&ids, 977);
    assert(aoe_ai_tactical_pick_leader_ids_ror(&ai, &ids, &ops) == 977);
    aoe_ai_int_list_release_ror(&ids);

    assert(aoe_ai_tactical_order_2bd_ror(&ai, 5, 6, 2.5f, 3, &ops) && ai.list_d58.count == 1 &&
           last_order.action == 0x2bd && last_order.d == 2.5f && last_order.a == -1.0f &&
           last_order.value == 3);
    assert(!aoe_ai_tactical_order_2cd_ror(&ai, -1, 6, &ops));
    assert(aoe_ai_tactical_order_2cd_ror(&ai, 5, 6, &ops) && ai.list_d58.count == 1 &&
           last_order.action == 0x2cd && last_order.target2 == 2 && last_order.d == 1.0f &&
           last_order.value == 100);

    /* Three computer players (2, 4, 5): rand 4 % 3 + 1 = 2nd, player 4. Player
       5 is no ally; 4 is above us but the statistics player: nothing sent. */
    ai.sn[196] = 1;
    ai.sn[197] = 10;
    tnow = 20000;
    aoe_ai_tactical_share_with_allies_ror(&ai, 7, 8, 9, &ops);
    assert(stats_p == 4 && sent_n == 0 && ai.field_fa0 == 20000);
    tnow = 25000;
    stats_p = 2;
    aoe_ai_tactical_share_with_allies_ror(&ai, 7, 8, 9, &ops);
    assert(sent_n == 0 && ai.field_fa0 == 20000);
    tnow = 30000;
    aoe_ai_tactical_share_with_allies_ror(&ai, 7, 8, 9, &ops);
    assert(sent_n == 1 && sent_to[0] == 4);
    aoe_ai_tactical_release_ror(&ai);
}

static void collect_tests(void) {
    static AoeAiTacticalRor ai;
    AoeAiUnitGroupRor *g;
    aoe_ai_tactical_init_ror(&ai, 1, NULL);
    aoe_ai_int_list_add_unique_ror(&ai.lists[2], 1);
    aoe_ai_int_list_add_unique_ror(&ai.lists[2], 2);
    aoe_ai_int_list_add_unique_ror(&ai.lists[2], 3);
    aoe_ai_int_list_add_unique_ror(&ai.list_d48, 99);
    g = aoe_ai_tactical_new_group_ror(&ai, 1);
    g->task = 100;
    aoe_ai_unit_group_add_ror(g, 2, NULL);
    aoe_ai_unit_group_add_ror(g, 4, NULL);
    aoe_ai_unit_group_add_ror(g, 5, NULL);
    aoe_ai_unit_group_add_ror(g, 6, NULL);
    assert(aoe_ai_tactical_collect_ungrouped_ror(&ai, 2) == 2 && ai.list_d48.count == 2 &&
           ai.list_d48.data[0] == 1 && ai.list_d48.data[1] == 3);
    assert(aoe_ai_tactical_collect_ungrouped_ror(&ai, 9) == 0 && ai.list_d48.count == 0);
    /* Surplus: 4 members, keep 2, commander 4 skipped: 2 and 5 go. */
    g->field_160 = 2;
    g->commander = 4;
    assert(aoe_ai_tactical_collect_surplus_ror(&ai, 0) == 2 && ai.list_d48.data[0] == 2 &&
           ai.list_d48.data[1] == 5);
    assert(aoe_ai_tactical_collect_surplus_ror(&ai, 1) == 0);
    g->field_180 = 2;
    assert(aoe_ai_tactical_collect_surplus_ror(&ai, 0) == 0);
    g->field_180 = 1;
    g->task = 0x6c;
    g->field_160 = 3;
    assert(aoe_ai_tactical_collect_surplus_ror(&ai, 1) == 1 && ai.list_d48.data[0] == 2);
    aoe_ai_tactical_release_ror(&ai);
}

static int pobject(void *c, int32_t id, AoeAiWorldObjectRor *o) {
    (void)c;
    memset(o, 0, sizeof *o);
    o->id = id;
    o->type_id = id == 13 ? 0x103 : 1;
    o->x = (float)id;
    o->y = 0.0f;
    if (id == 50) { o->x = 20.7f; o->y = 20.2f; }
    return id > 0 && id != 14;
}
static int paction(void *c, int32_t id, int32_t *a) { (void)c; *a = id == 12 ? 77 : 0; return id != 15; }
static double pdist(void *c, int32_t id, float x, float y, float z) {
    AoeAiWorldObjectRor o; (void)z; pobject(c, id, &o);
    return sqrt((double)(o.x - x) * (o.x - x) + (double)(o.y - y) * (o.y - y));
}
static int32_t pcenter_id = 50;
static int32_t pcenter(void *c) { (void)c; return pcenter_id; }
static uint8_t igrid[64][64];
static int igrid_get(void *c, int32_t x, int32_t y) { (void)c; return igrid[x][y]; }
static void igrid_set(void *c, int32_t x, int32_t y, int32_t v) { (void)c; igrid[x][y] = (uint8_t)v; }
static uint8_t izone(void *c, int32_t id) { (void)c; (void)id; return 1; }
static uint8_t itile_zone(void *c, int32_t id, int32_t x, int32_t y) { (void)c; (void)id; (void)y; return (uint8_t)(x == 30 ? 2 : 1); }

static void pick_tests(void) {
    static AoeAiTacticalRor ai;
    static AoeAiInfoRor info;
    AoeAiTacticalOpsRor ops;
    AoeAiInfoOpsRor iops;
    int32_t tile[2] = {0, 0}, out[2] = {-1, -1};
    memset(&ops, 0, sizeof ops);
    ops.object = pobject;
    ops.world_object = pobject;
    ops.unit_action = paction;
    ops.distance = pdist;
    ops.first_center = pcenter;
    aoe_ai_tactical_init_ror(&ai, 1, NULL);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 11);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 12);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 13);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 14);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 15);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 16);
    assert(aoe_ai_tactical_pick_unit_ror(&ai, tile, 1, 77, -9, -9, 0, &ops) == 11);
    assert(aoe_ai_tactical_pick_unit_ror(&ai, tile, 0, 77, -9, -9, 0, &ops) == 16);
    assert(aoe_ai_tactical_pick_unit_ror(&ai, tile, 0, 77, -9, -9, 1, &ops) == 16);
    aoe_ai_int_list_add_unique_ror(&ai.list_d58, 11);
    assert(aoe_ai_tactical_pick_unit_ror(&ai, tile, 1, -9, -9, -9, 0, &ops) == 12);
    assert(aoe_ai_tactical_pick_unit_ror(&ai, tile, 1, 0, 77, -9, 1, &ops) == -1);

    /* Exploring from the centre at (20,20): tiles at 10. Unit 25 sits at
       (25,0): (20,10) is nearest (dist sqrt(125)) but explored; (30,10)
       is another zone; next is (10,10) vs (30,20)... */
    memset(&iops, 0, sizeof iops);
    iops.zone = izone;
    iops.tile_zone = itile_zone;
    iops.grid_get = igrid_get;
    iops.grid_set = igrid_set;
    info.map_width = info.map_height = 64;
    memset(igrid, 0xff, sizeof igrid);
    igrid[20][10] = 1;
    assert(aoe_ai_tactical_explore_spot_ror(&ai, 25, out, &ops, &info, &iops) == 1);
    assert(out[0] == 10 && out[1] == 10 && igrid[10][10] == 0);
    /* No centre: the information AI's choice (nothing remembered: 0). */
    pcenter_id = -1;
    assert(aoe_ai_tactical_explore_spot_ror(&ai, 25, out, &ops, &info, &iops) == 0);
    aoe_ai_tactical_release_ror(&ai);
}

static int ng_zone(void *c, int32_t id, uint8_t *z) {
    (void)c;
    if (id < 0) return 0;
    *z = (uint8_t)(id == 70 ? 2 : 1);
    return 1;
}

static int idle_task(void *c, int32_t id, int32_t *t) { (void)c; *t = id == 5 ? -1 : 3; return 1; }
static int step_free(void *c, int32_t id, float x, float y) { (void)c; (void)id; (void)y; return x > 5.0f; }
static double step_dist(void *c, int32_t id, float x, float y, float z) {
    (void)c; (void)id; (void)z; return fabs((double)x - 20.0) + fabs((double)y - 20.0);
}
static int32_t world100(void *c) { (void)c; return 3; }

static void step_and_added_tests(void) {
    static AoeAiTacticalRor ai;
    AoeAiTacticalOpsRor ops;
    AoeAiWorldObjectRor u;
    memset(&ops, 0, sizeof ops);
    ops.object = tobject;
    ops.world_object = tobject;
    ops.unit_task = idle_task;
    ops.free_tile = step_free;
    ops.distance = step_dist;
    ops.order = order;
    /* Box (4, 4)..(10, 10): standable from x 5 on; nearest to (20, 20) is
       the bottom-right point (12, 12). */
    orders = 0;
    assert(aoe_ai_tactical_step_around_ror(&ops, 5, 4.0f, 4.0f, 10.0f, 10.0f) == 1);
    assert(orders == 1 && last_order.action == 0x2c6 && last_order.a == 12.0f &&
           last_order.b == 12.0f);
    assert(!aoe_ai_tactical_step_around_ror(&ops, 6, 4.0f, 4.0f, 10.0f, 10.0f));
    assert(!aoe_ai_tactical_step_around_ror(&ops, 5, -9.0f, 4.0f, 0.0f, 10.0f) && orders == 1);

    /* 0x004d2710 */
    aoe_ai_tactical_init_ror(&ai, 1, NULL);
    ops.world_100 = world100;
    memset(&u, 0, sizeof u);
    u.id = 30; u.type_class = 4;
    aoe_ai_tactical_unit_added_ror(&ai, &u, &ops);
    aoe_ai_tactical_unit_added_ror(&ai, &u, &ops);
    u.id = 31; u.type_class = 0x12;
    aoe_ai_tactical_unit_added_ror(&ai, &u, &ops);
    u.id = 32; u.type_class = 0x14;
    aoe_ai_tactical_unit_added_ror(&ai, &u, &ops);
    u.id = 33; u.type_class = 2;
    aoe_ai_tactical_unit_added_ror(&ai, &u, &ops);
    u.id = 34; u.type_class = 0x1e; u.type_id = 0x9f;
    aoe_ai_tactical_unit_added_ror(&ai, &u, &ops);
    u.id = 35; u.type_id = 0x10;
    aoe_ai_tactical_unit_added_ror(&ai, &u, &ops);
    assert(ai.lists[0].count == 1 && ai.lists[2].count == 1 && ai.lists[4].count == 2 &&
           ai.lists[8].count == 1 && ai.lists[8].data[0] == 32 && ai.lists[7].data[0] == 33 &&
           ai.lists[9].count == 1 && ai.lists[9].data[0] == 34 && ai.lists[5].count == 0);
    aoe_ai_tactical_release_ror(&ai);
    orders = 0;
}

/* Units 40..44 and 801/802: task -1 for 40 and the villagers 900..902,
   else 7; +0x2c id - 40. */
static int order_task(void *c, int32_t id, int32_t *t) {
    (void)c;
    if (id == 803) return 0;
    *t = id == 40 || id >= 900 ? -1 : 7;
    return 1;
}
static int order_2c(void *c, int32_t id, int32_t *n) { (void)c; *n = id - 40; return 1; }

static int idle_action(void *c, int32_t id, int32_t *a) { (void)c; (void)id; *a = -1; return 1; }
static double id_dist(void *c, int32_t id, float x, float y, float z) {
    (void)c; (void)y; (void)z; return fabs((double)x - (double)id);
}
static int32_t amounts[4] = {50, 400, 10, 300};
static int32_t amount(void *c, int32_t i) { (void)c; return amounts[i]; }
static void costs4(void *c, AoeAiCountersRor *k, int32_t limit) {
    (void)c;
    assert(limit == 7);
    aoe_ai_counters_set_ror(k, 0, 100);
    aoe_ai_counters_set_ror(k, 1, 0);
    aoe_ai_counters_set_ror(k, 2, 20);
    aoe_ai_counters_set_ror(k, 3, 100);
}

static void gather_order_tests(void) {
    static AoeAiTacticalRor ai;
    AoeAiTacticalOpsRor ops;
    aoe_ai_tactical_init_ror(&ai, 1, NULL);
    memset(&ops, 0, sizeof ops);
    ops.build_costs = costs4;
    ops.resource_amount = amount;
    ai.sn[147] = 7;
    ai.sn[190] = ai.sn[191] = ai.sn[192] = 1000;
    ai.sn[193] = 200;
    /* Surpluses: food -50, wood 400, stone -10, gold 200. Wanted (cost and
       below the cap): food, stone; then the rest by surplus. */
    aoe_ai_tactical_gather_order_ror(&ai, &ops);
    assert(ai.field_d98[0] == -50 && ai.field_d98[1] == 400 && ai.field_d98[2] == -10 &&
           ai.field_d98[3] == 200);
    assert(ai.field_d88[0] == 0 && ai.field_d88[1] == 2 && ai.field_d88[2] == 3 &&
           ai.field_d88[3] == 1);
    aoe_ai_tactical_release_ror(&ai);
}

static void removed_and_path_tests(void) {
    static AoeAiTacticalRor ai;
    AoeAiTacticalOpsRor ops;
    AoeAiUnitGroupRor *g;
    int32_t i;
    memset(&ops, 0, sizeof ops);
    ops.object = tobject;
    ops.world_object = tobject;
    ops.unit_task = order_task;
    ops.unit_2c = order_2c;
    aoe_ai_tactical_init_ror(&ai, 1, NULL);
    aoe_ai_int_list_add_unique_ror(&ai.lists[4], 50);
    aoe_ai_int_list_add_unique_ror(&ai.lists[6], 50);
    aoe_ai_int_list_add_unique_ror(&ai.lists[9], 51);
    aoe_ai_int_list_add_unique_ror(&ai.lists[9], 50);
    g = aoe_ai_tactical_new_group_ror(&ai, 1);
    aoe_ai_unit_group_add_ror(g, 50, NULL);
    aoe_ai_unit_group_add_ror(g, 52, NULL);
    aoe_ai_tactical_unit_removed_ror(&ai, 50, &ops, NULL);
    assert(ai.lists[4].count == 0 && ai.lists[6].count == 0 && ai.lists[9].count == 2 &&
           aoe_ai_unit_group_count_ror(g) == 1);
    aoe_ai_tactical_unit_removed_ror(&ai, 51, &ops, NULL);
    assert(ai.lists[9].count == 1 && ai.lists[9].data[0] == 50);

    /* 0x004d8960 over list 2: 40 (no task), 41..44 (+0x2c 1..4), 43 on
       +0xd58, 803 without a unit AI. */
    for (i = 40; i <= 44; ++i) aoe_ai_int_list_add_unique_ror(&ai.lists[2], i);
    aoe_ai_int_list_add_unique_ror(&ai.lists[2], 803);
    aoe_ai_int_list_add_unique_ror(&ai.list_d58, 43);
    assert(aoe_ai_tactical_collect_by_order_ror(&ai, 2, -1, 1, &ops) == 2);
    assert(ai.list_d48.count == 2 && ai.list_d48.data[0] == 40 && ai.list_d48.data[1] == 41);
    assert(aoe_ai_tactical_collect_by_order_ror(&ai, 2, 0, 0, &ops) == 1 && ai.list_d48.count == 3);
    assert(aoe_ai_tactical_collect_by_order_ror(&ai, 2, 1, 1, &ops) == 1 &&
           ai.list_d48.count == 1 && ai.list_d48.data[0] == 44);
    assert(aoe_ai_tactical_collect_by_order_ror(&ai, 2, 1, 0, &ops) == 1 && ai.list_d48.count == 1);
    /* 0x004d8be0: list 5 (any class), then list 2's priests (801) and
       siege (802) only; 801 / 802 have +0x2c 761 / 762. */
    aoe_ai_int_list_add_unique_ror(&ai.lists[5], 41);
    aoe_ai_int_list_add_unique_ror(&ai.lists[2], 801);
    aoe_ai_int_list_add_unique_ror(&ai.lists[2], 802);
    assert(aoe_ai_tactical_collect_support_ror(&ai, 100, 1, &ops) == 2 &&
           ai.list_d48.count == 2 && ai.list_d48.data[0] == 801);
    assert(aoe_ai_tactical_collect_support_ror(&ai, 2, -1, &ops) == 1 &&
           ai.list_d48.data[0] == 41);
    /* 0x004d90b0 over list 0: 900 (gatherer), 901 (on list 1), 902 idle. */
    ops.unit_action = idle_action;
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 900);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 901);
    aoe_ai_tactical_add_gatherer_ror(&ai, 900);
    aoe_ai_int_list_add_unique_ror(&ai.lists[1], 901);
    assert(aoe_ai_tactical_idle_villager_ror(&ai, 2, &ops) == -1);
    assert(aoe_ai_tactical_idle_villager_ror(&ai, 0, &ops) == 900);
    assert(aoe_ai_tactical_idle_villager_ror(&ai, 3, &ops) == 900);
    assert(aoe_ai_tactical_idle_villager_ror(&ai, 1, &ops) == 901);
    aoe_ai_int_list_add_unique_ror(&ai.lists[0], 902);
    assert(aoe_ai_tactical_idle_villager_ror(&ai, 2, &ops) == 902);
    {
        /* 0x004d93a0: 902 is free outright; without it, mode 0 picks the
           nearest / farthest of 900..902 from x = 902. */
        int32_t pt[2] = {902, 0};
        ops.distance = id_dist;
        assert(aoe_ai_tactical_villager_near_ror(&ai, 2, pt, 1, &ops) == 902);
        aoe_ai_int_list_remove_ror(&ai.lists[0], 902);
        assert(aoe_ai_tactical_villager_near_ror(&ai, 2, pt, 1, &ops) == -1);
        assert(aoe_ai_tactical_villager_near_ror(&ai, 0, pt, 1, &ops) == 901);
        assert(aoe_ai_tactical_villager_near_ror(&ai, 0, pt, 0, &ops) == 900);
        assert(aoe_ai_tactical_villager_near_ror(&ai, 1, pt, 0, &ops) == 901);
    }
    aoe_ai_tactical_release_ror(&ai);
}

static void nearest_group_tests(void) {
    static AoeAiTacticalRor ai;
    AoeAiTacticalOpsRor ops;
    AoeAiUnitGroupRor *a, *b, *c;
    int32_t pt[2] = {10, 10};
    aoe_ai_tactical_init_ror(&ai, 1, NULL);
    memset(&ops, 0, sizeof ops);
    ops.object_zone = ng_zone;
    a = aoe_ai_tactical_new_group_ror(&ai, 1);
    b = aoe_ai_tactical_new_group_ror(&ai, 1);
    c = aoe_ai_tactical_new_group_ror(&ai, 1);
    a->task = b->task = c->task = 0x6a;
    a->field_180 = b->field_180 = 4;
    c->field_180 = 5;
    a->loc_19c[0] = 13.9f; a->loc_19c[1] = 14.2f;   /* 3, 4: 5 */
    b->loc_19c[0] = 11.0f; b->loc_19c[1] = 10.0f;   /* 1 */
    c->loc_19c[0] = 10.0f; c->loc_19c[1] = 10.0f;
    a->commander = 70;
    b->commander = 71;
    assert(aoe_ai_tactical_nearest_group_ror(&ai, -1, 4, 6, pt, -1, &ops) == b);
    assert(aoe_ai_tactical_nearest_group_ror(&ai, -1, 4, 1, pt, -1, &ops) == NULL);
    assert(aoe_ai_tactical_nearest_group_ror(&ai, -1, -1, 6, pt, -1, &ops) == c);
    /* With max -1 the last match wins. */
    assert(aoe_ai_tactical_nearest_group_ror(&ai, -1, 4, -1, pt, -1, &ops) == b);
    a->loc_19c[0] = 10.0f;
    assert(aoe_ai_tactical_nearest_group_ror(&ai, -1, 4, -1, pt, -1, &ops) == b);
    /* Zone 2 keeps only a. */
    assert(aoe_ai_tactical_nearest_group_ror(&ai, -1, 4, 9, pt, 2, &ops) == a);
    /* Task 0x6a skips a group another names in +0x1c4. */
    c->field_1c4 = b->id;
    assert(aoe_ai_tactical_nearest_group_ror(&ai, 0x6a, 4, 9, pt, -1, &ops) == a);
    assert(aoe_ai_tactical_nearest_group_ror(&ai, 0x6b, 4, 9, pt, -1, &ops) == NULL);
    aoe_ai_tactical_release_ror(&ai);
}

int main(void) {
    static AoeAiTacticalRor ai, back;
    static uint8_t buf[1 << 16];
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;
    AoeAiUnitGroupRor *g1, *g2, *g3;
    AoeAiStrategyRor strategy;
    size_t full;

    nearest_group_tests();
    step_and_added_tests();
    removed_and_path_tests();
    gather_order_tests();

    aoe_ai_tactical_init_ror(&ai, 3, NULL);
    assert(ai.module.id == 0x3f1 && !strcmp(ai.module.name, "TacAI"));
    assert(ai.sn[0] == -1 && ai.sn[225] == -1 && ai.gatherers[49][4] == -1 &&
           ai.gatherers[49][5] == 0 && ai.field_d88[3] == -1 &&
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
    g1->task = 0x66;
    g2->task = 0x66;
    g2->field_180 = 4;
    g3->task = 0x6d;
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
    ai.gatherers[2][5] = 11;
    assert(aoe_ai_tactical_save_ror(&ai, &out));
    full = out.pos;
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_ai_tactical_load_ror(&back, 4, 7.2f, &in, NULL));
    assert(in.pos == full && back.module.player == 4);
    assert(back.lists[4].count == 2 && back.lists[4].data[1] == 7);
    assert(back.sn[117] == 100 && back.sn[105] == 12 && back.gatherers[2][5] == 11);
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
               back.gatherers[1][0] == 4 && back.gatherers[1][4] == -1 &&
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
    aoe_ai_tactical_new_group_ror(&ai, 1)->task = 0x6d;
    assert(aoe_ai_tactical_delete_groups_ror(&ai, 0x6d) && ai.groups_len == 1);
    aoe_ai_tactical_release_ror(&ai);
    member_tests();
    share_tests();
    collect_tests();
    pick_tests();
    puts("ai tactical ok");
    return 0;
}
