#include "game_ui.h"

#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define SP(p) ((AoeSinglePlayerMenuRor *)(p))

static const char *const NAME = "Single Player Menu";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void sp_destroy(AoeUiPanelRor *p) {
    AoeSinglePlayerMenuRor *m = SP(p);
    int i;
    DROP(m->title);
    for (i = 0; i < 6; ++i) DROP(m->buttons[i]);
    DROP(m->close);
    aoe_ui_ez_release_ror(&m->ez);
}

int aoe_single_player_menu_init_ror(AoeSinglePlayerMenuRor *m, AoeGameUiRor *ui,
                                    AoeUiAreaRor *area) {
    static const int32_t texts[6] = {0x240a, 0x2408, 0x240b, 0x2405, 0x2407, 0x2409};
    AoeUiEzRor *ez = &m->ez;
    AoeUiPanelRor *tab[6];
    int i;
    memset(m, 0, sizeof *m);
    m->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_single_player_menu_vtable_ror.base;
    if (ui->settings) {
        aoe_settings_set_flag_97d_ror(ui->settings, 1);
        aoe_settings_set_scenario_name_ror(ui->settings, "");
    }
    if (!aoe_ui_screen_setup_ror(ez, area, "scr2", 0xc384, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &m->title, 0x2404, 0x14, 0x14, 600, 0x1e,
                                      1, 1, 0, 0))
        return 1;
    for (i = 0; i < 6; ++i)
        if (!aoe_ui_ez_create_button_ror(ez, &ez->panel, &m->buttons[i], "", NULL, 0xaa,
                                         0x76 + 0x32 * i, 300, 0x28, 0, 0, 0))
            return 1;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &m->close, 0x3ea, 0, 0, 0, 0, 0, -1, -1,
                                        0))
        return 1;
    m->close->panel.vt->set_active(&m->close->panel, 1);
    m->close->panel.vt->set_position_mode(&m->close->panel, 9, 4, 4, 4, 4, 0x11, 0x11, 0x11,
                                          0x11, NULL, NULL, NULL, NULL);
    for (i = 0; i < 6; ++i) aoe_ui_button_set_text_id_ror(m->buttons[i], 0, texts[i]);
    m->buttons[5]->hotkey = 0x1b;
    m->buttons[5]->hotkey_shift = 0;
    aoe_ui_panel_set_current_ror(&ez->panel, &m->buttons[0]->panel);
    for (i = 0; i < 6; ++i) tab[i] = &m->buttons[i]->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 6);
    return 1;
}

static void setup_game(AoeSinglePlayerMenuRor *m, uint8_t scenario, uint8_t death_match) {
    AoeGameSettingsRor *g = m->ui->settings;
    if (!g) return;
    aoe_settings_set_int_9a0_ror(g, 0);
    aoe_settings_set_int_9a4_ror(g, 0);
    aoe_settings_set_flag_8fc_ror(g, scenario);
    aoe_settings_set_option_ae5_ror(g, death_match);
    aoe_settings_set_byte_1190_ror(g, 0);
}

static void go(AoeSinglePlayerMenuRor *m, void (*open)(void *), const char *screen) {
    const AoeGameUiOpsRor *ops = m->ui->ops;
    if (open) open(ops->context);
    aoe_ui_system_show_named_ror(m->ui->system, screen, 0);
    aoe_ui_system_close_ror(m->ui->system, NAME);
}

static int32_t sp_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeSinglePlayerMenuRor *m = SP(p);
    const AoeGameUiOpsRor *ops = m->ui->ops;
    int which = -1, i;
    if (from && code == 1) {
        for (i = 0; i < 6; ++i)
            if (m->buttons[i] && from == &m->buttons[i]->panel) which = i;
        if (m->close && from == &m->close->panel) which = 6;
    }
    if (which < 0 || !ops) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (which == 6) {
        if (ops->quit_program) ops->quit_program(ops->context);
        return 1;
    }
    if (which == 0 || which == 2) setup_game(m, 0, which == 2);
    if (which == 3) setup_game(m, 1, 0);
    if (ops->wait_cursor) ops->wait_cursor(ops->context);
    switch (which) {
    case 0: /* random map */
    case 2: /* death match */
        go(m, ops->open_mp_setup, "MP Setup Screen");
        break;
    case 3:
        go(m, ops->open_select_scenario, "Select Scenario Screen");
        break;
    case 4:
        go(m, ops->open_load_saved, "Load Saved Game Screen");
        break;
    case 1:
        if (ops->campaign_players && ops->campaign_players(ops->context) > 0)
            go(m, ops->open_name_selection, "Name Selection Screen");
        else if (ops->open_new_name)
            ops->open_new_name(ops->context, p);
        break;
    case 5:
        go(m, ops->open_main_menu, "Main Menu");
        break;
    }
    return 1;
}

const AoeUiEzVtblRor aoe_single_player_menu_vtable_ror = {
    AOE_UI_EZ_SLOTS(sp_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, sp_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
