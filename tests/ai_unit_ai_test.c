/* Patched RoR 1.0a per-class unit AIs (0x004e3b50..0x004e5dc0). */
#include "ai_unit_ai.h"

#include <assert.h>
#include <stdarg.h>
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

static int eb_arg = -1, a4_v = -1, bcc2;
static void t_eb(void *c, int32_t a) { (void)c; eb_arg = a; }
static void t_a4(void *c, int32_t v) { (void)c; a4_v = v; }
static int32_t t_bcc(void *c, void *m, int32_t a) { (void)c; (void)m; ++bcc2; return a; }
static int32_t t_3c(void *c, int32_t v) { (void)c; (void)v; return 66; }
static int32_t c5c_id = -1;
static void t_5c(void *c, int32_t id, int32_t v) { (void)c; (void)v; c5c_id = id; }
static int32_t t_restart = -1;
static void t_rs(void *c, int32_t t) { (void)c; t_restart = t; }
static int32_t t_diff(void *c) { (void)c; return 2; }
static double t_dist3(void *c, int32_t id) { (void)c; (void)id; return 3.0; }
static void me2(void *c, AoeAiUnitViewRor *o) { (void)c; memset(o, 0, sizeof *o); o->type_28 = 2.0f; }

static void cc_more_tests(void) {
    static AoeAiUnitAiRor ai;
    AoeAiUnitAiOpsRor o;
    AoeAiUnitAiMessageRor m = {44, 500, 0, 0};
    AoeAiUnitAiOrderRor ord = {-1, 9, 0};
    memset(&o, 0, sizeof o);
    o.base_414eb0 = t_eb;
    o.call_a4 = t_a4;
    o.base_cc = t_bcc;
    o.call_3c = t_3c;
    o.call_5c = t_5c;
    o.group_add = cc_add;
    o.unit_ec = cc_ec_f;
    assert(aoe_ai_unit_ai_class9_cc_ror(&m, 8, &o) == 4 && eb_arg == 8);
    m.field_10 = 1;
    assert(aoe_ai_unit_ai_class9_cc_ror(&m, 8, &o) == 3 && a4_v == 1 && cc_added == 44);
    assert(aoe_ai_unit_ai_type30_cc_ror(&m, 8, &o) == 3 && c5c_id == 66);
    m.type = 0x20f;
    assert(aoe_ai_unit_ai_type30_cc_ror(&m, 8, &o) == 2);
    m.type = 0x300;
    assert(aoe_ai_unit_ai_type30_cc_ror(&m, 8, &o) == 8 && bcc2 == 1);
    /* Type 0x7e: target 3 away, reach 2: pass on; message 600 timer. */
    o.object = obj_any;
    o.self = me2;
    o.distance_to = t_dist3;
    o.clear_target = cc_clear;
    o.restart_timer = t_rs;
    o.difficulty = t_diff;
    m.type = 0x1fc;
    assert(aoe_ai_unit_ai_7e_cc_ror(&ai, &ord, &m, 5, &o) == 5 && bcc2 == 2);
    m.type = 0x1fa; m.value = 600;
    o.settings_8fc = o.settings_97d = 1;
    aoe_ai_unit_ai_7e_cc_ror(&ai, &ord, &m, 5, &o);
    assert(ai.timer == 25000 && t_restart == 25000);
}

static int32_t bd4 = 5;
static int32_t t_bd4(void *c, int32_t f) { (void)c; (void)f; return bd4; }
static int32_t t_rand7(void *c) { (void)c; return 7; }
static int32_t t_e0(void *c, int32_t a, int32_t b) { (void)c; (void)a; (void)b; return 77; }
static int32_t c70_id = -1;
static int32_t t_70(void *c, int32_t id, int32_t v) { (void)c; (void)v; c70_id = id; return 1; }
static int32_t t_40_2(void *c) { (void)c; return 9; }
static double t_rng(void *c) { (void)c; return 5.0; }
static int e8_kind = -1;
static void t_e8(void *c, int32_t k, int32_t s) { (void)c; (void)s; e8_kind = k; }
static int alerted = -1;
static void t_alert(void *c, int32_t a) { (void)c; alerted = a; }

static void priest_more_tests(void) {
    AoeAiUnitAiOpsRor o;
    AoeAiUnitAiOrderRor ord = {-1, 9, 0x25b};
    AoeAiUnitAiMessageRor m = {44, 0x1f9, 0, 1};
    memset(&o, 0, sizeof o);
    o.owner_field_4 = pd_owner4;
    o.base_d4 = t_bd4;
    o.nearby = p_nearby;
    o.rand = t_rand7;
    o.call_e0 = t_e0;
    o.call_70 = t_70;
    o.call_40_2 = t_40_2;
    o.object = obj_any;
    o.self = me0;
    o.range_10c = t_rng;
    o.move = mv;
    /* Band 3 holds 2; 7 % 4 = 3 > 2: heal 77. */
    assert(aoe_ai_unit_ai_priest_d4_ror(&o) == 6 && c70_id == 77);
    bd4 = 6;
    assert(aoe_ai_unit_ai_priest_d4_ror(&o) == 6);
    bd4 = 5;
    o.call_e0 = NULL;
    mx = 0.0f;
    /* Target at x 3 within range 5: step back 2. */
    assert(aoe_ai_unit_ai_priest_d4_ror(&o) == 6 && mx == -2.0f);
    /* 0x004e5370: a healing priest re-heals. */
    o.call_e0 = t_e0;
    c70_id = -1;
    assert(aoe_ai_unit_ai_priest_cc_ror(&ord, &m, 1, &o) == 3 && c70_id == 77);
    /* 0x004e5c00 */
    o.owner_e8 = t_e8;
    o.alert_allies = t_alert;
    o.group_add = cc_add;
    m.type = 500;
    assert(aoe_ai_unit_ai_class14_cc_ror(&ord, &m, 1, &o) == 3 && e8_kind == 0x201 &&
           alerted == -1 && cc_added == 44);
    m.field_10 = 0;
    assert(aoe_ai_unit_ai_class14_cc_ror(&ord, &m, 1, &o) == 4 && e8_kind == 0x209);
}

static int32_t t_dock(void *c, int32_t w) { (void)c; return w == 1 ? 33 : -1; }
static int32_t t_7c(void *c, int32_t id, int32_t v) { (void)c; (void)v; return id == 33; }
static int g_has;
static int t_has(void *c, int32_t id) { (void)c; (void)id; return g_has; }
static int32_t removed = -1;
static void t_rm(void *c, int32_t id) { (void)c; removed = id; }
static int resent;
static void t_10(void *c) { (void)c; ++resent; }

static void attacked_tests(void) {
    AoeAiUnitAiOpsRor o;
    AoeAiUnitAiOrderRor ord = {-1, 9, 0x267};
    AoeAiUnitAiMessageRor m = {45, 0x202, 0, 1};
    memset(&o, 0, sizeof o);
    o.call_40_dock = t_dock;
    o.call_7c = t_7c;
    o.clear_target = cc_clear;
    assert(aoe_ai_unit_ai_class2_cc_ror(&ord, &m, 1, &o) == 6);
    ord.task = 0x25d;
    cc_cleared = 0;
    assert(aoe_ai_unit_ai_class2_cc_ror(&ord, &m, 1, &o) == 2 && cc_cleared == 2);
    /* 500: owner +4 set: re-send and +0xa4. */
    o.owner_field_4 = pd_owner4;
    o.call_10 = t_10;
    o.call_a4 = t_a4;
    o.group_add = cc_add;
    m.type = 500;
    ord.task = 0x25b;
    assert(aoe_ai_unit_ai_attacked_ror(&ord, &m, 1, &o) == 3 && resent == 1 && cc_added == 45);
    ord.task = 0x262;
    assert(aoe_ai_unit_ai_class2_cc_ror(&ord, &m, 1, &o) == 3 && resent == 1);
    /* 0x004e70c0 */
    o.group_has = t_has;
    o.group_remove = t_rm;
    o.base_414eb0 = t_eb;
    m.type = 699;
    assert(aoe_ai_unit_ai_5804_cc_ror(&ord, &m, 1, &o) == 2 && removed == -1);
    g_has = 1;
    assert(aoe_ai_unit_ai_5804_cc_ror(&ord, &m, 1, &o) == 2 && removed == 45);
    m.type = 500;
    m.field_10 = 0;
    assert(aoe_ai_unit_ai_5804_cc_ror(&ord, &m, 12, &o) == 4 && eb_arg == 12);
    m.type = 0x20f;
    assert(aoe_ai_unit_ai_5804_cc_ror(&ord, &m, 12, &o) == 2);
}

static float a0x = 0, a0y = 0;
static int32_t t_a0(void *c, float x, float y, float z, int32_t a, int32_t b) {
    (void)c; (void)z; (void)a; (void)b; a0x = x; a0y = y; return 1;
}
static int32_t reiss = -1;
static void t_reiss(void *c, int32_t a, int32_t f) { (void)c; (void)f; reiss = a; }
static int dodged;
static void t_dodge(void *c) { (void)c; dodged = 1; }
static int t_wtype(void *c, int32_t id, int16_t *t) { (void)c; (void)id; *t = 0x23; return 1; }
static double t_dist1(void *c, int32_t id) { (void)c; (void)id; return 1.0; }

static void cc_5564_tests(void) {
    AoeAiUnitAiOpsRor o;
    AoeAiUnitAiOrderRor ord = {5, 9, 600};
    AoeAiUnitAiMessageRor m = {45, 0x200, 0, 1};
    memset(&o, 0, sizeof o);
    o.self = me0;
    o.owner_field_4 = pd_owner4;
    o.range_10c = t_rng;
    o.object = obj_any;
    o.distance_to = t_dist1;
    o.reissue = t_reiss;
    o.call_a0 = t_a0;
    o.rand = t_rand7;
    o.world_type = t_wtype;
    o.set_dodged = t_dodge;
    o.base_cc = t_bcc;
    /* Target at x 3, range 5, distance 1: spare 4, step to x -4. */
    assert(aoe_ai_unit_ai_5564_cc_ror(&ord, &m, 1, &o) == 3 && reiss == 0x2bc && a0x == -4.0f);
    ord.task = 7;
    assert(aoe_ai_unit_ai_5564_cc_ror(&ord, &m, 1, &o) == 2);
    /* 0x20f from siege: x = 0 - 4 + 7. */
    m.type = 0x20f;
    assert(aoe_ai_unit_ai_5564_cc_ror(&ord, &m, 1, &o) == 3 && a0x == 3.0f && dodged == 1 &&
           reiss == 5);
}

static int t_info(void *c, int32_t id, int16_t *owner, int16_t *cls) {
    (void)c; *owner = (int16_t)(id == 46 ? 0 : 2); *cls = 10; return id != 99;
}
static int32_t t_c0(void *c, int32_t v) { (void)c; (void)v; return -1; }
static int32_t t_40a(void *c, int32_t a, int32_t b, int32_t cc, int32_t d) {
    (void)c; (void)a; (void)b; (void)cc; (void)d; return 88;
}
static int32_t c68_id = -1;
static int32_t t_68(void *c, int32_t id, int32_t v) { (void)c; (void)v; c68_id = id; return 1; }

static void villager_cc_tests(void) {
    AoeAiUnitAiOpsRor o;
    AoeAiUnitAiOrderRor ord = {-1, 46, 0x261};
    AoeAiUnitAiMessageRor m = {45, 500, 0, 1};
    memset(&o, 0, sizeof o);
    o.obj_info = t_info;
    o.owner_field_4 = pd_owner4;
    o.reissue = t_reiss;
    o.call_a4 = t_a4;
    o.call_5c = t_5c;
    o.base_cc = t_bcc;
    o.clear_target = cc_clear;
    /* A non-gaia attacker (45): re-send, 3. */
    reiss = -1;
    assert(aoe_ai_unit_ai_villager_cc_ror(&ord, &m, 1, &o) == 3 && reiss == -1);
    /* A gaia attacker (46) while busy: fight back, action 700. */
    m.sender = 46;
    ord.action = 0x2bf;
    assert(aoe_ai_unit_ai_villager_cc_ror(&ord, &m, 1, &o) == 3 && ord.action == 700 &&
           ord.field_2c == 100 && ord.flag_130 == 1 && c5c_id == 46);
    /* Already attacking: 2. */
    assert(aoe_ai_unit_ai_villager_cc_ror(&ord, &m, 1, &o) == 2);
    /* 0x1fb: a gatherer finds another resource. */
    m.type = 0x1fb;
    ord.field_38 = 7;
    o.call_c0 = t_c0;
    o.call_40_args = t_40a;
    o.call_68 = t_68;
    assert(aoe_ai_unit_ai_villager_cc_ror(&ord, &m, 1, &o) == 3 && c68_id == 88);
    /* No source: stop. */
    ord.field_38 = ord.field_5c = -1;
    cc_cleared = 0;
    assert(aoe_ai_unit_ai_villager_cc_ror(&ord, &m, 1, &o) == 2 && cc_cleared == 1);
    m.type = 0x1fc;
    assert(aoe_ai_unit_ai_villager_cc_ror(&ord, &m, 1, &o) == 1);
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

static double b_damage(void *c, int32_t id) { (void)c; return id == 41 ? 3.0 : 7.0; }
static int b_view(void *c, int32_t id, int16_t *cls, float *hp, int16_t *max, uint8_t *st) {
    (void)c; *cls = 0; *hp = id == 41 ? 4.0f : 9.0f; *max = 20; *st = id == 31 ? 3 : 2; return 1;
}
static int b_reports, b_last;
static void b_e8(void *c, int32_t kind, int32_t sender) { (void)c; assert(kind == 0x20d); ++b_reports; b_last = sender; }
static int32_t b_rand(void *c) { (void)c; return 12345; }
static int32_t b_dc = 8;
static int32_t b_call_dc(void *c) { (void)c; return b_dc; }
static int32_t b_30(void *c, int32_t a, int32_t b, int32_t d) { (void)c; assert(a == 1 && b == 0 && d == 0); return 55; }

static void base_method_tests(void) {
    AoeAiUnitAiOpsRor o;
    AoeAiUnitAiOrderRor ord;
    float v = -1.0f;
    memset(&o, 0, sizeof o);
    memset(&ord, 0, sizeof ord);
    o.refresh_nearby = p_refresh;
    o.nearby = p_nearby;
    o.target_view = b_view;
    o.damage_to = b_damage;
    o.owner_e8 = b_e8;
    o.rand = b_rand;
    /* Band 3: 42 takes more damage, 41 has fewer hit points. */
    assert(aoe_ai_unit_ai_most_damaged_ror(&o, &v) == 42 && v == 7.0f);
    assert(aoe_ai_unit_ai_weakest_ror(&o, &v) == 41 && v == 4.0f);
    /* Bands 3 and 2 (empty) reported; the timer is redrawn. */
    assert(aoe_ai_unit_ai_report_nearby_ror(&ord, &o) == 12 && b_reports == 2 &&
           ord.field_10c == 4345 && ord.field_108 == 0);
    /* +0xd4: 6 from +0xdc 8 on the first call, else from +0x5c. */
    o.call_dc = b_call_dc;
    o.call_30_args = b_30;
    o.call_5c_r = pd_5c;
    assert(aoe_ai_unit_ai_base_d4_ror(&o, 1) == 6);
    b_dc = 0;
    assert(aoe_ai_unit_ai_base_d4_ror(&o, 1) == 6);
    o.call_5c_r = NULL;
    assert(aoe_ai_unit_ai_base_d4_ror(&o, 0) == 5);
    refreshed = 0;
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

static const uint8_t nn_b1[16] = {10, 0, 0, 0, 5, 2, 0, 0, 11, 0, 0, 0, 3, 3, 0, 0};
static const uint8_t nn_b3[8] = {12, 0, 0, 0, 1, 2, 0, 0};
static int32_t nn_nearby(void *c, int b, const uint8_t **e) {
    (void)c;
    if (b == 1) { *e = nn_b1; return 2; }
    if (b == 3) { *e = nn_b3; return 1; }
    return 0;
}
static int nn_view(void *c, int32_t id, int16_t *cls, float *hp, int16_t *mx, uint8_t *st) {
    (void)c; (void)hp; (void)mx;
    *cls = id == 11 ? 4 : 3;
    *st = id == 12 ? 7 : 2;
    return 1;
}
static int nn_type(void *c, int32_t id, int16_t *t) { (void)c; *t = (int16_t)(id + 100); return id != 13; }
static void nn_self(void *c, AoeAiUnitViewRor *o) { (void)c; memset(o, 0, sizeof *o); o->id = 99; }
static int32_t nn_gcount(void *c) { (void)c; return 3; }
static int32_t nn_gat(void *c, int32_t i) { (void)c; return 11 + i; }
static double nn_dist(void *c, int32_t id) { (void)c; return id == 12 ? 2.0 : 4.0; }

static int pm_ready = 1, pm_runs[4], pm_value;
static int pm_ready_f(void *c) { (void)c; return pm_ready; }
static void pm_set(void *c, int32_t v) { (void)c; pm_value = v; }
static int pm_trig(void *c, int i, uint8_t *k, uint32_t *v, uint8_t *ix) {
    (void)c;
    *k = (uint8_t)(i == 0 ? 5 : i == 1 ? 4 : 4);
    *v = i == 1 ? 7u : 8u;
    *ix = (uint8_t)i;
    return 1;
}
static uint8_t pm_cat(void *c, int32_t id) { (void)c; return (uint8_t)(id == 42 ? 7 : 0xff); }
static void pm_run(void *c, uint8_t ix, int32_t a) { (void)c; (void)a; ++pm_runs[ix]; }

static void play_message_tests(void) {
    AoeAiUnitAiPlayOpsRor o = {NULL, pm_ready_f, pm_set, pm_trig, pm_cat, pm_run};
    int32_t m[3] = {9, 42, 500};
    aoe_ai_unit_ai_play_message_ror(m, &o);
    assert(pm_value == 9 && pm_runs[0] == 1 && pm_runs[1] == 1 && pm_runs[2] == 0);
    m[2] = 501;
    aoe_ai_unit_ai_play_message_ror(m, &o);
    assert(pm_runs[0] == 1);
    m[2] = 500;
    pm_ready = 0;
    aoe_ai_unit_ai_play_message_ror(m, &o);
    assert(pm_runs[0] == 1);
}

static void nearest_tests(void) {
    play_message_tests();
    AoeAiUnitAiOpsRor o;
    int32_t d, pl;
    float f;
    memset(&o, 0, sizeof o);
    o.nearby = nn_nearby; o.target_view = nn_view; o.world_type = nn_type; o.self = nn_self;
    o.group_count = nn_gcount; o.group_at = nn_gat; o.distance_to = nn_dist;
    assert(aoe_ai_unit_ai_nearest_nearby_ror(&o, -1, 0, -1, -1, &d, &pl) == 11 && d == 3 && pl == 2);
    assert(aoe_ai_unit_ai_nearest_nearby_ror(&o, 3, 0, -1, -1, &d, NULL) == 10 && d == 5);
    assert(aoe_ai_unit_ai_nearest_nearby_ror(&o, -1, 0, -1, 110, &d, NULL) == 10);
    assert(aoe_ai_unit_ai_nearest_nearby_ror(&o, -1, 1, -1, -1, &d, NULL) == -1 && d == -1);
    assert(aoe_ai_unit_ai_nearest_listed_ror(&o, &f) == 12 && f == 2.0f);
}

static void ai_kind_for_unit_tests(void) {
    int k = -1; int32_t a = 0;
    /* 0x004afbe0 */
    assert(!aoe_ai_unit_ai_kind_for_unit_ror(1, 99, &k, &a) && k == -1);
    assert(aoe_ai_unit_ai_kind_for_unit_ror(1, 83, &k, &a) && k == AOE_AI_UNIT_AI_KIND_5644 && a == 5);
    assert(aoe_ai_unit_ai_kind_for_unit_ror(0x1b, 0x17c, &k, &a) && k == AOE_AI_UNIT_AI_KIND_5724 && a == 10);
    assert(aoe_ai_unit_ai_kind_for_unit_ror(3, 5, &k, &a) && k == AOE_AI_UNIT_AI_KIND_5804);
    assert(aoe_ai_unit_ai_kind_for_unit_ror(10, 0x9d, &k, &a) && k == AOE_AI_UNIT_AI_CLASS_9);
    assert(aoe_ai_unit_ai_kind_for_unit_ror(10, 0x7e, &k, &a) && k == AOE_AI_UNIT_AI_TYPE_7E);
    assert(aoe_ai_unit_ai_kind_for_unit_ror(10, 1, &k, &a) && k == AOE_AI_UNIT_AI_CLASS_10);
    assert(aoe_ai_unit_ai_kind_for_unit_ror(0x12, 1, &k, &a) && k == AOE_AI_UNIT_AI_PRIEST);
    assert(!aoe_ai_unit_ai_kind_for_unit_ror(0x21, 1, &k, &a));
    assert(!aoe_ai_unit_ai_kind_for_unit_ror(0xb, 1, &k, &a));
    assert(aoe_ai_unit_ai_kind_for_unit_ror(0x22, 1, &k, &a) && k == AOE_AI_UNIT_AI_KIND_5564);
    assert(aoe_ai_unit_ai_kind_for_unit_ror(-1, 1, &k, &a) && k == AOE_AI_UNIT_AI_KIND_5564);
}


static int16_t sc_owner = 3; static float sc_sight = 6.5f; static int sc_has_ai = 1, sc_found = 1;
static int16_t sc_type = 0x76; static int32_t sc_x, sc_y, sc_r; static int16_t sc_from;
static int sc_target(void *c, int32_t id, int16_t *o, float *s, int *h) { (void)c; (void)id; *o = 7; *s = 4.9f; *h = sc_has_ai; return sc_found; }
static int16_t sc_own_owner(void *c) { (void)c; return sc_owner; }
static float sc_own_sight(void *c) { (void)c; return sc_sight; }
static int16_t sc_own_type(void *c) { (void)c; return sc_type; }
static void sc_tile(void *c, int32_t *x, int32_t *y) { (void)c; *x = 11; *y = 12; }
static int32_t sc_collect(void *c, int32_t x, int32_t y, int32_t r, int16_t owner, const int32_t *cl, int32_t n) {
    (void)c; (void)cl; sc_x = x; sc_y = y; sc_r = r; sc_from = owner; return n * 10;
}
static int32_t mem_zone_calls, mem_fail_at = -1; static int reach_at = 2;
static uint8_t mem_zone(void *c, const int32_t *ex, int32_t n) { (void)c; (void)ex; ++mem_zone_calls; return (uint8_t)(n + 1); }
static int mem_lookup(void *c, uint8_t z, const int32_t *ex, int32_t n, int32_t *obj, uint8_t *cost) {
    (void)c; (void)ex; if (n == mem_fail_at) return 0; *obj = 500 + z; *cost = (uint8_t)(n == 3 ? 50 : 4); return 1;
}
static float mem_sight(void *c) { (void)c; return 3.0f; }
static int mem_reach(void *c, int32_t obj) { (void)c; return obj == 500 + reach_at + 1; }

static void scan_tests(void) {
    AoeAiUnitAiScanOpsRor o = {NULL, sc_target, sc_own_owner, sc_own_sight, sc_own_type, sc_tile, sc_collect};
    AoeAiUnitAiMemoryOpsRor m = {NULL, mem_zone, mem_lookup, mem_sight, mem_reach};
    AoeAiUnitAiRor ai = {0};
    int flag = 0;
    uint32_t cost = 0;
    ai.classes[0] = 3; ai.classes[1] = 4; ai.class_count = 2;
    /* Order 0x2bd on a target with an AI: its owner and sight. */
    assert(aoe_ai_unit_ai_scan_ror(&ai, 0x2bd, 9, &flag, &o) == 20 && flag == 1 &&
           sc_from == 7 && sc_r == 4 && sc_x == 11 && sc_y == 12);
    assert(aoe_ai_unit_ai_scan_ror(&ai, 0x2bd, 9, &flag, &o) == 0); /* once only */
    flag = 0; sc_has_ai = 0;
    assert(aoe_ai_unit_ai_scan_ror(&ai, 0x2bd, 9, &flag, &o) == 20 && sc_from == 3 && sc_r == 12);
    flag = 0; sc_type = 1;
    assert(aoe_ai_unit_ai_scan_ror(&ai, 1, -1, &flag, &o) == 20 && sc_r == 6);
    flag = 0; ai.class_count = 0; ai.classes[0] = 0;
    assert(aoe_ai_unit_ai_scan_ror(&ai, 1, -1, &flag, &o) == 0 && flag == 1);
    /* The remembered object: the third zone is reachable (cost 4). */
    assert(aoe_ai_unit_ai_remembered_ror(&m, &cost) == 503 && cost == 4 && mem_zone_calls == 3);
    reach_at = 10; /* the fourth try costs 50 > 3 + 5: -1 */
    assert(aoe_ai_unit_ai_remembered_ror(&m, &cost) == -1);
    mem_fail_at = 1; reach_at = 5;
    assert(aoe_ai_unit_ai_remembered_ror(&m, NULL) == -1);
}

static int32_t ry_counts[3] = {0, 2, 1}; static int32_t ry_objs[3][2] = {{0, 0}, {1, 2}, {3, 0}};
static int ry_has[4] = {1, 1, 1, 1}; static int16_t ry_owner[4] = {0, 2, 3, 2}; static uint8_t ry_state[4]; static int16_t ry_cls[4];
static float ry_dist[4] = {0, 1.0f, 2.0f, 5.0f}; static int ry_idle[4] = {1, 1, 1, 1}; static int ry_orders; static int16_t ry_last_owner;
static int ry_target(void *c, int32_t id, float *x, float *y, int16_t *o) { (void)c; (void)id; *x = 7.0f; *y = 8.0f; *o = 9; return 1; }
static void ry_tell(void *c) { (void)c; }
static int32_t ry_bcount(void *c, int b) { (void)c; return ry_counts[b]; }
static int32_t ry_bobj(void *c, int b, int32_t i) { (void)c; return ry_objs[b][i]; }
static int ry_object(void *c, int32_t id, int *h, int16_t *o, uint8_t *s, int16_t *cl) { (void)c; *h = ry_has[id]; *o = ry_owner[id]; *s = ry_state[id]; *cl = ry_cls[id]; return 1; }
static int ry_allied(void *c, int16_t a, int16_t b) { (void)c; return a == 3 && b == 2; }
static float ry_distance(void *c, int32_t id) { (void)c; return ry_dist[id]; }
static int ry_is_idle(void *c, int32_t id) { (void)c; return ry_idle[id]; }
static float ry_range(void *c) { (void)c; return 4.5f; }
static void ry_order(void *c, int16_t owner, int32_t code, int32_t target, int16_t ours, float x, float y, float r) {
    (void)c; assert(code == 700 && target == 55 && ours == 2 && x == 7.0f && y == 8.0f && r == 4.5f); ry_last_owner = owner; ++ry_orders;
}
static void rally_tests(void) {
    AoeAiUnitAiRallyOpsRor o = {NULL, ry_target, ry_tell, ry_bcount, ry_bobj, 2, 2, ry_object, ry_allied, ry_distance, ry_is_idle, ry_range, ry_order};
    /* Objects 1 (ours) and 3 (allied owner 3? no: object 3 is owner 2 but 5.0 away): only 1 rallies;
       object 2 is ours by ID and skipped. */
    assert(aoe_ai_unit_ai_rally_ror(55, &o) == 1 && ry_last_owner == 2);
    ry_dist[3] = 2.5f; ry_owner[3] = 3; /* allied and near now */
    assert(aoe_ai_unit_ai_rally_ror(55, &o) == 2 && ry_last_owner == 3);
    ry_cls[1] = 0x12; ry_idle[3] = 0;
    assert(aoe_ai_unit_ai_rally_ror(55, &o) == 0);
}
static int ft_blocked(void *c, int32_t x, int32_t y) { (void)c; return x == 5 && y == 5; }
static uint8_t ft_zone(void *c, int32_t x, int32_t y) { (void)c; (void)y; return x == 4 ? 2 : 1; }
static int ft_accept(void *c, float x, float y) { (void)c; (void)x; (void)y; return 1; }
static int ft_filt(void *c, int32_t x, int32_t y) { (void)c; return x == 6 && y == 5; }
static int32_t ft_rand(void *c) { (void)c; return 0; }
static int32_t ft_move(void *c, int32_t x, int32_t y, float r) { (void)c; (void)x; (void)y; (void)r; return 7; }
static void free_tile_tests(void) {
    AoeAiUnitAiFreeTileOpsRor o = {NULL, 5.5f, 5.5f, 1.0f, 100, 100, 0, -1.0f, -1.0f, ft_rand, 1,
                                   ft_blocked, ft_zone, ft_accept, ft_filt, ft_move};
    int32_t x = -1, y = -1;
    /* (5,5) blocked; (5,4) is the first tile at distance 1. */
    assert(aoe_ai_unit_ai_free_tile_ror(0, 0, &x, &y, &o) == 7 && x == 5 && y == 4);
    o.has_point = 1; o.px = 7.0f; o.py = 5.0f; /* (6,5) best, filtered out -> (6,4)/(6,6) tie: first */
    assert(aoe_ai_unit_ai_free_tile_ror(0, 1, &x, &y, &o) == 7 && x == 6 && y == 4);
    assert(aoe_ai_unit_ai_free_tile_ror(0x2c1, 0, &x, &y, &o) == 0);
}

static int ma_free[64][64], ma_log_n; static char ma_log[512]; static int32_t ma_move_ok = 1, ma_order_set; static float ma_tx, ma_ty;
static int32_t ma_place(void *c, int32_t x, int32_t y, int32_t z) { (void)c; assert(z == 3); ma_log_n += snprintf(ma_log + ma_log_n, sizeof ma_log - (size_t)ma_log_n, "p(%d,%d)", (int)x, (int)y); return ma_free[y][x]; }
static float ma_dist(void *c, float x, float y, float z) { (void)c; (void)z; return (x - 10.0f) * (x - 10.0f) + (y - 10.0f) * (y - 10.0f); }
static int32_t ma_move(void *c, float x, float y, float z, float r, int32_t f) { (void)c; assert(z == 3.5f && r == 0.5f && f == 1); ma_log_n += snprintf(ma_log + ma_log_n, sizeof ma_log - (size_t)ma_log_n, "m(%g,%g)", (double)x, (double)y); return ma_move_ok; }
static void ma_order(void *c, int32_t code) { (void)c; ma_order_set = code; }
static void ma_target(void *c, int32_t id, float x, float y, float z) { (void)c; assert(id == -1 && z == 3.5f); ma_tx = x; ma_ty = y; }
static void move_aside_tests(void) {
    AoeAiUnitAiMoveAsideOpsRor o = {NULL, 5, 64, 64, 3.2f, ma_place, ma_dist, ma_move, ma_order, ma_target};
    assert(aoe_ai_unit_ai_move_aside_ror(&o, 20.5f, 20.5f, 3.5f, 0, 0) == 0 && ma_log_n == 0);
    /* The point's tile is free: ordered straight there; -0.5 clamps to 0, 70 to the last tile. */
    ma_free[20][20] = 1;
    assert(aoe_ai_unit_ai_move_aside_ror(&o, 20.5f, 20.5f, 3.5f, 0, 1) == 1 && strcmp(ma_log, "p(20,20)m(20.5,20.5)") == 0 && ma_order_set == 0x262 && ma_tx == 20.5f);
    ma_log_n = 0; ma_free[0][63] = 1;
    assert(aoe_ai_unit_ai_move_aside_ror(&o, 70.0f, -0.5f, 3.5f, 0, 1) == 1 && strcmp(ma_log, "p(63,0)m(63.5,0.5)") == 0 && ma_tx == 63.0f && ma_ty == 0.0f);
    /* Blocked: the ring from (18, 18); steps nearer (10, 10) than the point skip the next one. */
    ma_log_n = 0; ma_free[20][20] = 0; ma_free[22][22] = 1;
    assert(aoe_ai_unit_ai_move_aside_ror(&o, 20.5f, 20.5f, 3.5f, 1, 1) == 1 && strcmp(ma_log, "p(20,20)p(18,18)p(20,18)p(22,18)p(22,20)p(22,22)m(22.5,22.5)") == 0);
    /* With the distance check every ring step but (20, 20) lies nearer (10, 10) and skips its
       successor, so the blocked tile is the only one tried. */
    ma_log_n = 0;
    assert(aoe_ai_unit_ai_move_aside_ror(&o, 20.5f, 20.5f, 3.5f, 0, 1) == 0 && strcmp(ma_log, "p(20,20)p(20,20)") == 0);
    ma_log_n = 0; ma_free[22][22] = 0;
    assert(aoe_ai_unit_ai_move_aside_ror(&o, 20.5f, 20.5f, 3.5f, 1, 1) == 0 && strstr(ma_log, "p(20,22)p(18,22)p(18,20)") != NULL && strstr(ma_log, "m(") == NULL);
    ma_log_n = 0; ma_free[20][20] = 1; ma_move_ok = 0; ma_order_set = 0;
    assert(aoe_ai_unit_ai_move_aside_ror(&o, 20.5f, 20.5f, 3.5f, 0, 1) == 0 && ma_order_set == 0);
}

/* ---- 0x00414600 ---- */
typedef struct { int32_t id; float x, y, z; uint8_t state; int16_t owner, tmpl, cls; int has_ai; } CkObj;
static CkObj ck_objs[4]; static int ck_nobj; static char ck_log[512]; static int ck_n; static int32_t ck_nearest = -1, ck_11c = -1, ck_place = 1, ck_reach = 1, ck_mtarget = -1, ck_d4 = 0; static float ck_dist = 0.0f;
static void ck_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); ck_n += vsnprintf(ck_log + ck_n, sizeof ck_log - (size_t)ck_n, fmt, ap); va_end(ap); }
static int ck_7c(void *c) { (void)c; return 1; }
static int ck_resolve(void *c, int32_t id, float *x, float *y, float *z, uint8_t *st, int16_t *ow, int16_t *tm, int16_t *cl, int *ai) {
    int i; (void)c;
    for (i = 0; i < ck_nobj; ++i) if (ck_objs[i].id == id) { *x = ck_objs[i].x; *y = ck_objs[i].y; *z = ck_objs[i].z; *st = ck_objs[i].state; *ow = ck_objs[i].owner; *tm = ck_objs[i].tmpl; *cl = ck_objs[i].cls; *ai = ck_objs[i].has_ai; return 1; }
    return 0;
}
static int32_t ck_nearestf(void *c) { (void)c; return ck_nearest; }
static float ck_range(void *c, int32_t e) { (void)c; return 3.0f + (float)e; }
static void ck_order(void *c, int32_t code, int32_t t, int32_t ow, float x, float y, float z, float r) { (void)c; ck_add("order(%d,%d,%d,%g,%g,%g,%g) ", (int)code, (int)t, (int)ow, (double)x, (double)y, (double)z, (double)r); }
static void ck_clear(void *c) { (void)c; ck_add("clear "); }
static void ck_stop(void *c, int32_t f) { (void)c; ck_add("stop%d ", (int)f); }
static int32_t ck_11cf(void *c) { (void)c; return ck_11c; }
static float ck_dpoint(void *c, float x, float y, float z) { (void)c; (void)x; (void)y; (void)z; return ck_dist; }
static float ck_dto(void *c, int32_t id) { (void)c; (void)id; return ck_dist; }
static int32_t ck_placef(void *c, int32_t tx, int32_t ty, float r) { (void)c; ck_add("place(%d,%d,%g) ", (int)tx, (int)ty, (double)r); return ck_place; }
static int32_t ck_reachf(void *c, int32_t id, float r) { (void)c; ck_add("reach(%d,%g) ", (int)id, (double)r); return ck_reach; }
static void ck_move(void *c, float x, float y, float z, float r) { (void)c; ck_add("move(%g,%g,%g,%g) ", (double)x, (double)y, (double)z, (double)r); }
static void ck_moveobj(void *c, int32_t id, float r, int f) { (void)c; ck_add("follow(%d,%g,%d) ", (int)id, (double)r, f); }
static int32_t ck_mtargetf(void *c, int32_t id) { (void)c; (void)id; return ck_mtarget; }
static void ck_5c(void *c, int32_t t) { (void)c; ck_add("5c(%d) ", (int)t); }
static int32_t ck_d4f(void *c, int32_t a) { (void)c; (void)a; return ck_d4; }
static void check_tests(void) {
    float tx = -1.0f, ty = -1.0f, tz = 0.0f; int32_t follow = -1;
    AoeAiUnitAiCheckOpsRor o = {NULL, -1, 600, -1, 2, &tx, &ty, &tz, 2.0f, &follow, 0, 1, ck_7c, 7, 0.0f, ck_resolve, ck_nearestf, ck_range, ck_order, ck_clear, ck_stop, ck_11cf, ck_dpoint, ck_dto, ck_placef, ck_reachf, ck_move, ck_moveobj, ck_mtargetf, ck_5c, ck_d4f};
    CkObj t = {50, 10.0f, 11.0f, 0.0f, 2, 3, 5, 6, 1}, b = {51, 20.0f, 21.0f, 1.0f, 2, 4, 9, 3, 0};
    ck_objs[0] = t; ck_objs[1] = b; ck_nobj = 2;
    /* 700 (order 600 + 100): a nearer enemy building other than the target is left (9); a unit is attacked. */
    ck_nearest = 51;
    assert(aoe_ai_unit_ai_check_ror(&o) == 9 && ck_n == 0);
    ck_objs[1].cls = 4;
    assert(aoe_ai_unit_ai_check_ror(&o) == 9 && strcmp(ck_log, "order(700,51,4,20,21,1,5) ") == 0);
    ck_n = 0; o.target = 50;  /* a unit target: a non-building candidate is refused */
    assert(aoe_ai_unit_ai_check_ror(&o) == 9 && ck_n == 0);
    ck_objs[0].cls = 3;
    assert(aoe_ai_unit_ai_check_ror(&o) == 9 && strcmp(ck_log, "order(700,51,4,20,21,1,5) ") == 0);
    /* 0x2bd: the follow object is gone -> cleared; present and far -> followed (10). */
    ck_n = 0; o.action = 0x2bd; follow = 99; o.order = -1;
    assert(aoe_ai_unit_ai_check_ror(&o) == 9 && follow == -1 && strcmp(ck_log, "clear stop1 ") == 0);
    ck_n = 0; follow = 50; ck_dist = 10.0f;
    assert(aoe_ai_unit_ai_check_ror(&o) == 10 && tx == 10.0f && ty == 11.0f && strcmp(ck_log, "reach(50,2) follow(50,2,1) ") == 0);
    ck_n = 0; ck_dist = 1.0f; ck_mtarget = 77;
    assert(aoe_ai_unit_ai_check_ror(&o) == 10 && strcmp(ck_log, "5c(77) ") == 0);
    ck_n = 0; ck_mtarget = -1; ck_d4 = 6;
    assert(aoe_ai_unit_ai_check_ror(&o) == 10 && ck_n == 0);
    ck_d4 = 0;
    assert(aoe_ai_unit_ai_check_ror(&o) == 9);
    ck_n = 0; follow = -1; tx = 4.5f; ty = 6.5f; ck_dist = 9.0f;
    assert(aoe_ai_unit_ai_check_ror(&o) == 10 && strcmp(ck_log, "place(4,6,2) move(4.5,6.5,0,2) ") == 0);
    /* 0x2c8: a target that moved and is out of range is followed without the flag. */
    ck_n = 0; o.action = 0x2c8; o.target = 50; tx = 1.0f; ty = 1.0f;
    assert(aoe_ai_unit_ai_check_ror(&o) == 10 && strcmp(ck_log, "follow(50,2,0) ") == 0 && tx == 10.0f);
    ck_n = 0; ck_dist = 1.0f;
    assert(aoe_ai_unit_ai_check_ror(&o) == 9 && ck_n == 0);
    ck_n = 0; o.target = 60;
    assert(aoe_ai_unit_ai_check_ror(&o) == 9 && strcmp(ck_log, "clear stop1 ") == 0);
    ck_n = 0; o.target = -1; ck_dist = 5.0f;
    assert(aoe_ai_unit_ai_check_ror(&o) == 10 && strcmp(ck_log, "move(10,11,0,2) ") == 0);
}
/* ---- 0x004102f0 / 0x0040fbf0 ---- */
typedef struct { int32_t id; float x, y, z, hp; uint8_t state; int16_t max, owner; int has_ai; } PlObj;
static PlObj pl_objs[8]; static int pl_nobj; static char pl_log[2048]; static int pl_n; static AoeAiPlayRuleRor pl_rule; static const AoeAiPlayRuleRor *pl_rule_ptr = &pl_rule; static int32_t pl_leader_pick = -1, pl_gather = 2, pl_fire_ret; static float pl_dist = 5.0f; static int pl_busy;
static void pl_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); pl_n += vsnprintf(pl_log + pl_n, sizeof pl_log - (size_t)pl_n, fmt, ap); va_end(ap); }
static const AoeAiPlayRuleRor *pl_rulef(void *c, uint8_t s) { (void)c; pl_add("rule%d ", s); return pl_rule_ptr; }
static int pl_resolve(void *c, int32_t id, float *x, float *y, float *z, uint8_t *st, float *hp, int16_t *mx, int16_t *ow, int *ai) {
    int i; (void)c;
    for (i = 0; i < pl_nobj; ++i) if (pl_objs[i].id == id) { *x = pl_objs[i].x; *y = pl_objs[i].y; *z = pl_objs[i].z; *st = pl_objs[i].state; *hp = pl_objs[i].hp; *mx = pl_objs[i].max; *ow = pl_objs[i].owner; *ai = pl_objs[i].has_ai; return 1; }
    return 0;
}
static int pl_busyf(void *c, int32_t id) { (void)c; (void)id; return pl_busy; }
static int32_t pl_gatherf(void *c, uint8_t cat, uint8_t s) { (void)c; (void)cat; (void)s; return pl_gather; }
static void pl_sum(void *c, uint8_t cat) { (void)c; pl_add("sum%d ", cat); }
static void pl_purge(void *c) { (void)c; pl_add("purge "); }
static float pl_distf(void *c, float x, float y, float z) { (void)c; (void)x; (void)y; (void)z; return pl_dist; }
static void pl_order(void *c, int32_t m, int32_t code, int32_t t, int32_t ow, float x, float y, float z) { (void)c; pl_add("o(%d:%d,%d,%d,%g,%g,%g) ", (int)m, (int)code, (int)t, (int)ow, (double)x, (double)y, (double)z); }
static void pl_free(void *c) { (void)c; pl_add("free "); }
static int32_t pl_pick(void *c, int32_t m) { (void)c; pl_add("pick%d ", (int)m); return m == -1 ? -1 : pl_leader_pick; }
static int32_t pl_classes(void *c) { (void)c; return 2; }
static void pl_copy(void *c, int32_t m, int32_t k) { (void)c; pl_add("cls(%d,%d) ", (int)m, (int)k); }
static void pl_fill(void *c, const int32_t *ids, int32_t n) { (void)c; pl_add("fill%d:%d,%d ", (int)n, (int)ids[0], n > 1 ? (int)ids[1] : -1); }
static int pl_fire(void *c, uint8_t s, int32_t f) { (void)c; pl_add("fire(%d,%d) ", s, (int)f); return pl_fire_ret; }
static void play_tests(void) {
    AoeAiPlayStateRor m; int32_t group[3] = {7, 20, 21}; int i;
    AoeAiPlayOpsRor o = {NULL, &m, 1, pl_rulef, 7, group, 3, pl_resolve, pl_busyf, pl_gatherf, pl_sum, pl_purge, pl_distf, pl_order, pl_free, pl_pick, pl_classes, pl_copy, 45000, pl_fill, pl_fire};
    PlObj L = {7, 10.0f, 10.0f, 0.0f, 40.0f, 2, 40, 1, 1}, A = {20, 3.0f, 3.0f, 0.0f, 30.0f, 2, 40, 1, 1}, B = {21, 5.0f, 5.0f, 0.0f, 10.0f, 2, 40, 1, 1};
    pl_objs[0] = L; pl_objs[1] = A; pl_objs[2] = B; pl_nobj = 3;
    memset(&m, 0, sizeof m); for (i = 0; i < AOE_AI_PLAY_MEMBERS; ++i) { m.slots[i].id = -1; m.slots[i].category = 0xff; }
    m.slots[0].id = 20; m.slots[0].category = 1; m.slots[1].id = 21; m.slots[1].category = 2; m.leader = 7; m.target = 55; m.point[0] = 1; m.point[1] = 2; m.point[2] = 3; m.step = 4; m.start_time = 40000;
    memset(&pl_rule, 0, sizeof pl_rule);
    pl_rule.actions[0] = (AoeAiPlayActionRor){1, 1, 2, 3, 4};   /* Move category 1 to leader + (2, 3, 4) */
    pl_rule.actions[1] = (AoeAiPlayActionRor){2, 2, 0, 0, 0};   /* Attack the leader */
    pl_rule.actions[2] = (AoeAiPlayActionRor){2, 6, 0, 0, 0};   /* ResetHitPoints */
    pl_rule.actions[3] = (AoeAiPlayActionRor){1, 5, 2, 0, 0};   /* Heal category 2 */
    pl_rule.actions[4] = (AoeAiPlayActionRor){2, 4, 0, 0, 0};   /* Retreat */
    assert(aoe_ai_play_trigger_ror(&o, 3, 0) == 1 && m.step == 3);
    assert(strcmp(pl_log, "rule3 o(20:710,-1,-1,12,13,4) o(21:700,7,1,-1,-1,-1) sum2 o(20:703,21,-1,-1,-1,-1) o(21:710,-1,-1,1,2,3) ") == 0);
    pl_n = 0; pl_gather = 9; pl_busy = 1;  /* flag 1: busy members skipped, no hit point reset */
    assert(aoe_ai_play_trigger_ror(&o, 3, 1) == 1 && strcmp(pl_log, "rule3 ") == 0);
    pl_n = 0; pl_busy = 0;
    assert(aoe_ai_play_trigger_ror(&o, 3, 1) == 1 && strcmp(pl_log, "rule3 o(21:700,7,1,-1,-1,-1) o(20:703,21,-1,-1,-1,-1) o(21:710,-1,-1,1,2,3) ") == 0);
    pl_n = 0;
    assert(aoe_ai_play_trigger_ror(&o, 0xfe, 0) == 1 && strcmp(pl_log, "free ") == 0);
    pl_rule_ptr = NULL; assert(aoe_ai_play_trigger_ror(&o, 1, 0) == 0); pl_rule_ptr = &pl_rule;
    o.play_exists = 0; assert(aoe_ai_play_trigger_ror(&o, 1, 0) == 0); o.play_exists = 1;
    /* The update: a Time trigger (11) past 4 s fires step 9; its answer 1 ends the update. */
    pl_n = 0; pl_rule.triggers[0] = (AoeAiPlayTriggerRor){11, 4, 0, 9}; pl_fire_ret = 1;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(9,0) ") == 0);
    pl_n = 0; pl_fire_ret = 0; pl_rule.triggers[0].a = 5;  /* not yet: the step re-fires with flag 1 */
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(3,1) ") == 0);
    /* Death (2): one of the two category-1 members gone is 50 percent. */
    pl_n = 0; m.slots[2].id = 22; m.slots[2].category = 1; pl_rule.triggers[0] = (AoeAiPlayTriggerRor){2, 1, 50, 6};
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(6,0) fire(3,1) ") == 0);
    pl_n = 0; pl_rule.triggers[0].b = 51;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(3,1) ") == 0);
    /* HealthLost (3): total 40, 30 left -> 25 percent; HealOfGroup (9) waits for full health; DeathOfGroup (8). */
    pl_n = 0; m.totals[1] = 40; pl_rule.triggers[0] = (AoeAiPlayTriggerRor){3, 1, 25, 7}; pl_rule.triggers[1] = (AoeAiPlayTriggerRor){9, 1, 0, 8}; pl_rule.triggers[2] = (AoeAiPlayTriggerRor){8, 2, 0, 5};
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(7,0) fire(3,1) ") == 0);
    pl_n = 0; pl_rule.triggers[0].b = 26; pl_objs[1].hp = 40.0f;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(8,0) fire(3,1) ") == 0);
    pl_n = 0; pl_objs[1].hp = 30.0f; pl_objs[2].state = 4;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(5,0) fire(3,1) ") == 0);
    /* Gather (1): members of category 1 within 4 of the leader offset; a far one holds. */
    pl_n = 0; pl_objs[2].state = 2; m.slots[2].id = -1; m.slots[2].category = 0xff; pl_rule.triggers[0] = (AoeAiPlayTriggerRor){1, 4, 0, 2}; pl_rule.triggers[1].code = 0; pl_dist = 3.0f;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(2,0) fire(3,1) ") == 0);
    pl_n = 0; pl_dist = 6.0f;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "rule3 fire(3,1) ") == 0);
    /* A fresh play (0xfd): the class lists copied, the cast, the totals, step 0. */
    pl_n = 0; m.step = 0xfd;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "cls(7,0) cls(7,1) cls(20,0) cls(20,1) cls(21,0) cls(21,1) fill3:7,20 fire(0,0) ") == 0 && m.start_time == 45000 && m.totals[1] == 30 && m.totals[2] == 10);
    /* A gone leader: the first member with an AI picks one; none frees the members. */
    pl_n = 0; m.step = 3; m.leader = 99; pl_leader_pick = 20;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "pick-1 pick20 fire(3,0) rule3 fire(3,1) ") == 0 && m.leader == 20);
    pl_n = 0; m.leader = 99; pl_leader_pick = -1;
    aoe_ai_play_update_ror(&o);
    assert(strcmp(pl_log, "pick-1 pick20 pick21 free ") == 0);
}
int main(void) {
    free_tile_tests();
    ai_kind_for_unit_tests();
    nearest_tests();
    base_method_tests();
    priest_search_tests();
    d4_5564_tests();
    cc_5644_tests();
    cc_more_tests();
    priest_more_tests();
    attacked_tests();
    cc_5564_tests();
    villager_cc_tests();
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
    scan_tests();
    rally_tests();
    move_aside_tests();
    check_tests();
    play_tests();
    return 0;
}
