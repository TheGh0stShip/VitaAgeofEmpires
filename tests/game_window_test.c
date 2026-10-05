/* Patched RoR 1.0a game window handling (0x00419dc0..0x0041c1b0). */
#include "game_window.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int last_slot, handler_result = 1, defaults, pre, screen_result;
static int handler(void *c, int slot, uint32_t m, uint32_t w, uint32_t l) {
    (void)c; (void)m; (void)w; (void)l;
    last_slot = slot;
    return handler_result;
}
static int32_t def(void *c, uint32_t m, uint32_t w, uint32_t l) { (void)c; (void)m; (void)w; (void)l; ++defaults; return 99; }
static void pre_(void *c, uint32_t m, uint32_t w, uint32_t l) { (void)c; (void)m; (void)w; (void)l; ++pre; }
static int32_t screen(void *c, uint32_t m, uint32_t w, uint32_t l) { (void)c; (void)m; (void)w; (void)l; return screen_result; }
static int iconic_v;
static int iconic(void *c) { (void)c; return iconic_v; }

static int ks[256];
static int key_state(void *c, int vk) { (void)c; return ks[vk]; }
static int shots, keys;
static void shot(void *c) { (void)c; ++shots; }
static int key(void *c, int32_t k, int32_t r, int ctrl, int alt, int shift) {
    (void)c; (void)r; (void)ctrl; (void)alt; (void)shift;
    ++keys;
    return k != 5;
}

static int32_t comm_count(void *c) { (void)c; return 13; }
static int d4_slot = -1, d4_on = -1, ec_on = -1, commands;
static void player_d4(void *c, int32_t s, int on) { (void)c; d4_slot = s; d4_on = on; }
static void world_ec(void *c, int on) { (void)c; ec_on = on; }
static int command(void *c, int32_t id, int32_t v) { (void)c; (void)id; (void)v; ++commands; return 1; }

static int waiting, pauses = -1, actives, inactives, screen_acts;
static int comm_waiting(void *c) { (void)c; return waiting; }
static void comm_pause(void *c, int p) { (void)c; pauses = p; }
static void scr_act(void *c) { (void)c; ++screen_acts; }
static void on_act(void *c) { (void)c; ++actives; }
static void on_inact(void *c) { (void)c; ++inactives; }

static int killed, quits;
static void kill_timer(void *c) { (void)c; ++killed; }
static void post_quit(void *c) { (void)c; ++quits; }

static uint32_t now;
static uint32_t now_ms(void *c) { (void)c; return now; }
static int restores, sounds, idles, comms, s13c, s140;
static void restore(void *c) { (void)c; ++restores; }
static void sound(void *c) { (void)c; ++sounds; }
static void sidle(void *c) { (void)c; ++idles; }
static int present(void *c) { (void)c; return 1; }
static void ctick(void *c) { (void)c; ++comms; }
static void f13c(void *c) { (void)c; ++s13c; }
static void f140(void *c) { (void)c; ++s140; }

static int cursor_wait = -1, c2d0, c2c0, recap_set = -1;
static void set_cursor(void *c, int w) { (void)c; cursor_wait = w; }
static void k2d0(void *c) { (void)c; ++c2d0; }
static void k2c0(void *c) { (void)c; ++c2c0; }
static void recap(void *c, void *w, int s) { (void)c; (void)w; recap_set = s; }
static int32_t yes(void *c) { (void)c; return 5; }

static int gp_12c = 1, gp_clears, gp_shows, gp_stops;
static int32_t gp_on(void *c) { (void)c; return gp_12c; }
static void gp_clear(void *c) { (void)c; ++gp_clears; }
static void gp_show(void *c) { (void)c; ++gp_shows; }
static void gp_stop(void *c) { (void)c; ++gp_stops; }

static void paint_tests(void) {
    AoeGamePaintOpsRor p = {NULL, gp_on, 1, 1, gp_clear, gp_show};
    char title[256];
    assert(aoe_game_paint_ror(&p) == 1 && gp_clears == 1 && gp_shows == 1);
    p.draw_ready = 0;
    assert(aoe_game_paint_ror(&p) == 1 && gp_clears == 1);
    gp_12c = 0;
    assert(aoe_game_paint_ror(&p) == 0);
    aoe_game_set_title_ror("Age of Empires", title);
    assert(strcmp(title, "Age of Empires") == 0);
    aoe_game_stop_world_sounds_ror(gp_stop, NULL);
    assert(gp_stops == 1);
}

int main(void) {
    paint_tests();
    AoeGameWindowOpsRor w = {0};
    w.pre_dispatch = pre_;
    w.handler = handler;
    w.default_proc = def;
    w.screen_message = screen;
    w.iconic = iconic;
    /* Routing. */
    assert(aoe_game_window_proc_ror(&w, 0x100, 0, 0) == 99 && last_slot == AOE_GAME_SLOT_KEY && pre == 1);
    handler_result = 0;
    assert(aoe_game_window_proc_ror(&w, 0x111, 0, 0) == 0 && last_slot == AOE_GAME_SLOT_COMMAND);
    handler_result = 1;
    aoe_game_window_proc_ror(&w, 0xf, 0, 0);
    assert(last_slot == AOE_GAME_SLOT_PAINT);
    aoe_game_window_proc_ror(&w, 0x10, 0, 0);
    assert(last_slot == AOE_GAME_SLOT_CLOSE);
    aoe_game_window_proc_ror(&w, 0x30f, 0, 0);
    assert(last_slot == AOE_GAME_SLOT_CLOSE); /* query palette, then close */
    aoe_game_window_proc_ror(&w, 0x3b9, 0, 0);
    assert(last_slot == AOE_GAME_SLOT_MCI);
    assert(aoe_game_window_proc_ror(&w, 0x218, 0, 0) == 0x424d5144);
    w.size_locked = 1;
    last_slot = 0;
    aoe_game_window_proc_ror(&w, 5, 0, 0);
    assert(last_slot == 0);
    w.full_screen = 1;
    assert(aoe_game_window_proc_ror(&w, 0x112, 0xf100, 0) == 0);
    iconic_v = 1;
    assert(aoe_game_window_proc_ror(&w, 0x112, 0xf100, 0) == 99);
    /* The screen answers first, except for mouse moves and timers. */
    w.has_screen = 1;
    screen_result = 7;
    assert(aoe_game_window_proc_ror(&w, 0x100, 0, 0) == 7);
    last_slot = 0;
    aoe_game_window_proc_ror(&w, 0x113, 0, 0);
    assert(last_slot == AOE_GAME_SLOT_IDLE);

    /* Keys. */
    {
        AoeGameKeyOpsRor k = {0, 1, 1, iconic, key_state, shot, key};
        iconic_v = 0;
        assert(aoe_game_key_down_ror(&k, 1, 0x104, 0x79, 1) == 0 && keys == 0);
        ks[0x11] = 1;
        assert(aoe_game_key_down_ror(&k, 1, 0x100, 0x7b, 1) == 0 && shots == 1);
        ks[0x11] = 0;
        assert(aoe_game_key_down_ror(&k, 1, 0x100, 0x41, 0x30001) == 1 && keys == 1 && k.last_repeat == 1);
        assert(aoe_game_key_down_ror(&k, 1, 0x100, 5, 1) == 0);
        k.active = 0;
        assert(aoe_game_key_down_ror(&k, 1, 0x100, 0x41, 1) == 1 && keys == 2);
    }
    /* Commands. */
    {
        AoeGameCommandOpsRor cmd = {0, comm_count, 0, player_d4, 1, world_ec, command};
        aoe_game_command_ror(&cmd, 0x17b3, 4);
        assert(d4_slot == 4 && d4_on == 1 && commands == 1);
        aoe_game_command_ror(&cmd, 0x17a3, 0);
        assert(ec_on == 0);
        cmd.locked = 1;
        d4_slot = -1;
        aoe_game_command_ror(&cmd, 0x17b4, 2);
        assert(d4_slot == -1 && commands == 3);
    }
    /* Activation pauses a running single-player game and resumes it. */
    {
        AoeGameActivateOpsRor a = {0, 4, 0, 1, comm_waiting, comm_pause, 1, scr_act, on_act, on_inact};
        int32_t active = 1;
        aoe_game_activate_ror(&a, &active, 0);
        assert(!active && a.paused_by_focus == 1 && pauses == 1 && inactives == 1);
        aoe_game_activate_ror(&a, &active, 1);
        assert(active && a.paused_by_focus == 0 && pauses == 0 && screen_acts == 1 && actives == 1);
        a.single_player = 0;
        pauses = -1;
        aoe_game_activate_ror(&a, &active, 0);
        assert(pauses == -1 && a.paused_by_focus == 0);
    }
    /* Close. */
    {
        AoeGameCloseOpsRor cl = {0, 1, kill_timer, post_quit};
        int32_t up = 1;
        void *win = &up;
        aoe_game_close_ror(&cl, &up, &win);
        assert(!up && !win && killed == 1 && quits == 1);
    }
    /* Timer: palette restores at 250 ms, every 500 ms above five. */
    {
        int32_t count = 6;
        uint32_t last = 0;
        AoeGameTimerOpsRor t = {0, 1, 1, &count, &last, now_ms, restore, sound, sidle, present, ctick, 1, 0, f13c, f140};
        now = 400;
        aoe_game_timer_ror(&t);
        assert(restores == 0 && sounds == 1 && idles == 1 && comms == 1 && s13c == 1 && s140 == 0);
        now = 500;
        aoe_game_timer_ror(&t);
        assert(restores == 1 && count == 5 && last == 500);
        now = 750;
        aoe_game_timer_ror(&t);
        assert(restores == 2 && count == 4);
        t.window_ok = 0;
        assert(aoe_game_timer_ror(&t) == 0);
    }
    /* Caption. */
    {
        char out[256];
        uint32_t totals[10] = {30, 100, 200, 300, 400, 500, 600, 700, 800, 900};
        aoe_game_fps_caption_ror(out, sizeof out,
                                 "fps=%lu, rend=%lu, view=%lu, map=%lu, scr=%lu, upd=%lu, snd=%lu, misc=%lu, oth=%lu, tot=%lu",
                                 100, totals);
        assert(strcmp(out, "fps=30, rend=1, view=2, map=3, scr=4, upd=5, snd=6, misc=7, oth=8, tot=9") == 0);
    }
    /* Cursors. */
    {
        AoeGameCursorOpsRor cu = {0, 1, set_cursor, k2d0, k2c0, &cu, recap};
        int32_t flag = 1;
        aoe_game_wait_cursor_ror(&cu, &flag);
        assert(cursor_wait == 1 && c2d0 == 1 && flag == 0 && recap_set == 1);
        aoe_game_arrow_cursor_ror(&cu, &flag);
        assert(cursor_wait == 0 && c2c0 == 1 && flag == 1 && recap_set == 0);
    }
    assert(aoe_game_relay_ror(yes, NULL) == 1);
    puts("game window ok");
    return 0;
}
