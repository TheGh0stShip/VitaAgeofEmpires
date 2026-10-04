#include "action_attack.h"

#include <assert.h>
#include <string.h>

typedef struct {
    AoeLiveUnit *units[2];
    int16_t owners[2];
    int notices[8][3], notice_count;
    const void *graphic;
    float speed, distance;
    int can, holds, group_cleared, refuses, attached_target;
    uint8_t vis, ready, done, reached, flags, sub_result;
    int16_t frames;
    int reloads, fires_target, fires_point, starts, start_ok, clears;
    int32_t last_frame;
    float tick;
} Env;

static Env env;
static AoeLiveUnit attacker, victim;
static DatUnit attacker_t, victim_t;

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    return id == victim.core.unit_id && victim.core.unit_template ? &victim
                                                                  : NULL;
}
static void released(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void attached_hook(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t type) {
    (void)c; (void)t; (void)u;
    assert(type == 9);
}
static void clear_secondary(void *c, AoeMoveActionRor *a) { (void)c; a->secondary_id = -1; }
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v, int32_t arg) {
    (void)c; (void)u;
    assert(env.notice_count < 8);
    env.notices[env.notice_count][0] = code;
    env.notices[env.notice_count][1] = v;
    env.notices[env.notice_count][2] = arg;
    ++env.notice_count;
}
static void select_graphic(void *c, AoeLiveUnit *u, const void *g) { (void)c; (void)u; env.graphic = g; }
static float speed(void *c, AoeLiveUnit *u) { (void)c; (void)u; return env.speed; }
static int can_attack(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x, float y) {
    (void)c; (void)u; (void)t; (void)x; (void)y;
    return env.can;
}
static float dist_point(void *c, AoeLiveUnit *u, float x, float y, float z) {
    (void)c; (void)u; (void)x; (void)y; (void)z;
    return env.distance;
}
static float dist_target(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    (void)c; (void)u; (void)t;
    return env.distance;
}
static int holds(void *c, AoeLiveUnit *u) { (void)c; (void)u; return env.holds; }
static void group_clear(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++env.group_cleared; }
static int16_t owner_id(void *c, AoeLiveUnit *u) {
    (void)c;
    return u == &attacker ? env.owners[0] : env.owners[1];
}
static int refuses(void *c, AoeLiveUnit *u, int16_t other) { (void)c; (void)u; (void)other; return env.refuses; }
static uint8_t visibility(void *c, AoeLiveUnit *u, int32_t x, int32_t y) {
    (void)c; (void)u; (void)x; (void)y;
    return env.vis;
}
static int is_attached(void *c, AoeLiveUnit *u) { (void)c; (void)u; return env.attached_target; }
static uint8_t ready(void *c, AoeLiveUnit *u) { (void)c; (void)u; return env.ready; }
static void reset_reload(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++env.reloads; }
static uint8_t done(void *c, AoeLiveUnit *u) { (void)c; (void)u; return env.done; }
static uint8_t reached(void *c, AoeLiveUnit *u, int32_t f) { (void)c; (void)u; env.last_frame = f; return env.reached; }
static int16_t frames(void *c, AoeLiveUnit *u) { (void)c; (void)u; return env.frames; }
static uint8_t flags(void *c, AoeLiveUnit *u) { (void)c; (void)u; return env.flags; }
static void fire_target(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; ++env.fires_target; }
static void fire_point(void *c, AoeLiveUnit *u, float x, float y, float z) { (void)c; (void)u; (void)x; (void)y; (void)z; ++env.fires_point; }
static int start_move(void *c, AoeAttackActionRor *a) { (void)c; (void)a; ++env.starts; return env.start_ok; }
static uint8_t sub_update(void *c, AoeAttackActionRor *a) { (void)c; (void)a; return env.sub_result; }
static void sub_clear(void *c, AoeAttackActionRor *a) { (void)c; (void)a; ++env.clears; }
static float tick(void *c) { (void)c; return env.tick; }

static const char G_MOVE[] = "move", G_ATTACK[] = "attack", G_IDLE[] = "idle";

int main(void) {
    AoeAttackActionOps ops;
    AoeAttackActionRor a;
    AoeAttackParamsRor none = {0}, defaults = {G_MOVE, G_ATTACK, G_IDLE, 0, 0, 0, 0};
    AoeAttackParamsRor p = {0};
    uint8_t result;
    int32_t where;
    memset(&ops, 0, sizeof(ops));
    ops.move.resolve = resolve; ops.move.target_released = released;
    ops.move.target_attached = attached_hook;
    ops.move.clear_secondary = clear_secondary; ops.move.unit_notice = unit_notice;
    ops.move.select_graphic = select_graphic; ops.move.speed = speed;
    ops.can_attack = can_attack; ops.distance_to_point = dist_point;
    ops.distance_to_target = dist_target; ops.group_holds = holds;
    ops.group_clear_target = group_clear; ops.owner_id = owner_id;
    ops.refuses_owner = refuses; ops.visibility = visibility;
    ops.attached = is_attached; ops.ready = ready; ops.reset_reload = reset_reload;
    ops.sprite_done = done; ops.sprite_reached = reached;
    ops.sprite_frames = frames; ops.sprite_flags = flags;
    ops.fire_target = fire_target; ops.fire_point = fire_point;
    ops.start_move = start_move; ops.sub_update = sub_update;
    ops.sub_clear = sub_clear; ops.tick_seconds = tick;

    memset(&env, 0, sizeof(env));
    attacker.core.unit_template = &attacker_t;
    attacker.core.unit_id = 1;
    attacker_t.id = 4;
    attacker_t.size[0] = attacker_t.size[1] = 0.3f;
    victim.core.unit_template = &victim_t;
    victim.core.unit_id = 2;
    victim.core.position.x38 = 10.0f;
    victim.core.position.y3c = 10.0f;
    victim_t.unit_class = 6;
    env.owners[0] = 1;
    env.owners[1] = 2;
    env.can = 1;
    env.speed = 1.0f;
    env.start_ok = 1;
    env.vis = 0x0f;
    env.tick = 0.5f;

    /* Constructor defaults: graphics, template range, then 0.4. */
    assert(aoe_attack_action_init_target_ror(&a, &attacker, &victim, &none,
                                             &defaults, 0.0f, &ops));
    assert(a.base.type == 9 && a.base.target == &victim &&
           a.base.target_id == 2 && a.move_graphic == G_MOVE &&
           a.attack_graphic == G_ATTACK && a.idle_graphic == G_IDLE &&
           a.range == 0.4f && a.base.x == -1.0f && !a.friendly &&
           !a.approach_left);
    assert(aoe_attack_action_init_point_ror(&a, &attacker, 3, 4, 0, &none,
                                            &defaults, 5.0f) &&
           a.range == 5.0f && a.approach_left == 2 && !a.base.target);

    /* A mobile attacker approaches: state 4, one approach spent. */
    p.range = 1.0f;
    p.projectile_id = -1;
    assert(aoe_attack_action_init_target_ror(&a, &attacker, &victim, &p,
                                             &defaults, 0.0f, &ops));
    assert(aoe_attack_action_activate_ror(&a, &ops) && a.base.state == 4 &&
           env.starts == 1 && a.timer == 0.0f);

    /* The move sub-action arrives (1): state 6 measures the range (1.5 <=
       sqrt(0.18) + 1 + 0.3), caches the position, resets the reload and
       starts firing (state 7, attack graphic). */
    env.sub_result = 1;
    env.distance = 1.5f;
    env.ready = 1;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 0);
    assert(a.base.state == 7 && a.firing == 1 && env.graphic == G_ATTACK &&
           env.reloads == 1 && a.base.x == 10.0f && a.timer == 0.5f);

    /* Firing waits for the fire frame (half of 10 frames by default). */
    env.frames = 10;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 0 &&
           env.last_frame == 5 && !env.fires_target && a.firing);
    env.reached = 1;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) &&
           env.fires_target == 1 && !a.firing);

    /* Ready again: reload, re-check (cached position, no measuring) and
       fire in the same tick's frame check. */
    env.distance = 99.0f; /* would be too far if measured */
    assert(aoe_attack_action_update_ror(&a, &ops, &result) &&
           a.base.state == 7 && env.reloads == 3 && env.fires_target == 2);

    /* Moving the target beyond the 0.5 cache window measures: too far ->
       notice 0x1fd and approach. */
    victim.core.position.x38 = 10.6f;
    env.notice_count = 0;
    where = -1;
    assert(aoe_attack_action_check_range_ror(&a, &where, &ops) == 0 &&
           where == 2 && a.base.state == 4 && env.notices[0][0] == 0x1fd);
    /* Too close: notice 0x1fe, no state change for a mobile unit. */
    a.min_range = 2.0f;
    env.distance = 1.0f;
    a.base.state = 6;
    assert(aoe_attack_action_check_range_ror(&a, &where, &ops) == 0 &&
           where == 0 && a.base.state == 6 && env.notices[1][0] == 0x1fe);
    /* ...but a stationary one gives up (0xd), idle graphic. */
    env.speed = 0.0f;
    assert(aoe_attack_action_check_range_ror(&a, &where, &ops) == 0 &&
           a.base.state == 0x0d && env.graphic == G_IDLE);
    env.speed = 1.0f;
    a.min_range = 0.0f;
    /* An unseen target (visibility 0) also gives up; class 3 skips it. */
    env.vis = 0;
    assert(aoe_attack_action_check_range_ror(&a, &where, &ops) == 0 &&
           a.base.state == 0x0d);
    victim_t.unit_class = 3;
    env.distance = 1.0f;
    assert(aoe_attack_action_check_range_ror(&a, &where, &ops) == 1 &&
           where == 1);
    victim_t.unit_class = 6;
    env.vis = 0x80; /* fogged: needs a nonzero draw class */
    victim.core.position.x38 = 12.0f;
    assert(aoe_attack_action_check_range_ror(&a, &where, &ops) == 0 &&
           a.base.state == 0x0d);
    victim_t.draw_class = 1;
    assert(aoe_attack_action_check_range_ror(&a, &where, &ops) == 1);
    env.vis = 0x0f;

    /* State 0xd reports 3 with notice 0x1f9/0x258 each tick. */
    a.base.state = 0x0d;
    env.notice_count = 0;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 3 &&
           env.notices[0][0] == 0x1f9 && env.notices[0][1] == 0x258);

    /* Sub-action failure 4: state 0xe and the lost notice with the
       target ID. */
    a.base.state = 4;
    env.sub_result = 4;
    env.notice_count = 0;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 4 &&
           a.base.state == 0x0e && env.notices[0][0] == 0x1fb &&
           env.notices[0][2] == 2);

    /* A refusing owner stops (state 1) with the lost result. */
    env.refuses = 1;
    a.base.state = 6;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 4 &&
           a.base.state == 1);
    env.refuses = 0;

    /* Dead target (state >= 3): drop it, state 2, result 5. */
    victim.core.state = 3;
    a.base.state = 7;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 5 &&
           a.base.state == 2 && !a.base.target && a.base.target_id == -1);
    victim.core.state = 0;
    /* State 2 waits for the animation, then finishes: 2, or 5 if quiet. */
    env.done = 0;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 0);
    env.done = 1;
    env.notice_count = 0;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 2 &&
           a.base.state == 1 && env.notices[0][0] == 0x1fa);
    a.base.state = 2;
    a.base.quiet = 1;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 5);
    a.base.quiet = 0;

    /* State 1 reports done. Towers clear their group's target. */
    assert(aoe_attack_action_update_ror(&a, &ops, &result) && result == 1);
    attacker_t.id = 79;
    assert(aoe_attack_action_stop_ror(&a, &ops) && env.group_cleared == 1);
    attacker_t.id = 4;

    /* Point attacks: ranged unit, in range, fires at the point; once the
       approach count is spent the next ready tick finishes (state 2). */
    p.projectile_id = 5;
    p.range = 5.0f;
    assert(aoe_attack_action_init_point_ror(&a, &attacker, 3, 4, 0, &p,
                                            &defaults, 0.0f));
    env.distance = 2.0f;
    env.holds = 1; /* a holding group does not approach */
    assert(aoe_attack_action_activate_ror(&a, &ops) && a.base.state == 7 &&
           a.approach_left == 2);
    assert(aoe_attack_action_update_ror(&a, &ops, &result) &&
           env.fires_point == 1);
    a.approach_left = 0;
    assert(aoe_attack_action_update_ror(&a, &ops, &result) &&
           a.base.state == 2);
    env.holds = 0;

    /* Retargeting: same target in 4/6/7 is kept; otherwise approach. */
    assert(!aoe_attack_action_retarget_ror(&a, NULL, &ops));
    assert(aoe_attack_action_retarget_ror(&a, &victim, &ops) &&
           a.base.state == 4 && a.base.target == &victim);
    env.starts = 0;
    assert(aoe_attack_action_retarget_ror(&a, &victim, &ops) && !env.starts);
    /* A failed move allocation notices 0x1f9/0x262 and stops. */
    env.start_ok = 0;
    a.base.state = 6;
    env.notice_count = 0;
    assert(aoe_attack_action_set_state_ror(&a, 4, &ops) && a.base.state == 1 &&
           env.notices[0][0] == 0x1f9 && env.notices[0][1] == 0x262);

    /* Graphic swap after an upgrade. */
    {
        static const char T1[] = "t1", T2[] = "t2", W1[] = "w1", W2[] = "w2";
        AoeAttackTemplateGraphicsRor ot = {G_MOVE, G_ATTACK, G_IDLE};
        AoeAttackTemplateGraphicsRor nt = {W1, W2, T1};
        AoeAttackTaskGraphicsRor ok = {NULL, G_ATTACK, NULL};
        AoeAttackTaskGraphicsRor nk = {NULL, T2, NULL};
        a.move_graphic = G_MOVE;
        a.attack_graphic = G_ATTACK;
        a.idle_graphic = G_IDLE;
        aoe_attack_action_swap_graphics_ror(&a, &ot, &nt, &ok, &nk);
        assert(a.move_graphic == W1 && a.attack_graphic == T2 &&
               a.idle_graphic == T1);
        (void)W2;
    }
    return 0;
}
