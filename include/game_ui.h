#ifndef VITA_AOE_GAME_UI_H
#define VITA_AOE_GAME_UI_H

#include <stdint.h>

#include "game_base.h"
#include "game_settings.h"
#include "ui_dialog.h"
#include "ui_ez.h"
#include "ui_picture.h"
#include "palette_fade.h"

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
    /* The product ID from the registry ("PID"), NULL when absent; the
       credits screen. */
    const char *(*product_id)(void *context);
    void (*open_credits)(void *context);
    /* The viewed player's game state byte (+0x80: 0 playing). */
    uint8_t (*player_state)(void *context, void *player);
    /* The game object: the CD check (nonzero when the game CD is in),
       resetting the pointer and mouse capture, closing the main window,
       and playing the recorded demo (nonzero when it started). */
    int (*cd_check)(void *context);
    void (*reset_input)(void *context);
    void (*quit_program)(void *context);
    int (*play_demo)(void *context);
    /* The menu screens each opens (made and registered). */
    void (*open_single_player_menu)(void *context);
    void (*open_mp_startup)(void *context);
    void (*open_editor_menu)(void *context);
    /* A whole file (heap, free()) or NULL; the CD music: the current
       track (0 when not playing) and playing tracks from..to. */
    uint8_t *(*read_file)(void *context, const char *path, uint32_t *size);
    int32_t (*music_track)(void *context);
    void (*play_music)(void *context, int32_t from, int32_t to);
} AoeGameUiOpsRor;

typedef struct {
    AoeUiSystemRor *system;
    const AoeUiEzOpsRor *ez_ops;
    AoeGameBaseRor *game;
    AoeGameSettingsRor *settings;
    const AoeGameBaseOps *game_ops;
    const AoeGameUiOpsRor *ops;
    const AoeUiPictureOpsRor *picture_ops;
    int32_t quit_requested;          /* global 0x007c0338 */
    int32_t cd_ok;                   /* game +0x9ac */
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

/* "About Dialog" (vtable 0x005435ac, 0x0042f7f0..0x0042ff80): 520 x 472
   inside a bevelled frame: the title lines (strings 9241, 9255), the
   version line (9242 with "a" appended), the copyright line (9243 with
   the circle-p picture and 9244 beside it, or one line when 9252 has
   text), lines 9245 and 9251, "Product ID:" and the registry's product
   ID ("00000-000-0000000-00000" without one), the warning text (9247),
   9254, and Credits / Close (Escape) buttons. Close resumes a paused
   single-player game and goes back to the game screen when there is
   one; Credits opens the credits screen. */
typedef struct {
    AoeUiDialogRor dialog;
    AoeUiTextRor *title, *title2, *version;   /* +0x490, +0x494, +0x498 */
    AoeUiTextRor *copyright, *copyright2;     /* +0x49c, +0x4a0 */
    AoeUiTextRor *developer, *engine;         /* +0x4a4, +0x4a8 */
    AoeUiTextRor *pid_label, *pid;            /* +0x4ac, +0x4b0 */
    AoeUiTextRor *warning, *frame, *notice;   /* +0x4b4, +0x4b8, +0x4bc */
    AoeUiButtonRor *credits, *close;          /* +0x4c0, +0x4c4 */
    AoeUiPictureRor *logo;                    /* +0x4c8 */
    AoeGameUiRor *ui;
} AoeAboutDialogRor;

extern const AoeUiEzVtblRor aoe_about_dialog_vtable_ror;

/* 0x0042f7f0 constructor (with its setup); 0x0042fe60 destructor
   (deleting 0x0042fe40); +0xb8 0x0042ff80. */
int aoe_about_dialog_init_ror(AoeAboutDialogRor *d, AoeGameUiRor *ui,
                              AoeUiPanelRor *parent);

/* The main menu "Main Menu" (vtable 0x005469f8, 0x00486600..0x00486d00):
   a screen ("scr1", 50051) with five 300 x 40 buttons at x 170 from y 178
   every 50: Single Player, Multiplayer, Help, Scenario Builder, Exit
   (strings 9202, 9203, 9205, 9206, 9207), then at the bottom the title
   line (9241), the copyright line (9243, a "©" picture "circlep2" 50406
   after it, then 9244, centred together; or one line of 9243 + 9252 + " "
   + 9244 when string 9252 is not empty) and the warning (9253). */
typedef struct {
    AoeUiEzRor ez;
    AoeUiPanelRor *field_478, *field_47c;  /* +0x478, +0x47c */
    AoeUiButtonRor *buttons[7];            /* +0x480 */
    AoeUiTextRor *title;                   /* +0x49c */
    AoeUiTextRor *copyright;               /* +0x4a0 */
    AoeUiTextRor *year;                    /* +0x4a4 */
    AoeUiTextRor *warning;                 /* +0x4a8 */
    AoeUiPictureRor *circle;               /* +0x4ac */
    AoeUiPanelRor *field_4b0, *field_4b4;  /* +0x4b0, +0x4b4 */
    AoeGameUiRor *ui;
} AoeMainMenuRor;

extern const AoeUiEzVtblRor aoe_main_menu_vtable_ror;

/* 0x00486600 constructor; 0x00486b20 destructor (deleting 0x00486b00).
   +0xac 0x00486c10 keys: C the credits, L the recorded demo (an OK
   "Could not load game data." 2007 when it cannot start). +0xb8
   0x00486d00: Single Player and Scenario Builder need the CD once (else
   strings 2008 / 2010), reset the input and open their menus; Multiplayer
   resets the input and opens the multiplayer start screen; Help shows
   help string 120055; Exit closes the program. */
int aoe_main_menu_init_ror(AoeMainMenuRor *m, AoeGameUiRor *ui,
                           AoeUiAreaRor *area, const AoeUiPictureOpsRor *picture_ops);

/* "Main Error Screen" (vtable 0x00546b24, 0x00486fd0..0x004871d0): the
   message (font 4, centred, wrapped) in 560 x 400 at (40, 40), OK (4001)
   240 x 30 at (200, 440) and a top-right X (1002); either closes the
   program. 0x00487140 destructor (deleting 0x00487120), 0x004871b0 sets
   the message from a string ID, +0xb8 0x004871d0 the buttons. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *message;                 /* +0x478 */
    AoeUiButtonRor *ok, *close;            /* +0x47c, +0x480 */
    AoeGameUiRor *ui;
} AoeErrorScreenRor;

extern const AoeUiEzVtblRor aoe_error_screen_vtable_ror;

int aoe_error_screen_init_ror(AoeErrorScreenRor *e, AoeGameUiRor *ui,
                              AoeUiAreaRor *area);
void aoe_error_screen_set_message_ror(AoeErrorScreenRor *e, int32_t string_id);

/* The credits' scrolling text (vtable 0x00546594, 0x0047cc40..0x0047cf00):
   a text panel whose lines rise `speed` pixels a second from its bottom.
   0x0047cc40 constructor and setup (centred, wrapped, the text);
   0x0047cd40 destructor (deleting 0x0047cd20); 0x0047cd50 the text (the
   scroll restarts). +0x4c 0x0047cd80: scroll by elapsed time; once the
   last line has risen above the top the parent hears action 1. +0x30
   0x0047ce20 draws every line (0x0047cf00) at its scrolled height. */
typedef struct {
    AoeUiTextRor text;
    int32_t offset;            /* +0x188 */
    uint32_t last_ms;          /* +0x18c */
    int32_t speed;             /* +0x190: pixels a second */
    int32_t done;              /* +0x194 */
} AoeCreditsTextRor;

extern const AoeUiTextVtblRor aoe_credits_text_vtable_ror;

int aoe_credits_text_init_ror(AoeCreditsTextRor *c, AoeUiSystemRor *system,
                              AoeUiAreaRor *area, AoeUiPanelRor *parent,
                              int32_t x, int32_t y, int32_t w, int32_t h,
                              int32_t speed, uintptr_t font, int32_t font_w,
                              int32_t font_h, const char *text);
void aoe_credits_text_set_ror(AoeCreditsTextRor *c, const char *text);

/* The credits "Credits Screen" (vtable 0x00546468, 0x0047c510..
   0x0047cb80): a screen ("scr_cred", 50059) with the scrolling credits
   (font 4, speed 30, 500 x 300 in the middle) of "data2\list.cr" and,
   behind it, the background and six pictures ("credit2".."credit7",
   50155..50160) taking turns every ten seconds, faded out and in over
   half a second on a full-screen or memory display. While the music plays
   track 15 it is played on entry and 5..14 again on leaving.
   0x0047c510 constructor; 0x0047c790 destructor (deleting 0x0047c770);
   +0x4c 0x0047c870 the picture turns; +0x30 0x0047caa0 draw (the screen
   black while switching; the first time each picture is drawn once);
   +0xac 0x0047cb20 Enter or Escape, +0x70 0x0047c860 a press, and the
   text's end (+0xb8 0x0047cb40) all leave: 0x0047cb80 back to the game
   screen (resuming a single-player game) or a new main menu. */
typedef struct {
    AoeUiEzRor ez;
    AoeCreditsTextRor *text;   /* +0x478 */
    int32_t field_47c;         /* +0x47c, 1 */
    int32_t picture;           /* +0x480, 1..7 */
    uint32_t shown_ms;         /* +0x484 */
    uint32_t switch_ms;        /* +0x488 */
    int32_t state;             /* +0x48c: 1 showing, 2 out, 4 switching, 5 in */
    uint8_t palette[1024];     /* +0x490 */
    uint32_t fade_color;       /* +0x890 */
    void *pictures[7];         /* +0x894 */
    int32_t preload;           /* +0x8b0 */
    int32_t leave;             /* +0x8b4 */
    AoePaletteFadeRor fade;    /* the fade's globals 0x007bfafc.. */
    AoeGameUiRor *ui;
} AoeCreditsScreenRor;

extern const AoeUiEzVtblRor aoe_credits_screen_vtable_ror;

int aoe_credits_screen_init_ror(AoeCreditsScreenRor *c, AoeGameUiRor *ui,
                                AoeUiAreaRor *area);
void aoe_credits_screen_leave_ror(AoeCreditsScreenRor *c);

#endif
