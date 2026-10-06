/* Patched RoR 1.0a tribe player command methods (0x004f10e0..0x004f2d60). */
#include "tribe_player_commands.h"

#include <assert.h>
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
    return 0;
}
