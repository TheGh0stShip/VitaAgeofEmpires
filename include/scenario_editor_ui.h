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

/* The editor's tool state on the game object and its lists. */
typedef struct {
    int16_t place_unit;   /* game +0x3fc (-1 none) */
    int16_t terrain;      /* game +0x3fe */
    int16_t elevation;    /* game +0x400 */
    int16_t brush;        /* game +0x402 */
    int32_t map_tool;     /* editor +0x4a8 */
    int32_t unit_tool;    /* editor +0x4ac */
    int32_t brush_size;   /* editor +0x4b0 */
    int32_t terrain_tool; /* editor +0x4b4 */
} AoeScenarioEditorToolsRor;

typedef struct {
    void *context;
    /* 0x004650c0(1, index, 1) on a list: 0 terrain (+0x5a0), 1 elevation
       (+0x5a8), 2 units (+0x63c). */
    void (*list_select)(void *context, int list, int32_t index);
    /* 0x0045f280 on a radio button: 0 a map tool (+0x504[i]), 1 a brush
       size (+0x554[i]), 2 a unit tool (+0x6dc[i]), 3 a terrain tool
       (+0x580[i]). */
    void (*radio_select)(void *context, int group, int32_t index);
    /* The page's controls of a tool shown (1) or hidden (0) through
       +0x14; the last one shown gets the focus (0x00453eb0). */
    void (*show_tool)(void *context, int page, int32_t tool, int32_t show);
    void (*redraw)(void *context); /* +0x4d4 +0x20(1) */
    /* The game's +0x10 action. */
    void (*action)(void *context, int32_t action, int32_t arg);
    /* The unit list's template IDs (+0x644: 0x66-byte entries, ID +0x64). */
    int16_t (*unit_entry)(void *context, int32_t index);
    /* The current player's template (world +0x40[+0x4b8] +0x24[id]):
       present, of type 80 ('P') with its +0x16c flag. */
    int (*placeable_building)(void *context, int16_t id);
    int32_t game_6e; /* game +0x1b8, the default action's argument */
    /* A list's selected line (0x0046a0d0): 0 terrain (+0x5a0), 1
       elevation (+0x5a8). */
    int32_t (*list_line)(void *context, int list);
} AoeScenarioEditorToolOpsRor;

/* 0x004923f0: a brush size 1..5 (selected; 0 ignored) sets the brush
   width 1, 3, 5, 7, 9 (anything else 2). 0x004926b0: the terrain list
   picks terrain 6, 10, 1, 13, 20, 4, 19, 22 for lines 1..8 (else 0).
   0x00492750: the elevation line. 0x00492fc0: the unit line (-1: none)
   becomes the unit to place. 0x004920b0: the cancel: a placeable
   building being placed gives action 0x15, else action 8 with game
   +0x1b8. 0x00492110 / 0x00492260: the map / unit page's tool changes:
   the old tool's controls are hidden and (when `show`) the new tool's
   shown, the new tool kept (0 keeps nothing else) and its radio button
   selected, then the screen redrawn. */
void aoe_scenario_editor_brush_ror(AoeScenarioEditorToolsRor *t, int32_t size,
                                   const AoeScenarioEditorToolOpsRor *ops);
void aoe_scenario_editor_terrain_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                     const AoeScenarioEditorToolOpsRor *ops);
void aoe_scenario_editor_elevation_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                       const AoeScenarioEditorToolOpsRor *ops);
void aoe_scenario_editor_unit_ror(AoeScenarioEditorToolsRor *t, int16_t line,
                                  const AoeScenarioEditorToolOpsRor *ops);
void aoe_scenario_editor_cancel_ror(const AoeScenarioEditorToolsRor *t,
                                    const AoeScenarioEditorToolOpsRor *ops);
void aoe_scenario_editor_map_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool, int32_t show,
                                      const AoeScenarioEditorToolOpsRor *ops);
void aoe_scenario_editor_unit_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool, int32_t show,
                                       const AoeScenarioEditorToolOpsRor *ops);

/* 0x004924c0: the terrain page's tool (1 terrain, 2 elevation, 3
   cliffs): the old one's controls hidden (with the game back to action
   0), then (when `show`) the new one's shown, its list line picked again
   and the game in action 9 / 10 / 0x13; the brush controls only for
   tools 1 and 2. Then as the other pages. */
void aoe_scenario_editor_terrain_tool_ror(AoeScenarioEditorToolsRor *t, int32_t tool,
                                          int32_t show, const AoeScenarioEditorToolOpsRor *ops);
/* 0x00490cb0: the terrain list's nine names (strings 10621..10629);
   8. 0x00490d50: a hidden list (font 10) with its scrollbar: the
   elevation list ("%s %d" of string 10631 for 1..7) or the terrain list.
   0x00490230: a button labelled from a string ID. */
int32_t aoe_scenario_editor_terrain_lines_ror(AoeUiTextRor *list);
int aoe_scenario_editor_make_list_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                      AoeUiListRor **out, AoeUiScrollbarRor **bar, int elevation,
                                      int terrain);
int aoe_scenario_editor_button_id_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                      AoeUiButtonRor **out, int32_t text_id);

#endif
