#include "game_ror.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int no_sound, base_called, pop = -1;
static void ns(void *c) { (void)c; no_sound = 1; }
static void sp(void *c, uint8_t v) { (void)c; pop = v; }
static void bc(void *c) { (void)c; base_called++; }
static float mx, my;
static void move(void *c, float x, float y) { (void)c; mx = x; my = y; }
static int32_t seq[8], seq_i;
static int32_t rnd(void *c) { (void)c; return seq[seq_i++ % 8]; }
static int removed;
static void rm(void *c, const char *n) { (void)c; (void)n; removed++; }
static uint8_t sent[0x200];
static int32_t sent_n;
static void send(void *c, const uint8_t *d, int32_t n) { (void)c; memcpy(sent, d, (size_t)n); sent_n = n; }
static const uint8_t *recv(void *c, int32_t *n) { (void)c; *n = sent_n; return sent; }
static int ended = -1, hidden, msgs[4], nmsg, base_msgs, helped;
static void cursor(void *c, int w) { (void)c; (void)w; }
static void hide(void *c) { (void)c; hidden++; }
static void endv(void *c, int skip) { (void)c; ended = skip; }
static void sendm(void *c, uint32_t w, uint32_t m) { (void)c; (void)w; msgs[nmsg++] = (int)m; }
static int32_t bproc(void *c, uint32_t m) { (void)c; (void)m; return ++base_msgs; }
static int32_t bpal(void *c) { (void)c; return 7; }
static void bact(void *c, int32_t a) { (void)c; (void)a; }
static int help(void *c) { (void)c; return ++helped; }

static int menu_ends, menu_mode, menu_track;
static char menu_file[16], menu_shown[40];
static int menu_make_ok(void *c) { (void)c; return 1; }
static int menu_make_fail(void *c) { (void)c; return 0; }
static void menu_end(void *c) { (void)c; ++menu_ends; }
static void menu_show(void *c, const char *n) { (void)c; snprintf(menu_shown, sizeof menu_shown, "%s", n); }
static void menu_set_mode(void *c, int32_t m) { (void)c; menu_mode = m; }
static void menu_play_track(void *c, int32_t t) { (void)c; menu_track = t; }
static void menu_play_file(void *c, const char *f) { (void)c; snprintf(menu_file, sizeof menu_file, "%s", f); }

static uint32_t lv_clock;
static int lv_polls, lv_resets, lv_pending = 3, lv_status, lv_unlink, lv_state = -1, lv_video = -1;
static void lv_reset(void *c) { (void)c; ++lv_resets; }
static void lv_poll(void *c) { (void)c; ++lv_polls; lv_clock += 100; if (lv_polls % 6 == 0 && lv_pending) --lv_pending; }
static int32_t lv_unacked(void *c) { (void)c; return lv_pending; }
static void lv_stat(void *c, int32_t id) { (void)c; lv_status = id; }
static uint32_t lv_now(void *c) { (void)c; return lv_clock; }
static void lv_unl(void *c, int32_t l) { (void)c; lv_unlink = l; }
static void lv_set_state(void *c, int32_t s) { (void)c; lv_state = s; }
static void lv_play(void *c, int32_t k, int w) { (void)c; (void)k; lv_video = w; }

int main(void) {
    char coded[64];
    AoeGameRorCommandOpsRor cops = {NULL, ns, sp, bc};
    AoeViewHistoryRor h;
    AoeComputerNamesRor names;
    AoeTrigTablesRor *t;
    AoeTribeSettingsRor a, b;
    uint8_t packed[0x6c];
    AoeGameSettingsRor *g = calloc(1, sizeof *g), *g2 = calloc(1, sizeof *g2);
    AoeGameRorCommOpsRor comm = {NULL, 1, send, recv};
    AoeGameRorInputOpsRor in;
    char name[0x104];
    int i;

    /* "LIMIT=" is +!#L%c#L ^= in the code; digits pass through. */
    aoe_game_ror_encode_ror("LIMIT=", coded, 60);
    assert(strcmp(coded, "+!#L%c#L ^=") == 0);
    aoe_game_ror_command_line_ror("game nosound limit=150 x", &cops);
    assert(pop == 150 && !no_sound && base_called == 1);
    aoe_game_ror_command_line_ror("NoTerrainSound LIMIT=5", &cops);
    assert(pop == 25 && no_sound);
    aoe_game_ror_command_line_ror("LIMIT=999", &cops);
    assert(pop == 200);
    pop = -1;
    aoe_game_ror_command_line_ror("LIMITS", &cops);
    assert(pop == -1);

    /* View history. */
    memset(&h, 0, sizeof h);
    for (i = 0; i < 5; ++i) h.x[i] = h.y[i] = -1;
    h.head = h.cursor = -1;
    aoe_game_ror_view_push_ror(&h, 10, 20);
    aoe_game_ror_view_push_ror(&h, 10, 20);
    assert(h.head == 0);
    aoe_game_ror_view_push_ror(&h, 30, 40);
    aoe_game_ror_view_back_ror(&h, move, NULL);
    assert(mx == 30.0f && my == 40.0f && h.cursor == 0);
    aoe_game_ror_view_back_ror(&h, move, NULL);
    /* slot 4 is empty: back to the head */
    assert(mx == 10.0f && h.cursor == 1);
    for (i = 0; i < 6; ++i) aoe_game_ror_view_push_ror(&h, i, i);
    assert(h.head == 2 && h.x[2] == 5);

    /* Names: random picks retry while used. */
    aoe_game_ror_clear_names_ror(&names);
    seq[0] = 3; seq[1] = 3; seq[2] = 13; seq[3] = 4;
    assert(aoe_game_ror_pick_name_ror(&names, 2, 10, rnd, NULL) == 20 + 3 - 9);
    assert(aoe_game_ror_pick_name_ror(&names, 2, 10, rnd, NULL) == 20 + 4 - 9 && names.used[2][3]);
    for (i = 0; i < 10; ++i) names.used[1][i] = (uint8_t)(i != 6);
    for (i = 0; i < 8; ++i) seq[i] = 0;
    assert(aoe_game_ror_pick_name_ror(&names, 1, 10, rnd, NULL) == 10 + 6 - 9);

    /* Trig tables. */
    t = malloc(sizeof *t);
    aoe_game_ror_trig_ror(t);
    assert(t->sin4096[1024] == 1.0f && t->tan4096[1024] == 0.0f && t->atan4096[2048] == 0.0f);
    assert(fabsf(t->cos360[180] + 1.0f) < 1e-6f && t->tan360[90] == 0.0f);
    assert(fabsf(t->atan360[45] - (float)atan(45 * 0.01745329252)) < 1e-6f);
    free(t);

    /* Settings snapshot round trip and exchange. */
    aoe_settings_set_map_size_ror(g, 2);
    aoe_settings_set_byte_a94_ror(g, 3, 7);
    aoe_settings_set_int_aa0_ror(g, 8, -5);
    aoe_settings_set_starting_age_ror(g, 2);
    aoe_settings_set_option_ae5_ror(g, 1);
    aoe_game_ror_settings_get_ror(g, &a);
    aoe_game_ror_settings_pack_ror(&a, packed);
    aoe_game_ror_settings_unpack_ror(packed, &b);
    assert(memcmp(&a, &b, sizeof a) == 0 && packed[0x18 + 3] == 7 && packed[0x69] == 1);
    aoe_game_ror_send_settings_ror(g, &comm);
    assert(sent_n == 0x114);
    aoe_game_ror_receive_settings_ror(g2, &comm);
    aoe_game_ror_settings_get_ror(g2, &b);
    assert(b.map_size == 2 && b.byte_a94[3] == 7 && b.int_aa0[8] == -5 && b.starting_age == 2);
    sent_n = 3;
    aoe_settings_set_starting_age_ror(g2, 0);
    aoe_game_ror_receive_settings_ror(g2, &comm);
    assert(aoe_settings_starting_age_ror(g2) == 0);
    /* The base record alone. */
    aoe_settings_set_speed_ror(g, 2.5f);
    aoe_game_send_settings_ror(g, &comm);
    assert(sent_n == 0xa8);
    aoe_settings_set_speed_ror(g2, 1.0f);
    aoe_game_receive_settings_ror(g2, &comm);
    assert(aoe_settings_speed_ror(g2) == 2.5f);
    sent_n = 0x114;
    aoe_settings_set_speed_ror(g2, 1.0f);
    aoe_game_receive_settings_ror(g2, &comm);
    assert(aoe_settings_speed_ror(g2) == 1.0f);

    /* Input. */
    memset(&in, 0, sizeof in);
    in.cursor = cursor; in.hide_cursor = hide; in.end_video = endv; in.send = sendm;
    in.base_proc = bproc; in.base_palette = bpal; in.base_activate = bact; in.help = help;
    in.mode = 1;
    in.video = 5;
    assert(aoe_game_ror_window_proc_ror(&in, 0x201) == 0 && ended == 1);
    assert(aoe_game_ror_window_proc_ror(&in, 0x200) == 1 && hidden == 1);
    assert(aoe_game_ror_key_ror(&in, 0x1b) == 0 && ended == 0);
    assert(aoe_game_ror_key_ror(&in, 'a') == 1);
    aoe_game_ror_activate_ror(&in, 0);
    aoe_game_ror_activate_ror(&in, 0);
    aoe_game_ror_activate_ror(&in, 1);
    assert(nmsg == 2 && msgs[0] == 0x809 && msgs[1] == 0x855 && in.paused == 0);
    assert(aoe_game_ror_palette_ror(&in) == 1 && msgs[2] == 0x476);
    in.mode = 4;
    in.session = 1;
    assert(aoe_game_ror_palette_ror(&in) == 7);
    assert(aoe_game_ror_key_ror(&in, 0x70) == 0 && helped == 0);
    in.mode = 2;
    assert(aoe_game_ror_key_ror(&in, 0x70) == 0 && helped == 1);

    aoe_game_ror_set_name_ror(name, "save1");
    assert(strcmp(name, "save1") == 0);
    aoe_game_ror_set_name_ror(name, NULL);
    assert(name[0] == 0);
    {
        void *gs = (void *)1;
        aoe_game_ror_delete_screens_ror(rm, NULL, 1, &gs);
        assert(removed == 22 && gs == NULL);
    }
    assert(aoe_game_ror_end_text_id_ror(1) == 0x47e && aoe_game_ror_end_text_id_ror(2) == 0x47f);
    free(g);
    free(g2);
    puts("game_ror_test: ok");
    {
        /* 0x00500c60 / 0x00500b40 to a menu screen. */
        AoeGameRorMenuOpsRor m = {0, 1, menu_end, menu_make_fail, menu_show, menu_set_mode,
                                  2, 0, menu_play_track, menu_play_file};
        assert(!aoe_game_ror_to_menu_ror("Main Menu", &m) && menu_ends == 1 && !menu_mode);
        m.make_screen = menu_make_ok;
        m.video_playing = 0;
        assert(aoe_game_ror_to_menu_ror("Main Menu", &m));
        assert(menu_mode == 2 && !strcmp(menu_shown, "Main Menu") && !strcmp(menu_file, "xopen.mid"));
        m.music_source = 1;
        assert(aoe_game_ror_to_menu_ror("Campaign Selection Screen", &m) && menu_track == 2);
        m.music_source = 3;
        m.music_held = 1;
        menu_file[0] = 0;
        assert(aoe_game_ror_to_menu_ror("Main Menu", &m) && !menu_file[0]);
    }
    {
        /* 0x005063e0 / 0x00505f10 / 0x00503060 leaving a session. */
        AoeGameRorEndOpsRor e;
        memset(&e, 0, sizeof e);
        e.leave.reset_comm = lv_reset;
        e.leave.poll = lv_poll;
        e.leave.unacknowledged = lv_unacked;
        e.leave.status = lv_stat;
        e.leave.now_ms = lv_now;
        e.leave.unlink = lv_unl;
        e.set_state = lv_set_state;
        e.play_video = lv_play;
        aoe_game_ror_leave_ror(&e.leave);
        assert(lv_resets == 1 && !lv_polls);
        e.leave.networked = 1;
        e.leave.state = 4;
        aoe_game_ror_leave_ror(&e.leave);
        assert(lv_status == 0x4c0 && lv_pending == 0 && lv_unlink == 5 && lv_resets == 2);
        /* Commands that are never acknowledged stop after 15 s. */
        lv_pending = 1000;
        lv_polls = 0;
        lv_clock = 0;
        aoe_game_ror_leave_ror(&e.leave);
        assert(lv_clock >= 15000 && lv_clock < 16000);
        e.has_comm = 0;
        assert(aoe_game_ror_quit_session_ror(&e) && lv_state == -1);
        e.has_comm = 1;
        lv_pending = 0;
        assert(aoe_game_ror_quit_session_ror(&e) && lv_state == 0);
        lv_state = -1;
        e.leave.state = 5;
        e.has_world = 1;
        e.won = 1;
        aoe_game_ror_end_session_ror(&e);
        assert(lv_state == 0 && lv_video == 1);
    }
    return 0;
}
