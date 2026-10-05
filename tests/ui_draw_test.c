/* Patched RoR 1.0a list (0x00465340/0x00465470) and input (0x00464140)
   drawing. */
#include "ui_input.h"
#include "ui_list.h"
#include "draw_system.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) {
    (void)c; (void)f; (void)t;
    return n * 10;
}
static uint32_t colors[16];
static int ncolors;
static int tb(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r, uintptr_t f) {
    (void)c; (void)a; (void)r; (void)f;
    return 1;
}
static void to(void *c, int32_t x, int32_t y, const char *t, int32_t n,
               uint32_t color) {
    (void)c; (void)x; (void)y; (void)t; (void)n;
    if (ncolors < 16) colors[ncolors] = color;
    ++ncolors;
}
static void te(void *c) { (void)c; }

static uint8_t bits[200 * 100];
static uint8_t *rows[100];
static AoeDrawSystemRor d;
static AoeDrawSurfaceRor surf;

static void memory_surface(AoeUiAreaRor *area) {
    d.type = 1;
    surf.system = &d;
    surf.view.bits = bits;
    surf.view.width = 200;
    surf.view.height = 100;
    surf.view.pitch = 200;
    surf.view.direction = 1;
    surf.view.clip_right = 199;
    surf.view.clip_bottom = 99;
    for (int i = 0; i < 100; ++i) rows[i] = bits + 200 * i;
    surf.rows = rows;
    area->surface = &surf;
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 200, 100};
    static AoeUiListRor l;
    static AoeUiInputRor in;

    pf.text_width = width;
    pf.text_begin = tb;
    pf.text_out = to;
    pf.text_end = te;
    sys.platform = &pf;
    memory_surface(&area);

    /* The list: only the selected line takes the focus colours, and its
       highlight bar is drawn around it. */
    aoe_ui_list_init_ror(&l, &sys);
    aoe_ui_panel_setup_ror(&l.text.panel, &area, NULL, 0, 0, 120, 60, 0);
    aoe_ui_text_set_font_ror(&l.text, 7, 8, 18);
    aoe_ui_text_append_ror(&l.text, "a", 1);
    aoe_ui_text_append_ror(&l.text, "b", 2);
    aoe_ui_text_select_ror(&l.text, 1);
    l.text.vertical = 3;
    l.text.shadow_style = 0;
    l.text.color = 0x11;
    l.text.focus_color = 0x33;
    l.text.bevel_type = 0;
    l.text.panel.has_focus = 1;
    l.text.panel.active = 1;
    l.highlight = 1;
    memset(bits, 0, sizeof bits);
    l.text.panel.vt->draw(&l.text.panel);
    assert(ncolors == 2 && colors[0] == 0x11 && colors[1] == 0x33);
    /* The bar: a 0xff rectangle around line 1 inside the 1-pixel margin
       (columns 1..118, from row 21). */
    assert(bits[21 * 200 + 1] == 0xff && bits[21 * 200 + 118] == 0xff &&
           bits[21 * 200 + 0] == 0);
    l.text.panel.has_focus = 0;
    ncolors = 0;
    l.text.panel.vt->draw(&l.text.panel);
    assert(colors[1] == 0x11);
    aoe_ui_list_release_ror(&l);

    /* The input: the caret after the line's leading characters. */
    aoe_ui_input_init_ror(&in, &sys, NULL, 530);
    aoe_ui_panel_setup_ror(&in.text.panel, &area, NULL, 0, 50, 120, 30, 0);
    aoe_ui_text_set_font_ror(&in.text, 7, 8, 18);
    aoe_ui_text_set_text_ror(&in.text, "abc");
    in.text.align = 1;
    in.text.vertical = 3;
    in.text.column_chars = 2;
    in.text.selected = 0;
    in.caret_on = 1;
    in.caret_color = 0x77;
    in.text.panel.has_focus = 1;
    in.text.panel.active = 1;
    memset(bits, 0, sizeof bits);
    in.text.panel.vt->draw(&in.text.panel);
    /* x = left 0 + 2 * 10 + 5 = 25 and 26, from the line's top (50). */
    assert(bits[50 * 200 + 25] == 0x77 && bits[50 * 200 + 26] == 0x77 &&
           bits[50 * 200 + 27] == 0);
    in.caret_on = 0;
    memset(bits, 0, sizeof bits);
    in.text.panel.vt->draw(&in.text.panel);
    assert(bits[50 * 200 + 25] == 0);
    aoe_ui_input_release_ror(&in);
    puts("ui_draw_test: ok");
    return 0;
}
