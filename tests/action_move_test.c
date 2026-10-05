#include "action_move.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static AoeLiveUnit unit, deer;
static DatUnit deer_t;
static int deer_alive = 1, in_range_now, start_result = 0, status_now = 1;
static float unit_speed = 1.0f;
static char trace[256];
static const char standing[] = "stand", walking[] = "walk";

static void mark(const char *s) {
    strncat(trace, s, sizeof(trace) - strlen(trace) - 1);
}
static void expect(const char *s) {
    if (strcmp(trace, s)) {
        fprintf(stderr, "%s != %s\n", trace, s);
        assert(0);
    }
    trace[0] = 0;
}
static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    return id == 7 && deer_alive ? &deer : NULL;
}
static void released(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; mark("rel "); }
static void attached(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t type) {
    (void)c; (void)t; (void)u;
    assert(type == 1);
    mark("att ");
}
static void clear2(void *c, AoeMoveActionRor *a) { (void)c; a->secondary_id = -1; mark("clr2 "); }
static void notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v, int32_t arg) {
    char s[32];
    (void)c; (void)u; (void)v;
    snprintf(s, sizeof(s), "n%x/%d ", code, arg);
    mark(s);
}
static void graphic(void *c, AoeLiveUnit *u, const void *g) {
    (void)c; (void)u;
    mark(g == standing ? "gS " : g == walking ? "gW " : "g? ");
}
static const void *stand(void *c, AoeLiveUnit *u) { (void)c; (void)u; return standing; }
static int range(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float r) {
    (void)c; (void)u; (void)t;
    assert(r == 0.4f);
    return in_range_now;
}
static float speed(void *c, AoeLiveUnit *u) { (void)c; (void)u; return unit_speed; }
static void flag(void *c, AoeLiveUnit *u, int32_t v) { (void)c; (void)u; assert(v == 0); mark("f0 "); }
static void dest(void *c, AoeLiveUnit *u, float x, float y) {
    char s[32];
    (void)c; (void)u;
    snprintf(s, sizeof(s), "d%g,%g ", x, y);
    mark(s);
}
static int waypoints_stale(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 0; }
static void tile(void *c, AoeLiveUnit *u, const uint8_t t[3]) {
    char s[32];
    (void)c; (void)u;
    snprintf(s, sizeof(s), "t%u,%u,%u ", t[0], t[1], t[2]);
    mark(s);
}
static void set_range(void *c, AoeLiveUnit *u, float r) { (void)c; (void)u; (void)r; }
static void tid(void *c, AoeLiveUnit *u, int32_t id) {
    char s[16];
    (void)c; (void)u;
    snprintf(s, sizeof(s), "id%d ", id);
    mark(s);
}
static void tsize(void *c, AoeLiveUnit *u, float a, float b) { (void)c; (void)u; (void)a; (void)b; }
static int start(void *c, AoeLiveUnit *u) { (void)c; (void)u; mark("go "); return start_result; }
static int status(void *c, AoeLiveUnit *u) { (void)c; (void)u; return status_now; }

int main(void) {
    AoeMoveActionOps ops = {0, resolve, released, attached, clear2, notice,
                            graphic, stand, range, speed, flag, dest,
                            waypoints_stale, tile, set_range, tid, tsize, start,
                            status};
    AoeMoveActionRor a;
    uint8_t r;
    unit.constructed = 1;
    unit.core.unit_id = 1;
    unit.group_unit_count = 4;
    deer.constructed = 1;
    deer.core.unit_id = 7;
    deer.core.state = 2;
    deer.core.unit_template = &deer_t;
    deer.core.position.x38 = 10.25f;
    deer.core.position.y3c = 20.75f;
    deer_t.size[0] = 0.3f;

    /* Point move: range 0 -> 0.4, default graphic. */
    assert(aoe_move_action_init_point_ror(&a, &unit, 5.5f, 6.5f, 0, 0.0f,
                                          NULL, walking) &&
           a.range == 0.4f && a.graphic == walking && a.type == 1);
    assert(aoe_move_action_activate_ror(&a, &ops) && a.state == 3);
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 0 && a.state == 4);
    expect("d5.5,6.5 t5,6,0 id-1 go gW ");
    status_now = 1;
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 0);
    status_now = 0;
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 3 && a.state == 1);
    expect("gS n1f9/0 ");
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 1);

    /* Immobile unit: state 0xd reports arrival every update. */
    unit_speed = NAN;
    assert(aoe_move_action_activate_ror(&a, &ops) && a.state == 0x0d);
    expect("gS ");
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 3);
    expect("n1f9/0 ");
    unit_speed = 1.0f;

    /* Target move: attached, chases, re-paths when the deer drifts. */
    assert(aoe_move_action_init_target_ror(&a, &unit, &deer, 0.0f, walking,
                                           standing, &ops) &&
           a.target == &deer && a.target_id == 7 && a.x == 10.25f);
    expect("att ");
    assert(aoe_move_action_activate_ror(&a, &ops) && a.state == 3);
    start_result = 1; /* movement could not start: stays in state 3 */
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 0 && a.state == 3);
    expect("d10.25,20.75 t10,20,0 id7 go ");
    start_result = 0;
    assert(aoe_move_action_update_ror(&a, &ops, &r) && a.state == 4);
    expect("d10.25,20.75 t10,20,0 id7 go gW ");
    deer.core.position.x38 = 11.5f; /* 1.25 tiles away: re-path */
    status_now = 1;
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 0);
    expect("n1fc/0 d11.5,20.75 t11,20,0 id7 go gW ");
    /* Status 2 with the target close: arrive near, result 2. */
    status_now = 2;
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 2 && a.state == 1);
    expect("n1fa/0 gS "); /* 0x00405837 notices before the state change */
    /* In range at start: done immediately, clears the group count. */
    a.state = 3;
    in_range_now = 1;
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 2 &&
           unit.group_unit_count == 0);
    expect("gS n1fa/0 f0 ");
    in_range_now = 0;
    /* The deer dies: the target is dropped and the notice carries -1. */
    deer.core.state = 7;
    a.state = 4;
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 4 &&
           a.target == NULL && a.target_id == -1);
    expect("rel n1fb/-1 "); /* still resolvable: released via +0xc4 */
    /* Quiet actions return 5 instead of notifying. */
    deer.core.state = 2;
    assert(aoe_action_set_target_ror(&a, &deer, &ops));
    expect("att ");
    a.quiet = 1;
    deer_alive = 0;
    assert(aoe_move_action_update_ror(&a, &ops, &r) && r == 5);
    expect("");
    return 0;
}
