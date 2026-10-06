#ifndef VITA_AOE_GAME_ROR_H
#define VITA_AOE_GAME_ROR_H

#include "game_base.h"
#include <stdint.h>

#include "game_settings.h"

/* Patched Rise of Rome 1.0a tribe game (vtable 0x0054a264,
   0x004fde40..0x00506700): the RoR game object's own methods: the
   command line's hidden population limit, the tribe settings snapshot and
   its multiplayer exchange, the trigonometry tables, the view position
   history, the computer player name picker, the saved and loaded game
   names, the colour tables, the game screen's notifications and the
   intro video window's input. Windows calls, screens and the comm object
   are ops; semantic storage, offsets noted. */

/* 0x004fde40: the command line's letter code: each character 'A'..'_'
   becomes two bytes from a 31-entry table, the rest are copied; at most
   `n` bytes (a pair may end one past), always terminated. */
void aoe_game_ror_encode_ror(const char *in, char *out, int32_t n);

typedef struct {
    void *context;
    void (*no_terrain_sound)(void *context);              /* 0x007c052c = 1 */
    void (*set_population)(void *context, uint8_t limit);  /* 0x005055a0 */
    void (*base_command_line)(void *context);              /* 0x00417ba0 */
} AoeGameRorCommandOpsRor;

/* +0x6c 0x004fed50: the command line (game +0xc +0x518, upper-cased, 255
   characters): "NOTERRAINSOUND" turns terrain sounds off; "LIMIT=" (found
   in the coded line) sets the population limit (atoi, clamped to
   25..200); then the base parse. */
void aoe_game_ror_command_line_ror(const char *line, const AoeGameRorCommandOpsRor *ops);

/* The tribe settings block (0x6c bytes; +0x18..+0x5a per player). */
typedef struct {
    int32_t map_size, map_type, a84, a88, victory, victory_amount; /* +0x00..+0x14 */
    uint8_t byte_a94[9];        /* +0x18 */
    int32_t int_aa0[9];         /* +0x24 */
    uint8_t byte_ac4[9];        /* +0x48 */
    uint8_t byte_acd[9];        /* +0x51 */
    uint8_t ad6, ad7, ad8, ad9; /* +0x5a..+0x5d */
    int32_t resource_level;     /* +0x60 */
    int32_t starting_age;       /* +0x64 */
    uint8_t ae4, ae5;           /* +0x68, +0x69 */
    uint8_t population;         /* +0x6a */
} AoeTribeSettingsRor;

/* 0x00505170 snapshot / 0x00505070 restore (the setters, in order). */
void aoe_game_ror_settings_get_ror(const AoeGameSettingsRor *g, AoeTribeSettingsRor *out);
void aoe_game_ror_settings_set_ror(AoeGameSettingsRor *g, const AoeTribeSettingsRor *in);
/* The 0x6c-byte wire form (little-endian, the offsets above). */
void aoe_game_ror_settings_pack_ror(const AoeTribeSettingsRor *in, uint8_t out[0x6c]);
void aoe_game_ror_settings_unpack_ror(const uint8_t in[0x6c], AoeTribeSettingsRor *out);

/* +0x48 0x00504fd0 / +0x4c 0x00505020: with the comm (+0x18c) the
   0x114-byte record (the 0xa8 base record and the tribe block) sent
   (0x004257f0) / received (0x00425880; exactly 0x114 bytes) and applied. */
typedef struct {
    void *context;
    int has_comm;
    void (*send)(void *context, const uint8_t *data, int32_t n);
    const uint8_t *(*receive)(void *context, int32_t *n);
} AoeGameRorCommOpsRor;
/* The base game's +0x48 0x0041b430 / +0x4c 0x0041b450: the 0xa8-byte
   record alone (+0x8f8) sent / received (exactly 0xa8 bytes) and applied
   (0x0041b490). */
void aoe_game_send_settings_ror(const AoeGameSettingsRor *g, const AoeGameRorCommOpsRor *ops);
void aoe_game_receive_settings_ror(AoeGameSettingsRor *g, const AoeGameRorCommOpsRor *ops);
void aoe_game_ror_send_settings_ror(const AoeGameSettingsRor *g, const AoeGameRorCommOpsRor *ops);
void aoe_game_ror_receive_settings_ror(AoeGameSettingsRor *g, const AoeGameRorCommOpsRor *ops);

/* 0x005065f0 (also 0x005066f0 / 0x00506700): the trigonometry tables:
   4096 steps of 2 pi / 4096 and 360 degrees: sin, cos, tan (0 at the
   quarter turns) and atan of the angle in radians (0 at 0 and half a
   turn). The original evaluates in x87 precision; doubles here. */
typedef struct {
    float sin4096[4096], cos4096[4096], tan4096[4096], atan4096[4096];
    float sin360[360], cos360[360], tan360[360], atan360[360];
} AoeTrigTablesRor;
void aoe_game_ror_trig_ror(AoeTrigTablesRor *t);

/* The view position history (+0xc28 x[5], +0xc3c y[5], +0xc50 head,
   +0xc54 cursor). */
typedef struct {
    int32_t x[5], y[5];
    int32_t head, cursor;
} AoeViewHistoryRor;
/* 0x005064d0: remember (x, y) (the head again is not repeated);
   0x00506550: go back one (wrapping, back to the head at an empty slot)
   and move the viewed player's view there (`move`). */
void aoe_game_ror_view_push_ror(AoeViewHistoryRor *h, int32_t x, int32_t y);
void aoe_game_ror_view_back_ror(AoeViewHistoryRor *h,
                                void (*move)(void *context, float x, float y), void *context);

/* +0x1198: the computer names in use, ten per civilisation. 0x00505f90:
   a free name for `civ` (the count is string (civ x 5 + 0x893) x 2; up
   to 20 random tries, then the first free one); returns civ x 10 + index
   - 9 and marks it. 0x00506040 clears them. */
typedef struct {
    uint8_t used[16][10];
} AoeComputerNamesRor;
int32_t aoe_game_ror_pick_name_ror(AoeComputerNamesRor *n, int32_t civ, int32_t count,
                                   int32_t (*rand)(void *context), void *context);
void aoe_game_ror_clear_names_ror(AoeComputerNamesRor *n);

/* 0x004ff4b0 / 0x004ff4f0: the saved game name (+0xe64) / the restart
   name (+0xf68) (NULL clears); 0x005001a0 the first. */
void aoe_game_ror_set_name_ror(char name[0x104], const char *text);

/* 0x005060b0: the four colour tables (+0xa24..+0xa30); 0x005060e0 picks
   set 1 when settings +0x8dc is 1, else set 0. */
typedef struct {
    const void *a;
    int32_t b;
    const void *c;
    int32_t d;
} AoeColorTablesRor;
void aoe_game_ror_set_color_tables_ror(AoeColorTablesRor *t, const void *a, int32_t b,
                                       const void *c, int32_t d);
void aoe_game_ror_pick_color_tables_ror(AoeColorTablesRor *t, int16_t mode,
                                        const AoeColorTablesRor sets[2]);

typedef struct {
    void *context;
    int32_t mode;               /* +0x1b0: 1 video, 4..7 in game / editor */
    void *game_screen;          /* +0xc24 */
    /* 0x00451ae0 "Scenario Editor Screen" and its +0x4c8 / +0x4cc; the
       game screen's +0x4b8 / +0x4bc. */
    void *(*editor)(void *context);
    void *(*panel)(void *context, void *screen, int which);
    /* Notifications: 0x00481690 the edit mode change, 0x004816d0 the
       viewed player change, 0x00482140 the title text, 0x004821b0 the six
       lines (0x00423e20(1..6)). */
    void (*mode_changed)(void *context, int32_t old_mode, int32_t mode);
    void (*player_changed)(void *context, int32_t old_player, int32_t player);
    void (*title)(void *context, const char *text);
    const char *(*status_line)(void *context, int32_t i);
    void (*lines)(void *context, const char *const text[6]);
    /* Base methods: 0x00419970 / 0x00419990. */
    void (*set_edit_mode)(void *context, int32_t a, int32_t b);
    void (*set_player)(void *context, int32_t v);
    int32_t edit_mode;          /* +0x1b4 */
    int (*world)(void *context, int16_t *player); /* +0x3f4 and its +0x7c */
} AoeGameRorScreenOpsRor;

/* +0x10 0x004ff420: the edit mode set (0x00419970), the game screen told
   of the change; +0x14 0x004ff460: the viewed player (0x00419990), told
   with a world. 0x00500100 / 0x00500150: the main view / the minimap of
   the game screen (modes 4..6) or the editor (mode 7). 0x00501640 /
   0x00501660: the title (+0x106c) and the six status lines to the game
   screen. */
void aoe_game_ror_set_edit_mode_ror(const AoeGameRorScreenOpsRor *ops, int32_t a, int32_t b,
                                    int32_t *edit_mode);
void aoe_game_ror_set_player_ror(const AoeGameRorScreenOpsRor *ops, int32_t v);
void *aoe_game_ror_main_view_ror(const AoeGameRorScreenOpsRor *ops);
void *aoe_game_ror_minimap_ror(const AoeGameRorScreenOpsRor *ops);
void aoe_game_ror_title_ror(const AoeGameRorScreenOpsRor *ops, const char *title);
void aoe_game_ror_status_lines_ror(const AoeGameRorScreenOpsRor *ops);

/* The intro video window and keys. */
typedef struct {
    void *context;
    int32_t mode;               /* +0x1b0 */
    int32_t wait_cursor;        /* +0x1d8 */
    uint32_t video;             /* +0xbec: the video window */
    int32_t video_busy;         /* +0xc0c */
    int32_t paused;             /* +0xbf8 */
    int session;                /* 0x0041b760 */
    void (*cursor)(void *context, int wait);             /* 0x0041bd60 / 0x0041bdc0 */
    void (*hide_cursor)(void *context);                  /* SetCursor(NULL) */
    void (*end_video)(void *context, int skip);          /* 0x00500740 */
    void (*send)(void *context, uint32_t window, uint32_t msg);
    int32_t (*base_proc)(void *context, uint32_t msg);   /* 0x00419dc0 / old proc */
    int32_t (*base_palette)(void *context);              /* 0x0041ae00 */
    void (*base_activate)(void *context, int32_t active);/* 0x0041abb0 */
    int (*help)(void *context);                          /* focus panel +0xb0 */
} AoeGameRorInputOpsRor;

/* +0x08 0x005055e0: the window procedure: focus picks the cursor;
   mouse moves hide it and clicks end the video in mode 1; the rest to the
   base. 0x00506060: the video window's own procedure (the same, else the
   old procedure). +0xf4 0x00505940: palette change: in mode 1 the video
   realizes its palette (0x476) unless busy. +0xe0 0x005059a0:
   activation: the base, then the video paused (0x809) / resumed (0x855).
   +0x108 0x00505e60: keys: in mode 1 Enter and Space end, Escape skips;
   else F1 asks the focused panel for help (outside a game session). */
int32_t aoe_game_ror_window_proc_ror(AoeGameRorInputOpsRor *ops, uint32_t msg);
int32_t aoe_game_ror_video_proc_ror(AoeGameRorInputOpsRor *ops, uint32_t msg);
int32_t aoe_game_ror_palette_ror(AoeGameRorInputOpsRor *ops);
int32_t aoe_game_ror_activate_ror(AoeGameRorInputOpsRor *ops, int32_t active);
int32_t aoe_game_ror_key_ror(AoeGameRorInputOpsRor *ops, int32_t key);

/* The tribe game's world (+0x3f4), made by its vtable +0xbc. */
typedef struct {
    void *context;
    void *world;                                          /* +0x3f4 */
    void *(*create_world)(void *context);                 /* game +0xbc */
    /* World +0xb0 (game info +0x1f8, game +0x60, +0x18c) / +0xac (game
       info +0x1f8, +0xf3): nonzero on success. */
    int (*load)(void *context, void *world);
    int (*check)(void *context, void *world);
    void (*delete_world)(void *context, void *world);     /* world +0xa0(1) */
    void (*cursor)(void *context, int wait);              /* 0x0041bd60 / 0x0041bdc0 */
    void (*status)(void *context, int32_t text_id);       /* 0x00500390(id, 0, -1) */
    void (*status_close)(void *context);                  /* 0x005003e0 */
} AoeGameRorWorldOpsRor;

/* 0x005011e0: without a world one is made and loaded; a failed load
   deletes it again. 1 when a world is there. 0x00501140: under the wait
   cursor and status 0x44d, any world is deleted, a new one made, asked
   its +0xac check and deleted; returns the check (0 when none made). */
int aoe_game_ror_ensure_world_ror(AoeGameRorWorldOpsRor *ops);
int aoe_game_ror_check_world_ror(AoeGameRorWorldOpsRor *ops);

/* 0x004fe490: the game's dialogs and screens deleted
   (`remove(name)`, 0x00451df0), the game screen too when `all`. */
void aoe_game_ror_delete_screens_ror(void (*remove)(void *context, const char *name),
                                     void *context, int all, void **game_screen);
/* 0x00505f40: the end text: string 0x47e when the viewed player won
   (+0x80 == 1), else 0x47f. */
int32_t aoe_game_ror_end_text_id_ror(uint8_t won);

/* The tribe game going to a menu screen: 0x00500c60 the main menu
   (0x00486600, "Main Menu") and 0x00500b40 the campaign selection
   (0x0047a7c0, "Campaign Selection Screen"). A playing video is ended
   first; a screen that cannot be made, or whose setup
   failed, gives 0 (the failed one deleted); else it is shown, the game
   mode becomes 2 (+0x0c) and, with a music manager and the menu music not
   held (+0xc1c), the menu music starts by the manager's source: 1 CD track
   2, 2 "xopen.mid", 3 "open.wav". */
typedef struct {
    void *context;
    int video_playing;                          /* +0xc00 */
    void (*end_video)(void *context);
    /* Makes and registers the screen: 1 made, 0 failed (deleted). */
    int (*make_screen)(void *context);
    void (*show_named)(void *context, const char *name);
    void (*set_mode)(void *context, int32_t mode);
    int music_source;                           /* manager +0x104; 0 none */
    int music_held;                             /* +0xc1c */
    void (*play_track)(void *context, int32_t track);       /* 0x00450180 */
    void (*play_file)(void *context, const char *file);     /* 0x00450200 */
} AoeGameRorMenuOpsRor;
int aoe_game_ror_to_menu_ror(const char *screen, const AoeGameRorMenuOpsRor *ops);

/* Leaving a session. 0x005063e0: outside a networked game the comm
   object is reset (0x0041df70); in one that is running (states 4..7)
   the turn markers go out (0x00421680), the comm is polled
   and, with commands still unacknowledged (0x00421b10), the wait cursor
   and status 1216 show while polling continues for at least 500 ms and
   then as long as some remain, up to 15 s from the start (stopping early
   once the world is gone or its +0x1d is 1); then the reset and an
   unlink to level 5 (0x004237c0). 0x00505f10 (+0x12c): with a comm
   object, leave and return to state 0; always 1. 0x00503060: leave; a
   pending scenario test (+0xae8) returns to the editor;
   else a running or ended game (states 3..6) goes to state 0 and, with a
   world, the viewed player's victory (+0x80 1) or defeat video plays
   (kind 4 with the scenario's +0x15e8 / +0x16c8 name). */
typedef struct {
    void *context;
    int networked;
    int32_t state;                               /* +0x1b0 */
    void (*reset_comm)(void *context);
    void (*send_markers)(void *context);
    void (*poll)(void *context);
    int32_t (*unacknowledged)(void *context);
    void (*wait_cursor)(void *context);
    void (*status)(void *context, int32_t string_id);
    uint32_t (*now_ms)(void *context);
    int (*world_done)(void *context);           /* no world, or its +0x1d is 1 */
    void (*unlink)(void *context, int32_t level);
} AoeGameRorLeaveOpsRor;
void aoe_game_ror_leave_ror(const AoeGameRorLeaveOpsRor *ops);

typedef struct {
    AoeGameRorLeaveOpsRor leave;
    int has_comm;                                /* +0x18c */
    int test_pending;                            /* +0xae8 */
    int has_world;                               /* +0x3f4 */
    int won;                                     /* viewed player +0x80 == 1 */
    void (*set_state)(void *context, int32_t state);
    void (*back_to_editor)(void *context);
    void (*world_ended)(void *context);          /* world +0xb4 */
    void (*play_video)(void *context, int32_t kind, int won);
} AoeGameRorEndOpsRor;
int aoe_game_ror_quit_session_ror(const AoeGameRorEndOpsRor *ops);
void aoe_game_ror_end_session_ror(const AoeGameRorEndOpsRor *ops);


/* The debug run log (c:\aoerun.txt). */
typedef struct {
    void *context;
    int enabled;  /* 0x00580db0 */
    int opened;   /* 0x007c0650: the first line truncates the file */
    void *(*open)(void *context, const char *path, const char *mode); /* 0x005263f1 */
    void (*write)(void *context, void *file, const char *text);        /* 0x00526875 */
    void (*close)(void *context, void *file);                          /* 0x005262f0 */
    /* time() then ctime() (0x0052668c, 0x00528deb). */
    void (*now_text)(void *context, char *out, int32_t size);
} AoeGameRorRunLogRor;

/* 0x004fdea0(text, stamped): when enabled, opens the log ("w" the first
   time, "a" after; nothing when that fails) and writes "<time> - text\n"
   (the time text cut to 24 characters) when `stamped`, else "text\n". */
void aoe_game_ror_run_log_ror(AoeGameRorRunLogRor *log, const char *text, int32_t stamped);

/* +0x64 0x005065e0: returns 1, the same code as the base default
   0x004172b0 (aoe_game_default_true_ror in game_base.h). */

/* Destructor 0x004fe220 (the scalar deleting form 0x004fe200 frees the
   object afterwards): +0x1250 and +0x1b0 cleared, the video window
   (+0x1dc) destroyed, 0x004fe490 with `all`, then the 28 named screens
   below closed (0x00451df0), the five archives released (0x0046b1d0:
   sounds, graphics, Terrain, Border, Interfac .drs), the language
   library (0x007c0634) freed and the base destructor 0x00416e40 run. */
typedef struct {
    void *context;
    int32_t field_1250;         /* +0x1250 */
    int32_t mode;               /* +0x1b0 */
    uint32_t video_window;      /* +0x1dc */
    void **game_screen;         /* 0x004fe490's game screen slot */
    uint32_t *language_library; /* global 0x007c0634 */
    void (*destroy_window)(void *context, uint32_t window);
    void (*remove_screen)(void *context, const char *name);  /* 0x00451df0 */
    void (*close_archive)(void *context, const char *name);  /* 0x0046b1d0 */
    void (*free_library)(void *context, uint32_t library);
    void (*base_destroy)(void *context);                     /* 0x00416e40 */
} AoeGameRorDestroyOpsRor;
void aoe_game_ror_destroy_ror(AoeGameRorDestroyOpsRor *ops);

/* 0x00502a40(name): the name is copied to +0xae8, "Scenario Editor
   Screen" closed, vtable +0xc(0); 1 when 0x00502ad0 (the editor screen)
   opens it; otherwise 0x00504e50(name, 0), and when that fails the main
   menu (0x00500c60), and when that fails too 0x00419930. Returns 0. */
typedef struct {
    void *context;
    char scenario_name[0x104];  /* +0xae8 */
    void (*remove_screen)(void *context, const char *name);  /* 0x00451df0 */
    void (*set_state)(void *context, int32_t state);         /* vtable +0xc */
    int (*open_editor)(void *context, const char *name);     /* 0x00502ad0 */
    int (*fallback)(void *context, const char *name, int32_t flag); /* 0x00504e50 */
    int (*main_menu)(void *context);                         /* 0x00500c60 */
    void (*give_up)(void *context);                          /* 0x00419930 */
} AoeGameRorEditorOpsRor;
int aoe_game_ror_open_editor_ror(AoeGameRorEditorOpsRor *ops, const char *name);

/* 0x00506300 (from 0x00500740): with a video window up (+0xc00): when the
   display mode was switched (+0xc08), the mode is restored
   (0x00437370(+0xc10, +0xc14, 8)), the primary blanked (0x00437490), the
   palette (+0xc18) set (0x00438170), +0xc08 cleared, the lost-surface
   check and restore flags (0x004374d0, 0x00437610), 0x004197b0 and the
   global 0x00580e20 set to 1. Then always: blank, palette, 0x0041c290,
   the main window (+0x10) invalidated, vtable +0xc(2), vtable +0xb0, the
   arrow cursor set (0x00419d40) and shown (0x00419bb0); +0xc00 cleared. */
typedef struct {
    void *context;
    int32_t video_up;           /* +0xc00 */
    int32_t mode_switched;      /* +0xc08 */
    int32_t width, height;      /* +0xc10, +0xc14 */
    void *palette;              /* +0xc18 */
    int32_t *redraw_flag;       /* global 0x00580e20 */
    /* step ids: see AOE_GAME_ROR_VSTEP_* */
    void (*step)(void *context, int32_t id, int32_t a, int32_t b);
} AoeGameRorVideoCloseOpsRor;
enum {
    AOE_GAME_ROR_VSTEP_SET_MODE = 1,  /* 0x00437370(a, b, 8) */
    AOE_GAME_ROR_VSTEP_BLANK,         /* 0x00437490 */
    AOE_GAME_ROR_VSTEP_PALETTE,       /* 0x00438170(palette) */
    AOE_GAME_ROR_VSTEP_CHECK_LOST,    /* 0x004374d0 */
    AOE_GAME_ROR_VSTEP_CLEAR_RESTORED,/* 0x00437610 */
    AOE_GAME_ROR_VSTEP_4197B0,        /* 0x004197b0 */
    AOE_GAME_ROR_VSTEP_41C290,        /* 0x0041c290 */
    AOE_GAME_ROR_VSTEP_INVALIDATE,    /* InvalidateRect(+0x10, NULL, 1) */
    AOE_GAME_ROR_VSTEP_STATE,         /* vtable +0xc(a) */
    AOE_GAME_ROR_VSTEP_B0,            /* vtable +0xb0 */
    AOE_GAME_ROR_VSTEP_ARROW_CURSOR,  /* LoadCursorA(IDC_ARROW), 0x00419d40 */
    AOE_GAME_ROR_VSTEP_SHOW_CURSOR    /* 0x00419bb0 */
};
void aoe_game_ror_close_video_ror(AoeGameRorVideoCloseOpsRor *ops);

/* 0x00506130: the video window's set-up (1 at once when +0xc00 is set):
   "Blank Screen" shown (0x00451be0(name, 0)), 0x00419c20, double size
   (+0xc04) 1, changed (+0xc08) and 16-bit (+0xc0c) 0, the palette
   (+0xc18) taken from the draw system (+0x48 +8). In full screen (draw
   system +0x479 == 2): the cursor hidden (SetCursor(NULL), class cursor
   0), 0x004498c0, the old size (+0x2c, +0x30) kept in +0xc10/+0xc14, and
   unless the "8BITVIDEO" switch is given: 640x480x16 when available
   (0x00436f70) is set (0x00437370; success also sets 16-bit), else (and
   when the screen is not 640 wide) 640x480x8 likewise, each logged
   (0x0042c330). Then the result logged, vtable +0xc(1), +0xac, +0xc00 1.
   Returns 1. */
typedef struct {
    void *context;
    int32_t video_up, double_size, mode_switched, mode_16bit; /* +0xc00..+0xc0c */
    int32_t old_width, old_height;                            /* +0xc10, +0xc14 */
    int full_screen;          /* draw system +0x479 == 2 */
    int32_t screen_width, screen_height; /* draw system +0x2c, +0x30 */
    int eight_bit_switch;     /* 0x0041bfe0("8BITVIDEO") */
    void (*prepare)(void *context);      /* "Blank Screen", 0x00419c20, palette */
    void (*hide_cursor)(void *context);  /* SetCursor, SetClassLongA, 0x004498c0 */
    int (*mode_available)(void *context, int32_t w, int32_t h, int32_t bits); /* 0x00436f70 */
    int (*set_mode)(void *context, int32_t w, int32_t h, int32_t bits);       /* 0x00437370 */
    void (*log)(void *context, const char *text);                            /* 0x0042c330 */
    void (*state)(void *context, int32_t state);                             /* vtable +0xc */
    void (*after)(void *context);                                            /* vtable +0xac */
} AoeGameRorVideoOpenOpsRor;
int aoe_game_ror_open_video_ror(AoeGameRorVideoOpenOpsRor *ops);


/* 0x00501470: the performance title (+0x106c) and view line (+0x404).
   The first call notes the time (0x007c0530; bit 0 of 0x007c0528). With
   no frames counted (+0x508) the title is "f?"; else with 0x007c0648
   clear the per-frame averages "t%lu,f%lu,avg(r(v%lu,m%lu,o%lu),s%lu,
   u%lu,c%lu,l%lu,p%lu,o%lu)" of rates +0x520 / +0x524 and timing slots
   6, 7, 17, 3, 1, 14, 12 + 10, 13, 4 (their +4 over the frames), else
   the maxima "t%lu,f%lu,max(r(v%lu,m%lu,o%lu),s%lu,u%lu,c%lu,ls%lu,
   lg%lu,p%lu,o%lu)" from the slots' +0x1c (6, 7, 17, 3, 1, 14, 10, 12,
   13, 4). The view line is "avg view time=%lu, count=%lu" (slot 6 over
   the view count +0x50c) or "avg view time=???" without views. */
void aoe_game_ror_stats_text_ror(const AoeGameBaseRor *g, int show_max, char *title, int title_size,
                                 char *view, int view_size);

/* 0x005016d0: the AI debug lines. With a world (+0x3f4 +0x40) and its
   current player (+0x7c): when 0x00454150 reports 1 (debug), line 0 is
   "Player #%d: %s" with the player's AI text (vtable +0x14(1)) or
   "Player #%d: AI is not functional." without, and lines 1..5 the AI
   texts 2..6 ("" when none); otherwise line 0 is the text of slot 0,
   line 1 "Player #%d is not a computer player." and the rest "". When
   the debug panel (+0xc24) is up the six lines are shown (0x004821b0),
   in reverse order in debug mode. */
typedef struct {
    void *context;
    int has_world;
    int32_t player;            /* world +0x7c */
    int debug_mode;            /* 0x00454150() == 1 */
    int panel_up;              /* +0xc24 */
    const char *(*ai_text)(void *context, int32_t slot); /* player vtable +0x14 */
    void (*show)(void *context, const char *const lines[6]); /* 0x004821b0 */
} AoeGameRorAiLinesOpsRor;
void aoe_game_ror_ai_lines_ror(AoeGameRorAiLinesOpsRor *ops, char lines[6][256]);


/* 0x005003f0: play the video `name`. Any open video window (+0xbec) is
   stopped first (MCI_STOP when its MCIWNDM_GETMODE text is not
   "stopped"), closed (WM_CLOSE) and forgotten; +0xbf0 takes `after`.
   With videos disabled (+0x97e == 1) or no name the close path
   (0x00500740(1)) runs and 0 is returned. The file is "<avi dir
   (settings +0x1130)><name>.avi", else "<CDPath>game\avi\<name>.avi" when
   the registry has a CDPath (0x0046aee0); missing files fail likewise.
   With the global 0x007c0644 set and the video display up (+0xc00) or
   opened (0x00506130): the MCI window (MCIWndCreateA, style
   0x50001f0a) is subclassed (0x00500710, the old procedure in +0xbf4),
   its palette set (0x00438170) unless in 16-bit mode (+0xc0c), centred
   on the settings' screen size (+0x8e0 / +0x8e4; doubled in size with
   +0xc04), 0x0041c290, the main window invalidated and painted, the
   primary blanked (0x00437490), +0xbf8 cleared, +0xbfc the time,
   MCI_PLAY sent, the wait cursor released (0x0041bdc0) and the cursor
   hidden. Returns 1; a failed create fails. */
typedef struct {
    void *window;         /* +0xbec */
    int32_t after;        /* +0xbf0 */
    int32_t field_bf8;    /* +0xbf8 */
    uint32_t start_time;  /* +0xbfc */
} AoeGameRorVideoStateRor;
typedef struct {
    void *context;
    int no_videos;             /* +0x97e == 1 */
    const char *avi_dir;       /* settings +0x1130 */
    int videos_enabled;        /* 0x007c0644 */
    int32_t *video_up;         /* +0xc00 */
    int32_t mode_16bit, double_size; /* +0xc0c, +0xc04 */
    int32_t screen_width, screen_height; /* settings +0x8e0, +0x8e4 */
    void (*window_mode)(void *context, void *window, char *out, int size); /* MCIWNDM_GETMODE */
    void (*window_stop)(void *context, void *window);                      /* MCI_STOP */
    void (*window_close)(void *context, void *window);                     /* WM_CLOSE */
    int (*file_exists)(void *context, const char *path);
    const char *(*cd_path)(void *context);                /* NULL when unset */
    int (*open_video)(void *context);                     /* 0x00506130 */
    void *(*create_window)(void *context, const char *path); /* MCIWndCreateA, subclassed */
    void (*set_palette)(void *context, void *window);     /* MCIWNDM_GETPALETTE, 0x00438170 */
    void (*client_size)(void *context, void *window, int32_t *w, int32_t *h);
    void (*move_window)(void *context, void *window, int32_t x, int32_t y, int32_t w, int32_t h);
    void (*before_play)(void *context);                   /* 0x0041c290, invalidate, paint, blank */
    uint32_t (*time_now)(void *context);
    void (*play)(void *context, void *window);            /* MCI_PLAY */
    void (*after_play)(void *context);                    /* 0x0041bdc0, cursor hidden */
    void (*close_video)(void *context);                   /* 0x00500740(1) */
} AoeGameRorVideoPlayOpsRor;
int aoe_game_ror_play_video_ror(AoeGameRorVideoStateRor *s, const AoeGameRorVideoPlayOpsRor *o,
                                int32_t after, const char *name);


/* 0x00505670 (+0xc4): the tribe game's timer tick: the base tick
   (0x0041a500; 0 when it fails) and, unless already inside (+0x1250),
   by mode (+0x1b0): 1 with a video window (+0xbec), once a second
   (+0xbfc): the window's mode text; empty or "stopped" ends the video
   (0x00500740(1)); "paused" with +0xbf8 clear sends message 0x855; the
   second noted. 3: before the comm's start (0x00425410) every 500 ms
   (0x007c0658) the wait text: string 0x454, a newline, then for every
   playing slot 1..8 (0x00423da0) other than ours (0x00423c10) a newline
   and its name (0x0041dfa0), given to the "Multiplayer Wait Screen"
   when it is up (0x00451ae0, 0x0049ebe0); past the start the game
   begins (0x5046e0). 4 / 5 / 6 with 0x00580e28 clear and a game
   screen (+0xc24): its idle (0x0047fc60). */
typedef struct {
    void *context;
    int32_t mode;                /* +0x1b0 */
    void *video_window;          /* +0xbec */
    int32_t video_field_bf8;     /* +0xbf8 */
    uint32_t *video_time;        /* +0xbfc */
    uint32_t *wait_time;         /* 0x007c0658 */
    int32_t *busy;               /* +0x1250 */
    int has_game_screen;         /* +0xc24 */
    int global_580e28;           /* 0x00580e28 */
    int (*base_tick)(void *context);                    /* 0x0041a500 */
    uint32_t (*now)(void *context);
    void (*window_mode)(void *context, void *window, char *out, int size);
    void (*end_video)(void *context);                   /* 0x00500740(1) */
    void (*window_message_855)(void *context, void *window);
    int (*comm_started)(void *context);                 /* 0x00425410 */
    void (*string)(void *context, int32_t id, char *out, int size); /* +0x20 */
    int32_t (*own_slot)(void *context);                 /* 0x00423c10 */
    int (*slot_playing)(void *context, int32_t slot);   /* 0x00423da0 */
    const char *(*slot_name)(void *context, int32_t slot); /* 0x0041dfa0 */
    int (*wait_screen_up)(void *context);               /* 0x00451ae0 */
    void (*wait_screen_text)(void *context, const char *text); /* 0x0049ebe0 */
    void (*start_game)(void *context);                  /* 0x5046e0 */
    void (*screen_idle)(void *context);                 /* 0x0047fc60 */
} AoeGameRorIdleOpsRor;
int aoe_game_ror_idle_ror(const AoeGameRorIdleOpsRor *o);

/* 0x00502d60: start the saved game or scenario file `name`. +0x1250
   cleared, the wait cursor (0x0041bd60); in mode 4 / 5 / 6 the mode
   becomes 0, the world's +0xb8 runs, the game screen (+0xc24) is
   forgotten and "Game Screen" removed. Status 0x44d; a world is ensured
   (0x005011e0). Then status 0x452, the sound (+0x60) byte +1 held
   while the world's +0xcc(name) loads, every player's wonder countdown
   cleared (0x0041c0b0), 0x00558094 set. A loaded file: settings bytes
   0x985 / 0x986 from the map's +0x8db8 / +0x8db9, 0x982 and int 0x9a4 to
   1, 0x988 to the player count - 1; the path finding (0x0041b9c0) sets
   0x00554738 to 5000 (1), 7500 (2) or 2500; slots 0..8: a world player
   gets 0x0041c090(i, i), from 1 its name (0x004244a0 from +0x44) and
   state 2 (slot 1) or 4 (0x00423d40), others 0x0041c090(i, 0) and
   state 0; the name without its extension becomes the saved and
   restart names (0x004ff4b0, 0x004ff4f0); the screens go (0x004fe490(0))
   and the game starts (0x00503a70). Otherwise: a current screen
   (0x00451ab0) with a return name (0x00454170) is shown again
   (0x00451be0), "Status Screen" removed and the wait cursor released
   (0x0041bdc0), else the status closes (0x005003e0) and the cursor is
   released; 0. */
typedef struct {
    void *context;
    int32_t *mode;               /* +0x1b0 */
    int32_t *busy;               /* +0x1250 */
    void **game_screen;          /* +0xc24 */
    int has_sound;               /* +0x60 */
    int32_t *global_558094, *global_554738;
    int16_t player_count;        /* world +0x3c */
    uint8_t map_8db8, map_8db9;  /* world +0x28 -> */
    uint8_t path_finding;        /* 0x0041b9c0 */
    int (*ensure_world)(void *context);             /* 0x005011e0 */
    const char *(*return_name)(void *context);      /* current screen's 0x00454170; NULL */
    void (*cursor)(void *context, int wait);        /* 0x0041bd60 / 0x0041bdc0 */
    void (*world_stop)(void *context);              /* world +0xb8 */
    void (*remove_screen)(void *context, const char *name); /* 0x00451df0 */
    void (*show_screen)(void *context, const char *name);   /* 0x00451be0(name, 0) */
    void (*status)(void *context, int32_t id);      /* 0x00500390(id, 0, -1) */
    void (*status_close)(void *context);            /* 0x005003e0 */
    void (*sound_hold)(void *context, int held);    /* +0x60 byte +1 */
    int (*load)(void *context, const char *name);   /* world +0xcc */
    void (*clear_wonder)(void *context, int32_t player); /* 0x0041c0b0 */
    void (*settings_bytes)(void *context, uint8_t b985, uint8_t b986, uint8_t b982, int32_t i9a4, uint8_t b988);
    void (*slot_player)(void *context, int32_t slot, int32_t player); /* 0x0041c090 */
    void (*slot_name)(void *context, int32_t slot);                   /* 0x004244a0(slot, player +0x44) */
    void (*slot_state)(void *context, int32_t slot, int32_t state);   /* 0x00423d40 */
    void (*set_names)(void *context, const char *name);  /* 0x004ff4b0 + 0x004ff4f0 */
    void (*delete_screens)(void *context);               /* 0x004fe490(0) */
    int32_t (*start)(void *context);                     /* 0x00503a70 */
} AoeGameRorStartFileOpsRor;
int32_t aoe_game_ror_start_file_ror(const AoeGameRorStartFileOpsRor *o, const char *name);


/* 0x005046e0: the game starts. "Game Screen" shown (0x00451be0(name,
   0)), "Status Screen" and "Multiplayer" removed, "game started" logged
   with a stamp (0x004fdea0). Then four unstamped log lines: "    random
   game=%d, randommap=%d, mapsize=%s, maptype=%s" (+0x30, +0x34, the map
   size string 0x2973 + size for 0..5 and the type string 0x296a + type
   for 0..6, else 0x2804); "    resources=%s, age=%s, victory=%s,
   difficulty=%s" (resources: "DeathMatch" with option 0xae5, else the
   level 0 "Default", 1 "Low", 2 "Medium", 3 "High"; age 0 "Default", 2
   "Stone", 3 "Tool", 4 "Bronze", 5 "Iron"; victory 0 "Default", 7
   "Time(%d)" and 8 "Score(%d)" of +0xa90, 9 "Standard"; the difficulty
   string 0x2bd0 + byte 0x993; unknown values "Unknown"); "    fulltech=%d,
   revealmap=%d" (bytes 0x985, 0xad9); "    numplayers=%d" (the world's
   count - 1) followed by ", players=" and each slot 1..count-1's byte
   0xac4 as "%d", ", teams=" and each one's byte 0x994 - 1, ", civs=" and
   each one's byte 0xa94 as "%d ". Then the music (+0x64) by its kind
   (+0x104): 1 CD tracks 5..14 looped (0x004502a0(5, 14, 1, 0, 0)), 2 the
   track +0x248 (0x004502a0(1, track, 1, 0, 0)), 3 stopped (0x00450870);
   +0xc1c cleared. The timing reset (0x00417310), state 4 (vtable +0xc),
   a settings pause (0x0041b750 == 1) resumed (0x00419a60(0, 0)), +0x9fc
   cleared, the chat's lines dropped (0x0041ca10) and the viewed player
   (world +0x7c) flagged (0x0041ca50(player, 1)); with a pointer (+0x1c0),
   the app active (+0x18) and the window not iconic the system cursor is
   centred (0x0044a7d0); the wait cursor released (0x0041bdc0). */
typedef struct {
    void *context;
    int32_t random_game, random_map;   /* +0x30, +0x34 */
    int32_t map_size, map_type;        /* 0x00505260, 0x00505310 */
    int32_t victory, victory_value;    /* 0x00505370, 0x00505380 */
    uint8_t death_match;               /* 0x005054e0 */
    int32_t resource_level, starting_age; /* 0x005054b0, 0x005054c0 */
    uint8_t difficulty;                /* 0x0041b990 */
    uint8_t full_tech, reveal_map;     /* 0x0041b7c0, 0x005054a0 */
    int16_t player_count;              /* world +0x3c */
    int16_t viewed_player;             /* world +0x7c */
    uint8_t (*slot_ac4)(void *context, int i);  /* 0x00505410 */
    uint8_t (*slot_994)(void *context, int i);  /* 0x0041b9b0 */
    uint8_t (*slot_a94)(void *context, int i);  /* 0x005053b0 */
    int32_t music_kind;                /* +0x64 -> +0x104; 0 for no music */
    int32_t music_track;               /* +0x64 -> +0x248 */
    int32_t *menu_music_held;          /* +0xc1c */
    int32_t *field_9fc;                /* +0x9fc */
    int settings_paused;               /* 0x0041b750 == 1 */
    int can_centre_cursor;             /* +0x1c0, +0x18 and not IsIconic */
    void (*show_screen)(void *context, const char *name);
    void (*remove_screen)(void *context, const char *name);
    void (*log)(void *context, const char *text, int stamped);
    void (*string)(void *context, int32_t id, char *out, int size); /* +0x20 */
    void (*music_tracks)(void *context, int32_t first, int32_t last); /* 0x004502a0(f, l, 1, 0, 0) */
    void (*music_stop)(void *context);                           /* 0x00450870 */
    void (*timing_reset)(void *context);                         /* 0x00417310 */
    void (*set_state)(void *context, int32_t state);             /* vtable +0xc */
    void (*resume)(void *context);                               /* 0x00419a60(0, 0) */
    void (*chat_drop)(void *context);                            /* 0x0041ca10 */
    void (*chat_recipient)(void *context, int32_t player);       /* 0x0041ca50(player, 1) */
    void (*centre_cursor)(void *context);                        /* 0x0044a7d0 */
    void (*cursor_release)(void *context);                       /* 0x0041bdc0 */
} AoeGameRorStartOpsRor;
void aoe_game_ror_start_game_ror(const AoeGameRorStartOpsRor *o);


/* 0x004fef10 (+0x98 override): the 17 common sounds into +0x6c (count
   +0x68 = 17; a failed table gives 0): "button1.wav" 0xc47c, "button2.wav",
   "chatrcvd.wav", "cantdo.wav", "tribute.wav", "resdone.wav",
   "pkilled.wav", "pdropped.wav", "mstart.wav", "mdone.wav", "mkilled.wav",
   "artheld.wav", "artlost.wav", "convwarn.wav", "convdone.wav", "res.wav",
   "farmdie.wav" 0xc48c (each 0x004a2be0 with the sound system +0x60; a
   failed allocation leaves NULL), then every loaded one preloaded
   (0x004a2ca0(0, -1)). 1. */
#define AOE_GAME_ROR_SOUNDS 17
extern const char *const aoe_game_ror_sound_names_ror[AOE_GAME_ROR_SOUNDS];
typedef struct {
    void *context;
    void *(*alloc_zero)(void *context, uint32_t count, uint32_t size); /* 0x0052607b */
    void *(*load)(void *context, const char *name, int32_t id);        /* 0x004a2be0 */
    void (*preload)(void *context, void *sound);                       /* 0x004a2ca0 */
} AoeGameRorSoundOpsRor;
int aoe_game_ror_load_sounds_ror(const AoeGameRorSoundOpsRor *o, void ***sounds, int16_t *count);


/* 0x00503cb0 (player, text): a chat line checked for cheats. 0 without
   text, for a player outside the world's count or with cheats off
   (settings byte 0x982). The text is upper-cased and letter-coded
   (0x004fde40); the coded form is compared whole, the plain form
   searched (strstr). Codes to 0x0050c550 (the viewed player's world
   command): "HOME RUN" 0x66 (with 0x0041b750 only), STORMBILLY 0xe,
   "CONVERT THIS!" 0xf, "BIG MOMMA" 0x10, POW 0x11, GRANTLINKSPENCE 0x12,
   "KING ARTHUR" 0x13 (these six searched), "PHOTON MAN" 0, "E=MC2
   TROOPER" 1, "JACK BE NIMBLE" 2, "BIG BERTHA" 3, "FLYING DUTCHMAN" 4,
   UPSIDFLINTMOBILE 7, HOYOHOYO 8, "DARK RAIN" 10, "BLACK RIDER" 0xb,
   MEDUSA 0xc, ICBM 0xd; with 0x0041b750: DIEDIEDIE 100, KILL1..KILL7
   0xc9..0xcf and KILL9 0xd0; "HARI KARI" 0x65, RESIGN 0x67; GAIA with
   0x0041b750 clears the viewed player's selection (0x0045dcb0), runs the
   game's +0x10(0, 0) in edit mode 6 with +0x1b8 == 1 and its +0x14(0),
   and gives 1 (without it the original passes an uninitialised code; the
   port passes -1); "PEPPERONI PIZZA" 0x14, COINAGE 0x15, WOODSTOCK 0x17,
   QUARRY 0x16, STEROIDS 0x18, BIGDADDY 0xdc, "REVEAL MAP" 0xe6, "NO FOG"
   0xe7; ZEUS runs the player's +0x54(player, 0, 0, 0, 0). Anything else
   0; a code gives 1. The letter code maps A and Q alike, so a coded
   comparison accepts either. */
typedef struct {
    void *context;
    int16_t player_count;      /* world +0x3c */
    int cheats_on;             /* 0x0041b7a0 */
    int flag_41b750;           /* 0x0041b750 */
    int32_t edit_mode, field_1b8; /* game +0x1b4, +0x1b8 */
    void (*cheat)(void *context, int32_t code);         /* 0x0050c550 */
    void (*gaia)(void *context, int game_10);           /* 0x0045dcb0, +0x10(0, 0)?, +0x14(0) */
    void (*player_54)(void *context, int32_t player);   /* players[player] +0x54(player, 0, 0, 0, 0) */
} AoeGameRorCheatOpsRor;
int aoe_game_ror_cheat_ror(const AoeGameRorCheatOpsRor *o, int32_t player, const char *text);


/* 0x00501980 (+0x?: kind, a, b, c, d): a game notification shown on the
   game screen (+0xc24; nothing without it). "Us" is the world's viewed
   player (+0x7c); sounds through 0x0041c120; texts through the game's
   strings (+0x20) and formats (+0x28, kind 0x67); chat lines through
   0x00480760; the first message line through 0x00482260(1, text, 0x55,
   0); the view history through 0x005064d0(c, d). Kinds (a is a player
   unless noted): 1 / 2 / 3 ours: sound 3, format 0x66 / 0x67 / 0x69 as
   the first line. 4 (a comm slot): another player's slot not yet
   dropped (0x007c0500[a]) marks it, sound 7 and, for a player above 0,
   string 0xbc4 with the name as chat. 5 another player above 0: sound
   6, 0xbc5 (b 0) / 0xbc4 with the name. 6 another: sound 6, above 0
   0xbc6 with the name. 7 (b ours): sound 4, 0xbc8 / 0xbc7 / 0xbc1 (c 3)
   / 0xbc9 formatted with player a's name and a. 8: the timer panels
   (0x004839a0); b ours: 0xbd2 (c 0) / 0xbd1 (c 1) / 0xbd3 (c 3) / none,
   with player a's name. 100 ours: sound 5 for b 0x65..0x67 else 0xf,
   the buttons rebuilt (0x004822a0), the view remembered. 0x66 / 0x69
   ours: the buttons rebuilt and the view remembered. 0x68 ours: sound 3,
   format 1000 with b as the first line; 0x7d ours: format 1 with b.
   0x6b ours: sound 3, format 0x6a. 0x6c: sound 8, 0xbf2 (ours) or 0xbcb
   with a's name as chat, the view remembered. 0x6d: sound 9; a not us:
   0xbce / 0xbef (world +0x108) / 0xbcd when both players are allied
   with us (0x0045da60 0, their +0x108 set) with a's name, ours 0xbcc /
   0xbee; the view remembered, the chat shown and the timers refreshed.
   0x6e (world +0x108 clear): sound 10, 0xbcf ours else 0xbd0 with the
   name; likewise. 0x72 / 0x74: sound 0xb, 0xbd4 / 0xbd9 (ours, or both
   allied), 0xbd5 / 0xbda, 0xbf0 / 0xbf1 with +0x108; the chat shown,
   the timers refreshed and every player told (+0xe8(a's id, i, kind, 0,
   0, 0)). 0x73 / 0x75 (+0x108 clear): sound 0xc, 0xbd6 / 0xbdb as chat
   with the timers. 0x76..0x7a ours: sound 3, strings 0xbe7..0xbeb as the
   first line. 0x7b: sound 0xd when a is not us but b is; the view
   remembered. 0x7c ours: the buttons rebuilt. 0x7e ours: sound 0x10, the
   view remembered. */
enum {
    AOE_GN_SOUND = 1,       /* 0x0041c120(a) */
    AOE_GN_FIRST_LINE,      /* 0x00482260(1, text, 0x55, 0) */
    AOE_GN_CHAT,            /* 0x00480760(text) */
    AOE_GN_BUTTONS,         /* 0x004822a0 */
    AOE_GN_VIEW_REMEMBER,   /* 0x005064d0(c, d) */
    AOE_GN_TIMERS,          /* 0x004839a0 */
    AOE_GN_TELL_PLAYER      /* players[a] +0xe8(b, a, kind, 0, 0, 0) */
};
typedef struct {
    void *context;
    int has_game_screen;
    int16_t viewed_player, player_count;
    int32_t world_108;
    int32_t *dropped;            /* 0x007c0500[] */
    const char *(*player_name)(void *context, int32_t player);
    int (*player_108)(void *context, int32_t player);
    int (*stance)(void *context, int32_t player);   /* 0x0045da60 (0 allied) */
    void (*string)(void *context, int32_t id, char *out, int size);
    void (*format)(void *context, int32_t id, int32_t value, char *out, int size); /* +0x28(0x67, id, value) */
    void (*step)(void *context, int32_t id, int32_t a, int32_t b, const char *text);
} AoeGameNotifyOpsRor;
void aoe_game_ror_notify_ror(const AoeGameNotifyOpsRor *o, int32_t kind, int32_t a, int32_t b,
                             int32_t c, int32_t d);

#endif
