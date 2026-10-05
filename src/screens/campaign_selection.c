#include "game_ui.h"

#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define CS(p) ((AoeCampaignSelectionRor *)(p))

static const char *const NAME = "Campaign Selection Screen";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void cs_destroy(AoeUiPanelRor *p) {
    AoeCampaignSelectionRor *c = CS(p);
    DROP(c->title);
    DROP(c->campaign_label);
    DROP(c->campaigns);
    DROP(c->campaigns_bar);
    DROP(c->scenario_label);
    DROP(c->scenarios);
    DROP(c->scenarios_bar);
    DROP(c->difficulty_label);
    DROP(c->difficulty);
    DROP(c->ok);
    DROP(c->cancel);
    DROP(c->close);
    aoe_ui_ez_release_ror(&c->ez);
}

/* Fill a list from a names array and pick the chosen entry. */
static int32_t fill(AoeUiTextRor *t, int32_t count, char **names, int32_t chosen) {
    int32_t i;
    for (i = 0; i < count; ++i) aoe_ui_text_append_ror(t, names[i], i);
    aoe_ui_text_select_ror(t, (int16_t)aoe_ui_text_index_of_id_ror(t, chosen));
    for (i = 0; i < count; ++i) free(names[i]);
    free(names);
    return count;
}

void aoe_campaign_selection_fill_campaigns_ror(AoeCampaignSelectionRor *c) {
    const AoeGameUiOpsRor *ops = c->ui->ops;
    AoeUiTextRor *t = &c->campaigns->text;
    char **names = NULL;
    int32_t chosen = 0, count;
    aoe_ui_text_clear_ror(t);
    if (!ops || !ops->campaign_titles) return;
    count = ops->campaign_titles(ops->context, &names, &chosen);
    if (!count) return;
    t->sorted = 1;
    fill(t, count, names, chosen);
    if (aoe_ui_text_selection_ror(t) != -1 && ops->choose_campaign)
        ops->choose_campaign(ops->context, aoe_ui_text_selected_id_ror(t));
}

void aoe_campaign_selection_fill_scenarios_ror(AoeCampaignSelectionRor *c) {
    const AoeGameUiOpsRor *ops = c->ui->ops;
    AoeUiTextRor *t = &c->scenarios->text;
    char **names = NULL;
    int32_t chosen = 0, count;
    aoe_ui_text_clear_ror(t);
    if (!ops || !ops->campaign_scenarios) return;
    count = ops->campaign_scenarios(ops->context, &names, &chosen);
    if (!count) return;
    fill(t, count, names, chosen);
    aoe_ui_list_move_ror(c->scenarios, 7, 0, 1);
}

void aoe_campaign_selection_start_ror(AoeCampaignSelectionRor *c) {
    const AoeGameUiOpsRor *ops = c->ui->ops;
    AoeGameSettingsRor *g = c->ui->settings;
    int32_t active[9] = {0}, kind[9] = {0}, players = 0, human = -1, human_ac4 = 0, i, d;
    int32_t line = aoe_ui_text_selection_ror(&c->scenarios->text);
    if (line == -1 || !ops || !g) return;
    if (ops->choose_scenario) ops->choose_scenario(ops->context, line);
    aoe_settings_set_flag_8fc_ror(g, 1);
    aoe_settings_set_int_9a0_ror(g, 1);
    aoe_settings_set_int_9a4_ror(g, 0);
    aoe_settings_set_a8c_a90_ror(g, 0, 0);
    d = aoe_ui_dropdown_selected_id_ror(c->difficulty);
    aoe_settings_set_byte_993_ror(g, (uint8_t)d);
    aoe_settings_set_int_a10_ror(g, aoe_ui_dropdown_selected_id_ror(c->difficulty));
    aoe_settings_set_option_ae5_ror(g, 0);
    aoe_settings_set_byte_1190_ror(g, 0);
    aoe_settings_set_int_a84_ror(g, 1);
    aoe_settings_set_int_a88_ror(g, 1);
    aoe_settings_set_byte_985_ror(g, 0);
    aoe_settings_set_byte_986_ror(g, 1);
    aoe_settings_set_byte_982_ror(g, 1);
    aoe_settings_set_option_ad8_ror(g, 0);
    aoe_settings_set_option_ad7_ror(g, 0);
    aoe_settings_set_option_ad6_ror(g, 1);
    aoe_settings_set_option_ad9_ror(g, 0);
    aoe_settings_set_resource_level_ror(g, 0);
    aoe_settings_set_starting_age_ror(g, 0);
    aoe_settings_set_option_ae4_ror(g, 0);
    if (!ops->campaign_scenario_slots ||
        !ops->campaign_scenario_slots(ops->context, active, kind)) {
        aoe_ui_ez_ok_dialog_id_ror(&c->ez, 0x9ca, NULL, 0x1c2, 100);
        return;
    }
    for (i = 0; i < 9; ++i) {
        if (!active[i]) continue;
        aoe_settings_set_int_aa0_ror(g, players, i);
        aoe_settings_set_byte_ac4_ror(g, players, (uint8_t)(i + 1));
        aoe_settings_set_player_994_ror(g, players, 1);
        ++players;
        if (human == -1 && kind[i] == 1) {
            human = i;
            human_ac4 = i + 1;
        }
    }
    if (!players) return;
    aoe_settings_set_byte_988_ror(g, (uint8_t)players);
    if (human != -1) {
        for (i = 1; i < players; ++i)
            if (aoe_settings_int_aa0_ror(g, i) == human) {
                aoe_settings_set_int_aa0_ror(g, i, aoe_settings_int_aa0_ror(g, 0));
                aoe_settings_set_byte_ac4_ror(g, i, aoe_settings_byte_ac4_ror(g, 0));
                break;
            }
        aoe_settings_set_int_aa0_ror(g, 0, human);
        aoe_settings_set_byte_ac4_ror(g, 0, (uint8_t)human_ac4);
    }
    if (ops->set_slot_state) {
        ops->set_slot_state(ops->context, 1, 2);
        for (i = 2; i <= players; ++i) ops->set_slot_state(ops->context, i, 4);
        for (i = players + 1; i <= 9; ++i) ops->set_slot_state(ops->context, i, 0);
    }
    if (!ops->start_scenario_game || !ops->start_scenario_game(ops->context))
        aoe_ui_ez_ok_dialog_id_ror(&c->ez, 0x961, NULL, 0x1c2, 100);
}

int aoe_campaign_selection_init_ror(AoeCampaignSelectionRor *c, AoeGameUiRor *ui,
                                    AoeUiAreaRor *area) {
    AoeUiEzRor *ez = &c->ez;
    AoeUiPanelRor *tab[5];
    int32_t y;
    memset(c, 0, sizeof *c);
    c->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_campaign_selection_vtable_ror.base;
    if (!aoe_ui_screen_setup_ror(ez, area, "scr2", 0xc384, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &c->title, 0x2bcd, 0x14, 10, 600, 0x1e, 1,
                                      1, 0, 0))
        return 1;
    c->camp_x = 0x14;
    c->camp_y = 0x46;
    c->camp_w = 600;
    c->camp_h = 0x82;
    if (!aoe_ui_ez_create_list_ror(ez, &ez->panel, &c->campaigns, c->camp_x, c->camp_y, c->camp_w,
                                   c->camp_h, 0xb))
        return 1;
    if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &c->campaigns_bar, &c->campaigns->text, 0x14))
        return 1;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &c->campaign_label, 0x2bce, c->camp_x - 5,
                                      c->camp_y - 0x14, 200, 0x14, 4, 0, 0, 0))
        return 1;
    ((const AoeUiTextVtblRor *)c->campaigns->text.panel.vt)->set_text_id(&c->campaigns->text,
                                                                        0x2b5d);
    y = c->camp_h + 0x1e + c->camp_y;
    c->scen_x = 0x14;
    c->scen_y = y;
    c->scen_w = 600;
    c->scen_h = 0x82;
    if (!aoe_ui_ez_create_list_ror(ez, &ez->panel, &c->scenarios, c->scen_x, c->scen_y, c->scen_w,
                                   c->scen_h, 0xb))
        return 1;
    if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &c->scenarios_bar, &c->scenarios->text, 0x14))
        return 1;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &c->scenario_label, 0x25fe, c->scen_x - 5,
                                      c->scen_y - 0x14, 200, 0x14, 4, 0, 0, 0))
        return 1;
    c->scenarios->text.sorted = 0;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &c->difficulty_label, 0x2bcf, 0xf, 0x172,
                                      200, 0x14, 4, 0, 0, 0))
        return 1;
    if (!aoe_ui_ez_create_dropdown_ror(ez, &ez->panel, &c->difficulty, 0x82, 100, 0x14, 0x186,
                                       0x82, 0x18, 0xb))
        return 1;
    aoe_ui_dropdown_clear_ror(c->difficulty);
    aoe_ui_dropdown_append_id_ror(c->difficulty, 0x2bd4, 4);
    aoe_ui_dropdown_append_id_ror(c->difficulty, 0x2bd3, 3);
    aoe_ui_dropdown_append_id_ror(c->difficulty, 0x2bd2, 2);
    aoe_ui_dropdown_append_id_ror(c->difficulty, 0x2bd1, 1);
    aoe_ui_dropdown_append_id_ror(c->difficulty, 0x2bd0, 0);
    aoe_ui_dropdown_select_ror(
        c->difficulty,
        (int16_t)aoe_ui_dropdown_index_of_id_ror(
            c->difficulty, ui->settings ? aoe_settings_byte_993_ror(ui->settings) : 0));
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->ok, 0xfa1, 0, 0x46, 0x1b8, 0xf0, 0x1e,
                                        0, 0, 0))
        return 1;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->cancel, 0xfa2, 0, 0x14a, 0x1b8, 0xf0,
                                        0x1e, 0, 0, 0))
        return 1;
    c->cancel->hotkey = 0x1b;
    c->cancel->hotkey_shift = 0;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &c->close, 0x3ea, 0, 0, 0, 0, 0, -1, -1,
                                        0))
        return 1;
    c->close->panel.vt->set_active(&c->close->panel, 1);
    c->close->panel.vt->set_position_mode(&c->close->panel, 9, 4, 4, 4, 4, 0x11, 0x11, 0x11, 0x11,
                                          NULL, NULL, NULL, NULL);
    tab[0] = &c->campaigns->text.panel;
    tab[1] = &c->scenarios->text.panel;
    tab[2] = &c->difficulty->panel;
    tab[3] = &c->ok->panel;
    tab[4] = &c->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 5);
    aoe_ui_panel_set_current_ror(&ez->panel, &c->ok->panel);
    aoe_campaign_selection_fill_campaigns_ror(c);
    aoe_campaign_selection_fill_scenarios_ror(c);
    aoe_ui_panel_set_current_ror(&ez->panel, &c->campaigns->text.panel);
    c->filled = 1;
    return 1;
}

static int32_t cs_idle(AoeUiPanelRor *p) {
    AoeCampaignSelectionRor *c = CS(p);
    const AoeGameUiOpsRor *ops = c->ui->ops;
    if (!c->filled) {
        aoe_campaign_selection_fill_campaigns_ror(c);
        aoe_campaign_selection_fill_scenarios_ror(c);
        aoe_ui_panel_set_current_ror(p, &c->campaigns->text.panel);
        c->filled = 1;
    }
    if (ops && ops->arrow_cursor) ops->arrow_cursor(ops->context);
    return aoe_ui_panel_handle_idle_ror(p);
}

static int32_t cs_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeCampaignSelectionRor *c = CS(p);
    const AoeGameUiOpsRor *ops = c->ui->ops;
    AoeUiSystemRor *sys = c->ui->system;
    if (from && c->filled) {
        if (c->ok && from == &c->ok->panel && code == 1) {
            aoe_campaign_selection_start_ror(c);
            return 1;
        }
        if (c->cancel && from == &c->cancel->panel && code == 1) {
            if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
            if (ops && ops->open_name_selection) ops->open_name_selection(ops->context);
            aoe_ui_system_show_named_ror(sys, "Name Selection Screen", 0);
            aoe_ui_system_close_ror(sys, NAME);
            return 1;
        }
        if (c->close && from == &c->close->panel && code == 1) {
            if (ops && ops->quit_program) ops->quit_program(ops->context);
            return 1;
        }
        if (from == &c->campaigns->text.panel && code == 1) {
            if (ops && ops->choose_campaign)
                ops->choose_campaign(ops->context, aoe_ui_text_selected_id_ror(&c->campaigns->text));
            aoe_campaign_selection_fill_scenarios_ror(c);
            return 1;
        }
        if (from == &c->scenarios->text.panel && code == 3) {
            p->vt->action(p, &c->ok->panel, 1, 0, 0);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_campaign_selection_vtable_ror = {
    AOE_UI_EZ_SLOTS(cs_destroy, cs_idle, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, cs_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
