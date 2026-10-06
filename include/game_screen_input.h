#ifndef VITA_AOE_GAME_SCREEN_INPUT_H
#define VITA_AOE_GAME_SCREEN_INPUT_H

#include <stdint.h>

#include "game_screen.h"

/* Patched Rise of Rome 1.0a game screen key-down handler. The screen's
   chat buffer (+0x734 length, +0x738 text, 0x33 bytes), the eight group
   hotkeys (+0x784) and the four rebindable keys (+0x76c..+0x77c) are
   semantic storage; every action the handler dispatches is an op. */

enum { AOE_GS_INPUT_CHAT_LEN = 0x32, AOE_GS_INPUT_GROUP_KEYS = 8 };

typedef struct {
    int32_t chat_len;                             /* +0x734 */
    char chat[AOE_GS_INPUT_CHAT_LEN + 2];         /* +0x738..+0x76b */
    int32_t key_menu;                             /* +0x76c */
    int32_t key_a, key_b, key_c, key_d;           /* +0x770..+0x77c */
    int32_t group_keys[AOE_GS_INPUT_GROUP_KEYS];  /* +0x784 */
} AoeGameScreenInputRor;

typedef struct {
    void *ctx;
    int (*busy)(void *ctx);                                 /* 0x00451ad0 */
    int (*child_active)(void *ctx);                         /* +0x57c [0x1b] */
    int (*child_key)(void *ctx, int32_t key, int32_t count,
                     int alt, int ctrl, int shift);         /* +0x57c slot 0xac */
    int32_t (*game_state)(void *ctx);                       /* game [0x6c] */
    int32_t (*game_mode)(void *ctx);                        /* game [0x6d] */
    int32_t (*game_sub_mode)(void *ctx);                    /* game [0x6e] */
    int (*can_issue)(void *ctx);                            /* 0x0055873c */
    /* game slot 0x54: send chat; nonzero = consumed. */
    int (*send_chat)(void *ctx, const char *text);
    int (*popup_open)(void *ctx);                           /* 0x004199c0 */
    int32_t (*button_count)(void *ctx);                     /* +0x5e0 */
    int32_t (*button_id)(void *ctx, int32_t i);             /* +0x5d8 [i].4 */
    char (*button_hotkey)(void *ctx, int32_t id);
    void (*clear_popup)(void *ctx);                         /* 0x00419a20/0x004a32f0 */
    void (*press_button)(void *ctx, int32_t id);            /* 0x00484820 */
    void (*select_group)(void *ctx, int32_t n);             /* 0x00484eb0 */
    void (*cycle_idle)(void *ctx, int which);               /* 0x004858e0 */
    void (*group)(void *ctx, int32_t n, int mode);          /* 0x00485050 */
    void (*help)(void *ctx);                                /* 0x00457cd0 */
    void (*fkey)(void *ctx, int32_t n);                     /* 0x00485800 */
    void (*group_key)(void *ctx, int32_t n);                /* 0x00485390 */
    void (*key_a_action)(void *ctx);                        /* 0x00485770 */
    void (*key_b_action)(void *ctx);                        /* 0x004851d0 */
    void (*key_c_action)(void *ctx);                        /* 0x00484790 */
    void (*key_d_action)(void *ctx);                        /* 0x00485620 */
    void (*enter)(void *ctx);                               /* 0x00484a70 */
    void (*space)(void *ctx);                               /* 0x00485700 */
    void (*home)(void *ctx);                                /* 0x00506550 */
    void (*delete_key)(void *ctx);                          /* 0x00484980 */
    int (*toggle_flag)(void *ctx);                          /* viewed +0x11c */
    void (*key_s)(void *ctx, int on);                       /* 0x00484f90 */
    void (*key_t)(void *ctx);                               /* 0x00484be0 */
    int (*zoomed)(void *ctx);                               /* 0x0043c100 */
    void (*set_zoom)(void *ctx, int zoomed);                /* 0x004faf80 + panel 0x14 */
    void (*zoom_refresh)(void *ctx);                        /* 0x004839a0 + view 0x20 */
    void (*key_be)(void *ctx);                              /* 0x00485aa0 */
} AoeGameScreenInputOpsRor;

/* 0x004807b0: game screen key-down. Returns 1 when consumed. */
int aoe_game_screen_key_down_ror(AoeGameScreenInputRor *s, const AoeGameScreenInputOpsRor *o,
                                 int32_t key, int32_t count, int alt, int ctrl, int shift);

#endif
