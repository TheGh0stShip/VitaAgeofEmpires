#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define SMSG(p) ((AoeSendMessageDialogRor *)(p))
static const char *const SNAME = "Send Message Dialog";

static void free_panel(void *child) {
    AoeUiPanelRor *p = child;
    if (!p) return;
    p->vt->destroy(p);
    free(p);
}

/* 0x00435650: every child released (deleting form 0x00435630). The flag
   picture ("dlg_plbn.shp" 50746) belongs to the host's draw callback. */
static void smsg_destroy(AoeUiPanelRor *p) {
    AoeSendMessageDialogRor *d = SMSG(p);
    int i;
    free_panel(d->title);
    d->title = NULL;
    for (i = 0; i < 8; i++) {
        free_panel(d->names[i]);
        free_panel(d->civs[i]);
        free_panel(d->checks[i]);
        d->names[i] = d->civs[i] = NULL;
        d->checks[i] = NULL;
    }
    for (i = 0; i < 4; i++) {
        free_panel(d->radio_labels[i]);
        free_panel(d->radios[i]);
        d->radio_labels[i] = NULL;
        d->radios[i] = NULL;
    }
    free_panel(d->edit);
    free_panel(d->edit_label);
    free_panel(d->send);
    d->edit = NULL;
    d->edit_label = NULL;
    d->send = NULL;
    aoe_ui_dialog_release_ror(&d->dialog);
}

static int slot_status_human(const AoeGameUiOpsRor *ops, int32_t slot) {
    int32_t s = ops->comm_slot_status ? ops->comm_slot_status(ops->context, slot) : 0;
    return s == 2 || s == 4;
}

/* The font of a player row: 8 (dim) for a computer player, or in a
   networked game when no comm slot of that player is a connected human;
   11 otherwise. */
static int32_t row_font(const AoeGameUiOpsRor *ops, int32_t player) {
    int32_t i, n, found = 0;
    if (ops->player_is_computer && ops->player_is_computer(ops->context, player))
        return 8;
    if (!(ops->networked && ops->networked(ops->context))) return 0xb;
    n = ops->comm_slots ? ops->comm_slots(ops->context) : 0;
    for (i = 1; i <= n; i++) {
        if (ops->slot_player(ops->context, i) == player && slot_status_human(ops, i))
            found = 1;
    }
    return found ? 0xb : 8;
}

/* 0x00434fe0: 520 x 465 over the parent. A title (9831); a row per
   used comm slot 1..8 at y 40 + 30 n: the slot's name (x 50, 190 wide)
   and the civilization (x 260, 120 wide, font 11) and, for another
   slot held by a connected human, a recipient check box at x 10 (help
   30335) set from the slot's recipient flag; "Message" (9832) and a
   65-character edit at y 315; four radio buttons (help 30330..30334)
   with labels 9833, 9834, 9835, 9838 at y 350 / 385, the last one on;
   Send (9836, help 30001, Escape) at (100, 425) 300 x 30. The edit
   gets the focus. */
int aoe_send_message_dialog_init_ror(AoeSendMessageDialogRor *d, AoeGameUiRor *ui,
                                     AoeUiPanelRor *parent) {
    static const int32_t rx[4] = {0x2d, 0x113, 0x2d, 0x113};
    static const int32_t ry[4] = {0x15e, 0x15e, 0x181, 0x181};
    static const int32_t rl[4] = {0x2669, 0x266a, 0x266b, 0x266e};
    static const int32_t rh[4] = {0x767b, 0x767c, 0x767d, 0x767e};
    const AoeGameUiOpsRor *ops;
    AoeUiEzRor *ez, *pez = (AoeUiEzRor *)parent;
    int32_t slot, row = 0, y = 0x28, own;
    int i;
    if (!d || !ui || !ui->ops || !ui->ops->slot_player) return 0;
    ops = ui->ops;
    memset(d, 0, sizeof(*d));
    aoe_ui_dialog_init_ror(&d->dialog, ui->system, ui->ez_ops, SNAME);
    d->dialog.ez.panel.vt = &aoe_send_message_dialog_vtable_ror.base;
    d->ui = ui;
    d->has_flags = 1;
    ez = &d->dialog.ez;
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&d->dialog, parent->area, parent, 0x208, 0x1d1,
                                 aoe_ui_ez_popup_name_ror(pez),
                                 aoe_ui_ez_popup_id_ror(pez), 1))
        return 0;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->title, 0x2667, 5, 10,
                                      ez->panel.width - 0x28, 0x1e, 1, 1, 1, 0))
        return 0;
    own = ops->comm_own_slot ? ops->comm_own_slot(ops->context) : 0;
    for (slot = 1; slot < 9; slot++) {
        int32_t player = ops->slot_player(ops->context, slot);
        char civ[100];
        const char *name;
        if (!player) continue;
        name = ops->comm_slot_name ? ops->comm_slot_name(ops->context, slot) : NULL;
        if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->names[row], name ? name : "",
                                       0x32, y, 0xbe, 0x1e, row_font(ops, player), 0, 1, 0))
            return 0;
        civ[0] = 0;
        if (ops->player_civ_name) ops->player_civ_name(ops->context, player, civ, sizeof(civ));
        if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &d->civs[row], civ, 0x104, y, 0x78,
                                       0x1e, 0xb, 0, 1, 0))
            return 0;
        if (slot != own && slot_status_human(ops, slot)) {
            if (!aoe_ui_ez_create_check_ror(ez, &ez->panel, &d->checks[row], 10, y, 0x1e,
                                            0x1e, 0, 0))
                return 0;
            aoe_ui_panel_set_help_ror(&d->checks[row]->panel, 0x767f, -1);
            aoe_ui_button_set_state_ror(
                d->checks[row],
                ops->recipient && ops->recipient(ops->context, slot) ? 1 : 0);
        }
        d->slots[row] = slot;
        row++;
        y += 0x1e;
    }
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->edit_label, 0x2668, 10, 0x13b,
                                      0x78, 0x1e, 4, 1, 1, 0))
        return 0;
    if (!aoe_ui_ez_create_edit_ror(ez, &ez->panel, &d->edit, "", 0x41, 0, 0x82, 0x13b,
                                   0x17c, 0x17, 0xb, 1, 0))
        return 0;
    aoe_ui_panel_set_help_ror(&d->edit->panel, 0x767a, -1);
    for (i = 0; i < 4; i++)
        if (!aoe_ui_ez_create_radio_ror(ez, &ez->panel, &d->radios[i], rx[i], ry[i], 0x1e,
                                        0x1e, 0, 0))
            return 0;
    for (i = 0; i < 4; i++) {
        if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &d->radio_labels[i], rl[i],
                                          rx[i] + 0x1e, ry[i], 200, 0x1e, 0xb, 0, 1, 0))
            return 0;
        aoe_ui_panel_set_help_ror(&d->radio_labels[i]->panel, -1, -1);
    }
    for (i = 0; i < 4; i++) aoe_ui_button_set_group_ror(d->radios[i], d->radios, 4);
    aoe_ui_button_set_state_ror(d->radios[3], 1);
    for (i = 0; i < 4; i++) aoe_ui_panel_set_help_ror(&d->radios[i]->panel, rh[i], -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &d->send, 0x266c, 0, 100, 0x1a9,
                                        300, 0x1e, 0, 0, 0))
        return 0;
    aoe_ui_panel_set_help_ror(&d->send->panel, 0x7531, -1);
    d->send->hotkey = 0x1b;
    d->send->hotkey_shift = 0;
    aoe_ui_panel_set_current_ror(&ez->panel, &d->edit->panel);
    return 1;
}

/* 0x00435bc0: each row with a check box sets its slot's recipient flag
   from the box. */
void aoe_send_message_dialog_apply_ror(AoeSendMessageDialogRor *d) {
    const AoeGameUiOpsRor *ops = d->ui->ops;
    int i;
    for (i = 0; i < 8; i++)
        if (d->slots[i] && d->checks[i] && ops->set_recipient)
            ops->set_recipient(ops->context, d->slots[i],
                               aoe_ui_button_state_ror(d->checks[i]) != 0);
}

static void copy_text(AoeSendMessageDialogRor *d, char *line, size_t n) {
    const char *t = d->edit ? aoe_ui_edit_text_ror(d->edit) : NULL;
    snprintf(line, n, "%s", t ? t : "");
}

/* A radio pick: the boxes of the other players set by a relation (the
   viewed player's own row and rows without a box are left alone). */
static void pick(AoeSendMessageDialogRor *d, int which) {
    const AoeGameUiOpsRor *ops = d->ui->ops;
    int16_t me = d->ui->game ? d->ui->game->view_player : 0;
    int i;
    for (i = 0; i < 8; i++) {
        int32_t player;
        int on;
        if (!d->checks[i]) continue;
        player = ops->slot_player(ops->context, d->slots[i]);
        if (which == 2) {
            on = 1;
        } else if (which == 0) {
            on = player == me ||
                 (ops->player_relation_a && ops->player_relation_a(ops->context, me, player));
        } else {
            on = player != me && ops->player_relation_b &&
                 ops->player_relation_b(ops->context, me, player);
        }
        aoe_ui_button_set_state_ror(d->checks[i], on ? 1 : 0);
    }
}

/* +0xb8 0x00435750: Enter in the edit sends a non-empty line through
   the shared chat path and clears the edit; Send (a click) applies the
   boxes, shows the game screen, closes the dialog and sends the line;
   a check box picks the last radio; the first two radios set the boxes
   by the viewed player's relations (game player vtable +0x1c, +0x18),
   the third (a click) sets every box. Each of these then applies the
   boxes. */
static int32_t smsg_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                           int32_t value, int32_t extra) {
    AoeSendMessageDialogRor *d = SMSG(p);
    AoeGameUiRor *ui = d->ui;
    char line[0x44];
    int i;
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (d->edit && from == &d->edit->panel) {
        if (code == 0) {
            copy_text(d, line, sizeof(line));
            if (line[0]) {
                aoe_game_ui_chat_ror(ui, line);
                aoe_ui_edit_set_text_ror(d->edit, "");
            }
        }
        return 1;
    }
    if (d->send && from == &d->send->panel) {
        if (code == 1) {
            aoe_send_message_dialog_apply_ror(d);
            copy_text(d, line, sizeof(line));
            aoe_ui_system_show_named_ror(ui->system, "Game Screen", 0);
            aoe_ui_system_close_ror(ui->system, SNAME);
            if (line[0]) aoe_game_ui_chat_ror(ui, line);
            return 1;
        }
        aoe_send_message_dialog_apply_ror(d);
        return 1;
    }
    for (i = 0; i < 8; i++) {
        if (d->checks[i] && from == &d->checks[i]->panel) {
            aoe_ui_button_set_state_ror(d->radios[0], 0);
            aoe_ui_button_set_state_ror(d->radios[1], 0);
            aoe_ui_button_set_state_ror(d->radios[2], 0);
            aoe_ui_button_set_state_ror(d->radios[3], 1);
            aoe_send_message_dialog_apply_ror(d);
            return 1;
        }
    }
    if (from == &d->radios[0]->panel) pick(d, 0);
    else if (from == &d->radios[1]->panel) pick(d, 1);
    else if (from == &d->radios[2]->panel && code == 1) pick(d, 2);
    aoe_send_message_dialog_apply_ror(d);
    return 1;
}

/* +0x30 0x00435c10: the Ez draw, then each used slot's flag in its
   player's colour at (50, 40 + 30 n) from the dialog's corner. */
static void smsg_draw(AoeUiPanelRor *p) {
    AoeSendMessageDialogRor *d = SMSG(p);
    const AoeGameUiOpsRor *ops = d->ui->ops;
    int i;
    aoe_ui_ez_draw_slot_ror(p);
    if (!d->has_flags || !ops->draw_player_flag) return;
    for (i = 0; i < 8 && d->slots[i]; i++)
        ops->draw_player_flag(ops->context, p,
                              ops->slot_player(ops->context, d->slots[i]),
                              p->x + 0x32, p->y + 0x28 + 0x1e * i);
}

const AoeUiEzVtblRor aoe_send_message_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS_MD(smsg_destroy, smsg_draw, aoe_ui_panel_handle_idle_ror,
                       aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                       aoe_ui_panel_zero_ror, smsg_action, aoe_ui_ez_set_focus_slot_ror,
                       aoe_ui_ez_mouse_down_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
