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

#endif
