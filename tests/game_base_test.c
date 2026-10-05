/* Patched RoR 1.0a base game object pieces 0x00416e30..0x0041c290 and
   the player sync checksum 0x0045cec0..0x0045ce90. */
#include "game_base.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int refresh, mark, on, off, reset;
    uintptr_t system;
    int32_t comm;
} Env;

static void refresh(void *c) { ++((Env *)c)->refresh; }
static int32_t comm(void *c) { return ((Env *)c)->comm; }
static void mark(void *c) { ++((Env *)c)->mark; }
static void on(void *c) { ++((Env *)c)->on; }
static void off(void *c) { ++((Env *)c)->off; }
static void sys(void *c, uintptr_t v) { ((Env *)c)->system = v; }
static void reset(void *c) { ++((Env *)c)->reset; }
static int load(void *c, uint32_t id, char *out, int size) {
    (void)c;
    if (id != 5) return 0;
    strncpy(out, "Age of Empires", (size_t)size);
    return 1;
}

int main(void) {
    static AoeGameBaseRor g;
    Env env = {0};
    uintptr_t last = 0;
    AoeGameBaseOps ops = {&env, refresh, comm, mark, on, off, sys, &last,
                          reset, load};
    float attrs[3] = {100.9f, 20.5f, -3.5f};
    AoeChecksumObjectRor objs[2] = {{83, 2, 25.7f, 4, 1.5f, 2.0f, 0.25f},
                                    {12, 1, 10.0f, 0, 3.0f, 4.0f, 0.0f}};
    AoePlayerChecksumRor p1 = {2, 1, 3, attrs, objs, 2, 0, 0, 0, 7};
    AoePlayerChecksumRor p0 = {0, 0, 0, NULL, NULL, 0, 0, 0, 0, 0};
    AoePlayerChecksumRor *players[2] = {&p0, &p1};
    void *objects[2] = {&p0, &p1};
    uint32_t c;
    int32_t a, b, xy[2];
    uint32_t l, r, s, k;
    char text[8];

    /* Checksum: base 3, 2 objects, object sum 83+2+25+4 + 12+1+10+0 =
       137, resources 100 + 20 - 3 = 117, positions 10.75 -> 10. */
    assert(aoe_player_checksum_ror(&p1) ==
           ((((3u << 8) + 2u) << 8) + 137u) * 256u + 117u);
    assert(p1.valid && p1.position_sum == 10 && p1.field_1c == 0);
    assert(aoe_player_checksums_ror(&p1, &c, &a, &b) && a == 10);

    g.has_world = 1;
    g.player_count = 2;
    g.players = objects;
    g.checksums = players;
    assert(aoe_game_player_checksum_ror(&g, 1) == p1.checksum);
    g.view_player = 0;
    assert(aoe_game_player_checksum_ror(&g, -1) == 0 &&
           aoe_game_player_checksum_ror(&g, 2) == 0);
    assert(aoe_game_checksums_ror(&g, &c, &a, &b) && c == p1.checksum && a == 10);
    aoe_game_view_player_ror(&g, 1, &ops);
    assert(g.view_player == 1 && env.refresh == 1);
    aoe_game_view_player_ror(&g, 2, &ops);
    assert(g.view_player == 1 && env.refresh == 1);
    assert(aoe_game_view_player_object_ror(&g) == &p1);

    /* Timing. */
    aoe_game_timing_reset_ror(&g);
    aoe_game_timing_sample_ror(&g, 3, 10);
    aoe_game_timing_sample_ror(&g, 3, 4);
    assert(aoe_game_timing_total_ror(&g, 3) == 14 &&
           aoe_game_timing_last_ror(&g, 3) == 4 && g.timing[3].max == 10);
    aoe_game_timing_count_508_ror(&g);
    aoe_game_timing_set_last_ror(&g, 3, 1);
    assert(g.timing_504[1] == 1 && aoe_game_timing_last_ror(&g, 3) == 1 &&
           aoe_game_timing_last_ror(&g, 30) == 0);

    /* Comm state: changes mark; mode 7 reads 0. */
    env.comm = 1;
    aoe_game_comm_update_ror(&g, 0, 9, &ops);
    assert(g.comm_state == 1 && g.comm_value == 0 && env.mark == 1 &&
           g.world_dirty == 1);
    aoe_game_comm_update_ror(&g, 1, 9, &ops);
    assert(env.mark == 1);
    g.mode = 7;
    aoe_game_comm_update_ror(&g, 1, 9, &ops);
    assert(g.comm_state == 0 && g.comm_value == 9 && env.mark == 2);

    /* Mouse decode and the command line. */
    aoe_game_decode_mouse_ror(0xb, 0xfffe0010u, xy, &l, &r, &s, &k);
    assert(xy[0] == 16 && xy[1] == -2 && l == 1 && r == 2 && s == 8 && k == 0);
    g.command_line = "empiresx nosound";
    assert(aoe_game_has_option_ror(&g, "NOSOUND") &&
           !aoe_game_has_option_ror(&g, "NOMUSIC"));

    /* Cursor states. */
    g.cursor_capable = 1;
    aoe_game_cursor_set_ror(&g, 77, &ops);
    assert(last == 77 && env.system == 0);
    aoe_game_cursor_show_ror(&g, &ops);
    assert(g.cursor_shown && env.on == 1);
    aoe_game_cursor_software_ror(&g, 1, &ops);
    assert(env.off == 1 && env.system == 77);
    aoe_game_cursor_hide_ror(&g, &ops);
    assert(!g.cursor_shown && env.system == 0);
    aoe_game_cursor_software_ror(&g, 0, &ops);
    assert(env.off == 2);

    assert(!strcmp(aoe_game_string_ror(&ops, 5), "Age of Empires") &&
           !strcmp(aoe_game_load_string_ror(&ops, 6, text, 8), ""));
    aoe_game_set_state_ror(&g, 0x15, 2);
    aoe_game_mark_3f8_ror(&g, &ops);
    assert(g.state == 0x15 && g.field_3f8 == 1 && env.reset == 1);
    assert(aoe_game_default_true_ror(&g, 3) == 1);
    {
        /* Timing rollover (0x0041af70) after the setup (0x004190b0). */
        static AoeGameBaseRor tg;
        AoeGameRolloverOpsRor rops;
        FILE *logf = tmpfile();
        char line[512];
        memset(&rops, 0, sizeof rops);
        aoe_game_timing_setup_ror(&tg);
        aoe_game_timing_sample_ror(&tg, 1, 100);
        aoe_game_timing_sample_ror(&tg, 2, 300);
        aoe_game_timing_sample_ror(&tg, 5, 40);
        aoe_game_timing_sample_ror(&tg, 6, 50);
        tg.timing_504[0] = 20; /* frames */
        tg.timing_504[1] = 4;
        aoe_game_timing_rollover_ror(&tg, 5000, &rops);
        /* First time: slot 0 = the busy sum (100 + 300). */
        assert(tg.timing[0].field_4 == 400 && tg.timing[0].field_8 == 5000);
        assert(tg.timing[1].field_4 == 100 && tg.timing[1].total == 0 && tg.timing[1].field_1c == 100);
        assert(tg.timing[5].field_4 == 40 && tg.timing[4].field_4 == 0);
        assert(tg.timing[9].field_4 == 250);
        assert(tg.timing_504[6] == 50 && tg.timing_504[7] == 10 && tg.timing_504[8] == 0);
        assert(tg.timing_504[3] == 20 && tg.timing_504[0] == 0);
        aoe_game_timing_sample_ror(&tg, 3, 200);
        tg.timing_504[0] = 3;
        aoe_game_timing_rollover_ror(&tg, 6000, &rops);
        assert(tg.timing[0].field_4 == 1000 && tg.timing[4].field_4 == 800);
        assert(tg.timing_504[6] == 3);
        tg.mode = 4;
        rops.log = logf;
        tg.timing_504[1] = 2;
        aoe_game_timing_rollover_ror(&tg, 6500, &rops);
        rewind(logf);
        assert(fgets(line, sizeof line, logf) && strncmp(line, "0, 0, 0, 0, 0, 0, 0, 0, 0, 2, ", 30) == 0);
        fclose(logf);
    }
    return 0;
}
