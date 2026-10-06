#ifndef VITA_AOE_SCENARIO_EDITOR_UI_H
#define VITA_AOE_SCENARIO_EDITOR_UI_H

#include <stdint.h>

#include "ui_dropdown.h"
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

/* 0x0049a4d0 / 0x0049a560: a hidden check box / radio button at the
   front (sound 1) with the screen's bevel when on. */
int aoe_scenario_editor_check_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                  AoeUiButtonRor **out);
int aoe_scenario_editor_radio_ror(AoeScenarioEditorUiRor *e, AoeUiPanelRor *parent,
                                  AoeUiButtonRor **out);
/* 0x0049a2c0: the age choice (strings 4201..4205 as 0..4, the first
   chosen) and the +0x5bc panel hidden. */
void aoe_scenario_editor_ages_ror(AoeUiDropdownRor *ages, AoeUiPanelRor *panel_5bc);
/* 0x0049a1a0: a drop-down of the files "<dir>*<ext>" (the extension
   cut), after "None" (10101) and / or string 10107 when asked; the count
   of files. */
int32_t aoe_scenario_editor_file_choice_ror(
    AoeUiDropdownRor *d, const char *dir, const char *ext, int none, int other,
    void (*list_files)(void *context, const char *pattern,
                       void (*found)(void *user, const char *name), void *user),
    void *context);
/* 0x0049a0e0: the player page shown or hidden: its twelve buttons (each
   showing 2 when that scenario player's type (0x005075f0) is set, plus
   1 for the current
   one), the three other controls, and the player choice following the
   current player (+0x4b8, 1-based) or setting it. */
void aoe_scenario_editor_player_page_ror(AoeUiButtonRor *buttons[12], AoeUiPanelRor *others[3],
                                         AoeUiDropdownRor *players, int16_t *current,
                                         int32_t show, int (*player_on)(void *context, int32_t i),
                                         void *context);

/* Victory condition page controls (editor +0x738..+0x76c). */
enum {
    AOE_EDITOR_COND_CLASS = 1u << 0,        /* +0x738 */
    AOE_EDITOR_COND_PLAYER = 1u << 1,       /* +0x740 */
    AOE_EDITOR_COND_SUBTYPE = 1u << 2,      /* +0x744 */
    AOE_EDITOR_COND_TECH = 1u << 3,         /* +0x748 */
    AOE_EDITOR_COND_RELIC = 1u << 4,        /* +0x74c */
    AOE_EDITOR_COND_UNIT = 1u << 5,         /* +0x750 */
    AOE_EDITOR_COND_TARGET = 1u << 6,       /* +0x754 */
    AOE_EDITOR_COND_AREA = 1u << 7,         /* +0x758 */
    AOE_EDITOR_COND_AMOUNT_LABEL = 1u << 8, /* +0x764 */
    AOE_EDITOR_COND_PLAYER_LABEL = 1u << 9, /* +0x768 */
    AOE_EDITOR_COND_AMOUNT = 1u << 10       /* +0x76c */
};

typedef struct {
    uint32_t shown;    /* controls made visible (+0x14(show)) */
    int32_t help_id;   /* amount edit help (0x004542c0(id, -1)); 0 none */
    uint32_t focus;    /* one control bit; 0 clears the focus only */
    int tab_count;     /* 2 sets the tab order (+0xcc) player, amount */
} AoeEditorConditionPlanRor;

/* 0x00496560(kind, show): clears editor +0x4a0. With show 0 every
   condition control above is hidden (no focus change). Otherwise the
   focus is first cleared (0x00453eb0(NULL)) and kind picks:
   1 unit+target; 2 unit+target+area; 3 class+amount (help 0x7766);
   4 those + target+area (0x7767); 5 class+amount+player (0x7768, tab
   order); 6/9 unit; 7 class+player; 8 player; 10..14 amount (0x7769..
   0x776d); 15 tech; 16 amount (0x776e); 17 amount+subtype (0x776f);
   18 relic. The amount, else player, edit gets the focus where shown,
   except kind 17 which focuses the amount before showing the subtype.
   Returns 0 for show 0 or an unknown kind (focus cleared only). */
int aoe_editor_condition_plan_ror(int32_t kind, int32_t show,
                                  AoeEditorConditionPlanRor *plan);


/* The editor screen's destructor 0x00490e30 (deleting 0x00490190). Every
   owned child is a pointer at a fixed offset of the 0x92c-byte screen;
   the port keeps the original release order as a table of those
   offsets so a host that stores the panels elsewhere can map them. */
typedef struct {
    void *context;
    /* 0x00451df0 by name: "QuitSaveDialog", "OpenSaveDialog",
       "NewSaveDialog", "Save Game Screen", "Scenario Menu Dialog". */
    void (*close_dialog)(void *context, const char *name);
    /* 0x00419950 on the panel pointer at `offset`: deleted and cleared. */
    void (*delete_panel)(void *context, int32_t offset);
    /* +0x644: a buffer freed (0x005260f8); +0x660: an object deleted
       through its vtable; +0x648..+0x65c and +0x4c4: shapes freed
       (0x0049f840). */
    void (*free_buffer)(void *context, int32_t offset);
    void (*delete_object)(void *context, int32_t offset);
    void (*free_shape)(void *context, int32_t offset);
    /* +0x928 (the map view, when set): its vtable +0xb8 called. */
    void (*view_notify)(void *context);
    /* The screen's 0x00468170. */
    void (*base_release)(void *context);
} AoeScenarioEditorReleaseOpsRor;
void aoe_scenario_editor_screen_release_ror(const AoeScenarioEditorReleaseOpsRor *ops);
/* The number of panel offsets the release visits (for tests). */
int32_t aoe_scenario_editor_screen_panel_count_ror(void);

/* 0x004901b0: the editor's map view (+0x928) takes the world
   (0x0050fee0) and the viewed player (0x0050ffa0; 0x00419b70), then the
   diamond map does the same (0x0042c780, 0x0042cbb0). */
typedef struct {
    void *context;
    void (*view_set_map)(void *context, void *world);
    void (*view_set_player)(void *context, void *player);
    void (*minimap_set_world)(void *context, void *world);
    void (*minimap_set_player)(void *context, void *player);
    void *(*viewed_player)(void *context);
} AoeScenarioEditorAttachOpsRor;
void aoe_scenario_editor_attach_world_ror(void *world, const AoeScenarioEditorAttachOpsRor *ops);

/* 0x00493110: the editor screen's key handler. Nothing happens while a
   modal is up (0x00451ad0). With Alt held: 'A' flips the world's grid
   byte (+0x50) between 0 and 3; 'B' bumps the 0x007d200c counter, wrapping
   to 0 at 2 (or 5 when 0x0041b7f0 is set) and redraws the map view; the
   page letters press their tab button (vtable +0xe4) and open the page
   (0x00491650). Without Alt: Tab focuses the player list (page 3) or the
   victory list (page 6, if it has items) when no edit box (+0x3c) owns
   input; Esc clears the selection (0x00494a60); '1'..'8' pick a player on
   the per-page dropdown (0x00461c70, then vtable +0xb8 on the panel),
   page 8 also storing and reloading the player page (0x004953f0,
   0x00492780, 0x00495470); page 4 takes '0'..'8'. */
typedef enum {
    AOE_EDKEY_NONE = 0, AOE_EDKEY_GRID, AOE_EDKEY_CYCLE, AOE_EDKEY_PAGE,
    AOE_EDKEY_FOCUS_PLAYERS, AOE_EDKEY_FOCUS_VICTORY, AOE_EDKEY_CLEAR,
    AOE_EDKEY_DROPDOWN, AOE_EDKEY_PLAYER
} AoeEditorKeyActionRor;
typedef struct {
    int32_t page;          /* +0x4a4 */
    int32_t edit_focused;  /* the +0x4d4 control's +0x3c */
    int32_t victory_items; /* +0x76c list +0x6c */
    uint8_t grid;          /* world +0x50 */
    int32_t cycle;         /* 0x007d200c */
    int32_t cycle_wide;    /* 0x0041b7f0 */
} AoeEditorKeyStateRor;
typedef struct {
    AoeEditorKeyActionRor action;
    int32_t page;          /* PAGE: 1..10 */
    int32_t dropdown_index;/* DROPDOWN: index selected */
    int32_t panel_offset;  /* DROPDOWN: the panel told via +0xb8 */
    int32_t notify_arg;    /* DROPDOWN: its first argument */
    int32_t player;        /* PLAYER / page-8 DROPDOWN: player switched to */
} AoeEditorKeyResultRor;
int aoe_scenario_editor_key_ror(AoeEditorKeyStateRor *st, int32_t key, int32_t alt,
                                int32_t modal, AoeEditorKeyResultRor *out);

/* 0x00493060: the editor's resize: the base set-rect (0x004533f0), the
   panel layout (0x499160), then the base set-rect again. */
typedef struct {
    void *context;
    void (*base_set_rect)(void *context, int32_t w, int32_t h);
    void (*layout)(void *context);
} AoeScenarioEditorResizeOpsRor;
int aoe_scenario_editor_resize_ror(int32_t w, int32_t h, const AoeScenarioEditorResizeOpsRor *ops);


/* 0x00494b60: a new scenario in the editor. Without a world one is made
   (status 0x44d first when `with_status`; a failure closes the status
   and returns 0); the editor takes the game's world (+0x928); status
   0x44e (`kind` 0) or 0x44f; the request: 8 players named by strings
   0x2865..0x286c (0x00490210), civilisation 0, team numbers cycling 1..
   (the world's +0x44 - 1), -1 in the +0x570 slots and slot indices
   0..8, `kind`, the two sizes, flag 0; the game's +0x60 busy byte set
   around the world's generate (+0xd4 with `arg`); fewer than 8 players
   afterwards fails and clears the world (+0xb8). On success with the
   editor's +0x5b8 list the first inactive scenario slot i (0x00476250)
   in 1..7 selects count line i - 1 (0x00461d50) and 0x0049a350 runs. */
typedef struct {
    void *context;
    int (*has_world)(void *context);
    int (*ensure_world)(void *context);
    void (*status)(void *context, int32_t text_id);
    void (*status_close)(void *context);
    void (*player_name)(void *context, int32_t string_id, char *out, int size); /* 0x00490210 */
    int16_t (*max_team)(void *context);                   /* world +0x44 */
    void (*set_busy)(void *context, int on);              /* game +0x60's byte +1 */
    int (*generate)(void *context, int32_t kind, const char names[8][0x41], const uint8_t civs[8],
                    const uint8_t teams[8], const int16_t slots_570[8], const int32_t order[9],
                    int16_t size_a, int16_t size_b, int32_t arg);
    int16_t (*world_player_count)(void *context);         /* world +0x3c */
    void (*clear_world)(void *context);                   /* world +0xb8 */
    int has_player_list;                                  /* +0x5b8 */
    int (*slot_active)(void *context, int32_t slot);      /* 0x00476250 */
    void (*select_count)(void *context, int32_t line);    /* 0x00461d50 */
    void (*player_count_changed)(void *context);          /* 0x0049a350 */
} AoeScenarioEditorNewOpsRor;
int aoe_scenario_editor_new_scenario_ror(const AoeScenarioEditorNewOpsRor *o, int32_t kind,
                                         int32_t arg, int16_t size_a, int16_t size_b,
                                         int with_status);


/* 0x00496a40: the editor screen's control pointers cleared before they
   are built: +0x47c, +0x4c4..+0x5bc, +0x5c4..+0x610, +0x618..+0x660,
   the 16-bit +0x664, +0x668..+0x850 and +0x894 (+0x5c0, +0x614 and
   +0x854..+0x890 are left alone). */
typedef struct {
    void *ptr_47c;
    void *group_4c4[(0x5c0 - 0x4c4) / 4];
    void *kept_5c0;
    void *group_5c4[(0x614 - 0x5c4) / 4];
    void *kept_614;
    void *group_618[(0x664 - 0x618) / 4];
    int16_t field_664, pad_666;
    void *group_668[(0x854 - 0x668) / 4];
    void *kept_854[(0x894 - 0x854) / 4];
    void *ptr_894;
} AoeScenarioEditorControlsRor;
void aoe_scenario_editor_clear_controls_ror(AoeScenarioEditorControlsRor *c);


/* 0x00490450 (parent, &control): a drop-down list for the editor control
   at `offset` (the pointer's offset in the screen). Width 0xdc for
   +0x738, +0x744, +0x748, +0x74c, +0x600, +0x604, +0x608, +0x700; 0xb4
   for +0x734; 0xaa for +0x7b4..+0x7c0, +0x528, +0x530, +0x538; 0x96 for
   +0x5b4..+0x5bc, +0x618, +0x73c, +0x7cc, +0x894, +0x740; else 0x82.
   Created (0x00456bf0(parent, &control, w, 100, 0, 0, w, 0x16, 10); 0
   when that fails), brought to the front (0x00453f40(1, 0)), hidden
   (+0x14(0)) and cleared (0x00461f10). Filled by string IDs
   (0x00461dd0(id, value)): +0x530 map sizes 0x2973..0x2978 and +0x538
   map types 0x296a..0x2972 (both selecting 3); +0x5b8 / +0x740 players
   0x2877 + 0..7; +0x744 (0x2882, 0x2b), (0x2883, 0x29), (0x2884, 0x2c),
   (0x2886, 0x28), (0x2887, 0x15), (0x2888, 0x14), (0x2889, 0x25); +0x528
   0x297d, 0x297e, 0x2980, 0x2985; +0x5f8 0x27e3, 0x27e4; +0x734
   0x2851..0x2862 then 0x284f; +0x5fc 0x27f7..0x2802, 0x2806, 0x2807,
   0x2809, 0x2808; +0x748 0x1069..0x106c; +0x74c sorted (0x00461d30(1))
   with each world scenario's name (+0x104 list, string 0x004542a0 of the
   entry's +0x2a; empty ones skipped) and index; +0x738 sorted and
   cleared with player 1's unit templates (+0x22 count, +0x24 array;
   byte +0x4e clear): the string of +0xc when positive else the +0x8
   name, and the index. File lists (0x0049a1a0): +0x608 the settings
   +0x102b directory with ".per" (0, 1); +0x600 ".ai" (1, 1); +0x604
   ".cty" (1, 0); +0x7b4..+0x7bc the +0x1130 directory with ".avi" (1,
   0); +0x7c0 "" with ".bmp" (1, 0). Known controls then select
   (0x00461d50) their index (3 or 0). Returns whether the control's +0xd8
   is clear. */
typedef struct {
    void *context;
    int (*create)(void *context, int32_t width);          /* 0x00456bf0 */
    void (*to_front)(void *context);                      /* 0x00453f40(1, 0) */
    void (*hide)(void *context);                          /* +0x14(0) */
    void (*clear)(void *context);                         /* 0x00461f10 */
    void (*add_id)(void *context, int32_t id, int32_t value);        /* 0x00461dd0 */
    void (*add_text)(void *context, const char *text, int32_t value); /* 0x00461da0 */
    void (*sorted)(void *context, int32_t flag);          /* 0x00461d30 */
    void (*files)(void *context, int dir_1130, const char *ext, int32_t a, int32_t b); /* 0x0049a1a0 */
    int32_t scenario_count;
    const char *(*scenario_name)(void *context, int32_t i); /* NULL / "" skipped */
    int32_t template_count;
    /* NULL for a missing template or one with byte +0x4e set. */
    const char *(*template_name)(void *context, int32_t i, char *buf, int size);
    void (*select)(void *context, int32_t index);         /* 0x00461d50 */
    int failed;                                           /* control +0xd8 */
} AoeScenarioEditorDropdownOpsRor;
int aoe_scenario_editor_open_dropdown_ror(const AoeScenarioEditorDropdownOpsRor *o, int32_t offset);

#endif
