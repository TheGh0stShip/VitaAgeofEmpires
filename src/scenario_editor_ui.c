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
