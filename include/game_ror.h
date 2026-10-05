#ifndef VITA_AOE_GAME_ROR_H
#define VITA_AOE_GAME_ROR_H

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
   first (0x00506300); a screen that cannot be made, or whose setup
   failed, gives 0 (the failed one deleted); else it is shown, the game
   mode becomes 2 (+0x0c) and, with a music manager and the menu music not
   held (+0xc1c), the menu music starts by the manager's source: 1 CD track
   2, 2 "xopen.mid", 3 "open.wav". */
typedef struct {
    void *context;
    int video_playing;                          /* +0xc00 */
    void (*end_video)(void *context);           /* 0x00506300 */
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

#endif
