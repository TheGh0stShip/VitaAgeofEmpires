#ifndef VITA_AOE_SCENARIO_EDITOR_UI_H
#define VITA_AOE_SCENARIO_EDITOR_UI_H

#include <stdint.h>

#include "ui_ez.h"

/* Patched Rise of Rome 1.0a scenario editor screen (vtable 0x00547360,
   0x0048fc40..0x0049f5a0): the control makers its pages share. Each
   makes the control at (0, 0, 0, 0) in font 10 (the page lays it out
   later), brings it to the front (0x00453f40(1, 0)) and hides it
   (+0x14(0)). The editor's +0x31c..+0x325 bevel settings. */
typedef struct {
    AoeUiEzRor *ez;            /* the screen itself */
    int32_t bevel_on;          /* +0x31c */
    uint8_t bevel[6];          /* +0x320..+0x325 */
} AoeScenarioEditorUiRor;

/* 0x00490280: a button with sound 1 and command 0; `text2` / `text3`,
   when given, label states 2 and 3; the screen's bevel (type 3) when on.
   0x00490350 / 0x004903a0: a text from a string ID / a string.
   0x004903f0: an edit field with the text, limit, format and the two IME
   settings. Each returns 0 when the control could not be made. */
int aoe_scenario_editor_button_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                   AoeUiButtonRor **out, const char *text, const char *text1,
                                   const char *text2, const char *text3);
int aoe_scenario_editor_text_id_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                    AoeUiTextRor **out, int32_t text_id);
int aoe_scenario_editor_text_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                 AoeUiTextRor **out, const char *text);
int aoe_scenario_editor_edit_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                 AoeUiEditRor **out, const char *text, int16_t max,
                                 int32_t format, int32_t ime_enabled, int32_t ime_open);
/* 0x00490210: a string from the table with the first two arguments
   swapped (the id, then the buffer and size). */
int aoe_scenario_editor_string_ror(AoeScenarioEditorUiRor *e, char *out, int32_t id,
                                   int size);

#endif
