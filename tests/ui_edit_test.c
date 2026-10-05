/* Patched RoR 1.0a edit panel 0x00462050..0x00462e60. */
#include "ui_edit.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char text[256];
    int32_t sel_start, sel_end;
    int shown, focus, moves, undos, invalidates;
    int32_t x, y, w, h;
} Window;

static Window win;
static void *focused;
static int beeps;

static void *create(void *c, AoeUiAreaRor *a, int32_t x, int32_t y, int32_t w,
                    int32_t h, int ml) {
    (void)c; (void)a; (void)ml;
    memset(&win, 0, sizeof(win));
    win.x = x; win.y = y; win.w = w; win.h = h;
    return &win;
}
static void destroy(void *c, void *w) { (void)c; (void)w; }
static void show(void *c, void *w, int s) { (void)c; ((Window *)w)->shown = s; }
static void move(void *c, void *w, int32_t x, int32_t y, int32_t ww, int32_t h, int r) {
    Window *n = w; (void)c; (void)r;
    n->x = x; n->y = y; n->w = ww; n->h = h; ++n->moves;
}
static void invalidate(void *c, void *w, int u) { (void)c; (void)u; ++((Window *)w)->invalidates; }
static void *focus(void *c) { (void)c; return focused; }
static void set_focus(void *c, void *w) { (void)c; focused = w; }
static void *main_window(void *c, AoeUiAreaRor *a) { (void)c; return a->window; }
static void set_text(void *c, void *w, const char *t) { (void)c; snprintf(((Window *)w)->text, 256, "%s", t); }
static void get_text(void *c, void *w, char *out, int32_t size) { (void)c; snprintf(out, (size_t)size, "%s", ((Window *)w)->text); }
static void get_sel(void *c, void *w, int32_t *s, int32_t *e) { (void)c; *s = ((Window *)w)->sel_start; *e = ((Window *)w)->sel_end; }
static void set_sel(void *c, void *w, int32_t s, int32_t e) {
    Window *n = w; (void)c;
    n->sel_start = s;
    n->sel_end = e < 0 ? (int32_t)strlen(n->text) : e;
}
static void undo(void *c, void *w) { (void)c; ++((Window *)w)->undos; }
static int32_t def(void *c, void *w, uint32_t m, uint32_t wp, uint32_t lp) {
    Window *n = w; (void)c; (void)lp;
    if (m == 0x102) {
        size_t l = strlen(n->text);
        n->text[l] = (char)wp;
        n->text[l + 1] = '\0';
        n->sel_start = n->sel_end = (int32_t)l + 1;
    }
    return 7;
}
static void beep(void *c) { (void)c; ++beeps; }
static int lead(void *c, uint8_t b) { (void)c; return b >= 0x81; }

static int32_t acts[8], nacts, reject_escape;
static int32_t record(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t a, int32_t b, int32_t c) {
    (void)p; (void)from; (void)b; (void)c;
    acts[nacts++] = a;
    return a == 1 ? !reject_escape : 1;
}

static int typed(AoeUiEditRor *e, const char *keys) {
    int ok = 0;
    for (; *keys; ++keys) ok += aoe_ui_edit_accept_char_ror(e, (uint8_t)*keys) ? 1 : 0;
    return ok;
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)&sys, 800, 600};
    static AoeUiPanelRor parent;
    static AoeUiPanelVtblRor pvt;
    static AoeUiEditRor e;
    AoeUiEditOpsRor ops = {0};

    ops.create = create; ops.destroy = destroy; ops.show = show; ops.move = move;
    ops.invalidate = invalidate; ops.focus = focus; ops.set_focus = set_focus;
    ops.main_window = main_window; ops.set_text = set_text; ops.get_text = get_text;
    ops.get_selection = get_sel; ops.set_selection = set_sel; ops.undo = undo;
    ops.default_proc = def; ops.beep = beep; ops.lead_byte = lead;
    ops.scrollbar_width = 16;
    sys.platform = &pf;
    aoe_ui_panel_init_ror(&parent, &sys);
    aoe_ui_panel_setup_ror(&parent, &area, NULL, 0, 0, 800, 600, 0);
    pvt = aoe_ui_panel_vtable_ror;
    pvt.action = record;
    parent.vt = &pvt;

    aoe_ui_edit_init_ror(&e, &sys, &ops);
    assert(e.hidden && e.bevel_type == 1 && e.panel.handles_mouse);
    assert(aoe_ui_edit_setup_ror(&e, &area, &parent, 10, 20, 200, 24, 0, 5,
                                 "abcdefgh", 2, 1));
    /* max 5 -> 6; the text is cut to 5 characters. */
    assert(e.max == 6 && strcmp(e.text, "abcde") == 0 && e.length == 5 &&
           strcmp(win.text, "abcde") == 0);
    assert(win.x == 11 && win.y == 21 && win.w == 198 && win.h == 22 &&
           e.text_flags == 0x2020 && e.color == 0xffffff);

    /* Integer format: digits, a leading '-', nothing else; full beeps. */
    aoe_ui_edit_set_text_ror(&e, "");
    beeps = 0;
    assert(aoe_ui_edit_accept_char_ror(&e, '-') == 1);
    set_text(NULL, &win, "-");
    assert(aoe_ui_edit_accept_char_ror(&e, '-') == 0 && beeps == 1);
    assert(aoe_ui_edit_accept_char_ror(&e, '7') == 1 && !aoe_ui_edit_accept_char_ror(&e, 'x'));
    set_text(NULL, &win, "12345");
    assert(!aoe_ui_edit_accept_char_ror(&e, '1'));
    /* Backspace always, Tab and Enter never (single line). */
    assert(aoe_ui_edit_accept_char_ror(&e, 8) && !aoe_ui_edit_accept_char_ror(&e, 9) &&
           !aoe_ui_edit_accept_char_ror(&e, 0xd));
    /* A lead byte that cannot fit beeps and swallows the next character. */
    set_text(NULL, &win, "1234");
    assert(!aoe_ui_edit_accept_char_ror(&e, 0x82) && aoe_ui_edit_globals_ror.skip_char);
    assert(!aoe_ui_edit_accept_char_ror(&e, '1') && !aoe_ui_edit_globals_ror.skip_char);

    /* Number format 1: '.' only once one is there (the original's test). */
    e.format = 1;
    set_text(NULL, &win, "1");
    assert(!aoe_ui_edit_accept_char_ror(&e, '.'));
    set_text(NULL, &win, "1.");
    assert(aoe_ui_edit_accept_char_ror(&e, '.'));
    /* File names: one '.', no path or wildcard characters. */
    e.format = 4;
    set_text(NULL, &win, "a");
    assert(typed(&e, "b.") == 2);
    set_text(NULL, &win, "a.");
    assert(!aoe_ui_edit_accept_char_ror(&e, '.') && !aoe_ui_edit_accept_char_ror(&e, '\\') &&
           !aoe_ui_edit_accept_char_ror(&e, '*'));
    e.format = 6;
    set_text(NULL, &win, "c");
    assert(aoe_ui_edit_accept_char_ror(&e, ':'));
    set_text(NULL, &win, "cc");
    assert(!aoe_ui_edit_accept_char_ror(&e, ':') && aoe_ui_edit_accept_char_ror(&e, '\\'));
    /* Percent: two digits, or "10" then '0'. */
    e.format = 8;
    set_text(NULL, &win, "10");
    e.length = 2;
    assert(aoe_ui_edit_accept_char_ror(&e, '0'));
    set_text(NULL, &win, "99");
    assert(!aoe_ui_edit_accept_char_ror(&e, '0'));

    /* Reading back: an over-long text is cut and beeps. */
    e.format = 0;
    set_text(NULL, &win, "toolongtext");
    beeps = 0;
    assert(strcmp(aoe_ui_edit_text_ror(&e), "toolo") == 0 && beeps == 1);

    /* The window procedure (live panel): Enter answers 0, Escape 1, a
       refused Escape undoes; typed characters pass the filter. */
    e.panel.active = 1;
    e.panel.visible = 1;
    e.hidden = 0;
    nacts = 0;
    aoe_ui_edit_window_proc_ror(&e, 0x100, 0xd, 0);
    aoe_ui_edit_window_proc_ror(&e, 0x100, 0x1b, 0);
    assert(nacts == 2 && acts[0] == 0 && acts[1] == 1 && win.undos == 0);
    reject_escape = 1;
    aoe_ui_edit_window_proc_ror(&e, 0x100, 0x1b, 0);
    assert(win.undos == 1);
    set_text(NULL, &win, "ab");
    win.sel_start = win.sel_end = 2;
    assert(aoe_ui_edit_window_proc_ror(&e, 0x102, 'c', 0) == 7 && strcmp(win.text, "abc") == 0);
    win.invalidates = 0;
    /* The selection moved, so it repainted. */
    assert(win.invalidates == 0);
    assert(aoe_ui_edit_window_proc_ror(&e, 0x102, 0xd, 0) == 0 && strcmp(win.text, "abc") == 0);
    assert(aoe_ui_edit_class_proc_ror(NULL, 0x102, 'x', 0) == 0);

    /* Focus: the window gets it and selects all; losing it hides. */
    focused = area.window;
    e.panel.vt->set_focus(&e.panel, 1);
    assert(focused == &win && win.sel_start == 0 && win.sel_end == 3);
    e.panel.vt->set_focus(&e.panel, 0);
    assert(focused == area.window && e.hidden && !win.shown);

    /* Bevel 3 insets the window by 2; multi-line leaves the scrollbar. */
    aoe_ui_edit_set_bevel_ror(&e, 3, NULL);
    assert(e.inset == 2 && win.x == e.panel.x + 2 && win.w == 200 - 4 - 1);
    e.format = 7;
    e.panel.vt->set_rect(&e.panel, 10, 20, 200, 24);
    assert(e.inner.right == e.panel.rect.right - 2 - 16 - 1 &&
           e.inner.left == e.panel.rect.left + 3);
    /* Inactive hides. */
    e.hidden = 0;
    e.panel.vt->set_active(&e.panel, 0);
    assert(e.hidden);
    aoe_ui_edit_release_ror(&e);
    assert(!e.window && !e.text);
    aoe_ui_panel_release_ror(&parent);
    return 0;
}
