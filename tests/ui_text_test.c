/* Patched RoR 1.0a text panel 0x004685b0..0x0046a760. */
#include "ui_text.h"

#include "draw_system.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static char outs[16][32];
static int nouts, begun, ended, clips;
static int text_begin(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r,
                      uintptr_t f) {
    (void)c; (void)a; (void)r; (void)f;
    ++begun;
    return 1;
}
static void text_clip(void *c, const AoeUiRectRor *r) {
    (void)c; (void)r;
    ++clips;
}
static void text_out(void *c, int32_t x, int32_t y, const char *t, int32_t n,
                     uint32_t color) {
    (void)c;
    if (nouts < 16)
        snprintf(outs[nouts], sizeof outs[0], "%d,%d,%.*s,%x", (int)x, (int)y,
                 (int)n, t, (unsigned)color);
    ++nouts;
}
static void text_end(void *c) {
    (void)c;
    ++ended;
}

static int32_t width(void *c, uintptr_t font, const char *text, int32_t n) {
    (void)c; (void)font; (void)text;
    return n * 10; /* 10 pixels per character */
}

static int32_t revealed = -5;
static void reveal_fn(void *c, AoeUiTextRor *t, int32_t i) { (void)c; (void)t; revealed = i; }

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    static AoeUiTextRor t;
    AoeUiRectRor r;
    int32_t col;

    pf.text_width = width;
    sys.platform = &pf;
    aoe_ui_text_init_ror(&t, &sys);
    assert(t.color == 0xffffff && t.fill_color == 0xff && t.shadow_style == 2);
    aoe_ui_panel_setup_ror(&t.panel, &area, NULL, 0, 0, 120, 100, 0);
    aoe_ui_text_set_font_ror(&t, 7, 8, 18);
    assert(t.pitch == 20 && t.visible == 5);

    /* Lines, IDs, lookups and removal. */
    for (int i = 0; i < 8; ++i) {
        char line[8] = {(char)('a' + i), 0};
        assert(aoe_ui_text_append_ror(&t, line, 100 + i));
    }
    assert(aoe_ui_text_count_ror(&t) == 8 && t.bottom == 4);
    assert(aoe_ui_text_index_of_id_ror(&t, 103) == 3 &&
           aoe_ui_text_index_of_ror(&t, "f") == 5 &&
           !strcmp(aoe_ui_text_line_ror(&t, 2), "c") &&
           !strcmp(aoe_ui_text_line_ror(&t, 20), "") &&
           aoe_ui_text_id_ror(&t, 20) == -1);
    aoe_ui_text_select_id_ror(&t, 106);
    assert(aoe_ui_text_selection_ror(&t) == 6 &&
           !strcmp(aoe_ui_text_selected_text_ror(&t), "g"));
    assert(aoe_ui_text_insert_ror(&t, 1, "x", "col", 1));
    assert(!strcmp(aoe_ui_text_line_ror(&t, 1), "x") &&
           !strcmp(aoe_ui_text_column_ror(&t, 1), "col"));
    assert(aoe_ui_text_remove_ror(&t, 1) && !strcmp(aoe_ui_text_line_ror(&t, 1), "b"));
    assert(!aoe_ui_text_remove_ror(&t, 8));

    /* Scrolling clamps the window. */
    aoe_ui_text_scroll_ror(&t, 7, 0, 1);
    assert(t.top == 3 && t.bottom == 7);
    aoe_ui_text_scroll_ror(&t, 5, 0, 1);
    assert(t.top == 0);
    assert(t.panel.vt->key_action(&t.panel, 0x28, 1, 0, 0, 0) == 1 && t.top == 1);
    assert(!t.panel.vt->key_action(&t.panel, 'A', 1, 0, 0, 0));

    /* Sorted insertion ignores case. */
    aoe_ui_text_clear_ror(&t);
    aoe_ui_text_set_sorted_ror(&t, 1);
    aoe_ui_text_append_ror(&t, "pear", 0);
    aoe_ui_text_append_ror(&t, "Apple", 0);
    aoe_ui_text_append_ror(&t, "banana", 0);
    assert(!strcmp(aoe_ui_text_line_ror(&t, 0), "Apple") &&
           !strcmp(aoe_ui_text_line_ror(&t, 1), "banana") &&
           !strcmp(aoe_ui_text_line_ror(&t, 2), "pear"));
    aoe_ui_text_set_sorted_ror(&t, 0);

    /* Newline splitting, max chars and word wrap (width 110 = 11 chars). */
    assert(aoe_ui_text_set_text_ror(&t, "one\ntwo\nthree"));
    assert(t.count == 3 && !strcmp(aoe_ui_text_line_ror(&t, 2), "three"));
    t.max_chars = 2;
    aoe_ui_text_set_text_ror(&t, "longer");
    assert(!strcmp(aoe_ui_text_line_ror(&t, 0), "lo"));
    t.max_chars = 0;
    aoe_ui_text_set_wrap_ror(&t, 1);
    assert(aoe_ui_text_set_text_ror(&t, "the quick brown fox jumps"));
    assert(!strcmp(aoe_ui_text_line_ror(&t, 0), "the quick "));
    assert(!strcmp(aoe_ui_text_line_ror(&t, 1), "brown fox "));
    assert(!strcmp(aoe_ui_text_line_ror(&t, 2), "jumps") && t.count == 3);
    assert(aoe_ui_text_set_text_ror(&t, "hi\nthere"));
    assert(!strcmp(aoe_ui_text_line_ror(&t, 0), "hi ") &&
           !strcmp(aoe_ui_text_line_ror(&t, 1), "there"));
    assert(aoe_ui_text_set_text_ror(&t, "end\n"));
    assert(t.count == 2 && !strcmp(aoe_ui_text_line_ror(&t, 1), ""));
    assert(aoe_ui_text_set_text_ror(&t, "abcdefghijklmnop"));
    assert(!strcmp(aoe_ui_text_line_ror(&t, 0), "abcdefghij") &&
           !strcmp(aoe_ui_text_line_ror(&t, 1), "klmnop"));

    /* Line rectangles. */
    aoe_ui_text_set_text_ror(&t, "abc");
    t.align = 1;
    t.vertical = 3;
    aoe_ui_text_line_rect_ror(&t, 0, 0, &r, &col);
    assert(r.left == 0 && r.top == 0 && r.right == 29 && r.bottom == 20 && col == 5);
    t.align = 0;
    t.vertical = 0;
    aoe_ui_text_line_rect_ror(&t, 0, 0, &r, NULL);
    assert(r.left == 120 / 2 - 15 && r.top == 0 - 10 + 50);

    /* Bevel margins and text kinds. */
    assert(t.panel.vt == &aoe_ui_text_vtable_ror.base);
    aoe_ui_text_set_bevel_ror(&t, 4, (const uint8_t[6]){1, 2, 3, 4, 5, 6});
    assert(t.margin == 3 && t.bevel[5] == 6 && t.visible == 4);
    assert(aoe_ui_text_char_kind_ror((const uint8_t *)"\xb1", NULL) == 1 &&
           aoe_ui_text_char_kind_ror((const uint8_t *)"a", NULL) == 0);

    /* Drawing: shadow style 2 draws the shadow one pixel down-left first;
       left alignment adds 5; the bevel lands on the area's surface. */
    {
        static uint8_t bits[640 * 4];
        static AoeDrawSystemRor d;
        static AoeDrawSurfaceRor surf;
        static uint8_t *rows[4];
        pf.text_begin = text_begin;
        pf.text_clip = text_clip;
        pf.text_out = text_out;
        pf.text_end = text_end;
        d.type = 1;
        surf.system = &d;
        surf.view.bits = bits;
        surf.view.width = 640;
        surf.view.height = 4;
        surf.view.pitch = 640;
        surf.view.direction = 1;
        surf.view.clip_right = 639;
        surf.view.clip_bottom = 3;
        for (int i = 0; i < 4; ++i) rows[i] = bits + 640 * i;
        surf.rows = rows;
        area.surface = &surf;
        aoe_ui_text_set_text_ror(&t, "abc");
        t.align = 1;
        t.vertical = 3;
        t.shadow_style = 2;
        t.color = 0x11;
        t.shadow = 0x22;
        t.panel.vt->draw(&t.panel);
        assert(begun == 1 && ended == 1 && nouts == 2);
        /* The line starts at the 3-pixel bevel margin, plus 5 when left-aligned. */
        assert(!strcmp(outs[0], "7,4,abc,22") && !strcmp(outs[1], "8,3,abc,11"));
        /* Bevel type 4: the outer top line is colour c[5] (bevel +0x179). */
        assert(bits[1] == 6 && bits[640 + 2] == 5);
        assert(t.panel.redraw == 0);
        /* Inactive panels draw nothing. */
        t.panel.active = 0;
        nouts = 0;
        t.panel.vt->draw(&t.panel);
        assert(nouts == 0);
        t.panel.active = 1;
        area.surface = NULL;
    }
    aoe_ui_text_release_ror(&t);
    {
        static AoeUiTextRor a, b;
        uint8_t changed = 0;
        aoe_ui_text_init_ror(&a, &sys);
        aoe_ui_text_init_ror(&b, &sys);
        aoe_ui_panel_setup_ror(&a.panel, &area, NULL, 0, 0, 120, 100, 0);
        aoe_ui_panel_setup_ror(&b.panel, &area, NULL, 0, 0, 120, 100, 0);
        aoe_ui_text_set_font_ror(&a, 7, 8, 18);
        aoe_ui_text_set_font_ror(&b, 7, 8, 18);
        aoe_ui_text_append_ror(&a, "one", 0);
        aoe_ui_text_append_ror(&a, "two", 0);
        aoe_ui_text_select_ror(&a, 1);
        aoe_ui_text_move_selected_ror(&a, &b, reveal_fn, NULL, &changed);
        assert(changed && aoe_ui_text_count_ror(&a) == 1 && aoe_ui_text_count_ror(&b) == 1);
        assert(!strcmp(aoe_ui_text_line_ror(&b, 0), "two") && revealed == 0);
        aoe_ui_text_append_ror(&b, "three", 0);
        aoe_ui_text_select_ror(&b, 1);
        changed = 0;
        aoe_ui_text_move_up_ror(&b, reveal_fn, NULL, &changed);
        assert(changed && !strcmp(aoe_ui_text_line_ror(&b, 0), "three"));
        assert(!strcmp(aoe_ui_text_line_ror(&b, 1), "two") && aoe_ui_text_selection_ror(&b) == 0);
        changed = 0;
        aoe_ui_text_move_up_ror(&b, reveal_fn, NULL, &changed);
        assert(!changed);
        aoe_ui_text_append_ror(&a, "three", 0);
        aoe_ui_text_append_ror(&a, "four", 0);
        aoe_ui_text_append_ror(&b, "five", 0);
        assert(!aoe_ui_text_take_chosen_ror(&a, &b) && aoe_ui_text_count_ror(&a) == 3);
        aoe_ui_text_remove_ror(&b, 2);
        aoe_ui_text_append_ror(&a, "two", 0);
        assert(aoe_ui_text_take_chosen_ror(&a, &b) && aoe_ui_text_count_ror(&a) == 2);
        assert(aoe_ui_text_index_of_ror(&a, "four") >= 0 && aoe_ui_text_index_of_ror(&a, "two") == -1);
        aoe_ui_text_release_ror(&a);
        aoe_ui_text_release_ror(&b);
    }
    assert(!t.lines);
    return 0;
}
