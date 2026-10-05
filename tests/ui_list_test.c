/* Patched RoR 1.0a list panel 0x00464a50..0x004656e0. */
#include "ui_list.h"

#include <assert.h>
#include <string.h>

static uint32_t clock_ms;
static uint32_t now(void *c) { (void)c; return clock_ms; }
static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }

static int32_t codes[64], values[64], n_actions;
static int32_t record(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                      int32_t value, int32_t extra) {
    (void)p; (void)from; (void)extra;
    codes[n_actions] = code;
    values[n_actions++] = value;
    return 1;
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    static AoeUiPanelRor parent;
    static AoeUiPanelVtblRor pvt;
    static AoeUiListRor l;
    static const char *names[] = {"apple", "Banana", "cherry", "avocado",
                                  "date", "elder", "fig", "grape"};
    int i;

    pf.now_ms = now;
    pf.text_width = width;
    sys.platform = &pf;
    aoe_ui_panel_init_ror(&parent, &sys);
    pvt = aoe_ui_panel_vtable_ror;
    pvt.action = record;
    parent.vt = &pvt;

    aoe_ui_list_init_ror(&l, &sys);
    assert(l.highlight && l.scroll_interval == 50 && l.text.spacing == 1 &&
           l.text.margin == 1 && l.text.panel.handles_mouse &&
           l.text.panel.field_ee == 4);
    aoe_ui_panel_setup_ror(&l.text.panel, &area, &parent, 0, 100, 200, 66, 0);
    /* Line height 18: pitch 20, rows of 21 with spacing 1 -> 3 visible. */
    aoe_ui_text_set_font_ror(&l.text, 1, 8, 18);
    for (i = 0; i < 8; ++i) aoe_ui_text_append_ror(&l.text, names[i], i);
    assert(l.text.visible == 3 && l.text.top == 0 && l.text.bottom == 2);

    /* Rows: y 100 + margin 1 + spacing 1 = 102 starts row 0. */
    assert(aoe_ui_list_row_at_ror(&l, 102) == 0 &&
           aoe_ui_list_row_at_ror(&l, 123) == 1 &&
           aoe_ui_list_row_at_ror(&l, 165) == 3);

    /* Pressing selects and tells the parent. */
    n_actions = 0;
    l.text.panel.vt->actions[AOE_UI_LEFT_DOWN](&l.text.panel, 5, 124, 0, 0);
    assert(l.text.selected == 1 && n_actions == 1 && codes[0] == 1 && values[0] == 1);
    assert(sys.capture == &l.text.panel);
    l.text.panel.vt->actions[AOE_UI_LEFT_UP](&l.text.panel, 5, 124, 0, 0);
    assert(codes[1] == 2 && values[1] == 1 && !sys.capture);

    /* Down at the bottom edge scrolls; Up at the top edge scrolls back. */
    aoe_ui_list_move_ror(&l, 2, 0, 1);
    assert(l.text.selected == 2 && l.text.top == 0);
    aoe_ui_list_move_ror(&l, 2, 0, 1);
    assert(l.text.selected == 3 && l.text.top == 1 && l.text.bottom == 3);
    aoe_ui_list_move_ror(&l, 3, 0, 1);
    aoe_ui_list_move_ror(&l, 3, 0, 1);
    assert(l.text.selected == 1 && l.text.top == 1);
    aoe_ui_list_move_ror(&l, 3, 0, 1);
    assert(l.text.selected == 0 && l.text.top == 0);
    /* Page down: first to the window's bottom, then a page. */
    aoe_ui_list_move_ror(&l, 4, 0, 1);
    assert(l.text.selected == 2 && l.text.top == 0);
    aoe_ui_list_move_ror(&l, 4, 0, 1);
    assert(l.text.top == 2 && l.text.selected == 4);
    aoe_ui_list_move_ror(&l, 7, 0, 1);
    assert(l.text.selected == 7 && l.text.bottom == 7);
    aoe_ui_list_move_ror(&l, 2, 0, 1); /* already last */
    assert(l.text.selected == 7);
    aoe_ui_list_move_ror(&l, 6, 0, 1);
    assert(l.text.selected == 0 && l.text.top == 0);
    /* Direct moves clamp and scroll into view. */
    aoe_ui_list_move_ror(&l, 1, 40, 1);
    assert(l.text.selected == 7 && l.text.bottom == 7);
    aoe_ui_list_move_ror(&l, 1, -3, 1);
    assert(l.text.selected == 0 && l.text.top == 0);

    /* Keys. */
    n_actions = 0;
    assert(l.text.panel.vt->key_action(&l.text.panel, 0x28, 1, 0, 0, 0) == 1 &&
           l.text.selected == 1);
    assert(l.text.panel.vt->key_action(&l.text.panel, 0x0d, 1, 0, 0, 0) == 1 &&
           codes[n_actions - 1] == 3 && values[n_actions - 1] == 1);
    assert(!l.text.panel.vt->key_action(&l.text.panel, 'A', 1, 0, 0, 0));

    /* Type-ahead: the next line starting with the letter after the
       selection, wrapping to the start. */
    assert(l.text.panel.vt->char_action(&l.text.panel, 'a', 1) && l.text.selected == 3);
    assert(l.text.panel.vt->char_action(&l.text.panel, 'A', 1) && l.text.selected == 0);
    assert(l.text.panel.vt->char_action(&l.text.panel, 'b', 1) && l.text.selected == 1);
    assert(!l.text.panel.vt->char_action(&l.text.panel, 'z', 1) && l.text.selected == 1);

    /* Dragging below arms auto-scroll at the interval. */
    clock_ms = 1000;
    l.text.panel.vt->actions[AOE_UI_LEFT_DRAG](&l.text.panel, 5, 300, 0, 0);
    assert(l.scroll_down && !l.scroll_up && l.scroll_ms == 950);
    l.text.panel.vt->handle_idle(&l.text.panel);
    assert(l.text.selected == 3 && l.text.bottom == 3 && l.scroll_ms == 1000);
    clock_ms = 1020;
    l.text.panel.vt->handle_idle(&l.text.panel);
    assert(l.text.selected == 3);
    clock_ms = 1050;
    l.text.panel.vt->handle_idle(&l.text.panel);
    assert(l.text.selected == 4);
    /* Back inside stops it. */
    l.text.panel.vt->actions[AOE_UI_LEFT_DRAG](&l.text.panel, 5, 102, 0, 0);
    assert(!l.scroll_down && l.text.selected == l.text.top);

    /* Hover selects only when enabled. */
    l.text.panel.vt->actions[AOE_UI_HOVER](&l.text.panel, 5, 124, 0, 0);
    assert(l.text.selected == l.text.top);
    l.hover_select = 1;
    assert(l.text.panel.vt->actions[AOE_UI_HOVER](&l.text.panel, 5, 124, 0, 0) &&
           l.text.selected == l.text.top + 1);

    /* Bevel types set the spacing. */
    {
        const uint8_t c[6] = {0};
        const AoeUiTextVtblRor *vt = (const AoeUiTextVtblRor *)l.text.panel.vt;
        vt->set_bevel(&l.text, 4, c);
        assert(l.text.spacing == 3 && l.text.margin == 3);
        vt->set_bevel(&l.text, 0, c); /* no bevel: the text gives margin 0 */
        assert(l.text.spacing == 1 && l.text.margin == 1);
    }
    aoe_ui_list_release_ror(&l);
    aoe_ui_panel_release_ror(&parent);
    return 0;
}
