#include "action_repair.h"

#include <assert.h>
#include <string.h>

/* Patched RoR 1.0a repair action 0x6a (vtable 0x00548924) and the unit
   +0x8c repair methods 0x004aad30 / 0x004b0780 / 0x004acb70. */

static AoeLiveUnit repairer, house, boat, other;
static DatUnit r_t, h_t, b_t, o_t;
static DatUnitTask task;
static int moves, quiet_first, notices[8], notice_count, events, sounds;
static int pays, pay_ok = 1, pop_flags[4], move_tos, faced = 1;
static float pay_mults[4], move_x, move_y;
static uint8_t sub_result, serial, other_state;
static float distance = 2.0f;
static const void *shown;
static int terrain_map[16][16];
static const char G_MOVE[] = "m", G_WORK[] = "w", G_STAND[] = "s";

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    return id == house.core.unit_id ? &house
           : id == boat.core.unit_id ? &boat : NULL;
}
static void rel(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void att(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t ty) {
    (void)c; (void)t; (void)u;
    assert(ty == 0x6a);
}
static void clear2(void *c, AoeMoveActionRor *a) { (void)c; a->secondary_id = -1; }
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v,
                        int32_t a) {
    (void)c; (void)u; (void)a;
    assert(v == 0x26a);
    if (notice_count < 8) notices[notice_count] = code;
    ++notice_count;
}
static void sel(void *c, AoeLiveUnit *u, const void *g) { (void)c; (void)u; shown = g; }
static const void *graphic(void *c, int16_t id) {
    (void)c;
    return id == 1 ? G_MOVE : id == 2 ? G_WORK : id == 9 ? G_STAND : NULL;
}
static float tick(void *c) { (void)c; return 0.5f; }
static double dist(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    (void)c; (void)u; (void)t;
    return distance;
}
static int face(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; return faced; }
static int allowed(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    (void)c;
    return u->runtime_owner_index == t->runtime_owner_index;
}
static int pay(void *c, AoeLiveUnit *u, float mult, int pop) {
    (void)c; (void)u;
    if (pays < 4) {
        pay_mults[pays] = mult;
        pop_flags[pays] = pop;
    }
    ++pays;
    return pay_ok;
}
static uint8_t repair(void *c, AoeLiveUnit *t, float amount) {
    int r = aoe_live_unit_repair_ror(t, amount, pay, c);
    assert(r >= 0);
    return (uint8_t)r;
}
static void event(void *c, uint32_t e, int16_t o, int32_t id) {
    (void)c; (void)o; (void)id;
    assert(e == 0x6b);
    ++events;
}
static void play(void *c, int16_t s) { (void)c; (void)s; ++sounds; }
static int start_move(void *c, AoeRepairActionRor *a, AoeLiveUnit *t,
                      float range, const void *g, int qf) {
    (void)c; (void)a; (void)t; (void)g;
    if (t) assert(range == r_t.max_range);
    ++moves;
    quiet_first = qf;
    return 1;
}
static uint8_t sub_update(void *c, AoeRepairActionRor *a) { (void)c; (void)a; return sub_result; }
static void sub_clear(void *c, AoeRepairActionRor *a) { (void)c; (void)a; }
static uint8_t order_serial(void *c, AoeLiveUnit *u) {
    (void)c;
    return u == &boat ? serial : 0;
}
static AoeLiveUnit *owner_unit(void *c, AoeLiveUnit *u, uint32_t i) {
    static AoeLiveUnit *list[] = {&repairer, &other, &house, &boat};
    (void)c; (void)u;
    return i < 4 ? list[i] : NULL;
}
static AoeLiveUnit *head_target(void *c, AoeLiveUnit *u) {
    (void)c;
    return u == &other ? &boat : NULL;
}
static uint8_t head_state(void *c, AoeLiveUnit *u) {
    (void)c;
    return u == &other ? other_state : u == &boat ? 0x0b : 0;
}
static void head_dest(void *c, AoeLiveUnit *u, float *x, float *y) {
    (void)c; (void)u;
    *x = 9.25f;
    *y = 8.75f;
}
static int terrain_at(void *c, int32_t x, int32_t y) {
    (void)c;
    return x >= 0 && y >= 0 && x < 16 && y < 16 ? terrain_map[y][x] : -1;
}
static int fits(void *c, AoeLiveUnit *u, float x, float y) {
    (void)c; (void)u; (void)x; (void)y;
    return 1;
}
static int move_to(void *c, AoeLiveUnit *u, float x, float y, float z,
                   float range, int force) {
    (void)c; (void)z;
    assert(u == &boat && range == 0.0f && force == 1);
    move_x = x;
    move_y = y;
    ++move_tos;
    ++serial;
    return 1;
}

static void work_math(void) {
    /* Building: H 100, train 50: amount 1 -> 2 hp for 2/200 of the cost
       (population flag 0), then the type-70 step (free for a non-boat). */
    house.core.hit_points = 50.0f;
    pays = 0;
    assert(aoe_live_unit_repair_ror(&house, 1.0f, pay, NULL) == 0);
    assert(house.core.hit_points == 52.0f && pays == 1 &&
           pay_mults[0] == 0.01f && pop_flags[0] == 0);
    /* Capped at the missing hit points; reaching the maximum reports 1. */
    house.core.hit_points = 99.0f;
    assert(aoe_live_unit_repair_ror(&house, 1.0f, pay, NULL) == 1);
    assert(house.core.hit_points == 100.0f && pay_mults[1] == 0.005f);
    /* Unpaid: 2, unchanged. */
    pay_ok = 0;
    house.core.hit_points = 40.0f;
    assert(aoe_live_unit_repair_ror(&house, 1.0f, pay, NULL) == 2 &&
           house.core.hit_points == 40.0f);
    pay_ok = 1;
    /* Boat (class 21): pays with population flag 1. */
    boat.core.hit_points = 10.0f;
    pays = 0;
    assert(aoe_live_unit_repair_ror(&boat, 3.0f, pay, NULL) == 0 &&
           pop_flags[0] == 1 && boat.core.hit_points == 16.0f);
    /* Other type-70 units repair for free; root types too. */
    b_t.unit_class = 6;
    pays = 0;
    assert(aoe_live_unit_repair_ror(&boat, 3.0f, pay, NULL) == 0 &&
           pays == 0 && boat.core.hit_points == 19.0f);
    b_t.unit_class = 21;
    /* Root 0x004aad30: a negative amount stops at 1 hp. */
    o_t.type = 30;
    o_t.hit_points = 10;
    other.core.unit_template = &o_t;
    other.core.hit_points = 3.0f;
    assert(aoe_live_unit_repair_ror(&other, -5.0f, NULL, NULL) == 0 &&
           other.core.hit_points == 1.0f);
    o_t.id = 0x9c;
}

static void line_search(const AoeRepairActionOps *ro) {
    int32_t from[2] = {2, 2}, to[2] = {10, 6}, out[2] = {-1, -1};
    AoeLineSearchOps line = {ro->move.context, ro->terrain_at, ro->fits};
    const AoeLineSearchOps *o = &line;
    memset(terrain_map, 0, sizeof(terrain_map));
    terrain_map[4][6] = 2; /* (6, 4) beach */
    /* 16 steps of (0.5, 0.25): tiles (2,2) (3,2) ... the beach at (6,4). */
    assert(aoe_unit_line_terrain_point_ror(&repairer, from, to, out, 2, 2, 1,
                                           o) == 1 &&
           out[0] == 6 && out[1] == 4);
    /* require_fit other than 1 never matches; equal tiles return 0. */
    assert(aoe_unit_line_terrain_point_ror(&repairer, from, to, out, 2, 2, 0,
                                           o) == 0);
    assert(aoe_unit_line_terrain_point_ror(&repairer, from, from, out, 2, 2,
                                           1, o) == 0);
}

int main(void) {
    AoeRepairActionOps o;
    AoeRepairActionRor a;
    uint8_t r;
    memset(&o, 0, sizeof(o));
    o.move.resolve = resolve; o.move.target_released = rel;
    o.move.target_attached = att; o.move.unit_notice = unit_notice;
    o.move.select_graphic = sel; o.move.clear_secondary = clear2;
    o.graphic = graphic; o.tick_seconds = tick; o.distance = dist;
    o.face = face; o.may_repair = allowed; o.repair = repair;
    o.event = event; o.play_sound = play; o.start_move = start_move;
    o.sub_update = sub_update; o.sub_clear = sub_clear;
    o.order_serial = order_serial; o.owner_unit_at = owner_unit;
    o.head_target = head_target; o.head_state = head_state;
    o.head_destination = head_dest; o.terrain_at = terrain_at;
    o.fits = fits; o.move_to = move_to;

    repairer.core.unit_template = &r_t;
    repairer.runtime_owner_index = 1;
    repairer.core.position.x38 = 2.5f;
    repairer.core.position.y3c = 2.5f;
    r_t.id = 0x9c;
    r_t.standing_graphic = 9;
    r_t.work_rate = 2.0f;
    r_t.max_range = 0.5f;
    h_t.type = 80;
    h_t.hit_points = 100;
    h_t.train_time = 50;
    h_t.unit_class = 3;
    house.core.unit_template = &h_t;
    house.core.unit_id = 7;
    house.core.state = 2;
    house.runtime_owner_index = 1;
    house.core.position.x38 = 5.0f;
    house.core.position.y3c = 2.5f;
    b_t.type = 70;
    b_t.hit_points = 60;
    b_t.train_time = 30;
    b_t.unit_class = 21;
    boat.core.unit_template = &b_t;
    boat.core.unit_id = 8;
    boat.core.state = 2;
    boat.runtime_owner_index = 1;
    boat.core.position.x38 = 10.5f;
    boat.core.position.y3c = 6.5f;
    task.work_value1 = 1.0f;
    task.moving_graphic = 1;
    task.proceeding_graphic = 2;
    task.gathering_sound = 5;

    work_math();
    line_search(&o);

    /* A building: walk (template max range, quiet after the push), face,
       repair at tick 0.5 x work rate 2 x work value 1 = 1 per update. */
    house.core.hit_points = 97.0f;
    assert(aoe_repair_action_init_target_ror(&a, &repairer, &task, &house,
                                             &o) &&
           a.base.type == 0x6a && a.base.target == &house &&
           a.base.x == 5.0f && move_tos == 0);
    assert(aoe_repair_action_activate_ror(&a, 1, &o) && a.base.state == 4 &&
           moves == 1 && !quiet_first);
    /* Arrived but not within dist - 2.0 <= 0.4: wait for it (state 10). */
    sub_result = 1;
    distance = 2.5;
    assert(aoe_repair_action_update_ror(&a, &o, &r) && a.base.state == 0x0a &&
           a.timer == 1.0f && shown == G_STAND);
    assert(aoe_repair_action_update_ror(&a, &o, &r) && a.base.state == 0x0a &&
           a.timer == 0.5f);
    distance = 2.375f;
    assert(aoe_repair_action_update_ror(&a, &o, &r) && a.base.state == 6);
    assert(aoe_repair_action_update_ror(&a, &o, &r) && a.base.state == 7 &&
           shown == G_WORK && sounds == 1);
    pays = 0;
    assert(aoe_repair_action_update_ror(&a, &o, &r) && r == 0 &&
           house.core.hit_points == 99.0f && pays == 1);
    /* Full: state 1, notice 0x1fa, the target released; then done. */
    notice_count = 0;
    assert(aoe_repair_action_update_ror(&a, &o, &r) &&
           house.core.hit_points == 100.0f && a.base.state == 1 &&
           !a.base.target && notices[0] == 0x1fa);
    assert(aoe_repair_action_update_ror(&a, &o, &r) && r == 1);

    /* Unpaid: notice 0x1fb, event 0x6b, state 2. */
    house.core.hit_points = 50.0f;
    assert(aoe_repair_action_retarget_ror(&a, &house, 0, 0, 0, &o) &&
           a.base.state == 4);
    assert(aoe_repair_action_set_state_ror(&a, 7, &o));
    pay_ok = 0;
    notice_count = 0;
    assert(aoe_repair_action_update_ror(&a, &o, &r) && a.base.state == 2 &&
           notices[0] == 0x1fb && events == 1);
    pay_ok = 1;
    /* The target moved away (beyond reach): idle (state 3, 0x202). */
    assert(aoe_repair_action_set_state_ror(&a, 7, &o));
    house.core.position.x38 = 6.0f;
    distance = 3.0;
    notice_count = 0;
    assert(aoe_repair_action_update_ror(&a, &o, &r) && a.base.state == 3 &&
           notices[0] == 0x202);
    /* A target of another player is refused: state 1. */
    assert(aoe_repair_action_retarget_ror(&a, &house, 0, 0, 0, &o));
    house.runtime_owner_index = 2;
    assert(aoe_repair_action_update_ror(&a, &o, &r) && a.base.state == 1 &&
           !a.base.target);
    house.runtime_owner_index = 1;

    /* A boat: the beach tile on the line is the meeting point; the boat is
       ordered there and its order counter remembered. */
    other_state = 2; /* the other repairer is idle on it */
    assert(aoe_repair_action_init_target_ror(&a, &repairer, &task, &boat,
                                             &o));
    assert(move_tos == 1 && move_x == 6.5f && move_y == 4.5f &&
           a.base.x == 6.5f && a.base.y == 4.5f && a.boat_order == serial);
    /* State 10 calls the boat to the repairer while its counter matches. */
    assert(aoe_repair_action_set_state_ror(&a, 0x0a, &o) && move_tos == 2 &&
           move_x == 2.5f && move_y == 2.5f);
    assert(aoe_repair_action_set_state_ror(&a, 0x0a, &o) && move_tos == 2);
    /* Another repairer already working the boat: its walk destination. */
    other_state = 7;
    assert(aoe_repair_action_init_target_ror(&a, &repairer, &task, &boat,
                                             &o));
    assert(move_tos == 2 && a.base.x == 9.25f && a.base.y == 8.75f);

    /* A point: walk there quietly first; arrival finishes (state 1). */
    assert(aoe_repair_action_init_point_ror(&a, &repairer, &task, 4.0f, 4.0f,
                                            0.0f));
    moves = 0;
    assert(aoe_repair_action_walk_ror(&a, 4.0f, 4.0f, 0.0f, &o) &&
           a.base.state == 0x0b && moves == 1 && quiet_first);
    sub_result = 3;
    assert(aoe_repair_action_update_ror(&a, &o, &r) && a.base.state == 1);
    return 0;
}
