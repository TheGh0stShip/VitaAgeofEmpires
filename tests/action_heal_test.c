#include "action_heal.h"
#include "action_repair.h"

#include <assert.h>
#include <string.h>

/* Patched RoR 1.0a heal action 0x69 (vtable 0x005487e8). */

static AoeLiveUnit priest, man, boat;
static DatUnit p_t, m_t, b_t;
static DatUnitTask task;
static int moves, quiet_first, notices[8], notice_count, sounds, ally = 1;
static uint8_t sub_result;
static double distance = 1.0;
static float rate;
static const void *shown;
static const char G_MOVE[] = "m", G_WORK[] = "w", G_STAND[] = "s";

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    return id == man.core.unit_id ? &man : id == boat.core.unit_id ? &boat
                                                                    : NULL;
}
static void rel(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void att(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t ty) {
    (void)c; (void)t; (void)u;
    assert(ty == 0x69);
}
static void clear2(void *c, AoeMoveActionRor *a) { (void)c; a->secondary_id = -1; }
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v,
                        int32_t a) {
    (void)c; (void)u; (void)a;
    assert(v == 0x25b);
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
static int face(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; return 1; }
static int allowed(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; return ally; }
static uint8_t heal(void *c, AoeLiveUnit *t, float amount) {
    int r = aoe_live_unit_repair_ror(t, amount, NULL, c);
    assert(r >= 0);
    return (uint8_t)r;
}
static float heal_rate(void *c, AoeLiveUnit *u) { (void)c; (void)u; return rate; }
static void play(void *c, int16_t s) { (void)c; (void)s; ++sounds; }
static int start_move(void *c, AoeMoveActionRor *a, AoeLiveUnit *t,
                      float range, const void *g, int qf) {
    (void)c; (void)a; (void)g;
    if (t) assert(range == 0.4f);
    ++moves;
    quiet_first = qf;
    return 1;
}
static uint8_t sub_update(void *c, AoeMoveActionRor *a) { (void)c; (void)a; return sub_result; }
static void sub_clear(void *c, AoeMoveActionRor *a) { (void)c; (void)a; }

int main(void) {
    AoeHealActionOps o;
    AoeHealActionRor a;
    uint8_t r;
    int accepted;
    memset(&o, 0, sizeof(o));
    o.move.resolve = resolve; o.move.target_released = rel;
    o.move.target_attached = att; o.move.unit_notice = unit_notice;
    o.move.select_graphic = sel; o.move.clear_secondary = clear2;
    o.graphic = graphic; o.tick_seconds = tick; o.distance = dist;
    o.face = face; o.allowed = allowed; o.heal = heal;
    o.heal_rate = heal_rate; o.play_sound = play;
    o.start_move = start_move; o.sub_update = sub_update;
    o.sub_clear = sub_clear;
    priest.core.unit_template = &p_t;
    p_t.standing_graphic = 9;
    p_t.work_rate = 2.0f;
    m_t.type = 70;
    m_t.unit_class = 6;
    m_t.hit_points = 25;
    man.core.unit_template = &m_t;
    man.core.unit_id = 4;
    man.core.state = 2;
    man.core.hit_points = 20.0f;
    man.core.position.x38 = 3.0f;
    b_t.type = 70;
    b_t.unit_class = 2;
    b_t.hit_points = 25;
    boat.core.unit_template = &b_t;
    boat.core.unit_id = 5;
    boat.core.state = 2;
    task.work_value1 = 1.0f;
    task.work_range = 2.0f;
    task.moving_graphic = 1;
    task.proceeding_graphic = 2;
    task.gathering_sound = -1;

    /* Walk (range 0.4, quiet after push), face, heal 1 per update; resource
       56 at or below 0.1 is ignored. */
    assert(aoe_heal_action_init_target_ror(&a, &priest, &task, &man, &o.move));
    assert(a.base.type == 0x69 && a.base.x == 3.0f);
    assert(aoe_heal_action_activate_ror(&a, &o) && a.base.state == 4 &&
           moves == 1 && !quiet_first);
    sub_result = 2;
    assert(aoe_heal_action_update_ror(&a, &o, &r) && a.base.state == 6);
    assert(aoe_heal_action_update_ror(&a, &o, &r) && a.base.state == 7 &&
           shown == G_WORK && sounds == 0);
    rate = 0.1f;
    assert(aoe_heal_action_update_ror(&a, &o, &r) && r == 0 &&
           man.core.hit_points == 21.0f);
    /* Resource 56 above 0.1 multiplies: 1 x 3. */
    rate = 3.0f;
    assert(aoe_heal_action_update_ror(&a, &o, &r) &&
           man.core.hit_points == 24.0f);
    /* Full: idle (state 3, notice 0x202). */
    notice_count = 0;
    assert(aoe_heal_action_update_ror(&a, &o, &r) &&
           man.core.hit_points == 25.0f && a.base.state == 3 &&
           notices[0] == 0x202);
    /* The same target in 4/6/7 is kept; a boat target does not activate. */
    assert(aoe_heal_action_set_state_ror(&a, 7, &o));
    assert(aoe_heal_action_retarget_ror(&a, &man, &o, &accepted) &&
           accepted && a.base.state == 7);
    assert(aoe_heal_action_retarget_ror(&a, &boat, &o, &accepted) &&
           accepted && a.base.state == 7 && a.base.target == &boat);
    assert(aoe_heal_action_retarget_ror(&a, NULL, &o, &accepted) &&
           !accepted);
    /* A target that turned unfriendly: state 1, notice 0x1fb, result 4. */
    assert(aoe_heal_action_retarget_ror(&a, &man, &o, &accepted) &&
           a.base.state == 4);
    ally = 0;
    notice_count = 0;
    assert(aoe_heal_action_update_ror(&a, &o, &r) && r == 4 &&
           a.base.state == 1 && notices[0] == 0x1fb);
    ally = 1;
    /* A target that moved: followed within the work range, else idle. */
    assert(aoe_heal_action_set_state_ror(&a, 7, &o));
    man.core.position.x38 = 4.0f;
    distance = 1.5;
    assert(aoe_heal_action_update_ror(&a, &o, &r) && a.base.state == 7 &&
           a.base.x == 4.0f);
    man.core.position.x38 = 5.0f;
    distance = 2.5;
    assert(aoe_heal_action_update_ror(&a, &o, &r) && a.base.state == 3);
    /* A dead target idles; the sub-walk failing (3) fails the action. */
    assert(aoe_heal_action_retarget_ror(&a, &man, &o, &accepted));
    sub_result = 3;
    assert(aoe_heal_action_update_ror(&a, &o, &r) && a.base.state == 0x0d);
    notice_count = 0;
    assert(aoe_heal_action_update_ror(&a, &o, &r) && r == 3 &&
           a.base.state == 2 && notices[0] == 0x1f9);
    man.core.state = 3;
    assert(aoe_heal_action_update_ror(&a, &o, &r) && a.base.state == 3 &&
           !a.base.target);
    return 0;
}
