#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define NS(p) ((AoeNameSelectionRor *)(p))
#define NN(p) ((AoeNewNameDialogRor *)(p))

static const char *const NAME = "Name Selection Screen";
static const char *const NEW_NAME = "New Name Dialog";
static const char *const YES_NO = "YesNoDialog";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void ns_destroy(AoeUiPanelRor *p) {
    AoeNameSelectionRor *n = NS(p);
    aoe_ui_system_show_named_ror(n->ui->system, NEW_NAME, 0);
    DROP(n->title);
    DROP(n->label);
    DROP(n->list);
    DROP(n->list_bar);
    DROP(n->add);
    DROP(n->del);
    DROP(n->ok);
    DROP(n->cancel);
    DROP(n->close);
    aoe_ui_ez_release_ror(&n->ez);
}

void aoe_name_selection_fill_ror(AoeNameSelectionRor *n) {
    const AoeGameUiOpsRor *ops = n->ui->ops;
    AoeUiTextRor *t = &n->list->text;
    char **names = NULL;
    int32_t chosen = 0, count, i;
    aoe_ui_text_clear_ror(t);
    if (!ops || !ops->campaign_player_names) return;
    count = ops->campaign_player_names(ops->context, &names, &chosen);
    if (!count) return;
    t->sorted = 1;
    for (i = 0; i < count; ++i) aoe_ui_text_append_ror(t, names[i], i);
    aoe_ui_text_select_ror(t, (int16_t)aoe_ui_text_index_of_id_ror(t, chosen));
    for (i = 0; i < count; ++i) free(names[i]);
    free(names);
}

void aoe_name_selection_update_ror(AoeNameSelectionRor *n) {
    AoeUiPanelRor *tab[5];
    int empty = aoe_ui_text_count_ror(&n->list->text) == 0;
    int k = 2;
    aoe_ui_button_set_disabled_ror(n->del, empty);
    aoe_ui_button_set_disabled_ror(n->ok, empty);
    tab[0] = &n->list->text.panel;
    tab[1] = &n->add->panel;
    if (!empty) {
        tab[2] = &n->del->panel;
        tab[3] = &n->ok->panel;
        k = 4;
    }
    tab[k] = &n->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, (int16_t)(k + 1));
    aoe_ui_panel_set_current_ror(&n->ez.panel, empty ? &n->add->panel : &n->list->text.panel);
}

int aoe_name_selection_init_ror(AoeNameSelectionRor *n, AoeGameUiRor *ui, AoeUiAreaRor *area) {
    AoeUiEzRor *ez = &n->ez;
    memset(n, 0, sizeof *n);
    n->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_name_selection_vtable_ror.base;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr2", 0xc384, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &n->title, 0x29fe, 0x14, 0x14, 600, 0x1e, 1,
                                      1, 0, 0) ||
        !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &n->label, 0x29ff, 0x3c, 0x50, 0x8c, 0x14, 4,
                                      0, 0, 0) ||
        !aoe_ui_ez_create_list_ror(ez, &ez->panel, &n->list, 0x3c, 100, 0x14a, 300, 0xb) ||
        !aoe_ui_ez_create_text_scrollbar_ror(ez, &n->list_bar, &n->list->text, 0x14))
        return 1;
    aoe_ui_panel_set_help_ror(&n->list_bar->panel, 0x765c, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &n->add, 0x2a00, 0, 400, 100, 0xb4, 0x1e,
                                        0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&n->add->panel, 0x765d, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &n->del, 0x2a01, 0, 400, 0x8c, 0xb4, 0x1e,
                                        0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&n->del->panel, 0x765e, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &n->ok, 0xfa1, 0, 0x46, 0x1b8, 0xf0, 0x1e,
                                        0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&n->ok->panel, 0x7531, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &n->cancel, 0xfa2, 0, 0x14a, 0x1b8, 0xf0,
                                        0x1e, 0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&n->cancel->panel, 0x7532, -1);
    n->cancel->hotkey = 0x1b;
    n->cancel->hotkey_shift = 0;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &n->close, 0x3ea, 0, 0, 0, 0, 0, -1, -1,
                                        0))
        return 1;
    n->close->panel.vt->set_position_mode(&n->close->panel, 9, 4, 4, 4, 4, 0x11, 0x11, 0x11, 0x11,
                                          NULL, NULL, NULL, NULL);
    aoe_name_selection_update_ror(n);
    aoe_name_selection_fill_ror(n);
    n->field_49c = 0;
    return 1;
}

static void go(AoeNameSelectionRor *n, void (*open)(void *), const char *screen) {
    const AoeGameUiOpsRor *ops = n->ui->ops;
    if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
    if (ops && open) open(ops->context);
    aoe_ui_system_show_named_ror(n->ui->system, screen, 0);
    aoe_ui_system_close_ror(n->ui->system, NAME);
}

static int32_t ns_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeNameSelectionRor *n = NS(p);
    const AoeGameUiOpsRor *ops = n->ui->ops;
    AoeUiSystemRor *sys = n->ui->system;
    AoeUiTextRor *t = &n->list->text;
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (from == &n->ok->panel && code == 1) {
        if (aoe_ui_text_selection_ror(t) == -1) return 1;
        if (ops && ops->choose_campaign_player)
            ops->choose_campaign_player(ops->context, aoe_ui_text_selected_id_ror(t));
        go(n, ops ? ops->open_campaign_selection : NULL, "Campaign Selection Screen");
        return 1;
    }
    if (from == &n->cancel->panel && code == 1) {
        go(n, ops ? ops->open_single_player_menu : NULL, "Single Player Menu");
        return 1;
    }
    if (from == &n->close->panel) {
        if (ops && ops->quit_program) ops->quit_program(ops->context);
        return 1;
    }
    if (from == &t->panel && code == 3) return p->vt->action(p, &n->ok->panel, 1, 0, 0);
    if (from == &n->add->panel && code == 1) {
        AoeNewNameDialogRor *d = calloc(1, sizeof *d);
        /* The dialog registers itself; one that cannot be set up is
           closed through the system again. */
        if (d && !aoe_new_name_dialog_init_ror(d, n->ui, p)) {
            if (aoe_ui_system_find_ror(sys, NEW_NAME)) aoe_ui_system_close_ror(sys, NEW_NAME);
            else free(d);
            return 1;
        }
        aoe_ui_system_show_named_ror(sys, NEW_NAME, 0);
        return 1;
    }
    if (from == &n->del->panel && code == 1) {
        char fmt[0x100], line[0x200];
        if (aoe_ui_text_selection_ror(t) == -1) return 1;
        aoe_ui_string_ror(sys, 0x2a02, fmt, sizeof fmt);
        /* The string is used as the format for the name. */
        snprintf(line, sizeof line, fmt, aoe_ui_text_selected_text_ror(t));
        aoe_ui_ez_yes_no_dialog_ror(&n->ez, line, NULL, 0x1c2, 100);
        return 1;
    }
    if (from->name && strcmp(from->name, NEW_NAME) == 0) {
        if (code == 1) aoe_name_selection_fill_ror(n);
        aoe_ui_system_show_named_ror(sys, NAME, 0);
        aoe_ui_system_close_ror(sys, NEW_NAME);
        aoe_name_selection_update_ror(n);
        return 1;
    }
    if (from->name && strcmp(from->name, YES_NO) == 0) {
        int32_t line = aoe_ui_text_selection_ror(t);
        if (code == 0 && line != -1) {
            if (ops && ops->remove_campaign_player)
                ops->remove_campaign_player(ops->context, aoe_ui_text_id_ror(t, line));
            aoe_name_selection_fill_ror(n);
        }
        aoe_ui_system_close_ror(sys, YES_NO);
        aoe_name_selection_update_ror(n);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_name_selection_vtable_ror = {
    AOE_UI_EZ_SLOTS(ns_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, ns_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

static void nn_destroy(AoeUiPanelRor *p) {
    AoeNewNameDialogRor *d = NN(p);
    DROP(d->title);
    DROP(d->label);
    DROP(d->name);
    DROP(d->ok);
    DROP(d->cancel);
    aoe_ui_dialog_release_ror(&d->dialog);
}

int aoe_new_name_dialog_init_ror(AoeNewNameDialogRor *d, AoeGameUiRor *ui, AoeUiPanelRor *parent) {
    AoeUiEzRor *ez = &d->dialog.ez, *pez = (AoeUiEzRor *)parent;
    AoeUiPanelRor *tab[3];
    memset(d, 0, sizeof *d);
    d->ui = ui;
    aoe_ui_dialog_init_ror(&d->dialog, ui->system, ui->ez_ops, NEW_NAME);
    ez->panel.vt = &aoe_new_name_dialog_vtable_ror.base;
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&d->dialog, parent->area, parent, 500, 0xe6,
                                 aoe_ui_ez_popup_name_ror(pez), aoe_ui_ez_popup_id_ror(pez), 1))
        return 0;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->title, 0x2a03, 0x14, 0x14, 0x1cc, 0x1e, 1,
                                      1, 1, 0) ||
        !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->label, 0x2a04, 0x28, 0x55, 0x1a4, 0x14, 4,
                                      0, 1, 0) ||
        !aoe_ui_ez_create_edit_ror(ez, &ez->panel, &d->name, "", 0xf, 0, 0x28, 0x6e, 0x1a4, 0x17,
                                   0xb, 1, 1) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->ok, 0xfa1, 0, 0x3c, 0xb4, 0xb4, 0x1e, 0,
                                        0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->cancel, 0xfa2, 0, 0x104, 0xb4, 0xb4,
                                        0x1e, 0, 0, 0))
        return 1;
    d->cancel->hotkey = 0x1b;
    d->cancel->hotkey_shift = 0;
    tab[0] = &d->name->panel;
    tab[1] = &d->ok->panel;
    tab[2] = &d->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 3);
    aoe_ui_panel_set_current_ror(&ez->panel, &d->name->panel);
    return 1;
}

static int32_t nn_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeNewNameDialogRor *d = NN(p);
    const AoeGameUiOpsRor *ops = d->ui->ops;
    AoeUiSystemRor *sys = d->ui->system;
    if (from && d->name && d->ok && d->cancel) {
        if ((from == &d->ok->panel && code == 1) || (from == &d->name->panel && code == 0)) {
            const char *name = aoe_ui_edit_text_ror(d->name);
            if (!name || !*name) {
                aoe_ui_panel_set_current_ror(p, &d->name->panel);
                return 1;
            }
            if (ops && ops->add_campaign_player) ops->add_campaign_player(ops->context, name);
            if (ops && ops->open_campaign_selection) ops->open_campaign_selection(ops->context);
            aoe_ui_system_show_named_ror(sys, "Campaign Selection Screen", 0);
            aoe_ui_system_close_ror(sys, NEW_NAME);
            aoe_ui_system_close_ror(sys, NAME);
            return 1;
        }
        if ((from == &d->cancel->panel && code == 1) || (from == &d->name->panel && code == 1)) {
            const char *back = NAME;
            if (!aoe_ui_system_find_ror(sys, NAME)) {
                if (ops && ops->open_single_player_menu) ops->open_single_player_menu(ops->context);
                back = "Single Player Menu";
            }
            aoe_ui_system_show_named_ror(sys, back, 0);
            aoe_ui_system_close_ror(sys, NEW_NAME);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_new_name_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS(nn_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, nn_action,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
