#ifndef VITA_AOE_GAME_UI_H
#define VITA_AOE_GAME_UI_H

#include <stdint.h>

#include "game_base.h"
#include "game_settings.h"
#include "ui_dialog.h"
#include "ui_ez.h"

/* The game-side context shared by the Patched Rise of Rome 1.0a screens
   and in-game dialogs: the panel system (0x005830e8), the game object
   (0x00580e38) pieces they read, and callbacks for screens that are not
   translated yet. */

typedef struct {
    void *context;
    /* Screens and dialogs opened from others. */
    void (*open_save_screen)(void *context, int32_t mode);
    void (*open_game_settings)(void *context, AoeUiPanelRor *parent);
    void (*open_achievements)(void *context, const char *title);
    void (*open_about)(void *context, AoeUiPanelRor *parent);
    void (*open_instructions)(void *context);
    /* Scenario editor screen methods (on the editor panel). */
    void (*editor_quit)(void *context, AoeUiPanelRor *editor);
    void (*editor_save)(void *context, AoeUiPanelRor *editor, int32_t a,
                        int32_t b);
    void (*editor_save_as)(void *context, AoeUiPanelRor *editor, int32_t a);
    void (*editor_edit)(void *context, AoeUiPanelRor *editor);
    void (*editor_create)(void *context, AoeUiPanelRor *editor);
    /* The scenario to test (0 when there is none), and testing it. */
    void *(*editor_test_scenario)(void *context, AoeUiPanelRor *editor);
    void (*test_scenario)(void *context, void *scenario);
    /* Save the scenario in the editor before leaving it. */
    void (*editor_save_current)(void *context, AoeUiPanelRor *editor);
    /* The game's new-scenario start (0, 0). */
    void (*new_scenario)(void *context);
    void (*open_editor_open)(void *context);
    /* Chat: the game's command check on a typed line (vtable +0x54, 0
       for ordinary text), the game sound table's sound `n` (played
       unless already playing), the game screen showing a line, and
       sending one to the other players. */
    int (*chat_command)(void *context, int16_t player, const char *text);
    void (*play_sound)(void *context, int32_t index);
    void (*show_chat)(void *context, const char *text);
    void (*send_chat)(void *context, const char *text);
    /* The viewed player's game state byte (+0x80: 0 playing). */
    uint8_t (*player_state)(void *context, void *player);
} AoeGameUiOpsRor;

typedef struct {
    AoeUiSystemRor *system;
    const AoeUiEzOpsRor *ez_ops;
    AoeGameBaseRor *game;
    AoeGameSettingsRor *settings;
    const AoeGameBaseOps *game_ops;
    const AoeGameUiOpsRor *ops;
    int32_t quit_requested;          /* global 0x007c0338 */
} AoeGameUiRor;

/* The in-game menu "Menu Dialog" (vtable 0x00543bb8,
   0x00433c30..0x00434010): 400 wide, 300 high, 335 in a multiplayer game
   while the viewed player still plays, 105 more in single player. One
   column of 360 x 30 buttons at x 20: Quit Game (command 4); Resign (12,
   that multiplayer case); Achievements (7); Scenario Instructions (9); in
   single player Save (1), Load (3) and Restart (11); Game Settings (5);
   Help (8); About (10); Cancel (6, Escape). */
typedef struct {
    AoeUiDialogRor dialog;
    AoeUiButtonRor *buttons[11];     /* +0x490.. */
    int32_t count;
    AoeGameUiRor *ui;
} AoeMenuDialogRor;

extern const AoeUiEzVtblRor aoe_menu_dialog_vtable_ror;

/* 0x00433c30 constructor (with its setup); 0x00433fa0 destructor
   (deleting 0x00433f80). +0xb8 0x00434010: the commands. Save closes the
   menu after opening the save screen; Load, Quit Game, Restart and Resign
   show the game screen and ask yes/no (strings 3100, 3101, 3102, 3103 as
   "QuitAndLoadDialog", "QuitGameDialog", "RestartDialog",
   "ResignDialog", 450 x 100; Quit also raises the quit request); Help
   shows the game screen and its help; Cancel and Help resume a paused
   single-player game. Port choice: the original opens the question on
   the menu after closing it, so its answers reach the menu's parent; the
   port opens it on that parent (the game screen). */
int aoe_menu_dialog_init_ror(AoeMenuDialogRor *m, AoeGameUiRor *ui,
                             AoeUiPanelRor *parent);

/* The scenario editor's menu "Scenario Menu Dialog" (vtable 0x00543e20,
   0x004349a0..0x00434c60): 460 x 400 over the editor, seven 400 x 40
   buttons at x 30: Quit to Main Menu, Save, Save As, Edit Scenario,
   Create Scenario, Test, Cancel (Escape). Each shows the editor screen
   and closes the menu, then runs the editor's command; Test only when
   there is a scenario to test. The editor's "New Dialog" and "Open
   Dialog" questions answer here too: Yes saves first, Yes and No go on
   (a new scenario, or the scenario open screen), Cancel just closes. */
typedef struct {
    AoeUiDialogRor dialog;
    AoeUiPanelRor *editor;           /* +0x490 */
    void *field_494;                 /* +0x494 */
    AoeUiButtonRor *buttons[7];      /* +0x498..+0x4b0 */
    AoeGameUiRor *ui;
} AoeScenarioMenuDialogRor;

extern const AoeUiEzVtblRor aoe_scenario_menu_dialog_vtable_ror;

/* 0x004349a0 constructor (with its setup); 0x00434bc0 destructor
   (deleting 0x00434ba0) also closes "Open Dialog" and "New Dialog". +0xb8
   0x00434c60: the commands. */
int aoe_scenario_menu_dialog_init_ror(AoeScenarioMenuDialogRor *m,
                                      AoeGameUiRor *ui, AoeUiPanelRor *editor);

/* A typed chat line (the tail shared by 0x00435750 and 0x00435e60): a
   game command is handled by the game; otherwise single player plays
   sound 2 and shows the line, and multiplayer sends it unless the game
   is in mode 5. */
void aoe_game_ui_chat_ror(AoeGameUiRor *ui, const char *text);

/* The quick chat line "Send Quick Message Dialog" (vtable 0x00544088,
   0x00435cd0..0x00435fd0): 540 x 35, a label ("Chat", or "Comment" for a
   comment line) and a 65-character edit box with the IME on. Enter sends
   a non-empty line; Escape, an empty Enter or a press outside closes it
   and shows the game screen. */
typedef struct {
    AoeUiDialogRor dialog;
    AoeUiEditRor *edit;              /* +0x490 */
    AoeUiTextRor *label;             /* +0x494 */
    int32_t comment;                 /* +0x498 */
    AoeGameUiRor *ui;
} AoeQuickChatDialogRor;

extern const AoeUiEzVtblRor aoe_quick_chat_dialog_vtable_ror;

/* 0x00435cd0 constructor (with its setup); 0x00435df0 destructor
   (deleting 0x00435dd0); +0xb8 0x00435e60; +0x70 0x00435fd0. */
int aoe_quick_chat_dialog_init_ror(AoeQuickChatDialogRor *d,
                                   AoeGameUiRor *ui, AoeUiPanelRor *parent,
                                   int32_t comment);

#endif
