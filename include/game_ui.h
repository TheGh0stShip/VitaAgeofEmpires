#ifndef VITA_AOE_GAME_UI_H
#define VITA_AOE_GAME_UI_H

#include <stdint.h>

#include "game_base.h"
#include "campaign_progress.h"
#include "game_settings.h"
#include "load_game_screen.h"
#include "mp_setup_screen.h"
#include "ui_dialog.h"
#include "ui_ez.h"
#include "ui_picture.h"
#include "palette_fade.h"
#include "tribute.h"

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
    /* The options dialog: the world's game speed (+0x18); the speed
       changed (a networked game first tells the others, string 3059
       with the player's name, then 0x004e98f0); map orders on
       (0x0055873c); the music and sound levels (0..99: 99 when there is
       no music manager / sound system, else -volume / 100) and setting
       them (0x004500d0 / 0x004a2810 at -100 x level; 0 when there is
       none); whether the music is on (game info +0x8a4), turning it on
       (the registry's "Music Volume" = level x 100, game +0x58, the
       current track replayed) or off (+0x5c); the scroll speed (game
       info +0x8c8, stored with +0x8d0); the screen width (the draw
       surface +0x2c) and whether a mode is available (0x00436f70(w, 0,
       8)); a resolution change (0x0041be20 unless already changing); the
       mouse button count (game info +0x8dc; setting it also runs game
       +0x148); game +0xa08; the path budget (0x00554738). */
    float (*game_speed)(void *context);
    void (*change_game_speed)(void *context, float speed);
    int (*orders_enabled)(void *context);
    int32_t (*music_level)(void *context);
    int32_t (*sound_level)(void *context);
    int (*set_music_level)(void *context, int32_t level);
    int (*set_sound_level)(void *context, int32_t level);
    int (*music_on)(void *context);
    void (*music_enable)(void *context, int32_t level);
    void (*music_disable)(void *context);
    int32_t (*scroll_speed)(void *context);
    void (*set_scroll_speed)(void *context, int32_t speed);
    int32_t (*screen_width)(void *context);
    int (*screen_mode_available)(void *context, int32_t width);
    void (*set_resolution)(void *context, int32_t width, int32_t height);
    int32_t (*mouse_buttons)(void *context);
    void (*set_mouse_buttons)(void *context, int32_t buttons);
    int32_t (*game_flag_a08)(void *context);
    void (*set_game_flag_a08)(void *context, int32_t value);
    void (*set_path_budget)(void *context, int32_t budget);
    /* The create dialog: the registry's "Default Age Of Empires MP
       Gamename" (user key, 0x0046aee0; NULL when unset) and its write
       (0x0046ae80); the game created under a name (0x00424410 then the
       status screen, closedpw and the comm's create call), nonzero on success. */
    const char *(*mp_game_name)(void *context);
    void (*set_mp_game_name)(void *context, const char *name);
    int (*create_mp_game)(void *context, const char *name);
    /* The join screen's sessions: enumerated (the first time the
       enumerator 0x004285c0 is made, -1 when it cannot be; again
       0x00428890), the count (0x00451ac0); entry i's name and the two
       player counts (0x00428670, 0x004286c0, 0x00428690); joining the
       session on a list line (0x004286f0 then the comm's join; nonzero on
       success); the comm dropped with a reason (0x004237c0); the
       enumerator released (0x0042c330 and delete). */
    int32_t (*mp_enum_sessions)(void *context, int32_t again);
    void (*mp_session_info)(void *context, int32_t i, const char **name, int32_t *a,
                            int32_t *b);
    int (*mp_join_session)(void *context, int32_t line);
    void (*mp_drop)(void *context, int32_t reason);
    void (*mp_release_sessions)(void *context);
    /* The MP startup screen: the registry's "Default Age Of Empires
       Player Name" and "... Multiplayer Service" (user key; NULL when
       unset) and their writes; the services refreshed and counted
       (0x00428380, 0x00428400) and service i's name when it is usable
       (0x004284e0, 0x00428420; NULL otherwise); the comm started as a
       player name on service i (0x00424440, 0x00428380, 0x00428480,
       0x00424eb0, 0x00424a60). The Zone: the browser command (HKCR
       http\shell\open\command's program, which must exist, followed by
       the machine key's "Zone" address) into out, nonzero when there is
       one; the command run (WinExec). */
    const char *(*mp_player_name)(void *context);
    void (*set_mp_player_name)(void *context, const char *name);
    const char *(*mp_service_name)(void *context);
    void (*set_mp_service_name)(void *context, const char *name);
    int32_t (*mp_services)(void *context);
    const char *(*mp_service)(void *context, int32_t i);
    void (*mp_start)(void *context, const char *name, int32_t service);
    int (*zone_command)(void *context, char *out, uint32_t size);
    void (*run_zone)(void *context, const char *command);
    /* The send message dialog: the game player of comm slot i (0x0041c080;
       0 when unused), the comm slot count (0x0041b9a0), a slot's status
       (0x00423d10; 2 and 4 are connected humans), our slot (0x00423c10),
       a slot's name (0x004241a0); a player is a computer (type +0x80 ==
       2); its civilization's name (game string 105 for +0x109); the
       viewed player's two relation tests (player vtable +0x1c and
       +0x18); recipient flag i read (0x0041ca70) and set (0x0041ca50);
       the flag of a player drawn in its colour ("dlg_plbn.shp" 50746,
       frame 0, 0x0049fe00). */
    int32_t (*slot_player)(void *context, int32_t slot);
    int32_t (*comm_slots)(void *context);
    int32_t (*comm_slot_status)(void *context, int32_t slot);
    int32_t (*comm_own_slot)(void *context);
    const char *(*comm_slot_name)(void *context, int32_t slot);
    int (*player_is_computer)(void *context, int32_t player);
    void (*player_civ_name)(void *context, int32_t player, char *out, uint32_t size);
    int (*player_relation_a)(void *context, int32_t me, int32_t player);
    int (*player_relation_b)(void *context, int32_t me, int32_t player);
    int (*recipient)(void *context, int32_t slot);
    void (*set_recipient)(void *context, int32_t slot, int on);
    void (*draw_player_flag)(void *context, AoeUiPanelRor *panel, int32_t player,
                             int32_t x, int32_t y);
    /* The diplomacy dialog: the world's player count (+0x3c), player
       p's type (+0x80; 2 a computer, nonzero no tribute), name (+0x44),
       the viewed player's stance toward p (0x0045da60; 0 ally, 1
       neutral, 3 enemy) and allied victory flag (0x0045da50); attribute
       i of a player (+0x50); whether a player has a finished building
       of a type (0x00451370(type, -1, -1, 1, 2, 0)). */
    int32_t (*player_count)(void *context);
    int32_t (*player_type)(void *context, int32_t player);
    const char *(*player_name)(void *context, int32_t player);
    int32_t (*player_stance)(void *context, int32_t me, int32_t player);
    int (*allied_victory)(void *context, int32_t player);
    float (*player_attribute)(void *context, int32_t player, int32_t i);
    int (*has_building)(void *context, int32_t player, int32_t type);
    /* The diplomacy dialog's OK: a stance command (0x004e9890), an
       allied victory command (0x004e9840), a tribute through the giver's
       +0xf8(player, resource, amount, fee). Its draw: player p's colour
       table (+0x10c; its +0x26 colour; NULL without one), the fill colour
       (0x00540760) and a colour-icon frame 0 drawn through a table
       (0x0049fe00, mode 1). */
    void (*set_stance)(void *context, int32_t me, int32_t player, uint8_t stance);
    void (*set_allied_victory)(void *context, int32_t me, int on);
    void (*give_tribute)(void *context, int32_t me, int32_t player, int resource, float amount,
                         float fee);
    const uint8_t *(*player_colour_table)(void *context, int32_t player, int16_t *colour);
    void (*set_fill_colour)(void *context, uint8_t colour);
    void (*draw_colour_icon)(void *context, AoeUiPanelRor *panel, void *picture, int32_t x,
                             int32_t y, const uint8_t *table);
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

/* The in-game "Send Message Dialog" (vtable 0x00543f54,
   0x00434fe0..0x00435c10): a row per used comm slot (name, civilization,
   a recipient box for another connected human), four recipient radios,
   a 65-character line and Send. */
typedef struct {
    AoeUiDialogRor dialog;
    AoeUiTextRor *title;                     /* +0x490 */
    AoeUiTextRor *names[8];                  /* +0x494 */
    AoeUiTextRor *civs[8];                   /* +0x4b4 */
    AoeUiButtonRor *checks[8];               /* +0x4d4 */
    AoeUiTextRor *radio_labels[4];           /* +0x4f4 */
    AoeUiButtonRor *radios[4];               /* +0x504 */
    AoeUiEditRor *edit;                      /* +0x518 */
    AoeUiTextRor *edit_label;                /* +0x51c */
    AoeUiButtonRor *send;                    /* +0x520 */
    int32_t slots[8];                        /* +0x52c */
    int32_t has_flags;                       /* +0x528 picture */
    AoeGameUiRor *ui;
} AoeSendMessageDialogRor;

extern const AoeUiEzVtblRor aoe_send_message_dialog_vtable_ror;

/* 0x00434fe0 constructor (with its setup); 0x00435650 destructor
   (deleting 0x00435630); +0xb8 0x00435750; +0x30 0x00435c10. */
int aoe_send_message_dialog_init_ror(AoeSendMessageDialogRor *d, AoeGameUiRor *ui,
                                     AoeUiPanelRor *parent);
/* 0x00435bc0 */
void aoe_send_message_dialog_apply_ror(AoeSendMessageDialogRor *d);

/* The in-game "Diplomacy Dialog" (vtable 0x00543814): stances, tribute
   and allied victory (tribute.h holds its logic and draw). */
typedef struct {
    AoeUiDialogRor dialog;
    AoeUiTextRor *title;                     /* +0x490 */
    AoeUiTextRor *headers[6];                /* +0x494.. */
    AoeUiTextRor *notice;                    /* +0x510 */
    AoeUiTextRor *fee;                       /* +0x4a8 */
    AoeUiTextRor *ally_label;                /* +0x4ac */
    AoeUiTextRor *names[8], *civs[8];
    AoeUiButtonRor *gifts[4][8];             /* wood, food, gold, stone */
    AoeUiButtonRor *stances[24];             /* +0x594: 3 per player */
    AoeUiButtonRor *allied_victory;          /* +0x5f4 */
    AoeUiButtonRor *ok, *clear, *cancel, *help; /* +0x5f8..+0x604 */
    AoeUiTextRor *stocks[4];                 /* wood, food, gold, stone */
    void *icons;                             /* +0x608 tradicon.shp 50732 */
    AoeTributeRor tribute;                   /* +0x610.. */
    int32_t own_row;                         /* +0x6b0 */
    int32_t has_market;                      /* +0x6b4 */
    AoeGameUiRor *ui;
    void *colours;                           /* +0x60c dlg_plbn.shp 50746 */
} AoeDiplomacyDialogRor;

extern const AoeUiEzVtblRor aoe_diplomacy_dialog_vtable_ror;

/* 0x00431470 constructor (with its setup); 0x004321f0 destructor
   (deleting 0x004321d0); +0xb8 0x004323b0 the action slot and +0x30
   0x00433230 the draw slot (tribute.h). */
int aoe_diplomacy_dialog_init_ror(AoeDiplomacyDialogRor *d, AoeGameUiRor *ui,
                                  AoeUiPanelRor *parent);

/* The "MP Startup Screen" (vtable 0x00546c50, 0x00487250..0x00487f40;
   640 x 480 on "scr3" 50053): a title (9611), "name" (9612), the
   remembered player name (15 characters), "connection" (9613), the
   service list (font 11) with its bar, OK, Cancel (Escape), help (?)
   and the close button; tab order name, OK, list, Cancel, help, the
   name focused. Building it clears the multiplayer flag. 0x00487770
   destructor (deleting 0x00487750) also closes "Create Dialog",
   "OKDialog" and "YesNoDialog". 0x00487d70 fills the list: each usable
   service (ID = its index), the Internet Gaming Zone (9614, ID -1)
   when a Zone command exists, the remembered service selected. +0xb8
   0x00487880: OK sets the multiplayer flag and needs a name ("Name
   Error" 9615) and a service ("List Error" 9618); the Zone asks first
   ("Start Zone" 9619: yes runs the browser and quits); otherwise the
   name and service are remembered, the comm started and the join screen
   opened. Cancel (or Escape in the name) drops the comm (2) and opens
   the main menu; a list double click is OK; help arms help mode; close
   quits. */
typedef struct {
    AoeUiEzRor ez;
    AoeUiTextRor *title, *name_label;        /* +0x478, +0x47c */
    AoeUiEditRor *name;                      /* +0x480 */
    AoeUiTextRor *list_label;                /* +0x484 */
    AoeUiButtonRor *ok, *cancel, *help;      /* +0x488..+0x490 */
    AoeUiButtonRor *close;                   /* +0x494 */
    AoeUiListRor *list;                      /* +0x498 */
    AoeUiScrollbarRor *list_bar;             /* +0x49c */
    char zone[0x200];                        /* +0x4a0 (program at +0x59f) */
    AoeGameUiRor *ui;
} AoeMpStartupScreenRor;

extern const AoeUiEzVtblRor aoe_mp_startup_screen_vtable_ror;

/* 0x00487250 */
int aoe_mp_startup_screen_init_ror(AoeMpStartupScreenRor *m, AoeGameUiRor *ui,
                                   AoeUiAreaRor *area);
/* 0x00487d70 */
void aoe_mp_startup_screen_fill_ror(AoeMpStartupScreenRor *m);

/* The "Join Screen" (vtable 0x00546208, 0x0047b440..0x0047beb0;
   640 x 480 on "scr3" 50053): a title (9631), a label (9640), the
   session list (font 11, "none" 9643 at first) with its bar, Refresh
   (9633), Join (9632), Create (9634), Cancel (Escape) and the close
   button; tab order list, Refresh, Join, Create, Cancel, Refresh
   focused. Building it sets comm slot 1 to 2 and slots 2..9 to 0.
   0x0047b780 destructor (deleting 0x0047b760) releases the session
   enumerator. 0x0047ba60 refresh: "searching" (9641), the sessions
   enumerated and listed as "name ( a / b )", or 9643 when none. +0x4c
   0x0047b870 idle: while joining, more than 10 s without an answer
   drops the comm (5) with message 9635. +0x64 0x0047b900 user
   messages: 0x17a8..0x17aa and 0x17b2 while joining drop the comm the
   same way; 0x17b8 while joining opens the MP setup screen. +0xb8
   0x0047bc20: a list double click joins; Join with sessions joins the
   selection (a failed join shows 9635 with the line, none shows
   9636); Cancel drops the comm (2) and opens the MP startup screen;
   Create opens the create dialog; Refresh refreshes; close quits. */
typedef struct {
    AoeUiEzRor ez;
    int32_t sessions_made;                   /* +0x478 */
    int32_t listed;                          /* +0x47c */
    AoeUiTextRor *title, *label;             /* +0x480, +0x484 */
    AoeUiListRor *list;                      /* +0x488 */
    AoeUiScrollbarRor *list_bar;             /* +0x48c */
    AoeUiButtonRor *join, *create, *cancel;  /* +0x490..+0x498 */
    AoeUiButtonRor *refresh, *close;         /* +0x49c, +0x4a0 */
    int32_t count;                           /* +0x4a4 */
    int32_t joining;                         /* +0x4a8 */
    uint32_t join_ms;                        /* +0x4ac */
    AoeGameUiRor *ui;
} AoeJoinScreenRor;

extern const AoeUiEzVtblRor aoe_join_screen_vtable_ror;

/* 0x0047b440 */
int aoe_join_screen_init_ror(AoeJoinScreenRor *j, AoeGameUiRor *ui, AoeUiAreaRor *area);
/* 0x0047ba60 */
void aoe_join_screen_refresh_ror(AoeJoinScreenRor *j);

/* The "MP Setup Screen" (vtable 0x00546d7c) as panels: its constructor
   0x00487f40 over the state and methods of mp_setup_screen.h. In the
   constructor's order: the base screen named, the settings reset
   (0x00488c40), the chat log dropped (0x0041ca10) and players 1..8 made
   chat recipients (0x0041ca50); 640 x 480 on "scr3" 50053 in multiplayer
   (+0x97e) else "scr2" 50052, shadow 0; the title (9678 multiplayer,
   9679), the name (9680) and civilisation (9681) column labels, the
   settings label (9682; neither +0x97d nor hosting), the colour (9646) and
   team (9647) labels, right-aligned (0x004f7cf0); eight rows 24 apart
   from y 85 (the lists a pixel above the row, the row texts two and
   three below it): the kind list (width 158 at x 7: 9683 -> 4, in multiplayer
   9684 -> 1, then 9685 / 9686 -> 0), the civilisation list (128 at 170:
   10231..10242 -> 1..12, 10246 -> 13, 10247 -> 14, 10249 -> 15, 10248 ->
   16, 10250 -> 17) and the position list (40 at 324), each with its
   button on the left and no border; the colour button (30 x 20 at 310,
   command 1) with a text behind it, the team button (9648, at 380) cycling
   five states ("1".."4" with IDs 2..5) with its text, three row texts,
   the ready text (9695, font 6, inactive) and one more; the settings
   button (9682 at 420, 50; +0x97d or hosting), the summary title (at 420,
   84; 220 x 68) and 20 lines (at 420, from y 83, 24 apart); in multiplayer the chat label (9687), the
   chat text (400 x 96 at 10, 300, font 10) with its scrollbar, bevel 3
   and the input (247 characters, at 10, 402); in single player the player
   count label (9688) and list (at 31, 322); then the bottom row at y 440:
   hosting, the ready check (at 10) and text (9663), Start (9710, disabled
   until everyone is ready), Cancel (4002 at 405) and the addresses button
   (9711 at 600, 400), and the ready message (0x004251c0) with zeros; a
   guest the check (at 100), its text, and Cancel (at 330); single player
   Start (9710 at 70, focused) and Cancel (at 330); Help (4009 at 600) and
   the hidden 1002 button (active, position mode 9); Cancel's hotkey is
   Escape. Then the tab order (0x0048ced0), the summary refresh
   (0x0048c010) and, single player, the players (0x0048cfb0) and rows
   (0x0048b640), and the chat marker 3, 2, 1 sent (0x004213e0).

   The state's game and comm ops are the caller's; the two panel
   refreshes (aoe_mp_setup_screen_info_ror / _rows_ror) need their own
   ops and are not run here. The idle, action and user slots are the base
   ones until the translated handlers are bound to these panels. */
enum { AOE_MP_SETUP_INFO_LINES = 20 };

typedef struct {
    AoeUiEzRor ez;
    AoeMpSetupScreenRor state;
    AoeUiTextRor *title;                        /* +0x478 */
    AoeUiTextRor *name_label, *civ_label;       /* +0x47c, +0x480 */
    AoeUiTextRor *settings_label;               /* +0x484 */
    AoeUiTextRor *color_label, *team_label;     /* +0x488, +0x48c */
    /* Row texts filled by the rows refresh 0x0048b640. */
    AoeUiTextRor *texts_490[AOE_MP_SETUP_PLAYERS], *texts_4b0[AOE_MP_SETUP_PLAYERS];
    AoeUiTextRor *texts_4d0[AOE_MP_SETUP_PLAYERS], *ready_texts[AOE_MP_SETUP_PLAYERS]; /* +0x4f0 */
    AoeUiTextRor *texts_510[AOE_MP_SETUP_PLAYERS];
    AoeUiTextRor *texts_530[AOE_MP_SETUP_PLAYERS], *texts_550[AOE_MP_SETUP_PLAYERS];
    AoeUiButtonRor *color_buttons[AOE_MP_SETUP_PLAYERS]; /* +0x570 */
    AoeUiButtonRor *team_buttons[AOE_MP_SETUP_PLAYERS];  /* +0x590 */
    AoeUiDropdownRor *kind_lists[AOE_MP_SETUP_PLAYERS];  /* +0x5b0 */
    AoeUiDropdownRor *civ_lists[AOE_MP_SETUP_PLAYERS];   /* +0x5d0 */
    AoeUiDropdownRor *pos_lists[AOE_MP_SETUP_PLAYERS];   /* +0x5f0 */
    AoeUiEditRor *chat_input;                   /* +0x610 */
    AoeUiTextRor *chat_label, *chat;            /* +0x614, +0x618 */
    AoeUiScrollbarRor *chat_bar;                /* +0x61c */
    AoeUiButtonRor *settings;                   /* +0x620 */
    AoeUiTextRor *info_title;                   /* +0x624 */
    AoeUiTextRor *info[AOE_MP_SETUP_INFO_LINES]; /* +0x62c.. */
    AoeUiTextRor *count_label;                  /* +0x6dc */
    AoeUiDropdownRor *count;                    /* +0x6e0 */
    AoeUiButtonRor *start, *cancel, *help;      /* +0x700, +0x70c, +0x710 */
    AoeUiButtonRor *ready, *hidden;             /* +0x714, +0x718 */
    AoeUiTextRor *ready_label;                  /* +0x71c */
    AoeUiButtonRor *addresses;                  /* +0x858 */
    AoeGameUiRor *ui;
} AoeMpSetupUiRor;

extern const AoeUiEzVtblRor aoe_mp_setup_ui_vtable_ror;

/* The "Load Saved Game Screen" (vtable 0x005468cc) as panels: its
   constructor 0x00485c90 over the state and methods of
   load_game_screen.h. The screen set up (0x00468180) with its own info
   name and ID, shadow 0, 640 x 480; the title (9471, font 1, centred) at
   20, 10 (600 x 30) and the label (9474, font 4) at 20, 45 (600 x 20);
   the list (font 11) at 20, 65 (600 x 352) with its scrollbar (width 20,
   help 30320); OK (4001) at 30, Delete (9417) at 230 and Cancel (4002)
   at 430, each 180 x 30 at y 440, OK's help 30001 and Cancel's 30002;
   Escape on Cancel and Delete (0x2e) on Delete; the list filled
   (0x004860a0); OK and Delete disabled when nothing is selected; the
   first saved-game name (0x005001a0) selected and scrolled to when
   listed (0x004650c0(1, index, 1)); the list focused; tab order list,
   OK, Delete, Cancel. The list's ops are bound to the panel here and the
   files come from the game UI's ops (game_dir, list_files); the action
   handler 0x00486200 runs when `action_ops` is given, else the base one. */
typedef struct {
    AoeUiEzRor ez;
    AoeLoadGameScreenRor state;
    AoeLoadGameScreenOpsRor state_ops;          /* bound to this screen */
    const AoeLoadGameActionOpsRor *action_ops;  /* the caller's, or NULL */
    AoeUiTextRor *title, *label;                /* +0x478, +0x47c */
    AoeUiListRor *list;                         /* +0x480 */
    AoeUiScrollbarRor *list_bar;                /* +0x484 */
    AoeUiButtonRor *ok, *cancel, *remove;       /* +0x488, +0x48c, +0x490 */
    AoeGameUiRor *ui;
} AoeLoadGameUiRor;

extern const AoeUiEzVtblRor aoe_load_game_ui_vtable_ror;

/* 0x00485c90. 0 when the screen could not be set up (+0xd8 set), else 1,
   even when a panel failed. */
int aoe_load_game_ui_init_ror(AoeLoadGameUiRor *l, AoeGameUiRor *ui, AoeUiAreaRor *area);

/* 0x00487f40 with the state's ops (required). 0 when the screen could
   not be set up (+0xd8 set), else 1, even when a panel failed. */
int aoe_mp_setup_ui_init_ror(AoeMpSetupUiRor *u, AoeGameUiRor *ui, AoeUiAreaRor *area,
                             const AoeMpSetupScreenOpsRor *ops);

/* The "Create Dialog" over the join screen (vtable 0x00546334,
   0x0047beb0..0x0047c510): 500 x 230, a title (9638), a label (9642),
   a 40-character edit box holding the remembered MP game name and OK /
   Cancel (Escape) buttons; tab order edit, OK, Cancel. */
typedef struct {
    AoeUiDialogRor dialog;
    AoeUiTextRor *title, *label;     /* +0x490, +0x494 */
    AoeUiEditRor *edit;              /* +0x498 */
    AoeUiButtonRor *ok, *cancel;     /* +0x49c, +0x4a0 */
    AoeGameUiRor *ui;
} AoeCreateDialogRor;

extern const AoeUiEzVtblRor aoe_create_dialog_vtable_ror;

/* 0x0047beb0 constructor; 0x0047c120 destructor (deleting 0x0047c100);
   +0xb8 0x0047c1b0: OK or Enter with a blank name refocuses the edit;
   otherwise the name is remembered and the game created: on success the
   MP setup screen is made and shown and the status, create and join
   screens close; on failure the join screen comes back with an OK
   message (9645, 450 x 100). Cancel or Escape goes back to the join
   screen. */
int aoe_create_dialog_init_ror(AoeCreateDialogRor *d, AoeGameUiRor *ui,
                               AoeUiPanelRor *parent);

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
/* The tribe game's 0x005002e0(text, info name, info ID): a new "Status
   Screen" when none is registered (the original then also updates the
   window, 0x00419ba0, for an existing one: not needed here), else its
   message replaced; then it is shown. 0 when it could not be made. */
int aoe_status_screen_show_ror(AoeGameUiRor *ui, AoeUiAreaRor *area, const char *text,
                               const char *info_name, int32_t info_id);

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
   the history (message 4, 10801 "History"), the instructions (0, 10802)
   and the hints (1, 10807 "Hints"), kept to the messages there are (no buttons with
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
    AoeUiButtonRor *buttons[3];       /* +0x488: history, instructions, hints */
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

/* The "Config Dialog" (vtable 0x005436e0, 0x004300d0..0x00430e30; 600 x
   390 over its parent): the title (9431); game speed (9439: 9432..9434 as
   1.0, 1.5, 2.0; disabled in state 5); music and sound volume (9435 and
   9438 with 9436 / 9437 at the ends; value bars of 100 by 10 / 20); scroll
   speed (9456, 9457 / 9458; the bar is the speed less 10); screen size
   (9440: 9447..9449 as 640 x 480, 800 x 600, 1024 x 768, unavailable
   modes disabled); mouse buttons (9450: 9451 two, 9452 one); game +0xa08
   (9453: 9454 on, 9455 off); path finding (9741: 9742..9744 low, medium,
   high, disabled when networked; the settings' 0xa14, or 0x983 when
   networked); OK and Cancel (Escape) with the focus on OK. 0x004300d0
   constructor (`mode` 0 in the game); 0x00430c40 destructor (deleting
   0x00430c20). +0xb8 0x00430e30: the volume and scroll bars apply as they
   move (the music bar also re-applies the sound level); Cancel restores
   the volumes and scroll speed; OK in the game resumes it, stores the
   mouse buttons, game +0xa08, (in a single player game) the path finding
   and its budget 2500 / 5000 / 7500, the game speed (outside state 5,
   when orders are on and it changed; also kept in the settings in a
   single player game), the music (on at a level below 99, else off; then
   the sound level) and the resolution. Both return to the game screen in
   the game (the dialog hidden) or to the main menu otherwise (OK then
   applies nothing), closing the dialog. Picking a speed or screen size
   only consumes the click. */
typedef struct {
    AoeUiDialogRor dialog;
    int32_t mode;                                        /* +0x490 */
    AoeUiTextRor *title, *speed_label, *speed_text[3];   /* +0x494..+0x4a4 */
    AoeUiButtonRor *speed[3];                            /* +0x4a8 */
    AoeUiTextRor *music_label, *music_high, *music_low;  /* +0x4b4 */
    AoeUiValueScrollbarRor *music;                       /* +0x4c0 */
    AoeUiTextRor *sound_label, *sound_high, *sound_low;  /* +0x4c4 */
    AoeUiValueScrollbarRor *sound;                       /* +0x4d0 */
    AoeUiTextRor *scroll_label, *scroll_fast, *scroll_slow; /* +0x4d4 */
    AoeUiValueScrollbarRor *scroll;                      /* +0x4e0 */
    AoeUiTextRor *screen_label, *screen_text[3];         /* +0x4e4 */
    AoeUiButtonRor *screen[3];                           /* +0x4f4 */
    AoeUiTextRor *mouse_label, *mouse_text[2];           /* +0x500 */
    AoeUiButtonRor *mouse[2];                            /* +0x50c */
    AoeUiTextRor *flag_label, *flag_text[2];             /* +0x514 */
    AoeUiButtonRor *flag[2];                             /* +0x520 */
    AoeUiTextRor *path_label, *path_text[3];             /* +0x528 */
    AoeUiButtonRor *path[3];                             /* +0x538 */
    AoeUiButtonRor *ok, *cancel;                         /* +0x544, +0x548 */
    int32_t music_old, sound_old;                        /* +0x54c, +0x550 */
    float speed_old;                                     /* +0x554 */
    int32_t scroll_old;                                  /* +0x558 */
    int32_t music_now, sound_now;                        /* +0x55c, +0x560 */
    AoeGameUiRor *ui;
} AoeConfigDialogRor;

extern const AoeUiEzVtblRor aoe_config_dialog_vtable_ror;
int aoe_config_dialog_init_ror(AoeConfigDialogRor *d, AoeGameUiRor *ui, int32_t mode,
                               AoeUiPanelRor *parent);

#endif
