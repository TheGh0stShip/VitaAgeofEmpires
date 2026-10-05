#include "ui_scrollbar.h"

#include <string.h>

#define S(p) ((AoeUiScrollbarRor *)(p))

static uint32_t now(AoeUiScrollbarRor *s) {
    const AoeUiPlatformRor *pf = s->panel.system ? s->panel.system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

static void redraw(AoeUiScrollbarRor *s) { s->panel.vt->set_redraw(&s->panel, 1); }

static void set_rect4(AoeUiRectRor *r, int32_t l, int32_t t, int32_t rr, int32_t b) {
    r->left = l;
    r->top = t;
    r->right = rr;
    r->bottom = b;
}

void aoe_ui_scrollbar_init_ror(AoeUiScrollbarRor *s, AoeUiSystemRor *system) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    aoe_ui_panel_init_ror(&s->panel, system);
    s->panel.vt = &aoe_ui_scrollbar_vtable_ror;
    s->repeat_interval = 100;
    s->repeat_delay = 250;
    s->bevel_type = 1;
    s->panel.field_ee = 5;
    s->panel.drag_slack = 0;
}

void aoe_ui_scrollbar_release_ror(AoeUiScrollbarRor *s) {
    if (s) aoe_ui_panel_release_ror(&s->panel);
}

static void destroy(AoeUiPanelRor *p) { aoe_ui_scrollbar_release_ror(S(p)); }

int aoe_ui_scrollbar_setup_ror(AoeUiScrollbarRor *s, AoeUiAreaRor *area,
                               AoeUiPanelRor *parent, int32_t x, int32_t y,
                               int32_t w, int32_t h, int32_t field_11c,
                               int32_t thumb_size, AoeUiPanelRor *target,
                               int32_t range, int32_t horizontal) {
    if (!s) return 0;
    s->horizontal = horizontal;
    s->field_11c = field_11c;
    s->thumb_size = thumb_size;
    s->target = target;
    s->range = range;
    aoe_ui_panel_setup_ror(&s->panel, area, parent, x, y, w, h, 0);
    return 1;
}

void aoe_ui_scrollbar_place_parts_ror(AoeUiScrollbarRor *s) {
    if (!s) return;
    if (s->horizontal == 1) {
        int32_t x = s->thumb_x, r = s->panel.height - 5 + x;
        s->thumb.left = x;
        s->thumb.right = r;
        s->page_back.right = x - 1;
        s->page_back.left = s->back.right + 1;
        s->page_forward.left = r + 1;
        s->page_forward.right = s->forward.left - 1;
        return;
    }
    {
        int32_t y = s->thumb_y, b = s->panel.width - 5 + y;
        s->thumb.top = y;
        s->thumb.bottom = b;
        s->page_back.bottom = y - 1;
        s->page_back.top = s->back.bottom + 1;
        s->page_forward.top = b + 1;
        s->page_forward.bottom = s->forward.top - 1;
    }
}

void aoe_ui_scrollbar_set_rect_slot_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                        int32_t w, int32_t h) {
    AoeUiScrollbarRor *s = S(p);
    aoe_ui_panel_set_rect_ror(p, x, y, w, h);
    if (s->horizontal == 1) {
        int32_t hh = p->height, top = p->y + 2, bottom = hh - 5 + p->y;
        int32_t left = p->x + 2, fwd = left + (p->width - hh), last = p->x - 3 + hh;
        set_rect4(&s->back, left, top, last, bottom);
        set_rect4(&s->forward, fwd, top, fwd - 5 + hh, bottom);
        s->thumb.top = s->page_back.top = s->page_forward.top = top;
        s->thumb.bottom = s->page_back.bottom = s->page_forward.bottom = bottom;
        s->thumb_min_x = s->thumb_x = last + 1;
        s->thumb_max_x = fwd - hh + 4;
    } else {
        int32_t ww = p->width, left = p->x + 2, top = p->y + 2;
        int32_t fwd = top + (p->height - ww), right = ww - 5 + left;
        set_rect4(&s->back, left, top, right, ww - 5 + top);
        set_rect4(&s->forward, left, fwd, right, fwd - 5 + ww);
        s->thumb.left = s->page_back.left = s->page_forward.left = left;
        s->thumb.right = s->page_back.right = s->page_forward.right = right;
        s->thumb_max_y = fwd - ww + 4;
        s->thumb_min_y = s->thumb_y = ww - 5 + top + 1;
    }
    aoe_ui_scrollbar_place_parts_ror(s);
}

void aoe_ui_scrollbar_set_position_ror(AoeUiScrollbarRor *s, int32_t position) {
    if (!s) return;
    s->position = position;
    if (s->horizontal == 1) {
        if (s->range > 0)
            s->thumb_x = ((s->thumb_size / 2 + s->thumb_max_x - s->thumb_min_x) *
                          position) / s->range + s->thumb_min_x;
        if (s->thumb_x < s->thumb_min_x) s->thumb_x = s->thumb_min_x;
        else if (s->thumb_max_x < s->thumb_x) s->thumb_x = s->thumb_max_x;
    } else {
        if (s->range > 0) {
            if (position == s->range - 1)
                s->thumb_y = s->thumb_max_y;
            else
                s->thumb_y = ((s->thumb_size / 2 + s->thumb_max_y - s->thumb_min_y) *
                              position) / s->range + s->thumb_min_y;
        }
        if (s->thumb_y < s->thumb_min_y) s->thumb_y = s->thumb_min_y;
        else if (s->thumb_max_y < s->thumb_y) s->thumb_y = s->thumb_max_y;
    }
    aoe_ui_scrollbar_place_parts_ror(s);
    redraw(s);
}

void aoe_ui_scrollbar_set_range_ror(AoeUiScrollbarRor *s, int32_t range,
                                    int32_t position) {
    if (!s) return;
    s->range = range;
    aoe_ui_scrollbar_set_position_ror(s, position);
}

void aoe_ui_scrollbar_set_pictures_ror(AoeUiScrollbarRor *s, void *picture,
                                       int32_t frame1, int32_t frame2,
                                       int32_t frame3, int32_t frame4) {
    if (!s) return;
    s->picture = picture;
    s->frames[0] = frame1;
    s->frames[1] = frame2;
    s->frames[2] = frame3;
    s->frames[3] = frame4;
}

void aoe_ui_scrollbar_set_bevel_ror(AoeUiScrollbarRor *s, int32_t type,
                                    const uint8_t colors[6]) {
    if (!s) return;
    s->bevel_type = type;
    if (colors) memcpy(s->bevel, colors, sizeof(s->bevel));
}

void aoe_ui_scrollbar_scroll_ror(AoeUiScrollbarRor *s, int32_t code,
                                 int32_t position) {
    int32_t at;
    if (!s) return;
    if (s->target) {
        s->target->vt->action(s->target, &s->panel, code, position, 0);
        return;
    }
    at = s->position;
    switch (code) {
    case 0: position = at - s->line_step; break;
    case 1: position = at + s->line_step; break;
    case 2: position = at - s->page_step; break;
    case 3: position = at + s->page_step; break;
    case 4: break;
    default: return;
    }
    if (position < 0) position = 0;
    else if (s->range - 1 < position) position = s->range - 1;
    if (position != at) {
        aoe_ui_scrollbar_set_position_ror(s, position);
        if (s->panel.parent)
            s->panel.parent->vt->action(s->panel.parent, &s->panel, 4, position, 0);
    }
}

static void stop_repeat(AoeUiScrollbarRor *s) {
    s->repeating = 0;
    s->repeat_state = 0;
    redraw(s);
}

static int32_t idle(AoeUiPanelRor *p) {
    AoeUiScrollbarRor *s = S(p);
    uint32_t t;
    if (s->repeat_state == 1) {
        t = now(s);
        if (s->repeat_delay <= t - s->repeat_ms) {
            s->repeat_state = 2;
            s->repeat_ms = t - s->repeat_interval;
        }
    } else if (s->repeat_state == 2 && s->repeating &&
               s->repeat_interval <= (t = now(s)) - s->repeat_ms) {
        s->repeat_ms = t;
        switch (s->part) {
        case 1: aoe_ui_scrollbar_scroll_ror(s, 0, 0); break;
        case 2: aoe_ui_scrollbar_scroll_ror(s, 1, 0); break;
        case 4:
            if (!aoe_ui_point_in_rect_ror(s->mouse_x, s->mouse_y, &s->page_back))
                stop_repeat(s);
            else
                aoe_ui_scrollbar_scroll_ror(s, 2, 0);
            break;
        case 5:
            if (!aoe_ui_point_in_rect_ror(s->mouse_x, s->mouse_y, &s->page_forward))
                stop_repeat(s);
            else
                aoe_ui_scrollbar_scroll_ror(s, 3, 0);
            break;
        default: break;
        }
    }
    return 0;
}

static int32_t left_down(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t sh) {
    AoeUiScrollbarRor *s = S(p);
    (void)c; (void)sh;
    aoe_ui_panel_capture_ror(p);
    if (aoe_ui_point_in_rect_ror(x, y, &s->back)) {
        s->part = 1;
        s->repeating = 1;
        aoe_ui_scrollbar_scroll_ror(s, 0, 0);
    } else if (aoe_ui_point_in_rect_ror(x, y, &s->forward)) {
        s->part = 2;
        s->repeating = 1;
        aoe_ui_scrollbar_scroll_ror(s, 1, 0);
    } else if (aoe_ui_point_in_rect_ror(x, y, &s->thumb)) {
        s->part = 3;
        if (s->horizontal == 1) s->grab_x = x - s->thumb_x;
        else s->grab_y = y - s->thumb_y;
    } else if (aoe_ui_point_in_rect_ror(x, y, &s->page_back)) {
        s->part = 4;
        s->repeating = 1;
        aoe_ui_scrollbar_scroll_ror(s, 2, 0);
    } else if (aoe_ui_point_in_rect_ror(x, y, &s->page_forward)) {
        s->part = 5;
        s->repeating = 1;
        aoe_ui_scrollbar_scroll_ror(s, 3, 0);
    } else {
        s->part = 0;
    }
    if (s->part) {
        AoeUiPanelRor *target = s->target;
        if (target && target->tab_stop && target->parent)
            aoe_ui_panel_set_current_ror(target->parent, target);
        s->mouse_x = x;
        s->mouse_y = y;
        s->repeat_state = 1;
        s->repeat_ms = now(s);
        redraw(s);
    }
    return 0;
}

static int arm(AoeUiScrollbarRor *s, int inside, int32_t code) {
    if (!inside) {
        if (s->repeating) stop_repeat(s);
        return 0;
    }
    if (s->repeating) return 0;
    s->repeating = 1;
    s->repeat_state = 2;
    s->repeat_ms = now(s);
    aoe_ui_scrollbar_scroll_ror(s, code, 0);
    redraw(s);
    return 0;
}

static void drag_thumb(AoeUiScrollbarRor *s, int32_t mouse, int32_t last,
                       int32_t *thumb, int32_t lo, int32_t hi, int32_t grab) {
    int32_t delta = mouse - last, at = *thumb, pos;
    if ((at != lo || delta < 1 || grab + at <= mouse) &&
        (at != hi || delta > -1 || mouse < grab + at) && delta != 0) {
        at += delta;
        *thumb = at < lo ? lo : hi < at ? hi : at;
        aoe_ui_scrollbar_place_parts_ror(s);
        redraw(s);
        if (lo == hi) {
            aoe_ui_scrollbar_scroll_ror(s, 4, 0);
            return;
        }
        pos = ((*thumb - lo) * s->range) / (hi - lo);
        if (pos < 0) {
            aoe_ui_scrollbar_scroll_ror(s, 4, 0);
            return;
        }
        if (s->range - 1 < pos) pos = s->range - 1;
        aoe_ui_scrollbar_scroll_ror(s, 4, pos);
    }
}

static int32_t left_drag(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t sh) {
    AoeUiScrollbarRor *s = S(p);
    (void)c; (void)sh;
    switch (s->part) {
    case 1: arm(s, aoe_ui_point_in_rect_ror(x, y, &s->back), 0); break;
    case 2: arm(s, aoe_ui_point_in_rect_ror(x, y, &s->forward), 1); break;
    case 3:
        if (s->horizontal == 1)
            drag_thumb(s, x, s->mouse_x, &s->thumb_x, s->thumb_min_x,
                       s->thumb_max_x, s->grab_x);
        else
            drag_thumb(s, y, s->mouse_y, &s->thumb_y, s->thumb_min_y,
                       s->thumb_max_y, s->grab_y);
        break;
    case 4: arm(s, aoe_ui_point_in_rect_ror(x, y, &s->page_back), 2); break;
    case 5: arm(s, aoe_ui_point_in_rect_ror(x, y, &s->page_forward), 3); break;
    default: break;
    }
    if (s->part) {
        s->mouse_x = x;
        s->mouse_y = y;
    }
    return 0;
}

static int32_t left_up(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t sh) {
    AoeUiScrollbarRor *s = S(p);
    (void)x; (void)y; (void)c; (void)sh;
    if (p->parent) p->parent->vt->action(p->parent, p, 5, s->position, 0);
    aoe_ui_panel_release_capture_ror(p);
    s->part = 0;
    s->repeat_state = 0;
    s->repeating = 0;
    redraw(s);
    return 0;
}

/* +0xac 0x00405a00: 0. */
static int32_t no_key_action(AoeUiPanelRor *p, int32_t a, int32_t b,
                             int32_t c, int32_t d, int32_t e) {
    (void)p; (void)a; (void)b; (void)c; (void)d; (void)e;
    return 0;
}

const AoeUiPanelVtblRor aoe_ui_scrollbar_vtable_ror = {
    destroy,
    aoe_ui_panel_setup_ror,
    aoe_ui_panel_set_rect_ltrb_ror,
    aoe_ui_scrollbar_set_rect_slot_ror,
    aoe_ui_panel_set_color_ror,
    aoe_ui_panel_set_active_ror,
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
    {aoe_ui_panel_no_action_ror, left_down, aoe_ui_panel_no_action_ror,
     left_drag, left_up, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror},
    no_key_action,
    aoe_ui_panel_zero_ror,
    aoe_ui_panel_no_char_action_ror,
    aoe_ui_panel_action_ror,
    aoe_ui_panel_get_true_rect_ror,
    aoe_ui_panel_is_inside_ror,
    aoe_ui_panel_set_focus_ror,
    aoe_ui_panel_set_tab_order_ror,
    aoe_ui_panel_get_help_ror,
    aoe_ui_panel_stop_sound_ror,
    aoe_ui_panel_restart_sound_ror,
    aoe_ui_panel_slot_e0_ror,
};


/* ---- value scrollbars ---- */

void aoe_ui_value_scrollbar_init_ror(AoeUiValueScrollbarRor *v,
                                     AoeUiSystemRor *system,
                                     int32_t horizontal) {
    if (!v) return;
    aoe_ui_scrollbar_init_ror(&v->bar, system);
    v->field_1c4 = 1;
    v->field_1c8 = 0;
    v->bar.panel.vt = horizontal ? &aoe_ui_hscroll_vtable_ror
                                 : &aoe_ui_vscroll_vtable_ror;
}

void aoe_ui_value_scrollbar_release_ror(AoeUiValueScrollbarRor *v) {
    if (v) aoe_ui_scrollbar_release_ror(&v->bar);
}

static void value_destroy(AoeUiPanelRor *p) {
    aoe_ui_value_scrollbar_release_ror((AoeUiValueScrollbarRor *)p);
}

int aoe_ui_value_scrollbar_setup_ror(AoeUiValueScrollbarRor *v,
                                     AoeUiAreaRor *area, AoeUiPanelRor *parent,
                                     int32_t x, int32_t y, int32_t w,
                                     int32_t h, int32_t thumb_size,
                                     int32_t range, int32_t line_step,
                                     int32_t page_step, int32_t field_1c4,
                                     int32_t field_1c8) {
    if (!v) return 0;
    v->field_1c4 = field_1c4;
    v->field_1c8 = field_1c8;
    if (!aoe_ui_scrollbar_setup_ror(&v->bar, area, parent, x, y, w, h, 0,
                                    thumb_size, NULL, range,
                                    v->bar.panel.vt == &aoe_ui_hscroll_vtable_ror))
        return 0;
    v->bar.line_step = line_step;
    v->bar.page_step = page_step;
    return 1;
}

static void hscroll_set_rect(AoeUiPanelRor *p, int32_t x, int32_t y,
                             int32_t w, int32_t h) {
    AoeUiScrollbarRor *s = S(p);
    int32_t left, right;
    aoe_ui_scrollbar_set_rect_slot_ror(p, x, y, w, h);
    left = p->x;
    right = p->width - 1 + left;
    s->back.left = s->back.right = left;
    s->forward.left = s->forward.right = right;
    s->thumb_min_x = s->thumb_x = left + 1;
    s->thumb_max_x = right - p->height + 2;
    aoe_ui_scrollbar_place_parts_ror(s);
}

const AoeUiPanelVtblRor aoe_ui_vscroll_vtable_ror = {
    value_destroy,
    aoe_ui_panel_setup_ror,
    aoe_ui_panel_set_rect_ltrb_ror,
    aoe_ui_scrollbar_set_rect_slot_ror,
    aoe_ui_panel_set_color_ror,
    aoe_ui_panel_set_active_ror,
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
    {aoe_ui_panel_no_action_ror, left_down, aoe_ui_panel_no_action_ror,
     left_drag, left_up, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror},
    no_key_action,
    aoe_ui_panel_zero_ror,
    aoe_ui_panel_no_char_action_ror,
    aoe_ui_panel_action_ror,
    aoe_ui_panel_get_true_rect_ror,
    aoe_ui_panel_is_inside_ror,
    aoe_ui_panel_set_focus_ror,
    aoe_ui_panel_set_tab_order_ror,
    aoe_ui_panel_get_help_ror,
    aoe_ui_panel_stop_sound_ror,
    aoe_ui_panel_restart_sound_ror,
    aoe_ui_panel_slot_e0_ror,
};

const AoeUiPanelVtblRor aoe_ui_hscroll_vtable_ror = {
    value_destroy,
    aoe_ui_panel_setup_ror,
    aoe_ui_panel_set_rect_ltrb_ror,
    hscroll_set_rect,
    aoe_ui_panel_set_color_ror,
    aoe_ui_panel_set_active_ror,
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
    {aoe_ui_panel_no_action_ror, left_down, aoe_ui_panel_no_action_ror,
     left_drag, left_up, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror},
    no_key_action,
    aoe_ui_panel_zero_ror,
    aoe_ui_panel_no_char_action_ror,
    aoe_ui_panel_action_ror,
    aoe_ui_panel_get_true_rect_ror,
    aoe_ui_panel_is_inside_ror,
    aoe_ui_panel_set_focus_ror,
    aoe_ui_panel_set_tab_order_ror,
    aoe_ui_panel_get_help_ror,
    aoe_ui_panel_stop_sound_ror,
    aoe_ui_panel_restart_sound_ror,
    aoe_ui_panel_slot_e0_ror,
};
