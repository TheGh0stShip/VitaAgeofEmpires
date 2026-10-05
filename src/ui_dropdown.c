#include "ui_dropdown.h"

#include <stdlib.h>
#include <string.h>

#define D(p) ((AoeUiDropdownRor *)(p))
#define TEXT_VT(t) ((const AoeUiTextVtblRor *)(t)->panel.vt)

static AoeUiPanelRor *as_panel(void *child) { return (AoeUiPanelRor *)child; }

static void free_child(void *child) {
    AoeUiPanelRor *p = as_panel(child);
    if (!p) return;
    p->vt->destroy(p);
    free(p);
}

void aoe_ui_dropdown_init_ror(AoeUiDropdownRor *d, AoeUiSystemRor *system,
                              const AoeUiDropdownOpsRor *ops) {
    if (!d) return;
    aoe_ui_panel_init_ror(&d->panel, system);
    d->panel.vt = &aoe_ui_dropdown_vtable_ror;
    d->ops = ops;
    d->text = NULL;
    d->button = NULL;
    d->list = NULL;
    d->scrollbar = NULL;
    d->state = 0;
    d->selected = 0;
    d->text_w = d->text_h = d->button_w = d->button_h = 0;
    d->list_w = d->list_h = d->scroll_w = 0;
    d->scroll_field_11c = d->scroll_thumb = 0;
    d->opened_up = 0;
    d->line_height = 0;
    d->border_color = 0;
    d->button_left = 0;
    d->panel.field_ee = 6;
    d->border = 1;
    d->list_scroll.context = NULL;
    d->list_scroll.set_range = NULL;
}

void aoe_ui_dropdown_release_ror(AoeUiDropdownRor *d) {
    if (!d) return;
    free_child(d->button);
    d->button = NULL;
    free_child(d->list);
    d->list = NULL;
    free_child(d->scrollbar);
    d->scrollbar = NULL;
    free_child(d->text);
    d->text = NULL;
    aoe_ui_panel_release_ror(&d->panel);
}

static void dropdown_destroy(AoeUiPanelRor *p) { aoe_ui_dropdown_release_ror(D(p)); }

/* ---- the opening button ---- */

static AoeUiButtonVtblRor button_vtable;
static int button_vtable_ready;

static void button_destroy(AoeUiPanelRor *p) {
    aoe_ui_button_release_ror((AoeUiButtonRor *)p);
}

static int32_t button_mouse_move(AoeUiPanelRor *p, int32_t x, int32_t y,
                                 int32_t c, int32_t s) {
    AoeUiDropdownRor *owner = ((AoeUiDropdownButtonRor *)p)->owner;
    if (p->captured && owner && owner->list) {
        AoeUiPanelRor *list = &owner->list->text.panel;
        if (list->vt->is_inside(list, x, y)) {
            p->vt->mouse_up(p, 1, x, y, c, s);
            list->vt->mouse_down(list, 1, x, y, c, s);
            return 0;
        }
        if (owner->scrollbar &&
            owner->scrollbar->panel.vt->is_inside(&owner->scrollbar->panel, x, y)) {
            p->vt->mouse_up(p, 1, x, y, c, s);
            list->vt->mouse_down(list, 1, list->width + list->x - 1, y, c, s);
            return 0;
        }
    }
    return aoe_ui_panel_mouse_move_ror(p, x, y, c, s);
}

static AoeUiDropdownButtonRor *new_button(AoeUiDropdownRor *owner) {
    AoeUiDropdownButtonRor *b = calloc(1, sizeof(*b));
    if (!b) return NULL;
    /* The subclass replaces two slots of the button vtable. */
    if (!button_vtable_ready) {
        button_vtable = aoe_ui_button_vtable_ror;
        button_vtable.base.destroy = button_destroy;
        button_vtable.base.mouse_move = button_mouse_move;
        button_vtable_ready = 1;
    }
    aoe_ui_button_init_ror(&b->button, owner->panel.system, NULL);
    b->button.panel.vt = &button_vtable.base;
    b->owner = owner;
    return b;
}

/* ---- setup ---- */

static void list_set_range(void *context, int32_t max, int32_t position) {
    aoe_ui_scrollbar_set_range_ror((AoeUiScrollbarRor *)context, max, position);
}

int aoe_ui_dropdown_setup_ror(
    AoeUiDropdownRor *d, AoeUiAreaRor *area, AoeUiPanelRor *parent,
    uintptr_t font, int32_t font_width, int32_t line_height, int32_t fill,
    uint8_t fill_color, int32_t border, uint32_t border_color, int32_t x,
    int32_t y, int32_t w, int32_t h, int16_t max_chars, int16_t selected,
    int32_t button_w, int32_t button_h, int32_t list_w, int32_t list_h,
    const char *const *lines, int16_t count, int32_t scroll_w,
    int32_t scroll_field_11c, int32_t scroll_thumb) {
    AoeUiSystemRor *system;
    if (!d) return 0;
    system = d->panel.system;
    d->selected = selected;
    d->button_w = button_w;
    d->button_h = button_h;
    d->list_w = list_w;
    d->list_h = list_h;
    d->scroll_w = scroll_w;
    d->scroll_field_11c = scroll_field_11c;
    d->scroll_thumb = scroll_thumb;
    d->text_w = w - button_w - 1;
    d->text_h = h;
    d->border_color = (int32_t)(border_color & 0xff);
    d->line_height = line_height;
    aoe_ui_panel_setup_ror(&d->panel, area, parent, x, y, w, h, 0);

    d->text = calloc(1, sizeof(*d->text));
    if (!d->text) return 0;
    aoe_ui_text_init_ror(d->text, system);
    if (!aoe_ui_text_setup_ror(d->text, area, &d->panel, 0, 0, d->text_w,
                               d->text_h, font, font_width, line_height, NULL,
                               0, 0, fill, fill_color, border,
                               (uint8_t)border_color, max_chars,
                               lines ? lines[selected] : NULL))
        return 0;
    aoe_ui_text_set_alignment_ror(d->text, 0, 1);

    d->button = new_button(d);
    if (!d->button) return 0;
    if (!aoe_ui_button_setup_ror(&d->button->button, area, &d->panel, 0, 0,
                                 d->button_w, d->button_h, 6, NULL, 1, 0))
        return 0;

    d->list = calloc(1, sizeof(*d->list));
    if (!d->list) return 0;
    aoe_ui_list_init_ror(d->list, system);
    if (!aoe_ui_text_setup_lines_ror(&d->list->text, area, &d->panel, 0, 0,
                                     d->list_w, d->list_h, font, font_width,
                                     line_height, NULL, 0, 0, fill,
                                     fill_color, border, (uint8_t)border_color,
                                     max_chars, lines, count))
        return 0;
    d->list->hover_select = 1;
    d->list->text.panel.vt->set_active(&d->list->text.panel, 0);
    aoe_ui_text_select_ror(&d->list->text, d->selected);

    d->scrollbar = calloc(1, sizeof(*d->scrollbar));
    if (!d->scrollbar) return 0;
    aoe_ui_scrollbar_init_ror(d->scrollbar, system);
    if (!aoe_ui_scrollbar_setup_ror(d->scrollbar, d->panel.area, &d->panel, 0,
                                    0, d->scroll_w, d->list_h,
                                    d->scroll_field_11c, d->scroll_thumb,
                                    &d->list->text.panel, count, 0))
        return 0;
    d->list_scroll.context = d->scrollbar;
    d->list_scroll.set_range = list_set_range;
    d->list->text.scroll = &d->list_scroll;
    aoe_ui_text_set_scrollbar_ror(&d->list->text, &d->scrollbar->panel, 0);
    d->scrollbar->panel.vt->set_active(&d->scrollbar->panel, 0);
    aoe_ui_dropdown_set_state_ror(d, 1);
    return 1;
}

/* ---- open and close ---- */

static void place_closed_row(AoeUiDropdownRor *d, int32_t mode) {
    AoeUiPanelRor *text = &d->text->panel, *button = &d->button->button.panel;
    if (d->button_left == 1) {
        button->vt->set_position_mode(button, mode, 0, 0, 1, 0, d->button_w,
                                      d->button_w, d->button_h, d->button_h,
                                      NULL, NULL, NULL, NULL);
        text->vt->set_position_mode(text, mode, 1, 0, 0, 0, d->text_w,
                                    d->text_w, d->text_h, d->text_h, button,
                                    NULL, NULL, NULL);
    } else {
        text->vt->set_position_mode(text, mode, 0, 0, 1, 0, d->text_w,
                                    d->text_w, d->text_h, d->text_h, NULL,
                                    NULL, NULL, NULL);
        button->vt->set_position_mode(button, mode, 1, 0, 0, 0, d->button_w,
                                      d->button_w, d->button_h, d->button_h,
                                      text, NULL, NULL, NULL);
    }
}

/* The list height for `rows` rows. */
static int32_t list_height(const AoeUiDropdownRor *d, int32_t rows) {
    const AoeUiTextRor *t = &d->list->text;
    if (t->spacing == 0)
        return rows * (d->line_height + 2) + t->margin * 2;
    return rows * (d->line_height + 1 + t->spacing * 2) + t->margin * 2;
}

static void show_scrollbar_if_needed(AoeUiDropdownRor *d) {
    if (aoe_ui_text_visible_ror(&d->list->text) <
        aoe_ui_text_count_ror(&d->list->text))
        d->scrollbar->panel.vt->set_active(&d->scrollbar->panel, 1);
}

/* The list shrinks by `by` rows' worth of pixels, then snaps to whole
   visible rows. */
static void shrink_list(AoeUiDropdownRor *d, int32_t by) {
    AoeUiPanelRor *list = &d->list->text.panel;
    d->list_h -= by;
    list->vt->set_fixed_position(list, 0, 0, d->list_w, d->list_h);
    d->list_h = list_height(d, aoe_ui_text_visible_ror(&d->list->text));
}

void aoe_ui_dropdown_set_state_ror(AoeUiDropdownRor *d, int32_t state) {
    AoeUiPanelRor *p, *parent, *list, *scroll;
    int32_t px, py, pw, ph, old;
    if (!d || state == d->state) return;
    /* Port guard: the original assumes setup created every part. */
    if (!d->text || !d->button || !d->list || !d->scrollbar) return;
    p = &d->panel;
    parent = p->parent;
    list = &d->list->text.panel;
    scroll = &d->scrollbar->panel;
    if (!parent) {
        px = py = 0;
        pw = p->area ? p->area->width : 0;
        ph = p->area ? p->area->height : 0;
    } else {
        px = parent->x;
        py = aoe_ui_panel_y_ror(parent);
        pw = aoe_ui_panel_width_ror(parent);
        ph = aoe_ui_panel_height_ror(parent);
    }
    old = d->state;
    d->state = 0;
    if (state == 1) {
        if (old == 2 && parent && parent->parent &&
            (p->clip.left < px || p->clip.top < py ||
             p->clip.right > pw - 1 + px || p->clip.bottom > ph - 1 + py))
            parent->parent->vt->set_redraw(parent->parent, 2);
        list->vt->set_active(list, 0);
        scroll->vt->set_active(scroll, 0);
        place_closed_row(d, 7);
        p->clip_to_parent = 1;
        if (d->opened_up) {
            p->vt->set_fixed_position(p, p->x - px,
                                      p->height + p->y - d->text_h - py,
                                      d->text_w + d->button_w + 1, d->text_h);
            d->opened_up = 0;
        } else {
            p->vt->set_fixed_position(p, p->x - px, p->y - py,
                                      d->text_w + d->button_w + 1, d->text_h);
        }
        if (parent) parent->vt->set_redraw(parent, 1);
        else p->vt->set_redraw(p, 1);
        aoe_ui_panel_set_current_ror(p, &d->text->panel);
        if (old == 2) {
            if (!(d->ops && d->ops->pop_modal && d->ops->pop_modal(d->ops->context)) &&
                d->ops && d->ops->set_modal)
                d->ops->set_modal(d->ops->context, NULL);
            if (parent) parent->vt->action(parent, p, 2, 0, 0);
        }
    } else if (state == 2) {
        int32_t total, bottom, area_h = p->area ? p->area->height : 0;
        list->vt->set_active(list, 1);
        d->list_h = list_height(d, aoe_ui_text_count_ror(&d->list->text));
        list->vt->set_fixed_position(list, 0, 0, d->list_w, d->list_h);
        scroll->vt->set_active(scroll,
                               aoe_ui_text_visible_ror(&d->list->text) <
                                   aoe_ui_text_count_ror(&d->list->text));
        place_closed_row(d, 7);
        scroll->vt->set_position_mode(scroll, 10, 1, 1, 0, 0, d->scroll_w,
                                      d->scroll_w, 0, 0, NULL,
                                      &d->text->panel, NULL, NULL);
        list->vt->set_position_mode(list, 8, 0, 1, 1, 0, 0, 0, 0, 0, NULL,
                                    &d->text->panel, scroll, NULL);
        total = d->list_h + 1 + d->text_h;
        p->clip_to_parent = 0;
        p->vt->set_fixed_position(p, p->x - px, p->y - py, d->list_w, total);
        bottom = aoe_ui_panel_height_ror(list) + aoe_ui_panel_y_ror(list);
        if (bottom > area_h - 4) {
            int32_t over = bottom + 4 - area_h;
            int32_t above = aoe_ui_panel_height_ror(list) + 4 - p->y;
            if (above < over) {
                /* Open upward, shrunk by what does not fit above. */
                if (above > 0) {
                    shrink_list(d, above);
                    total = d->list_h + d->text_h + 1;
                    if (!scroll->active) {
                        list->vt->set_fixed_position(list, 0, 0, d->list_w,
                                                     d->list_h);
                        show_scrollbar_if_needed(d);
                    }
                }
                place_closed_row(d, 8);
                scroll->vt->set_position_mode(scroll, 9, 1, 1, 0, 0,
                                              d->scroll_w, d->scroll_w, 0, 0,
                                              NULL, NULL, NULL,
                                              &d->text->panel);
                list->vt->set_position_mode(list, 7, 0, 1, 1, 0, 0, 0, 0, 0,
                                            NULL, NULL, scroll,
                                            &d->text->panel);
                p->vt->set_fixed_position(p, p->x - px,
                                          d->text_h - total - py + p->y,
                                          d->list_w, total);
                d->opened_up = 1;
            } else {
                shrink_list(d, over);
                total = d->list_h + d->text_h + 1;
                if (!scroll->active) {
                    list->vt->set_fixed_position(list, 0, 0, d->list_w,
                                                 d->list_h);
                    show_scrollbar_if_needed(d);
                    list->vt->set_position_mode(list, 8, 0, 1, 1, 0, 0, 0, 0,
                                                0, NULL, &d->text->panel,
                                                scroll, NULL);
                }
                p->vt->set_fixed_position(p, p->x - px, p->y - py, d->list_w,
                                          total);
            }
        }
        p->vt->set_redraw(p, 1);
        aoe_ui_panel_set_z_ror(p, 1, 0);
        aoe_ui_panel_set_current_ror(p, list);
        if (d->ops && d->ops->set_modal) d->ops->set_modal(d->ops->context, p);
        if (parent) parent->vt->action(parent, p, 1, 0, 0);
    }
    d->state = state;
}

/* ---- slots ---- */

static void dropdown_set_rect(AoeUiPanelRor *p, int32_t x, int32_t y,
                              int32_t w, int32_t h) {
    AoeUiDropdownRor *d = D(p);
    aoe_ui_panel_set_rect_ror(p, x, y, w, h);
    if (d->state == 1) {
        d->text_w = p->width - d->button_w - 1;
        d->text_h = p->height;
        aoe_ui_dropdown_set_state_ror(d, 0);
        aoe_ui_dropdown_set_state_ror(d, 1);
    } else if (d->state == 2) {
        d->list_w = p->width;
        d->list_h = p->height - d->text_h - 1;
        aoe_ui_dropdown_set_state_ror(d, 1);
        aoe_ui_dropdown_set_state_ror(d, 2);
    }
}

static int32_t dropdown_mouse_down(AoeUiPanelRor *p, uint8_t button,
                                   int32_t x, int32_t y, int32_t c,
                                   int32_t s) {
    AoeUiDropdownRor *d = D(p);
    if (p->tab_stop && p->parent) aoe_ui_panel_set_current_ror(p->parent, p);
    if (d->state == 2 && d->list) {
        AoeUiPanelRor *list = &d->list->text.panel;
        int32_t in_list = list->vt->is_inside(list, x, y);
        int32_t in_scroll =
            d->scrollbar ? d->scrollbar->panel.vt->is_inside(&d->scrollbar->panel, x, y) : 0;
        if (!in_list && !in_scroll) {
            aoe_ui_dropdown_set_state_ror(d, 1);
            aoe_ui_dropdown_select_ror(d, d->selected);
            return 1;
        }
    }
    return aoe_ui_panel_mouse_down_ror(p, button, x, y, c, s);
}

static int32_t dropdown_mouse_move(AoeUiPanelRor *p, int32_t x, int32_t y,
                                   int32_t c, int32_t s) {
    return aoe_ui_panel_mouse_move_ror(p, x, y, c, s);
}

static int32_t dropdown_mouse_up(AoeUiPanelRor *p, uint8_t button, int32_t x,
                                 int32_t y, int32_t c, int32_t s) {
    return aoe_ui_panel_mouse_up_ror(p, button, x, y, c, s);
}

static int32_t dropdown_key_down(AoeUiPanelRor *p, int32_t key, int32_t count,
                                 int32_t alt, int32_t ctrl, int32_t shift) {
    AoeUiDropdownRor *d = D(p);
    if (p->has_focus && (key == 0x28 || key == 0x26) && d->state == 1) {
        aoe_ui_dropdown_set_state_ror(d, 2);
        return 1;
    }
    if (p->has_focus && key == 0x1b && d->state == 2) {
        aoe_ui_dropdown_set_state_ror(d, 1);
        return 1;
    }
    return aoe_ui_panel_key_down_ror(p, key, count, alt, ctrl, shift);
}

static int32_t dropdown_action(AoeUiPanelRor *p, AoeUiPanelRor *from,
                               int32_t code, int32_t value, int32_t extra) {
    AoeUiDropdownRor *d = D(p);
    if (d->button && from == &d->button->button.panel) {
        if (code == 2 && d->state == 1) aoe_ui_dropdown_set_state_ror(d, 2);
    } else if (d->list && from == &d->list->text.panel &&
               (code == 3 || code == 2)) {
        d->selected = (int16_t)value;
        TEXT_VT(d->text)->set_text(
            d->text, aoe_ui_text_line_ror(&d->list->text, (int16_t)value));
        aoe_ui_dropdown_set_state_ror(d, 1);
        if (p->parent) p->parent->vt->action(p->parent, p, 0, 0, 0);
    }
    return aoe_ui_panel_action_ror(p, from, code, value, extra);
}

static int32_t no_key_action(AoeUiPanelRor *p, int32_t a, int32_t b,
                             int32_t c, int32_t e, int32_t f) {
    (void)p; (void)a; (void)b; (void)c; (void)e; (void)f;
    return 0;
}

static void dropdown_set_tab_order(AoeUiPanelRor *p, AoeUiPanelRor *prev,
                                   AoeUiPanelRor *next) {
    AoeUiDropdownRor *d = D(p);
    aoe_ui_panel_set_tab_order_ror(p, prev, next);
    if (d->text) d->text->panel.vt->set_tab_order(&d->text->panel, NULL, NULL);
}

/* ---- settings ---- */

void aoe_ui_dropdown_set_colors_ror(AoeUiDropdownRor *d, uint32_t color,
                                    uint32_t shadow) {
    if (d && d->text) aoe_ui_text_set_colors_ror(d->text, color, shadow);
}

void aoe_ui_dropdown_set_button_left_ror(AoeUiDropdownRor *d, int32_t left) {
    int32_t state;
    if (!d) return;
    d->button_left = left;
    if (d->text) {
        state = d->state;
        d->state = 0;
        aoe_ui_dropdown_set_state_ror(d, state);
    }
}

void aoe_ui_dropdown_set_border_ror(AoeUiDropdownRor *d, int32_t border) {
    if (!d) return;
    d->border = border;
    if (d->text) {
        aoe_ui_text_set_border_ror(d->text, border, (uint8_t)d->border_color);
        d->panel.vt->set_redraw(&d->panel, 1);
    }
}

static int32_t clamp_bevel(int32_t type) { return type >= 1 && type <= 4 ? type : 0; }

void aoe_ui_dropdown_set_bevel_ror(AoeUiDropdownRor *d, int32_t type,
                                   const uint8_t colors[6]) {
    static const int32_t text_types[5] = {0, 1, 5, 6, 7};
    if (!d) return;
    if (d->text)
        TEXT_VT(d->text)->set_bevel(d->text, text_types[clamp_bevel(type)], colors);
    if (d->list)
        TEXT_VT(&d->list->text)->set_bevel(&d->list->text, clamp_bevel(type), colors);
    if (d->scrollbar)
        aoe_ui_scrollbar_set_bevel_ror(d->scrollbar, clamp_bevel(type), colors);
    if (d->button)
        aoe_ui_button_set_bevel_ror(&d->button->button, clamp_bevel(type), colors);
}

void aoe_ui_dropdown_set_pictures_ror(AoeUiDropdownRor *d, void *picture,
                                      int16_t frame, void *scroll1,
                                      void *scroll2, void *scroll3,
                                      void *scroll4) {
    if (!d) return;
    if (d->button) {
        if (picture)
            aoe_ui_button_set_picture_ror(&d->button->button, 0, picture, frame);
        else
            aoe_ui_button_set_picture_ror(&d->button->button, 0, NULL, -1);
    }
    if (d->scrollbar) {
        void *const pictures[5] = {picture, scroll1, scroll2, scroll3, scroll4};
        aoe_ui_scrollbar_set_pictures_ror(d->scrollbar, pictures);
    }
}

/* ---- list access ---- */

int16_t aoe_ui_dropdown_selected_ror(const AoeUiDropdownRor *d) {
    return d && d->list ? aoe_ui_text_selected_ror(&d->list->text) : 0;
}

static void show_line(AoeUiDropdownRor *d, int32_t index) {
    if (d->text)
        TEXT_VT(d->text)->set_text(d->text,
                                   aoe_ui_text_line_ror(&d->list->text, index));
}

void aoe_ui_dropdown_select_ror(AoeUiDropdownRor *d, int16_t index) {
    if (!d) return;
    d->selected = index;
    if (!d->list) return;
    aoe_ui_text_select_ror(&d->list->text, index);
    show_line(d, index);
}

void aoe_ui_dropdown_select_id_ror(AoeUiDropdownRor *d, int32_t id) {
    if (!d || !d->list) return;
    aoe_ui_text_select_id_ror(&d->list->text, id);
    d->selected = (int16_t)aoe_ui_text_selection_ror(&d->list->text);
    show_line(d, d->selected);
}

const char *aoe_ui_dropdown_selected_text_ror(const AoeUiDropdownRor *d) {
    return d && d->list ? aoe_ui_text_selected_text_ror(&d->list->text) : NULL;
}

void aoe_ui_dropdown_set_sorted_ror(AoeUiDropdownRor *d, int32_t sorted) {
    if (d && d->list) d->list->text.sorted = sorted;
}

int aoe_ui_dropdown_append_ror(AoeUiDropdownRor *d, const char *text,
                               int32_t id) {
    return d && d->list ? aoe_ui_text_append_ror(&d->list->text, text, id) : 0;
}

int aoe_ui_dropdown_append_id_ror(AoeUiDropdownRor *d, int32_t string_id,
                                  int32_t id) {
    return d && d->list ? aoe_ui_text_append_id_ror(&d->list->text, string_id, id)
                        : 0;
}

int aoe_ui_dropdown_insert_ror(AoeUiDropdownRor *d, int32_t index,
                               const char *text, int32_t id) {
    return d && d->list
               ? aoe_ui_text_insert_ror(&d->list->text, index, text, NULL, id)
               : 0;
}

int aoe_ui_dropdown_remove_ror(AoeUiDropdownRor *d, int32_t index) {
    return d && d->list ? aoe_ui_text_remove_ror(&d->list->text, index) : 0;
}

int32_t aoe_ui_dropdown_selection_ror(const AoeUiDropdownRor *d) {
    return d && d->list ? aoe_ui_text_selection_ror(&d->list->text) : 0;
}

int32_t aoe_ui_dropdown_index_of_id_ror(const AoeUiDropdownRor *d,
                                        int32_t id) {
    return d && d->list ? aoe_ui_text_index_of_id_ror(&d->list->text, id) : 0;
}

int32_t aoe_ui_dropdown_index_of_ror(const AoeUiDropdownRor *d,
                                     const char *text) {
    return d && d->list ? aoe_ui_text_index_of_ror(&d->list->text, text) : 0;
}

int32_t aoe_ui_dropdown_selected_id_ror(const AoeUiDropdownRor *d) {
    return d && d->list ? aoe_ui_text_selected_id_ror(&d->list->text) : 0;
}

int32_t aoe_ui_dropdown_id_ror(const AoeUiDropdownRor *d, int32_t index) {
    return d && d->list ? aoe_ui_text_id_ror(&d->list->text, index) : 0;
}

void aoe_ui_dropdown_clear_ror(AoeUiDropdownRor *d) {
    if (d && d->list) aoe_ui_text_clear_ror(&d->list->text);
}

const AoeUiPanelVtblRor aoe_ui_dropdown_vtable_ror = {
    dropdown_destroy,
    aoe_ui_panel_setup_ror,
    aoe_ui_panel_set_rect_ltrb_ror,
    dropdown_set_rect,
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
    aoe_ui_panel_handle_idle_ror,
    aoe_ui_panel_handle_size_ror,
    aoe_ui_panel_handle_paint_ror,
    dropdown_key_down,
    aoe_ui_panel_char_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    dropdown_mouse_down,
    dropdown_mouse_move,
    dropdown_mouse_up,
    aoe_ui_panel_mouse_dbl_ror,
    {aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror},
    no_key_action,
    aoe_ui_panel_zero_ror,
    aoe_ui_panel_no_char_action_ror,
    dropdown_action,
    aoe_ui_panel_get_true_rect_ror,
    aoe_ui_panel_is_inside_ror,
    aoe_ui_panel_set_focus_ror,
    dropdown_set_tab_order,
    aoe_ui_panel_get_help_ror,
    aoe_ui_panel_stop_sound_ror,
    aoe_ui_panel_restart_sound_ror,
    aoe_ui_panel_slot_e0_ror,
};
