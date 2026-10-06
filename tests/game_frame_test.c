#include "game_frame.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t created, moved[4], shown, sent_msg, published_win, after;
static int reg_style;
static int32_t screen_w = 800, screen_h = 600, game_w = 640, game_h = 480;
static int regc(void *c, uint32_t s, const char *n, uintptr_t i, const char *ic, const char *m) {
    (void)c; (void)n; (void)i; (void)ic; assert(strcmp(m, "") == 0); reg_style = (int)s; return 1;
}
static uint32_t cw(void *c, const char *n, const char *t, uint32_t s, int32_t w, int32_t h) {
    (void)c; (void)n; (void)t; assert(s == 0x82080000u && w == 800 && h == 600); return ++created;
}
static void ss(void *c, int32_t *w, int32_t *h) { (void)c; *w = screen_w; *h = screen_h; }
static void wr(void *c, uint32_t w, int32_t r[4]) { (void)c; (void)w; r[0] = 10; r[1] = 20; r[2] = 810; r[3] = 640; }
static void cr(void *c, uint32_t w, int32_t r[4]) { (void)c; (void)w; r[0] = 0; r[1] = 0; r[2] = 790; r[3] = 590; }
static void mw(void *c, uint32_t w, int32_t x, int32_t y, int32_t ww, int32_t hh) {
    (void)c; (void)w; moved[0] = (uint32_t)x; moved[1] = (uint32_t)y; moved[2] = (uint32_t)ww; moved[3] = (uint32_t)hh;
}
static void sh(void *c, uint32_t w, int32_t cmd) { (void)c; (void)w; shown = (uint32_t)cmd; }
static void up(void *c, uint32_t w) { (void)c; (void)w; }
static void sd(void *c, uint32_t w, uint32_t m) { (void)c; (void)w; sent_msg = m; }
static void aft(void *c) { (void)c; after++; }

static void *comm_obj = (void *)0x55, *pub;
static int setups, listens, worlds, frees, recreated;
static void *cnew(void *c) { (void)c; return comm_obj; }
static void cfree(void *c, void *x) { (void)c; (void)x; frees++; }
static void csetup(void *c, void *x) { (void)c; (void)x; setups++; }
static void wcomm(void *c, void *x) { (void)c; (void)x; worlds++; }
static void listen_(void *c, void *l, void *x) { (void)c; (void)l; (void)x; listens++; }
static void recr(void *c) { (void)c; recreated++; }

static int calls[8], mx, my, mbits;
static int fwd(void *c, int which, uint32_t a, uint32_t b, uint32_t d, uint32_t e) {
    (void)c; (void)a; (void)b; (void)d; (void)e; calls[which]++; return 1;
}
static int32_t gpal(void *c) { (void)c; calls[3]++; return 1; }
static void spal(void *c) { (void)c; calls[4]++; }
static int dsize(void *c, int32_t *w, int32_t *h) { (void)c; *w = 1024; *h = 768; return 1; }
static int presize(void *c, int32_t w, int32_t h) { (void)c; (void)w; (void)h; calls[5]++; return 1; }
static void adisp(void *c) { (void)c; calls[6]++; }
static int inc(void *c) { (void)c; return 0; }
static void coff(void *c) { (void)c; calls[7]++; }
static void cres(void *c) { (void)c; }
static void mouse(void *c, int32_t x, int32_t y, uint32_t l, uint32_t r, uint32_t s, uint32_t k) {
    (void)c; mx = x; my = y; mbits = (l ? 1 : 0) | (r ? 2 : 0) | (s ? 4 : 0) | (k ? 8 : 0);
}
static void tick(void *c) { (void)c; }
static int32_t camp[3] = {1, 2, 3};
static int32_t g0(void *c) { (void)c; return camp[0]; }
static int32_t g1(void *c) { (void)c; return camp[1]; }
static int32_t g2(void *c) { (void)c; return camp[2]; }
static int ch(void *c, int32_t v) { (void)c; return v >= 0; }
static int playing(void *c, void *s) { (void)c; return s == (void *)1; }
static int play(void *c, void *s) { (void)c; (void)s; return 7; }

static int gpf_validates, gpf_clears, gpf_shows, gpf_redraws, gpf_destroys;
static uint8_t gpf_restore_v = 2;
static int gpf_iconic_v;
static int gpf_iconic(void *c) { (void)c; return gpf_iconic_v; }
static void gpf_validate(void *c) { (void)c; ++gpf_validates; }
static void gpf_clear(void *c) { (void)c; ++gpf_clears; }
static uint8_t gpf_restore(void *c) { (void)c; return gpf_restore_v; }
static void gpf_redraw(void *c) { (void)c; ++gpf_redraws; }
static void gpf_destroy(void *c) { (void)c; ++gpf_destroys; }
static int gpf_rect(void *c, int32_t r[4]) { (void)c; r[0] = r[1] = 0; r[2] = r[3] = 9; return 1; }
static void gpf_show(void *c, const int32_t r[4]) { (void)c; assert(r[2] == 9); ++gpf_shows; }

static void paint_font_tests(void) {
    AoeGamePaintFrameOpsRor o = {NULL, 1, 1, gpf_iconic, gpf_validate, gpf_clear, gpf_restore,
                                 gpf_redraw, gpf_redraw, gpf_destroy, gpf_rect, gpf_show};
    AoeLogFontRor f;
    assert(aoe_game_frame_paint_ror(&o) == 1 && gpf_redraws == 2 && gpf_validates == 0);
    gpf_restore_v = 3;
    assert(aoe_game_frame_paint_ror(&o) == 0 && gpf_destroys == 1 && gpf_validates == 1);
    o.draw_active = 0;
    assert(aoe_game_frame_paint_ror(&o) == 0 && gpf_shows == 1 && gpf_validates == 2);
    o.window_ok = 0;
    assert(aoe_game_frame_paint_ror(&o) == 0 && gpf_clears == 1 && gpf_validates == 3);
    aoe_game_font_desc_ror("Arial", 12, 700, 0, "0", &f);
    assert(f.height == -16 && f.weight == 700 && f.quality == 2 && f.pitch_family == 2 &&
           strcmp(f.face, "Arial") == 0);
    aoe_game_font_desc_ror("Arial", 9, 400, 1, "238", &f);
    assert(f.height == -12 && f.charset == 238 && f.strikeout == 1);
}

int main(void) {
    paint_font_tests();
    AoeGameFrameOpsRor f;
    AoeGameCommOpsRor co;
    AoeGameDisplayOpsRor d;
    AoeGameCampaignOpsRor ca = {NULL, g0, g1, g2, ch, ch, ch};
    void *comm = NULL, *chat = NULL;
    void *sounds[3] = {(void *)1, NULL, (void *)2};
    int32_t c1, c2, c3, w = 640, h = 480;

    memset(&f, 0, sizeof f);
    f.register_class = regc; f.create_window = cw; f.screen_size = ss; f.window_rect = wr;
    f.client_rect = cr; f.move_window = mw; f.show = sh; f.update = up; f.focus = up; f.send = sd;
    f.after_create = aft; f.width = &game_w; f.height = &game_h; f.main_window = &published_win;
    f.menu_name = "";
    assert(aoe_game_frame_register_ror(&f) && reg_style == 0xb);
    f.existing_window = 1;
    reg_style = 0;
    assert(aoe_game_frame_register_ror(&f) && reg_style == 0);
    /* Full screen at another size: no window. */
    assert(aoe_game_frame_create_ror(&f) == 0 && created == 0);
    f.windowed = 1;
    f.show_command = 5;
    assert(aoe_game_frame_create_ror(&f) == 1 && created == 1 && published_win == 1 && after == 1);
    /* client 790 x 590 -> 640 x 480: width 640 - 10 - 790 + 810 = 650 */
    assert(moved[0] == 10 && moved[1] == 20 && moved[2] == 650 && moved[3] == 510 && shown == 5);
    aoe_game_frame_close_ror(&f);
    assert(sent_msg == 0x10);

    memset(&co, 0, sizeof co);
    co.comm_new = cnew; co.comm_free = cfree; co.comm_setup = csetup; co.world_comm = wcomm;
    co.listen = listen_; co.published = &pub; co.recreate = recr; co.chat = &chat;
    co.has_world = 1;
    co.listeners[1] = (void *)9;
    assert(aoe_game_comm_create_ror(&comm, &co) && comm == comm_obj && pub == comm_obj);
    assert(setups == 1 && worlds == 1 && listens == 1);
    aoe_game_comm_restart_ror(&comm, &co);
    assert(comm == NULL && pub == NULL && frees == 1 && recreated == 1);

    memset(&d, 0, sizeof d);
    d.draw_forward = fwd; d.game_palette = gpal; d.set_palette = spal; d.draw_size = dsize;
    d.panel_resize = presize; d.after_display = adisp; d.in_client = inc; d.cursor_off = coff;
    d.cursor_restore = cres; d.mouse = mouse; d.tick = tick;
    d.has_draw = 1;
    aoe_game_palette_changed_ror(&d, 5, 5, 0, 0);
    assert(calls[0] == 1 && calls[3] == 0 && calls[4] == 0);
    aoe_game_palette_changed_ror(&d, 5, 6, 0, 0);
    assert(calls[4] == 1);
    d.full_screen_palette = 1;
    aoe_game_palette_changed_ror(&d, 5, 6, 0, 0);
    assert(calls[3] == 1);
    aoe_game_display_changed_ror(&d, &w, &h, 0, 0, 1, 0);
    assert(calls[6] == 0);
    aoe_game_display_changed_ror(&d, &w, &h, 0, 0, 0, 0);
    assert(w == 1024 && h == 768 && calls[5] == 1 && calls[6] == 1);
    d.has_mouse = 1;
    d.cursor_capable = 1;
    aoe_game_mouse_message_ror(&d, 1 | 8, (0xffffu << 16) | 30);
    assert(mx == 30 && my == -1 && mbits == (1 | 4) && calls[7] == 1);

    aoe_game_campaign_get_ror(&ca, &c1, &c2, &c3);
    assert(c1 == 1 && c2 == 2 && c3 == 3);
    assert(aoe_game_campaign_set_ror(&ca, 0, 1, 2) && !aoe_game_campaign_set_ror(&ca, 0, -1, 2));

    assert(aoe_game_play_sound_ror(sounds, 0, playing, play, NULL) == 1);
    assert(aoe_game_play_sound_ror(sounds, 2, playing, play, NULL) == 7);
    assert(aoe_game_play_sound_ror(sounds, 1, playing, play, NULL) == 0);
    assert(aoe_game_play_sound_ror(NULL, 1, playing, play, NULL) == 0);
    puts("game_frame_test: ok");
    return 0;
}
