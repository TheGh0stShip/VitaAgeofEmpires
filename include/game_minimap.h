#ifndef VITA_AOE_GAME_MINIMAP_H
#define VITA_AOE_GAME_MINIMAP_H

#include <stdint.h>

#include "diamond_map.h"

/* Patched Rise of Rome 1.0a game minimap: the game layer over the
   diamond map (vtable 0x0054a73c, 0x00518010..0x00518690: mouse input)
   and the game screen's layer over that (vtable 0x0054a3f4,
   0x005085a0..0x00508690: frame redraws and the flashing of the viewer's
   last-drawn objects).

   Mouse, while the game runs (states 4..7) and is not waiting on the
   network: with mouse style 2 (game +0x0c +0x8dc) the left button drags
   the view and the right button orders, otherwise the reverse. A click
   on the map in edit mode 0 orders the selection there (player vtable
   +0xa8, when map orders are on, global 0x0055873c); edit modes 4 and 5
   use +0xa0 and +0xa4 and then return to mode 0. With style 2 a
   right-click with the second flag goes to player vtable +0xd8 (x, y,
   1.0). With the other style a right-click leaves an edit mode, or
   clears the selection. */

typedef struct {
    void *context;
    int32_t (*state)(void *context);         /* game +0x1b0 */
    int32_t (*comm_value)(void *context);    /* 0x004199c0 */
    int32_t (*mouse_style)(void *context);   /* game +0x0c +0x8dc */
    int (*orders_enabled)(void *context);    /* 0x0055873c */
    void (*set_edit_mode)(void *context, int32_t mode, int32_t value);
    void (*clear_selection)(void *context, const void *player);
    /* Player vtable +0xa0 / +0xa4 / +0xa8 (0, x, y) and +0xd8 (x, y, 1). */
    uint8_t (*player_order)(void *context, const void *player, int32_t slot,
                            float x, float y);
    void (*player_point)(void *context, const void *player, float x, float y);
    /* 0x0047efb0 on the game screen: its frame pieces redrawn. */
    void (*redraw_frame)(void *context, int32_t redraw);
} AoeGameMinimapOpsRor;

typedef struct AoeGameMinimapRor AoeGameMinimapRor;

typedef struct {
    AoeDiamondMapVtblRor base;
    uint8_t (*order_here)(AoeGameMinimapRor *, int32_t, int32_t);   /* +0xf8 */
    uint8_t (*order_a0)(AoeGameMinimapRor *, int32_t, int32_t);     /* +0xfc */
    uint8_t (*order_a4)(AoeGameMinimapRor *, int32_t, int32_t);     /* +0x100 */
} AoeGameMinimapVtblRor;

struct AoeGameMinimapRor {
    AoeDiamondMapRor map;
    uint32_t flash_ms;           /* +0x16c */
    int32_t flash;               /* +0x170 */
    const AoeGameMinimapOpsRor *game_ops;
};

const AoeGameMinimapVtblRor *aoe_game_minimap_vtable_ror(void);

/* 0x00518010 then 0x005085a0: the constructors; 0x005085c0 the deleting
   destructor (the diamond map's). */
void aoe_game_minimap_init_ror(AoeGameMinimapRor *g, AoeUiSystemRor *system,
                               const AoeDiamondMapOpsRor *ops,
                               const AoeGameMinimapOpsRor *game_ops);

/* Slots: +0x20 0x005085e0, +0x30 0x00518490 (the base draw, timed),
   +0x70 0x00518040 (presses only on the map), +0x84 0x00518090 left
   down, +0x8c / +0xa0 0x005182b0 drags, +0x90 0x00518110 left up, +0x98
   0x00518230 right down, +0xa4 0x00518320 right up, +0xe8 0x00508610
   objects (the flash toggles every 333 ms), +0xf0 0x00508650 dots (pass
   4 in 0x56 or 0xff by the flash), +0xf8 0x005184c0, +0xfc 0x00518550,
   +0x100 0x005185f0. */
int32_t aoe_game_minimap_left_down_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                       int32_t c, int32_t s);
int32_t aoe_game_minimap_left_up_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                     int32_t c, int32_t s);
int32_t aoe_game_minimap_right_down_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                        int32_t c, int32_t s);
int32_t aoe_game_minimap_right_up_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                      int32_t c, int32_t s);
int32_t aoe_game_minimap_drag_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                  int32_t c, int32_t s);

#endif
