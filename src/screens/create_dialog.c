#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define CD(p) ((AoeCreateDialogRor *)(p))

static const char *const NAME = "Create Dialog";
static const char *const JOIN = "Join Screen";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

/* 0x0047c120 (deleting 0x0047c100). */
static void cd_destroy(AoeUiPanelRor *p) {
    AoeCreateDialogRor *d = CD(p);
    DROP(d->title);
    DROP(d->label);
    DROP(d->edit);
    DROP(d->ok);
    DROP(d->cancel);
    aoe_ui_dialog_release_ror(&d->dialog);
}

/* 0x0047beb0 */
int aoe_create_dialog_init_ror(AoeCreateDialogRor *d, AoeGameUiRor *ui,
                               AoeUiPanelRor *parent) {
    const AoeGameUiOpsRor *ops;
    AoeUiEzRor *ez, *pez = (AoeUiEzRor *)parent;
    AoeUiPanelRor *tab[3];
    char name[0x100];
    const char *saved;
    if (!d || !ui) return 0;
    ops = ui->ops;
    aoe_ui_dialog_init_ror(&d->dialog, ui->system, ui->ez_ops, NAME);
    d->dialog.ez.panel.vt = &aoe_create_dialog_vtable_ror.base;
    d->title = d->label = NULL;
    d->edit = NULL;
    d->ok = d->cancel = NULL;
    d->ui = ui;
    ez = &d->dialog.ez;
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&d->dialog, parent->area, parent, 500, 0xe6,
                                 aoe_ui_ez_popup_name_ror(pez),
                                 aoe_ui_ez_popup_id_ror(pez), 1))
        return 0;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->title, 0x25a6, 0x14, 0x14, 0x1cc,
                                      0x1e, 1, 1, 1, 0) ||
        !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->label, 0x25aa, 0x28, 0x55, 0x1a4,
                                      0x14, 4, 0, 1, 0))
        return 0;
    saved = ops && ops->mp_game_name ? ops->mp_game_name(ops->context) : NULL;
    snprintf(name, sizeof name, "%s", saved ? saved : "");
    if (!aoe_ui_ez_create_edit_ror(ez, &ez->panel, &d->edit, name, 0x28, 0, 0x28, 0x6e, 0x1a4,
                                   0x17, 0xb, 1, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->ok, 0xfa1, 0, 0x3c, 0xb4, 0xb4, 0x1e,
                                        0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->cancel, 0xfa2, 0, 0x104, 0xb4, 0xb4,
                                        0x1e, 0, 0, 0))
        return 0;
    d->cancel->hotkey = 0x1b;
    d->cancel->hotkey_shift = 0;
    tab[0] = &d->edit->panel;
    tab[1] = &d->ok->panel;
    tab[2] = &d->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 3);
    aoe_ui_panel_set_current_ror(&ez->panel, &d->edit->panel);
    return 1;
}

/* 0x0047c1b0 */
static int32_t cd_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeCreateDialogRor *d = CD(p);
    const AoeGameUiOpsRor *ops = d->ui->ops;
    AoeUiSystemRor *sys = d->ui->system;
    AoeUiPanelRor *edit = d->edit ? &d->edit->panel : NULL;
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if ((from == &d->ok->panel && code == 1) || (from == edit && code == 0)) {
        char name[0x100];
        AoeUiPanelRor *join;
        if (aoe_ui_edit_blank_ror(d->edit)) {
            aoe_ui_panel_set_current_ror(p, edit);
            return 1;
        }
        if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
        snprintf(name, sizeof name, "%s", aoe_ui_edit_text_ror(d->edit));
        if (ops && ops->set_mp_game_name) ops->set_mp_game_name(ops->context, name);
        aoe_ui_system_show_named_ror(sys, JOIN, 0);
        if (ops && ops->create_mp_game && ops->create_mp_game(ops->context, name)) {
            if (ops->open_mp_setup) ops->open_mp_setup(ops->context);
            aoe_ui_system_show_named_ror(sys, "MP Setup Screen", 0);
            aoe_ui_system_close_ror(sys, "Status Screen");
            aoe_ui_system_close_ror(sys, NAME);
            aoe_ui_system_close_ror(sys, JOIN);
            return 1;
        }
        aoe_ui_system_close_ror(sys, NAME);
        aoe_ui_system_show_named_ror(sys, JOIN, 0);
        aoe_ui_system_close_ror(sys, "Status Screen");
        /* The dialog is gone: the message goes over the join screen. */
        join = aoe_ui_system_find_ror(sys, JOIN);
        if (join) aoe_ui_ez_ok_dialog_id_ror((AoeUiEzRor *)join, 0x25ad, NULL, 0x1c2, 100);
        return 1;
    }
    if ((from == &d->cancel->panel && code == 1) || (from == edit && code == 1)) {
        aoe_ui_system_show_named_ror(sys, JOIN, 0);
        aoe_ui_system_close_ror(sys, NAME);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_create_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS(cd_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, cd_action,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
