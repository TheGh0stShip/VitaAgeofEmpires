/* Patched RoR 1.0a tribe player command methods (0x004f10e0..0x004f2d60). */
#include "tribe_player_commands.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

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
    return 0;
}
