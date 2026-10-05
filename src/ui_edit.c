#include "ui_edit.h"

#include <stdlib.h>
#include <string.h>

#define E(p) ((AoeUiEditRor *)(p))

AoeUiEditGlobalsRor aoe_ui_edit_globals_ror;

#define AOE_UI_EDIT_BLACK_BRUSH 4 /* GetStockObject(BLACK_BRUSH) */

void aoe_ui_edit_init_ror(AoeUiEditRor *e, AoeUiSystemRor *system,
                          const AoeUiEditOpsRor *ops) {
    if (!e) return;
    memset(e, 0, sizeof(*e));
    aoe_ui_panel_init_ror(&e->panel, system);
    e->panel.vt = &aoe_ui_edit_vtable_ror;
    e->ops = ops;
    e->panel.handles_mouse = 1;
    e->hidden = 1;
    e->bevel_type = 1;
}

static void hide(AoeUiEditRor *e) {
    if (e->ops && e->ops->show) e->ops->show(e->ops->context, e->window, 0);
    e->hidden = 1;
}

void aoe_ui_edit_release_ror(AoeUiEditRor *e) {
    const AoeUiEditOpsRor *ops;
    if (!e) return;
    ops = e->ops;
    if (e->window && !e->hidden) hide(e);
    free(e->text);
    e->text = NULL;
    if (e->window) {
        if (e->ime) {
            if (ops && ops->ime_release)
                ops->ime_release(ops->context, e->window, e->ime, e->ime_enabled);
            e->ime = 0;
        }
        if (ops && ops->destroy) ops->destroy(ops->context, e->window);
        e->window = NULL;
    }
    aoe_ui_panel_release_ror(&e->panel);
}

static void edit_destroy(AoeUiPanelRor *p) { aoe_ui_edit_release_ror(E(p)); }

int aoe_ui_edit_setup_ror(AoeUiEditRor *e, AoeUiAreaRor *area,
                          AoeUiPanelRor *parent, int32_t x, int32_t y,
                          int32_t w, int32_t h, uintptr_t font, int16_t max,
                          const char *text, int32_t format,
                          int32_t select_on_focus) {
    const AoeUiEditOpsRor *ops;
    if (!e) return 0;
    ops = e->ops;
    e->format = format;
    if (max > 0) ++max;
    e->max = max;
    e->font = font;
    e->select_on_focus = select_on_focus;
    if (!aoe_ui_panel_setup_ror(&e->panel, area, parent, x, y, w, h, 0xff))
        return 0;
    e->window = ops && ops->create
                    ? ops->create(ops->context, area, x + 1, y + 1, w - 2, h - 2,
                                  e->format == 7)
                    : NULL;
    if (!e->window) return 0;
    e->text = calloc((size_t)(e->max + 2 > 0 ? e->max + 2 : 1), 1);
    if (!e->text) return 0;
    e->text[0] = '\0';
    e->length = 0;
    if (text && *text) aoe_ui_edit_set_text_ror(e, text);
    e->brush = AOE_UI_EDIT_BLACK_BRUSH;
    e->color = 0xffffff;
    e->focus_color = 0xffffff;
    e->background = 0;
    e->shadow = 0;
    e->focus_shadow = 0;
    if (e->font && ops->set_font) ops->set_font(ops->context, e->window, e->font);
    e->text_flags = e->format != 7 ? 0x2020 : 0x2010;
    return 1;
}

void aoe_ui_edit_set_text_ror(AoeUiEditRor *e, const char *text) {
    if (!e) return;
    if (e->window && e->text) {
        if (!text) {
            e->text[0] = '\0';
            e->length = 0;
        } else {
            int32_t n = e->max > 0 ? e->max : 0;
            memset(e->text, 0, (size_t)n);
            if (n > 1) {
                strncpy(e->text, text, (size_t)(n - 1));
                e->text[n - 1] = '\0';
            } else {
                e->text[0] = '\0';
            }
            e->length = (int32_t)strlen(e->text);
        }
        if (e->ops && e->ops->set_text)
            e->ops->set_text(e->ops->context, e->window, e->text);
        e->selection = 0;
    }
    e->panel.vt->set_redraw(&e->panel, 1);
}

void aoe_ui_edit_sync_ror(AoeUiEditRor *e) {
    const AoeUiEditOpsRor *ops;
    int32_t start = 0, end = 0;
    if (!e || !e->window || !e->text) return;
    ops = e->ops;
    if (ops->get_text) ops->get_text(ops->context, e->window, e->text, e->max + 1);
    e->length = (int32_t)strlen(e->text);
    if (e->max > 0) e->text[e->max - 1] = '\0';
    if (e->max - 1 < e->length) {
        if (ops->set_text) ops->set_text(ops->context, e->window, e->text);
        if (ops->beep) ops->beep(ops->context);
    }
    if (ops->get_selection) ops->get_selection(ops->context, e->window, &start, &end);
    e->selection = end - start;
}

const char *aoe_ui_edit_text_ror(AoeUiEditRor *e) {
    if (!e) return NULL;
    aoe_ui_edit_sync_ror(e);
    return e->text;
}

void aoe_ui_edit_set_colors_ror(AoeUiEditRor *e, uint32_t color,
                                uint32_t shadow) {
    if (!e) return;
    e->color = color;
    e->shadow = shadow;
    e->panel.vt->set_redraw(&e->panel, 1);
}

void aoe_ui_edit_set_bevel_ror(AoeUiEditRor *e, int32_t type,
                               const uint8_t colors[6]) {
    if (!e) return;
    if (colors) memcpy(e->bevel, colors, sizeof(e->bevel));
    e->bevel_type = type;
    switch (type) {
    case 1: case 2: case 5: e->inset = 1; break;
    case 3: case 6: e->inset = 2; break;
    case 4: case 7: e->inset = 3; break;
    default: e->inset = 0;
    }
    e->panel.vt->set_rect(&e->panel, e->panel.x, e->panel.y, e->panel.width,
                          e->panel.height);
    e->panel.vt->set_redraw(&e->panel, 1);
}

void aoe_ui_edit_set_ime_ror(AoeUiEditRor *e, int32_t enabled, int32_t open) {
    if (!e) return;
    e->ime_enabled = enabled;
    e->ime_open = open;
}

static int beep(AoeUiEditRor *e) {
    if (e->ops && e->ops->beep) e->ops->beep(e->ops->context);
    return 0;
}

static int digit(uint32_t c) { return c > 0x2f && c < 0x3a; }

int aoe_ui_edit_accept_char_ror(AoeUiEditRor *e, uint32_t c) {
    AoeUiEditGlobalsRor *g = &aoe_ui_edit_globals_ror;
    if (!e) return 0;
    if (g->skip_char) {
        g->skip_char = 0;
        return 0;
    }
    if (c == 9) return 0;
    if (c == 0xd) {
        if (e->format != 7) return 0;
    } else if (c == 8) {
        return 1;
    }
    aoe_ui_edit_sync_ror(e);
    if (e->selection == 0) {
        if (e->max - 1 <= e->length) return beep(e);
        if (e->max - 2 <= e->length && e->ops && e->ops->lead_byte &&
            e->ops->lead_byte(e->ops->context, (uint8_t)c)) {
            g->skip_char = 1;
            return beep(e);
        }
    }
    switch (e->format) {
    case 1: case 2: case 3:
        aoe_ui_edit_sync_ror(e);
        if (digit(c)) return 1;
        if (c == '-') {
            if (e->text[0] == '\0') {
                if (e->format != 3) return 1;
                return beep(e);
            }
        } else if (c == '.' && strchr(e->text, '.')) {
            /* The original's test is inverted: '.' is accepted (format 1)
               only once the text already has one. */
            if (e->format == 1) return 1;
            return beep(e);
        }
        return beep(e);
    case 4: case 5:
        aoe_ui_edit_sync_ror(e);
        if (c == '.') {
            if (e->format == 4) {
                if (!strchr(e->text, '.')) return 1;
                return beep(e);
            }
            return beep(e);
        }
        if (c == '\\' || c == ':') return beep(e);
        break;
    case 6:
        aoe_ui_edit_sync_ror(e);
        if (c == ':') {
            if (e->length == 1) return 1;
            return beep(e);
        }
        break;
    case 8:
        if (e->length < 2 && digit(c)) return 1;
        if (e->length == 2 && c == '0') {
            if (strcmp("10", e->text) == 0) return 1;
            return beep(e);
        }
        return beep(e);
    default:
        return 1;
    }
    if (c != '/' && c != '"' && c != '*' && c != '?' && c != '>' && c != '<' &&
        c != '|')
        return 1;
    return beep(e);
}

static int live(const AoeUiEditRor *e) {
    return e->window && e->panel.active && e->panel.visible && !e->hidden;
}

int32_t aoe_ui_edit_window_proc_ror(AoeUiEditRor *e, uint32_t msg,
                                    uint32_t wp, uint32_t lp) {
    const AoeUiEditOpsRor *ops;
    AoeUiPanelRor *parent;
    int32_t s0 = 0, e0 = 0, s1 = 0, e1 = 0, result;
    int track = 0;
    if (!e) return 0;
    ops = e->ops;
    parent = e->panel.parent;
    if (live(e)) {
        if (msg < 0x102) {
            if (msg == 0x101) {
                if (wp == 9) return 0;
            } else if (msg == 0xf) {
                if (ops->invalidate) ops->invalidate(ops->context, e->window, 0);
            } else if (msg == 0x14) {
                if (e->brush) {
                    if (ops->fill_background)
                        ops->fill_background(ops->context, e->window, e->brush);
                    return 1;
                }
            } else if (msg == 0x100) {
                if (!parent) {
                    if (wp == 0x1b) {
                        if (ops->undo) ops->undo(ops->context, e->window);
                        return 0;
                    }
                } else {
                    if (parent->vt->handle_message(parent, e->window, 0x100, wp, lp))
                        return 0;
                    if (wp == 9) {
                        int alt = ops->key_down && ops->key_down(ops->context, 0x12);
                        int ctrl = ops->key_down && ops->key_down(ops->context, 0x11);
                        int shift = ops->key_down && ops->key_down(ops->context, 0x10);
                        aoe_ui_edit_globals_ror.passing_tab = 1;
                        e->panel.vt->key_down(&e->panel, 9, (int32_t)lp, alt, ctrl,
                                              shift);
                        aoe_ui_edit_globals_ror.passing_tab = 0;
                        return 0;
                    }
                    if (wp == 0xd) {
                        parent->vt->action(parent, &e->panel, 0, 0, 0);
                        return 0;
                    }
                    if (wp == 0x1b) {
                        if (!parent->vt->action(parent, &e->panel, 1, 0, 0) && ops->undo)
                            ops->undo(ops->context, e->window);
                        return 0;
                    }
                }
            }
        } else if (msg == 0x102) {
            if (!aoe_ui_edit_accept_char_ror(e, wp)) return 0;
        } else if (msg == 0x104) {
            if (!e->ime || !e->ime_enabled) {
                void *main = ops->main_window
                                 ? ops->main_window(ops->context, e->panel.area)
                                 : NULL;
                if (ops->set_focus) ops->set_focus(ops->context, main);
                if (ops->post_main) ops->post_main(ops->context, main, 0x104, wp, lp);
            }
            return 0;
        } else if (msg == 0x200) {
            if (ops->software_cursor && !ops->software_cursor(ops->context) &&
                ops->hide_pointer)
                ops->hide_pointer(ops->context);
        }
    }
    if (e->window && msg != 0xb0 && ops->get_selection) {
        track = 1;
        ops->get_selection(ops->context, e->window, &s0, &e0);
    }
    result = ops->default_proc ? ops->default_proc(ops->context, e->window, msg, wp, lp)
                               : 0;
    if (track) {
        ops->get_selection(ops->context, e->window, &s1, &e1);
        if (s1 != s0 || e1 != e0) {
            if (ops->invalidate) ops->invalidate(ops->context, e->window, 1);
        }
    }
    if (msg == 0x302) {
        aoe_ui_edit_sync_ror(e);
        return result;
    }
    if (msg == 0x201 || msg == 0x202 || msg == 0x204 || msg == 0x205) {
        AoeUiPanelRor *screen = e->panel.system ? e->panel.system->screen : NULL;
        uint32_t y = (lp >> 16) + (uint32_t)e->panel.y;
        uint32_t x = (uint32_t)e->panel.x + (lp & 0xffff);
        if (screen)
            result = screen->vt->handle_message(
                screen, e->panel.area ? e->panel.area->window : NULL, msg, wp,
                (y << 16) | (x & 0xffff));
    }
    return result;
}

int32_t aoe_ui_edit_class_proc_ror(AoeUiEditRor *owner, uint32_t message,
                                   uint32_t wparam, uint32_t lparam) {
    return owner ? aoe_ui_edit_window_proc_ror(owner, message, wparam, lparam) : 0;
}

/* ---- slots ---- */

static void edit_set_rect(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t w,
                          int32_t h) {
    AoeUiEditRor *e = E(p);
    int32_t in;
    aoe_ui_panel_set_rect_ror(p, x, y, w, h);
    in = e->inset;
    if (e->window && e->ops->move) {
        int repaint = !e->hidden && p->visible && p->active;
        e->ops->move(e->ops->context, e->window, p->x + in, p->y + in,
                     p->width - 2 * in - 1, p->height - 2 * in - 1, repaint);
    }
    if (e->format == 7) {
        e->inner.left = p->rect.left + 1 + in;
        e->inner.top = p->rect.top + in;
        e->inner.right = p->rect.right - in - e->ops->scrollbar_width - 1;
        e->inner.bottom = p->rect.bottom - in;
        return;
    }
    e->inner.left = p->rect.left + 8 + in;
    e->inner.top = p->rect.top + in;
    e->inner.right = p->rect.right - in;
    e->inner.bottom = p->rect.bottom - in;
}

static void edit_set_active(AoeUiPanelRor *p, int32_t active) {
    AoeUiEditRor *e = E(p);
    aoe_ui_panel_set_active_ror(p, active);
    if (e->window && !p->active) hide(e);
}

static void edit_set_redraw(AoeUiPanelRor *p, int32_t level) {
    AoeUiEditRor *e = E(p);
    aoe_ui_panel_set_redraw_ror(p, 1);
    if (p->active && level && e->window && !e->hidden && e->ops->invalidate)
        e->ops->invalidate(e->ops->context, e->window, 0);
}

static int32_t edit_handle_message(AoeUiPanelRor *p, void *window,
                                   uint32_t msg, uint32_t wp, uint32_t lp) {
    AoeUiEditRor *e = E(p);
    if (e->window && p->active && p->visible && !e->hidden && msg != 7) {
        /* Port convention: for these notifications `window` is the
           sending control. */
        if (msg == 0x111) {
            if (window == e->window && wp >> 16 == 0x400 && e->ops->invalidate)
                e->ops->invalidate(e->ops->context, e->window, 0);
        } else if (msg == 0x133 && e->brush && window == e->window) {
            return 1;
        }
    }
    return aoe_ui_panel_handle_message_ror(p, window, msg, wp, lp);
}

static void edit_set_focus(AoeUiPanelRor *p, int32_t focus) {
    AoeUiEditRor *e = E(p);
    const AoeUiEditOpsRor *ops = e->ops;
    AoeUiEditGlobalsRor *g = &aoe_ui_edit_globals_ror;
    int32_t had = p->has_focus;
    aoe_ui_panel_set_focus_ror(p, focus);
    if (e->window) {
        void *current = ops->focus ? ops->focus(ops->context) : NULL;
        void *main = ops->main_window ? ops->main_window(ops->context, p->area) : NULL;
        if (!p->has_focus) {
            if (current == e->window && current != main && ops->set_focus)
                ops->set_focus(ops->context, main);
        } else {
            if (current != e->window && ops->set_focus)
                ops->set_focus(ops->context, e->window);
            if (!e->ime && ops->ime_context)
                e->ime = ops->ime_context(ops->context, e->window, e->ime_enabled);
            if (e->ime && e->ime_enabled && ops->ime_set_open)
                g->ime_status = ops->ime_set_open(ops->context, e->ime, e->ime_open != 0);
        }
    }
    if (had && !p->has_focus) {
        aoe_ui_edit_sync_ror(e);
        p->vt->set_redraw(p, 1);
    }
    if (e->window && p->has_focus && !had && e->select_on_focus &&
        ops->set_selection)
        ops->set_selection(ops->context, e->window, 0, -1);
    if (!g->passing_tab && e->window && !p->has_focus && !e->hidden) hide(e);
    if (p->has_focus) g->skip_char = 0;
}

static int32_t no_key_action(AoeUiPanelRor *p, int32_t a, int32_t b,
                             int32_t c, int32_t d, int32_t f) {
    (void)p; (void)a; (void)b; (void)c; (void)d; (void)f;
    return 0;
}

const AoeUiPanelVtblRor aoe_ui_edit_vtable_ror = {
    edit_destroy,
    aoe_ui_panel_setup_ror,
    aoe_ui_panel_set_rect_ltrb_ror,
    edit_set_rect,
    aoe_ui_panel_set_color_ror,
    edit_set_active,
    aoe_ui_panel_set_position_mode_ror,
    aoe_ui_panel_set_fixed_position_ror,
    edit_set_redraw,
    aoe_ui_panel_redraw_overlaps_ror,
    aoe_ui_panel_begin_draw_ror,
    aoe_ui_panel_end_draw_ror,
    aoe_ui_panel_draw_ror,
    aoe_ui_panel_draw_rect_ror,
    aoe_ui_panel_draw_offset_ror,
    aoe_ui_panel_draw_parent_rect_ror,
    aoe_ui_panel_draw_parent_offset_ror,
    aoe_ui_panel_validate_ror,
    edit_handle_message,
    aoe_ui_panel_handle_idle_ror,
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
    {aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror},
    no_key_action,
    aoe_ui_panel_zero_ror,
    aoe_ui_panel_no_char_action_ror,
    aoe_ui_panel_action_ror,
    aoe_ui_panel_get_true_rect_ror,
    aoe_ui_panel_is_inside_ror,
    edit_set_focus,
    aoe_ui_panel_set_tab_order_ror,
    aoe_ui_panel_get_help_ror,
    aoe_ui_panel_stop_sound_ror,
    aoe_ui_panel_restart_sound_ror,
    aoe_ui_panel_slot_e0_ror,
};
