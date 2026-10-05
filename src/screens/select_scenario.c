#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define SEL(p) ((AoeSelectScenarioRor *)(p))

static const char *const NAME = "Select Scenario Screen";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void free_arrays(AoeSelectScenarioRor *s) {
    int32_t i;
    if (s->descriptions) {
        for (i = 0; i < s->count; ++i) free(s->descriptions[i]);
        free(s->descriptions);
        s->descriptions = NULL;
    }
    free(s->flags);
    s->flags = NULL;
}

static void sel_destroy(AoeUiPanelRor *p) {
    AoeSelectScenarioRor *s = SEL(p);
    DROP(s->title);
    DROP(s->name_label);
    DROP(s->players_label);
    DROP(s->list);
    DROP(s->list_bar);
    DROP(s->description_label);
    DROP(s->description);
    DROP(s->description_bar);
    DROP(s->ok);
    DROP(s->cancel);
    DROP(s->close);
    /* The original never frees the description and flag arrays. */
    free_arrays(s);
    aoe_ui_ez_release_ror(&s->ez);
}

void aoe_select_scenario_fill_ror(AoeSelectScenarioRor *s) {
    const AoeGameUiOpsRor *ops = s->ui->ops;
    AoeUiTextRor *t = &s->list->text;
    const char *name;
    int32_t i;
    if (ops && ops->scenario_index_refresh) ops->scenario_index_refresh(ops->context);
    aoe_ui_text_clear_ror(t);
    t->sorted = 1;
    free_arrays(s);
    s->count = 0;
    while (ops && ops->scenario_index_name &&
           (name = ops->scenario_index_name(ops->context, s->count)) != NULL) {
        char base[260];
        size_t n = strlen(name);
        n = n >= 4 ? n - 4 : 0;
        if (n >= sizeof base) n = sizeof base - 1;
        memcpy(base, name, n);
        base[n] = '\0';
        aoe_ui_text_append_ror(t, base, s->count);
        ++s->count;
    }
    if (s->count <= 0) return;
    s->descriptions = calloc((size_t)s->count, sizeof *s->descriptions);
    s->flags = calloc((size_t)s->count, sizeof *s->flags);
    if (!s->descriptions || !s->flags) return;
    for (i = 0; i < s->count; ++i) {
        const char *desc = NULL;
        int32_t flag = 0, players = 0;
        if (!ops->scenario_index_info ||
            !ops->scenario_index_info(ops->context, aoe_ui_text_id_ror(t, i), &desc, &flag,
                                      &players))
            continue;
        s->descriptions[i] = calloc(0x1000, 1);
        if (s->descriptions[i] && desc) snprintf(s->descriptions[i], 0x1000, "%s", desc);
        s->flags[i] = flag;
        if (players > 0) {
            char column[16], text[260];
            if (flag == 0 && players > 2)
                snprintf(column, sizeof column, "%d - %d", 2, (int)players);
            else
                snprintf(column, sizeof column, "%d", (int)players);
            snprintf(text, sizeof text, "%s", aoe_ui_text_line_ror(t, i));
            aoe_ui_text_replace_ror(t, i, text, column, 0);
        }
    }
}

static void show_description(AoeSelectScenarioRor *s) {
    const AoeUiTextVtblRor *vt = (const AoeUiTextVtblRor *)s->description->panel.vt;
    int32_t i = aoe_ui_text_selection_ror(&s->list->text);
    vt->set_text(s->description, "");
    if (i >= 0 && s->descriptions && i < s->count && s->descriptions[i])
        vt->set_text(s->description, s->descriptions[i]);
    s->chosen = i;
}

static void pick_settings_scenario(AoeSelectScenarioRor *s) {
    const char *name = s->ui->settings ? aoe_settings_scenario_name_ror(s->ui->settings) : "";
    int32_t i = aoe_ui_text_index_of_ror(&s->list->text, name ? name : "");
    s->chosen = -1;
    if (i != -1) {
        aoe_ui_list_move_ror(s->list, 1, (int16_t)i, 1);
        s->chosen = i - 1; /* as in the original */
        return;
    }
    aoe_ui_list_move_ror(s->list, 1, 0, 1);
}

static void show_labels(AoeSelectScenarioRor *s) {
    AoeUiPanelRor *tab[3];
    AoeUiPanelRor *p = &s->ez.panel;
    s->description_label->panel.vt->set_active(&s->description_label->panel, 1);
    s->description->panel.vt->set_active(&s->description->panel, 1);
    tab[0] = &s->list->text.panel;
    tab[1] = &s->ok->panel;
    tab[2] = &s->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, 3);
    if (!p->current || !p->current->active) aoe_ui_panel_set_current_ror(p, &s->ok->panel);
}

static void store_choice(AoeSelectScenarioRor *s) {
    AoeGameSettingsRor *g = s->ui->settings;
    if (!g) return;
    aoe_settings_set_flag_8fc_ror(g, 1);
    aoe_settings_set_scenario_name_ror(g, aoe_ui_text_selected_text_ror(&s->list->text));
    aoe_settings_set_option_ae5_ror(g, 0);
}

int aoe_select_scenario_init_ror(AoeSelectScenarioRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area) {
    AoeUiEzRor *ez = &s->ez;
    const AoeGameUiOpsRor *ops = ui->ops;
    int net;
    memset(s, 0, sizeof *s);
    s->ui = ui;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_select_scenario_vtable_ror.base;
    net = ops && ops->networked && ops->networked(ops->context);
    if (!aoe_ui_screen_setup_ror(ez, area, net ? "scr3" : "scr2", net ? 0xc385 : 0xc384, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &s->title, 0x25fa, 0x14, 10, 600, 0x1e, 1,
                                      1, 0, 0))
        return 1;
    s->list_x = 0x14;
    s->list_y = 0x46;
    s->list_w = 600;
    s->list_h = 0x107;
    if (!aoe_ui_ez_create_list_ror(ez, &ez->panel, &s->list, s->list_x, s->list_y, s->list_w,
                                   s->list_h, 0xb))
        return 1;
    s->list->text.column_x = (s->list_w - 0x3c) * ez->panel.width / 0x280;
    s->list->text.panel.vt->set_redraw(&s->list->text.panel, 1);
    ((const AoeUiTextVtblRor *)s->list->text.panel.vt)->set_text_id(&s->list->text, 0x25fd);
    if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &s->list_bar, &s->list->text, 0x14)) return 1;
    aoe_ui_panel_set_help_ror(&s->list_bar->panel, -1, -1);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &s->name_label, 0x25fe, s->list_x,
                                      s->list_y - 0x14, 300, 0x14, 4, 0, 0, 0))
        return 1;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &s->players_label, 0x25ff,
                                      s->list_w - 0xdc + s->list_x, s->list_y - 0x14, 200, 0x14,
                                      4, 0, 0, 0))
        return 1;
    aoe_ui_text_set_alignment_ror(s->players_label, 3, 2);
    s->desc_y = s->list_h + 0x19 + s->list_y;
    s->desc_x = s->list_x;
    s->desc_w = s->list_w;
    s->desc_h = 0x3c;
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &s->description_label, 0x2600, s->desc_x,
                                      s->desc_y - 0x14, 400, 0x14, 4, 0, 0, 0))
        return 1;
    if (!aoe_ui_ez_create_text_ror(ez, &ez->panel, &s->description, "", s->desc_x, s->desc_y,
                                   s->desc_w, s->desc_h, 0xb, 0, 0, 1))
        return 1;
    ((const AoeUiTextVtblRor *)s->description->panel.vt)->set_bevel(s->description, 3, ez->bevel);
    if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &s->description_bar, s->description, 0x14))
        return 1;
    aoe_ui_panel_set_help_ror(&s->description_bar->panel, 0x7532, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &s->ok, 0xfa1, 0, 0x46, 0x1b8, 0xf0, 0x1e,
                                        0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&s->ok->panel, 0x7531, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &s->cancel, 0xfa2, 0, 0x14a, 0x1b8, 0xf0,
                                        0x1e, 0, 0, 0))
        return 1;
    aoe_ui_panel_set_help_ror(&s->cancel->panel, 0x7532, -1);
    s->cancel->hotkey = 0x1b;
    s->cancel->hotkey_shift = 0;
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &s->close, 0x3ea, 0, 0, 0, 0, 0, -1, -1,
                                        0))
        return 1;
    s->close->panel.vt->set_active(&s->close->panel, 1);
    s->close->panel.vt->set_position_mode(&s->close->panel, 9, 4, 4, 4, 4, 0x11, 0x11, 0x11, 0x11,
                                          NULL, NULL, NULL, NULL);
    aoe_select_scenario_fill_ror(s);
    s->filled = 1;
    pick_settings_scenario(s);
    show_description(s);
    show_labels(s);
    aoe_ui_panel_set_current_ror(&ez->panel, &s->list->text.panel);
    return 1;
}

static int32_t sel_idle(AoeUiPanelRor *p) {
    AoeSelectScenarioRor *s = SEL(p);
    const AoeGameUiOpsRor *ops = s->ui->ops;
    if (!s->filled) {
        aoe_select_scenario_fill_ror(s);
        show_description(s);
        s->filled = 1;
        show_labels(s);
        aoe_ui_panel_set_current_ror(p, &s->list->text.panel);
    }
    if (ops && ops->arrow_cursor) ops->arrow_cursor(ops->context);
    return aoe_ui_panel_handle_idle_ror(p);
}

static int32_t sel_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                          int32_t extra) {
    AoeSelectScenarioRor *s = SEL(p);
    const AoeGameUiOpsRor *ops = s->ui->ops;
    AoeUiSystemRor *sys = s->ui->system;
    if (from && s->filled) {
        if (s->ok && from == &s->ok->panel && code == 1) {
            aoe_ui_panel_set_current_ror(p, &s->list->text.panel);
            store_choice(s);
            if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
            if (ops && ops->open_mp_setup) ops->open_mp_setup(ops->context);
            aoe_ui_system_show_named_ror(sys, "MP Setup Screen", 0);
            aoe_ui_system_close_ror(sys, NAME);
            return 1;
        }
        if (s->cancel && from == &s->cancel->panel && code == 1) {
            aoe_ui_panel_set_current_ror(p, &s->list->text.panel);
            if (ops && ops->wait_cursor) ops->wait_cursor(ops->context);
            if (ops && ops->open_single_player_menu) ops->open_single_player_menu(ops->context);
            aoe_ui_system_show_named_ror(sys, "Single Player Menu", 0);
            aoe_ui_system_close_ror(sys, NAME);
            return 1;
        }
        if (s->close && from == &s->close->panel && code == 1) {
            if (ops && ops->quit_program) ops->quit_program(ops->context);
            return 1;
        }
        if (from == &s->list->text.panel) {
            if (code == 3) {
                p->vt->action(p, &s->ok->panel, 1, 0, 0);
                return 1;
            }
            if (code == 1) {
                show_description(s);
                show_labels(s);
                return 1;
            }
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_select_scenario_vtable_ror = {
    AOE_UI_EZ_SLOTS(sel_destroy, sel_idle, aoe_ui_panel_handle_paint_ror,
                    aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror, sel_action,
                    aoe_ui_screen_focus_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
