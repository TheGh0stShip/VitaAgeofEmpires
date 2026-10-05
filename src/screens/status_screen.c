#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

static void status_destroy(AoeUiPanelRor *p) {
    AoeStatusScreenRor *st = (AoeStatusScreenRor *)p;
    if (st->text) {
        st->text->panel.vt->destroy(&st->text->panel);
        free(st->text);
        st->text = NULL;
    }
    aoe_ui_ez_release_ror(&st->ez);
}

int aoe_status_screen_init_ror(AoeStatusScreenRor *st, AoeGameUiRor *ui, AoeUiAreaRor *area,
                               const char *name, const char *text, int32_t text_id,
                               const char *info_name, int32_t info_id) {
    AoeUiEzRor *ez = &st->ez;
    AoeUiPanelRor *current = ui->system->screen;
    const AoeGameUiOpsRor *ops = ui->ops;
    char info[260];
    memset(st, 0, sizeof *st);
    st->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, name);
    ez->panel.vt = &aoe_status_screen_vtable_ror.base;
    info[0] = '\0';
    if (info_name || info_id != -1) {
        snprintf(info, sizeof info, "%s", info_name ? info_name : "");
    } else if (current && current != aoe_ui_system_find_ror(ui->system, "Achievements Screen")) {
        const char *n = aoe_ui_ez_info_name_ror((const AoeUiEzRor *)current);
        snprintf(info, sizeof info, "%s", n ? n : "");
        info_id = aoe_ui_ez_info_id_ror((const AoeUiEzRor *)current);
    }
    if (info[0] == '\0') {
        int net = ops && ops->networked && ops->networked(ops->context);
        snprintf(info, sizeof info, "%s", net ? "scr3" : "scr2");
        info_id = net ? 0xc385 : 0xc384;
    }
    if (!aoe_ui_screen_setup_ror(ez, area, info, info_id, 0)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (text)
        aoe_ui_ez_create_text_ror(ez, &ez->panel, &st->text, text, 0x28, 0x28, 0x230, 400, 4, 1,
                                  1, 1);
    else
        aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &st->text, text_id, 0x28, 0x28, 0x230, 400,
                                     4, 1, 1, 1);
    return 1;
}

const AoeUiEzVtblRor aoe_status_screen_vtable_ror = {
    AOE_UI_EZ_SLOTS(status_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror,
                    aoe_ui_ez_action_slot_ror, aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

void aoe_status_screen_set_text_ror(AoeStatusScreenRor *st, const char *text) {
    if (st->text) ((const AoeUiTextVtblRor *)st->text->panel.vt)->set_text(st->text, text);
}

int aoe_status_screen_show_ror(AoeGameUiRor *ui, AoeUiAreaRor *area, const char *text,
                               const char *info_name, int32_t info_id) {
    static const char *const name = "Status Screen";
    AoeStatusScreenRor *st = (AoeStatusScreenRor *)aoe_ui_system_find_ror(ui->system, name);
    if (!st) {
        st = calloc(1, sizeof *st);
        if (!st) return 0;
        aoe_status_screen_init_ror(st, ui, area, name, text, -1, info_name, info_id);
    } else {
        aoe_status_screen_set_text_ror(st, text);
    }
    aoe_ui_system_show_named_ror(ui->system, name, 0);
    return 1;
}
