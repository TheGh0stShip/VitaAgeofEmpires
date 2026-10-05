#ifndef VITA_AOE_GAME_FRAME_H
#define VITA_AOE_GAME_FRAME_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a base game frame (0x00417a20..0x0041c120):
   the main window (class, creation, caption, close, clearing), the comm
   object's lifetime, the palette and display handlers, the mouse
   dispatch, the campaign position and small forwards. Windows, the draw
   system, the comm object and the campaign are ops; the decisions,
   arguments and order are kept here. */

typedef struct {
    void *context;
    /* The settings (game +0x0c). */
    const char *class_name;      /* +0x000 */
    const char *title;           /* +0x07a */
    uintptr_t instance;          /* +0x410 */
    int32_t existing_window;     /* +0x414 */
    const char *icon_name;       /* +0x61c */
    const char *menu_name;       /* +0x645 ("" when empty) */
    int32_t show_command;        /* +0x618 */
    int32_t windowed;            /* +0x894 */
    int32_t *width, *height;     /* +0x8e0, +0x8e4 (updated on display change) */
    uint32_t window;             /* game +0x10 */
    /* Windows. */
    int (*register_class)(void *context, uint32_t style, const char *class_name,
                          uintptr_t instance, const char *icon, const char *menu);
    uint32_t (*create_window)(void *context, const char *class_name, const char *title,
                              uint32_t style, int32_t w, int32_t h);
    void (*screen_size)(void *context, int32_t *w, int32_t *h);
    void (*window_rect)(void *context, uint32_t window, int32_t r[4]);
    void (*client_rect)(void *context, uint32_t window, int32_t r[4]);
    void (*move_window)(void *context, uint32_t window, int32_t x, int32_t y, int32_t w,
                        int32_t h);
    void (*show)(void *context, uint32_t window, int32_t command);
    void (*update)(void *context, uint32_t window);
    void (*focus)(void *context, uint32_t window);
    void (*send)(void *context, uint32_t window, uint32_t msg);
    void (*fill_black)(void *context, uint32_t window);
    void (*set_text)(void *context, uint32_t window, const char *text);
    void (*after_create)(void *context);         /* 0x00452250 */
    uint32_t *main_window;                       /* global 0x00580d7c */
} AoeGameFrameOpsRor;

/* +0x70 0x00417fe0: register the window class (style 0xb, the relay
   window procedure 0x00418080, the icon, the menu) unless a window is
   given; 1 on success. +0x74 0x004180b0: create the popup window
   (0x82080000) the size of the screen when windowed or when the screen
   is already the game size, resize its client area to the game size,
   show it (windowed) and publish it. */
int aoe_game_frame_register_ror(const AoeGameFrameOpsRor *ops);
int aoe_game_frame_create_ror(AoeGameFrameOpsRor *ops);
/* 0x00419930 close (WM_CLOSE); 0x00419ba0 UpdateWindow; 0x00419880 the
   client area black (with a window); 0x0041b390 the caption (+0x404). */
void aoe_game_frame_close_ror(const AoeGameFrameOpsRor *ops);
void aoe_game_frame_update_ror(const AoeGameFrameOpsRor *ops);
void aoe_game_frame_clear_ror(const AoeGameFrameOpsRor *ops);
void aoe_game_frame_caption_ror(const AoeGameFrameOpsRor *ops, const char *caption);
/* 0x00418080: the window procedure: the game's +0x08 (0 without a
   game). */
int32_t aoe_game_frame_relay_ror(void *game, int32_t (*proc)(void *game, uint32_t window,
                                                            uint32_t msg, uint32_t wparam,
                                                            uint32_t lparam),
                                 uint32_t window, uint32_t msg, uint32_t wparam,
                                 uint32_t lparam);

typedef struct {
    void *context;
    /* 0x0041e1c0(window, settings +0x878, chat log, +0x8e8..+0x8f4) new
       comm (0x1774 bytes); 0x0041e850 destroy; the four handlers set on it
       (0x0041e0c0 +0x190, 0x0041e0f0 +0x198, 0x004ed890 +0x19c,
       0x0041e110 +0x1a0, 0x004ed890 +0x194), the world told
       (0x0051c940), the two listeners (0x00580e14 / 0x00580e18,
       0x004259d0). */
    void *(*comm_new)(void *context);
    void (*comm_free)(void *context, void *comm);
    void (*comm_setup)(void *context, void *comm);
    int has_world;
    void (*world_comm)(void *context, void *comm);
    void *listeners[2];
    void (*listen)(void *context, void *listener, void *comm);
    void **published;            /* global 0x00580da8 */
    void (*recreate)(void *context); /* +0x90 */
    /* 0x0041c830 the chat log (global 0x00580da0). */
    void *(*chat_new)(void *context, uint32_t window);
    void **chat;
    /* 0x00452030(name +0x66e, 50500) the palette into +0x1c. */
    void *(*palette_load)(void *context, const char *name, int32_t id);
} AoeGameCommOpsRor;

/* +0x90 0x00418780: the comm object into +0x18c (0 when it fails), set
   up and published. +0x44 0x004188e0: destroyed (unpublished), then +0x90
   again. +0x8c 0x004185b0: the chat log. 0x00418480: the palette. */
int aoe_game_comm_create_ror(void **comm, const AoeGameCommOpsRor *ops);
void aoe_game_comm_restart_ror(void **comm, const AoeGameCommOpsRor *ops);
int aoe_game_chat_create_ror(const AoeGameCommOpsRor *ops, uint32_t window);
int aoe_game_palette_load_ror(void **palette, const AoeGameCommOpsRor *ops, const char *name);

typedef struct {
    void *context;
    int has_draw;                /* +0x48 */
    int full_screen_palette;     /* draw +0x478 / +0x479 == 1 */
    int32_t mode;                /* +0x1b0 */
    int has_mouse;               /* +0x14 */
    int cursor_capable;          /* +0x54 */
    int software_cursor;         /* +0x1d0 */
    int (*draw_forward)(void *context, int which, uint32_t a, uint32_t b, uint32_t c,
                        uint32_t d);  /* 0x00437a00 / 0x00437990 / 0x00437a30 */
    int32_t (*game_palette)(void *context);   /* +0xf4 */
    void (*set_palette)(void *context);       /* 0x00438200(0, 0x100, draw +0x78) */
    int (*draw_size)(void *context, int32_t *w, int32_t *h); /* draw +0x2c / +0x30 */
    int (*panel_resize)(void *context, int32_t w, int32_t h); /* focus panel +0x50 */
    void (*after_display)(void *context);     /* +0x128 */
    /* Mouse: 0x00449b20 in the client, 0x0044a2a0(0), 0x0044a850; then
       +0x104(x, y, left, right, shift, control) and +0xc4. */
    int (*in_client)(void *context);
    void (*cursor_off)(void *context);
    void (*cursor_restore)(void *context);
    void (*mouse)(void *context, int32_t x, int32_t y, uint32_t left, uint32_t right,
                  uint32_t shift, uint32_t control);
    void (*tick)(void *context);
} AoeGameDisplayOpsRor;

/* +0xf0 0x0041ad70: palette changed by another window (wparam != hwnd):
   the game's own palette (+0xf4) unless the draw system has one in
   use, else the palette entries reloaded (not in mode 1). +0xec
   0x0041acd0: the display changed (not wparam 1): the draw system told,
   the game size taken from it, the focused panel resized, +0x128.
   0x0041a600: mouse messages (with the mouse): the cursor kept in step,
   the message decoded and dispatched, then a tick. */
int32_t aoe_game_palette_changed_ror(const AoeGameDisplayOpsRor *ops, uint32_t window,
                                     uint32_t wparam, uint32_t a, uint32_t b);
int32_t aoe_game_display_changed_ror(const AoeGameDisplayOpsRor *ops, int32_t *width,
                                     int32_t *height, uint32_t a, uint32_t b, uint32_t wparam,
                                     uint32_t c);
int32_t aoe_game_mouse_message_ror(const AoeGameDisplayOpsRor *ops, uint32_t wparam,
                                   uint32_t lparam);

typedef struct {
    void *context;
    /* 0x0043ce60 campaign, 0x00451ac0 player, 0x0043ce40 scenario; the
       choosers 0x0043cca0 / 0x0043cc80 / 0x0043ccd0. */
    int32_t (*campaign)(void *context);
    int32_t (*player)(void *context);
    int32_t (*scenario)(void *context);
    int (*choose_campaign)(void *context, int32_t v);
    int (*choose_player)(void *context, int32_t v);
    int (*choose_scenario)(void *context, int32_t v);
} AoeGameCampaignOpsRor;

/* 0x00417a20 the campaign position read; 0x00417a60 chosen (campaign,
   player, scenario; 1 when all three take). */
void aoe_game_campaign_get_ror(const AoeGameCampaignOpsRor *ops, int32_t *campaign,
                               int32_t *player, int32_t *scenario);
int aoe_game_campaign_set_ror(const AoeGameCampaignOpsRor *ops, int32_t campaign,
                              int32_t player, int32_t scenario);

/* 0x0041c120: sound i (+0x6c) playing, or played (0x004a3440 /
   0x004a32f0); 0 without it. */
int aoe_game_play_sound_ror(void *const *sounds, int32_t i,
                            int (*playing)(void *context, void *sound),
                            int (*play)(void *context, void *sound), void *context);

#endif
