#include "ui_dialog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui_ez_slots.h"

#define DLG(p) ((AoeUiDialogRor *)(p))
#define EZ_VT_OF(d) ((const AoeUiEzVtblRor *)(d)->dialog.ez.panel.vt)
#define MSG(p) ((AoeUiMessageDialogRor *)(p))

int aoe_ui_dialog_init_ror(AoeUiDialogRor *d, AoeUiSystemRor *system,
                           const AoeUiEzOpsRor *ops, const char *name) {
    int ok;
    if (!d) return 0;
    ok = aoe_ui_ez_init_named_ror(&d->ez, system, ops, name);
    d->field_480 = d->field_484 = d->field_488 = 0xb;
    d->field_478 = 0;
    d->saved_focus = NULL;
    d->field_48c = 0;
    d->ez.panel.vt = &aoe_ui_dialog_vtable_ror.base;
    return ok;
}

void aoe_ui_dialog_release_ror(AoeUiDialogRor *d) {
    AoeUiPanelRor *parent;
    if (!d) return;
    if (!aoe_ui_system_pop_modal_ror(d->ez.panel.system))
        aoe_ui_system_set_modal_ror(d->ez.panel.system, NULL);
    parent = d->ez.panel.parent;
    if (parent) {
        parent->vt->set_redraw(parent, 2);
        aoe_ui_panel_set_current_ror(parent, d->saved_focus);
    }
    aoe_ui_ez_release_ror(&d->ez);
}

void aoe_ui_dialog_destroy_slot_ror(AoeUiPanelRor *p) {
    aoe_ui_dialog_release_ror(DLG(p));
}

static void centre(AoeUiAreaRor *area, AoeUiPanelRor *parent, int32_t *w,
                   int32_t *h) {
    if (!parent) {
        *w = area ? area->width : 0;
        *h = area ? area->height : 0;
    } else {
        *w = aoe_ui_panel_width_ror(parent);
        *h = aoe_ui_panel_height_ror(parent);
    }
}

int aoe_ui_dialog_setup_ror(AoeUiDialogRor *d, AoeUiAreaRor *area,
                            AoeUiPanelRor *parent, int32_t w, int32_t h,
                            const char *info_name, int32_t info_id,
                            int32_t field_470) {
    int32_t pw, ph;
    if (!d) return 0;
    centre(area, parent, &pw, &ph);
    if (!aoe_ui_ez_setup_ror(&d->ez, area, parent, info_name, info_id, 0,
                             pw / 2 - w / 2, ph / 2 - h / 2, w, h, field_470)) {
        d->ez.panel.field_d8 = 1;
        return 0;
    }
    if (d->ez.panel.parent) {
        d->saved_focus = d->ez.panel.parent->current;
        aoe_ui_panel_set_current_ror(d->ez.panel.parent, &d->ez.panel);
    }
    aoe_ui_panel_set_z_ror(&d->ez.panel, 1, 0);
    aoe_ui_system_set_modal_ror(d->ez.panel.system, &d->ez.panel);
    return 1;
}

int aoe_ui_dialog_setup_color_ror(AoeUiDialogRor *d, AoeUiAreaRor *area,
                                  AoeUiPanelRor *parent, int32_t w,
                                  int32_t h, uint8_t color,
                                  int32_t field_470) {
    int32_t pw, ph;
    if (!d) return 0;
    centre(area, parent, &pw, &ph);
    if (!aoe_ui_ez_setup_ror(&d->ez, area, parent, NULL, -1, 0, pw / 2 - w / 2,
                             ph / 2 - h / 2, w, h, field_470)) {
        d->ez.panel.field_d8 = 1;
        return 0;
    }
    d->ez.panel.vt->set_color(&d->ez.panel, color);
    aoe_ui_panel_set_z_ror(&d->ez.panel, 1, 0);
    aoe_ui_system_set_modal_ror(d->ez.panel.system, &d->ez.panel);
    if (d->ez.panel.parent) {
        d->saved_focus = d->ez.panel.parent->current;
        aoe_ui_panel_set_current_ror(d->ez.panel.parent, &d->ez.panel);
    }
    return 1;
}

static void free_child(void *child);

const AoeUiEzVtblRor aoe_ui_dialog_vtable_ror = {
    AOE_UI_EZ_DEFAULT_SLOTS(aoe_ui_dialog_destroy_slot_ror,
                            aoe_ui_panel_handle_paint_ror,
                            aoe_ui_panel_zero_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

/* ---- the message dialog ---- */

int aoe_ui_message_dialog_init_ror(AoeUiMessageDialogRor *m,
                                   AoeUiSystemRor *system,
                                   const AoeUiEzOpsRor *ops,
                                   const char *name) {
    int ok;
    if (!m) return 0;
    ok = aoe_ui_dialog_init_ror(&m->dialog, system, ops, name);
    m->type = 0;
    m->text = NULL;
    m->buttons[0] = m->buttons[1] = m->buttons[2] = NULL;
    m->dialog.ez.panel.vt = &aoe_ui_message_dialog_vtable_ror.base;
    return ok;
}

static void free_child(void *child) {
    AoeUiPanelRor *p = child;
    if (!p) return;
    p->vt->destroy(p);
    free(p);
}

static void message_destroy(AoeUiPanelRor *p) {
    AoeUiMessageDialogRor *m = MSG(p);
    int i;
    free_child(m->text);
    m->text = NULL;
    for (i = 0; i < 3; ++i) {
        free_child(m->buttons[i]);
        m->buttons[i] = NULL;
    }
    aoe_ui_dialog_release_ror(&m->dialog);
}

int aoe_ui_message_dialog_setup_ror(AoeUiMessageDialogRor *m,
                                    AoeUiPanelRor *parent,
                                    const char *info_name, int32_t info_id,
                                    int32_t w, int32_t h, uint8_t type,
                                    const char *text, int32_t button_w,
                                    int32_t button_h) {
    AoeUiEzRor *ez;
    int32_t count, gap, x, y, i;
    if (!m) return 0;
    ez = &m->dialog.ez;
    m->buttons[0] = m->buttons[1] = m->buttons[2] = NULL;
    if (!aoe_ui_dialog_setup_ror(&m->dialog, parent ? parent->area : NULL,
                                 parent, w, h, info_name, info_id, 1))
        return 0;
    m->type = type;
    if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &m->text, text, 3, 3,
                                   ez->panel.width - 3,
                                   ez->panel.height - button_h - 6, 0xb, 1, 1,
                                   1))
        return 0;
    switch (m->type) {
    case 0: case 5: count = 1; break;
    case 1: case 2: case 3: count = 2; break;
    case 4: count = 3; break;
    default: return 0;
    }
    gap = ez->panel.width - count * button_w;
    gap = gap < 1 ? 0 : gap / (count + 1);
    y = ez->panel.height - button_h - 7;
    x = gap;
    for (i = 0; i < count; ++i) {
        if (!aoe_ui_ez_create_button_ror(ez, &ez->panel, &m->buttons[i], "", "",
                                         x, y, button_w, button_h, 0xb, 0, 0))
            return 0;
        x += gap + button_w;
    }
    {
        AoeUiSystemRor *s = ez->panel.system;
        AoeUiButtonRor *escape = NULL;
        switch (m->type) {
        case 0: case 5:
            aoe_ui_button_set_text_id_ror(m->buttons[0], 0, 0xfa1);
            break;
        case 1:
            aoe_ui_button_set_text_id_ror(m->buttons[0], 0, 0xfa1);
            aoe_ui_button_set_text_id_ror(m->buttons[1], 0, 0xfa2);
            escape = m->buttons[1];
            break;
        case 2:
            aoe_ui_button_set_text_id_ror(m->buttons[0], 0, 0xfa3);
            aoe_ui_button_set_text_id_ror(m->buttons[1], 0, 0xfa4);
            escape = m->buttons[1];
            break;
        case 3:
            aoe_ui_button_set_text_id_ror(m->buttons[0], 0, 0xfa5);
            aoe_ui_button_set_text_id_ror(m->buttons[1], 0, 0xfa7);
            escape = m->buttons[1];
            break;
        case 4:
            aoe_ui_button_set_text_id_ror(m->buttons[0], 0, 0xfa3);
            aoe_ui_button_set_text_id_ror(m->buttons[1], 0, 0xfa4);
            aoe_ui_button_set_text_id_ror(m->buttons[2], 0, 0xfa2);
            escape = m->buttons[2];
            break;
        }
        (void)s;
        if (escape) {
            escape->hotkey = 0x1b;
            escape->hotkey_shift = 0;
        }
    }
    {
        AoeUiPanelRor *panels[3];
        for (i = 0; i < count; ++i) panels[i] = &m->buttons[i]->panel;
        aoe_ui_panels_tab_cycle_ror(panels, (int16_t)count);
    }
    aoe_ui_panel_set_current_ror(&ez->panel, &m->buttons[0]->panel);
    return 1;
}

static int32_t message_idle(AoeUiPanelRor *p) {
    const AoeUiEzOpsRor *ops = MSG(p)->dialog.ez.ops;
    if (ops && ops->game_idle &&
        !(ops->idle_blocked && ops->idle_blocked(ops->context)))
        ops->game_idle(ops->context);
    return aoe_ui_panel_handle_idle_ror(p);
}

static int32_t message_action(AoeUiPanelRor *p, AoeUiPanelRor *from,
                              int32_t code, int32_t value, int32_t extra) {
    AoeUiMessageDialogRor *m = MSG(p);
    AoeUiPanelRor *parent = p->parent;
    if (parent && code == 1) {
        if (m->buttons[0] && from == &m->buttons[0]->panel) {
            const AoeUiEzOpsRor *ops = m->dialog.ez.ops;
            if (m->type == 5 && ops) {
                if (ops->help_mode) *ops->help_mode = 1;
                if (ops->win_help) ops->win_help(ops->context);
            }
            parent->vt->action(parent, p, 0, 0, 0);
            return 1;
        }
        if (m->buttons[1] && from == &m->buttons[1]->panel) {
            parent->vt->action(parent, p, 1, 0, 0);
            return 1;
        }
        if (m->buttons[2] && from == &m->buttons[2]->panel) {
            parent->vt->action(parent, p, 2, 0, 0);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_ui_message_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS(message_destroy, message_idle,
                    aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror,
                    message_action, aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

/* ---- the list dialog ---- */

#define LST(p) ((AoeUiListDialogRor *)(p))

int aoe_ui_list_dialog_init_ror(AoeUiListDialogRor *d, AoeUiSystemRor *system,
                                const AoeUiEzOpsRor *ops, const char *name) {
    int ok;
    if (!d) return 0;
    ok = aoe_ui_dialog_init_ror(&d->dialog, system, ops, name);
    d->selected = -1;
    d->title = NULL;
    d->list = NULL;
    d->scrollbar = NULL;
    d->ok = d->cancel = NULL;
    d->selected_text = NULL;
    d->dialog.ez.panel.vt = &aoe_ui_list_dialog_vtable_ror.base;
    return ok;
}

static void list_destroy(AoeUiPanelRor *p) {
    AoeUiListDialogRor *d = LST(p);
    free_child(d->title);
    free_child(d->list);
    free_child(d->scrollbar);
    free_child(d->ok);
    free_child(d->cancel);
    d->title = NULL;
    d->list = NULL;
    d->scrollbar = NULL;
    d->ok = d->cancel = NULL;
    aoe_ui_dialog_release_ror(&d->dialog);
}

int aoe_ui_list_dialog_setup_ror(
    AoeUiListDialogRor *d, AoeUiPanelRor *parent, const char *info_name,
    int32_t info_id, int32_t w, int32_t h, int32_t title_x, int32_t title_y,
    int32_t title_w, int32_t title_h, const char *title, int32_t list_x,
    int32_t list_y, int32_t list_w, int32_t list_h, const char *const *lines,
    int16_t count, int16_t selected, int32_t ok_x, int32_t cancel_x,
    int32_t button_y, int32_t button_w, int32_t button_h) {
    AoeUiEzRor *ez;
    if (!d) return 0;
    ez = &d->dialog.ez;
    if (!aoe_ui_dialog_setup_ror(&d->dialog, parent ? parent->area : NULL,
                                 parent, w, h, info_name, info_id, 1))
        return 0;
    if (title && *title &&
        !aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->title, title, title_x,
                                   title_y, title_w, title_h, 4, 1, 1, 1))
        return 0;
    if (!aoe_ui_ez_create_list_ror(ez, &ez->panel, &d->list, list_x, list_y,
                                   list_w, list_h, 0xb))
        return 0;
    if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &d->scrollbar, &d->list->text,
                                             0x14))
        return 0;
    if (lines)
        ((const AoeUiTextVtblRor *)d->list->text.panel.vt)
            ->set_lines(&d->list->text, lines, count);
    d->selected = selected;
    aoe_ui_list_move_ror(d->list, 1, selected, 1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->ok, 0xfa1, 0, ok_x,
                                        button_y, button_w, button_h, 0, 0, 0))
        return 0;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->cancel, 0xfa2, 0,
                                        cancel_x, button_y, button_w, button_h,
                                        0, 0, 0))
        return 0;
    d->cancel->hotkey = 0x1b;
    d->cancel->hotkey_shift = 0;
    aoe_ui_panel_set_current_ror(&ez->panel, &d->list->text.panel);
    return 1;
}

static int32_t list_action(AoeUiPanelRor *p, AoeUiPanelRor *from,
                           int32_t code, int32_t value, int32_t extra) {
    AoeUiListDialogRor *d = LST(p);
    if (from) {
        int chosen = 0;
        if (d->ok && from == &d->ok->panel) {
            if (code == 1) chosen = 1;
        } else if (d->cancel && from == &d->cancel->panel) {
            if (code == 1) {
                if (p->parent) p->parent->vt->action(p->parent, p, 2, 0, 0);
                return 1;
            }
        } else if (d->list && from == &d->list->text.panel && code == 3) {
            chosen = 1;
        }
        if (chosen) {
            d->selected = aoe_ui_text_selected_ror(&d->list->text);
            d->selected_text = aoe_ui_text_selected_text_ror(&d->list->text);
            if (p->parent) p->parent->vt->action(p->parent, p, 1, 0, 0);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

int16_t aoe_ui_list_dialog_selected_ror(const AoeUiListDialogRor *d) {
    return d ? d->selected : -1;
}

const char *aoe_ui_list_dialog_text_ror(const AoeUiListDialogRor *d) {
    return d ? d->selected_text : NULL;
}

const AoeUiEzVtblRor aoe_ui_list_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS(list_destroy, aoe_ui_panel_handle_idle_ror,
                    aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                    aoe_ui_panel_zero_ror, list_action,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

/* ---- the help dialog ---- */

#define HLP(p) ((AoeUiHelpDialogRor *)(p))

static int32_t iabs(int32_t v) { return v < 0 ? -v : v; }

static void help_layout(AoeUiHelpDialogRor *d, AoeUiPanelRor *parent,
                        int32_t at_x, int32_t at_y, int32_t lines,
                        int32_t font_h) {
    AoeUiPanelRor *p = &d->dialog.ez.panel;
    int32_t h = (font_h + 1) * lines + 0x37, x, y, above;
    int32_t px = parent->x, py = aoe_ui_panel_y_ror(parent);
    int32_t pw = aoe_ui_panel_width_ror(parent);
    int32_t ph = aoe_ui_panel_height_ror(parent);
    if (pw + px >= at_x + 0x163)
        x = iabs(px - 5 - at_x);
    else if (px + 10 > at_x - 0x159)
        x = pw / 2 - 0xaa;
    else
        x = iabs(at_x - px - 0x159);
    if (x < 0) x = 0;
    above = at_y - h;
    if (py <= above - 10)
        y = iabs(above - py - 10);
    else if (ph + py >= h + at_y + 10)
        y = iabs(at_y - py + 10);
    else
        y = ph / 2 - h / 2;
    if (y < 0) y = 0;
    p->vt->set_fixed_position(p, x, y, 0x154, h);
    EZ_VT_OF(d)->set_reference(&d->dialog.ez, p->max_w, p->max_h);
}

static void help_destroy(AoeUiPanelRor *p) {
    AoeUiHelpDialogRor *d = HLP(p);
    free_child(d->ok);
    free_child(d->more);
    free_child(d->text);
    d->ok = d->more = NULL;
    d->text = NULL;
    aoe_ui_dialog_release_ror(&d->dialog);
}

int aoe_ui_help_dialog_init_ror(AoeUiHelpDialogRor *d, AoeUiSystemRor *system,
                                const AoeUiEzOpsRor *ops,
                                AoeUiPanelRor *parent, const char *owner,
                                const char *text, int32_t page, int32_t x,
                                int32_t y) {
    AoeUiEzRor *ez, *pez = (AoeUiEzRor *)parent;
    uintptr_t font = 0;
    int32_t fw = 0, fh = 0, lines, row;
    AoeUiPanelRor *tab[2];
    size_t n;
    if (!d) return 0;
    aoe_ui_dialog_init_ror(&d->dialog, system, ops, "Help Dialog");
    d->dialog.ez.panel.vt = &aoe_ui_help_dialog_vtable_ror.base;
    ez = &d->dialog.ez;
    d->ok = d->more = NULL;
    d->field_594 = NULL;
    d->text = NULL;
    n = owner ? strlen(owner) : 0;
    if (n >= sizeof(d->owner)) n = sizeof(d->owner) - 1;
    if (n) memcpy(d->owner, owner, n);
    d->owner[n] = '\0';
    d->page = page;
    if (ops && ops->font) ops->font(ops->context, 0xb, &font, &fw, &fh);
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&d->dialog, parent->area, parent, 0x154, 400,
                                 aoe_ui_ez_popup_name_ror(pez),
                                 aoe_ui_ez_popup_id_ror(pez), 1))
        return 0;
    aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->text, text, 10, 10, 0x14a,
                              0x186, 0xb, 0, 0, 1);
    if (!d->text) return 0;
    lines = d->text->bottom + 2;
    help_layout(d, parent, x, y, lines, fh);
    d->text->panel.vt->set_fixed_position(&d->text->panel, 10, 10, 0x14a,
                                          (fh + 1) * lines);
    row = fh * lines + 0x14;
    if (ops && ops->single_player && ops->single_player(ops->context) &&
        page >= 0) {
        const char *s = aoe_ui_string_static_ror(system, page);
        if (s && *s)
            aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->more, 0x243d, 0,
                                           0x19, row, 0x78, 0x19, 0xb, 0, 0);
    }
    aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->ok, 0xfa1, 0,
                                   d->more ? 0xc3 : 0x6e, row, 0x78, 0x19, 0xb,
                                   0, 0);
    if (!d->ok) return 0;
    d->ok->hotkey = 0x1b;
    d->ok->hotkey_shift = 0;
    aoe_ui_panel_set_current_ror(&ez->panel, &d->ok->panel);
    tab[0] = &d->ok->panel;
    tab[1] = d->more ? &d->more->panel : NULL;
    aoe_ui_panels_tab_cycle_ror(tab, (int16_t)(d->more ? 2 : 1));
    return 1;
}

static int32_t help_action(AoeUiPanelRor *p, AoeUiPanelRor *from,
                           int32_t code, int32_t value, int32_t extra) {
    AoeUiHelpDialogRor *d = HLP(p);
    if (code == 1 && from) {
        AoeUiSystemRor *s = p->system;
        int32_t page = d->page;
        const AoeUiEzOpsRor *ops = d->dialog.ez.ops;
        if (d->ok && from == &d->ok->panel) {
            aoe_ui_system_show_named_ror(s, d->owner, 0);
            aoe_ui_system_close_ror(s, "Help Dialog");
            return 1;
        }
        if (d->more && from == &d->more->panel) {
            char keyword[0x200];
            const char *k = aoe_ui_string_static_ror(s, page);
            snprintf(keyword, sizeof(keyword), "%s", k ? k : "");
            aoe_ui_system_show_named_ror(s, d->owner, 0);
            aoe_ui_system_close_ror(s, "Help Dialog");
            if (ops && ops->win_help_key) ops->win_help_key(ops->context, keyword);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

static int32_t help_mouse_down(AoeUiPanelRor *p, uint8_t button, int32_t x,
                               int32_t y, int32_t c, int32_t s) {
    AoeUiHelpDialogRor *d = HLP(p);
    if (!p->vt->is_inside(p, x, y)) {
        p->vt->action(p, d->ok ? &d->ok->panel : NULL, 1, 0, 0);
        return 1;
    }
    return aoe_ui_ez_mouse_down_slot_ror(p, button, x, y, c, s);
}

const AoeUiEzVtblRor aoe_ui_help_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS_M(help_destroy, aoe_ui_panel_handle_idle_ror,
                      aoe_ui_panel_handle_paint_ror,
                      aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror,
                      help_action, aoe_ui_ez_set_focus_slot_ror,
                      help_mouse_down),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
