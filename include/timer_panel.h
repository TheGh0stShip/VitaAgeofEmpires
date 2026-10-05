#ifndef VITA_AOE_TIMER_PANEL_H
#define VITA_AOE_TIMER_PANEL_H

#include <stdint.h>

#include "draw_system.h"
#include "ui_panel.h"

/* Patched Rise of Rome 1.0a game-screen clock and countdown line
   (tpnl_tim, vtable 0x0054a050, 0x004fac10..0x004fb710): one line in a
   player's colour, rendered like the population counter (a "time message
   panel" surface copied out keyed on colour 1, four-way shadow). Modes:
   0 hidden; 1 real seconds since the refresh and 2 the world's game time,
   both as "%02ld:%02ld:%02ld (%3.1f)" with the game speed, left aligned;
   right aligned, with the value rounded up to a multiple of five, 3 the
   world's +0x118 years ("%d Years", 11300), 4 the player's wonder (game
   +0x9d8[player], "Wonder: %d Years", 11301), 5 its artifacts (+0x848,
   11302) and 6 its ruins (+0x844, 11303) countdowns, where -1 reads 0. */

typedef struct {
    void *context;
    /* World player `index` (world +0x40[index]); its number (+0x4a) and
       colour (+0x10c +0x26). */
    const void *(*world_player)(void *context, int32_t index);
    int16_t (*player_number)(void *context, const void *player);
    int16_t (*player_color)(void *context, const void *player);
    /* The world's game time in milliseconds (+0x04), speed (+0x18) and
       +0x118 years. */
    uint32_t (*world_ms)(void *context);
    float (*world_speed)(void *context);
    float (*world_years)(void *context);
    /* Game +0x9d8[number]; player +0x848 and +0x844 (-1 for none). */
    int32_t (*wonder_years)(void *context, int16_t number);
    float (*artifact_years)(void *context, const void *player);
    float (*ruin_years)(void *context, const void *player);
    void (*string)(void *context, int32_t id, char *out, int size);
} AoeTimerPanelOpsRor;

typedef struct {
    AoeUiPanelRor panel;
    uintptr_t font;            /* +0xf4: game font 11 */
    uint32_t color, shadow;    /* +0xf8 0xffffff, +0xfc */
    int32_t mode, drawn_mode;  /* +0x100, +0x104 */
    int32_t kind, drawn_kind;  /* +0x108: 1 clock, 2 countdown; +0x10c */
    const void *player;        /* +0x118 */
    uint32_t start_ms;         /* +0x11c */
    int32_t value, last_value; /* +0x120, +0x124 */
    int32_t shown, drawn_shown;/* +0x128, +0x12c (-1) */
    int32_t has_region;        /* +0x130 */
    AoeDrawSurfaceRor *buffer; /* +0x134 */
    int32_t buffer_rect[4];    /* +0x138 */
    int32_t source_rect[4];    /* +0x148 */
    int32_t dest_rect[4];      /* +0x158 */
    int32_t buffer_ready;      /* +0x168 */
    int32_t buffer_w, buffer_h;/* +0x16c, +0x170 (-1) */
    const AoeTimerPanelOpsRor *ops;
} AoeTimerPanelRor;

const AoeUiPanelVtblRor *aoe_timer_panel_vtable_ror(void);

/* 0x004fac10 constructor and setup (area, parent; game font 11);
   0x004fad30 destructor (deleting 0x004fad10). */
int aoe_timer_panel_init_ror(AoeTimerPanelRor *t, AoeUiSystemRor *system,
                             const AoeTimerPanelOpsRor *ops, AoeUiAreaRor *area,
                             AoeUiPanelRor *parent, uintptr_t font);
/* 0x004faf50: the world and the real-time start. */
void aoe_timer_panel_refresh_ror(AoeTimerPanelRor *t, uint32_t now_ms);
/* 0x004faf80: the mode and world player (nothing when both are as they
   are). The player's colour: 0 blue on grey 0xb3b3b3, else on black; no
   player white. Active unless mode 0; redraws, and runs the idle. */
void aoe_timer_panel_set_ror(AoeTimerPanelRor *t, int32_t mode, int32_t index);
/* 0x004fb710: render into the buffer (1 when done). */
int aoe_timer_panel_render_ror(AoeTimerPanelRor *t);
/* Slots: +0x0c 0x004fadc0 size (as the population counter's), +0x30
   0x004fb0f0 draw, +0x4c 0x004fb5b0 idle (the value; a change redraws it
   and the parent), +0xbc 0x004fb540 the copied rectangle. */
void aoe_timer_panel_draw_ror(AoeUiPanelRor *p);
int32_t aoe_timer_panel_idle_ror(AoeUiPanelRor *p);

#endif
