#include "action_build.h"

#include <assert.h>
#include <string.h>

static AoeLiveUnit builder, site;
static DatUnit b_t, s_t;
static DatUnitTask task;
static int moves, quiet_first, notices[4], notice_count, repairs, farms,
    resets, walls, sounds;
static uint8_t sub_result;
static int faced = 1, owner_kind_v, instant;
static float work_total, train = 10.0f;
static const void *shown;
static const char G_MOVE[] = "m", G_WORK[] = "w", G_STAND[] = "s";

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    return id == site.core.unit_id ? &site : NULL;
}
static void rel(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void att(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t ty) {
    (void)c; (void)t; (void)u;
    assert(ty == 0x65);
}
static void clear2(void *c, AoeMoveActionRor *a) { (void)c; a->secondary_id = -1; }
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v,
                        int32_t a) {
    (void)c; (void)u; (void)a;
    assert(v == 0x25a);
    if (notice_count < 4) notices[notice_count] = code;
    ++notice_count;
}
static void sel(void *c, AoeLiveUnit *u, const void *g) { (void)c; (void)u; shown = g; }
static const void *graphic(void *c, int16_t id) {
    (void)c;
    return id == 1 ? G_MOVE : id == 2 ? G_WORK : id == 9 ? G_STAND : NULL;
}
static float tick(void *c) { (void)c; return 1.0f; }
static int32_t okind(void *c, AoeLiveUnit *u) { (void)c; (void)u; return owner_kind_v; }
static int face(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; return faced; }
static int wall_at(void *c, AoeLiveUnit *u, int32_t x, int32_t y) {
    (void)c; (void)u; (void)x; (void)y;
    return walls;
}
/* A minimal 0x004ad7b0: work accumulates to the train time. */
static int add_work(void *c, AoeLiveUnit *t, float amount, int *done) {
    (void)c;
    work_total += amount;
    *done = work_total >= train;
    if (*done) {
        t->core.state = 2;
        t->core.hit_points = 100.0f; /* the real 0x004ad7b0 raises hp */
    }
    return 1;
}
static void reset(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++resets; }
static void repair(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; ++repairs; }
static void onto(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; ++farms; }
static void play(void *c, int16_t s) { (void)c; (void)s; ++sounds; }
static int start_move(void *c, AoeBuildActionRor *a, AoeLiveUnit *t,
                      float range, const void *g, int qf) {
    (void)c; (void)a; (void)t; (void)range; (void)g;
    ++moves;
    quiet_first = qf;
    return 1;
}
static uint8_t sub_update(void *c, AoeBuildActionRor *a) { (void)c; (void)a; return sub_result; }
static void sub_clear(void *c, AoeBuildActionRor *a) { (void)c; (void)a; }

int main(void) {
    AoeBuildActionOps o;
    AoeBuildActionRor b;
    uint8_t r;
    memset(&o, 0, sizeof(o));
    o.move.resolve = resolve; o.move.target_released = rel;
    o.move.target_attached = att; o.move.unit_notice = unit_notice;
    o.move.select_graphic = sel; o.move.clear_secondary = clear2;
    o.graphic = graphic; o.tick_seconds = tick; o.owner_kind = okind;
    o.face = face; o.wall_at = wall_at; o.add_build_work = add_work;
    o.reset_group = reset; o.request_repair = repair; o.order_onto = onto;
    o.play_sound = play; o.start_move = start_move; o.sub_update = sub_update;
    o.sub_clear = sub_clear;
    builder.core.unit_template = &b_t;
    builder.runtime_owner_index = 1;
    builder.core.position.x38 = 3.5f;
    builder.core.position.y3c = 3.5f;
    b_t.standing_graphic = 9;
    b_t.work_rate = 1.0f;
    site.core.unit_template = &s_t;
    site.core.unit_id = 7;
    site.runtime_owner_index = 1;
    site.core.position.x38 = 6.0f;
    site.core.position.y3c = 3.5f;
    s_t.id = 70;
    s_t.hit_points = 100;
    s_t.size[0] = s_t.size[1] = 1.0f;
    task.work_amount = 2.0f;
    task.action_range = 0.3f;
    task.approach_graphic = 1;
    task.action_graphic = 2;
    task.action_sound = 5;

    /* Another player's foundation is not taken as the target. */
    site.runtime_owner_index = 2;
    assert(aoe_build_action_init_target_ror(&b, &builder, &task, &site, &o) &&
           !b.base.target && b.base.type == 0x65);
    site.runtime_owner_index = 1;

    /* Own foundation: walk, face, build (2 per tick, 10 needed). */
    assert(aoe_build_action_init_target_ror(&b, &builder, &task, &site, &o));
    assert(aoe_build_action_activate_ror(&b, 1, &o) && b.base.state == 4 &&
           moves == 1 && !quiet_first);
    sub_result = 1;
    assert(aoe_build_action_update_ror(&b, &o, &r) && b.base.state == 6 &&
           shown == G_MOVE);
    assert(aoe_build_action_update_ror(&b, &o, &r) && b.base.state == 7 &&
           shown == G_WORK && sounds == 1);
    for (int i = 0; i < 4; ++i)
        assert(aoe_build_action_update_ror(&b, &o, &r) && b.base.state == 7);
    /* The fifth tick completes it: back to idle (state 3, notice 0x202). */
    notice_count = 0;
    assert(aoe_build_action_update_ror(&b, &o, &r) && b.base.state == 3 &&
           work_total == 10.0f && notices[0] == 0x202);
    /* Finished and intact: the target is dropped. */
    assert(aoe_build_action_update_ror(&b, &o, &r) && !b.base.target);

    /* Finished but damaged: a repair is requested instead. */
    site.core.hit_points = 50.0f;
    assert(aoe_build_action_retarget_ror(&b, &site, 0, 0, 0, &o) &&
           b.base.state == 4 && b.base.target == &site);
    assert(aoe_build_action_update_ror(&b, &o, &r) && repairs == 1 &&
           resets == 1);

    /* A farm (template 50) waits a tick, then the builder farms it. */
    s_t.id = 50;
    site.core.state = 0;
    site.core.storage.carried_amount = 250.0f;
    work_total = 0.0f;
    assert(aoe_build_action_init_target_ror(&b, &builder, &task, &site, &o));
    assert(aoe_build_action_activate_ror(&b, 1, &o));
    b.base.state = 7;
    for (int i = 0; i < 5; ++i)
        assert(aoe_build_action_update_ror(&b, &o, &r));
    assert(b.timer == -1.0f && b.base.state == 7);
    assert(aoe_build_action_update_ror(&b, &o, &r) && farms == 1);

    /* Diagonal to the foundation with walls on both sides: give up. */
    s_t.id = 70;
    site.core.state = 0;
    site.core.position.x38 = 5.0f;
    site.core.position.y3c = 5.0f;
    walls = 1;
    assert(aoe_build_action_init_target_ror(&b, &builder, &task, &site, &o));
    assert(aoe_build_action_set_state_ror(&b, 7, &o) && b.base.state == 0x0d);
    notice_count = 0;
    assert(aoe_build_action_update_ror(&b, &o, &r) && r == 3 &&
           b.base.state == 2 && notices[0] == 0x1f9);
    walls = 0;

    /* Point walks are quiet before the push; arrival idles. */
    assert(aoe_build_action_walk_ror(&b, 9, 9, 0, &o) && b.base.state == 0x0b &&
           quiet_first);
    assert(aoe_build_action_update_ror(&b, &o, &r) && b.base.state == 3);
    /* Computer owners stop instead of idling. */
    owner_kind_v = 1;
    assert(aoe_build_action_set_state_ror(&b, 3, &o) && b.base.state == 1);
    assert(aoe_build_action_update_ror(&b, &o, &r) && r == 1);
    return 0;
}
