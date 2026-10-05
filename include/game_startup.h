#ifndef VITA_AOE_GAME_STARTUP_H
#define VITA_AOE_GAME_STARTUP_H

#include <stdint.h>

#include "game_settings.h"

/* Patched Rise of Rome 1.0a base game start-up and shutdown (vtable
   0x005430b4, 0x004162b0..0x00419c00): the start-up configuration the
   program entry passes in (game +0x0c), the command line, the registry
   values, the start-up chain and its error codes, and the checks it
   makes. Windows calls (window, mutex, CD volume, memory status, message
   loop, registry) are ops. Semantic storage; original offsets noted. */

/* The start-up configuration (game +0x0c). */
typedef struct {
    int32_t cd_required;        /* +0x408 */
    int32_t cd_players;         /* +0x40c: players per CD */
    char volume_label[32];      /* +0x2fd */
    char command_line[256];     /* +0x518 */
    int32_t check_expiry;       /* +0x87c */
    int16_t expiry_year, expiry_month, expiry_day; /* +0x880..+0x884 */
    int32_t single_instance;    /* +0x88c */
    int32_t no_startup;         /* +0x890 */
    int32_t full_screen;        /* +0x894 */
    int32_t check_depth;        /* +0x89c: need an 8-bit desktop */
    int32_t system_memory;      /* +0x8a0 */
    int32_t music;              /* +0x8a4 */
    int32_t sound;              /* +0x8a8 */
    int32_t music_cd;           /* +0x8ac */
    int32_t music_8b0;          /* +0x8b0 */
    int32_t music_midi;         /* +0x8b4 */
    int32_t music_8b8;          /* +0x8b8 */
    int32_t scroll_speed;       /* +0x8c8, also +0x8d0 */
    int32_t scroll_speed_8d0;
    int16_t mouse_style;        /* +0x8dc: 2 one-button */
    int32_t screen_w, screen_h; /* +0x8e0, +0x8e4 */
} AoeStartupConfigRor;

/* Game fields the start-up code sets (base game object offsets). */
typedef struct {
    AoeStartupConfigRor *config;  /* +0x0c */
    int32_t error;                /* +0x40: 0 or a start-up error code */
    int32_t custom_mouse;         /* +0x54 */
    int32_t msync;                /* +0x1c8 */
    int32_t rollover_text;        /* +0xa08 */
    float game_speed;             /* +0xa0c (the settings' float_a0c) */
    int32_t difficulty;           /* +0xa10 */
    /* Path Finding is settings +0xa14 (0x0041b9c0 / 0x0041bbf0), MP Path
       Finding +0x983 (0x0041b9d0 / 0x0041bc00). */
    AoeGameSettingsRor *settings;
    int32_t cd_ok;                /* +0x9ac */
    /* Globals set by the command line. */
    int32_t mfill;                /* 0x00557388 */
    int32_t resend1, resend2;     /* 0x005817c0, 0x005817c4 */
} AoeGameStartupRor;

/* 0x00417ba0: the command line (upper-cased, first 255 characters):
   NOSTARTUP / NO_STARTUP / NO STARTUP, SYSTEMMEMORY ..., MIDIMUSIC ...
   (MIDI music: +0x8a4 on, +0x8ac / +0x8b0 / +0x8b8 off, +0x8b4 on),
   MSYNC, MFILL (cleared when absent), NOSOUND ..., 640 / 800 / 1024
   (screen size), RESEND1= / RESEND2= (the digits up to a space),
   NOMUSIC ... (or no sound) turns music off; custom mouse on for an
   8-bit full screen, NORMALMOUSE ... off. The three spellings of each
   word: no separator, an underscore, a space. */
int aoe_game_parse_command_line_ror(AoeGameStartupRor *g);

/* The registry values read at start-up (0x004166b0) and written at
   shutdown (0x00416e40). `get` returns -1 for a missing value. */
typedef struct {
    void *context;
    int32_t (*get)(void *context, const char *name);
    void (*set)(void *context, const char *name, int32_t value);
} AoeRegistryOpsRor;

/* Screen Size (640/800/1024 -> 480/600/768; 1280 -> 1024), Rollover Text
   (2 off), Mouse Style (1 or 2), Game Speed (/ 10, scale 0x00543200),
   Difficulty, Path Finding / MP Path Finding (1..3 -> 0..2), Scroll Speed
   (10..200 into both scroll fields). */
void aoe_game_read_registry_ror(AoeGameStartupRor *g, const AoeRegistryOpsRor *r);
/* Screen Size (the draw system's width when there is one), Rollover Text
   (2 off, 1 on), Mouse Style (2, else 1), Game Speed (x 10), Difficulty,
   Path Finding and MP Path Finding (+1), Scroll Speed. */
void aoe_game_write_registry_ror(const AoeGameStartupRor *g, int32_t screen_width,
                                 const AoeRegistryOpsRor *r);

/* 0x004174e0: the string for a message of kind 1 (start-up error codes
   1..0x17) or kind 2 (the code itself as a string ID); 0 for none. */
int32_t aoe_game_error_string_id_ror(int32_t kind, int32_t code);

/* 0x004181e0: the screen mode. Without the 8-bit desktop when asked:
   error 0x13. Tries the configured width, then 640, 800, 1024 (skipping
   the configured one) and finally 1024 for a 1280 setting, storing the
   size that worked. `try_mode(width)` stands for 0x00436f70(width, 0, 8).
   Returns 0 when none works. */
int aoe_game_choose_screen_ror(AoeGameStartupRor *g, int32_t desktop_bits,
                               int (*try_mode)(void *context, int32_t width),
                               void *context);

/* 0x00418a20: the music source (1 CD, 2 MIDI, 4 +0x8b0, 3 +0x8b8 else 1)
   and its volume: a Music Volume of 9900 or more (as -volume) turns music
   off (returns -1); otherwise the volume to apply (-5000 by default). */
int32_t aoe_game_music_source_ror(const AoeStartupConfigRor *c);
int32_t aoe_game_music_volume_ror(AoeStartupConfigRor *c, int32_t registry_value);

/* 0x00419530: the CD check. With players > 0, true when players <= CD
   players x the slots of human players (0x00423d10 == 2) whose
   0x0041b800 is set. Otherwise true without a CD requirement or with the
   override (0x00580de4), else the CDPath drive must be a CD (type 5) with
   the configured volume label (case-insensitive). */
typedef struct {
    void *context;
    int32_t (*slot_kind)(void *context, int32_t slot);      /* 0x00423d10 */
    int (*slot_flag)(void *context, int32_t index);         /* 0x0041b800 */
    int override_cd;                                        /* 0x00580de4 */
    /* The CDPath registry string, its drive type and volume label. */
    const char *(*cd_path)(void *context);
    uint32_t (*drive_type)(void *context, const char *root);
    int (*volume_label)(void *context, const char *root, char *out, int size);
} AoeCdCheckOpsRor;
int aoe_game_cd_check_ror(const AoeStartupConfigRor *c, int32_t players,
                          const AoeCdCheckOpsRor *ops);

/* 0x00419670: not expired: today (seconds) <= the expiry date's
   mktime (month - 1). */
int aoe_game_not_expired_ror(const AoeStartupConfigRor *c, int64_t now,
                             int64_t (*make_time)(int year, int month0, int day));

/* 0x004196e0: another instance (the mutex already exists) is brought to
   the front (closed instead under LOBBY); returns 1 to continue. */
typedef struct {
    void *context;
    int (*mutex_exists)(void *context);          /* CreateMutexA + 0xb7 */
    void *(*find_window)(void *context);         /* the class and title */
    int (*lobby)(void *context);                 /* 0x0041bfe0("LOBBY") */
    void (*close_window)(void *context, void *w);
    void (*bring_to_front)(void *context, void *w);
    void (*release_mutex)(void *context, int close);
} AoeInstanceOpsRor;
int aoe_game_single_instance_ror(const AoeInstanceOpsRor *ops);

/* 0x004166b0's start-up chain: each step's failure code (in order): the
   game's +0x6c (2), the language library (1), memory (0x16: under 20 MB
   physical or 10 MB page file, and under 10 / 20 MB, and under 30 MB
   page file), +0x148, expiry (3), single instance (4), DirectX version
   below 5.01 without NODXCHECK (0x14), CD (stored), +0x70 (5), +0x74 (6),
   +0x78 (7 unless set), +0x7c (0x11), +0xa0 (7), +0x144 (7), +0x80 (8),
   +0x94 (10), +0x8c (0x10), +0x90 (9), +0x98 (0xb), +0x9c (0xc), then the
   comm object (0x0043ba80) and +0xa4 (either failing: 0xd). The CD
   check's result is stored (+0x9ac) between, not tested. */
enum {
    AOE_START_STEP_6C, AOE_START_STEP_LIBRARY, AOE_START_STEP_MEMORY,
    AOE_START_STEP_148, AOE_START_STEP_EXPIRY, AOE_START_STEP_INSTANCE,
    AOE_START_STEP_DIRECTX, AOE_START_STEP_70, AOE_START_STEP_74,
    AOE_START_STEP_78, AOE_START_STEP_7C, AOE_START_STEP_A0,
    AOE_START_STEP_144, AOE_START_STEP_80, AOE_START_STEP_94,
    AOE_START_STEP_8C, AOE_START_STEP_90, AOE_START_STEP_98,
    AOE_START_STEP_9C, AOE_START_STEP_COMM, AOE_START_STEP_A4,
    AOE_START_STEPS
};
typedef struct {
    void *context;
    /* Run a step: 1 success. Memory: physical and page file (bytes);
       DirectX: the version (0x0043cf60) and NODXCHECK. */
    int (*step)(void *context, int step);
    void (*memory)(void *context, uint32_t *physical, uint32_t *page_file);
    int (*nodxcheck)(void *context);
    uint32_t (*directx_version)(void *context);
} AoeStartupChainOpsRor;
/* Returns 1 when every step succeeds (the caller then shows the window,
   starts the timer and opens the log files). */
int aoe_game_startup_chain_ror(AoeGameStartupRor *g, const AoeStartupChainOpsRor *ops);
/* The memory test alone (1 enough). */
int aoe_game_enough_memory_ror(uint32_t physical, uint32_t page_file);

/* 0x00417ac0: the message loop. While the game is not active, or its
   state is neither 4 nor 2, or the comm object is waiting: block for a
   message; otherwise poll and run the idle (+0xc4) when there is none.
   WM_QUIT (0x12) ends it with its wParam; +0xc0 filters messages. */
typedef struct {
    void *context;
    int (*active)(void *context);              /* game +0x18 */
    int32_t (*state)(void *context);           /* game +0x1b0 */
    int (*comm_waiting)(void *context);        /* +0x18c and 0x004239b0 */
    /* GetMessage (0 on WM_QUIT) / PeekMessage (0 when none); the message
       and its wParam. */
    int (*get)(void *context, uint32_t *message, uint32_t *wparam);
    int (*peek)(void *context, uint32_t *message, uint32_t *wparam);
    void (*dispatch)(void *context);
    void (*idle)(void *context);               /* +0xc4 */
    int (*filter)(void *context);              /* +0xc0 */
} AoeMessageLoopOpsRor;
uint32_t aoe_game_message_loop_ror(const AoeMessageLoopOpsRor *ops);

/* 0x00418e70: the twelve fonts' string IDs (name; +1 size, +2 weight
   word) and whether the second form is wanted: 0 110, 1 113, 4 116,
   6 131, 7 119, 8 128 (1), 9 122, 10 125, 11 128; others none (0). */
int32_t aoe_game_font_string_ror(int16_t index, int *second);

/* 0x00419b20: a windowed cursor position in client coordinates, with
   values above 60000 wrapped negative. */
void aoe_game_wrap_point_ror(int32_t *x, int32_t *y);

#endif
