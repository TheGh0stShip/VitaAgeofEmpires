/* Patched RoR 1.0a tactical-AI unit group 0x004cc630..0x004cd2c0. */
#include "ai_unit_group.h"

#include <assert.h>
#include <string.h>

static int32_t led[41], led_n = -1, lead_by = -2, released = -2, follows, unfollows;
static int32_t dismissed = -1;

static void lead(void *c, int32_t cmd, const int32_t *ids, int32_t n, float r) {
    (void)c; assert(r == 4.0f);
    lead_by = cmd; led_n = n; memcpy(led, ids, (size_t)n * 4);
}
static void follow(void *c, int32_t cmd, int32_t id, float r) { (void)c; (void)cmd; (void)id; assert(r == 4.0f); ++follows; }
static void unfollow(void *c, int32_t cmd, int32_t id) { (void)c; (void)cmd; (void)id; ++unfollows; }
static void release(void *c, int32_t cmd) { (void)c; released = cmd; }
static int32_t now(void *c) { (void)c; return 777; }
static int present(void *c, int32_t id) { (void)c; return id != 12; }
static int hp(void *c, int32_t id, float *out) { (void)c; if (id == 12) return 0; *out = (float)id + 0.9f; return 1; }
static void dismiss(void *c, int32_t id, int32_t v) { (void)c; assert(v == 100); dismissed = id; }
static double dist(void *c, int32_t id, float x, float y, float z) {
    (void)c; assert(x == 5.0f && y == 6.0f && z == 7.0f);
    return id == 21 ? 0.25 : (double)id;
}
static int unit_ai(void *c, int32_t id, int32_t *type, int32_t *task, int32_t *action) {
    (void)c;
    if (id == 20) return 0;
    *type = id == 22 ? 0x7d : 1;
    *task = id == 23 ? 600 : id == 24 ? -1 : 5;
    *action = id == 25 ? 0x2bd : 0;
    return 1;
}
static int32_t sn_asked;
static int32_t sn(void *c, int32_t i) { (void)c; sn_asked = i; return i == 23 ? 30 : 3; }

/* 0x004cd2d0 group update. Units 30.. are live; 39 is gone; unit 31 is
   busy (task 5). */
static int32_t u_stops, u_follow_n, u_zero_n, u_move_n, u_2d4_n, u_last_value;
static float u_last_d;
static int u_ai(void *c, int32_t id, int32_t *type, int32_t *task, int32_t *action) {
    (void)c;
    if (id == 39) return 0;
    *type = 1;
    *task = id == 31 ? 5 : -1;
    *action = id == 32 ? 7 : -1;
    return 1;
}
static int u_hp(void *c, int32_t id, float *out) { (void)c; *out = (float)id + 0.5f; return 1; }
static void u_stop(void *c, int32_t id, int32_t v) { (void)c; (void)id; assert(v == 100); ++u_stops; }
static int u_present(void *c, int32_t id) { (void)c; (void)id; return 1; }
static int32_t u_sn(void *c, int32_t i) { (void)c; return i == 200 ? 50 : i == 0x29 ? 4 : 0; }
static void u_2bd(void *c, int32_t id, int32_t target, float d, int32_t v) {
    (void)c; (void)id; (void)target; u_last_d = d; u_last_value = v; ++u_follow_n;
}
static void u_zero(void *c, int32_t id, int32_t z) { (void)c; (void)id; assert(z == 0); ++u_zero_n; }
static int32_t u_rand(void *c) { (void)c; return 149; } /* 49 < 50 */
static float u_t28(void *c, int32_t id) { (void)c; (void)id; return 6.0f; }
static int u_pos(void *c, int32_t id, float out[3]) {
    (void)c; out[0] = id == 33 ? 90.0f : 1.0f; out[1] = 1.0f; out[2] = 0.0f; return 1;
}
static int32_t u_zone(void *c, int32_t id) { (void)c; (void)id; return 0x102; }
static int32_t u_zone_at(void *c, int32_t id, int32_t x, int32_t y) { (void)c; (void)id; (void)y; return x >= 50 ? 3 : 2; }
static double u_dist(void *c, int32_t id, float x, float y, float z) { (void)c; (void)x; (void)y; (void)z; return id == 30 ? 2.0 : 10.0; }
static void u_2d4(void *c, int32_t id, int32_t t, float x, float y, float z, float d, int32_t v) {
    (void)c; (void)id; (void)t; (void)x; (void)y; (void)z; (void)v; assert(d == 4.0f); ++u_2d4_n;
}
static void u_move(void *c, int32_t id, float x, float y, int32_t v) { (void)c; (void)id; (void)x; (void)y; (void)v; ++u_move_n; }

static void update_tests(void) {
    static AoeAiUnitGroupRor g;
    AoeAiUnitGroupOpsRor ops;
    int i;
    memset(&ops, 0, sizeof(ops));
    ops.unit_ai = u_ai; ops.hit_points = u_hp; ops.dismiss = u_stop;
    ops.present = u_present; ops.sn = u_sn; ops.order_2bd = u_2bd;
    ops.order_zero = u_zero; ops.rand = u_rand; ops.type_28 = u_t28;
    ops.position = u_pos; ops.zone = u_zone; ops.zone_at = u_zone_at;
    ops.distance = u_dist; ops.order_2d4 = u_2d4; ops.order_move = u_move;

    aoe_ai_unit_group_init_ror(&g);
    assert(aoe_ai_unit_group_update_ror(&g, &ops, 0, 1, 0) == 0); /* no commander */
    for (i = 30; i < 34; ++i) aoe_ai_unit_group_add_ror(&g, i, &ops);
    aoe_ai_unit_group_add_ror(&g, 39, &ops);
    g.commander = 30;
    g.flag_2c8 = 5;
    /* Task 0: everyone live stops, the gone member is dropped, values are
       the truncated hit points and the record is taken. */
    assert(aoe_ai_unit_group_update_ror(&g, &ops, 0, 1, 0) == 1);
    assert(u_stops == 4 && g.count == 4 && g.values[0] == 30 &&
           g.field_168 == 30 + 31 + 32 + 33 && g.field_16c == 4 &&
           g.type == 0 && g.flag_2c8 == 0);
    /* Task 5 records but returns 0. */
    assert(aoe_ai_unit_group_update_ror(&g, &ops, 5, 1, 0) == 0 && g.type == 1);
    /* Task 7: the commander gets the zero order; idle others follow at
       half of +0x28 with 99; the busy one is skipped unless forced. */
    assert(aoe_ai_unit_group_update_ror(&g, &ops, 7, 0, 0) == 1);
    assert(u_zero_n == 1 && u_follow_n == 2 && u_last_d == 3.0f &&
           u_last_value == 99);
    /* Task 9: unit 33 sits in another zone and is dropped; 30 is within
       number 41 (gap 1.0 < 4) with no action, so it is left alone. */
    g.loc_19c[0] = 60.0f; /* outside the commander's zone: reset */
    u_stops = 0;
    assert(aoe_ai_unit_group_update_ror(&g, &ops, 9, 0, 1) == 1);
    assert(g.loc_19c[0] == 1.0f && g.flag_2c8 == 1);
    assert(g.count == 3 && !aoe_ai_unit_group_contains_ror(&g, 33));
    assert(u_stops == 0 && u_2d4_n == 2); /* 30 idle within; 31, 32 far */
    /* Task 3: gap 9 against 6 / 3 moves 31 and 32; 30 (gap 1) stays. */
    assert(aoe_ai_unit_group_update_ror(&g, &ops, 3, 0, 0) == 1 && u_move_n == 2);
    /* Unknown task. */
    assert(aoe_ai_unit_group_update_ror(&g, &ops, 0x16, 0, 0) == 0);
}

int main(void) {
    static AoeAiUnitGroupRor g, h;
    AoeAiUnitGroupOpsRor ops = {NULL, lead, follow, unfollow, release, now,
                                present, hp, dismiss, dist, unit_ai, sn};
    static uint8_t buf[2048];
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;

    aoe_ai_unit_group_init_ror(&g);
    assert(g.id == -1 && g.commander == -1 && g.members[39] == -1 &&
           g.loc_1ac[2] == -1.0f && g.list_2d0[19] == -1 && g.count == 0);

    assert(aoe_ai_unit_group_add_ror(&g, 10, &ops) && g.added_time == 777);
    assert(aoe_ai_unit_group_add_ror(&g, 11, &ops));
    assert(aoe_ai_unit_group_add_ror(&g, 12, &ops) && follows == 0);
    aoe_ai_unit_group_set_commander_ror(&g, 10, &ops);
    assert(lead_by == 10 && led_n == 3 && led[2] == 12);
    assert(aoe_ai_unit_group_add_ror(&g, 13, &ops) && follows == 1 && g.count == 4);

    /* Slots keep holes; the n-th lookups skip them. */
    assert(aoe_ai_unit_group_remove_ror(&g, 11, &ops) && unfollows == 1);
    assert(g.members[1] == -1 && aoe_ai_unit_group_member_ror(&g, 1) == 12 &&
           aoe_ai_unit_group_member_ror(&g, 2) == 13 &&
           aoe_ai_unit_group_member_ror(&g, 3) == -1 &&
           aoe_ai_unit_group_value_ror(&g, 0) == -1);
    assert(aoe_ai_unit_group_contains_ror(&g, 13) && !aoe_ai_unit_group_contains_ror(&g, 11));
    assert(!aoe_ai_unit_group_remove_ror(&g, 99, &ops));
    /* A re-add fills the first hole. */
    assert(aoe_ai_unit_group_add_ror(&g, 14, &ops) && g.members[1] == 14);

    /* Hit points truncate per member; a missing object adds nothing. */
    assert(aoe_ai_unit_group_hit_points_ror(&g, &ops) == 10 + 14 + 13);

    /* Pruning drops unit 12 and dismisses it. */
    assert(aoe_ai_unit_group_prune_ror(&g, &ops) && dismissed == 12 &&
           g.count == 3 && unfollows == 2);
    /* Removing the commander releases it. */
    assert(aoe_ai_unit_group_remove_slot_ror(&g, 0, &ops) && released == 10 &&
           g.commander == -1 && g.count == 2);
    assert(!aoe_ai_unit_group_remove_slot_ror(&g, 40, &ops));

    aoe_ai_unit_group_set_all_locations_ror(&g, 1, 2, 3);
    assert(g.location[0] == 1 && g.loc_19c[1] == 2 && g.loc_1ac[2] == 3 &&
           g.loc_18c[0] == -1.0f);

    /* Save writes the newest layout: 8 + 320 + 20 + 16 + 12 + 48 + 16 + 12
       + 2 + 240 + 4 + 1 + 4 + 80 + 8 + 4 + 4 bytes. */
    g.flag_2c8 = 1;
    g.field_328 = 5;
    g.block_1d8[0xef] = 0xab;
    assert(aoe_ai_unit_group_save_ror(&g, &out));
    assert(out.pos == 799);
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_ai_unit_group_load_ror(&h, 7.2f, &in) && in.pos == out.pos);
    assert(h.members[1] == 14 && h.count == 2 && h.block_1d8[0xef] == 0xab &&
           h.flag_2c8 == 1 && h.field_328 == 5 && h.added_time == 777 &&
           h.loc_19c[1] == 2.0f);
    /* An older save stops before the version-gated tail. */
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    aoe_ai_unit_group_init_ror(&h);
    assert(aoe_ai_unit_group_load_ror(&h, 6.9f, &in) && in.pos == 698 &&
           h.flag_2c8 == 0 && h.field_328 == 0);

    aoe_ai_unit_group_clear_ror(&g, &ops);
    assert(g.count == 0 && g.members[1] == -1);

    /* Range: a missing member is dropped; distance less 1.0 (0.25 gives
       0) against SN 41, or 23 for task 0x6d. */
    aoe_ai_unit_group_init_ror(&g);
    aoe_ai_unit_group_set_all_locations_ror(&g, 5, 6, 7);
    aoe_ai_unit_group_add_ror(&g, 20, &ops);
    aoe_ai_unit_group_add_ror(&g, 21, &ops);
    aoe_ai_unit_group_add_ror(&g, 4, &ops);
    assert(aoe_ai_unit_group_in_range_ror(&g, &ops) && sn_asked == 41 &&
           g.count == 2 && g.members[0] == -1);
    aoe_ai_unit_group_add_ror(&g, 5, &ops);
    assert(!aoe_ai_unit_group_in_range_ror(&g, &ops));
    g.task = 0x6d;
    assert(aoe_ai_unit_group_in_range_ror(&g, &ops) && sn_asked == 23);

    /* Settled: type 0x7d skipped, task -1 fine, action 0x2bd fine, task
       600 skipped only when allowed for group tasks 100 / 0x67. */
    aoe_ai_unit_group_init_ror(&g);
    aoe_ai_unit_group_add_ror(&g, 22, &ops);
    aoe_ai_unit_group_add_ror(&g, 24, &ops);
    aoe_ai_unit_group_add_ror(&g, 25, &ops);
    aoe_ai_unit_group_add_ror(&g, 20, &ops);
    assert(aoe_ai_unit_group_settled_ror(&g, &ops, 0) && g.count == 3);
    aoe_ai_unit_group_add_ror(&g, 23, &ops);
    g.task = 100;
    assert(!aoe_ai_unit_group_settled_ror(&g, &ops, 0));
    assert(aoe_ai_unit_group_settled_ror(&g, &ops, 1));
    g.task = 101;
    assert(!aoe_ai_unit_group_settled_ror(&g, &ops, 1));

    /* Waypoints and the +0x2d0 list. */
    {
        float w[3];
        int i;
        for (i = 0; i < 16; ++i) aoe_ai_unit_group_add_waypoint_ror(&g, (float)i, 2.0f);
        assert(g.flag_1d4 == 14 && aoe_ai_unit_group_waypoint_ror(&g, 13, w) &&
               w[0] == 13.0f && w[1] == 2.0f && w[2] == 1.0f);
        assert(g.block_1d8[16 + 2] == 0x80 && g.block_1d8[16 + 3] == 0x3f);
        for (i = 0; i < 25; ++i) aoe_ai_unit_group_list_add_ror(&g, 100 + i);
        assert(g.field_2cc == 19 && aoe_ai_unit_group_list_ror(&g, 18) == 118 &&
               aoe_ai_unit_group_list_ror(&g, 19) == -1 &&
               aoe_ai_unit_group_list_ror(&g, 20) == -1);
        aoe_ai_unit_group_set_field_1cc_ror(&g, 9);
        assert(aoe_ai_unit_group_field_1cc_ror(&g) == 9);
    }
    update_tests();
    return 0;
}
