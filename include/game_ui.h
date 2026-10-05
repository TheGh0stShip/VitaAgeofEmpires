#ifndef VITA_AOE_GAME_UI_H
#define VITA_AOE_GAME_UI_H

#include <stdint.h>

#include "game_base.h"
#include "campaign_progress.h"
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
    /* Files: each name matching a wildcard pattern (FindFirst / Next),
       whether a file exists (OpenFile OF_EXIST), delete one (remove). The
       scenario directory (game info +0xc17). */
    void (*list_files)(void *context, const char *pattern,
                       void (*found)(void *user, const char *name), void *user);
    int (*file_exists)(void *context, const char *path);
    void (*delete_file)(void *context, const char *path);
    const char *(*scenario_dir)(void *context);
    /* 0x00504e50: the editor on a scenario file (0: a new one). */
    void (*editor_load)(void *context, const char *file, int32_t flag);
    /* The wait cursor (0x0041bd60); the screens made and registered:
       "Scenario Editor Open" (0x0049aaa0), "Campaign Editor Screen"
       (0x00479360), "Main Menu" (0x00486600). */
    void (*wait_cursor)(void *context);
    void (*open_editor_open_screen)(void *context);
    void (*open_campaign_editor)(void *context);
    void (*open_main_menu)(void *context);
    /* A networked game (0x0041b760). */
    int (*networked)(void *context);
    /* Screens the single player menu makes: "MP Setup Screen"
       (0x00487f40), "Select Scenario Screen" (0x0049b360), "Load Saved
       Game Screen" (0x00485c90), "Name Selection Screen" (0x0048dec0) and
       the new-name dialog over a screen (0x0048e800); the campaign file's
       player count (0x0043cd00). */
    void (*open_mp_setup)(void *context);
    void (*open_select_scenario)(void *context);
    void (*open_load_saved)(void *context);
    void (*open_name_selection)(void *context);
    void (*open_new_name)(void *context, AoeUiPanelRor *parent);
    int32_t (*campaign_players)(void *context);
    /* The world's scenario (+0x5c): its title (0x00475b30) and message i
       (0x00476c70; NULL when none). The game start (0x00503a70); back in
       a running game, resuming when it is paused by the settings
       (0x0041b750 == 1, game +0xa00 clear: 0x00419a60(0, 0)). A picture
       drawn at a point of an area (0x0045b370). */
    const char *(*scenario_title)(void *context);
    const char *(*scenario_message)(void *context, int32_t i);
    void (*start_game)(void *context);
    void (*resume_game)(void *context);
    void (*draw_picture)(void *context, void *picture, AoeUiAreaRor *area, int32_t x, int32_t y);
    /* The scenario index (0x00476f10 refresh; entry i's file name, NULL
       past the end, 0x004772e0; the record for an ID, 0x00477310: its
       description, flag and player count; 0 when none). The release of
       the wait cursor unless the game's +0x1d8 (0x0041bdc0). */
    void (*scenario_index_refresh)(void *context);
    const char *(*scenario_index_name)(void *context, int32_t i);
    int (*scenario_index_info)(void *context, int32_t id, const char **description,
                               int32_t *flag, int32_t *players);
    void (*arrow_cursor)(void *context);
    /* The host shares its player data (0x00420640(0)); the time. */
    void (*share_player_data)(void *context);
    uint32_t (*now_ms)(void *context);
    /* The game's state (+0x1b0); the viewed player's selection and block
       cleared (0x0045dcb0, 0x0045e250); the scenario's player i active
       (0x00475ad0). */
    int32_t (*game_state)(void *context);
    void (*clear_selection)(void *context);
    void (*set_scenario_player)(void *context, int32_t player, int32_t active);
    /* Saving: the saved-game directory (game info +0xb12), the first
       saved-game name (0x005001a0), the status screen with a string
       (0x00500390), a game save (0x00501250) and a scenario save
       (0x00501380), nonzero on success; the editor's scenario saved
       under a file name (0x00475800 then the world's +0xe4; nonzero on
       success) and its modified flag cleared; the main menu instead of
       the game (0x00500c60; 0 when it could not be shown). */
    const char *(*game_dir)(void *context);
    const char *(*default_save_name)(void *context);
    void (*show_status)(void *context, int32_t string_id);
    int (*save_game)(void *context, const char *file);
    int (*save_scenario)(void *context, const char *file);
    int (*editor_save_file)(void *context, const char *file);
    void (*editor_saved)(void *context);
    int (*to_main_menu)(void *context);
    /* The editor: its pages stored into the scenario (0x00491650(11)
       then 0x00507800(1)); the scenario's active and human player counts
       (0x00476a90 / 0x00476ac0); its file name (0x00475b30 / 0x00475800);
       the registry's "Game File Number" (0x0046af50); the status screen
       closed (0x005003e0); the save screen opened for a save as (mode 2,
       an empty name, `after`); a player's tech toggle i (0x00507680 /
       0x00507380). */
    void (*editor_commit)(void *context);
    void (*scenario_players)(void *context, int32_t *active, int32_t *human);
    const char *(*scenario_file)(void *context);
    void (*set_scenario_file)(void *context, const char *file);
    int32_t (*game_file_number)(void *context);
    void (*close_status)(void *context);
    void (*open_editor_save_as)(void *context, int32_t after);
    int32_t (*scenario_tech)(void *context, int32_t slot, int32_t i);
    void (*set_scenario_tech)(void *context, int32_t slot, int32_t i, int32_t value);
    /* Campaigns: the titles (0x0043cd70) and the chosen campaign's
       scenario names (0x0043cdf0), each a malloc'd array of malloc'd
       names (the caller frees both; the count returned, 0 for none) with
       the chosen entry's index; choosing a campaign by ID (0x0043cca0)
       and a scenario by line (0x0043ccd0); the chosen scenario's slots
       (0x00417670(0, 1) then its 0x00476250 / 0x00476210 for slots
       0..8; 0 when it cannot be read); a comm slot's state (0x00423d40);
       the scenario game started (0x00502910(0), nonzero on success). */
    int32_t (*campaign_titles)(void *context, char ***names, int32_t *chosen);
    int32_t (*campaign_scenarios)(void *context, char ***names, int32_t *chosen);
    void (*choose_campaign)(void *context, int32_t id);
    void (*choose_scenario)(void *context, int32_t line);
    int (*campaign_scenario_slots)(void *context, int32_t active[9], int32_t kind[9]);
    void (*set_slot_state)(void *context, int32_t slot, int32_t state);
    int (*start_scenario_game)(void *context);
    /* The campaign files (campaign and scenario directories, listing,
       reading and writing); the game's campaign index reloaded
       (0x0041b9f0); whether a file can be opened for writing (the
       original's open(0x8301) probe). */
    const AoeCampaignOpsRor *(*campaign_file_ops)(void *context);
    void (*refresh_campaigns)(void *context);
    int (*file_writable)(void *context, const char *path);
    /* The player file: the player names with the chosen one's ID
       (0x0043cd00; as for the campaign titles), choosing (0x0043cc80),
       adding (0x0043cc00, then chosen) and removing (0x0043cef0) a player
       by ID; the "Campaign Selection Screen" made and registered
       (0x0047a7c0). */
    int32_t (*campaign_player_names)(void *context, char ***names, int32_t *chosen);
    void (*choose_campaign_player)(void *context, int32_t id);
    void (*add_campaign_player)(void *context, const char *name);
    void (*remove_campaign_player)(void *context, int32_t id);
    void (*open_campaign_selection)(void *context);
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

/* The editor's "Scenario Editor Open" screen (vtable 0x005475b8,
   0x0049aaa0..0x0049af70): the scenarios in the scenario directory
   (.scn, then .scx, without the extension, sorted), Open, Delete (with a
   yes/no "Cancel Dialog") and Cancel (Escape). */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;              /* +0x478 */
    AoeUiListRor *list;               /* +0x47c */
    AoeUiScrollbarRor *scrollbar;     /* +0x480 */
    AoeUiButtonRor *open;             /* +0x484 */
    AoeUiButtonRor *cancel;           /* +0x488 */
    AoeUiButtonRor *del;              /* +0x48c */
    AoeGameUiRor *ui;
} AoeScenarioEditorOpenRor;

extern const AoeUiEzVtblRor aoe_scenario_editor_open_vtable_ror;

/* 0x0049aaa0 constructor (the current screen's info name and ID; title
   9421 at 20, 20, 600 x 30 in font 1; the list 600 x 340 at 20, 70 in
   font 11 with its 20-wide scrollbar; Open (4001), Delete (9417, the
   Delete key) and Cancel (4002, Escape) 180 x 30 at x 30, 230, 430 and
   y 440; both Open and Delete disabled with nothing to pick; the list
   has the focus and the four cycle by Tab). 0x0049ae10 fills the list;
   0x0049ad70 destructor (deleting 0x0049ad50). +0xb8 0x0049af70: the
   delete confirmation (9418 with the name; yes deletes both files),
   Open or a double-click (the .scn, else the .scx, else the message
   9423), Cancel (back to the editor screen, else the editor menu). */
int aoe_scenario_editor_open_init_ror(AoeScenarioEditorOpenRor *o, AoeGameUiRor *ui,
                                      AoeUiAreaRor *area, const char *info_name,
                                      int32_t info_id);
void aoe_scenario_editor_open_fill_ror(AoeScenarioEditorOpenRor *o);

/* The editor menu "Scenario Editor Menu" (vtable 0x0054748c,
   0x0049a5f0..0x0049a8a0): title 9261 and four 300 x 40 buttons at x 170
   from y 170 every 50: create a scenario (9262), edit one (9263), the
   campaign editor (9265), cancel (9264, Escape); and the hidden close
   button (string 1002). 0x0049a5f0 constructor ("scr6", 50054);
   0x0049a810 destructor (deleting 0x0049a7f0). +0xb8 0x0049a8a0: a new
   scenario (0x00504e50(0, 0)), the open screen, the campaign editor, the
   main menu (with the wait cursor), or the close button closes the
   program. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;              /* +0x478 */
    AoeUiButtonRor *buttons[4];       /* +0x47c..+0x488 */
    AoeUiButtonRor *close;            /* +0x48c */
    AoeGameUiRor *ui;
} AoeScenarioEditorMenuRor;

extern const AoeUiEzVtblRor aoe_scenario_editor_menu_vtable_ror;
int aoe_scenario_editor_menu_init_ror(AoeScenarioEditorMenuRor *m, AoeGameUiRor *ui,
                                      AoeUiAreaRor *area);

/* The status screen (vtable 0x00547a68, 0x0049e7a0..0x0049eb80): a
   named screen showing one message (font 4, centred, wrapped, 560 x 400
   at 40, 40), from a string (0x0049e7a0) or a string ID (0x0049e9a0).
   Its background: the given info name and ID, else the current screen's
   (unless that is the "Achievements Screen"), else "scr2" / 50052 (or
   "scr3" / 50053 in a networked game). 0x0049eb80 destructor (deleting
   0x0049e980). */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *text;               /* +0x478 */
    AoeGameUiRor *ui;
} AoeStatusScreenRor;

extern const AoeUiEzVtblRor aoe_status_screen_vtable_ror;
int aoe_status_screen_init_ror(AoeStatusScreenRor *st, AoeGameUiRor *ui, AoeUiAreaRor *area,
                               const char *name, const char *text, int32_t text_id,
                               const char *info_name, int32_t info_id);
/* 0x0049ebe0: a new message. */
void aoe_status_screen_set_text_ror(AoeStatusScreenRor *st, const char *text);

/* The single player menu "Single Player Menu" (vtable 0x0054793c,
   0x0049e040..0x0049e330): title 9220 and six 300 x 40 buttons at x 170
   from y 118 every 50: random map (9226), campaign (9224), death match
   (9227), scenario (9221), saved game (9223) and cancel (9225, Escape),
   and the hidden close button. 0x0049e040 constructor ("scr2", 50052;
   the settings' +0x97d set and its scenario name cleared); 0x0049e2a0
   destructor (deleting 0x0049e280). +0xb8 0x0049e330: the game setup
   (+0x8fc, +0x9a0, +0x9a4, +0xae5 and +0x1190 cleared; +0xae5 set for
   death match; +0x8fc set for a scenario, which goes to the scenario
   choice instead), the saved games, the campaign (the name choice when
   the campaign file has players, else the new name dialog over this
   screen), the main menu, or the program closes; each with the wait
   cursor. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;              /* +0x478 */
    AoeUiButtonRor *buttons[6];       /* +0x47c..+0x490 */
    AoeUiButtonRor *close;            /* +0x498 */
    AoeGameUiRor *ui;
} AoeSinglePlayerMenuRor;

extern const AoeUiEzVtblRor aoe_single_player_menu_vtable_ror;
int aoe_single_player_menu_init_ror(AoeSinglePlayerMenuRor *m, AoeGameUiRor *ui,
                                    AoeUiAreaRor *area);

/* The scenario's "Mission Dialog" (vtable 0x00547b94,
   0x0049ec00..0x0049f460): the scenario title (else string 9813), the
   message text with its scrollbar and three buttons switching it between
   the instructions (message 0, 10802), the hints (4, 10801) and the
   history (1, 10807), kept to the messages there are (no buttons with
   only one, no text with none), OK, and a picture. Laid out for 640,
   800 and 1024 wide screens. 0x0049ec00 constructor ("scr9", 50060;
   `in_game` +0x494, the picture +0x498); 0x0049f2c0 destructor
   (deleting 0x0049f2a0): before the game a playing sound is stopped.
   +0x30 0x0049f3a0: the picture drawn below the title. +0xb8 0x0049f460:
   a button shows its message; OK starts the game (before it) or goes
   back to the game screen and resumes it. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;              /* +0x478 */
    AoeUiTextRor *text;               /* +0x47c */
    AoeUiScrollbarRor *scrollbar;     /* +0x480 */
    AoeUiButtonRor *ok;               /* +0x484 */
    AoeUiButtonRor *buttons[3];       /* +0x488: hints, instructions, history */
    uint8_t in_game;                  /* +0x494 */
    void *picture;                    /* +0x498 */
    AoeGameUiRor *ui;
} AoeMissionDialogRor;

extern const AoeUiEzVtblRor aoe_mission_dialog_vtable_ror;
int aoe_mission_dialog_init_ror(AoeMissionDialogRor *d, AoeGameUiRor *ui, AoeUiAreaRor *area,
                                const char *text, uint8_t in_game, void *picture);

/* The "Select Scenario Screen" (vtable 0x005476e4,
   0x0049b360..0x0049bed0): the scenarios of the index (sorted, with
   their player counts as a second column, "2 - n" for a scenario not
   flagged with more than two players), the chosen one's description,
   OK, Cancel (Escape) and the hidden close button. 0x0049b360
   constructor ("scr2" / 50052, "scr3" / 50053 networked); 0x0049b860
   destructor (deleting 0x0049b7d0; 0x0049b7f0 clears the fields first);
   0x0049bbc0 fills the list;
   0x0049be30 picks the settings' scenario; 0x0049bde0 shows the chosen
   one's description; 0x0049bed0 shows the labels and orders Tab;
   0x0049be90 puts the choice in the settings. +0x4c 0x0049b950 the idle
   (filled once; the arrow cursor). +0xb8 0x0049b9b0: OK (the settings,
   then the game setup), Cancel (the single player menu), close, a
   double-click (OK) or a pick. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;              /* +0x478 */
    AoeUiTextRor *name_label;         /* +0x47c */
    AoeUiTextRor *players_label;      /* +0x480 */
    int32_t list_x, list_y, list_w, list_h; /* +0x484..+0x490 */
    AoeUiListRor *list;               /* +0x494 */
    AoeUiScrollbarRor *list_bar;      /* +0x498 */
    int32_t count;                    /* +0x49c */
    char **descriptions;              /* +0x4a0 */
    int32_t *flags;                   /* +0x4a4 */
    int32_t filled;                   /* +0x4a8 */
    AoeUiTextRor *description_label;  /* +0x4ac */
    int32_t desc_x, desc_y, desc_w, desc_h; /* +0x4b0..+0x4bc */
    AoeUiTextRor *description;        /* +0x4c0 */
    AoeUiScrollbarRor *description_bar; /* +0x4c4 */
    AoeUiButtonRor *ok, *cancel, *close; /* +0x4c8, +0x4cc, +0x4d0 */
    int32_t chosen;                   /* +0x4d4 */
    AoeGameUiRor *ui;
} AoeSelectScenarioRor;

extern const AoeUiEzVtblRor aoe_select_scenario_vtable_ror;
int aoe_select_scenario_init_ror(AoeSelectScenarioRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area);
void aoe_select_scenario_fill_ror(AoeSelectScenarioRor *s);

/* The "Game Settings Screen" (vtable 0x00547810,
   0x0049bf40..0x0049df10) over the "MP Setup Screen": the scenario list
   (in a networked game every scenario; otherwise the chosen one only;
   "Random Map" (9653, ID -2) and "Death Match" (9751, ID -1) on top with
   "2 - 8" players when they apply), its description, and the settings:
   map size (0..5), map type (0..8), victory (standard 0 or, with a
   scenario, 9; conquest 1; time limit 7 with its years; score 8 with its
   points), difficulty (4..0), starting age (0..5), resources (0, 2, 3),
   path finding (0..2), the population limit (networked: 25..200), and
   the check boxes (+0xad8 inverted, +0xad9, +0x985 and, networked,
   +0x982); OK, Cancel (Escape) and help. 0x0049bf40 constructor (via
   0x0049ced0's clearing); 0x0049cfe0 destructor (deleting 0x0049ceb0).
   0x0049d4b0 fills the list, 0x0049d8c0 reads the settings, 0x0049dba0
   stores them, 0x0049d7f0 shows the description, 0x0049dde0 /
   0x0049df10 show what applies. +0x4c 0x0049d270 the idle (filled once;
   networked, the host's player data each new millisecond); +0x64
   0x0049d320 user messages (the setup screen's first); +0xb8
   0x0049d360. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;                        /* +0x478 */
    int32_t random_map;                         /* +0x47c: a random map is chosen */
    AoeUiTextRor *size_label;                   /* +0x480 */
    AoeUiDropdownRor *size;                     /* +0x484 */
    AoeUiTextRor *type_label;                   /* +0x488 */
    AoeUiDropdownRor *type;                     /* +0x48c */
    AoeUiTextRor *victory_label;                /* +0x490 */
    AoeUiDropdownRor *victory;                  /* +0x494 */
    AoeUiTextRor *scenario_victory;             /* +0x498 */
    AoeUiTextRor *score_label;                  /* +0x4a0 */
    AoeUiDropdownRor *score;                    /* +0x4a4 */
    AoeUiTextRor *years_label;                  /* +0x4a8 */
    AoeUiDropdownRor *years;                    /* +0x4ac */
    AoeUiTextRor *difficulty_label;             /* +0x4b0 */
    AoeUiDropdownRor *difficulty;               /* +0x4b4 */
    AoeUiTextRor *age_label;                    /* +0x4b8 */
    AoeUiDropdownRor *age;                      /* +0x4bc */
    AoeUiTextRor *resources_label;              /* +0x4c0 */
    AoeUiDropdownRor *resources;                /* +0x4c4 */
    AoeUiTextRor *path_label;                   /* +0x4c8 */
    AoeUiDropdownRor *path;                     /* +0x4cc */
    AoeUiTextRor *population_label;             /* +0x4d0 */
    AoeUiDropdownRor *population;               /* +0x4d4 */
    AoeUiTextRor *list_label, *players_label;   /* +0x4d8, +0x4dc */
    int32_t list_x, list_y, list_w, list_h;     /* +0x4e0..+0x4ec */
    AoeUiListRor *list;                         /* +0x4f0 */
    AoeUiScrollbarRor *list_bar;                /* +0x4f4 */
    int32_t count;                              /* +0x4f8 */
    char **descriptions;                        /* +0x4fc */
    int32_t *flags;                             /* +0x500 */
    int32_t filled;                             /* +0x504 */
    AoeUiTextRor *description_label;            /* +0x508 */
    int32_t desc_x, desc_y, desc_w, desc_h;     /* +0x50c..+0x518 */
    AoeUiTextRor *description;                  /* +0x51c */
    AoeUiScrollbarRor *description_bar;         /* +0x520 */
    AoeUiButtonRor *checks[4];                  /* +0x524..+0x530 */
    AoeUiTextRor *check_texts[4];               /* +0x534..+0x540 */
    AoeUiButtonRor *ok, *cancel, *help;         /* +0x544..+0x54c */
    AoeUiPanelRor *setup;                       /* +0x550: the MP setup screen */
    int32_t last_line;                          /* +0x554 (-1) */
    int32_t offset;                             /* +0x558: minus the entries on top */
    uint32_t last_ms;                           /* +0x55c */
    AoeGameUiRor *ui;
} AoeGameSettingsScreenRor;

extern const AoeUiEzVtblRor aoe_game_settings_screen_vtable_ror;
int aoe_game_settings_screen_init_ror(AoeGameSettingsScreenRor *g, AoeGameUiRor *ui,
                                      AoeUiAreaRor *area);
void aoe_game_settings_screen_fill_ror(AoeGameSettingsScreenRor *g);
void aoe_game_settings_screen_read_ror(AoeGameSettingsScreenRor *g);
void aoe_game_settings_screen_store_ror(AoeGameSettingsScreenRor *g);

/* The scenario editor screen's own small methods (vtable 0x00547360);
   the screen's pages are not translated. */
typedef struct {
    AoeUiEzRor ez;
    uint8_t modified;                 /* +0x4c0 */
    AoeUiPanelRor *view, *minimap;    /* +0x4c8, +0x4cc */
    AoeUiPanelRor *bar;               /* +0x4d4 */
    AoeUiPanelRor *panel_774;         /* +0x774 */
    uint32_t view_interval, view_time;/* +0x92c, +0x930 */
    uint32_t map_interval, map_time;  /* +0x934, +0x938 */
    int16_t current_player;           /* +0x4b8, 1-based */
    AoeUiDropdownRor *players;        /* +0x73c */
    AoeUiButtonRor *techs[16];        /* +0x7d4 */
    char test_file[0x104];            /* global 0x007c0340 */
    AoeGameUiRor *ui;
} AoeScenarioEditorRor;

/* 0x004953e0 modified; 0x00494b20 a new scenario (with a modified one
   first the yes/no/cancel "NewSaveDialog", string 9287, 450 x 100);
   0x00494a90 the scenario menu dialog over the editor (0 when it could
   not be made); 0x00494a60 the selection cleared; 0x00492c10 a scenario
   player on or off; 0x00493030 the +0x774 panel shown or hidden and the
   bar redrawn. +0x4c 0x00493090 the idle: while editing (state 7) the
   view and minimap redraw on their intervals unless a panel is modal. */
uint8_t aoe_scenario_editor_modified_ror(const AoeScenarioEditorRor *e);
void aoe_scenario_editor_new_ror(AoeScenarioEditorRor *e);
int aoe_scenario_editor_menu_ror(AoeScenarioEditorRor *e, AoeScenarioMenuDialogRor *dialog);
void aoe_scenario_editor_clear_selection_ror(AoeScenarioEditorRor *e);
void aoe_scenario_editor_set_player_ror(AoeScenarioEditorRor *e, int16_t player, int32_t on);
void aoe_scenario_editor_show_774_ror(AoeScenarioEditorRor *e, int32_t on);
int32_t aoe_scenario_editor_idle_ror(AoeScenarioEditorRor *e);
/* File operations. 0x00494db0 open and 0x00494e50 quit (a modified
   scenario asks first: "OpenSaveDialog" / "QuitSaveDialog", 9287);
   0x00494eb0 save (needs an active and a human player, 10050 / 10051;
   with no file name the save as 0x00495110; the status, the save or
   2403; a name other than "default<n>" clears the modified flag; then
   by `after` as the save screen); 0x00495220 the file name, made
   "default<n>.scx" when there is none; 0x004952a0 that, then a save;
   0x004952c0 the scenario saved as "default<n>.scx" for a test, its name
   without the extension (NULL on failure). 0x004953f0 / 0x00495470: the
   current player's sixteen tech toggles stored / shown (a pressed
   button means off). */
void aoe_scenario_editor_open_ror(AoeScenarioEditorRor *e);
void aoe_scenario_editor_quit_ror(AoeScenarioEditorRor *e);
void aoe_scenario_editor_save_ror(AoeScenarioEditorRor *e, int close_menu, int32_t after);
void aoe_scenario_editor_save_as_ror(AoeScenarioEditorRor *e, int32_t after);
const char *aoe_scenario_editor_file_ror(AoeScenarioEditorRor *e);
void aoe_scenario_editor_save_default_ror(AoeScenarioEditorRor *e);
const char *aoe_scenario_editor_test_save_ror(AoeScenarioEditorRor *e);
void aoe_scenario_editor_store_techs_ror(AoeScenarioEditorRor *e);
void aoe_scenario_editor_show_techs_ror(AoeScenarioEditorRor *e);
/* +0x30 0x00494910 draw: the ez draw, then, when the bevel is on, there
   is a view and the panel's rect equals its clip, the editor's three-ring
   bevel (+0x320..+0x325) above the view (0, 0 .. width - 1, view top - 1)
   and below it (0, view bottom + 1 .. width - 1, height - 1) between
   begin_draw(0) and end_draw ("scr_sed::draw" lock). */
void aoe_scenario_editor_draw_ror(AoeScenarioEditorRor *e);

/* The "Save Game Screen" (vtable 0x00547234, 0x0048eca0..0x0048fac0):
   a name field over the existing saved games (.gam / .gmx in the game
   directory; mode 0) or scenarios (.scn / .scx in the scenario directory;
   modes 1 and 2, the second from the editor), Save, Delete (the Delete
   key, with a yes/no "Cancel Dialog") and Cancel (Escape). Saving an
   existing name asks first ("YesNoDialog", 9416), which also removes
   the old .gam / .scn; the file written is the .gmx / .scx. 0x0048eca0
   constructor (the current screen's popup background; the name given,
   else the first saved-game name in mode 0; the field holds 80
   characters, fewer for a long directory); 0x0048f070 destructor
   (deleting 0x0048f050); 0x0048f120 fills the list; +0x48 0x0048ec80 the
   base message handler; +0xb8 0x0048f2f0. 0x0048fac0: the editor's save
   (the status, the scenario saved or 2403, the flag cleared, then by
   `after`: 1 the main menu, 2 the open screen, 3 a new scenario, else
   back to the editor). */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;              /* +0x478 */
    AoeUiEditRor *name;               /* +0x47c */
    AoeUiListRor *list;               /* +0x480 */
    AoeUiScrollbarRor *list_bar;      /* +0x484 */
    AoeUiButtonRor *save;             /* +0x488 */
    AoeUiButtonRor *cancel;           /* +0x48c */
    AoeUiButtonRor *del;              /* +0x490 */
    char file[0x104];                 /* +0x494 */
    char typed[0x104];                /* +0x598 */
    int32_t mode;                     /* +0x69c */
    int32_t after;                    /* +0x6a0 */
    AoeGameUiRor *ui;
} AoeSaveGameScreenRor;

extern const AoeUiEzVtblRor aoe_save_game_screen_vtable_ror;
int aoe_save_game_screen_init_ror(AoeSaveGameScreenRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area,
                                  int32_t mode, const char *name, int32_t after,
                                  const char *popup_name, int32_t popup_id);
void aoe_save_game_screen_fill_ror(AoeSaveGameScreenRor *s);
void aoe_save_game_screen_editor_save_ror(AoeSaveGameScreenRor *s);

/* The "Campaign Selection Screen" (vtable 0x005460dc, 0x0047a7c0..
   0x0047b140; "scr2" / 50052): the title (11213), the campaigns (11214;
   heading 11101) and their scenarios (9726) each with a scroll bar, the
   difficulty (11215: 11220 down to 11216 as 4..0, from the settings'
   byte 0x993), OK, Cancel (Escape) and the hidden close button. 0x0047a7c0
   constructor (0x0047ac20 clears the fields first); 0x0047ac90 destructor
   (deleting 0x0047ac00); 0x0047af90 fills the campaigns (sorted; the
   chosen one picked and its campaign chosen); 0x0047b080 fills the
   chosen campaign's scenarios (unsorted, the chosen one picked, the list
   scrolled). +0x4c 0x0047ad80 the idle (filled once, the campaign list
   focused; the arrow cursor). +0xb8 0x0047ade0: OK starts (0x0047b140),
   Cancel opens the name selection, close quits, picking a campaign
   chooses it and refills the scenarios, double-clicking a scenario is OK.
   0x0047b140: with a scenario picked, it is chosen; the settings for a
   campaign game (flag 0x8fc, 0x9a0 1, 0x9a4 0, 0xa8c / 0xa90 0, the
   difficulty in 0x993 and 0xa10, 0xae5 and 0x1190 0, 0xa84 and 0xa88 1,
   0x985 0, 0x986 and 0x982 1, 0xad6 1, 0xad7 / 0xad8 / 0xad9 0,
   resources, age and 0xae4 0); every active scenario slot of 0..8 in
   order gets a player (0xaa0 = slot, 0xac4 = slot + 1, 0x994 = 1), the
   first human one swapped to player 0; the player count (0x988); comm
   slot 1 human (2), the others up to the count computer (4), the rest
   closed (0); then the game. A scenario that cannot be read shows 2506,
   a game that cannot start 2401. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title;                        /* +0x478 */
    int32_t camp_x, camp_y, camp_w, camp_h;     /* +0x47c..+0x488 */
    AoeUiTextRor *campaign_label;               /* +0x48c */
    AoeUiListRor *campaigns;                    /* +0x490 */
    AoeUiScrollbarRor *campaigns_bar;           /* +0x494 */
    int32_t scen_x, scen_y, scen_w, scen_h;     /* +0x498..+0x4a4 */
    AoeUiTextRor *scenario_label;               /* +0x4a8 */
    AoeUiListRor *scenarios;                    /* +0x4ac */
    AoeUiScrollbarRor *scenarios_bar;           /* +0x4b0 */
    AoeUiTextRor *difficulty_label;             /* +0x4b4 */
    AoeUiDropdownRor *difficulty;               /* +0x4b8 */
    AoeUiButtonRor *ok, *cancel, *close;        /* +0x4bc..+0x4c4 */
    int32_t filled;                             /* +0x4c8 */
    AoeGameUiRor *ui;
} AoeCampaignSelectionRor;

extern const AoeUiEzVtblRor aoe_campaign_selection_vtable_ror;
int aoe_campaign_selection_init_ror(AoeCampaignSelectionRor *c, AoeGameUiRor *ui,
                                    AoeUiAreaRor *area);
void aoe_campaign_selection_fill_campaigns_ror(AoeCampaignSelectionRor *c);
void aoe_campaign_selection_fill_scenarios_ror(AoeCampaignSelectionRor *c);
void aoe_campaign_selection_start_ror(AoeCampaignSelectionRor *c);

/* The "Campaign Editor Screen" (vtable 0x00545fb0, 0x00479360..
   0x0047a3c0; "scr4" / 50054): the title (11201, 640 x 50), "campaign
   file" (11202) over the campaign names (an editable drop-down; its edit
   starts with 11210, at most 100 characters), Save (11204), the available
   scenarios (11207) and the campaign's (11208), each with a scroll bar,
   Add (11205), Remove (11206), Up (11221), Down (11222), Done (4001) and
   Cancel (4002, Escape). 0x00479360 constructor; 0x004797d0 destructor
   (deleting 0x004797b0; the "Cancel Dialog" and "Load Dialog" closed
   first). 0x00479950 the campaign names ("*.cpn" then "*.cpx" in the
   campaign directory, sorted, extensions cut); 0x00479ab0 the available
   scenarios ("*.scn" then "*.scx" in the scenario directory, sorted);
   0x00479c10 the open campaign's scenario names; 0x00479d10 open the
   edit's campaign (".cpn", else ".cpx"): the available scenarios refilled
   and, when all of the campaign's are among them, taken out of that list
   and shown as the campaign's (else that list emptied); 0x00479ea0 save
   (a name that is empty or starts with a space: 11212; no scenarios:
   11211; an existing "<name>.cpn" must be writable and is removed; the
   campaign written as "<name>.cpx" with the title <name> and each
   scenario's file "<line>.scn" when that exists, else ".scx"; then the
   names refilled and the game's campaigns reloaded). Every change marks
   the screen changed (+0x478); open and save clear it. +0xb8 0x0047a3c0:
   picking a name in the drop-down opens it, asking first ("Load Dialog",
   11209 with the edit's text) when there are unsaved changes; Done asks
   the same ("Cancel Dialog") before the editor menu; Save saves; Cancel
   goes to the editor menu; Add / Remove / double-clicks move a scenario;
   Up / Down reorder. A dialog's yes saves first, no goes on, cancel
   stays. */
typedef struct {
    AoeUiEzRor ez;
    uint8_t changed;                            /* +0x478 */
    AoeUiTextRor *title, *file_label;           /* +0x47c, +0x480 */
    AoeUiTextRor *available_label, *chosen_label; /* +0x484, +0x488 */
    AoeUiListRor *available, *chosen;           /* +0x48c, +0x490 */
    AoeUiScrollbarRor *available_bar, *chosen_bar; /* +0x494, +0x498 */
    AoeUiEditRor *name;                         /* +0x49c */
    AoeUiButtonRor *done, *cancel, *save;       /* +0x4a0, +0x4a4, +0x4a8 */
    AoeUiButtonRor *add, *remove;               /* +0x4ac, +0x4b0 */
    AoeUiButtonRor *up, *down;                  /* +0x4b4, +0x4b8 */
    AoeUiDropdownRor *files;                    /* +0x4bc */
    AoeCampaignRor *campaign;                   /* +0x4c0 */
    AoeGameUiRor *ui;
} AoeCampaignEditorRor;

extern const AoeUiEzVtblRor aoe_campaign_editor_vtable_ror;
int aoe_campaign_editor_init_ror(AoeCampaignEditorRor *c, AoeGameUiRor *ui, AoeUiAreaRor *area);
void aoe_campaign_editor_fill_names_ror(AoeCampaignEditorRor *c);
void aoe_campaign_editor_fill_available_ror(AoeCampaignEditorRor *c);
void aoe_campaign_editor_open_ror(AoeCampaignEditorRor *c);
void aoe_campaign_editor_save_ror(AoeCampaignEditorRor *c);

/* The "Name Selection Screen" (vtable 0x00546fd4, 0x0048dec0..0x0048e470;
   "scr2" / 50052): the title (10750), "players" (10751) over the player
   names with a scroll bar, New (10752), Delete (10753), OK, Cancel
   (Escape) and the hidden close button; help 30300..30302, 30001, 30002.
   0x0048dec0 constructor (0x0048e2b0 clears the fields first);
   0x0048e1d0 destructor (deleting 0x0048e1b0; the "New Name Dialog"
   shown first); 0x0048e2f0 fills the names (sorted, the chosen one
   picked); 0x0048e3b0 disables Delete and OK without names and sets the
   Tab order (list, New, then Delete and OK when there are names, Cancel)
   and the focus (New without names, else the list). +0xb8 0x0048e470: OK
   chooses the picked player and opens the campaign selection; Cancel the
   single player menu; close quits; a double-click is OK; New opens the
   "New Name Dialog" over this screen; Delete asks (10754 with the name);
   a yes removes the player. The new-name dialog closing refills the names
   after an OK; either answer re-runs 0x0048e3b0.

   The "New Name Dialog" (vtable 0x00547100, 0x0048e800..0x0048eaa0; 500 x
   230 over its parent): the title (10755), "name" (10756), the name edit
   (15 characters), OK and Cancel (Escape); Tab over edit, OK, Cancel.
   0x0048ea10 destructor (deleting 0x0048e9f0). +0xb8 0x0048eaa0: OK (or
   Enter in the edit) with a name adds the player and opens the campaign
   selection, closing the dialog and the name screen (an empty name
   refocuses the edit); Cancel (or Escape in the edit) returns to the name
   screen when it is registered, else to the single player menu. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title, *label;              /* +0x478, +0x47c */
    AoeUiListRor *list;                       /* +0x480 */
    AoeUiScrollbarRor *list_bar;              /* +0x484 */
    AoeUiButtonRor *add, *del, *ok, *cancel, *close; /* +0x488..+0x498 */
    int32_t field_49c;                        /* +0x49c */
    AoeGameUiRor *ui;
} AoeNameSelectionRor;

typedef struct {
    AoeUiDialogRor dialog;
    AoeUiTextRor *title, *label;              /* +0x490, +0x494 */
    AoeUiEditRor *name;                       /* +0x498 */
    AoeUiButtonRor *ok, *cancel;              /* +0x49c, +0x4a0 */
    AoeGameUiRor *ui;
} AoeNewNameDialogRor;

extern const AoeUiEzVtblRor aoe_name_selection_vtable_ror;
extern const AoeUiEzVtblRor aoe_new_name_dialog_vtable_ror;
int aoe_name_selection_init_ror(AoeNameSelectionRor *n, AoeGameUiRor *ui, AoeUiAreaRor *area);
void aoe_name_selection_fill_ror(AoeNameSelectionRor *n);
void aoe_name_selection_update_ror(AoeNameSelectionRor *n);
int aoe_new_name_dialog_init_ror(AoeNewNameDialogRor *d, AoeGameUiRor *ui, AoeUiPanelRor *parent);

#endif
