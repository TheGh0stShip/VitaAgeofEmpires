#ifndef VITA_AOE_GAME_WINDOW_H
#define VITA_AOE_GAME_WINDOW_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a base game window handling (vtable 0x005430b4,
   0x00419dc0..0x0041c1b0): the window procedure's routing of messages to
   the game's handler slots, and the handlers that do more than forward.
   The window system and the game's other objects are ops. */

/* Handler slots (game vtable offsets). */
enum {
    AOE_GAME_SLOT_IDLE = 0xc4,     /* WM_TIMER */
    AOE_GAME_SLOT_MOUSE = 0xc8,    /* WM_MOUSEMOVE */
    AOE_GAME_SLOT_KEY = 0xcc,      /* WM_KEYDOWN / WM_SYSKEYDOWN */
    AOE_GAME_SLOT_USER = 0xd0,     /* WM_USER */
    AOE_GAME_SLOT_COMMAND = 0xd4,  /* WM_COMMAND */
    AOE_GAME_SLOT_MCI = 0xd8,      /* MM_MCINOTIFY (0x3b9), 0x500 */
    AOE_GAME_SLOT_PAINT = 0xdc,    /* WM_PAINT */
    AOE_GAME_SLOT_ACTIVATE = 0xe0, /* WM_ACTIVATEAPP */
    AOE_GAME_SLOT_INITMENU = 0xe4, /* WM_INITMENU */
    AOE_GAME_SLOT_EXITMENU = 0xe8, /* WM_EXITMENULOOP */
    AOE_GAME_SLOT_SIZE = 0xec,     /* WM_SIZE */
    AOE_GAME_SLOT_PALETTE = 0xf0,  /* WM_PALETTECHANGED */
    AOE_GAME_SLOT_QUERYPAL = 0xf4, /* WM_QUERYNEWPALETTE */
    AOE_GAME_SLOT_CLOSE = 0xf8,    /* WM_CLOSE (and after WM_QUERYNEWPALETTE) */
    AOE_GAME_SLOT_DESTROY = 0xfc   /* WM_DESTROY */
};

typedef struct {
    void *context;
    /* The sound and music objects see every message first (0x004a2920,
       0x00450ae0). */
    void (*pre_dispatch)(void *context, uint32_t msg, uint32_t wparam, uint32_t lparam);
    /* The current screen's +0x48 (0 when none or unhandled). */
    int has_screen;
    int32_t (*screen_message)(void *context, uint32_t msg, uint32_t wparam,
                              uint32_t lparam);
    /* A handler slot: nonzero to fall through to the default procedure. */
    int (*handler)(void *context, int slot, uint32_t msg, uint32_t wparam,
                   uint32_t lparam);
    int32_t (*default_proc)(void *context, uint32_t msg, uint32_t wparam,
                            uint32_t lparam);
    /* WM_ACTIVATEAPP's cursor and palette work (while the window is up,
       +0x14), WM_SYSKEYUP Alt on the screen (its +0xe0), WM_ACTIVATE's
       focus tracking (while up), the cursor fix after a mouse move (when
       0x00580dac asks). */
    void (*activate_app)(void *context, uint32_t wparam);
    void (*screen_alt_up)(void *context);
    void (*track_focus)(void *context, uint32_t wparam, uint32_t lparam);
    void (*after_mouse)(void *context);
    /* Full screen (+0x0c +0x894), iconic, the size-message lock
       (0x00582eb8). */
    int full_screen;
    int (*iconic)(void *context);
    int size_locked;
} AoeGameWindowOpsRor;

/* 0x00419dc0. */
int32_t aoe_game_window_proc_ror(const AoeGameWindowOpsRor *ops, uint32_t msg,
                                 uint32_t wparam, uint32_t lparam);

/* 0x0041a6c0 (+0xcc): keys. Without an active window: handled (1). The
   modifiers (Ctrl, Alt, Shift) and the key are stored; Alt, Space or
   F10 as WM_SYSKEYDOWN on our full-screen, non-iconic window are
   swallowed (0); Ctrl+F12 saves "AoE%04d.bmp" (0); otherwise the key
   goes to +0x108 (key, repeat count, ctrl, alt, shift). */
typedef struct {
    void *context;
    int active;                                       /* +0x14 */
    int full_screen;
    int (*iconic)(void *context);
    int (*key_state)(void *context, int vk);          /* GetKeyState < 0 */
    void (*screenshot)(void *context);                /* 0x0043a4d0 */
    int (*key)(void *context, int32_t key, int32_t repeat, int ctrl, int alt,
               int shift);                            /* +0x108 */
    /* The stored state (0x00580d78 key, 0x00580d80 repeat, 0x00580da4
       ctrl, 0x00580d9c alt, 0x00580b74 shift). */
    int32_t last_key, last_repeat, ctrl, alt, shift;
} AoeGameKeyOpsRor;
int aoe_game_key_down_ror(AoeGameKeyOpsRor *ops, int own_window, uint32_t msg,
                          uint32_t wparam, uint32_t lparam);

/* 0x0041a7d0 (+0xd4): menu commands. With more than 12 entries in the
   comm object's list (0x00422cd0) and no lock (0x00580e24): 0x17b3 /
   0x17b4 call the slot's player +0xd4(slot, 1 / 0). 0x17a2 / 0x17a3 call
   the world's +0xec(1 / 0). Then +0x10c(id, value). */
typedef struct {
    void *context;
    int32_t (*comm_count)(void *context);
    int locked;
    void (*player_d4)(void *context, int32_t slot, int on);
    int has_world;
    void (*world_ec)(void *context, int on);
    int (*command)(void *context, int32_t id, int32_t value);
} AoeGameCommandOpsRor;
int aoe_game_command_ror(const AoeGameCommandOpsRor *ops, int32_t id, int32_t value);

/* 0x0041abb0 (+0xe0): activation. Active: a game paused by losing focus
   (+0x9fc == 1, states 4/6/7) resumes (comm 0x00419a60(0, 0)); the
   screen hears +0xe0; then +0x118. Inactive: a single-player game that
   is running (states 4/6/7, not waiting) pauses (0x00419a60(1, 0) and
   +0x9fc = 1); then +0x11c. */
typedef struct {
    void *context;
    int32_t state;                           /* +0x1b0 */
    int32_t paused_by_focus;                 /* +0x9fc */
    int single_player;                       /* 0x0041b750 */
    int (*comm_waiting)(void *context);      /* 0x004199c0 */
    void (*comm_pause)(void *context, int pause);
    int has_screen;
    void (*screen_activate)(void *context);
    void (*on_active)(void *context);        /* +0x118 */
    void (*on_inactive)(void *context);      /* +0x11c */
} AoeGameActivateOpsRor;
int aoe_game_activate_ror(AoeGameActivateOpsRor *ops, int32_t *active, int on);

/* 0x0041af30 (+0xf8 / close): window gone; the timer (when running) is
   killed and the loop told to quit. */
typedef struct {
    void *context;
    int timer_running;                       /* +0x44 */
    void (*kill_timer)(void *context);
    void (*post_quit)(void *context);
} AoeGameCloseOpsRor;
int aoe_game_close_ror(const AoeGameCloseOpsRor *ops, int32_t *window_active,
                       void **window);

/* 0x0041a500 (+0xc4): the timer tick. With a window: a palette restore
   countdown (0x00580dc8: every 250 ms, 500 ms while above 5) re-applies
   the palette; the sound object, the screen's idle (+0x4c), the comm
   object (when 0x0041b760 is set: 0x0041f330), +0x13c and +0x140 when
   their flags are set. 1 with a window. */
typedef struct {
    void *context;
    int window_ok;                                /* +0x14 and +0x10 */
    int app_active;                               /* +0x18 */
    int32_t *restore_count;                       /* 0x00580dc8 */
    uint32_t *restore_ms;                         /* 0x00580dcc */
    uint32_t (*now_ms)(void *context);
    void (*restore_palette)(void *context);       /* 0x00438200 */
    void (*sound_tick)(void *context);
    void (*screen_idle)(void *context);
    int (*comm_present)(void *context);
    void (*comm_tick)(void *context);
    int flag_8ec, flag_8f0;                       /* +0x8ec, +0x8f0 */
    void (*slot_13c)(void *context);
    void (*slot_140)(void *context);
} AoeGameTimerOpsRor;
int aoe_game_timer_ror(const AoeGameTimerOpsRor *ops);

/* 0x0041b300: the frame-rate caption ("fps=%lu, rend=%lu, view=%lu,
   map=%lu, scr=%lu, upd=%lu, snd=%lu, misc=%lu, oth=%lu, tot=%lu")
   into +0x404 from the timing table (each total divided
   by the sample count +0x504, at least 1). `totals` holds the timing
   words at +0x51c, +0x56c, +0x5ec, +0x60c, +0x58c, +0x54c, +0x66c,
   +0x6cc, +0x5ac, +0x52c. */
void aoe_game_fps_caption_ror(char *out, int size, const char *format,
                              uint32_t samples, const uint32_t totals[10]);

/* 0x0041bd60: the wait cursor (IDC_WAIT, when Windows draws the cursor),
   0x004522d0, +0x1d8 = 0, and the capture (+0x1dc) taken again.
   0x0041bdc0: the capture released, 0x004522c0, +0x1d8 = 1, the arrow. */
typedef struct {
    void *context;
    int windows_cursor;                           /* +0x1cc */
    void (*set_cursor)(void *context, int wait);  /* LoadCursor + 0x00419d40 */
    void (*call_4522d0)(void *context);
    void (*call_4522c0)(void *context);
    void *capture;                                /* +0x1dc */
    /* Release any capture, then (when `set`) capture `window`. */
    void (*recapture)(void *context, void *window, int set);
} AoeGameCursorOpsRor;
void aoe_game_wait_cursor_ror(const AoeGameCursorOpsRor *ops, int32_t *flag_1d8);
void aoe_game_arrow_cursor_ror(const AoeGameCursorOpsRor *ops, int32_t *flag_1d8);

/* Relays: 0x004174c0 (+0x1c: +0x28 with a 0 third argument), 0x0041a890
   (+0xd0 -> +0x110), 0x0041a8b0 (+0xd8 -> +0x114), 0x0041ac90 (+0xe4 ->
   +0x120), 0x0041acb0 (+0xe8 -> +0x124): each "nonzero" of the target. */
int aoe_game_relay_ror(int32_t (*target)(void *context), void *context);

#endif
