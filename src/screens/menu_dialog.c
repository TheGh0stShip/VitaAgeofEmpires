#include "game_ui.h"

#include <stdlib.h>

#include "../ui_ez_slots.h"

#define MENU(p) ((AoeMenuDialogRor *)(p))

static int single_player(const AoeGameUiRor *ui) {
    return ui->settings && aoe_settings_flag_97d_ror(ui->settings);
}

static int multiplayer(const AoeGameUiRor *ui) {
    return ui->settings && aoe_settings_flag_97e_ror(ui->settings);
}

static int viewed_player_playing(const AoeGameUiRor *ui) {
    void *player = ui->game ? aoe_game_view_player_object_ror(ui->game) : NULL;
    return player && ui->ops && ui->ops->player_state &&
           ui->ops->player_state(ui->ops->context, player) == 0;
}

static void menu_destroy(AoeUiPanelRor *p) {
    AoeMenuDialogRor *m = MENU(p);
    int i;
    for (i = 0; i < 11; ++i) {
        if (m->buttons[i]) {
            m->buttons[i]->panel.vt->destroy(&m->buttons[i]->panel);
            free(m->buttons[i]);
            m->buttons[i] = NULL;
        }
    }
    aoe_ui_dialog_release_ror(&m->dialog);
}

static int add(AoeMenuDialogRor *m, int32_t text, int32_t y, int32_t command) {
    AoeUiEzRor *ez = &m->dialog.ez;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &m->buttons[m->count],
                                        text, 0, 0x14, y, 0x168, 0x1e, 0, 0,
                                        command))
        return 0;
    ++m->count;
    return 1;
}

int aoe_menu_dialog_init_ror(AoeMenuDialogRor *m, AoeGameUiRor *ui,
                             AoeUiPanelRor *parent) {
    AoeUiEzRor *pez = (AoeUiEzRor *)parent;
    int32_t h = 300, y, i;
    int resign;
    AoeUiPanelRor *tab[11];
    if (!m || !ui) return 0;
    aoe_ui_dialog_init_ror(&m->dialog, ui->system, ui->ez_ops, "Menu Dialog");
    m->dialog.ez.panel.vt = &aoe_menu_dialog_vtable_ror.base;
    for (i = 0; i < 11; ++i) m->buttons[i] = NULL;
    m->count = 0;
    m->ui = ui;
    resign = multiplayer(ui) && viewed_player_playing(ui);
    if (resign) h = 0x14f;
    if (single_player(ui)) h += 0x69;
    if (!parent ||
        !aoe_ui_dialog_setup_ror(&m->dialog, parent->area, parent, 400, h,
                                 aoe_ui_ez_popup_name_ror(pez),
                                 aoe_ui_ez_popup_id_ror(pez), 1))
        return 0;
    if (!add(m, 0x2439, 0xf, 4)) return 0;
    y = 0x41;
    if (resign) {
        if (!add(m, 0x2440, 0x41, 0xc)) return 0;
        y = 100;
    }
    if (!add(m, 0x2437, y, 7) || !add(m, 0x243e, y + 0x23, 9)) return 0;
    y += 0x46;
    if (single_player(ui)) {
        if (!add(m, 0x2438, y, 1) || !add(m, 0x243c, y + 0x23, 3) ||
            !add(m, 0x243f, y + 0x46, 0xb))
            return 0;
        y += 0x69;
    }
    if (!add(m, 0x243a, y, 5) || !add(m, 0x23f5, y + 0x23, 8) ||
        !add(m, 0x23f9, y + 0x46, 10) || !add(m, 0x243b, y + 0x78, 6))
        return 0;
    m->buttons[m->count - 1]->hotkey_shift = 0;
    m->buttons[m->count - 1]->hotkey = 0x1b;
    aoe_ui_panel_set_current_ror(&m->dialog.ez.panel, &m->buttons[0]->panel);
    for (i = 0; i < m->count; ++i) tab[i] = &m->buttons[i]->panel;
    aoe_ui_panels_tab_cycle_ror(tab, (int16_t)m->count);
    return 1;
}

static void resume(AoeGameUiRor *ui) {
    if (single_player(ui) && ui->game && ui->game->comm_state == 0)
        aoe_game_comm_update_ror(ui->game, 0, 0, ui->game_ops);
}

static void ask(AoeGameUiRor *ui, AoeUiPanelRor *parent, int32_t text,
                const char *name) {
    AoeUiSystemRor *s = ui->system;
    aoe_ui_system_close_ror(s, "Menu Dialog");
    aoe_ui_system_show_named_ror(s, "Game Screen", 0);
    if (parent)
        aoe_ui_ez_yes_no_dialog_id_ror((AoeUiEzRor *)parent, text, name, 0x1c2,
                                       100);
}

static int32_t menu_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                           int32_t value, int32_t extra) {
    AoeMenuDialogRor *m = MENU(p);
    AoeGameUiRor *ui = m->ui;
    AoeUiSystemRor *s = ui->system;
    AoeUiPanelRor *parent = p->parent;
    const AoeGameUiOpsRor *ops = ui->ops;
    if (!from || code != 1)
        return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    switch (value) {
    case 1:
        if (ops && ops->open_save_screen) ops->open_save_screen(ops->context, 0);
        aoe_ui_system_show_named_ror(s, "Save Game Screen", 0);
        aoe_ui_system_close_ror(s, "Menu Dialog");
        return 1;
    case 2:
        if (ops && ops->open_save_screen) ops->open_save_screen(ops->context, 1);
        aoe_ui_system_show_named_ror(s, "Save Game Screen", 0);
        return 1;
    case 3:
        ask(ui, parent, 0xc1c, "QuitAndLoadDialog");
        return 1;
    case 8:
        resume(ui);
        aoe_ui_system_close_ror(s, "Menu Dialog");
        aoe_ui_system_show_named_ror(s, "Game Screen", 0);
        if (s->screen) s->screen->vt->slot_b0(s->screen);
        return 1;
    case 4:
        ui->quit_requested = 1;
        ask(ui, parent, 0xc1d, "QuitGameDialog");
        return 1;
    case 5:
        aoe_ui_system_close_ror(s, "Menu Dialog");
        aoe_ui_system_show_named_ror(s, "Game Screen", 0);
        if (ops && ops->open_options) ops->open_options(ops->context, parent);
        return 1;
    case 7:
        if (ops && ops->open_achievements)
            ops->open_achievements(ops->context,
                                   aoe_ui_string_static_ror(s, 0x269e));
        aoe_ui_system_show_named_ror(s, "Achievements Screen", 0);
        aoe_ui_system_close_ror(s, "Menu Dialog");
        return 1;
    case 10:
        aoe_ui_system_close_ror(s, "Menu Dialog");
        aoe_ui_system_show_named_ror(s, "Game Screen", 0);
        if (ops && ops->open_diplomacy) ops->open_diplomacy(ops->context, parent);
        return 1;
    case 11:
        ask(ui, parent, 0xc1e, "RestartDialog");
        return 1;
    case 9:
        if (ops && ops->open_mission) ops->open_mission(ops->context);
        aoe_ui_system_show_named_ror(s, "Mission Dialog", 0);
        aoe_ui_system_close_ror(s, "Menu Dialog");
        return 1;
    case 6:
        resume(ui);
        aoe_ui_system_show_named_ror(s, "Game Screen", 0);
        aoe_ui_system_close_ror(s, "Menu Dialog");
        return 1;
    case 12:
        ask(ui, parent, 0xc1f, "ResignDialog");
        return 1;
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_menu_dialog_vtable_ror = {
    AOE_UI_EZ_SLOTS(menu_destroy, aoe_ui_panel_handle_idle_ror,
                    aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                    aoe_ui_panel_zero_ror, menu_action,
                    aoe_ui_ez_set_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
