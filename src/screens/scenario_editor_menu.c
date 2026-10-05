#include "game_ui.h"

#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define MENU(p) ((AoeScenarioEditorMenuRor *)(p))

static const char *const NAME = "Scenario Editor Menu";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void menu_destroy(AoeUiPanelRor *p) {
    AoeScenarioEditorMenuRor *m = MENU(p);
    int i;
    DROP(m->title);
    for (i = 0; i < 4; ++i) DROP(m->buttons[i]);
    DROP(m->close);
    aoe_ui_ez_release_ror(&m->ez);
}

int aoe_scenario_editor_menu_init_ror(AoeScenarioEditorMenuRor *m, AoeGameUiRor *ui,
                                      AoeUiAreaRor *area) {
    static const int32_t texts[4] = {0x242e, 0x242f, 0x2431, 0x2430};
    AoeUiEzRor *ez = &m->ez;
    AoeUiPanelRor *tab[4];
    int i;
    memset(m, 0, sizeof *m);
    m->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_scenario_editor_menu_vtable_ror.base;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr6", 0xc386, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &m->title, 0x242d, 0x14, 0x14, 600, 0x1e,
                                      1, 1, 0, 0))
        return 1;
    for (i = 0; i < 4; ++i)
        if (!aoe_ui_ez_create_button_ror(ez, &ez->panel, &m->buttons[i], "", NULL, 0xaa,
                                         0xaa + 0x32 * i, 300, 0x28, 0, 0, 0))
            return 1;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &m->close, 0x3ea, 0, 0, 0, 0, 0, -1, -1,
                                        0))
        return 1;
    m->close->panel.vt->set_active(&m->close->panel, 1);
    m->close->panel.vt->set_position_mode(&m->close->panel, 9, 4, 4, 4, 4, 0x11, 0x11, 0x11,
                                          0x11, NULL, NULL, NULL, NULL);
    for (i = 0; i < 4; ++i) aoe_ui_button_set_text_id_ror(m->buttons[i], 0, texts[i]);
    m->buttons[3]->hotkey = 0x1b;
    m->buttons[3]->hotkey_shift = 0;
    for (i = 0; i < 4; ++i) tab[i] = &m->buttons[i]->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 4);
    aoe_ui_panel_set_current_ror(&ez->panel, &m->buttons[0]->panel);
    return 1;
}

static int32_t menu_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                           int32_t extra) {
    AoeScenarioEditorMenuRor *m = MENU(p);
    AoeUiSystemRor *s = m->ui->system;
    const AoeGameUiOpsRor *ops = m->ui->ops;
    if (from && code == 1) {
        if (m->buttons[0] && from == &m->buttons[0]->panel) {
            if (ops && ops->editor_load) ops->editor_load(ops->context, NULL, 0);
            return 1;
        }
        if (m->buttons[1] && from == &m->buttons[1]->panel) {
            if (ops && ops->open_editor_open_screen) ops->open_editor_open_screen(ops->context);
            aoe_ui_system_show_named_ror(s, "Scenario Editor Open", 0);
            aoe_ui_system_close_ror(s, NAME);
            return 1;
        }
        if (m->buttons[2] && from == &m->buttons[2]->panel) {
            if (ops && ops->open_campaign_editor) ops->open_campaign_editor(ops->context);
            aoe_ui_system_show_named_ror(s, "Campaign Editor Screen", 0);
            aoe_ui_system_close_ror(s, NAME);
            return 1;
        }
        if (m->buttons[3] && from == &m->buttons[3]->panel) {
            if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
            if (ops && ops->open_main_menu) ops->open_main_menu(ops->context);
            aoe_ui_system_show_named_ror(s, "Main Menu", 0);
            aoe_ui_system_close_ror(s, NAME);
            return 1;
        }
        if (m->close && from == &m->close->panel) {
            if (ops && ops->quit_program) ops->quit_program(ops->context);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_scenario_editor_menu_vtable_ror = {
    AOE_UI_EZ_SLOTS(menu_destroy, aoe_ui_panel_handle_idle_ror, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, menu_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
