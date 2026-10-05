#ifndef VITA_AOE_POPULATION_PANEL_H
#define VITA_AOE_POPULATION_PANEL_H

#include <stdint.h>

#include "draw_system.h"
#include "ui_panel.h"

/* Patched Rise of Rome 1.0a game-screen population counter (tpnl_tim,
   vtable 0x00549f6c, 0x004fa150..0x004fac10): "<label><pop>/<limit>"
   (string 11050, "pop: " in languagex.dll; "%s%ld/%ld"), centred with a
   four-way shadow. The text is rendered into a "time message panel"
   surface the panel's size and copied out keyed on colour 1; without
   that surface it is drawn straight onto the area. When the player is at
   the population limit (game +0xa34[player] > 0) the text flashes: the
   flash colour for a world second, then a plain second, then lit again
   while still at the limit. */

typedef struct {
    void *context;
    /* The viewed player (0x00419b70): its number (+0x4a) and attributes
       (+0x50[index]: 11 population, 4 houses, 32 the cap). */
    const void *(*player)(void *context);
    int16_t (*player_number)(void *context, const void *player);
    float (*attribute)(void *context, const void *player, int index);
    /* Game +0xa34[number]: at the population limit when above zero. */
    int32_t (*limit_flag)(void *context, int16_t number);
    /* The world's seconds per frame (world +0x3c +0x84). */
    float (*frame_seconds)(void *context, const void *player);
    void (*string)(void *context, int32_t id, char *out, int size);
} AoePopulationPanelOpsRor;

typedef struct {
    AoeUiPanelRor panel;
    uintptr_t font;            /* +0xf4: game font 11 */
    uint32_t color;            /* +0xf8 0xffffff */
    uint32_t shadow;           /* +0xfc 0 */
    uint32_t flash_color;      /* +0x100 0xffff */
    /* +0x108 (the world's player table) is not kept. */
    const void *player;        /* +0x10c */
    int32_t population;        /* +0x110 */
    int32_t limit;             /* +0x114 */
    uint8_t flash;             /* +0x118: 0 off, 1 lit, 2 dark */
    float flash_seconds;       /* +0x11c */
    int32_t has_region;        /* +0x120 */
    AoeDrawSurfaceRor *buffer; /* +0x124 */
    int32_t buffer_rect[4];    /* +0x128 */
    int32_t source_rect[4];    /* +0x138 */
    int32_t dest_x, dest_y;    /* +0x148, +0x14c */
    int32_t dest_right, dest_bottom; /* +0x150, +0x154 */
    int32_t buffer_ready;      /* +0x158 */
    int32_t buffer_w, buffer_h;/* +0x15c, +0x160, -1 */
    const AoePopulationPanelOpsRor *ops;
} AoePopulationPanelRor;

const AoeUiPanelVtblRor *aoe_population_panel_vtable_ror(void);

/* 0x004fa150 constructor and setup (area, parent; game font 11); 0x004fa260
   destructor (deleting 0x004fa240). */
int aoe_population_panel_init_ror(AoePopulationPanelRor *pp,
                                  AoeUiSystemRor *system,
                                  const AoePopulationPanelOpsRor *ops,
                                  AoeUiAreaRor *area, AoeUiPanelRor *parent,
                                  uintptr_t font);
/* 0x004fa480: the player and its numbers (population = attribute 11,
   limit = attributes 4 + 11 capped by 32). */
void aoe_population_panel_refresh_ror(AoePopulationPanelRor *pp);
/* 0x004fa970: render the text into the buffer (1 when done). */
int aoe_population_panel_render_ror(AoePopulationPanelRor *pp);
/* Slots: +0x0c 0x004fa2f0 size (a new buffer when it grows or would
   reach the area's right edge), +0x30 0x004fa4f0 draw, +0x4c 0x004fa960
   idle (the base, 0), +0xbc 0x004fa8f0 the copied rectangle while
   buffered. */
void aoe_population_panel_set_rect_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                       int32_t w, int32_t h);
void aoe_population_panel_draw_ror(AoeUiPanelRor *p);

#endif
