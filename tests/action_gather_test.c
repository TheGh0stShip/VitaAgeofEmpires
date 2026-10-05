#include "action_gather.h"

#include <assert.h>
#include <string.h>

static AoeLiveUnit villager, bush, center;
static DatUnit vil_t, bush_t, center_t;
static DatUnitTask task;
static int moves, quiet_first_seen, sounds, notices[4], notice_count;
static AoeLiveUnit *move_target;
static float move_range;
static const void *move_g, *shown;
static uint8_t sub_result;
static int faced = 1, owner_kind_v;
static float distance_v = 0.5f, attribute_v = 1.0f, tick_v = 1.0f;
static int16_t deposit_type;
static float deposit_amount;
static int find_calls;
static AoeLiveUnit *find_result;
static const char G_MOVE[] = "m", G_WORK[] = "w", G_CARRY[] = "c",
                  G_STAND[] = "s";

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    if (id == bush.core.unit_id) return &bush;
    if (id == center.core.unit_id) return &center;
    return NULL;
}
static void released(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void attached(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t ty) {
    (void)c; (void)t; (void)u;
    assert(ty == 5);
}
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v,
                        int32_t a) {
    (void)c; (void)u; (void)v; (void)a;
    if (notice_count < 4) notices[notice_count] = code;
    ++notice_count;
}
static void select_graphic(void *c, AoeLiveUnit *u, const void *g) {
    (void)c; (void)u;
    shown = g;
}
static const void *graphic(void *c, int16_t id) {
    (void)c;
    switch (id) {
    case 1: return G_MOVE;
    case 2: return G_WORK;
    case 4: return G_CARRY;
    case 9: return G_STAND;
    default: return NULL;
    }
}
static float tick(void *c) { (void)c; return tick_v; }
static int32_t owner_kind(void *c, AoeLiveUnit *u) { (void)c; (void)u; return owner_kind_v; }
static int face(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; return faced; }
static float distance(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; return distance_v; }
static float attribute(void *c, AoeLiveUnit *u, int16_t i) { (void)c; (void)u; (void)i; return attribute_v; }
static void deposit(void *c, AoeLiveUnit *u, int16_t type, float amount,
                    int flag) {
    assert(flag == 1);
    (void)c; (void)u;
    deposit_type = type;
    deposit_amount = amount;
}
static AoeLiveUnit *find(void *c, AoeLiveUnit *u, int16_t a, int16_t b,
                         float x, float y, AoeLiveUnit *z) {
    (void)c; (void)u; (void)x; (void)y; (void)z;
    assert(a == 100 && b == 101);
    ++find_calls;
    return find_result;
}
static void play(void *c, int16_t s) { (void)c; (void)s; ++sounds; }
static int start_move(void *c, AoeGatherActionRor *a, AoeLiveUnit *t,
                      float range, const void *g, int quiet_first) {
    (void)c; (void)a;
    ++moves;
    move_target = t;
    move_range = range;
    move_g = g;
    quiet_first_seen = quiet_first;
    return 1;
}
static uint8_t sub_update(void *c, AoeGatherActionRor *a) { (void)c; (void)a; return sub_result; }
static void sub_clear(void *c, AoeGatherActionRor *a) { (void)c; (void)a; }

static int start_attack(void *c, AoeGatherActionRor *a, const void *mg,
                        const void *ag, float range) {
    (void)c; (void)a; (void)mg; (void)ag; (void)range;
    return 1;
}

int main(void) {
    AoeGatherActionOps ops;
    AoeGatherActionRor g;
    uint8_t r;
    memset(&ops, 0, sizeof(ops));
    ops.move.resolve = resolve; ops.move.target_released = released;
    ops.move.target_attached = attached; ops.move.unit_notice = unit_notice;
    ops.move.select_graphic = select_graphic;
    ops.graphic = graphic; ops.tick_seconds = tick; ops.owner_kind = owner_kind;
    ops.face = face; ops.distance = distance; ops.player_attribute = attribute;
    ops.deposit = deposit; ops.find_drop_site = find; ops.play_sound = play;
    ops.start_move = start_move; ops.sub_update = sub_update;
    ops.sub_clear = sub_clear;
    ops.start_attack = start_attack;

    villager.core.unit_template = &vil_t;
    villager.core.unit_id = 1;
    villager.runtime_owner_index = 1;
    villager.core.storage.carried_resource_type = -1;
    vil_t.standing_graphic = 9;
    vil_t.resource_capacity = 10;
    vil_t.work_rate = 0.5f;
    vil_t.drop_sites[0] = 100;
    vil_t.drop_sites[1] = 101;
    bush.core.unit_template = &bush_t;
    bush.core.unit_id = 2;
    bush.core.position.x38 = 7.0f;
    bush.core.storage.carried_amount = 13.0f;
    bush_t.id = 59;
    bush_t.resource_capacity = 150; /* clamps the target too */
    center.core.unit_template = &center_t;
    center.core.unit_id = 3;
    center.runtime_owner_index = 1;
    center_t.id = 100;
    task.resource_in = 0;
    task.resource_multiplier = -1;
    task.resource_out = -1;
    task.work_value1 = 4.0f;
    task.work_range = 0.25f;
    task.moving_graphic = 1;
    task.proceeding_graphic = 2;
    task.carrying_graphic = 4;
    task.gathering_sound = 7;
    task.deposit_sound = -1;

    /* Construction records the target's position and template ID;
       activation sets the carried type and walks (state 4, work range). */
    assert(aoe_gather_action_init_target_ror(&g, &villager, &task, &bush,
                                             &ops));
    assert(g.base.type == 5 && g.target_template == 59 && g.base.x == 7.0f);
    assert(aoe_gather_action_activate_ror(&g, 1, &ops) && g.base.state == 4 &&
           villager.core.storage.carried_resource_type == 0 && moves == 1 &&
           move_target == &bush && move_range == 0.25f && move_g == G_MOVE &&
           !quiet_first_seen);

    /* Arrival -> face (6) -> work (7, working graphic and sound). */
    sub_result = 1;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 6 &&
           shown == G_MOVE);
    faced = 0;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 6);
    faced = 1;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 7 &&
           shown == G_WORK && sounds == 1);

    /* Work: 1 s * 0.5 * 4 = 2 per tick. */
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && r == 0 &&
           villager.core.storage.carried_amount == 2.0f &&
           bush.core.storage.carried_amount == 11.0f && g.base.state == 7);
    for (int i = 0; i < 3; ++i)
        assert(aoe_gather_action_update_ror(&g, &ops, &r) &&
               g.base.state == 7);
    /* Full at the capacity of 10: bring it back (state 8). */
    find_result = &center;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 8 &&
           villager.core.storage.carried_amount == 10.0f &&
           bush.core.storage.carried_amount == 3.0f && find_calls == 1 &&
           g.drop_site == &center && move_target == &center &&
           move_g == G_CARRY);

    /* Too far from the drop site on arrival: wait 3 s, then retry. */
    distance_v = 1.5f;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.timer == -3.0f);
    tick_v = 2.0f;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.timer == -1.0f);
    moves = 0;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.timer == 0.0f &&
           moves == 1 && g.base.state == 8);
    tick_v = 1.0f;

    /* Deposit within 1.0: resource type in (out is -1), carried cleared,
       back to the resource (state 4) with the drop site released. */
    distance_v = 0.5f;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) &&
           deposit_type == 0 && deposit_amount == 10.0f &&
           villager.core.storage.carried_resource_type == -1 &&
           villager.core.storage.carried_amount == 0.0f &&
           g.base.state == 4 && !g.drop_site && move_target == &bush);

    /* Exhausting the target: 3 left, then state 8 with a partial load. */
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 6);
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 7);
    assert(aoe_gather_action_update_ror(&g, &ops, &r) &&
           villager.core.storage.carried_amount == 2.0f);
    assert(aoe_gather_action_update_ror(&g, &ops, &r) &&
           bush.core.storage.carried_amount == 0.0f &&
           villager.core.storage.carried_amount == 3.0f &&
           g.base.state == 8);
    /* Deposit; the empty target is still held, so walk back (state 4). */
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 4);
    /* There, the empty bush is dropped and the search cycle starts. */
    notice_count = 0;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 6);
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 3 &&
           !g.base.target && g.timer == -10.0f && notices[0] == 0x202);
    /* The search notice repeats every 10 s. */
    tick_v = 6.0f;
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && notice_count == 1);
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && notice_count == 2 &&
           g.timer == -10.0f);
    tick_v = 1.0f;

    /* Capacity scales with the task's attribute; farms (template 50) stay
       gatherable while empty. */
    bush_t.id = 50;
    bush.core.storage.carried_amount = 0.0f;
    assert(aoe_gather_action_retarget_ror(&g, &bush, 0, 0, 0, &ops) &&
           g.base.state == 4 && g.base.target == &bush);
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 6);
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 7);

    /* Retargeting to an own drop site while carrying delivers there. */
    villager.core.storage.carried_amount = 1.0f;
    villager.core.storage.carried_resource_type = 0;
    assert(aoe_gather_action_retarget_ror(&g, &center, 0, 0, 0, &ops) &&
           g.base.state == 8 && g.drop_site == &center && !g.base.target);
    /* Delivering to a point walks there (state 15, quiet before push). */
    assert(aoe_gather_action_deliver_ror(&g, NULL, 4, 5, 0, &ops) &&
           g.base.state == 0x0f && quiet_first_seen && g.base.x == 4.0f);
    /* Arriving at the point idles (state 2) for a human owner. */
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && g.base.state == 2);

    /* A target owned by another player stops with result 4. */
    bush.runtime_owner_index = 2;
    assert(aoe_gather_action_retarget_ror(&g, &bush, 0, 0, 0, &ops));
    assert(aoe_gather_action_update_ror(&g, &ops, &r) && r == 4 &&
           g.base.state == 1);
    bush.runtime_owner_index = 0;

    /* Unit methods. */
    assert(aoe_unit_keeps_empty_target_ror(45) &&
           !aoe_unit_keeps_empty_target_ror(59));
    bush_t.unit_class = 9;
    assert(aoe_unit_keeps_resource_ror(&bush, 4) &&
           !aoe_unit_keeps_resource_ror(&bush, 6));
    bush_t.unit_class = 3;
    assert(aoe_unit_keeps_resource_ror(&bush, 6));
    vil_t.unit_class = 2;
    assert(!aoe_unit_zone_drop_search_ror(&villager));
    villager.core.storage.carried_amount = 8.0f;
    aoe_unit_add_carried_ror(&villager, 5.0f, 1, 1);
    assert(villager.core.storage.carried_amount == 10.0f);
    return 0;
}
