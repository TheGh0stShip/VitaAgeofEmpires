#include "scenario_editor_ui.h"

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
