#include "action_trade.h"

#include <assert.h>
#include <math.h>
#include <string.h>

/* Patched RoR 1.0a trade action 0x6f (vtable 0x005489f0). */

static AoeLiveUnit boat, dock, home;
static DatUnit b_t, d_t, h_t;
static DatUnitTask task;
static float own_res[16], their_res[16];
static uint8_t sub_result, civ;
static int notices[8], notice_count;

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    return id == dock.core.unit_id ? &dock
           : id == home.core.unit_id ? &home : NULL;
}
static void rel(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void att(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t ty) {
    (void)c; (void)t; (void)u;
    assert(ty == 0x6f);
}
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v,
                        int32_t a) {
    (void)c; (void)u; (void)a;
    assert(v == 0x267);
    if (notice_count < 8) notices[notice_count] = code;
    ++notice_count;
}
static void sel(void *c, AoeLiveUnit *u, const void *g) { (void)c; (void)u; (void)g; }
static const void *graphic(void *c, int16_t id) { (void)c; (void)id; return "g"; }
static float tick(void *c) { (void)c; return 0.25f; }
static int face(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; return 1; }
static float *res_of(AoeLiveUnit *u) {
    return u->runtime_owner_index == 1 ? own_res : their_res;
}
static float resource(void *c, AoeLiveUnit *u, int16_t type) {
    (void)c;
    return res_of(u)[type];
}
static void store(void *c, AoeLiveUnit *u, int16_t type, float v) {
    (void)c;
    res_of(u)[type] = v;
}
static void add(void *c, AoeLiveUnit *u, int16_t type, float v) {
    (void)c;
    res_of(u)[type] += v;
}
static uint8_t civ_of(void *c, AoeLiveUnit *u) { (void)c; (void)u; return civ; }
static AoeLiveUnit *find(void *c, AoeLiveUnit *u, int16_t a, int16_t b,
                         float x, float y, AoeLiveUnit *z) {
    (void)c; (void)u; (void)b; (void)x; (void)y;
    assert(a == 45 && z == NULL); /* boats (class 2/21): no zone match */
    return &home;
}
static void play(void *c, int16_t s) { (void)c; (void)s; }
static int start_move(void *c, AoeMoveActionRor *a, AoeLiveUnit *t,
                      float range, const void *g, int qf) {
    (void)c; (void)a; (void)t; (void)range; (void)g; (void)qf;
    return 1;
}
static uint8_t sub_update(void *c, AoeMoveActionRor *a) { (void)c; (void)a; return sub_result; }
static void sub_clear(void *c, AoeMoveActionRor *a) { (void)c; (void)a; }

int main(void) {
    AoeTradeActionOps o;
    AoeTradeActionRor a;
    uint8_t r;
    memset(&o, 0, sizeof(o));
    o.move.resolve = resolve; o.move.target_released = rel;
    o.move.target_attached = att; o.move.unit_notice = unit_notice;
    o.move.select_graphic = sel;
    o.graphic = graphic; o.tick_seconds = tick; o.face = face;
    o.resource = resource; o.resource_store = store; o.resource_add = add;
    o.civ = civ_of; o.find_home = find; o.play_sound = play;
    o.start_move = start_move; o.sub_update = sub_update;
    o.sub_clear = sub_clear;
    b_t.unit_class = 2;
    b_t.resource_capacity = 20;
    b_t.drop_sites[0] = 45;
    b_t.drop_sites[1] = -1;
    boat.core.unit_template = &b_t;
    boat.runtime_owner_index = 1;
    d_t.id = 45;
    dock.core.unit_template = &d_t;
    dock.core.unit_id = 10;
    dock.core.state = 2;
    dock.runtime_owner_index = 2;
    dock.core.position.x38 = 40.0f;
    dock.core.position.y3c = 10.0f;
    h_t.id = 45;
    home.core.unit_template = &h_t;
    home.core.unit_id = 11;
    home.core.state = 2;
    home.runtime_owner_index = 1;
    home.core.position.x38 = 10.0f;
    home.core.position.y3c = 10.0f;
    task.action_range = 1.0f;
    task.carry_resource = -1;
    task.action_sound = task.deposit_sound = -1;

    /* Carried type 7 becomes wood (1); sail, face (a dock, template 45,
       counts even when empty), load. */
    boat.core.storage.carried_resource_type = 7;
    own_res[1] = 12.0f;
    their_res[9] = 100.0f;
    assert(aoe_trade_action_init_target_ror(&a, &boat, &task, &dock,
                                            &o.move));
    assert(aoe_trade_action_activate_ror(&a, 1, &o) && a.base.state == 4 &&
           boat.core.storage.carried_resource_type == 1);
    sub_result = 2;
    assert(aoe_trade_action_update_ror(&a, &o, &r) && a.base.state == 6);
    assert(aoe_trade_action_update_ror(&a, &o, &r) && a.base.state == 7 &&
           a.origin_x == 40.0f && a.origin_y == 10.0f);
    /* Load: min(capacity 20 - 0, wood 12, goods 100) = 12. */
    assert(aoe_trade_action_update_ror(&a, &o, &r));
    assert(boat.core.storage.carried_amount == 12.0f && own_res[1] == 0.0f &&
           their_res[9] == 88.0f && a.base.state == 7);
    /* Nothing more to pay with: still loading (goods remain). */
    assert(aoe_trade_action_update_ror(&a, &o, &r) && a.base.state == 7);
    /* Wood to fill up: full at 20 -> home (8). */
    own_res[1] = 50.0f;
    assert(aoe_trade_action_update_ror(&a, &o, &r) &&
           boat.core.storage.carried_amount == 20.0f && a.base.state == 8 &&
           a.home == &home);
    /* Paid at home: d^2 = 900 -> 900 x 0.000251256 x 0.7 = 0.15829 ->
       raised to 0.35 -> 7 gold; then back to the dock (4). */
    assert(aoe_trade_action_update_ror(&a, &o, &r) && a.base.state == 4);
    assert(fabsf(own_res[3] - 7.0f) < 1e-4f &&
           boat.core.storage.carried_amount == 0.0f);
    /* A far dock: 60 tiles -> 0.63314; Palmyra doubles -> 1.26627 x 20 =
       25.3 gold. */
    dock.core.position.x38 = 70.0f;
    a.origin_x = 70.0f;
    civ = 0x0f;
    boat.core.storage.carried_amount = 20.0f;
    own_res[3] = 0.0f;
    assert(aoe_trade_action_set_state_ror(&a, 8, &o));
    assert(aoe_trade_action_update_ror(&a, &o, &r));
    {
        double rate = 3600.0 * (double)0.00025125627871602774f *
                      (double)0.699999988079071f * 2.0;
        assert(fabsf(own_res[3] - (float)(rate * 20.0)) < 1e-4f);
    }
    /* Capped at 75. */
    dock.core.position.x38 = 200.0f;
    a.origin_x = 200.0f;
    boat.core.storage.carried_amount = 20.0f;
    own_res[3] = 0.0f;
    assert(aoe_trade_action_set_state_ror(&a, 8, &o));
    assert(aoe_trade_action_update_ror(&a, &o, &r) && own_res[3] == 75.0f);
    /* The dock changed hands to the boat's owner: done, result 4. */
    assert(aoe_trade_action_set_state_ror(&a, 4, &o));
    dock.runtime_owner_index = 1;
    notice_count = 0;
    assert(aoe_trade_action_update_ror(&a, &o, &r) && r == 4 &&
           a.base.state == 1 && notices[0] == 0x1fb && !a.base.target);
    /* A failed walk waits 0.5 s, then retries. */
    dock.runtime_owner_index = 2;
    assert(aoe_trade_action_retarget_ror(&a, &dock, 0, 0, 0, &o) &&
           a.base.state == 4);
    sub_result = 3;
    assert(aoe_trade_action_update_ror(&a, &o, &r) && a.timer == -0.5f);
    assert(aoe_trade_action_update_ror(&a, &o, &r) && a.timer == -0.25f);
    assert(aoe_trade_action_update_ror(&a, &o, &r) && a.timer == 0.0f &&
           a.base.state == 4);
    return 0;
}
