#include "game_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_ez_slots.h"

#define GS(p) ((AoeGameSettingsScreenRor *)(p))

static const char *const NAME = "Game Settings Screen";

static void drop(AoeUiPanelRor **slot) {
    if (*slot) {
        (*slot)->vt->destroy(*slot);
        free(*slot);
        *slot = NULL;
    }
}
#define DROP(x) drop((AoeUiPanelRor **)&(x))

static void free_arrays(AoeGameSettingsScreenRor *g) {
    int32_t i;
    if (g->descriptions) {
        for (i = 0; i < g->count; ++i) free(g->descriptions[i]);
        free(g->descriptions);
        g->descriptions = NULL;
    }
    free(g->flags);
    g->flags = NULL;
}

static void gs_destroy(AoeUiPanelRor *p) {
    AoeGameSettingsScreenRor *g = GS(p);
    int i;
    DROP(g->title);
    DROP(g->size_label);
    DROP(g->size);
    DROP(g->type_label);
    DROP(g->type);
    DROP(g->victory_label);
    DROP(g->victory);
    DROP(g->years);
    DROP(g->years_label);
    DROP(g->score);
    DROP(g->score_label);
    DROP(g->scenario_victory);
    DROP(g->difficulty_label);
    DROP(g->difficulty);
    DROP(g->age_label);
    DROP(g->age);
    DROP(g->resources_label);
    DROP(g->resources);
    DROP(g->list_label);
    DROP(g->players_label);
    DROP(g->list);
    DROP(g->list_bar);
    DROP(g->description_label);
    DROP(g->description);
    DROP(g->description_bar);
    DROP(g->path_label);
    DROP(g->path);
    DROP(g->population_label);
    DROP(g->population);
    for (i = 0; i < 4; ++i) {
        DROP(g->checks[i]);
        DROP(g->check_texts[i]);
    }
    DROP(g->ok);
    DROP(g->cancel);
    DROP(g->help);
    free_arrays(g);
    aoe_ui_ez_release_ror(&g->ez);
}

static int networked(const AoeGameSettingsScreenRor *g) {
    const AoeGameUiOpsRor *ops = g->ui->ops;
    return ops && ops->networked && ops->networked(ops->context);
}

static int label(AoeGameSettingsScreenRor *g, AoeUiTextRor **out, int32_t id, int32_t x,
                 int32_t y, int32_t w, int32_t font) {
    return aoe_ui_ez_create_text_id_ror(&g->ez, &g->ez.panel, out, id, x, y, w, 0x14, font, 0, 0,
                                        0);
}

static int dropdown(AoeGameSettingsScreenRor *g, AoeUiDropdownRor **out, int32_t x, int32_t y) {
    if (!aoe_ui_ez_create_dropdown_ror(&g->ez, &g->ez.panel, out, 0xb4, 100, x, y, 0xb4, 0x18,
                                       0xb))
        return 0;
    aoe_ui_dropdown_clear_ror(*out);
    return 1;
}

static void help(void *panel, int32_t id, int32_t page) {
    aoe_ui_panel_set_help_ror((AoeUiPanelRor *)panel, id, page);
}

static void select_value(AoeUiDropdownRor *d, int32_t id) {
    aoe_ui_dropdown_select_ror(d, (int16_t)aoe_ui_dropdown_index_of_id_ror(d, id));
}

void aoe_game_settings_screen_fill_ror(AoeGameSettingsScreenRor *g) {
    const AoeGameUiOpsRor *ops = g->ui->ops;
    AoeGameSettingsRor *st = g->ui->settings;
    AoeUiTextRor *t = &g->list->text;
    int net = networked(g);
    const char *chosen = st ? aoe_settings_scenario_name_ror(st) : "";
    int32_t i;
    aoe_ui_text_clear_ror(t);
    g->offset = 0;
    free_arrays(g);
    g->count = 0;
    if (net || (st && aoe_settings_flag_8fc_ror(st))) {
        const char *name;
        if (ops && ops->scenario_index_refresh) ops->scenario_index_refresh(ops->context);
        t->sorted = 1;
        for (i = 0; ops && ops->scenario_index_name &&
                    (name = ops->scenario_index_name(ops->context, i)) != NULL;
             ++i) {
            char base[260];
            size_t n = strlen(name);
            n = n >= 4 ? n - 4 : 0;
            if (n >= sizeof base) n = sizeof base - 1;
            memcpy(base, name, n);
            base[n] = '\0';
            if (net || strcmp(base, chosen ? chosen : "") == 0) {
                aoe_ui_text_append_ror(t, base, i);
                ++g->count;
            }
        }
        if (g->count > 0) {
            g->descriptions = calloc((size_t)g->count, sizeof *g->descriptions);
            g->flags = calloc((size_t)g->count, sizeof *g->flags);
        }
        for (i = 0; g->descriptions && g->flags && i < g->count; ++i) {
            const char *desc = NULL;
            int32_t flag = 0, players = 0;
            if (!ops->scenario_index_info ||
                !ops->scenario_index_info(ops->context, aoe_ui_text_id_ror(t, i), &desc, &flag,
                                          &players))
                continue;
            g->descriptions[i] = calloc(0x1000, 1);
            if (g->descriptions[i] && desc) snprintf(g->descriptions[i], 0x1000, "%s", desc);
            g->flags[i] = flag;
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
    t->sorted = 0;
    if (st && (net || (aoe_settings_flag_8fc_clear_ror(st) && !aoe_settings_option_ae5_ror(st)))) {
        aoe_ui_text_insert_ror(t, 0, aoe_ui_string_static_ror(g->ui->system, 0x25b5), "2 - 8", -2);
        --g->offset;
    }
    if (st && (net || (aoe_settings_flag_8fc_clear_ror(st) && aoe_settings_option_ae5_ror(st)))) {
        aoe_ui_text_insert_ror(t, 1, aoe_ui_string_static_ror(g->ui->system, 0x2617), "2 - 8", -1);
        --g->offset;
    }
}

static void show_description(AoeGameSettingsScreenRor *g) {
    const AoeUiTextVtblRor *vt = (const AoeUiTextVtblRor *)g->description->panel.vt;
    int32_t line = aoe_ui_text_selection_ror(&g->list->text);
    int32_t id = aoe_ui_text_selected_id_ror(&g->list->text);
    int32_t at = g->offset + line;
    vt->set_text(g->description, "");
    if (id == -2)
        vt->set_text_id(g->description, 0x25dc);
    else if (id == -1)
        vt->set_text_id(g->description, 0x2618);
    else if (g->descriptions && at >= 0 && at < g->count && g->descriptions[at])
        vt->set_text(g->description, g->descriptions[at]);
    if (line != g->last_line) aoe_ui_dropdown_select_ror(g->victory, 0);
    g->last_line = line;
    g->random_map = id < 0;
}

void aoe_game_settings_screen_read_ror(AoeGameSettingsScreenRor *g) {
    AoeGameSettingsRor *st = g->ui->settings;
    int net = networked(g);
    if (!st) return;
    g->random_map = aoe_settings_flag_8fc_ror(st) ? 0 : 1;
    g->last_line = 0;
    if (g->random_map == 1) {
        aoe_ui_list_move_ror(g->list, 1, aoe_settings_option_ae5_ror(st) ? 1 : 0, 1);
    } else {
        int32_t i = aoe_ui_text_index_of_ror(&g->list->text, aoe_settings_scenario_name_ror(st));
        if (i != -1) {
            aoe_ui_list_move_ror(g->list, 1, (int16_t)i, 1);
            g->last_line = i - 1;
        } else {
            aoe_ui_list_move_ror(g->list, 1, 0, 1);
        }
    }
    select_value(g->size, aoe_settings_map_size_ror(st));
    select_value(g->type, aoe_settings_int_a80_ror(st));
    select_value(g->victory, aoe_settings_int_a8c_ror(st));
    if (g->checks[3])
        aoe_ui_button_set_state_ror(g->checks[3], aoe_settings_byte_982_ror(st));
    aoe_ui_button_set_state_ror(g->checks[1], aoe_settings_byte_985_ror(st));
    aoe_ui_button_set_state_ror(g->checks[0], aoe_settings_option_ad8_ror(st) == 0);
    aoe_ui_button_set_state_ror(g->checks[2], aoe_settings_option_ad9_ror(st));
    select_value(g->resources, aoe_settings_resource_level_ror(st));
    select_value(g->age, aoe_settings_starting_age_ror(st));
    select_value(g->difficulty, aoe_settings_byte_993_ror(st));
    select_value(g->path, net ? aoe_settings_byte_983_ror(st) : aoe_settings_byte_a14_ror(st));
    if (net && g->population) select_value(g->population, aoe_settings_population_limit_ror(st));
    if (aoe_settings_int_a8c_ror(st) == 7)
        select_value(g->years, aoe_settings_int_a90_ror(st));
    else if (aoe_settings_int_a8c_ror(st) == 8)
        select_value(g->score, aoe_settings_int_a90_ror(st));
}

void aoe_game_settings_screen_store_ror(AoeGameSettingsScreenRor *g) {
    AoeGameSettingsRor *st = g->ui->settings;
    int32_t id = aoe_ui_text_selected_id_ror(&g->list->text), v;
    int net = networked(g);
    if (!st) return;
    if (id < 0) {
        aoe_settings_set_flag_8fc_ror(st, 0);
        aoe_settings_set_scenario_name_ror(st, "");
    } else {
        aoe_settings_set_flag_8fc_ror(st, 1);
        aoe_settings_set_scenario_name_ror(st, aoe_ui_text_selected_text_ror(&g->list->text));
    }
    aoe_settings_set_option_ae5_ror(st, id == -1);
    v = aoe_ui_dropdown_selected_id_ror(g->victory);
    if (v == 7)
        aoe_settings_set_a8c_a90_ror(st, 7, aoe_ui_dropdown_selected_id_ror(g->years));
    else if (v == 8)
        aoe_settings_set_a8c_a90_ror(st, 8, aoe_ui_dropdown_selected_id_ror(g->score));
    else
        aoe_settings_set_a8c_a90_ror(st, v, 1);
    if (aoe_settings_flag_8fc_clear_ror(st)) {
        aoe_settings_set_map_size_ror(st, aoe_ui_dropdown_selected_id_ror(g->size));
        aoe_settings_set_int_a80_ror(st, aoe_ui_dropdown_selected_id_ror(g->type));
    }
    if (g->checks[3])
        aoe_settings_set_byte_982_ror(st, (uint8_t)aoe_ui_button_state_ror(g->checks[3]));
    aoe_settings_set_byte_985_ror(st, (uint8_t)aoe_ui_button_state_ror(g->checks[1]));
    aoe_settings_set_option_ad8_ror(st, aoe_ui_button_state_ror(g->checks[0]) == 0);
    aoe_settings_set_option_ad9_ror(st, (uint8_t)aoe_ui_button_state_ror(g->checks[2]));
    aoe_settings_set_resource_level_ror(st, id == -1 ? 0
                                                     : aoe_ui_dropdown_selected_id_ror(g->resources));
    if (net)
        aoe_settings_set_byte_983_ror(st, (uint8_t)aoe_ui_dropdown_selected_id_ror(g->path));
    else
        aoe_settings_set_byte_a14_ror(st, (uint8_t)aoe_ui_dropdown_selected_id_ror(g->path));
    if (net && g->population)
        aoe_settings_set_population_ror(st, (uint8_t)aoe_ui_dropdown_selected_id_ror(g->population));
    aoe_settings_set_starting_age_ror(st, aoe_ui_dropdown_selected_id_ror(g->age));
    aoe_settings_set_byte_993_ror(st, (uint8_t)aoe_ui_dropdown_selected_id_ror(g->difficulty));
    if (aoe_settings_flag_97d_ror(st))
        aoe_settings_set_int_a10_ror(st, aoe_ui_dropdown_selected_id_ror(g->difficulty));
}

static void set_active(void *panel, int on) {
    AoeUiPanelRor *p = panel;
    if (p) p->vt->set_active(p, on);
}

/* 0x0049df10: the victory detail that applies. */
static void show_victory(AoeGameSettingsScreenRor *g) {
    int32_t v;
    set_active(g->years_label, 0);
    set_active(g->years, 0);
    set_active(g->score_label, 0);
    set_active(g->score, 0);
    if (!g->random_map) {
        int32_t line = aoe_ui_text_selection_ror(&g->list->text);
        int32_t at = g->offset + line;
        if (g->filled && line >= 0 && g->flags && at >= 0 && at < g->count && g->flags[at]) {
            set_active(g->scenario_victory, 1);
            set_active(g->victory_label, 0);
            set_active(g->victory, 0);
            return;
        }
    }
    set_active(g->scenario_victory, 0);
    set_active(g->victory_label, 1);
    set_active(g->victory, 1);
    v = aoe_ui_dropdown_selected_id_ror(g->victory);
    if (v == 7) {
        set_active(g->years_label, 1);
        set_active(g->years, 1);
    }
    if (v == 8) {
        set_active(g->score_label, 1);
        set_active(g->score, 1);
    }
}

/* 0x0049dde0: the description or the map settings, the resources, the
   victory and the Tab order. */
static void show_parts(AoeGameSettingsScreenRor *g) {
    AoeUiPanelRor *tab[3], *p = &g->ez.panel;
    int map = g->random_map != 0, n = 0;
    int32_t id;
    set_active(g->description_label, !map);
    set_active(g->description, !map);
    set_active(g->size, map);
    set_active(g->type, map);
    set_active(g->size_label, map);
    set_active(g->type_label, map);
    id = aoe_ui_text_selected_id_ror(&g->list->text);
    set_active(g->resources, id != -1);
    set_active(g->resources_label, id != -1);
    show_victory(g);
    if (networked(g)) tab[n++] = &g->list->text.panel;
    tab[n++] = &g->ok->panel;
    tab[n++] = &g->cancel->panel;
    aoe_ui_panels_tab_cycle_ror(tab, (int16_t)n);
    if (!p->current || !p->current->active) aoe_ui_panel_set_current_ror(p, &g->ok->panel);
}

int aoe_game_settings_screen_init_ror(AoeGameSettingsScreenRor *g, AoeGameUiRor *ui,
                                      AoeUiAreaRor *area) {
    static const int32_t sizes[6] = {0x2973, 0x2974, 0x2975, 0x2976, 0x2977, 0x2978};
    static const char *const scores[14] = {"3000", "2700", "2400", "2100", "1800", "1500", "1200",
                                           "900",  "800",  "700",  "600",  "500",  "400",  "300"};
    static const int32_t score_values[14] = {3000, 2700, 2400, 2100, 1800, 1500, 1200,
                                             900,  800,  700,  600,  500,  400,  300};
    static const int32_t ages[6][2] = {{0x10e7, 0}, {0x106e, 1}, {0x1069, 2},
                                       {0x106a, 3}, {0x106b, 4}, {0x106c, 5}};
    AoeUiEzRor *ez = &g->ez;
    AoeGameSettingsRor *st = ui->settings;
    int net, i;
    memset(g, 0, sizeof *g);
    g->ui = ui;
    g->last_line = -1;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &aoe_game_settings_screen_vtable_ror.base;
    g->setup = aoe_ui_system_find_ror(ui->system, "MP Setup Screen");
    net = networked(g);
    if (!aoe_ui_screen_setup_ror(ez, area, net ? "scr3" : "scr2", net ? 0xc385 : 0xc384, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_ui_ez_shadow_area_ror(ez, 0);
    aoe_ui_ez_set_reference_slot_ror(ez, 0x280, 0x1e0);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &g->title, 0x2616, 0x14, 10, 600, 0x1e, 1,
                                      1, 0, 1))
        return 1;
    if (!label(g, &g->size_label, 0x29a0, 0x14, 0xd8, 0xb4, 0xb) || !dropdown(g, &g->size, 0x14, 0xec))
        return 1;
    for (i = 0; i < 6; ++i) aoe_ui_dropdown_append_id_ror(g->size, sizes[i], i);
    help(g->size, 0x762a, -1);
    if (!label(g, &g->type_label, 0x25db, 0xdc, 0xd8, 0xb4, 0xb) || !dropdown(g, &g->type, 0xdc, 0xec))
        return 1;
    for (i = 0; i < 9; ++i) aoe_ui_dropdown_append_id_ror(g->type, 0x296a + i, i);
    help(g->type, 0x762b, -1);
    if (!label(g, &g->victory_label, 0x2a34, 0x14, 0x108, 0xb4, 0xb) ||
        !dropdown(g, &g->victory, 0x14, 0x11c))
        return 1;
    {
        int scenario = st && aoe_settings_flag_8fc_ror(st);
        if (scenario) aoe_ui_dropdown_append_id_ror(g->victory, 0x10e7, 0);
        aoe_ui_dropdown_append_id_ror(g->victory, 0x10ec, scenario ? 9 : 0);
    }
    aoe_ui_dropdown_append_id_ror(g->victory, 0x10e1, 1);
    aoe_ui_dropdown_append_id_ror(g->victory, 0x10e9, 7);
    aoe_ui_dropdown_append_id_ror(g->victory, 0x10ea, 8);
    help(g->victory, 0x762c, 0x25f0a);
    if (!label(g, &g->years_label, 0x10e9, 0x14, 0x138, 0xb4, 0xb) ||
        !dropdown(g, &g->years, 0x14, 0x14c))
        return 1;
    for (i = 0; i < 7; ++i) aoe_ui_dropdown_append_id_ror(g->years, 0x2634 + i, 15000 - 2000 * i);
    help(g->years, 0x763b, -1);
    if (!label(g, &g->score_label, 0x10ea, 0x14, 0x138, 0xb4, 0xb) ||
        !dropdown(g, &g->score, 0x14, 0x14c))
        return 1;
    for (i = 0; i < 14; ++i) aoe_ui_dropdown_append_ror(g->score, scores[i], score_values[i]);
    help(g->score, 0x763c, -1);
    if (!aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &g->scenario_victory, 0x25d9, 0x14, 0x108,
                                      0xb4, 0x5c, 0xb, 1, 1, 1))
        return 1;
    ((const AoeUiTextVtblRor *)g->scenario_victory->panel.vt)
        ->set_bevel(g->scenario_victory, 3, ez->bevel);
    if (!label(g, &g->age_label, 0x2606, 0xdc, 0x108, 0xb4, 0xb) || !dropdown(g, &g->age, 0xdc, 0x11c))
        return 1;
    for (i = 0; i < 6; ++i) aoe_ui_dropdown_append_id_ror(g->age, ages[i][0], ages[i][1]);
    help(g->age, 0x7631, -1);
    if (!label(g, &g->resources_label, 0x2607, 0xdc, 0x138, 0xb4, 0xb) ||
        !dropdown(g, &g->resources, 0xdc, 0x14c))
        return 1;
    aoe_ui_dropdown_append_id_ror(g->resources, 0x10e7, 0);
    aoe_ui_dropdown_append_id_ror(g->resources, 0x2609, 2);
    aoe_ui_dropdown_append_id_ror(g->resources, 0x260a, 3);
    help(g->resources, 0x7632, -1);
    if (!label(g, &g->path_label, 0x260d, 0x1a4, 0x138, 0xb4, 0xb) ||
        !dropdown(g, &g->path, 0x1a4, 0x14c))
        return 1;
    for (i = 0; i < 3; ++i) aoe_ui_dropdown_append_id_ror(g->path, 0x260e + i, i);
    help(g->path, 0x763e, -1);
    if (net) {
        if (!label(g, &g->population_label, 0x2613, 0x1a4, 0x168, 0xb4, 0xb) ||
            !dropdown(g, &g->population, 0x1a4, 0x17c))
            return 1;
        for (i = 0x19; i < 0xc9; i += 0x19) {
            char text[8];
            snprintf(text, sizeof text, "%d", i);
            aoe_ui_dropdown_append_ror(g->population, text, i);
        }
        help(g->population, 0x763f, -1);
    }
    if (!label(g, &g->difficulty_label, 0x2bcf, 0x1a4, 0x108, 0xb4, 0xb) ||
        !dropdown(g, &g->difficulty, 0x1a4, 0x11c))
        return 1;
    for (i = 4; i >= 0; --i) aoe_ui_dropdown_append_id_ror(g->difficulty, 0x2bd0 + i, i);
    help(g->difficulty, 0x763d, -1);
    {
        static const int32_t cx[3] = {10, 0x1ae, 0xdc}, tx[3] = {0x28, 0x1cc, 0xfa};
        static const int32_t texts[3] = {0x2601, 0x25fc, 0x2604};
        static const int32_t helps[3] = {0x7633, 0x7634, 0x7637};
        static const int order[3] = {0, 2, 1}; /* made in the original's order */
        int k;
        for (k = 0; k < 3; ++k) {
            int c = order[k];
            if (!aoe_ui_ez_create_check_ror(ez, &ez->panel, &g->checks[c], cx[c], 0x19a, 0x1e,
                                            0x1e, 0, 0) ||
                !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &g->check_texts[c], texts[c],
                                              tx[c], 0x19a, 0xaa, 0x1e, 0xb, 0, 1, 0))
                return 1;
            help(g->check_texts[c], helps[c], -1);
        }
    }
    if (net) {
        if (!aoe_ui_ez_create_check_ror(ez, &ez->panel, &g->checks[3], 10, 0x177, 0x1e, 0x1e, 0,
                                        0) ||
            !aoe_ui_ez_create_text_id_ror(ez, &ez->panel, &g->check_texts[3], 0x25fb, 0x28, 0x177,
                                          0xa5, 0x1e, 0xb, 0, 1, 0))
            return 1;
        help(g->check_texts[3], 0x7638, -1);
    }
    g->list_x = 0x14;
    g->list_y = 0x3c;
    g->list_w = 600;
    g->list_h = net ? 0x6f : 0x18;
    if (!aoe_ui_ez_create_list_ror(ez, &ez->panel, &g->list, g->list_x, g->list_y, g->list_w,
                                   g->list_h, 0xb))
        return 1;
    g->list->text.column_x = (g->list_w - 0x3c) * ez->panel.width / 0x280;
    g->list->text.panel.vt->set_redraw(&g->list->text.panel, 1);
    if (!net) {
        g->list->text.panel.handles_mouse = 0;
        aoe_ui_list_set_highlight_ror(g->list, 0);
        aoe_ui_text_set_spacing_ror(&g->list->text, 0);
    } else {
        ((const AoeUiTextVtblRor *)g->list->text.panel.vt)->set_text_id(&g->list->text, 0x25fd);
        help(&g->list->text, 0x7639, -1);
        if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &g->list_bar, &g->list->text, 0x14)) return 1;
    }
    if (!label(g, &g->list_label, net ? 0x25fe : 0x2620, g->list_x, g->list_y - 0x14, 300, 4) ||
        !label(g, &g->players_label, 0x25ff, g->list_w - 0xdc + g->list_x, g->list_y - 0x14, 0xdc,
               4))
        return 1;
    aoe_ui_text_set_alignment_ror(g->players_label, 3, 2);
    g->desc_y = g->list_h + 0x18 + g->list_y;
    g->desc_w = g->list_w;
    g->desc_x = g->list_x;
    g->desc_h = 0x104 - g->desc_y;
    if (!label(g, &g->description_label, 0x2600, g->desc_x, g->desc_y - 0x14, 300, 4) ||
        !aoe_ui_ez_create_text_ror(ez, &ez->panel, &g->description, "", g->desc_x, g->desc_y,
                                   g->desc_w, g->desc_h, 0xb, 0, 0, 1))
        return 1;
    ((const AoeUiTextVtblRor *)g->description->panel.vt)->set_bevel(g->description, 3, ez->bevel);
    if (!aoe_ui_ez_create_text_scrollbar_ror(ez, &g->description_bar, g->description, 0x14))
        return 1;
    help(g->description_bar, 0x763a, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &g->ok, 0xfa1, 0, 0x3c, 0x1bd, 0xf0, 0x1e,
                                        0, 0, 0))
        return 1;
    help(g->ok, 0x7531, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &g->cancel, 0xfa2, 0, 0x140, 0x1bd, 0xf0,
                                        0x1e, 0, 0, 0))
        return 1;
    g->cancel->hotkey = 0x1b;
    g->cancel->hotkey_shift = 0;
    help(g->cancel, 0x7532, -1);
    if (!aoe_ui_ez_create_button_id_ror(ez, &ez->panel, &g->help, 0xfa9, 0, 0x244, 0x1bd, 0x1e,
                                        0x1e, 0, 0, 0))
        return 1;
    aoe_game_settings_screen_fill_ror(g);
    g->filled = 1;
    aoe_game_settings_screen_read_ror(g);
    show_description(g);
    show_parts(g);
    aoe_ui_panel_set_current_ror(&ez->panel, &g->ok->panel);
    return 1;
}

static int32_t gs_idle(AoeUiPanelRor *p) {
    AoeGameSettingsScreenRor *g = GS(p);
    const AoeGameUiOpsRor *ops = g->ui->ops;
    if (!g->filled) {
        aoe_game_settings_screen_fill_ror(g);
        show_description(g);
        g->filled = 1;
        show_parts(g);
        aoe_ui_panel_set_current_ror(p, &g->list->text.panel);
    }
    if (ops && ops->arrow_cursor) ops->arrow_cursor(ops->context);
    if (networked(g) && ops && ops->now_ms) {
        uint32_t now = ops->now_ms(ops->context);
        if (g->last_ms == 0) {
            g->last_ms = now;
        } else if (now != g->last_ms) {
            if (ops->share_player_data) ops->share_player_data(ops->context);
            g->last_ms = ops->now_ms(ops->context);
        }
    }
    return aoe_ui_panel_handle_idle_ror(p);
}

/* 0x0049d320 (+0x64, user messages): the setup screen's first. */
static int32_t gs_user(AoeUiPanelRor *p, uint32_t a, uint32_t b) {
    AoeGameSettingsScreenRor *g = GS(p);
    if (g->setup && g->setup->vt->user(g->setup, a, b)) return 1;
    return aoe_ui_panel_ignore_ror(p, a, b);
}

static int32_t gs_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t value,
                         int32_t extra) {
    AoeGameSettingsScreenRor *g = GS(p);
    if (!from || !g->filled) return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
    if (from == &g->ok->panel && code == 1) {
        aoe_ui_panel_set_current_ror(p, &g->list->text.panel);
        aoe_game_settings_screen_store_ror(g);
        if (g->setup) g->setup->vt->action(g->setup, p, 1, 0, 0);
        return 1;
    }
    if (from == &g->cancel->panel && code == 1) {
        aoe_ui_panel_set_current_ror(p, &g->list->text.panel);
        aoe_game_settings_screen_read_ror(g);
        if (g->setup) g->setup->vt->action(g->setup, p, 0, 0, 0);
        return 1;
    }
    if (from == &g->help->panel && code == 1) {
        aoe_ui_ez_help_mode_on_ror(&g->ez);
        return 1;
    }
    if (g->victory && from == &g->victory->panel && code == 0) {
        int32_t v = aoe_ui_dropdown_selected_id_ror(g->victory);
        if (v == 7) aoe_ui_dropdown_select_ror(g->years, 3);
        if (v == 8) aoe_ui_dropdown_select_ror(g->score, 7);
        show_parts(g);
        return 1;
    }
    if (from == &g->list->text.panel) {
        if (code == 3) return 1;
        if (code == 1) {
            show_description(g);
            show_parts(g);
            return 1;
        }
    }
    return aoe_ui_ez_action_slot_ror(p, from, code, value, extra);
}

const AoeUiEzVtblRor aoe_game_settings_screen_vtable_ror = {
    AOE_UI_EZ_SLOTS_MDU(gs_destroy, aoe_ui_ez_draw_slot_ror, gs_idle,
                        aoe_ui_panel_handle_paint_ror, aoe_ui_ez_no_key_action_ror,
                        aoe_ui_panel_zero_ror, gs_action, aoe_ui_screen_focus_slot_ror,
                        aoe_ui_ez_mouse_down_slot_ror, gs_user),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};
