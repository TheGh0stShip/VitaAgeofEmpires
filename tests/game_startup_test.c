/* Patched RoR 1.0a game start-up (0x004162b0..0x00419c00). */
#include "game_startup.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int32_t reg[16];
static const char *names[16] = {"Screen Size", "Rollover Text", "Mouse Style", "Game Speed",
                                "Difficulty", "Path Finding", "MP Path Finding", "Scroll Speed"};
static int32_t get(void *c, const char *n) {
    (void)c;
    for (int i = 0; i < 8; ++i) if (strcmp(names[i], n) == 0) return reg[i];
    return -1;
}
static void set(void *c, const char *n, int32_t v) {
    (void)c;
    for (int i = 0; i < 8; ++i) if (strcmp(names[i], n) == 0) reg[i] = v;
}
static int widths_ok[4];
static int tries;
static int try_mode(void *c, int32_t w) {
    (void)c;
    ++tries;
    return (w == 640 && widths_ok[0]) || (w == 800 && widths_ok[1]) || (w == 1024 && widths_ok[2]) ||
           (w == 1280 && widths_ok[3]);
}
static int32_t slot_kind(void *c, int32_t s) { (void)c; return s <= 3 ? 2 : 0; }
static int slot_flag(void *c, int32_t i) { (void)c; return i != 1; }
static const char *cd_path(void *c) { (void)c; return "D:\\"; }
static uint32_t drive(void *c, const char *r) { (void)c; (void)r; return 5; }
static int label(void *c, const char *r, char *out, int n) { (void)c; (void)r; snprintf(out, (size_t)n, "aoe"); return 1; }
static int64_t mk(int y, int m0, int d) { return (int64_t)y * 10000 + m0 * 100 + d; }
static int fail_step = -1, steps_run[AOE_START_STEPS];
static int step(void *c, int s) { (void)c; steps_run[s]++; return s != fail_step; }
static uint32_t phys = 0x2000000, page = 0x2000000;
static void memory(void *c, uint32_t *p, uint32_t *f) { (void)c; *p = phys; *f = page; }
static int nodx(void *c) { (void)c; return 0; }
static uint32_t dxv = 0x600;
static uint32_t dx(void *c) { (void)c; return dxv; }

/* Message loop script. */
static int loop_active = 1, peeks, gets, dispatches, idles;
static int act(void *c) { (void)c; return loop_active; }
static int32_t st(void *c) { (void)c; return 4; }
static int wait_(void *c) { (void)c; return 0; }
static int get_(void *c, uint32_t *m, uint32_t *w) { (void)c; ++gets; *m = 0x12; *w = 7; return 0; }
static int peek_(void *c, uint32_t *m, uint32_t *w) {
    (void)c;
    ++peeks;
    if (peeks == 1) return 0;
    if (peeks == 2) { *m = 0x100; *w = 0; return 1; }
    *m = 0x12; *w = 42; return 1;
}
static void disp(void *c) { (void)c; ++dispatches; }
static void idle(void *c) { (void)c; ++idles; }
static int filt(void *c) { (void)c; return 1; }

int main(void) {
    static AoeStartupConfigRor cfg;
    static AoeGameSettingsRor settings;
    static AoeGameStartupRor g;
    AoeRegistryOpsRor r = {0, get, set};
    g.config = &cfg;
    g.settings = &settings;

    /* Command line. */
    cfg.sound = 1;
    cfg.music = 1;
    cfg.check_depth = 1;
    cfg.full_screen = 1;
    strcpy(cfg.command_line, "-midi music no_startup 800 resend1=12 msync");
    aoe_game_parse_command_line_ror(&g);
    assert(cfg.no_startup && cfg.music && cfg.music_midi && !cfg.music_cd);
    assert(cfg.screen_w == 800 && cfg.screen_h == 600 && g.resend1 == 12 && g.msync);
    assert(g.mfill == 1 && g.custom_mouse == 1);
    strcpy(cfg.command_line, "NOSOUND normalmouse mfill");
    aoe_game_parse_command_line_ror(&g);
    assert(cfg.sound == 0 && cfg.music == 0 && g.custom_mouse == 0 && g.mfill == 0);

    /* Registry. */
    for (int i = 0; i < 8; ++i) reg[i] = -1;
    reg[0] = 1024; reg[1] = 2; reg[2] = 2; reg[3] = 15; reg[4] = 3; reg[5] = 2; reg[6] = 4; reg[7] = 50;
    aoe_game_read_registry_ror(&g, &r);
    assert(cfg.screen_w == 1024 && cfg.screen_h == 768 && g.rollover_text == 0 && cfg.mouse_style == 2);
    assert(g.game_speed > 1.49f && g.game_speed < 1.51f && g.difficulty == 3);
    assert(settings.byte_a14 == 1 && settings.byte_983 == 0 && cfg.scroll_speed == 50);
    reg[7] = 300;
    aoe_game_read_registry_ror(&g, &r);
    assert(cfg.scroll_speed == 50);
    aoe_game_write_registry_ror(&g, 800, &r);
    assert(reg[0] == 800 && reg[1] == 2 && reg[2] == 2 && reg[3] == 15 && reg[5] == 2 && reg[6] == 1);

    /* Error strings. */
    assert(aoe_game_error_string_id_ror(1, 3) == 0x7e1 && aoe_game_error_string_id_ror(1, 0x16) == 0x7df);
    assert(aoe_game_error_string_id_ror(1, 0x10) == 0x7d5 && aoe_game_error_string_id_ror(2, 1234) == 1234);
    assert(aoe_game_error_string_id_ror(1, 99) == 0);

    /* Screen mode: falls back to 640 when 1024 fails. */
    cfg.screen_w = 1024;
    widths_ok[0] = 1;
    assert(aoe_game_choose_screen_ror(&g, 8, try_mode, NULL) && cfg.screen_w == 640 && cfg.screen_h == 480);
    widths_ok[0] = 0;
    assert(!aoe_game_choose_screen_ror(&g, 8, try_mode, NULL));
    assert(!aoe_game_choose_screen_ror(&g, 4, try_mode, NULL) && g.error == 0x13);
    g.error = 0;

    /* Music. */
    memset(&cfg.music_cd, 0, 4 * sizeof(int32_t));
    cfg.music_midi = 1;
    assert(aoe_game_music_source_ror(&cfg) == 2);
    cfg.music_midi = 0;
    cfg.music_8b8 = 1;
    assert(aoe_game_music_source_ror(&cfg) == 3);
    cfg.music = 1;
    assert(aoe_game_music_volume_ror(&cfg, -1) == -5000 && aoe_game_music_volume_ror(&cfg, 300) == -300);
    assert(aoe_game_music_volume_ror(&cfg, 9900) == -1 && cfg.music == 0);

    /* CD check: three human slots, two flagged; two players per CD. */
    {
        AoeCdCheckOpsRor cd = {0, slot_kind, slot_flag, 0, cd_path, drive, label};
        cfg.cd_required = 1;
        cfg.cd_players = 2;
        strcpy(cfg.volume_label, "AOE");
        assert(aoe_game_cd_check_ror(&cfg, 4, &cd) && !aoe_game_cd_check_ror(&cfg, 5, &cd));
        assert(aoe_game_cd_check_ror(&cfg, 0, &cd));
        strcpy(cfg.volume_label, "OTHER");
        assert(!aoe_game_cd_check_ror(&cfg, 0, &cd));
        cfg.cd_required = 0;
        assert(aoe_game_cd_check_ror(&cfg, 0, &cd));
    }
    /* Expiry. */
    cfg.expiry_year = 1999, cfg.expiry_month = 1, cfg.expiry_day = 1;
    assert(aoe_game_not_expired_ror(&cfg, mk(1999, 0, 1), mk) && !aoe_game_not_expired_ror(&cfg, mk(1999, 0, 2), mk));

    /* Memory and the start-up chain. */
    assert(aoe_game_enough_memory_ror(0x1400000, 0xa00000) && !aoe_game_enough_memory_ror(0x900000, 0x1400000));
    assert(aoe_game_enough_memory_ror(0, 0x1e00000));
    {
        AoeStartupChainOpsRor ch = {0, step, memory, nodx, dx};
        assert(aoe_game_startup_chain_ror(&g, &ch) && g.error == 0);
        fail_step = AOE_START_STEP_90;
        assert(!aoe_game_startup_chain_ror(&g, &ch) && g.error == 9);
        g.error = 0;
        fail_step = AOE_START_STEP_COMM;
        assert(!aoe_game_startup_chain_ror(&g, &ch) && g.error == 0xd);
        g.error = 5;
        fail_step = AOE_START_STEP_78;
        assert(!aoe_game_startup_chain_ror(&g, &ch) && g.error == 5); /* kept */
        g.error = 0;
        assert(!aoe_game_startup_chain_ror(&g, &ch) && g.error == 7);
        fail_step = -1;
        g.error = 0;
        dxv = 0x500;
        assert(!aoe_game_startup_chain_ror(&g, &ch) && g.error == 0x14);
        dxv = 0x600;
        g.error = 0;
        phys = page = 0x100000;
        assert(!aoe_game_startup_chain_ror(&g, &ch) && g.error == 0x16);
    }

    /* Message loop: idle once, dispatch a key, quit with the wParam. */
    {
        AoeMessageLoopOpsRor ml = {0, act, st, wait_, get_, peek_, disp, idle, filt};
        assert(aoe_game_message_loop_ror(&ml) == 42 && idles == 1 && dispatches == 1 && gets == 0);
        loop_active = 0;
        assert(aoe_game_message_loop_ror(&ml) == 7 && gets == 1);
    }

    /* Fonts and cursor wrap. */
    {
        int second;
        assert(aoe_game_font_string_ror(8, &second) == 0x80 && second);
        assert(aoe_game_font_string_ror(11, &second) == 0x80 && !second);
        assert(aoe_game_font_string_ror(2, &second) == 0);
        int32_t x = 65535, y = 10;
        aoe_game_wrap_point_ror(&x, &y);
        assert(x == -1 && y == 10);
    }
    puts("game startup ok");
    return 0;
}
