/* Patched RoR 1.0a per-class unit AIs (0x004e3b50..0x004e5dc0). */
#include "ai_unit_ai.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int based, restarted;
static void base_init(void *c) { (void)c; ++based; }
static void restart(void *c, int32_t t) { (void)c; restarted = t; }
static AoeAiUnitViewRor me;
static void self(void *c, AoeAiUnitViewRor *o) { (void)c; *o = me; }
static int object(void *c, int32_t id, AoeAiUnitViewRor *o) {
    (void)c;
    memset(o, 0, sizeof *o);
    o->id = id;
    o->type_class = (int16_t)(id % 100);
    o->type_id = (int16_t)(id / 100);
    o->x = 3.0f;
    o->y = 4.0f;
    return id >= 0;
}
static int32_t target_id = -1;
static int32_t target(void *c) { (void)c; return target_id; }
static float mx, my, mz;
static int moved;
static void move(void *c, float x, float y, float z) { (void)c; mx = x; my = y; mz = z; ++moved; }
static int32_t c5c = -1, c64 = -1, b5c = -1;
static void call_5c(void *c, int32_t id, int32_t v) { (void)c; (void)v; c5c = id; }
static void call_64(void *c, int32_t id, int32_t v) { (void)c; (void)v; c64 = id; }
static void base_5c(void *c, int32_t id, int32_t v) { (void)c; (void)v; b5c = id; }
static int call_c4(void *c, int32_t id) { (void)c; return id == 7; }
static double speed(void *c) { (void)c; return 0.5; }
static int32_t owner4 = 1;
static int32_t owner_field_4(void *c) { (void)c; return owner4; }
static float res27, res28;
static float owner_res(void *c, int32_t i) { (void)c; return i == 27 ? res27 : i == 28 ? res28 : 0.0f; }
static int32_t level = 2;
static int32_t difficulty(void *c) { (void)c; return level; }
static double dist = 3.0;
static int destroyed, freed, cleared, c58 = -1, d4_arg = -1;
static int32_t d4_result;
static void base_destroy(void *c) { (void)c; ++destroyed; }
static void free_self(void *c) { (void)c; ++freed; }
static int32_t call_d4(void *c, int32_t a) { (void)c; d4_arg = a; return d4_result; }
static void clear_target(void *c) { (void)c; ++cleared; }
static void call_58(void *c, int32_t a) { (void)c; c58 = a; }
static double distance_to(void *c, int32_t id) { (void)c; (void)id; return dist; }

static int cleared, c58, c1c, bcc, c6c = -1, c94 = -1;
static float f94;
static void call_1c(void *c, void *m) { (void)c; (void)m; ++c1c; }
static int32_t base_cc(void *c, void *m, int32_t a) { (void)c; (void)m; ++bcc; return a; }
static int32_t call_6c(void *c, int32_t id, int32_t v) { (void)c; c6c = id; return v; }
static int32_t call_94(void *c, int32_t id, float f) { (void)c; c94 = id; f94 = f; return 2; }
static double unit_10c(void *c, int32_t v) { (void)c; return v * 0.5; }

/* Priest searches: band 0 holds 31 (dist 2, player 1) and 32 (dist 6),
   band 3 holds 41 (dist 4, enemy player 2) and 42 (dist 0, player 3). */
static uint8_t band0[16] = {31, 0, 0, 0, 2, 1, 0, 0, 32, 0, 0, 0, 6, 1, 0, 0};
static uint8_t band3[16] = {41, 0, 0, 0, 4, 2, 0, 0, 42, 0, 0, 0, 0, 3, 0, 0};
static int refreshed;
static void p_refresh(void *c) { (void)c; ++refreshed; }
static int32_t p_nearby(void *c, int b, const uint8_t **e) {
    (void)c;
    if (b == 0) { *e = band0; return 2; }
    if (b == 3) { *e = band3; return 2; }
    return 0;
}
static int p_view(void *c, int32_t id, int16_t *cls, float *hp, int16_t *max, uint8_t *st) {
    (void)c; *cls = 0; *hp = 5.0f; *max = (int16_t)(id == 42 ? 100 : 20); *st = 2; return 1;
}
static int16_t p_owner(void *c) { (void)c; return 1; }
static int p_enemy(void *c, int32_t p) { (void)c; return p >= 2; }
static int p_194(void *c, int32_t id) { (void)c; (void)id; return 1; }
static int p_c4(void *c, int32_t id) { (void)c; (void)id; return 1; }

static int32_t pd_owner4(void *c) { (void)c; return 1; }
static void pd_self(void *c, AoeAiUnitViewRor *o) { (void)c; memset(o, 0, sizeof *o); o->field_44 = 100.0f; }
static int32_t pd_30(void *c) { (void)c; return 55; }
static int pd_resent;
static void pd_10(void *c) { (void)c; ++pd_resent; }
static int32_t pd_5c(void *c, int32_t id, int32_t v) { (void)c; return id == 55 && v == 1; }
static int pd_state = 3;
static int pd_head(void *c) { (void)c; return pd_state; }

static int32_t d4_type = 0x30;
static int16_t u_type(void *c) { (void)c; return (int16_t)d4_type; }
static int32_t no_30(void *c) { (void)c; return -1; }
static int32_t tgt7(void *c) { (void)c; return 7; }
static int obj_any(void *c, int32_t id, AoeAiUnitViewRor *o) { (void)c; memset(o, 0, sizeof *o); o->id = id; o->x = 3.0f; return 1; }
static void me0(void *c, AoeAiUnitViewRor *o) { (void)c; memset(o, 0, sizeof *o); }
static double spd1(void *c) { (void)c; return 1.0; }
static float mx = -1.0f;
static void mv(void *c, float x, float y, float z) { (void)c; (void)y; (void)z; mx = x; }

static int cc_cleared, cc_added = -1, cc_ec;
static void cc_clear(void *c) { (void)c; ++cc_cleared; }
static void cc_add(void *c, int32_t id) { (void)c; cc_added = id; }
static void cc_ec_f(void *c, int32_t v) { (void)c; cc_ec = v; }
static uint8_t cc_vis(void *c, int32_t x, int32_t y) { (void)c; (void)y; return (uint8_t)(x == 3 ? 0xf : 0); }

static void cc_5644_tests(void) {
    AoeAiUnitAiOpsRor o;
    AoeAiUnitAiOrderRor ord = {-1, 7, 5};
    AoeAiUnitAiMessageRor m = {44, 500, 0};
    memset(&o, 0, sizeof o);
    o.clear_target = cc_clear;
    o.group_add = cc_add;
    o.unit_ec = cc_ec_f;
    o.object = obj_any;
    o.vision = cc_vis;
    assert(aoe_ai_unit_ai_5644_cc_ror(&ord, &m, &o) == 2 && cc_cleared == 0);
    m.type = 0x1fa; m.value = 5;
    aoe_ai_unit_ai_5644_cc_ror(&ord, &m, &o);
    assert(cc_cleared == 1);
    m.value = 6;
    aoe_ai_unit_ai_5644_cc_ror(&ord, &m, &o);
    assert(cc_cleared == 1);
    ord.action = 105;
    aoe_ai_unit_ai_5644_cc_ror(&ord, &m, &o);
    assert(cc_cleared == 2);
    m.type = 0x1fc;   /* target at x 3, visible: ignored */
    aoe_ai_unit_ai_5644_cc_ror(&ord, &m, &o);
    assert(cc_cleared == 2);
    m.type = 600;
    aoe_ai_unit_ai_5644_cc_ror(&ord, &m, &o);
    assert(cc_added == 44 && cc_ec == 1 && cc_cleared == 2);
    m.type = 0x300;
    aoe_ai_unit_ai_5644_cc_ror(&ord, &m, &o);
    assert(cc_cleared == 3);
}

static void d4_5564_tests(void) {
    AoeAiUnitAiOpsRor o;
    AoeAiUnitAiOrderRor ord = {-1, -1};
    memset(&o, 0, sizeof o);
    o.owner_index = p_owner;
    o.owner_field_4 = pd_owner4;
    o.unit_type = u_type;
    o.call_30 = no_30;
    o.target = tgt7;
    o.object = obj_any;
    o.self = me0;
    o.speed = spd1;
    o.move = mv;
    /* Backs off 3 tiles from the target at x 3. */
    assert(aoe_ai_unit_ai_5564_d4_ror(&ord, 0, &o) == 6 && mx == -3.0f);
    ord.action = 0x2bd;
    assert(aoe_ai_unit_ai_5564_d4_ror(&ord, 0, &o) == 5);
    ord.action = 0x2d5;
    d4_type = 0x23;
    assert(aoe_ai_unit_ai_5564_d4_ror(&ord, 0, &o) == 5);
    o.settings_97d = 1;
    ord.action = -1;
    assert(aoe_ai_unit_ai_5564_d4_ror(&ord, 0, &o) == 5);
}

static void priest_search_tests(void) {
    AoeAiUnitAiOpsRor o;
    float d = -1.0f;
    memset(&o, 0, sizeof o);
    o.refresh_nearby = p_refresh;
    o.nearby = p_nearby;
    o.target_view = p_view;
    o.owner_index = p_owner;
    o.owner_enemy = p_enemy;
    o.unit_194 = p_194;
    o.call_c4 = p_c4;
    /* Heal: 32 is too far unless `far`; 31 scores 20 - 5 / 2. */
    assert(aoe_ai_unit_ai_priest_heal_target_ror(99, 0, &d, &o) == 31 && d == 2.0f);
    assert(aoe_ai_unit_ai_priest_heal_target_ror(31, 0, &d, &o) == -1);
    assert(aoe_ai_unit_ai_priest_heal_target_ror(31, 1, &d, &o) == 32 && d == 6.0f);
    /* Convert: 42 (100 / max(0, 1)) beats 41 (20 / 4). */
    assert(aoe_ai_unit_ai_priest_convert_target_ror(&d, &o) == 42 && d == 1.0f && refreshed == 4);
    {
        /* 0x004e5620 */
        AoeAiUnitAiOrderRor ord = {7, 12};
        o.owner_field_4 = pd_owner4;
        o.self = pd_self;
        o.call_30 = pd_30;
        o.call_10 = pd_10;
        o.call_5c_r = pd_5c;
        o.head_state = pd_head;
        assert(aoe_ai_unit_ai_priest_d8_ror(&ord, &o) == 10 && pd_resent == 1);
        ord.target = 55;
        assert(aoe_ai_unit_ai_priest_d8_ror(&ord, &o) == 9);
        ord.target = 12;
        pd_state = 7;
        assert(aoe_ai_unit_ai_priest_d8_ror(&ord, &o) == 9 && pd_resent == 1);
    }
}

int main(void) {
    priest_search_tests();
    d4_5564_tests();
    cc_5644_tests();
    AoeAiUnitAiRor ai;
    AoeAiUnitAiOpsRor ops;
    memset(&ops, 0, sizeof ops);
    ops.base_init = base_init;
    ops.restart_timer = restart;
    ops.self = self;
    ops.object = object;
    ops.target = target;
    ops.move = move;
    ops.call_5c = call_5c;
    ops.call_64 = call_64;
    ops.base_5c = base_5c;
    ops.call_c4 = call_c4;
    ops.speed = speed;
    ops.owner_field_4 = owner_field_4;
    ops.owner_resource = owner_res;
    ops.difficulty = difficulty;
    ops.distance_to = distance_to;

    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_CLASS_9, &ops);
    assert(based == 1 && ai.class_count == 16 && ai.classes[15] == 0x27 &&
           ai.timer == 4000 && restarted == 4000);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_TYPE_30, &ops);
    assert(ai.class_count == 0 && ai.timer == 0);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_CLASS_15, &ops);
    assert(ai.class_count == 8 && ai.classes[7] == 0x21);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_KIND_5564, &ops);
    assert(ai.class_count == 21 && ai.classes[1] == 3 && ai.classes[20] == 0x27 &&
           ai.timer == 1500 && restarted == 1500);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_KIND_5644, &ops);
    assert(ai.class_count == 19 && ai.classes[1] == 4 && ai.classes[14] == 0x16 &&
           ai.timer == 5000);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_KIND_5724, &ops);
    assert(ai.class_count == 21 && ai.timer == 0);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_KIND_5804, &ops);
    assert(ai.class_count == 21 && ai.classes[15] == 0x1b);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_PRIEST, &ops);
    assert(ai.class_count == 17 && ai.classes[0] == 3 && ai.classes[1] == 0);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_CLASS_4, &ops);
    assert(ai.class_count == 11 && ai.classes[10] == 0x1b);

    assert(aoe_ai_unit_ai_slot_54_ror() == 1 && aoe_ai_unit_ai_slot_d4_ror() == 5 &&
           aoe_ai_unit_ai_slot_d8_ror() == 9);
    assert(aoe_ai_unit_ai_resource_of_ror(0xf) == 0 && aoe_ai_unit_ai_resource_of_ror(0x21) == 2 &&
           aoe_ai_unit_ai_resource_of_ror(0x20) == 4 && aoe_ai_unit_ai_resource_of_ror(6) == -1);

    aoe_ai_unit_ai_class4_5c_ror(110, 0, &ops);
    aoe_ai_unit_ai_class4_5c_ror(103, 0, &ops);
    aoe_ai_unit_ai_class4_5c_ror(-1, 0, &ops);
    assert(c64 == 110 && b5c == 103);
    assert(aoe_ai_unit_ai_priest_50_ror(1, 7, &ops) && !aoe_ai_unit_ai_priest_50_ror(0, 7, &ops) &&
           !aoe_ai_unit_ai_priest_50_ror(1, 8, &ops));

    /* Conversion: faith 100 needed. */
    me.field_44 = 99.0f;
    assert(!aoe_ai_unit_ai_can_convert_ror(104, &ops));
    me.field_44 = 100.0f;
    assert(aoe_ai_unit_ai_can_convert_ror(104, &ops));
    assert(!aoe_ai_unit_ai_can_convert_ror(10900 + 4, &ops)); /* type 0x6d */
    assert(!aoe_ai_unit_ai_can_convert_ror(127, &ops)); /* class 0x1b */
    assert(!aoe_ai_unit_ai_can_convert_ror(118, &ops)); /* priest, no tech */
    res27 = 1.0f;
    assert(aoe_ai_unit_ai_can_convert_ror(118, &ops));
    assert(!aoe_ai_unit_ai_can_convert_ror(103, &ops));
    res28 = 1.0f;
    assert(aoe_ai_unit_ai_can_convert_ror(103, &ops));

    /* Backing off: target at (3,4), unit at (0,0): distance 5, step
       0.5 * 3 = 1.5 away. */
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_CLASS_9, &ops);
    assert(aoe_ai_unit_ai_back_off_ror(&ai, &ops) == 5);
    target_id = 5;
    me.z = 2.0f;
    assert(aoe_ai_unit_ai_back_off_ror(&ai, &ops) == 6 && mx == -0.9f && my == -1.2f && mz == 2.0f);
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_CLASS_2, &ops);
    assert(aoe_ai_unit_ai_back_off_ror(&ai, &ops) == 6 && mx == -2.1f && my == -2.8f);
    owner4 = 0;
    assert(aoe_ai_unit_ai_back_off_ror(&ai, &ops) == 5 && moved == 2);

    /* Type 0x7e: reach type +0x28 x 2 = 4 normally. */
    aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_TYPE_7E, &ops);
    me.type_28 = 2.0f;
    target_id = 9;
    restarted = 0;
    ai.timer = 1;
    assert(aoe_ai_unit_ai_7e_d4_ror(&ai, &ops) == 6 && c5c == 9 && ai.timer == 6000 &&
           restarted == 6000);
    ops.settings_8fc = ops.settings_97d = 1;
    level = 3; /* 0.75 x 2 = 1.5 */
    c5c = -1;
    assert(aoe_ai_unit_ai_7e_d4_ror(&ai, &ops) == 5 && c5c == -1);
    dist = 1.0;
    level = 4; /* 0.5 x 2 = 1.0 */
    assert(aoe_ai_unit_ai_7e_d4_ror(&ai, &ops) == 6);
    target_id = -1;
    assert(aoe_ai_unit_ai_7e_d4_ror(&ai, &ops) == 5);

    ops.clear_target = clear_target;
    ops.call_58 = call_58;
    ops.call_1c = call_1c;
    ops.base_cc = base_cc;
    ops.call_6c = call_6c;
    ops.call_94 = call_94;
    ops.unit_10c = unit_10c;
    /* Class 10 +0xcc: reach type +0x28 = 2. */
    c5c = -1;
    dist = 2.0;
    assert(aoe_ai_unit_ai_class10_cc_ror(9, 0x1fc, NULL, 4, &ops) == 3 && c5c == 9 && bcc == 0);
    dist = 2.5;
    assert(aoe_ai_unit_ai_class10_cc_ror(9, 0x1fc, NULL, 4, &ops) == 4 && cleared == 1 &&
           c58 == 1 && c1c == 1 && bcc == 1);
    assert(aoe_ai_unit_ai_class10_cc_ror(-1, 0x1fc, NULL, 4, &ops) == 4 && cleared == 2);
    assert(aoe_ai_unit_ai_class10_cc_ror(9, 0x1fb, NULL, 7, &ops) == 7 && cleared == 2 && bcc == 3);
    /* Priest +0x5c. */
    assert(aoe_ai_unit_ai_priest_5c_ror(-1, 7, 11, &ops) == 11 && c6c == 7);
    assert(aoe_ai_unit_ai_priest_5c_ror(-1, 8, 6, &ops) == 0 && c94 == -1); /* owner4 0 */
    owner4 = 1;
    assert(aoe_ai_unit_ai_priest_5c_ror(3, 8, 6, &ops) == 0 && c94 == -1);
    assert(aoe_ai_unit_ai_priest_5c_ror(-1, 8, 6, &ops) == 2 && c94 == 8 && f94 == 3.0f);
    {
        AoeAiUnitAiOpsRor o2;
        memset(&o2, 0, sizeof o2);
        o2.base_destroy = base_destroy;
        o2.free_self = free_self;
        o2.call_d4 = call_d4;
        o2.clear_target = clear_target;
        o2.call_58 = call_58;
        aoe_ai_unit_ai_init_ror(&ai, AOE_AI_UNIT_AI_KIND_5644, &o2);
        cleared = 0;
        c58 = -1;
        /* 0x004e5e90: only bit 0 frees. */
        assert(aoe_ai_unit_ai_delete_ror(&ai, 2, &o2) == &ai && destroyed == 1 && freed == 0 &&
               ai.class_count == 0);
        assert(aoe_ai_unit_ai_delete_ror(&ai, 3, &o2) == &ai && destroyed == 2 && freed == 1);
        /* 0x004e6d20. */
        ai.field_f8 = 7;
        d4_result = 5;
        assert(aoe_ai_unit_ai_5644_d8_ror(&ai, &o2) == 9 && d4_arg == 1 && cleared == 0 &&
               c58 == -1 && ai.field_f8 == 7);
        d4_result = 6;
        assert(aoe_ai_unit_ai_5644_d8_ror(&ai, &o2) == 10 && cleared == 1 && c58 == 1 &&
               ai.field_f8 == 0);
    }
    puts("ai unit ai ok");
    return 0;
}
