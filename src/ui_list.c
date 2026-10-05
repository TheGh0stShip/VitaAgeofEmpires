#include "ui_list.h"

#include <ctype.h>

#define L(p) ((AoeUiListRor *)(p))

static uint32_t now(const AoeUiListRor *l) {
    const AoeUiPlatformRor *pf =
        l->text.panel.system ? l->text.panel.system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

void aoe_ui_list_init_ror(AoeUiListRor *l, AoeUiSystemRor *system) {
    if (!l) return;
    aoe_ui_text_init_ror(&l->text, system);
    l->text.panel.vt = &aoe_ui_list_vtable_ror.base;
    l->text.panel.drag_slack = 0;
    l->scroll_ms = 0;
    l->scroll_up = l->scroll_down = 0;
    l->hover_select = 0;
    l->highlight = 1;
    l->text.panel.field_ee = 4;
    l->text.bevel_type = 1;
    l->scroll_interval = 50;
    l->text.panel.handles_mouse = 1;
    l->text.spacing = 1;
    l->text.margin = 1;
}

void aoe_ui_list_release_ror(AoeUiListRor *l) {
    if (l) aoe_ui_text_release_ror(&l->text);
}

static void destroy(AoeUiPanelRor *p) { aoe_ui_list_release_ror(L(p)); }

int32_t aoe_ui_list_row_at_ror(const AoeUiListRor *l, int32_t y) {
    int32_t spacing, pitch;
    if (!l) return 0;
    spacing = l->text.spacing;
    pitch = spacing ? l->text.pitch - 1 + spacing * 2 : l->text.pitch;
    if (!pitch) return l->text.top;
    return (y - l->text.margin - l->text.panel.y - spacing) / pitch + l->text.top;
}

static void report(AoeUiListRor *l, int32_t code) {
    AoeUiPanelRor *parent = l->text.panel.parent;
    int16_t sel = l->text.selected;
    if (parent && sel >= 0 && sel < l->text.count)
        parent->vt->action(parent, &l->text.panel, code, sel, 0);
}

void aoe_ui_list_move_ror(AoeUiListRor *l, uint8_t how, int16_t value,
                          int32_t update) {
    AoeUiTextRor *t;
    int16_t old, edge;
    if (!l) return;
    t = &l->text;
    old = t->selected;
    switch (how) {
    case 1:
        t->selected = value;
        if (value < 0) t->selected = 0;
        else if (value > t->count - 1) t->selected = (int16_t)(t->count - 1);
        if (t->selected < t->top)
            aoe_ui_text_scroll_ror(t, 1, t->selected, update);
        else if (t->selected > t->bottom)
            aoe_ui_text_scroll_ror(t, 0, (int16_t)(t->selected - t->bottom), update);
        break;
    case 2:
        edge = t->bottom;
        if (old < t->top || old > t->bottom) t->selected = edge;
        if (t->selected >= edge) {
            if (t->selected < t->count - 1) {
                ++t->selected;
                aoe_ui_text_scroll_ror(t, 2, 0, update);
            }
        } else {
            ++t->selected;
        }
        break;
    case 3:
        edge = t->top;
        if (old < edge || old > t->bottom) t->selected = edge;
        if (t->selected <= edge) {
            if (t->selected > 0) {
                --t->selected;
                aoe_ui_text_scroll_ror(t, 3, 0, update);
            }
        } else {
            --t->selected;
        }
        break;
    case 4:
        if (old < t->top || old > t->bottom) t->selected = t->bottom;
        if (t->selected == t->bottom)
            aoe_ui_text_scroll_ror(t, 4, 0, update);
        t->selected = t->bottom;
        break;
    case 5:
        if (old < t->top || old > t->bottom) t->selected = t->top;
        if (t->selected == t->top)
            aoe_ui_text_scroll_ror(t, 5, 0, update);
        t->selected = t->top;
        break;
    case 6:
        aoe_ui_text_scroll_ror(t, 6, 0, update);
        t->selected = t->top;
        break;
    case 7:
        aoe_ui_text_scroll_ror(t, 7, 0, update);
        t->selected = t->bottom;
        break;
    default:
        break;
    }
    t->panel.vt->set_redraw(&t->panel, 1);
    if (t->selected != old) report(l, 1);
}

void aoe_ui_list_select_at_ror(AoeUiListRor *l, int32_t x, int32_t y) {
    (void)x;
    if (l) aoe_ui_list_move_ror(l, 1, (int16_t)aoe_ui_list_row_at_ror(l, y), 1);
}

void aoe_ui_list_set_highlight_ror(AoeUiListRor *l, int32_t on) {
    if (l) l->highlight = on;
}

static int32_t idle(AoeUiPanelRor *p) {
    AoeUiListRor *l = L(p);
    uint32_t t;
    aoe_ui_panel_handle_idle_ror(p);
    if (!l->scroll_up && !l->scroll_down) return 0;
    t = now(l);
    if (t - l->scroll_ms < l->scroll_interval) return 0;
    l->scroll_ms = t;
    if (l->scroll_up) {
        if (l->text.selected != l->text.top)
            aoe_ui_list_move_ror(l, 1, l->text.top, 1);
        aoe_ui_list_move_ror(l, 3, 0, 1);
    } else {
        if (l->text.selected != l->text.bottom)
            aoe_ui_list_move_ror(l, 1, l->text.bottom, 1);
        aoe_ui_list_move_ror(l, 2, 0, 1);
    }
    return 0;
}

static int32_t hover(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    AoeUiListRor *l = L(p);
    int32_t row;
    (void)c; (void)s;
    if (!l->hover_select) return 0;
    if (!p->vt->is_inside(p, x, y)) return 0;
    row = (int16_t)aoe_ui_list_row_at_ror(l, y);
    if (l->text.top <= row && row <= l->text.bottom)
        aoe_ui_list_select_at_ror(l, x, y);
    return 1;
}

static int32_t left_down(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)c; (void)s;
    aoe_ui_panel_capture_ror(p);
    aoe_ui_list_select_at_ror(L(p), x, y);
    return 1;
}

static int32_t left_drag(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    AoeUiListRor *l = L(p);
    (void)c; (void)s;
    if (y >= p->y) {
        if (y < p->height + p->y) {
            l->scroll_up = l->scroll_down = 0;
            aoe_ui_list_select_at_ror(l, x, y);
            return 1;
        }
        l->scroll_up = 0;
        if (l->scroll_down) return 1;
        l->scroll_down = 1;
    } else {
        l->scroll_down = 0;
        if (l->scroll_up) return 1;
        l->scroll_up = 1;
    }
    l->scroll_ms = now(l) - l->scroll_interval;
    return 1;
}

static int32_t left_up(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    AoeUiListRor *l = L(p);
    (void)x; (void)y; (void)c; (void)s;
    aoe_ui_panel_release_capture_ror(p);
    l->scroll_up = l->scroll_down = 0;
    report(l, 2);
    return 1;
}

static int32_t left_dbl(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)c; (void)s;
    aoe_ui_list_select_at_ror(L(p), x, y);
    report(L(p), 3);
    return 1;
}

static int32_t key_action(AoeUiPanelRor *p, int32_t key, int32_t count,
                          int32_t a, int32_t b, int32_t c) {
    AoeUiListRor *l = L(p);
    (void)count; (void)a; (void)b; (void)c;
    switch (key) {
    case 0x0d: report(l, 3); return 1;
    case 0x21: aoe_ui_list_move_ror(l, 5, 0, 1); return 1;
    case 0x22: aoe_ui_list_move_ror(l, 4, 0, 1); return 1;
    case 0x23: aoe_ui_list_move_ror(l, 7, 0, 1); return 1;
    case 0x24: aoe_ui_list_move_ror(l, 6, 0, 1); return 1;
    case 0x26: aoe_ui_list_move_ror(l, 3, 0, 1); return 1;
    case 0x28: aoe_ui_list_move_ror(l, 2, 0, 1); return 1;
    default: return 0;
    }
}

static int32_t char_action(AoeUiPanelRor *p, int32_t ch, int32_t count) {
    AoeUiListRor *l = L(p);
    const AoeUiLineRor *line;
    int target = toupper((unsigned char)ch), i = 0;
    (void)count;
    for (line = l->text.lines; line; line = line->next, ++i)
        if (l->text.selected < i && toupper((unsigned char)line->text[0]) == target) {
            aoe_ui_list_move_ror(l, 1, (int16_t)i, 1);
            return 1;
        }
    if (l->text.selected > 0)
        for (i = 0, line = l->text.lines; line; line = line->next) {
            if (toupper((unsigned char)line->text[0]) == target) {
                aoe_ui_list_move_ror(l, 1, (int16_t)i, 1);
                return 1;
            }
            if (l->text.selected <= ++i) return 0;
        }
    return 0;
}

static void set_focus(AoeUiPanelRor *p, int32_t focus) {
    aoe_ui_panel_set_focus_ror(p, focus);
    if (p->has_focus) report(L(p), 1);
}

static void bevel(AoeUiTextRor *t, int32_t type, const uint8_t colors[6]) {
    switch (type) {
    case 3: case 6: t->spacing = 2; break;
    case 4: case 7: t->spacing = 3; break;
    default: t->spacing = 1; break;
    }
    aoe_ui_text_bevel_slot_ror(t, type, colors);
    if (!t->margin) {
        t->margin = 1;
        aoe_ui_text_layout_ror(t, 1);
    }
}

const AoeUiTextVtblRor aoe_ui_list_vtable_ror = {
    {destroy,
     aoe_ui_panel_setup_ror,
     aoe_ui_panel_set_rect_ltrb_ror,
     aoe_ui_text_set_rect_slot_ror,
     aoe_ui_panel_set_color_ror,
     aoe_ui_text_set_active_slot_ror,
     aoe_ui_panel_set_position_mode_ror,
     aoe_ui_panel_set_fixed_position_ror,
     aoe_ui_panel_set_redraw_ror,
     aoe_ui_panel_redraw_overlaps_ror,
     aoe_ui_panel_begin_draw_ror,
     aoe_ui_panel_end_draw_ror,
     aoe_ui_panel_draw_ror,
     aoe_ui_panel_draw_rect_ror,
     aoe_ui_panel_draw_offset_ror,
     aoe_ui_panel_draw_parent_rect_ror,
     aoe_ui_panel_draw_parent_offset_ror,
     aoe_ui_panel_validate_ror,
     aoe_ui_panel_handle_message_ror,
     idle,
     aoe_ui_panel_handle_size_ror,
     aoe_ui_panel_handle_paint_ror,
     aoe_ui_panel_key_down_ror,
     aoe_ui_panel_char_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_ignore_ror,
     aoe_ui_panel_mouse_down_ror,
     aoe_ui_panel_mouse_move_ror,
     aoe_ui_panel_mouse_up_ror,
     aoe_ui_panel_mouse_dbl_ror,
     {hover, left_down, aoe_ui_panel_no_action_ror, left_drag, left_up,
      left_dbl, aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
      aoe_ui_panel_no_action_ror},
     key_action,
     aoe_ui_panel_zero_ror,
     char_action,
     aoe_ui_text_action_ror,
     aoe_ui_panel_get_true_rect_ror,
     aoe_ui_panel_is_inside_ror,
     set_focus,
     aoe_ui_panel_set_tab_order_ror,
     aoe_ui_panel_get_help_ror,
     aoe_ui_panel_stop_sound_ror,
     aoe_ui_panel_restart_sound_ror,
     aoe_ui_panel_slot_e0_ror},
    aoe_ui_text_lines_slot_ror,
    aoe_ui_text_set_text_id_ror,
    aoe_ui_text_set_text_ror,
    bevel,
};
