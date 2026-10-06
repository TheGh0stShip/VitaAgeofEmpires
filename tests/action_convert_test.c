#include "action_convert.h"

#include <assert.h>
#include <string.h>

/* Patched RoR 1.0a conversion action 0x68 (vtable 0x0054871c). */

static AoeLiveUnit priest, man, tc;
static DatUnit p_t, m_t, tc_t;
static DatUnitTask task;
static int moves, notices[8], notice_count, flares, changes, alerts;
static int kind = 2, ally;
static uint32_t events[8];
static int event_count;
static float res27, res28, res41;
static uint16_t next_rand;
static uint8_t sub_result, civ;
static float range_used;
static const char G_MOVE[] = "m", G_WORK[] = "w", G_STAND[] = "s";

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    return id == man.core.unit_id ? &man : id == tc.core.unit_id ? &tc : NULL;
}
static void rel(void *c, AoeLiveUnit *t, AoeLiveUnit *u) { (void)c; (void)t; (void)u; }
static void att(void *c, AoeLiveUnit *t, AoeLiveUnit *u, int16_t ty) {
    (void)c; (void)t; (void)u;
    assert(ty == 0x68);
}
static void clear2(void *c, AoeMoveActionRor *a) { (void)c; a->secondary_id = -1; }
static void unit_notice(void *c, AoeLiveUnit *u, int32_t code, int32_t v,
                        int32_t a) {
    (void)c; (void)u; (void)a;
    assert(v == 0x25c);
    if (notice_count < 8) notices[notice_count] = code;
    ++notice_count;
}
static void sel(void *c, AoeLiveUnit *u, const void *g) { (void)c; (void)u; (void)g; }
static const void *graphic(void *c, int16_t id) {
    (void)c;
    return id == 1 ? G_MOVE : id == 2 ? G_WORK : id == 9 ? G_STAND : NULL;
}
static float tick(void *c) { (void)c; return 1.0f; }
static int face(void *c, AoeLiveUnit *u, AoeLiveUnit *t) { (void)c; (void)u; (void)t; return 1; }
static int allied(void *c, AoeLiveUnit *u, AoeLiveUnit *t) {
    (void)c;
    return ally || u->runtime_owner_index == t->runtime_owner_index;
}
static int32_t okind(void *c, AoeLiveUnit *u) { (void)c; (void)u; return kind; }
static float resource(void *c, AoeLiveUnit *u, int16_t type) {
    (void)c; (void)u;
    return type == 27 ? res27 : type == 28 ? res28 : 0.0f;
}
static void event(void *c, uint32_t e, int32_t a, int32_t b, int32_t x,
                  int32_t y) {
    (void)c; (void)a; (void)b; (void)x; (void)y;
    if (event_count < 8) events[event_count] = e;
    ++event_count;
}
static uint16_t rnd(void *c) { (void)c; return next_rand; }
static uint8_t civ_of(void *c, AoeLiveUnit *u) { (void)c; (void)u; return civ; }
static int change(void *c, AoeLiveUnit *t, int16_t owner) {
    (void)c;
    t->runtime_owner_index = owner;
    ++changes;
    return 1;
}
static void res_add(void *c, AoeLiveUnit *u, int16_t type, float amount) {
    (void)c; (void)u;
    assert(type == 41);
    res41 += amount;
}
static void carried(void *c, AoeLiveUnit *u, int16_t type, float amount) {
    (void)c;
    u->core.storage.carried_resource_type = type;
    u->core.storage.carried_amount = amount;
}
static void flare(void *c, AoeLiveUnit *t, float x, float y, float z) {
    (void)c; (void)t; (void)x; (void)y; (void)z;
    ++flares;
}
static void tnotice(void *c, AoeLiveUnit *t, AoeLiveUnit *u) {
    (void)c; (void)t; (void)u;
    ++alerts;
}
static void play(void *c, int16_t s) { (void)c; (void)s; }
static int start_move(void *c, AoeMoveActionRor *a, AoeLiveUnit *t,
                      float range, const void *g, int qf) {
    (void)c; (void)a; (void)t; (void)g; (void)qf;
    range_used = range;
    ++moves;
    return 1;
}
static uint8_t sub_update(void *c, AoeMoveActionRor *a) { (void)c; (void)a; return sub_result; }
static void sub_clear(void *c, AoeMoveActionRor *a) { (void)c; (void)a; }

static AoeConvertActionOps ops(void) {
    AoeConvertActionOps o;
    memset(&o, 0, sizeof(o));
    o.move.resolve = resolve; o.move.target_released = rel;
    o.move.target_attached = att; o.move.unit_notice = unit_notice;
    o.move.select_graphic = sel; o.move.clear_secondary = clear2;
    o.graphic = graphic; o.tick_seconds = tick; o.face = face;
    o.allied = allied; o.owner_kind = okind; o.resource = resource;
    o.event = event; o.rand = rnd; o.civ = civ_of; o.change_owner = change;
    o.resource_add = res_add; o.set_carried = carried; o.flare = flare;
    o.target_notice = tnotice; o.play_sound = play;
    o.start_move = start_move; o.sub_update = sub_update;
    o.sub_clear = sub_clear;
    return o;
}

int main(void) {
    AoeConvertActionOps o = ops();
    AoeConvertActionRor a;
    uint8_t r;
    priest.core.unit_template = &p_t;
    priest.runtime_owner_index = 1;
    p_t.standing_graphic = 9;
    p_t.work_rate = 1.0f;
    p_t.max_range = 10.0f;
    m_t.unit_class = 6;
    m_t.id = 75;
    man.core.unit_template = &m_t;
    man.core.unit_id = 4;
    man.core.state = 2;
    man.runtime_owner_index = 2;
    tc_t.id = 109;
    tc_t.unit_class = 3;
    tc.core.unit_template = &tc_t;
    tc.core.unit_id = 5;
    tc.core.state = 2;
    tc.runtime_owner_index = 2;
    task.work_value1 = 0.3f;
    task.work_value2 = 3.0f;
    task.moving_graphic = 1;
    task.proceeding_graphic = 2;
    task.gathering_sound = -1;

    /* Faith below 100: fail (13) with event 0x76. */
    assert(aoe_convert_action_init_target_ror(&a, &priest, &task, &man,
                                              &o.move) &&
           a.range == 1.0f && a.base.type == 0x68);
    priest.core.storage.carried_amount = 99.0f;
    assert(aoe_convert_action_activate_ror(&a, 1, &o) &&
           a.base.state == 0x0d && events[0] == 0x76);
    /* A town centre: never; owner kind 1 walks to the point (11). */
    event_count = 0;
    kind = 1;
    assert(aoe_convert_action_init_target_ror(&a, &priest, &task, &tc,
                                              &o.move));
    assert(aoe_convert_action_activate_ror(&a, 1, &o) &&
           a.base.state == 0x0b && events[0] == 0x79);
    /* Other buildings need resource 28 (range 1.0). */
    kind = 2;
    tc_t.id = 12;
    event_count = 0;
    priest.core.storage.carried_amount = 100.0f;
    assert(aoe_convert_action_activate_ror(&a, 1, &o) &&
           a.base.state == 0x0d && events[0] == 0x78);
    res28 = 1.0f;
    assert(aoe_convert_action_activate_ror(&a, 1, &o) &&
           a.base.state == 4 && range_used == 1.0f);
    /* Priests need resource 27. */
    m_t.unit_class = 0x12;
    event_count = 0;
    assert(aoe_convert_action_init_target_ror(&a, &priest, &task, &man,
                                              &o.move));
    assert(aoe_convert_action_activate_ror(&a, 1, &o) &&
           a.base.state == 0x0d && events[0] == 0x77);
    res27 = 1.0f;
    assert(aoe_convert_action_activate_ror(&a, 1, &o) &&
           a.base.state == 4 && range_used == 10.0f);

    /* Walk, face, then attempts every work value 2 (3 s): the state-7
       entry alerts the target's owner with a flare. */
    m_t.unit_class = 6;
    sub_result = 2;
    assert(aoe_convert_action_update_ror(&a, &o, &r) && a.base.state == 6);
    assert(aoe_convert_action_update_ror(&a, &o, &r) && a.base.state == 7 &&
           a.timer == 3.0f && flares == 1);
    assert(aoe_convert_action_update_ror(&a, &o, &r) && a.timer == 2.0f &&
           alerts == 1);
    assert(aoe_convert_action_update_ror(&a, &o, &r) && a.timer == 1.0f);
    /* rand 9831 / 32767 = 0.30002 > 0.3: fails, walks again (4). */
    next_rand = 9831;
    assert(aoe_convert_action_update_ror(&a, &o, &r) && a.base.state == 4 &&
           changes == 0 && flares == 2);
    /* 9830 / 32767 = 0.29999 <= 0.3: converted (0x7b), resource 41 +1,
       faith spent, states 3 then 4. */
    assert(aoe_convert_action_set_state_ror(&a, 7, &o));
    a.timer = 0.0f;
    next_rand = 9830;
    event_count = 0;
    assert(aoe_convert_action_update_ror(&a, &o, &r) && changes == 1 &&
           man.runtime_owner_index == 1 && res41 == 1.0f &&
           priest.core.storage.carried_amount == 0.0f &&
           priest.core.storage.carried_resource_type == 0x22 &&
           events[0] == 0x7b && a.base.state == 4);
    /* Now allied: done with result 4 (notice 0x1fb). */
    notice_count = 0;
    assert(aoe_convert_action_update_ror(&a, &o, &r) && r == 4 &&
           a.base.state == 1 && notices[0] == 0x1fb);
    /* Boats and priests count double; Macedonian targets four times. */
    man.runtime_owner_index = 2;
    m_t.unit_class = 0x15;
    priest.core.storage.carried_amount = 100.0f;
    assert(aoe_convert_action_retarget_ror(&a, &man, &o, &(int){0}));
    assert(aoe_convert_action_set_state_ror(&a, 7, &o));
    a.timer = 0.0f;
    next_rand = 5000; /* 0.1526 x 2 = 0.305 > 0.3 */
    assert(aoe_convert_action_update_ror(&a, &o, &r) && changes == 1);
    m_t.unit_class = 6;
    civ = 0x10;
    assert(aoe_convert_action_set_state_ror(&a, 7, &o));
    a.timer = 0.0f;
    next_rand = 2500; /* 0.0763 x 4 = 0.305 */
    assert(aoe_convert_action_update_ror(&a, &o, &r) && changes == 1);
    next_rand = 2400; /* 0.0732 x 4 = 0.293 */
    assert(aoe_convert_action_set_state_ror(&a, 7, &o));
    a.timer = 0.0f;
    assert(aoe_convert_action_update_ror(&a, &o, &r) && changes == 2);
    /* 0x004b2700: immediate conversion within range (dist - 0.5). */
    {
        int done = -1;
        man.runtime_owner_index = 2;
        a.base.target = &man;
        a.base.state = 4;
        a.range = 1.0f;
        priest.core.position.x38 = 0; priest.core.position.y3c = 0;
        man.core.position.x38 = 1.6f; man.core.position.y3c = 0;
        assert(aoe_convert_action_complete_ror(&a, &o, &done) && done == 0 &&
               man.runtime_owner_index == 2);
        man.core.position.x38 = 1.5f;
        event_count = 0;
        res41 = 0;
        assert(aoe_convert_action_complete_ror(&a, &o, &done) && done == 1 &&
               man.runtime_owner_index == priest.runtime_owner_index &&
               events[0] == 0x7b && res41 == 1.0f && a.base.state == 3 &&
               priest.core.storage.carried_amount == 0.0f);
        /* State 7 ignores distance; priests are never taken. */
        man.runtime_owner_index = 2;
        man.core.position.x38 = 50.0f;
        a.base.state = 7;
        m_t.unit_class = 0x12;
        assert(aoe_convert_action_complete_ror(&a, &o, &done) && done == 0);
        m_t.unit_class = 6;
        a.base.state = 7;
        assert(aoe_convert_action_complete_ror(&a, &o, &done) && done == 1);
        a.base.target = NULL;
        assert(aoe_convert_action_complete_ror(&a, &o, &done) && done == 0);
    }
    return 0;
}
