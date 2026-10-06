#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define MS(p) ((AoeMpStartupScreenRor *)(p))

static const char *const NAME = "MP Startup Screen";
static const char *const NAME_ERROR = "Name Error";
static const char *const LIST_ERROR = "List Error";
static const char *const START_ZONE = "Start Zone";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

/* 0x00487770 (deleting 0x00487750). */
static void ms_destroy(AoeUiPanelRor *p) {
    AoeMpStartupScreenRor *m = MS(p);
    AoeUiSystemRor *sys = m->ui->system;
    DROP(m->title);
    DROP(m->name_label);
    DROP(m->name);
    DROP(m->list_label);
    DROP(m->list);
    DROP(m->list_bar);
    DROP(m->ok);
    DROP(m->cancel);
    DROP(m->help);
    DROP(m->close);
    if (aoe_ui_system_find_ror(sys, "Create Dialog")) aoe_ui_system_close_ror(sys, "Create Dialog");
    if (aoe_ui_system_find_ror(sys, "OKDialog")) aoe_ui_system_close_ror(sys, "OKDialog");
    if (aoe_ui_system_find_ror(sys, "YesNoDialog")) aoe_ui_system_close_ror(sys, "YesNoDialog");
    aoe_ui_ez_release_ror(&m->ez);
}

/* 0x00487d70 */
void aoe_mp_startup_screen_fill_ror(AoeMpStartupScreenRor *m) {
    const AoeGameUiOpsRor *ops = m->ui->ops;
    AoeUiTextRor *t = &m->list->text;
    char remembered[0x100];
    const char *r;
    int32_t n, i, chosen = -1;
    aoe_ui_text_clear_ror(t);
    n = ops && ops->mp_services ? ops->mp_services(ops->context) : 0;
    r = ops && ops->mp_service_name ? ops->mp_service_name(ops->context) : NULL;
    snprintf(remembered, sizeof remembered, "%s", r ? r : "");
    for (i = 0; i < n; ++i) {
        const char *name = ops->mp_service ? ops->mp_service(ops->context, i) : NULL;
        if (!name) continue;
        aoe_ui_text_append_ror(t, name, i);
        if (strcmp(name, remembered) == 0) chosen = i;
    }
    if (m->zone[0]) aoe_ui_text_append_id_ror(t, 0x258e, -1);
    if (chosen != -1) aoe_ui_text_select_ror(t, (int16_t)aoe_ui_text_index_of_id_ror(t, chosen));
}

/* 0x00487250 */
int aoe_mp_startup_screen_init_ror(AoeMpStartupScreenRor *m, AoeGameUiRor *ui,
                                   AoeUiAreaRor *area) {
    const AoeGameUiOpsRor *ops = ui->ops;
    AoeUiEzRor *ez = &m->ez;
    AoeUiPanelRor *tab[5];
    char name[0x100];
    const char *saved;
    memset(m, 0, sizeof *m);
    m->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_mp_startup_screen_vtable_ror.base;
    if (ui->settings) aoe_settings_set_flag_97e_ror(ui->settings, 0);
    if (!aoe_ui_screen_setup_ror(ez, area, "scr3", 0xc385, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &m->title, 0x258b, 0x14, 0x14, 600, 0x1e, 1,
                                      1, 0, 0) ||
        !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &m->name_label, 0x258c, 0x14, 0x4b, 500,
                                      0x14, 4, 0, 0, 0))
        return 1;
    saved = ops && ops->mp_player_name ? ops->mp_player_name(ops->context) : NULL;
    snprintf(name, sizeof name, "%s", saved ? saved : "");
    if (!aoe_ui_ez_create_edit_ror(ez, &ez->panel, &m->name, name, 0xf, 0, 0x14, 100, 600, 0x17,
                                   0xb, 1, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&m->name->panel, 0x7595, -1);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &m->list_label, 0x258d, 0x14, 0xa5, 600,
                                      0x14, 4, 0, 0, 0) ||
        !aoe_ui_ez_create_list_ror(ez, &ez->panel, &m->list, 0x14, 0xbe, 600, 0xd2, 0xb) ||
        !aoe_ui_ez_create_text_scrollbar_ror(ez, &m->list_bar, &m->list->text, 0x14))
        return 1;
    aoe_ui_panel_set_help_ror(&m->list_bar->panel, 0x7596, 0x25f08);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &m->ok, 0xfa1, 0, 0x46, 0x1b8, 0xf0, 0x1e,
                                        0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&m->ok->panel, 0x7531, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &m->cancel, 0xfa2, 0, 0x14a, 0x1b8, 0xf0,
                                        0x1e, 0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&m->cancel->panel, 0x7532, -1);
    m->cancel->hotkey = 0x1b;
    m->cancel->hotkey_shift = 0;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &m->help, 0xfa9, 0, 0x244, 0x1b8, 0x1e,
                                        0x1e, 0, 0, 0) ||
        !aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &m->close, 0x3ea, 0, 0, 0, 0, 0, -1, -1,
                                        0))
        return 1;
    m->close->panel.vt->set_active(&m->close->panel, 1);
    m->close->panel.vt->set_position_mode(&m->close->panel, 9, 4, 4, 4, 4, 0x11, 0x11, 0x11, 0x11,
                                          NULL, NULL, NULL, NULL);
    if (!(ops && ops->zone_command && ops->zone_command(ops->context, m->zone, sizeof m->zone)))
        m->zone[0] = '\0';
    aoe_mp_startup_screen_fill_ror(m);
    aoe_ui_panel_set_current_ror(&ez->panel, &m->name->panel);
    tab[0] = &m->name->panel;
    tab[1] = &m->ok->panel;
    tab[2] = &m->list->text.panel;
    tab[3] = &m->cancel->panel;
    tab[4] = &m->help->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 5);
    return 1;
}

static int named(AoeUiPanelRor *from, const char *name) {
    return from->name && strcmp(from->name, name) == 0;
}

/* 0x00487880 (+0xb8) */
static int32_t ms_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeMpStartupScreenRor *m = MS(p);
    const AoeGameUiOpsRor *ops = m->ui->ops;
    AoeUiSystemRor *sys = m->ui->system;
    AoeUiTextRor *t = &m->list->text;
    if (!from) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (from == &t->panel && code == 3) return p->vt->action(p, &m->ok->panel, 1, 0, 0);
    if (from == &m->close->panel && code == 1) {
        if (ops && ops->quit_program) ops->quit_program(ops->context);
        return 1;
    }
    if (from == &m->ok->panel && code == 1) {
        char name[0x100];
        const char *service;
        AoeJoinScreenRor *j;
        if (m->ui->settings) aoe_settings_set_flag_97e_ror(m->ui->settings, 1);
        if (!*aoe_ui_edit_text_ror(m->name)) {
            aoe_ui_ez_ok_dialog_id_ror(&m->ez, 0x258f, NAME_ERROR, 0x1c2, 100);
            return 1;
        }
        if (aoe_ui_text_count_ror(t) == 0) {
            aoe_ui_ez_ok_dialog_id_ror(&m->ez, 0x2592, LIST_ERROR, 0x1c2, 100);
            return 1;
        }
        if (m->zone[0] && aoe_ui_text_selected_id_ror(t) == -1) {
            aoe_ui_ez_yes_no_dialog_id_ror(&m->ez, 0x2593, START_ZONE, 0x1c2, 100);
            return 1;
        }
        if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
        snprintf(name, sizeof name, "%s", aoe_ui_edit_text_ror(m->name));
        service = aoe_ui_text_selected_text_ror(t);
        if (ops && ops->set_mp_player_name) ops->set_mp_player_name(ops->context, name);
        if (ops && ops->set_mp_service_name)
            ops->set_mp_service_name(ops->context, service ? service : "");
        if (ops && ops->mp_start) ops->mp_start(ops->context, name, aoe_ui_text_selected_id_ror(t));
        j = calloc(1, sizeof *j);
        if (j && !aoe_join_screen_init_ror(j, m->ui, p->area)) {
            if (aoe_ui_system_find_ror(sys, "Join Screen")) aoe_ui_system_close_ror(sys, "Join Screen");
            else free(j);
        }
        aoe_ui_system_show_named_ror(sys, "Join Screen", 0);
        aoe_ui_system_close_ror(sys, NAME);
        return 1;
    }
    if ((from == &m->cancel->panel && code == 1) || (from == &m->name->panel && code == 1)) {
        if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
        if (ops && ops->mp_drop) ops->mp_drop(ops->context, 2);
        if (ops && ops->open_main_menu) ops->open_main_menu(ops->context);
        aoe_ui_system_show_named_ror(sys, "Main Menu", 0);
        aoe_ui_system_close_ror(sys, NAME);
        return 1;
    }
    if (from == &m->help->panel && code == 1) {
        aoe_ui_ez_help_mode_on_ror(&m->ez);
        return 1;
    }
    if (named(from, LIST_ERROR)) {
        aoe_ui_system_close_ror(sys, LIST_ERROR);
        aoe_ui_panel_set_current_ror(p, &t->panel);
        return 1;
    }
    if (named(from, NAME_ERROR)) {
        aoe_ui_system_close_ror(sys, NAME_ERROR);
        aoe_ui_panel_set_current_ror(p, &m->name->panel);
        return 1;
    }
    if (named(from, START_ZONE)) {
        aoe_ui_system_close_ror(sys, START_ZONE);
        aoe_ui_panel_set_current_ror(p, &t->panel);
        if (code != 0) return 1;
        if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
        if (ops && ops->run_zone) ops->run_zone(ops->context, m->zone);
        if (ops && ops->quit_program) ops->quit_program(ops->context);
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_mp_startup_screen_vtable_ror = {
    AOE_UI_EZ_SLOTS(ms_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, ms_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
