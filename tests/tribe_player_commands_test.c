/* Patched RoR 1.0a tribe player command methods (0x004f10e0..0x004f2d60). */
#include "tribe_player_commands.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "aoe_stream.h"

static AoeTribeCommandUnitRor u1 = {11, 2}, u2 = {12, 2}, bld = {30, 2};
static int16_t last_min = -9, last_cls = -9, last_8f = -9, units_n = 2;
static int16_t collect(void *c, int16_t m, int16_t cls, int16_t t, int16_t f, const AoeTribeCommandUnitRor **out) {
    (void)c; (void)t;
    last_min = m, last_cls = cls, last_8f = f;
    out[0] = &u1;
    out[1] = &u2;
    return units_n;
}
static int tstate = 2;
static int template_state(void *c, int16_t id) { (void)c; (void)id; return tstate; }
static int acks_b, acks_u;
static void ack_b(void *c) { (void)c; ++acks_b; }
static void ack_u(void *c, const AoeTribeCommandUnitRor *u) { (void)c; assert(u == &u1); ++acks_u; }
static const AoeTribeCommandUnitRor *sel = &bld;
static const AoeTribeCommandUnitRor *selected(void *c) { (void)c; return sel; }
static uint8_t otype = 'P';
static uint8_t object_type(void *c, const AoeTribeCommandUnitRor *o) { (void)c; (void)o; return otype; }
static int16_t ocls = 3;
static int16_t object_class(void *c, const AoeTribeCommandUnitRor *o) { (void)c; (void)o; return ocls; }
static uint8_t queued[0x200];
static size_t qsize;
static int queue(void *c, const uint8_t *cmd, size_t n) { (void)c; memcpy(queued, cmd, n); qsize = n; return 1; }

static void count_refresh(void *c) { ++*(int32_t *)c; }
static const AoeTribeCommandUnitRor *lookup_target;
static const AoeTribeCommandUnitRor *lookup_fn(void *c, int32_t id) { (void)c; (void)id; return lookup_target; }
static int f_kicks, f_added, f_trained[4], f_ids[3], f_tributes, f_sample = -9;
static char f_log[64];
static void f_kick(void *c, int32_t k) { (void)c; f_kicks += k; }
static int32_t f_sn(void *c, int32_t i) { (void)c; return i * 2; }
static void f_logw(void *c, const char *l) { (void)c; strncpy(f_log, l, sizeof f_log - 1); }
static void f_added_fn(void *c, int32_t id) { (void)c; f_added = id; }
static void f_add(void *c, void *u, int32_t a, int32_t b, int32_t cc) { (void)c; (void)u; (void)a; (void)b; (void)cc; }
static int f_info(void *c, void *u, int16_t *cls, int32_t *id) { (void)c; *cls = *(int16_t *)u; *id = 42; return 1; }
static void f_train(void *c, int16_t p, int32_t a, int32_t b, int32_t cc) {
    (void)c; f_trained[0] = p; f_trained[1] = a; f_trained[2] = b; f_trained[3] = cc;
}
static void f_order(void *c, int16_t p, const int32_t *ids, int32_t n, int32_t a, int32_t b, int32_t cc, int32_t d) {
    (void)c; (void)a; (void)b; (void)cc; (void)d; f_ids[0] = p; f_ids[1] = ids[0]; f_ids[2] = n;
}
static void f_trib(void *c, int16_t p, int32_t a, int32_t r, float amt, int32_t b) {
    (void)c; (void)p; (void)a; (void)r; (void)amt; (void)b; ++f_tributes;
}
static void f_samp(void *c, int32_t skip, int32_t v) { (void)c; (void)skip; f_sample = v; }

static int32_t relay_calls, relay_sum;
static int32_t relay_msg(void *c, int32_t a, int32_t b, int32_t d, int32_t e, int32_t f) {
    (void)c; ++relay_calls; return a + b + d + e + f;
}
static void relay_place(void *c, int32_t a, int32_t b, int32_t d, float x, float y, int32_t e,
                        int32_t f) {
    (void)c; ++relay_calls; relay_sum = a + b + d + (int32_t)x + (int32_t)y + e + f;
}
static void relay_named(void *c, int32_t a, int32_t b) { (void)c; ++relay_calls; relay_sum = a * b; }
static void relay_done4(void *c, int32_t a, int32_t b, int32_t d, int32_t e) {
    (void)c; ++relay_calls; relay_sum = a + b + d + e;
}
static void relay_three(void *c, int32_t a, int32_t b, int32_t d) {
    (void)c; ++relay_calls; relay_sum = a - b - d;
}


static char tp_log[32];
static int tp_n, tp_host = 1;
static void tp_mark(char c) { tp_log[tp_n++] = c; tp_log[tp_n] = 0; }
static void tp_base(void *c, AoeOutcomeStream *s) { (void)c; (void)s; tp_mark('B'); }
static void tp_research(void *c, AoeOutcomeStream *s) { (void)c; aoe_stream_put_u8(s, 0xee); tp_mark('R'); }
static void tp_history(void *c, AoeOutcomeStream *s) { (void)c; (void)s; tp_mark('H'); }
static int32_t tp_is_host(void *c) { (void)c; return tp_host; }
static void tp_ai(void *c, AoeOutcomeStream *s) { (void)c; (void)s; tp_mark('A'); }
static void tp_free(void *c, void *o) { (void)c; tp_mark(*(char *)o); }
static void tp_destroy(void *c) { (void)c; tp_mark('Z'); }

static void test_tribe_player_save(void) {
    AoeTribePlayerSaveOpsRor ops = {NULL, 1, tp_base, tp_research, tp_history, tp_is_host, tp_ai};
    AoeTribePlayerSaveRor p = {9, 0x228, 0x22c, 0x230, 0x234, 0x238, 0x844, 0x848, 1, 1, 2, 3, 4};
    uint8_t buf[128];
    AoeOutcomeStream s = {0};
    s.data = buf; s.size = sizeof buf;
    aoe_tribe_player_save_ror(&p, &s, &ops);
    assert(strcmp(tp_log, "BRHA") == 0 && s.pos == 1 + 16 + 1 + 4 + 8 + 4);
    assert(buf[0] == 9 && buf[1] == 0x2c && buf[5] == 0x34 && buf[9] == 0x28 && buf[13] == 0x38);
    assert(buf[17] == 0xee && buf[18] == 0x30 && buf[22] == 0x44 && buf[30] == 1);
    tp_n = 0; tp_host = 0; s.pos = 0;
    aoe_tribe_player_save_timed_ror(&p, &s, &ops);
    assert(strcmp(tp_log, "BRH") == 0 && s.pos == 34 + 16);
    assert(buf[30] == 0 && buf[34] == 1 && buf[38] == 2 && buf[42] == 4 && buf[46] == 3);
    tp_n = 0; p.type = 2; s.pos = 0;
    aoe_tribe_player_save_ror(&p, &s, &ops);
    assert(s.pos == 30);
    {
        char r = 'r', a = 'a', h = 'h';
        AoeTribePlayerMembersRor m = {NULL, &r, &a, &h, tp_free, tp_free, tp_free, tp_destroy};
        tp_n = 0;
        aoe_tribe_player_destroy_ror(&m);
        assert(strcmp(tp_log, "rahZ") == 0 && !m.research && !m.ai && !m.history);
        tp_n = 0; m.ai = &a;
        aoe_tribe_player_destroy_ror(&m);
        assert(strcmp(tp_log, "aZ") == 0);
    }
}


static int ao_class = 3, ao_id = 5, ao_added, ao_event = -9;
static uint8_t ao_state;
static int32_t ao_base(void *c, void *u, int32_t a, int32_t b) { (void)c; (void)u; return a + b; }
static int ao_info(void *c, void *u, int16_t *cl, int32_t *id) { (void)c; (void)u; *cl = (int16_t)ao_class; *id = ao_id; return 1; }
static void ao_ai(void *c, void *u) { (void)c; (void)u; ++ao_added; }
static void ao_build(void *c, void *u, int32_t arg) { (void)c; (void)u; ao_event = arg; }
static uint8_t ao_game(void *c) { (void)c; return ao_state; }

static void test_tribe_add_object(void) {
    AoeTribeAddObjectOpsRor ops = {NULL, 1, ao_base, ao_info, ao_ai, ao_build, ao_game};
    int unit = 0;
    assert(aoe_tribe_add_object_ror(&ops, &unit, 2, 3) == 5 && ao_added == 1 && ao_event == 1);
    ao_state = 3;
    aoe_tribe_add_object_ror(&ops, &unit, 0, 0);
    assert(ao_added == 2 && ao_event == 0);
    ao_class = 11; ao_event = -9;
    aoe_tribe_add_object_ror(&ops, &unit, 0, 0);
    assert(ao_added == 2 && ao_event == -9);
    ao_class = 3; ao_id = -1;
    aoe_tribe_add_object_ror(&ops, &unit, 0, 0);
    ao_id = 5; ops.has_ai = 0;
    assert(aoe_tribe_add_object_ror(&ops, &unit, 1, 1) == 2 && ao_added == 2);
}

static int rp_allies = 3; /* bit 0: 2 -> other, bit 1: other -> 2 */
static int rp_allied(void *c, int16_t from, int16_t to) {
    (void)c; (void)to; return from == 2 ? (rp_allies & 1) : (rp_allies & 2) != 0;
}
static int rp_acks;
static void rp_ack(void *c, const AoeTribeCommandUnitRor *u, const AoeTribeCommandUnitRor *t) {
    (void)c; assert(u == &u1 && t->id == 99); ++rp_acks;
}

static void test_tribe_repair(void) {
    AoeTribePlayerOpsRor ops = {0, 2, collect, template_state, ack_b, ack_u, selected, object_type, object_class, queue};
    AoeTribeCommandUnitRor other = {99, 5}, mine = {99, 2};
    AoeTribeRepairTargetRor t = {&mine, 'P', 3, 10.0f, 20, 2};
    units_n = 2;
    qsize = 0;
    assert(aoe_tribe_player_repair_ror(&ops, &t, rp_allied, rp_ack) == 1);
    assert(last_min == 4 && last_cls == 4 && last_8f == -1 && rp_acks == 1 && qsize > 0 && queued[0] == 0x6e);
    t.hp = 20.0f;
    assert(aoe_tribe_player_repair_ror(&ops, &t, rp_allied, rp_ack) == 0);
    t.hp = 10.0f;
    t.state = 3;
    assert(aoe_tribe_player_repair_ror(&ops, &t, rp_allied, rp_ack) == 0);
    t.state = 0;
    t.unit = &other;
    rp_allies = 1; /* one-way alliance: the building path still works */
    assert(aoe_tribe_player_repair_ror(&ops, &t, rp_allied, rp_ack) == 1);
    t.type = 'F';
    t.cls = 20; /* a ship needs both ways */
    assert(aoe_tribe_player_repair_ror(&ops, &t, rp_allied, rp_ack) == 0);
    rp_allies = 3;
    assert(aoe_tribe_player_repair_ror(&ops, &t, rp_allied, rp_ack) == 1);
    t.cls = 6;
    assert(aoe_tribe_player_repair_ror(&ops, &t, rp_allied, rp_ack) == 0);
    t.cls = 2;
    units_n = 0;
    assert(aoe_tribe_player_repair_ror(&ops, &t, rp_allied, rp_ack) == 0);
    units_n = 2;
}

static int tlo_hits[3];
static void *tlo_combat(void *c, void *s, void *p) { (void)c; (void)p; ++tlo_hits[0]; return s; }
static void *tlo_building(void *c, void *s, void *p) { (void)c; (void)s; ++tlo_hits[1]; return p; }
static void *tlo_base(void *c, uint8_t k, void *s, void *p) { (void)c; (void)s; (void)p; ++tlo_hits[2]; return k == 10 ? c : NULL; }

static void test_tribe_load_object(void) {
    int ctx, stream, player;
    AoeTribeLoadObjectOpsRor ops = {&ctx, tlo_combat, tlo_building, tlo_base};
    assert(aoe_tribe_load_object_ror(&ops, 'F', &stream, &player) == &stream);
    assert(aoe_tribe_load_object_ror(&ops, 'P', &stream, &player) == &player);
    assert(aoe_tribe_load_object_ror(&ops, 10, &stream, &player) == &ctx);
    assert(aoe_tribe_load_object_ror(&ops, 'Z', &stream, &player) == NULL);
    assert(tlo_hits[0] == 1 && tlo_hits[1] == 1 && tlo_hits[2] == 2);
}

static int ttl_last;
static void *ttl_f(void *c, void *s, int32_t a, int32_t b) { (void)c; (void)a; (void)b; ttl_last = 'F'; return s; }
static void *ttl_p(void *c, void *s, int32_t a, int32_t b) { (void)c; (void)s; ttl_last = 'P'; return a == 7 && b == 8 ? c : NULL; }
static void *ttl_z(void *c, void *s, int32_t a, int32_t b) { (void)c; (void)s; (void)a; (void)b; ttl_last = 'Z'; return NULL; }
static void ttl_base(void *c, void *s, int16_t slot, uint8_t k, int32_t a, int32_t b) {
    (void)c; (void)s; (void)a; (void)b; assert(slot == 3); ttl_last = k;
}

static void test_tribe_template_load(void) {
    int ctx, stream;
    void *slots[4] = {&ctx, &ctx, &ctx, &ctx};
    AoeTribeTemplateLoadOpsRor ops = {&ctx, slots, ttl_f, ttl_p, ttl_z, ttl_base};
    aoe_tribe_template_load_ror(&ops, &stream, 0, 'F', 7, 8);
    assert(slots[0] == &stream && ttl_last == 'F');
    aoe_tribe_template_load_ror(&ops, &stream, 1, 'P', 7, 8);
    assert(slots[1] == &ctx && ttl_last == 'P');
    aoe_tribe_template_load_ror(&ops, &stream, 2, 'Z', 7, 8);
    assert(slots[2] == NULL && ttl_last == 'Z');
    aoe_tribe_template_load_ror(&ops, &stream, 3, 10, 7, 8);
    assert(slots[3] == &ctx && ttl_last == 10);
}


static char tl_log[64]; static int tl_n;
static void tl_tmpl(void *c, int16_t slot, uint8_t kind, AoeTribeTemplateKindRor which, AoeOutcomeStream *s) {
    (void)c; (void)s; tl_n += snprintf(tl_log + tl_n, sizeof tl_log - (size_t)tl_n, "T%d%c%d", (int)slot, (char)kind, (int)which);
}
static void tl_map(void *c, AoeOutcomeStream *s) { (void)c; (void)s; tl_n += snprintf(tl_log + tl_n, sizeof tl_log - (size_t)tl_n, "M"); }
static void tl_mem(void *c, AoeOutcomeStream *s) { (void)c; (void)s; tl_n += snprintf(tl_log + tl_n, sizeof tl_log - (size_t)tl_n, "O"); }
static void tl_new(void *c, int w) { (void)c; tl_n += snprintf(tl_log + tl_n, sizeof tl_log - (size_t)tl_n, "n%d", w); }
static void tl_load(void *c, int w, AoeOutcomeStream *s) { (void)c; (void)s; tl_n += snprintf(tl_log + tl_n, sizeof tl_log - (size_t)tl_n, "l%d", w); }
static void load_templates_tests(void) {
    AoeTribePlayerLoadOpsRor o = {NULL, tl_tmpl, tl_map, tl_mem, tl_new, tl_load};
    uint8_t buf[64]; AoeOutcomeStream s = {0};
    s.data = buf; s.size = sizeof buf;
    aoe_stream_put(&s, "\x03\x00", 2);
    aoe_stream_put_i32(&s, 1); aoe_stream_put_i32(&s, 0); aoe_stream_put_i32(&s, 1);
    aoe_stream_put_u8(&s, 'F'); aoe_stream_put_u8(&s, 'Q');
    s.size = s.pos; s.pos = 0;
    assert(aoe_tribe_player_load_templates_ror(&s, 7.23f, &o) == 3);
    assert(strcmp(tl_log, "T0F1T2Q0MOn0n1n2l0l1l2") == 0);
    assert(aoe_tribe_template_kind_ror('P') == AOE_TRIBE_TEMPLATE_P && aoe_tribe_template_kind_ror('Z') == AOE_TRIBE_TEMPLATE_Z);
    tl_n = 0; tl_log[0] = 0; s.pos = 0;
    assert(aoe_tribe_player_load_templates_ror(&s, 7.0f, &o) == 3 && strstr(tl_log, "l1l2") == NULL);
    s.pos = 0; s.size = 1;
    assert(aoe_tribe_player_load_templates_ror(&s, 7.23f, &o) == -1);
}
static int m_exists = 1, m_live = 1, m_mode = 0x6b, m_find = 1, m_log[8];
static int m_ex(void *c, int32_t id, float *x, float *y) { (void)c; (void)id; if (x) *x = 3.0f, *y = 4.0f; return m_exists; }
static int16_t m_tid(void *c, int32_t id) { (void)c; return (int16_t)(id + 1); }
static int m_live_f(void *c) { (void)c; return m_live; }
static int m_ours(void *c, int32_t id) { (void)c; return id == 5; }
static int32_t m_mode_f(void *c) { (void)c; return m_mode; }
static void m_6b(void *c) { (void)c; ++m_log[0]; }
static void m_fwd(void *c, int32_t i, int32_t f, int32_t k, int32_t a, int32_t b, int32_t d) { (void)c; (void)i; (void)f; (void)a; (void)b; (void)d; m_log[1] = k; }
static int m_can(void *c, int32_t id) { (void)c; (void)id; return 1; }
static int m_fp(void *c, int32_t id, float x, float y, int32_t *px, int32_t *py) { (void)c; (void)id; assert(x == 3.0f && y == 4.0f); *px = 7; *py = 8; return m_find; }
static void m_goto(void *c, int32_t id, int32_t px, int32_t py) { (void)c; (void)id; m_log[2] = px * 10 + py; }
static void m_fb(void *c, int32_t id) { (void)c; (void)id; ++m_log[3]; }
static void m_flash(void *c, int16_t t, int32_t a, int32_t b) { (void)c; assert(a == 0 && b == 10); m_log[4] = t; }
static void m_20a(void *c, int32_t id, int32_t a, int32_t b) { (void)c; (void)id; (void)a; (void)b; ++m_log[5]; }
static void m_20d(void *c, int32_t a) { (void)c; m_log[6] = a; }
static void test_message_router(void) {
    AoeTribeMessageOpsRor o = {0, m_ex, m_tid, m_live_f, m_ours, m_mode_f, m_6b, m_fwd, m_can, m_fp, m_goto, m_fb, m_flash, m_20a, m_20d};
    aoe_tribe_player_message_ror(&o, 5, 0, 0x1f9, 0x25d, 0, 0);
    assert(m_log[0] == 1);
    aoe_tribe_player_message_ror(&o, 6, 0, 0x1f9, 0x25d, 0, 0);
    assert(m_log[0] == 1);
    aoe_tribe_player_message_ror(&o, 5, 0, 0x202, 0x26f, 0, 0);
    assert(m_log[2] == 78);
    m_find = 0;
    aoe_tribe_player_message_ror(&o, 5, 0, 0x202, 0x25d, 0, 0);
    assert(m_log[3] == 1);
    aoe_tribe_player_message_ror(&o, 5, 0, 0x209, 41, 0, 0);
    assert(m_log[4] == 42);
    aoe_tribe_player_message_ror(&o, 5, 0, 0x20d, 9, 0, 0);
    assert(m_log[6] == 9);
    m_exists = 0; /* only 0x20a/0x72/0x74 pass the gate */
    aoe_tribe_player_message_ror(&o, 5, 0, 0x20d, 3, 0, 0);
    assert(m_log[6] == 9);
    aoe_tribe_player_message_ror(&o, 5, 0, 0x74, 0, 0, 0);
    assert(m_log[1] == 0x74);
    aoe_tribe_player_message_ror(&o, 5, 0, 0x201, 0, 0, 0);
    assert(m_log[1] == 0x74);
    aoe_tribe_player_message_ror(&o, 5, 0, 0x20a, 0, 0, 0);
    assert(m_log[5] == 1);
}

static int t_log[8];
static void t_sample(void *c, int32_t s, int32_t v) { (void)c; (void)s; t_log[0] = v; ++t_log[1]; }
static int t_full(void *c) { (void)c; return 0; }
static float t_frac(void *c) { (void)c; return 0.5f; }
static void t_notify(void *c, int32_t k) { (void)c; t_log[2] = k; }
static void t_base(void *c) { (void)c; ++t_log[3]; }
static int t_ready(void *c) { (void)c; return 1; }
static int32_t t_fc(void *c) { (void)c; return -1; }
static void t_sig(void *c, int32_t v) { (void)c; t_log[4] = v; }
static void t_ai(void *c, int32_t v) { (void)c; (void)v; ++t_log[5]; }
static int t_flag(void *c) { (void)c; return 1; }
static void t_hook(void *c) { (void)c; ++t_log[6]; }
static void test_player_tick(void) {
    AoeTribeTickOpsRor o = {0, t_sample, t_full, t_frac, t_notify, t_base, t_ready, t_fc, t_sig, t_ai, t_flag, t_hook};
    AoeTribeTickRecordRor r = {0};
    AoeTribeTickUnitRor units[2] = {{11, 2}, {4, 1}};
    AoeTribeTickStateRor p = {0};
    r.score = 42.6f; r.r50 = 1.0f; r.r4c = 50.0f; r.r24 = 99.0f; r.r28 = 2.0f; r.r00 = 10.0f; r.r84 = 1.0f; r.rb4 = 7;
    p.counter = 14; p.down_22c = 1; p.player_id = 1; p.has_ai = 1; p.rec = &r; p.units = units; p.unit_count = 2;
    aoe_tribe_player_tick_ror(&p, 1.5f, &o);
    assert(p.counter == 0 && t_log[1] == 1 && t_log[0] == 42);
    assert(r.explored == 50.0f && r.margin == 0.0f && r.r24 == 100.0f && p.down_22c == 0);
    assert(t_log[2] == 2 && r.rb4 == 0 && p.accum == 0.5f);
    assert(t_log[3] == 1 && t_log[4] == 3 && t_log[5] == 1 && t_log[6] == 1);
    assert(r.r00 > 8.93f && r.r00 < 8.94f); /* 10 - 42.6 * 1.5 / 60 */
    units[1].state = 2; t_log[2] = 0; r.rb4 = 7;
    aoe_tribe_player_tick_ror(&p, 0.6f, &o);
    assert(t_log[2] == 0 && r.rb4 == 7 && p.counter == 1);
}


static float tl_attr48 = 0.0f; static int tl_owns6d = 0;
static int tl_info(void *c, int16_t slot, uint8_t *kind, uint8_t *en, int16_t *at, int16_t *f4c, const char **name) {
    (void)c;
    static const char *names[] = {"Villager", "Axeman", "Boat", "Priest"};
    if (slot == 0x6d) { *kind = 'P'; *en = 0; *at = -1; *f4c = 9; *name = "Wonder"; return 1; }
    if (slot < 0 || slot > 3) return 0;
    *kind = slot == 2 ? 'E' : 'F'; *en = slot != 3; *at = slot == 0 ? 0x76 : 12; *f4c = (int16_t)(slot * 10); *name = names[slot]; return 1;
}
static void tl_costs(void *c, int16_t slot, int16_t t[3], int16_t a[3]) { (void)c; t[0] = 0; t[1] = 3; t[2] = -1; a[0] = (int16_t)(50 + slot); a[1] = 20; a[2] = 0; }
static int32_t tl_help(void *c, int16_t s) { (void)c; return 1000 + s; }
static int32_t tl_tree(void *c, int16_t s) { (void)c; return 2000 + s; }
static float tl_a48(void *c) { (void)c; return tl_attr48; }
static int tl_owns(void *c, int16_t id) { (void)c; (void)id; return tl_owns6d; }
static char dump_log[2048]; static int dump_n;
static void dp_civ(void *c, char *o, int n) { (void)c; snprintf(o, (size_t)n, "Greek"); }
static const char *dp_bl(void *c) { (void)c; return "bl"; }
static const char *dp_cp(void *c) { (void)c; return "cp"; }
static const char *dp_vc(void *c) { (void)c; return "vc"; }
static int32_t dp_pts(void *c, int k) { (void)c; return 10 + k; }
static int32_t dp_ev(void *c, int i) { (void)c; return i; }
static int32_t dp_smp(void *c, int i) { (void)c; return 100 + i; }
static int32_t dp_score(void *c) { (void)c; return 999; }
static float dp_attr(void *c, int i) { (void)c; return 5.0f + (float)i; }
static void dp_bai(void *c, char *o, int n) { (void)c; snprintf(o, (size_t)n, "Build AI (last a, last2 b, Progress c, Phase d)"); }
static void dp_civs(void *c, char *o, int n) { (void)c; snprintf(o, (size_t)n, "Civilians (Total 1, Exp 2 3 4 5 6, Need 7 8 9 10)"); }
static int32_t dp_sold(void *c) { (void)c; return 17; }
static void dp_line(void *c, const char *t) { (void)c; dump_n += snprintf(dump_log + dump_n, sizeof dump_log - (size_t)dump_n, "%s|", t); }
static void train_and_dump_tests(void) {
    AoeTribeTrainListOpsRor o = {NULL, 4, tl_info, tl_costs, tl_help, tl_tree, tl_a48, tl_owns};
    AoeTribeDumpOpsRor d = {NULL, 3, 1, dp_civ, dp_bl, dp_cp, dp_vc, dp_pts, dp_ev, dp_smp, dp_score, dp_attr, dp_bai, dp_civs, dp_sold, dp_line};
    AoeTribeTrainEntryRor *list = NULL; int16_t n = -1;
    /* Building 12: slot 1 (Axeman); slot 2 is kind 'E', slot 3 disabled. */
    aoe_tribe_player_train_list_ror(&o, 12, &list, &n);
    assert(n == 1 && list && strcmp(list[0].name, "Axeman") == 0 && list[0].id == 1 && list[0].field_4c == 10 &&
           list[0].cost_type[1] == 3 && list[0].cost_amount[0] == 51.0f && list[0].help == 1001 && list[0].tree_text == 2001);
    /* Building 0x76 with attribute 48 at 0: the Villager plus the Wonder entry. */
    aoe_tribe_player_train_list_ror(&o, 0x76, &list, &n);
    assert(n == 2 && list[0].id == 0 && list[1].id == 0x6d && strcmp(list[1].name, "Wonder") == 0);
    tl_owns6d = 1;
    aoe_tribe_player_train_list_ror(&o, 0x76, &list, &n);
    assert(n == 1);
    aoe_tribe_player_train_list_ror(&o, 99, &list, &n);
    assert(n == 0 && list == NULL);
    aoe_tribe_player_dump_ror(&d, 1);
    assert(strncmp(dump_log, "Player 3:|Computer Player: Greek|Build List: bl|City Plan: cp|VC File: vc|Points:|Combat 10|", 90) == 0);
    assert(strstr(dump_log, "Total: 999|Population: 16|Kills: 100|Kill Ratio: 102|Razings: 101|Food: 5|Gold: 8|Stone: 7|Wood: 6|Technology: 117|Exploration: 107|Build AI") != NULL &&
           strstr(dump_log, "|Number Soldiers 17|") != NULL);
    dump_n = 0; d.is_computer = 0;
    aoe_tribe_player_dump_ror(&d, 1);
    assert(strstr(dump_log, "Human Player: Greek|Points:") != NULL && strstr(dump_log, "Soldiers") == NULL);
}

static AoeTribeCursorTargetRor cc_t; static int cc_found = 1, cc_refresh; static int32_t cc_stance;
static int cc_target(void *c, int32_t id, AoeTribeCursorTargetRor *o) { (void)c; (void)id; *o = cc_t; return cc_found; }
static void cc_refresh_f(void *c) { (void)c; ++cc_refresh; }
static int32_t cc_stance_f(void *c, int32_t owner) { (void)c; assert(owner == 2); return cc_stance; }
static void cursor_command_tests(void) {
    /* Rows by class: class 0 -> stance 1 attack (1, 10), stance 0 action 0 (0, 20), stance 2 heal (4, 30),
       stance 3 repair (5, 40), stance 4 convert (7, 50); class 3 (buildings) row out of place at index 1. */
    static const AoeTribeCursorRowRor flagged[2] = {{{0, 1, 10, 0, 20, 4, 30, 5, 40, 7, 50}}, {{3, 8, 11, 9, 21, 2, 31, 3, 41, -1, 0}}};
    static const AoeTribeCursorRowRor plain[1] = {{{0, 0, 100, 6, 200, 0, 300, 0, 400, 0, 500}}};
    uint32_t flags = 1; int32_t cmd = -1, cur = -1;
    AoeTribeCursorOpsRor o = {NULL, cc_target, 4, cc_refresh_f, 1, &flags, flagged, 2, plain, 1, cc_stance_f, 0, -1, 1, 1};
    memset(&cc_t, 0, sizeof cc_t); cc_t.owner = 2; cc_t.max_hp = 40; cc_t.hp = 25.0f;
    cc_stance = 1;
    assert(aoe_tribe_player_cursor_command_ror(&o, 7, &cmd, &cur) == 1 && cmd == 4 && cur == 10 && cc_refresh == 1);
    cc_t.state = 3;  /* a dead target is not attacked: 0/0 -> plain table retried, flags cleared */
    assert(aoe_tribe_player_cursor_command_ror(&o, 7, &cmd, &cur) == 1 && cmd == 0 && cur == 100 && flags == 0);
    flags = 0x10; cc_stance = 2; cc_t.state = 0;
    assert(aoe_tribe_player_cursor_command_ror(&o, 7, &cmd, &cur) == 1 && cmd == 5 && cur == 30);
    o.priests = 1; o.last_priest = 7;  /* the selected priest itself is not healed */
    assert(aoe_tribe_player_cursor_command_ror(&o, 7, &cmd, &cur) == 1 && cmd == 0 && cur == 300);
    flags = 0x20; cc_stance = 3; cc_t.template_id = 0x9e;
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 1 && cmd == 3 && cur == 40 + 0x32);
    flags = 0x80; cc_stance = 4; cc_t.template_id = 0;
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 1 && cmd == 5 && cur == 50);
    /* Class 3 found by search; stance 1 action 8 without the attack bit retries as 7 with +0x32. */
    cc_t.unit_class = 3; cc_t.template_id = 0x6d; cc_stance = 1;
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 0 && flags == 0);  /* retried on the plain table, which has no class 3 */
    cc_t.template_id = 5; flags = 0x80;
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 1 && cmd == 5 && cur == 11 + 0x32);
    flags = 0x40; cc_stance = 0;  /* action 9 with bit 6 */
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 1 && cmd == 3 && cur == 21);
    cc_stance = 4;  /* action -1 */
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 0);
    cc_stance = 5;
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 0);
    cc_t.unit_class = 9;
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 0);
    cc_t.owner = 4;
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 0);
    cc_found = 0;
    assert(aoe_tribe_player_cursor_command_ror(&o, 8, &cmd, &cur) == 0 && cmd == 0 && cur == 0);
}

static int32_t st_rands[64], st_rn; static char st_log[1024]; static int st_n, st_block_main = 1, st_block_extra, st_anchor = 1; static int16_t st_tmpl = 0x6d;
static void st_add(const char *fmt, ...) { va_list ap; va_start(ap, fmt); st_n += vsnprintf(st_log + st_n, sizeof st_log - (size_t)st_n, fmt, ap); va_end(ap); }
static int32_t st_rand(void *c) { (void)c; return st_rands[st_rn++ & 63]; }
static int st_mblocked(void *c, float x, float y) { (void)c; st_add("mb(%g,%g) ", (double)x, (double)y); return st_block_main-- > 0; }
static void st_cmain(void *c, float x, float y) { (void)c; st_add("main(%g,%g) ", (double)x, (double)y); }
static int st_eblocked(void *c, float x, float y) { (void)c; st_add("eb(%g,%g) ", (double)x, (double)y); return st_block_extra-- > 0; }
static void st_cextra(void *c, float x, float y) { (void)c; st_add("extra(%g,%g) ", (double)x, (double)y); }
static void st_view(void *c, int32_t tx, int32_t ty, float x, float y) { (void)c; st_add("view(%d,%d,%g,%g) ", (int)tx, (int)ty, (double)x, (double)y); }
static void st_work(void *c, float a) { (void)c; st_add("work%g ", (double)a); }
static int st_anchorf(void *c, float *x, float *y, int16_t *t) { (void)c; *x = 40.5f; *y = 41.5f; *t = st_tmpl; return st_anchor; }
static int32_t st_stance(void *c, int32_t p) { (void)c; return p == 2 ? 0 : 1; }
static void st_reveal(void *c, int32_t p) { (void)c; st_add("reveal%d ", (int)p); }
static void st_wonder(void *c, int32_t p) { (void)c; st_add("wonder%d ", (int)p); }
static int32_t st_slot(void *c, int32_t s) { (void)c; return s == 3 ? 3 : s == 5 ? 1 : 0; }
static void st_recip(void *c, int32_t s, int32_t st) { (void)c; st_add("recip(%d,%d) ", (int)s, (int)st); }
static void place_start_tests(void) {
    AoeTribeStartOpsRor o = {NULL, 0, 100, 60, st_rand, st_mblocked, st_cmain, st_eblocked, st_cextra, st_view, st_work, st_anchorf, 1, 4, 1, 1, st_stance, st_reveal, st_wonder, st_slot, st_recip};
    /* Rolls: y 0x7fff -> 15 + 30 = 45.5, x 0 -> 15.5 (blocked); then y 0 -> 15.5, x 0x7fff -> 85.5 free. Extras: (ry, rx, fy, fx). */
    int32_t r[] = {0x7fff, 0, 0, 0x7fff, 0x7fff, 0, 0, 0, 0x7fff, 0x7fff, 0, 0, 0, 0x7fff, 0, 0, 0, 0, 0x7fff, 0x7fff}; int i;
    for (i = 0; i < 20; ++i) st_rands[i] = r[i];
    aoe_tribe_player_place_start_ror(&o);
    /* Player 1 is us, player 2 has no stance: only player 3 is shown the wonder; comm slot 3 holds it. */
    assert(strcmp(st_log, "mb(16,46) mb(86,16) main(85.5,15.5) view(85,15,85.5,15.5) work10000 eb(82.5,18.5) extra(82.5,18.5) eb(88.5,18.5) extra(88.5,18.5) eb(88.5,12.5) extra(88.5,12.5) reveal3 wonder3 recip(3,1) ") == 0);
    /* With units: the anchor's view; a non-wonder anchor is only revealed; not the viewed player: no recipients. */
    st_n = 0; o.has_units = 1; st_tmpl = 5; o.viewed_player = 2;
    aoe_tribe_player_place_start_ror(&o);
    assert(strcmp(st_log, "view(40,41,40.5,41.5) reveal3 ") == 0);
    st_n = 0; o.share = 0; st_anchor = 0;
    aoe_tribe_player_place_start_ror(&o);
    assert(st_n == 0);
}
int main(void) {
    AoeTribePlayerOpsRor ops = {0, 2, collect, template_state, ack_b, ack_u, selected, object_type, object_class, queue};
    /* Build: villagers (min 4, class 4), template 118 acknowledges, 0x66. */
    assert(aoe_tribe_player_build_ror(&ops, 70, 5.0f, 6.0f) && last_min == 4 && last_cls == 4 && acks_b == 1);
    assert(queued[0] == 0x66 && queued[1] == 2);
    tstate = 1;
    assert(!aoe_tribe_player_build_ror(&ops, 70, 5, 6));
    assert(aoe_tribe_player_build_ror(&ops, 0x6d, 5, 6)); /* 0x6d needs only presence */
    tstate = 0;
    assert(!aoe_tribe_player_build_ror(&ops, 0x6d, 5, 6));
    tstate = 2;
    units_n = 0;
    assert(!aoe_tribe_player_build_ror(&ops, 70, 5, 6));
    units_n = 2;
    assert(aoe_tribe_player_build_69_ror(&ops, 70, 1, 2, 3, 300) && queued[0] == 0x69);
    /* Train / research at the selected building. */
    assert(aoe_tribe_player_train_ror(&ops, 83) && queued[0] == 0x64 && qsize == 0x10);
    assert(aoe_tribe_player_research_ror(&ops, 5) && queued[0] == 0x65);
    otype = 'F';
    assert(!aoe_tribe_player_train_ror(&ops, 83));
    /* Delete. */
    assert(aoe_tribe_player_delete_selected_ror(&ops) && queued[0] == 0x6a);
    ocls = 1;
    assert(!aoe_tribe_player_delete_selected_ror(&ops));
    otype = 'D';
    assert(!aoe_tribe_player_delete_ror(&ops, &bld, 1));
    otype = 'P';
    assert(!aoe_tribe_player_delete_ror(&ops, &bld, 0));
    lookup_target = &bld;
    assert(aoe_tribe_player_delete_id_ror(&ops, lookup_fn, 5) && queued[0] == 0x6a);
    otype = 'D';
    assert(!aoe_tribe_player_delete_id_ror(&ops, lookup_fn, 5));
    otype = 'P';
    lookup_target = NULL;
    assert(!aoe_tribe_player_delete_id_ror(&ops, lookup_fn, 5));
    /* Selection commands acknowledge with the first unit. */
    assert(aoe_tribe_player_6d_ror(&ops, 7) && last_8f == 5 && acks_u == 1 && queued[0] == 0x6d);
    assert(aoe_tribe_player_6b_ror(&ops, 1, 2) && last_8f == 4 && queued[0] == 0x6b);
    assert(aoe_tribe_player_6f_ror(&ops, 1, 2) && last_8f == 7 && queued[0] == 0x6f);
    assert(aoe_tribe_player_6d_one_ror(&ops, 44, 3) && queued[0] == 0x6d);
    assert(aoe_tribe_player_66_one_ror(&ops, 44, 70, 1, 2, -1) && queued[0] == 0x66);
    assert(aoe_tribe_player_67_ror(&ops, 5, 9) && queued[0] == 0x67);
    /* Template and tech checks. */
    tstate = 1;
    assert(!aoe_tribe_player_template_usable_ror(&ops, 70) && aoe_tribe_player_template_usable_ror(&ops, 0x6d));
    {
        uint8_t entries[24] = {0};
        entries[8 + 4] = 1;
        assert(aoe_tribe_tech_state_is_one_ror(entries, 1) && !aoe_tribe_tech_state_is_one_ror(entries, 0));
    }
    assert(aoe_tribe_research_open_ror(2) && !aoe_tribe_research_open_ror(3) && !aoe_tribe_research_open_ror(-1));
    puts("tribe player commands ok");
    {
        AoeTribeForwardOpsRor f = {NULL, 0, 3, f_kick, f_sn, f_logw, f_added_fn, f_add, f_info,
                                   f_train, f_train, f_order, f_trib, f_samp};
        int16_t cls = 4;
        int32_t rec[4] = {1, 2, 3, 4};
        uint8_t buf[16];
        AoeOutcomeStream st = {buf, sizeof buf, 0, 0};
        aoe_tribe_ai_kick_ror(&f, 1);
        assert(f_kicks == 0 && aoe_tribe_ai_strategic_ror(&f, 5) == -1);
        aoe_tribe_ai_log_ror(&f, "x%d", 1);
        assert(f_log[0] == 0);
        f.has_ai = 1;
        aoe_tribe_ai_kick_ror(&f, 1);
        assert(f_kicks == 1 && aoe_tribe_ai_strategic_ror(&f, 5) == 10);
        aoe_tribe_ai_log_ror(&f, "x%d", 7);
        assert(!strcmp(f_log, "x7"));
        aoe_tribe_add_unit_ror(&f, &cls, 0, 0, 0);
        assert(f_added == 42);
        cls = 11;
        f_added = 0;
        aoe_tribe_add_unit_ror(&f, &cls, 0, 0, 0);
        assert(f_added == 0);
        aoe_tribe_queue_train_ror(&f, 10, 20, 30);
        assert(f_trained[0] == 3 && f_trained[1] == 20 && f_trained[2] == 10 && f_trained[3] == 30);
        aoe_tribe_queue_research_ror(&f, 1, 2, 3);
        assert(f_trained[1] == 2 && f_trained[2] == 1);
        aoe_tribe_order_one_ror(&f, 77, 0, 0, 0, 0);
        assert(f_ids[0] == 3 && f_ids[1] == 77 && f_ids[2] == 1);
        assert(!aoe_tribe_tribute_ror(&f, 0, -1, 5.0f, 0) && !aoe_tribe_tribute_ror(&f, 0, 1, 0.0f, 0));
        assert(aoe_tribe_tribute_ror(&f, 0, 1, 5.0f, 0) && f_tributes == 1);
        aoe_tribe_sample_score_ror(&f, -3.0f);
        assert(f_sample == 0);
        aoe_tribe_sample_score_ror(&f, 9.75f);
        assert(f_sample == 9);
        aoe_tribe_save_record_ror(rec, &st);
        assert(st.pos == 16 && buf[0] == 1 && buf[4] == 2 && buf[8] == 4 && buf[12] == 3);
    }
    {
        int a, b, c;
        AoeTribeListUnitRor u[4] = {{&a, 5, 2}, {&b, 6, 2}, {NULL, 5, 2}, {&c, 5, 1}};
        float secs = 0.5f;
        int32_t ctr = 29, flag = 0, refreshes = 0;
        assert(aoe_tribe_next_of_template_ror(u, 4, 5, &a) == &c);
        assert(aoe_tribe_next_of_template_ror(u, 4, 5, &c) == &a);
        assert(aoe_tribe_next_of_template_ror(u, 4, 5, NULL) == &a);
        assert(aoe_tribe_next_of_template_ror(u, 4, 7, NULL) == NULL);
        aoe_tribe_cycle_tick_ror(&secs, &ctr, 1.75f);
        assert(ctr == 1 && secs == 0.25f);
        assert(!aoe_tribe_shared_vision_check_ror(0.0f, &flag, count_refresh, &refreshes) && !flag);
        assert(aoe_tribe_shared_vision_check_ror(1.0f, &flag, count_refresh, &refreshes));
        assert(flag == 1 && refreshes == 1);
    }
    {
        AoeTribeAiRelayOpsRor r = {NULL, 0, relay_msg, relay_place, relay_named, relay_done4,
                                   relay_three, relay_three};
        assert(aoe_tribe_ai_message_ror(&r, 1, 2, 3, 4, 5) == 0);
        aoe_tribe_build_place_ror(&r, 1, 1, 1, 1.0f, 1.0f, 1, 1);
        aoe_tribe_build_named_ror(&r, 2, 3);
        aoe_tribe_build_done_4_ror(&r, 1, 2, 3, 4);
        aoe_tribe_build_done_3_ror(&r, 1, 2, 3);
        aoe_tribe_tactical_relay_ror(&r, 1, 2, 3);
        assert(relay_calls == 0);
        r.has_ai = 1;
        assert(aoe_tribe_ai_message_ror(&r, 1, 2, 3, 4, 5) == 15);
        aoe_tribe_build_place_ror(&r, 1, 2, 3, 4.5f, 5.0f, 6, 7);
        assert(relay_sum == 28);
        aoe_tribe_build_named_ror(&r, 2, 3);
        assert(relay_sum == 6);
        aoe_tribe_build_done_4_ror(&r, 1, 2, 3, 4);
        assert(relay_sum == 10);
        aoe_tribe_build_done_3_ror(&r, 9, 2, 3);
        assert(relay_sum == 4);
        aoe_tribe_tactical_relay_ror(&r, 10, 2, 3);
        assert(relay_sum == 5 && relay_calls == 6);
    }
    test_tribe_player_save();
    test_tribe_add_object();
    test_tribe_load_object();
    test_tribe_template_load();
    test_tribe_repair();
    load_templates_tests();
    test_message_router();
    test_player_tick();
    train_and_dump_tests();
    cursor_command_tests();
    place_start_tests();
    return 0;
}
