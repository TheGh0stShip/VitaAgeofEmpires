#include "action_base.h"

#include <assert.h>
#include <string.h>

/* Patched RoR 1.0a fly (10) and explore (4) actions. */
static AoeLiveUnit bird;
static DatUnit bird_t;
static DatUnitTask task;
static uint16_t rnd_v;
static uint8_t sub_result;
static int moves, quiet_seen = -2, notices[4], notice_count;
static float placed_z, last_range;
static const void *shown;
static const char G_FLY[] = "f", G_GLIDE[] = "g", G_STAND[] = "s", G_CUR[] = "c";

static const void *graphic(void *c, int16_t id) {
    (void)c;
    return id == 1 ? G_FLY : id == 2 ? G_GLIDE : id == 9 ? G_STAND : NULL;
}
static void sel(void *c, AoeLiveUnit *u, const void *g) { (void)c; (void)u; shown = g; }
static float tick(void *c) { (void)c; return 1.0f; }
static uint16_t rnd(void *c) { (void)c; return rnd_v; }
static int32_t mw(void *c) { (void)c; return 100; }
static int start_move(void *c, AoeMoveActionRor *a, float range,
                      const void *g, int q) {
    (void)c; (void)a; (void)g;
    ++moves;
    quiet_seen = q;
    last_range = range;
    return 1;
}
static uint8_t sub_update(void *c, AoeMoveActionRor *a) { (void)c; (void)a; return sub_result; }
static void sub_clear(void *c, AoeMoveActionRor *a) { (void)c; (void)a; }
static void place(void *c, AoeLiveUnit *u, float x, float y, float z) {
    (void)c; (void)u; (void)x; (void)y;
    placed_z = z;
}
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v,
                        int32_t a) {
    (void)c; (void)u; (void)a;
    assert(v == 0x25d);
    notices[notice_count++ & 3] = code;
}

static AoeLiveUnit *resolve(void *c, int32_t id) { (void)c; (void)id; return NULL; }
static void rel(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void att(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t ty) { (void)c; (void)t; (void)u; (void)ty; }

static int tu_head_v, tu_new_v = 1, tu_clears, tu_adopts;
static float tu_x, tu_y;
static AoeLiveUnit *tu_target;
static int tu_action;
static void *tu_head(void *c, AoeLiveUnit *u) { (void)c; (void)u; return tu_head_v ? &tu_action : NULL; }
static void *tu_new(void *c, AoeLiveUnit *u, float x, float y, float z) {
    (void)c; (void)u; (void)z; tu_x = x; tu_y = y; return tu_new_v ? &tu_action : NULL;
}
static void tu_clear(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++tu_clears; }
static void tu_adopt(void *c, AoeLiveUnit *u, void *a) { (void)c; (void)u; (void)a; ++tu_adopts; }
static void tu_retarget(void *c, void *a, AoeLiveUnit *t, float x, float y, float z) {
    (void)c; (void)a; (void)z; tu_target = t; tu_x = x; tu_y = y;
}

static void transport_unit_tests(void) {
    static AoeLiveUnit boat;
    AoeTransportUnitOpsRor o = {NULL, tu_head, tu_new, tu_clear, tu_adopt, tu_retarget};
    boat.core.position.x38 = 4.0f;
    boat.core.position.y3c = 5.0f;
    /* No head: a new action, the list replaced, then the retarget. */
    assert(aoe_transport_unit_go_ror(&boat, 9.0f, 8.0f, 0.0f, &o) == 1 && tu_clears == 1 &&
           tu_adopts == 1 && tu_target == &boat && tu_x == 9.0f && tu_y == 8.0f);
    /* A head transport from here goes to the unit's own position. */
    tu_head_v = 1;
    assert(aoe_transport_unit_go_from_here_ror(&boat, 9.0f, 8.0f, 0.0f, &o) == 1 &&
           tu_clears == 1 && tu_x == 4.0f && tu_y == 5.0f);
    tu_head_v = 0;
    tu_new_v = 0;
    assert(aoe_transport_unit_go_ror(&boat, 9.0f, 8.0f, 0.0f, &o) == 0);
}

int main(void) {
    transport_unit_tests();
    AoeBaseActionOps o;
    AoeBaseActionRor a;
    uint8_t r;
    memset(&o, 0, sizeof(o));
    o.move.select_graphic = sel; o.move.unit_notice = unit_notice;
    o.move.resolve = resolve; o.move.target_released = rel;
    o.move.target_attached = att;
    o.graphic = graphic; o.tick_seconds = tick; o.rand = rnd;
    o.map_width = mw; o.map_height = mw; o.start_move = start_move;
    o.sub_update = sub_update; o.sub_clear = sub_clear; o.place = place;
    bird_t.standing_graphic = 9;
    bird_t.walking_graphic = 1;
    bird.core.unit_template = &bird_t;
    bird.graphic.current_graphic = G_CUR;
    task.moving_graphic = 1;
    task.proceeding_graphic = 2;
    task.work_range = 0.5f;

    /* Fly: lifted to z 6, then 3 picks rand x 100 / 32767 for x and y. */
    assert(aoe_fly_action_init_ror(&a, &bird, &task, -1, -1, -1));
    assert(aoe_fly_action_activate_ror(&a, 1, &o) && placed_z == 6.0f &&
           a.base.state == 3);
    rnd_v = 16384; /* 16384 x 100 / 32767 = 50; x 10 / 32767 = 5 -> 10 s */
    assert(aoe_fly_action_update_ror(&a, &o, &r) && a.base.state == 4 &&
           a.base.x == 50.0f && a.base.y == 50.0f && moves == 1 &&
           quiet_seen == -1 && last_range == 0.5f && a.timer == 10.0f &&
           shown == G_FLY);
    /* After 10 s it glides (6) with a new 5..15 s timer, still walking. */
    for (int i = 0; i < 9; ++i)
        assert(aoe_fly_action_update_ror(&a, &o, &r) && a.base.state == 4);
    assert(aoe_fly_action_update_ror(&a, &o, &r) && a.base.state == 6 &&
           shown == G_GLIDE);
    /* Arrival (1) picks the next point. */
    sub_result = 1;
    assert(aoe_fly_action_update_ror(&a, &o, &r) && a.base.state == 3);
    sub_result = 0;

    /* Explore: no point idles (3, notice 0x202); a point walks quietly. */
    notice_count = 0;
    assert(aoe_explore_action_init_ror(&a, &bird, &task, -1, -1, -1));
    assert(aoe_explore_action_activate_ror(&a, &o) && a.base.state == 3 &&
           notices[0] == 0x202);
    assert(aoe_explore_action_retarget_ror(&a, 20.0f, 30.0f, 0.0f, &o) &&
           a.base.state == 0x0b && quiet_seen == 1 && last_range == 0.0f);
    sub_result = 3;
    assert(aoe_explore_action_update_ror(&a, &o, &r) && a.base.state == 0x0d);
    assert(aoe_explore_action_update_ror(&a, &o, &r) && r == 3 &&
           a.base.state == 2);
    assert(aoe_explore_action_retarget_ror(&a, -1.0f, 5.0f, 0.0f, &o) &&
           a.base.state == 3);
    return 0;
}
