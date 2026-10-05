/* Patched RoR 1.0a computer-player composite (0x004c6960..0x004c8630). */
#include "ai_player.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

static char trace[512];
static void note(const char *s) { strncat(trace, s, sizeof trace - strlen(trace) - 1); }
static void m_init(void *c, int m, AoeOutcomeStream *in) {
    char b[8]; (void)c; snprintf(b, sizeof b, "%c%d", in ? 'L' : 'I', m); note(b);
}
static void m_owner(void *c, int m) { char b[8]; (void)c; snprintf(b, sizeof b, "O%d", m); note(b); }
static void m_name(void *c, int m, int32_t p, const char *n) {
    char b[8]; (void)c; assert(p == 3 && !strcmp(n, "Bob")); snprintf(b, sizeof b, "N%d", m); note(b);
}
static void m_set(void *c, int m, int32_t v) { char b[8]; (void)c; assert(v == 5); snprintf(b, sizeof b, "S%d", m); note(b); }
static int m_save(void *c, int m, AoeOutcomeStream *out) {
    char b[8]; (void)c; snprintf(b, sizeof b, "W%d", m); note(b); aoe_stream_put_u8(out, (uint8_t)m); return 1;
}
static void m_release(void *c, int m) { char b[8]; (void)c; snprintf(b, sizeof b, "R%d", m); note(b); }
static void base_set(void *c, int32_t v) { (void)c; assert(v == 5); note("B"); }
static int32_t kinds[4][40], kind_n[4];
static void info_kind(void *c, int32_t k, int32_t v) { (void)c; kinds[k][kind_n[k]++] = v; }
static int32_t preset = -1;
static void personality(void *c, int32_t p) { (void)c; preset = p; }
static void t_added(void *c, int32_t id) { (void)c; (void)id; note("ta"); }
static void t_removed(void *c, int32_t id) { (void)c; (void)id; note("tr"); }
static void b_removed(void *c, int32_t id) { (void)c; (void)id; note("br"); }
static void i_added(void *c, int32_t id) { (void)c; (void)id; note("ia"); }
static void i_removed(void *c, int32_t id) { (void)c; (void)id; note("ir"); }
static int32_t detasked = -1;
static void detask(void *c, int32_t id) { (void)c; detasked = id; }
static int object_class(void *c, int32_t id, int16_t *cls) { (void)c; *cls = (int16_t)(id % 100); return id >= 0; }
static int32_t kicked = -1;
static void kick(void *c, int32_t k) { (void)c; kicked = k; }
static int32_t sn_125 = 60, sn_126 = 20;
static int32_t sn(void *c, int32_t i) { (void)c; return i == 0x7d ? sn_125 : i == 0x7e ? sn_126 : 0; }
static int32_t rnd(void *c) { (void)c; return 37; }

/* Handler tests (0x004c8210..0x004c86a0): a trace of the calls. */
static char ht[1024];
static void hn(const char *f, int32_t a, int32_t b, int32_t c) {
    char x[48];
    snprintf(x, sizeof x, "%s(%d,%d,%d) ", f, a, b, c);
    strncat(ht, x, sizeof ht - strlen(ht) - 1);
}
static int h_object(void *c, int32_t id, int16_t *t, float pos[3]) {
    (void)c;
    if (id == 3) return 0;
    *t = (int16_t)(id == 2 ? 0x6d : 0x53);
    pos[0] = 1.0f; pos[1] = 2.0f; pos[2] = 3.0f;
    return 1;
}
static void h_build(void *c, int32_t id) { (void)c; hn("build", id, 0, 0); }
static void h_place(void *c, float x, float y, float z) { (void)c; hn("place", (int)x, (int)y, (int)z); }
static void h_add(void *c, int32_t p, int k, int32_t d) { (void)c; hn("add", p, k, d); }
static void h_set(void *c, int32_t p, int k, int32_t v) { (void)c; hn("set", p, k, v); }
static void h_chg(void *c, int32_t p, int f) { (void)c; hn("chg", p, f, 0); }
static void h_req(void *c, int32_t p, int32_t s) { (void)c; hn("req", p, s, 0); }
static int h_stance(void *c, int32_t from, int slot, int32_t to) {
    (void)c;
    if (slot == 0x20) return from == 1 && to == 2;
    if (slot == 0x1c) return from == 2 && to == 3;
    if (slot == 0x18) return (from == 2 && to == 4) || (from == 1 && to == 5);
    return 0;
}
static void h_cmd(void *c, int32_t t, int32_t s) { (void)c; hn("cmd", t, s, 0); }
static void h_chat(void *c, int32_t to, int32_t id) { (void)c; hn("chat", to, id, 0); }
static void h_t28(void *c, int32_t p, int32_t v) { (void)c; hn("t28", p, v, 0); }
static void h_tribute(void *c, int32_t to, int32_t r, float a) { (void)c; hn("tribute", to, r, (int)a); }
static int h_market(void *c) { (void)c; return 1; }
static void h_scan(void *c) { (void)c; hn("scan", 0, 0, 0); }
static void h_tac(void *c, int32_t p, int32_t a, int32_t b, int32_t d) { (void)c; hn("tac", p, a, b + d); }
static void h_setsn(void *c, int32_t i, int32_t v) { (void)c; hn("sn", i, v, 0); }
static int32_t h_sn(void *c, int32_t i) { (void)c; return i == 0x80 ? 6 : i == 0xd9 ? 1 : 0; }
static int32_t h_kind(void *c) { (void)c; return 4; }
static int32_t h_f4(void *c, int32_t p) { (void)c; return p == 3; }
static void h_sub5(void *c, int32_t f) { (void)c; hn("sub5", f, 0, 0); }

static void handler_tests(void) {
    static AoeAiPlayerAiRor ai;
    AoeAiPlayerOpsRor ops;
    int32_t *ids = (int32_t *)malloc(2 * sizeof *ids);
    memset(&ops, 0, sizeof ops);
    ops.object_at = h_object; ops.build_object = h_build; ops.con_place = h_place;
    ops.dip_add = h_add; ops.dip_set = h_set; ops.dip_changeable = h_chg;
    ops.dip_request = h_req; ops.stance_test = h_stance;
    ops.diplomacy_command = h_cmd; ops.chat = h_chat; ops.tribe_28 = h_t28;
    ops.tribute = h_tribute; ops.has_market = h_market; ops.info_scan = h_scan;
    ops.tactical_tribute = h_tac; ops.set_sn = h_setsn; ops.sn = h_sn;
    ops.game_kind = h_kind; ops.player_field_4 = h_f4; ops.command_sub5 = h_sub5;
    ops.self = 1;
    ops.player_count = 6;

    /* 0x004c8210: three listed, capacity two (grown); 3 is missing. */
    ids[0] = 2; ids[1] = 3;
    ai.decision.ids = ids;
    ai.decision.capacity = 2;
    ai.decision.count = 3;
    aoe_ai_player_refresh_objects_ror(&ai, &ops);
    assert(ai.decision.capacity == 3 && ai.decision.ids[2] == 0);
    assert(strcmp(ht, "build(2,0,0) place(1,2,3) build(0,0,0) ") == 0);
    free(ai.decision.ids);

    /* 0x004c8300 */
    ht[0] = 0;
    ai.field_1258c = 1;
    aoe_ai_player_tribute_received_ror(&ai, 6, 3, 250, &ops);
    assert(ai.field_12598 == 250 && ht[0] == 0);
    aoe_ai_player_tribute_received_ror(&ai, 6, 1, 250, &ops);
    assert(ht[0] == 0);
    ai.field_125a0 = 300;
    aoe_ai_player_tribute_received_ror(&ai, 2, 0, 250, &ops);
    assert(strcmp(ht, "add(2,2,2) add(2,0,-2) ") == 0 && ai.field_125a4[2] == 250);
    ht[0] = 0;
    aoe_ai_player_tribute_received_ror(&ai, 2, 0, 50, &ops);
    assert(strcmp(ht, "add(2,2,0) add(2,0,0) cmd(2,2,0) chat(2,2311,0) "
                      "cmd(3,2,0) cmd(4,0,0) sn(217,0,0) ") == 0);
    ht[0] = 0;
    aoe_ai_player_tribute_received_ror(&ai, 5, 0, 100, &ops);
    assert(strcmp(ht, "add(5,2,1) add(5,0,-1) chat(5,2310,0) ") == 0);

    /* 0x004c8540 */
    ht[0] = 0;
    aoe_ai_player_befriend_ror(&ai, &ops);
    assert(strcmp(ht, "set(6,0,0) set(6,2,100) t28(6,3,0) chat(6,2301,0) ") == 0 &&
           ai.field_1258c == 0 && ai.field_12594 == 1);

    /* 0x004c86a0 */
    ht[0] = 0;
    assert(aoe_ai_player_message_ror(&ai, 2, 0, 0, 0, 0, &ops) == 1);
    assert(aoe_ai_player_message_ror(&ai, 2, 1, 3, 40, 0, &ops) == 1);
    assert(aoe_ai_player_message_ror(&ai, 1, 1, 3, 40, 0, &ops) == 1);
    assert(aoe_ai_player_message_ror(&ai, 2, 3, 7, 8, 9, &ops) == 1);
    assert(aoe_ai_player_message_ror(&ai, 2, 4, 0, 0, 0, &ops) == 0);
    assert(strcmp(ht, "scan(0,0,0) tribute(2,3,40) tac(2,7,17) ") == 0);
    ht[0] = 0;
    assert(aoe_ai_player_message_ror(&ai, 2, 6, 4, 0, 0, &ops) == 1);
    assert(strcmp(ht, "chg(4,1,0) set(4,0,100) req(4,3,0) chg(4,0,0) ") == 0);
    ht[0] = 0;
    aoe_ai_player_message_ror(&ai, 2, 6, 4, 1, 0, &ops);
    assert(strcmp(ht, "chg(4,1,0) set(4,0,49) set(4,2,49) req(4,1,0) chg(4,0,0) ") == 0);
    ht[0] = 0;
    aoe_ai_player_message_ror(&ai, 2, 6, 3, 2, 0, &ops);
    aoe_ai_player_message_ror(&ai, 2, 6, 4, 2, 0, &ops);
    assert(strcmp(ht, "chg(3,1,0) set(3,2,100) req(3,0,0) sub5(0,0,0) chg(3,0,0) "
                      "chg(4,1,0) set(4,2,100) req(4,0,0) sub5(1,0,0) chg(4,0,0) ") == 0);
    ht[0] = 0;
    aoe_ai_player_message_ror(&ai, 2, 6, 4, 5, 0, &ops);
    assert(strcmp(ht, "chg(4,1,0) chg(4,0,0) ") == 0);
    assert(aoe_ai_player_message_ror(&ai, 2, 7, 900, 0, 0, &ops) == 1 &&
           ai.field_125a0 == 900);
}

int main(void) {
    static AoeAiPlayerAiRor ai, back;
    static uint8_t buf[512];
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;
    AoeAiPlayerOpsRor ops;
    size_t n;

    handler_tests();
    memset(&ops, 0, sizeof ops);
    ops.module_init = m_init;
    ops.module_owner = m_owner;
    ops.module_name = m_name;
    ops.module_set = m_set;
    ops.module_save = m_save;
    ops.module_release = m_release;
    ops.base_set = base_set;
    ops.info_kind = info_kind;
    ops.personality = personality;
    ops.tactical_added = t_added;
    ops.tactical_removed = t_removed;
    ops.build_removed = b_removed;
    ops.info_added = i_added;
    ops.info_removed = i_removed;
    ops.detask = detask;
    ops.object_class = object_class;
    ops.kick = kick;
    ops.sn = sn;
    ops.rand = rnd;

    aoe_ai_player_init_ror(&ai, 3, 7, "Bob", NULL, &ops);
    assert(!strcmp(trace, "I0I1I2I3I4I5I6I7I8O0O1O2O3O4O5O6O7O8N0N1N2N3N4N5N6N7N8"));
    assert(ai.setup_pending == 1 && ai.field_12574 == 1 && ai.update_count == 7 &&
           ai.decision.module.id == 1000 && ai.decision.module.player == 3 &&
           !strcmp(ai.decision.module.player_name, "Bob") && preset == 0);
    assert(kind_n[0] == 9 && kinds[0][8] == 0x32 && kind_n[1] == 4 + 20 + 6 &&
           kinds[1][4] == 0x86 && kinds[1][29] == 0xc5 && kind_n[2] == 1 &&
           kind_n[3] == 5 && kinds[3][4] == 0x153);

    trace[0] = 0;
    aoe_ai_player_set_ror(&ai, 5, &ops);
    assert(!strcmp(trace, "S0S1S2S3S4S5S6S7S8B"));

    /* Objects. */
    trace[0] = 0;
    assert(aoe_ai_player_object_added_ror(&ai, 42, 1, &ops) == 1 &&
           ai.decision.count == 1 && !strcmp(trace, "taia"));
    assert(!aoe_ai_player_object_added_ror(&ai, 43, 0, &ops) && ai.decision.count == 1);
    trace[0] = 0;
    assert(aoe_ai_player_object_removed_ror(&ai, 42, &ops) == 1 &&
           ai.decision.count == 0 && !strcmp(trace, "trbrir"));

    /* Actions and classes. */
    assert(aoe_ai_player_action_class_ror(&ai, 0x265) == 4 &&
           aoe_ai_player_action_class_ror(&ai, 0x26c) == 3 &&
           aoe_ai_player_action_class_ror(&ai, 0x999) == -1);
    assert(aoe_ai_player_object_fits_ror(&ai, 104, 0x25a, &ops) &&
           !aoe_ai_player_object_fits_ror(&ai, 103, 0x25a, &ops) &&
           aoe_ai_player_object_fits_ror(&ai, 203, 0x26b, &ops) &&
           !aoe_ai_player_object_fits_ror(&ai, -1, 0x26b, &ops));
    assert(aoe_ai_player_active_unit_ror(&ops, 4) && aoe_ai_player_active_unit_ror(&ops, 0x22) &&
           !aoe_ai_player_active_unit_ror(&ops, 3) && !aoe_ai_player_active_unit_ror(&ops, 0x21) &&
           !aoe_ai_player_active_unit_ror(&ops, 0xe) && !aoe_ai_player_active_unit_ror(&ops, -5));
    aoe_ai_player_detask_ror(&ops, 9);
    assert(detasked == 9);

    /* Delay: 37 % 20 - 10 + 60. */
    aoe_ai_player_roll_delay_ror(&ai, &ops);
    assert(ai.tribute_delay == 67);
    sn_126 = 0;
    aoe_ai_player_roll_delay_ror(&ai, &ops);
    assert(ai.tribute_delay == 60);

    aoe_ai_player_kick_ror(&ai, 4, &ops);
    assert(kicked == -1);
    aoe_ai_player_kick_ror(&ai, 3, &ops);
    assert(kicked == 3);

    /* Save and load. */
    ai.field_125a4[8] = 88;
    ai.field_1259c = 5;
    ai.diplomacy_time = 1234;
    trace[0] = 0;
    assert(aoe_ai_player_save_ror(&ai, &out, &ops));
    assert(!strcmp(trace, "W0W1W2W3W4W5W6W7W8"));
    n = out.pos;
    assert(n == 4 + 9 + 4 * 20);
    in = (AoeOutcomeStream){buf, n, 0, 0};
    /* The module loaders are callbacks: skip the nine marker bytes. */
    {
        static uint8_t copy[512];
        size_t k;
        memcpy(copy, buf, 4);
        for (k = 13; k < n; ++k) copy[k - 9] = buf[k];
        in = (AoeOutcomeStream){copy, n - 9, 0, 0};
        trace[0] = 0;
        assert(aoe_ai_player_load_ror(&back, 3, "Bob", 7.08f, &in, NULL, &ops));
        assert(in.pos == n - 9 && back.setup_pending == 0 && back.update_count == 7 &&
               back.diplomacy_time == 1234 && back.tribute_delay == 60 &&
               back.field_1259c == 5 && back.field_125a4[8] == 88);
        assert(!strncmp(trace, "L0L1L2L3L4L5L6L7L8O0", 20));
        /* Before 7.08 the tail stays 0; before 1.2 only three values. */
        in = (AoeOutcomeStream){copy, n - 9, 0, 0};
        assert(aoe_ai_player_load_ror(&back, 3, "Bob", 7.0f, &in, NULL, &ops) &&
               in.pos == 4 + 9 * 4 && back.field_125a4[8] == 0 && back.tribute_delay == 60);
        in = (AoeOutcomeStream){copy, n - 9, 0, 0};
        assert(aoe_ai_player_load_ror(&back, 3, "Bob", 1.0f, &in, NULL, &ops) &&
               in.pos == 4 + 3 * 4 && back.tribute_delay == 0);
    }

    trace[0] = 0;
    aoe_ai_player_release_ror(&ai, &ops);
    assert(!strcmp(trace, "R8R7R6R5R4R3R2R1R0"));
    aoe_ai_player_release_ror(&back, NULL);
    puts("ai player ok");
    return 0;
}
