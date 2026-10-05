#include "scenario_editor_ui.h"

#include <stdio.h>
#include <string.h>

static void front_hidden(AoeUiPanelRor *p) {
    aoe_ui_panel_set_z_ror(p, 1, 0);
    aoe_ui_panel_set_active_ror(p, 0);
}

int aoe_scenario_editor_button_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                   AoeUiButtonRor **out, const char *text, const char *text1,
                                   const char *text2, const char *text3) {
    if (!aoe_ui_ez_create_button_ror(e->ez, parent, out, text, text1, 0, 0, 0, 0, 10, 1, 0))
        return 0;
    if (text2) aoe_ui_button_set_text_ror(*out, 2, text2);
    if (text3) aoe_ui_button_set_text_ror(*out, 3, text3);
    front_hidden(&(*out)->panel);
    if (e->bevel_on) aoe_ui_button_set_bevel_ror(*out, 3, e->bevel);
    return 1;
}

int aoe_scenario_editor_text_id_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                    AoeUiTextRor **out, int32_t text_id) {
    if (!aoe_ui_ez_create_text_id_ror(e->ez, parent, out, text_id, 0, 0, 0, 0, 10, 0, 0, 0))
        return 0;
    front_hidden(&(*out)->panel);
    return 1;
}

int aoe_scenario_editor_text_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                 AoeUiTextRor **out, const char *text) {
    if (!aoe_ui_ez_create_text_ror(e->ez, parent, out, text, 0, 0, 0, 0, 10, 0, 0, 0)) return 0;
    front_hidden(&(*out)->panel);
    return 1;
}

int aoe_scenario_editor_edit_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                 AoeUiEditRor **out, const char *text, int16_t max,
                                 int32_t format, int32_t ime_enabled, int32_t ime_open) {
    if (!aoe_ui_ez_create_edit_ror(e->ez, parent, out, text, max, format, 0, 0, 0, 0, 10,
                                   ime_enabled, ime_open))
        return 0;
    front_hidden(&(*out)->panel);
    return 1;
}

int aoe_scenario_editor_string_ror(AoeScenarioEditorUiRor *e, char *out, int32_t id, int size) {
    return aoe_ui_string_ror(e->ez->panel.system, id, out, size);
}

void aoe_scenario_editor_brush_ror(AoeScenarioEditorToolsRor *t, int32_t size,
                                   const AoeScenarioEditorToolOpsRor *ops) {
    if (size == 0) return;
    t->brush_size = size;
    ops->radio_select(ops->context, 1, size);
    t->brush = (int16_t)(size >= 1 && size <= 5 ? size * 2 - 1 : 2);
}

void aoe_scenario_editor_terrain_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                     const AoeScenarioEditorToolOpsRor *ops) {
    static const int16_t terrain[9] = {0, 6, 10, 1, 13, 20, 4, 19, 22};
    t->terrain = (line >= 0 && line <= 8) ? terrain[line] : 0;
    ops->list_select(ops->context, 0, line);
}

void aoe_scenario_editor_elevation_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                       const AoeScenarioEditorToolOpsRor *ops) {
    t->elevation = line;
    ops->list_select(ops->context, 1, line);
}

void aoe_scenario_editor_unit_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                  const AoeScenarioEditorToolOpsRor *ops) {
    t->place_unit = line == -1 ? -1 : ops->unit_entry(ops->context, line);
    ops->list_select(ops->context, 2, line);
}

void aoe_scenario_editor_cancel_ror(const AoeScenarioEditorToolsRor *t,
                                    const AoeScenarioEditorToolOpsRor *ops) {
    if (t->place_unit != -1 && ops->placeable_building(ops->context, t->place_unit))
        ops->action(ops->context, 0x15, 0);
    else
        ops->action(ops->context, 8, ops->game_6e);
}

static void switch_tool(int32_t *current, int page, int32_t tool, int32_t show,
                        const AoeScenarioEditorToolOpsRor *ops) {
    if (*current) ops->show_tool(ops->context, page, *current, 0);
    if (tool == 0) return; /* the original returns without keeping it */
    if (show) ops->show_tool(ops->context, page, tool, 1);
    *current = tool;
    ops->radio_select(ops->context, page == 0 ? 0 : 2, tool);
    ops->redraw(ops->context);
}

void aoe_scenario_editor_map_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool, int32_t show,
                                      const AoeScenarioEditorToolOpsRor *ops) {
    switch_tool(&t->map_tool, 0, tool, show, ops);
}

void aoe_scenario_editor_unit_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool, int32_t show,
                                       const AoeScenarioEditorToolOpsRor *ops) {
    switch_tool(&t->unit_tool, 1, tool, show, ops);
}

void aoe_scenario_editor_terrain_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool,
                                          int32_t show, const AoeScenarioEditorToolOpsRor *ops) {
    static const int32_t actions[4] = {0, 9, 10, 0x13};
    int pass;
    for (pass = 0; pass < 2; ++pass) {
        int32_t which, on;
        if (pass == 0) {
            which = t->terrain_tool;
            on = 0;
        } else {
            if (tool == 0) return;
            if (!show) break;
            which = tool;
            on = 1;
        }
        if (which < 1 || which > 3) continue;
        ops->show_tool(ops->context, 2, which, on);
        if (!on) {
            ops->action(ops->context, 0, 0);
            continue;
        }
        if (which == 1)
            aoe_scenario_editor_terrain_ror(t, (int16_t)ops->list_line(ops->context, 0), ops);
        else
            aoe_scenario_editor_elevation_ror(t, (int16_t)ops->list_line(ops->context, 1), ops);
        ops->action(ops->context, actions[which], 0);
    }
    t->terrain_tool = tool;
    ops->radio_select(ops->context, 3, tool);
    ops->redraw(ops->context);
}

int32_t aoe_scenario_editor_terrain_lines_ror(AoeUiTextRor *list) {
    int32_t i;
    aoe_ui_text_clear_ror(list);
    for (i = 0; i < 9; ++i) aoe_ui_text_append_id_ror(list, 0x297d + i, 0);
    return 8;
}

int aoe_scenario_editor_make_list_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                      AoeUiListRor **out, AoeUiScrollbarRor **bar, int elevation,
                                      int terrain) {
    if (!aoe_ui_ez_create_list_ror(e->ez, parent, out, 0, 0, 0, 0, 10)) return 0;
    if (!*out) return 0; /* host guard: the maker reports success either way */
    (*out)->text.panel.vt->set_active(&(*out)->text.panel, 0);
    aoe_ui_text_clear_ror(&(*out)->text);
    if (!aoe_ui_ez_create_text_scrollbar_ror(e->ez, bar, &(*out)->text, 0x14)) return 0;
    if (elevation) {
        char base[0x32], line[0x40];
        int32_t i;
        aoe_scenario_editor_string_ror(e, base, 0x2987, sizeof base);
        for (i = 1; i <= 7; ++i) {
            snprintf(line, sizeof line, "%s %d", base, (int)i);
            aoe_ui_text_append_ror(&(*out)->text, line, 0);
        }
        return 1;
    }
    if (terrain) aoe_scenario_editor_terrain_lines_ror(&(*out)->text);
    return 1;
}

int aoe_scenario_editor_button_id_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                      AoeUiButtonRor **out, int32_t text_id) {
    char text[0x78];
    aoe_scenario_editor_string_ror(e, text, text_id, sizeof text);
    return aoe_scenario_editor_button_ror(e, parent, out, text, NULL, NULL, NULL);
}

static int bevelled(AoeScenarioEditorUiRor *e, AoeUiButtonRor *b) {
    front_hidden(&b->panel);
    if (e->bevel_on) aoe_ui_button_set_bevel_ror(b, 3, e->bevel);
    return 1;
}

int aoe_scenario_editor_check_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                  AoeUiButtonRor **out) {
    if (!aoe_ui_ez_create_check_ror(e->ez, parent, out, 0, 0, 0, 0, 1, 0) || !*out) return 0;
    return bevelled(e, *out);
}

int aoe_scenario_editor_radio_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                  AoeUiButtonRor **out) {
    if (!aoe_ui_ez_create_radio_ror(e->ez, parent, out, 0, 0, 0, 0, 1, 0) || !*out) return 0;
    return bevelled(e, *out);
}

void aoe_scenario_editor_ages_ror(AoeUiDropdownRor *ages, AoeUiPanelRor *panel_5bc) {
    int32_t i;
    aoe_ui_dropdown_clear_ror(ages);
    for (i = 0; i < 5; ++i) aoe_ui_dropdown_append_id_ror(ages, 0x1069 + i, i);
    aoe_ui_dropdown_select_ror(ages, 0);
    if (panel_5bc) panel_5bc->vt->set_active(panel_5bc, 0);
}

typedef struct {
    AoeUiDropdownRor *d;
    size_t cut;
    int32_t count;
} FileChoice;

static void add_file(void *user, const char *name) {
    FileChoice *f = user;
    char base[260];
    size_t n = strlen(name);
    n = n >= f->cut ? n - f->cut : 0;
    if (n >= sizeof base) n = sizeof base - 1;
    memcpy(base, name, n);
    base[n] = '\0';
    aoe_ui_dropdown_append_ror(f->d, base, 0);
    ++f->count;
}

int32_t aoe_scenario_editor_file_choice_ror(
    AoeUiDropdownRor *d, const char *dir, const char *ext, int none, int other,
    void (*list_files)(void *context, const char *pattern,
                       void (*found)(void *user, const char *name), void *user),
    void *context) {
    FileChoice f;
    char pattern[0x104];
    aoe_ui_dropdown_clear_ror(d);
    if (none) aoe_ui_dropdown_append_id_ror(d, 0x2775, 0);
    if (other) aoe_ui_dropdown_append_id_ror(d, 0x277b, 0);
    f.d = d;
    f.cut = strlen(ext);
    f.count = 0;
    snprintf(pattern, sizeof pattern, "%s*%s", dir, ext);
    if (list_files) list_files(context, pattern, add_file, &f);
    return f.count;
}

void aoe_scenario_editor_player_page_ror(AoeUiButtonRor *buttons[12], AoeUiPanelRor *others[3],
                                         AoeUiDropdownRor *players, int16_t *current,
                                         int32_t show, int (*player_on)(void *context, int32_t i),
                                         void *context) {
    int32_t i;
    for (i = 0; i < 12; ++i) {
        buttons[i]->panel.vt->set_active(&buttons[i]->panel, show);
        if (show)
            aoe_ui_button_set_state_ror(
                buttons[i], (int16_t)((player_on(context, i) ? 2 : 0) + (*current == i)));
    }
    for (i = 0; i < 3; ++i) others[i]->vt->set_active(others[i], show);
    if (show)
        aoe_ui_dropdown_select_ror(players, (int16_t)(*current - 1));
    else
        *current = (int16_t)(aoe_ui_dropdown_selection_ror(players) + 1);
}
