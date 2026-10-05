/* Patched RoR 1.0a computer-player composite (0x004c6960..0x004c8630). */
#include "ai_player.h"

#include <assert.h>
#include <stdio.h>
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

int main(void) {
    static AoeAiPlayerAiRor ai, back;
    static uint8_t buf[512];
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;
    AoeAiPlayerOpsRor ops;
    size_t n;

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
