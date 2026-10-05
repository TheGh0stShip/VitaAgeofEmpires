/* Patched RoR 1.0a game-screen command button (0x004f7420..0x004f7be0). */
#include "command_button.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { void *shape; int32_t frame, x, y, remap, disabled; AoeUiRectRor clip; } Draw;
static Draw draws[8];
static int ndraws;
static void draw_shape(void *c, AoeUiAreaRor *a, void *s, int32_t f, int32_t x,
                       int32_t y, const AoeUiRectRor *clip, int32_t remap,
                       int disabled) {
    (void)c; (void)a;
    Draw d = {s, f, x, y, remap, disabled, *clip};
    draws[ndraws++] = d;
}
static int32_t color(void *c) { (void)c; return 2; }

typedef struct { int32_t x, y; uint32_t color; char text[32]; } Out;
static Out outs[8];
static int nouts;
static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) {
    (void)c; (void)f; (void)t;
    return n * 10;
}
static int tb(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r, uintptr_t f) {
    (void)c; (void)a; (void)r; (void)f;
    return 1;
}
static void to(void *c, int32_t x, int32_t y, const char *t, int32_t n,
               uint32_t col) {
    (void)c;
    outs[nouts].x = x, outs[nouts].y = y, outs[nouts].color = col;
    snprintf(outs[nouts].text, sizeof outs[nouts].text, "%.*s", (int)n, t);
    ++nouts;
}
static void te(void *c) { (void)c; }

static AoeUiPanelRor *cover_panel;
static int32_t covered(AoeUiPanelRor *p, int32_t x, int32_t y) {
    (void)p;
    return x < 15 && y < 15;
}

int main(void) {
    static AoeUiSystemRor sys;
    AoeUiPlatformRor pf = {0};
    AoeCommandButtonOpsRor ops = {0, draw_shape, color};
    AoeUiAreaRor area = {(void *)1, 640, 480};
    static AoeCommandButtonRor c;
    static AoeUiPanelRor parent, map;
    static AoeUiPanelVtblRor map_vt;
    AoeUiButtonRor *b = &c.button;
    AoeUiPanelRor *p = &b->panel;
    char bg, pic;
    pf.text_width = width;
    pf.text_begin = tb;
    pf.text_out = to;
    pf.text_end = te;
    sys.platform = &pf;
    aoe_ui_panel_init_ror(&parent, &sys);
    aoe_ui_panel_setup_ror(&parent, &area, NULL, 0, 0, 640, 480, 0);
    assert(aoe_command_button_init_ror(&c, &sys, NULL, &ops, &area, &parent,
                                       NULL, 7, 8, 12, &bg));
    assert(b->draw_type == 1 && b->notify == 1 && b->ids[0] == -1 &&
           b->font == 7 && b->font_height == 12 && c.background == &bg);
    p->vt->set_fixed_position(p, 10, 20, 40, 30);

    /* Style 0: the background frame (pressed + base) and the picture inset
       by 2 (3 pressed), in the player's colours. */
    aoe_ui_button_set_picture_ror(b, 0, &pic, 5);
    c.player_colors = 1;
    c.background_frame = 4;
    p->vt->draw(p);
    assert(ndraws == 2 && draws[0].shape == &bg && draws[0].frame == 4 &&
           draws[0].x == 10 && draws[0].y == 20 && draws[0].remap == -1);
    assert(draws[1].shape == &pic && draws[1].frame == 5 && draws[1].x == 12 &&
           draws[1].y == 22 && draws[1].remap == 0x30 &&
           draws[1].clip.left == p->clip.left + 2 &&
           draws[1].clip.right == p->clip.right - 2);
    ndraws = 0;
    b->pressed = 1;
    b->disabled = 1;
    p->vt->draw(p);
    assert(draws[0].frame == 5 && draws[1].x == 13 && draws[1].y == 23 &&
           draws[1].clip.bottom == p->clip.bottom - 3 && draws[1].disabled);

    /* Style 2: no background; pressed shows the next frame in place. */
    ndraws = 0;
    c.style = 2;
    c.player_colors = 0;
    p->vt->draw(p);
    assert(ndraws == 1 && draws[0].frame == 6 && draws[0].x == 10 &&
           draws[0].remap == -1);

    /* Style 1 with text: centred, shadow first, shifted when pressed; the
       count in the corner. */
    ndraws = 0;
    b->pressed = 0;
    b->disabled = 0;
    c.style = 1;
    aoe_ui_button_set_text_ror(b, 0, "Go");
    aoe_command_button_show_count_ror(&c, 1);
    aoe_command_button_set_count_ror(&c, 12);
    p->vt->draw(p);
    assert(ndraws == 1 && nouts == 3);
    assert(outs[1].x == 10 + 20 - 10 && outs[1].y == 20 - 6 + 15 &&
           outs[1].color == b->text_color[0]);
    assert(outs[0].x == outs[1].x - 1 && outs[0].color == b->text_shadow[0]);
    assert(strcmp(outs[2].text, "12") == 0 && outs[2].x == 13 &&
           outs[2].y == 22 && outs[2].color == 0xcccccc);

    /* The help line; styles 2 and 3 are not inside over the map. */
    aoe_command_button_set_help_ror(&c, "Build a house");
    assert(strcmp(c.help, "Build a house") == 0);
    aoe_command_button_set_help_ror(&c, NULL);
    assert(c.help[0] == '\0');
    map_vt.is_inside = covered;
    map.vt = &map_vt;
    cover_panel = &map;
    c.cover = &cover_panel;
    assert(p->vt->is_inside(p, 12, 22));
    p->vt->set_fixed_position(p, 0, 0, 40, 30);
    assert(p->vt->is_inside(p, 12, 12));
    c.style = 2;
    assert(!p->vt->is_inside(p, 12, 12) && p->vt->is_inside(p, 20, 12));

    p->vt->destroy(p);
    puts("command button ok");
    return 0;
}
