/* Patched RoR 1.0a drop-down panel 0x00460a00..0x00461f50. */
#include "ui_dropdown.h"

#include <assert.h>
#include <string.h>

static uint32_t now(void *c) { (void)c; return 0; }
static int32_t width(void *c, uintptr_t f, const char *t, int32_t n) { (void)c; (void)f; (void)t; return n * 8; }

static int32_t codes[64], n_actions;
static AoeUiPanelRor *froms[64];
static int32_t record(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                      int32_t value, int32_t extra) {
    (void)p; (void)value; (void)extra;
    froms[n_actions] = from;
    codes[n_actions++] = code;
    return 1;
}

static AoeUiPanelRor *modal;
static int pops;
static int pop_modal(void *c) { (void)c; ++pops; return 0; }
static void set_modal(void *c, AoeUiPanelRor *p) { (void)c; modal = p; }

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    static AoeUiPanelRor parent;
    static AoeUiPanelVtblRor pvt;
    static AoeUiDropdownRor d, up;
    static const char *names[] = {"apple", "banana", "cherry", "date",
                                  "elder", "fig", "grape", "hazel"};
    AoeUiDropdownOpsRor ops = {NULL, pop_modal, set_modal};
    AoeUiPanelRor *p = &d.panel;

    pf.now_ms = now;
    pf.text_width = width;
    sys.platform = &pf;
    aoe_ui_panel_init_ror(&parent, &sys);
    aoe_ui_panel_setup_ror(&parent, &area, NULL, 0, 0, 640, 480, 0);
    pvt = aoe_ui_panel_vtable_ror;
    pvt.action = record;
    parent.vt = &pvt;

    aoe_ui_dropdown_init_ror(&d, &sys, &ops);
    assert(d.border == 1 && d.panel.field_ee == 6 && d.state == 0);
    assert(aoe_ui_dropdown_setup_ror(&d, &area, &parent, 1, 8, 14, 1, 3, 1,
                                     0x1ff, 100, 50, 150, 20, 32, 2, 20, 20,
                                     150, 100, names, 8, 16, 0, 12));
    /* Closed: the text is w - button - 1 wide; the parts are hidden. */
    assert(d.state == 1 && d.text_w == 129 && d.text_h == 20 &&
           d.border_color == 0xff && d.line_height == 14);
    assert(p->width == 150 && p->height == 20 && p->clip_to_parent == 1);
    assert(!d.list->text.panel.active && !d.scrollbar->panel.active);
    assert(d.list->hover_select && d.list->text.count == 8 &&
           d.list->text.selected == 2);
    assert(strcmp(d.text->lines->text, "cherry") == 0);
    assert(d.text->align == 1 && d.list->text.align == 1 &&
           d.list->text.vertical == 3);

    /* Opening: rows of line height + 1 + 2 * spacing, plus the margins. */
    n_actions = 0;
    p->vt->action(p, &d.button->button.panel, 2, 0, 0);
    assert(d.state == 2 && d.list_h == 8 * (14 + 1 + 2) + 2);
    assert(p->height == d.list_h + 1 + 20 && p->width == 150);
    assert(d.list->text.panel.active && modal == p && !d.opened_up);
    /* The base action then forwards the button's press to the parent. */
    assert(p->clip_to_parent == 0 && n_actions == 2 && codes[0] == 1 &&
           froms[0] == p && codes[1] == 2 &&
           froms[1] == &d.button->button.panel);
    assert(p->current == &d.list->text.panel);

    /* Choosing a line closes (2), tells the parent (0), and the list's
       action is forwarded. */
    p->vt->action(p, &d.list->text.panel, 3, 4, 0);
    assert(d.state == 1 && d.selected == 4 && p->height == 20);
    assert(strcmp(d.text->lines->text, "elder") == 0);
    assert(n_actions == 5 && codes[2] == 2 && codes[3] == 0 &&
           froms[3] == p && codes[4] == 3 &&
           froms[4] == &d.list->text.panel);
    assert(pops == 1 && modal == NULL && p->current == &d.text->panel);

    /* Keys need focus: Down opens, Escape closes. */
    assert(p->vt->key_down(p, 0x28, 1, 0, 0, 0) == 0);
    p->has_focus = 1;
    assert(p->vt->key_down(p, 0x28, 1, 0, 0, 0) == 1 && d.state == 2);
    assert(p->vt->key_down(p, 0x1b, 1, 0, 0, 0) == 1 && d.state == 1);
    p->has_focus = 0;

    /* Dragging the button's press into the open list hands it over. */
    aoe_ui_dropdown_set_state_ror(&d, 2);
    {
        AoeUiPanelRor *b = &d.button->button.panel, *l = &d.list->text.panel;
        b->captured = 1;
        sys.capture = b;
        b->vt->mouse_move(b, l->clip.left + 2, l->clip.top + 2, 0, 0);
        assert(sys.capture == l && !b->captured);
    }
    aoe_ui_dropdown_set_state_ror(&d, 1);

    /* A press outside the open list closes it and re-selects. */
    aoe_ui_dropdown_set_state_ror(&d, 2);
    d.list->text.selected = 6;
    assert(p->vt->mouse_down(p, 1, 600, 5, 0, 0) == 1 && d.state == 1);
    assert(d.list->text.selected == 4 &&
           strcmp(d.text->lines->text, "elder") == 0);

    /* List access. */
    assert(aoe_ui_dropdown_selected_ror(&d) == 4);
    assert(strcmp(aoe_ui_dropdown_selected_text_ror(&d), "elder") == 0);
    assert(aoe_ui_dropdown_index_of_ror(&d, "fig") == 5);
    assert(aoe_ui_dropdown_append_ror(&d, "iris", 77));
    assert(aoe_ui_dropdown_index_of_id_ror(&d, 77) == 8);
    assert(aoe_ui_dropdown_id_ror(&d, 8) == 77);
    aoe_ui_dropdown_select_id_ror(&d, 77);
    assert(d.selected == 8 && aoe_ui_dropdown_selected_id_ror(&d) == 77 &&
           strcmp(d.text->lines->text, "iris") == 0);
    assert(aoe_ui_dropdown_remove_ror(&d, 0) && d.list->text.count == 8);
    aoe_ui_dropdown_set_sorted_ror(&d, 1);
    assert(d.list->text.sorted == 1);

    /* Bevel: the text maps 2..4 to 5..7, the others keep the type. */
    {
        const uint8_t colors[6] = {1, 2, 3, 4, 5, 6};
        aoe_ui_dropdown_set_bevel_ror(&d, 3, colors);
        assert(d.text->bevel_type == 6 && d.scrollbar->bevel_type == 3 &&
               d.button->button.bevel_type == 3);
    }
    /* Border: the text follows with the saved colour. */
    aoe_ui_dropdown_set_border_ror(&d, 0);
    assert(d.border == 0 && d.text->border == 0 && d.text->bevel_type == 0);
    /* Pictures: none gives the button frame -1; the scrollbar shares it. */
    {
        int pic, s1;
        aoe_ui_dropdown_set_pictures_ror(&d, &pic, 3, &s1, NULL, NULL, NULL);
        assert(d.button->button.pictures[0] == &pic &&
               d.button->button.frames[0] == 3 &&
               d.scrollbar->pictures[0] == &pic &&
               d.scrollbar->pictures[1] == &s1);
        aoe_ui_dropdown_set_pictures_ror(&d, NULL, 3, NULL, NULL, NULL, NULL);
        assert(d.button->button.frames[0] == -1);
    }
    /* The button on the left re-lays out the closed row. */
    aoe_ui_dropdown_set_button_left_ror(&d, 1);
    assert(d.button_left == 1 && d.state == 1);

    /* Near the bottom the list opens upward, keeping the text's bottom. */
    aoe_ui_dropdown_init_ror(&up, &sys, &ops);
    assert(aoe_ui_dropdown_setup_ror(&up, &area, &parent, 1, 8, 14, 1, 3, 1,
                                     0, 100, 440, 150, 20, 32, 0, 20, 20, 150,
                                     100, names, 8, 16, 0, 12));
    aoe_ui_dropdown_set_state_ror(&up, 2);
    /* The list (138 high below y 461) overflows by 123 at the bottom and
       would not overflow at the top (138 + 4 - 440 < 0). */
    assert(up.opened_up && up.list_h == 138 && up.panel.height == 159 &&
           up.panel.y == 20 - 159 + 440);
    assert(up.list->text.panel.vt == &aoe_ui_list_vtable_ror.base);
    aoe_ui_dropdown_set_state_ror(&up, 1);
    assert(!up.opened_up && up.panel.y == 440 && up.panel.height == 20);
    aoe_ui_dropdown_release_ror(&up);

    /* A long list near the top shrinks downward to whole rows instead. */
    {
        static const char *many[40];
        int i;
        for (i = 0; i < 40; ++i) many[i] = names[i % 8];
        aoe_ui_dropdown_init_ror(&up, &sys, &ops);
        assert(aoe_ui_dropdown_setup_ror(&up, &area, &parent, 1, 8, 14, 1, 3,
                                         1, 0, 100, 100, 150, 20, 32, 0, 20,
                                         20, 150, 100, many, 40, 16, 0, 12));
        aoe_ui_dropdown_set_state_ror(&up, 2);
        /* 40 rows (682) end at 803: 327 too low, 586 too high, so the
           list loses 327 and snaps to the 20 rows that fit (342). */
        assert(!up.opened_up && up.panel.y == 100 && up.list_h == 342 &&
               up.panel.height == 342 + 20 + 1 && up.list->text.visible == 20 &&
               up.scrollbar->panel.active);
    }

    aoe_ui_dropdown_clear_ror(&d);
    assert(d.list->text.count == 0);
    aoe_ui_dropdown_release_ror(&d);
    aoe_ui_dropdown_release_ror(&up);
    assert(!d.list && !d.text && !d.button && !d.scrollbar);
    return 0;
}
