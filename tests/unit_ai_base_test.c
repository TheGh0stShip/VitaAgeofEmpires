/* Patched RoR 1.0a unit AI base orders (0x0040f6a0..0x00413190). */
#include "unit_ai_base.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int object(void *c, int32_t id, AoeUnitAiObjectRor *o) {
    (void)c;
    if (id < 100) return 0;
    o->type = (int16_t)(id / 10);
    o->x = (float)id;
    o->y = 2.0f;
    o->z = 3.0f;
    return 1;
}
static int last_slot, target_result = 1, point_result = 1, stops, self_stops;
static float px, py, pz;
static int unit_target(void *c, int s, int32_t id, int32_t e, int f) { (void)c; (void)id; (void)e; (void)f; last_slot = s; return target_result; }
static int unit_point(void *c, int s, float x, float y, float z, int32_t e, int f) { (void)c; (void)e; (void)f; last_slot = s; px = x; py = y; pz = z; return point_result; }
static float range(void *c) { (void)c; return 5.0f; }
static int in_range = 1;
static int in_range_f(void *c, int32_t id, float r) { (void)c; (void)id; assert(r == 5.0f); return in_range; }
static int free_after = 3, free_calls;
static int free_tile(void *c, float x, float y) { (void)c; (void)x; (void)y; return ++free_calls >= free_after; }
static float speed(void *c) { (void)c; return 2.0f; }
static void ustop(void *c) { (void)c; ++stops; }
static int can_act = 1;
static int can(void *c, const AoeUnitAiObjectRor *o, int32_t id) { (void)c; (void)o; (void)id; return can_act; }
static void sstop(void *c, int f) { (void)c; (void)f; ++self_stops; }
static int seq;
static int rnd(void *c) { (void)c; return seq++; }
static uint32_t vis(void *c, int32_t x, int32_t y) { (void)c; return (uint32_t)(x + y); }

int main(void) {
    static AoeUnitAiRor ai;
    AoeUnitAiOpsRor ops = {0};
    ops.object = object;
    ops.unit_id = 55;
    ops.unit_type = 4;
    ops.unit_x = 10.0f;
    ops.unit_y = 10.0f;
    ops.unit_z = 1.0f;
    ops.explored_mask = 1;
    ops.visible_mask = 2;
    ops.visibility = vis;
    ops.map_w = 20;
    ops.map_h = 20;
    ops.unit_target = unit_target;
    ops.unit_point = unit_point;
    ops.unit_range = range;
    ops.unit_in_range = in_range_f;
    ops.unit_free_tile = free_tile;
    ops.unit_speed = speed;
    ops.unit_stop = ustop;
    ops.can_act_on = can;
    ops.self_stop = sstop;
    ops.rand = rnd;
    ai.order = -1;
    ai.action = -1;

    /* Bookkeeping. */
    aoe_unit_ai_set_order_ror(&ai, 7);
    assert(ai.order == 7 && ai.last_order == -1);
    aoe_unit_ai_set_action_ror(&ai, 9);
    assert(ai.action == 9 && ai.last_action == -1);
    aoe_unit_ai_set_target_ror(&ai, &ops, 150, 1, 2, 3);
    assert(ai.target_id == 150 && ai.target_type == 15 && ai.target_z == 3.0f);
    aoe_unit_ai_set_target_ror(&ai, &ops, 5, 1, 2, 3);
    assert(ai.target_id == 5 && ai.target_type == 15 && ai.last_target_id == 150); /* type kept */
    aoe_unit_ai_clear_target_ror(&ai);
    assert(ai.target_id == -1 && ai.target_type == -1 && ai.last_target_type == 15 && ai.target_x == -1.0f);
    /* Stop: forced keeps an unrelated action. */
    ai.order = 600;
    ai.action = 9;
    aoe_unit_ai_stop_ror(&ai, &ops, 1);
    assert(ai.order == -1 && ai.last_order == 600 && ai.action == 9 && stops == 1);
    ai.order = 600;
    ai.action = 700;
    aoe_unit_ai_stop_ror(&ai, &ops, 1);
    assert(ai.action == -1 && ai.last_action == 700);

    /* Waypoints: eight kept. */
    for (int i = 0; i < 9; ++i) aoe_unit_ai_add_waypoint_ror(&ai, i, i * 2);
    assert(ai.waypoint_next == 1 && ai.waypoint_x[0] == 8.0f && aoe_unit_ai_has_waypoint_ror(&ai, 8, 16));
    assert(!aoe_unit_ai_has_waypoint_ror(&ai, 3, 6)); /* past next is not searched */

    /* Attack. */
    ai.order = -1;
    assert(aoe_unit_ai_attack_ror(&ai, &ops, 120, 0) && ai.order == 600 && last_slot == 0x150 && ai.target_x == 120.0f);
    assert(!aoe_unit_ai_attack_ror(&ai, &ops, 130, 0)); /* busy */
    in_range = 0;
    assert(!aoe_unit_ai_attack_new_ror(&ai, &ops, 130));
    in_range = 1;
    assert(aoe_unit_ai_attack_new_ror(&ai, &ops, 130) && ai.target_id == 130);
    assert(!aoe_unit_ai_attack_new_ror(&ai, &ops, 130)); /* same target */
    /* Slot orders. */
    assert(aoe_unit_ai_order_target_ror(&ai, &ops, 0x164, 140, 0) && ai.order == 0x265 && self_stops == 1);
    assert(!aoe_unit_ai_order_target_ror(&ai, &ops, 0x164, 140, 0));
    assert(aoe_unit_ai_order_target_ror(&ai, &ops, 0x88, 7, 0) && ai.order == 0x25b && ai.target_type == -1 && ai.target_x == 0.0f);
    target_result = 0;
    assert(!aoe_unit_ai_order_target_ror(&ai, &ops, 0x170, 140, 1));
    target_result = 1;
    /* Point orders. */
    ai.order = -1;
    assert(aoe_unit_ai_order_xy_ror(&ai, &ops, 0x25d, 4, 6, 0) && px == 4.0f && ai.target_z == 1.0f && ai.order == 0x25d);
    assert(aoe_unit_ai_order_id_xy_ror(&ai, &ops, 150, 4, 6, 1) && ai.order == 0x26e && ai.target_id == 150 && ai.target_y == 6.0f);
    assert(aoe_unit_ai_order_266_ror(&ai, &ops, 1, 2, 3, 1) && ai.order == 0x266 && ai.target_id == -1);
    assert(aoe_unit_ai_order_26d_ror(&ai, &ops, 1, 2, 3) && ai.target_id == 55 && ai.target_type == 4);
    ai.order = -1;
    assert(aoe_unit_ai_move_to_point_ror(&ai, &ops, 5, 6, 7, 0, 0) && ai.order == 0x262);
    assert(aoe_unit_ai_move_to_object_ror(&ai, &ops, 0x15c, 9, 0, 1) && ai.target_x == 0.0f);
    /* Move near: clamped, jittered until the tile is free (third try). */
    seq = 0;
    free_calls = 0;
    assert(aoe_unit_ai_move_near_ror(&ai, &ops, 30.0f, -2.0f, 0, 1) && free_calls == 3);
    assert(px >= 0.0f && px <= 19.0f && py >= 0.0f && py <= 19.0f);
    /* Follow. */
    ai.order = -1;
    assert(aoe_unit_ai_follow_ror(&ai, &ops, 0x259, 150, 4, 0) && ai.field_4c == 150 && ai.field_48 == 4);
    assert(aoe_unit_ai_follow_ror(&ai, &ops, 0x264, 160, 5, 1) && ai.order == 0x264 && ai.field_4c == 150);
    assert(aoe_unit_ai_follow_point_ror(&ai, &ops, 3, 4, 6, 1) && ai.field_4c == -1 && ai.target_z == 1.0f);
    /* Move away from the group at (14, 10): toward x = 10 - 1 * 2 * 4 = 2. */
    {
        int32_t members[1] = {140};
        ai.group = members;
        ai.group_count = 1;
        ops.unit_x = 10.0f;
        free_after = 1;
        free_calls = 0;
        ops.object = object;
        assert(aoe_unit_ai_move_away_ror(&ai, &ops, 1));
        /* member at (140, 2): dx 130, dy -8. */
        assert(px < 10.0f && py > 10.0f);
    }
    /* Lists, view, timer. */
    {
        int32_t cls[2] = {4, 9}, ids[2] = {7, 8};
        ai.classes = cls;
        ai.class_count = 2;
        ai.table_ids = ids;
        ai.table_count = 2;
        assert(aoe_unit_ai_class_allowed_ror(&ai, 9) && !aoe_unit_ai_class_allowed_ror(&ai, 5));
        assert(aoe_unit_ai_table_has_ror(&ai, 8) && !aoe_unit_ai_table_has_ror(&ai, 9));
    }
    assert(aoe_unit_ai_tile_view_ror(&ops, 1, 1) == 0xf && aoe_unit_ai_tile_view_ror(&ops, 1, 0) == 0x80 &&
           aoe_unit_ai_tile_view_ror(&ops, 4, 0) == 0);
    ai.timer_base = 4000;
    seq = 2500;
    aoe_unit_ai_reset_timer_ror(&ai, &ops);
    assert(ai.timer == 2500 % 2000 + 3000);
    puts("unit ai base ok");
    {
        /* 0x00414fc0 / 0x00414f40. */
        int32_t *ids = malloc(2 * sizeof *ids), cap = 2;
        AoeUnitAiOpsRor none = {0};
        AoeUnitAiObjectRor o;
        ids[0] = 7; ids[1] = 9;
        assert(aoe_unit_ai_resize_ids_ror(&ids, &cap, 5) && cap == 5);
        assert(ids[0] == 7 && ids[1] == 9);
        assert(aoe_unit_ai_resize_ids_ror(&ids, &cap, 1) && cap == 1 && ids[0] == 7);
        assert(!aoe_unit_ai_resize_ids_ror(&ids, &cap, 0) && cap == 1);
        free(ids);
        assert(!aoe_unit_ai_object_ror(&none, 3, &o));
    }
    {
        /* 0x00410930 / 0x0040ef80. */
        AoeUnitAiRor n;
        memset(&n, 0, sizeof n);
        n.group = malloc(2 * sizeof *n.group);
        n.group[0] = 5; n.group[1] = 6;
        n.group_count = 2;
        n.order = 0x262;
        assert(aoe_unit_ai_notify_filter_ror(&n, 1, 5, 699, 0) == 0);
        assert(aoe_unit_ai_notify_filter_ror(&n, 2, 6, 699, 0) == 2);
        assert(aoe_unit_ai_notify_filter_ror(&n, 2, 7, 699, 0) == 0);
        assert(aoe_unit_ai_notify_filter_ror(&n, 2, 1, 0x1fb, 0x262) == 2);
        assert(aoe_unit_ai_notify_filter_ror(&n, 2, 1, 0x1fb, 0x25a) == 0);
        assert(aoe_unit_ai_notify_filter_ror(&n, 2, 1, 0x1fa, 0x262) == 2);
        n.order = 0x258;
        assert(aoe_unit_ai_notify_filter_ror(&n, 2, 1, 0x1fa, 0x262) == 1);
        aoe_unit_ai_release_ror(&n);
        assert(!n.group && !n.group_count);
    }
    return 0;
}
