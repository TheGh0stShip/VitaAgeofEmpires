/* 0x004816d0: the viewed player's interface style. */
#include "game_screen.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char log_[4096];
static int shapes_made, shapes_freed, button_id = 6;

static void say(const char *fmt, ...) {
    va_list a;
    size_t n = strlen(log_);
    va_start(a, fmt);
    vsnprintf(log_ + n, sizeof log_ - n, fmt, a);
    va_end(a);
}

static void panels(void *c) { (void)c; say("P "); }
static void mode(void *c) { (void)c; say("M "); }
static void store(void *c) { (void)c; say("S "); }
static void age(void *c) { (void)c; say("A "); }
static void info(void *c, const char *n, int32_t id) { (void)c; say("info %s %x ", n, id); }
static void popup(void *c, const char *n, int32_t id) { (void)c; say("popup %s %x ", n, id); }
static void pictures(void *c, const char *n, int32_t id) { (void)c; say("pics %s %x ", n, id); }
static void bevel(void *c, const uint8_t k[6]) {
    (void)c;
    say("bevel %x %x %x %x %x %x ", k[0], k[1], k[2], k[3], k[4], k[5]);
}
static void button_bevel(void *c, uint16_t off, int32_t type, const uint8_t k[6]) {
    (void)c;
    say("bb %x %d %x ", off, type, k[5]);
}
static void picture(void *c, uint16_t off, int32_t state, void *pic, int16_t frame) {
    (void)c;
    say("pic %x %d %s %d ", off, state, pic ? (const char *)pic : "-", frame);
}
static int32_t id_of(void *c, uint16_t off) { (void)c; (void)off; return button_id; }
static void button_colors(void *c, uint16_t off, int32_t state, uint32_t a, uint32_t b) {
    (void)c;
    say("bc %x %d %x %x ", off, state, a, b);
}
static void border(void *c, int32_t i, void *shape) {
    (void)c;
    if (i == 0 || i == 11 || i == 12 || i == 16) say("border %d %s ", i, (const char *)shape);
}
static void colors(void *c, uint16_t off, uint32_t a, uint32_t b) {
    (void)c;
    say("tc %x %x %x ", off, a, b);
}
static char names[16][32];
static void *load(void *c, const char *name, int32_t id) {
    (void)c;
    say("load %s %x ", name, id);
    snprintf(names[shapes_made % 16], sizeof names[0], "%s", name);
    return names[shapes_made++ % 16];
}
static void unload(void *c, void *shape) { (void)c; say("free %s ", (const char *)shape); shapes_freed++; }
static void rebuild(void *c) { (void)c; say("B "); }
static void redraw(void *c) { (void)c; say("R"); }

int main(void) {
    int32_t style = -1, width = 0;
    void *shapes[AOE_GSP_SHAPES] = {0};
    void *page = NULL;
    AoeGameScreenStyleOpsRor o;
    memset(&o, 0, sizeof o);
    o.width = 640;
    o.height = 480;
    o.style = &style;
    o.style_width = &width;
    o.player_id = 1;
    o.player_style = 4;
    o.frame_4b0 = 7;
    o.frame_4b2 = 8;
    o.frame_4b4 = 9;
    o.player_to_panels = panels;
    o.mode_changed = mode;
    o.store_player = store;
    o.age = age;
    o.set_info = info;
    o.set_popup = popup;
    o.set_button_pictures = pictures;
    o.set_bevel_colors = bevel;
    o.button_bevel = button_bevel;
    o.button_picture = picture;
    o.button_id = id_of;
    o.button_text_colors = button_colors;
    o.button_border = border;
    o.text_colors = colors;
    o.load_shape = load;
    o.free_shape = unload;
    o.rebuild_buttons = rebuild;
    o.redraw = redraw;

    /* Style 4 (the Roman interface) at 640 x 480. */
    aoe_game_screen_style_ror(&o, shapes, &page);
    assert(strcmp(log_,
                  "P M S A info dlg6_4 cf08 popup dlg6_4 cf08 pics btn6_4 cf0c "
                  "bevel 73 74 75 75 76 77 bb 4f8 3 77 bb 4fc 3 77 bb 500 3 77 bb 504 4 77 "
                  "bb 508 4 77 load btnbrda4.shp cf0e load btnoth4.shp cf11 "
                  "load btnbrdb4.shp cf0f load btnbrdc4.shp cf10 load gamea4.shp cf12 "
                  "border 0 btnbrda4.shp border 11 btnbrda4.shp border 12 btnbrdb4.shp "
                  "border 16 btnbrdb4.shp pic 4dc 0 btnoth4.shp 7 pic 4f4 0 btnoth4.shp 9 "
                  "tc 4c0 ffffff 0 tc 580 ffffff 0 bc 4f8 0 ffffff 0 bc 4fc 0 ffffff 0 "
                  "bc 500 0 ffffff 0 bc 504 0 ffffff 0 bc 508 0 ffffff 0 B R") == 0);
    assert(style == 4 && width == 640 && shapes[AOE_GSP_GAME] && page == shapes[AOE_GSP_OTHER]);

    /* The same style and width: only the panels, buttons and redraw. */
    log_[0] = '\0';
    aoe_game_screen_style_ror(&o, shapes, &page);
    assert(strcmp(log_, "P M S A B R") == 0);

    /* Player 0 (gaia) never restyles. */
    log_[0] = '\0';
    o.player_id = 0;
    o.player_style = 1;
    aoe_game_screen_style_ror(&o, shapes, &page);
    assert(strcmp(log_, "P M S A B R") == 0 && style == 4);

    /* A style above 4 is style 0 (black text on white); the old shapes go
       first, each before its replacement. Button +0x4f4's ID other than 6
       takes frame +0x4b2. */
    log_[0] = '\0';
    o.player_id = 2;
    o.player_style = 9;
    button_id = 3;
    aoe_game_screen_style_ror(&o, shapes, &page);
    assert(style == 9);
    assert(strstr(log_, "info dlg6_0 c356 popup dlg6_0 c356 pics btn6_0 c5ad "
                        "bevel 7e 6d 6f 6f ee 38 ") != NULL);
    assert(strstr(log_, "free btnbrda4.shp load btnbrda0.shp c619 free btnoth4.shp load "
                        "btnoth0.shp c625 free btnbrdb4.shp load btnbrdb0.shp c61d free "
                        "btnbrdc4.shp load btnbrdc0.shp c63b free gamea4.shp load gamea0.shp c62d ") !=
           NULL);
    assert(strstr(log_, "pic 4f4 0 btnoth0.shp 8 tc 4c0 0 ffffff tc 580 0 ffffff "
                        "bc 4f8 0 0 ffffff") != NULL);
    assert(shapes_freed == 5);

    /* 800 x 600: "gameb", and the page picture is the large set. */
    log_[0] = '\0';
    o.width = 800;
    o.height = 600;
    o.player_style = 1;
    aoe_game_screen_style_ror(&o, shapes, &page);
    assert(strstr(log_, "load gameb1.shp c632 ") != NULL && page == shapes[AOE_GSP_COMMANDS]);
    assert(strstr(log_, "bevel 72 73 74 74 b7 b8 ") != NULL);
    puts("game screen style ok");
    return 0;
}
