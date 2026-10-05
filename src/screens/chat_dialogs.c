#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

void aoe_game_ui_chat_ror(AoeGameUiRor *ui, const char *text) {
    const AoeGameUiOpsRor *ops;
    int16_t player;
    if (!ui || !text) return;
    ops = ui->ops;
    player = ui->game ? ui->game->view_player : 0;
    if (ops && ops->chat_command && ops->chat_command(ops->context, player, text))
        return;
    if (!(ui->settings && aoe_settings_flag_97e_ror(ui->settings))) {
        if (ops && ops->play_sound) ops->play_sound(ops->context, 2);
        if (ops && ops->show_chat) ops->show_chat(ops->context, text);
    } else if (!ui->game || ui->game->mode != 5) {
        if (ops && ops->send_chat) ops->send_chat(ops->context, text);
    }
}

#define QCHAT(p) ((AoeQuickChatDialogRor *)(p))
static const char *const QNAME = "Send Quick Message Dialog";

static void free_panel(void *child) {
    AoeUiPanelRor *p = child;
    if (!p) return;
    p->vt->destroy(p);
    free(p);
}

static void qchat_destroy(AoeUiPanelRor *p) {
    AoeQuickChatDialogRor *d = QCHAT(p);
    free_panel(d->edit);
    free_panel(d->label);
    d->edit = NULL;
    d->label = NULL;
    aoe_ui_dialog_release_ror(&d->dialog);
}

int aoe_quick_chat_dialog_init_ror(AoeQuickChatDialogRor *d,
                                   AoeGameUiRor *ui, AoeUiPanelRor *parent,
                                   int32_t comment) {
    AoeUiEzRor *ez, *pez = (AoeUiEzRor *)parent;
    if (!d || !ui) return 0;
    aoe_ui_dialog_init_ror(&d->dialog, ui->system, ui->ez_ops, QNAME);
    d->dialog.ez.panel.vt = &aoe_quick_chat_dialog_vtable_ror.base;
    d->comment = comment;
    d->edit = NULL;
    d->label = NULL;
    d->ui = ui;
    ez = &d->dialog.ez;
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&d->dialog, parent->area, parent, 0x21c, 0x23,
                                 aoe_ui_ez_popup_name_ror(pez),
                                 aoe_ui_ez_popup_id_ror(pez), 1))
        return 0;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->label,
                                      d->comment ? 0x266d : 0x2668, 5, 5, 0x78,
                                      0x19, 4, 1, 1, 0))
        return 0;
    if (!aoe_ui_ez_create_edit_ror(ez, &ez->panel, &d->edit, "", 0x41, 0, 0x82,
                                   5, 0x195, 0x19, 0xb, 1, 0))
        return 0;
    aoe_ui_panel_set_current_ror(&ez->panel, &d->edit->panel);
    return 1;
}

static int32_t qchat_action(AoeUiPanelRor *p, AoeUiPanelRor *from,
                            int32_t code, int32_t value, int32_t extra) {
    AoeQuickChatDialogRor *d = QCHAT(p);
    AoeGameUiRor *ui = d->ui;
    if (from && d->edit && from == &d->edit->panel) {
        if (code == 0) {
            const char *t = aoe_ui_edit_text_ror(d->edit);
            if (t && *t) {
                char line[0x44];
                snprintf(line, sizeof(line), "%s", t);
                aoe_ui_system_show_named_ror(ui->system, "Game Screen", 0);
                aoe_ui_system_close_ror(ui->system, QNAME);
                aoe_game_ui_chat_ror(ui, line);
                return 1;
            }
        } else if (code != 1) {
            return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
        }
        aoe_ui_system_show_named_ror(ui->system, "Game Screen", 0);
        aoe_ui_system_close_ror(ui->system, QNAME);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

static int32_t qchat_mouse_down(AoeUiPanelRor *p, uint8_t button, int32_t x,
                                int32_t y, int32_t c, int32_t s) {
    AoeQuickChatDialogRor *d = QCHAT(p);
    if (!p->vt->is_inside(p, x, y)) {
        p->vt->action(p, d->edit ? &d->edit->panel : NULL, 1, 0, 0);
        return 1;
    }
    return aoe_ui_ez_mouse_down_slot_ror(p, button, x, y, c, s);
}

const AoeUiEzVtblRor aoe_quick_chat_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS_M(qchat_destroy, aoe_ui_panel_handle_idle_ror,
                      aoe_ui_panel_handle_paint_ror,
                      aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror,
                      qchat_action, aoe_ui_ez_set_focus_slot_ror,
                      qchat_mouse_down),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
