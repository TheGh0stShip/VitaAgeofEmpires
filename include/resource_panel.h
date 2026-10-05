#ifndef VITA_AOE_RESOURCE_PANEL_H
#define VITA_AOE_RESOURCE_PANEL_H

#include <stdint.h>

#include "ui_panel.h"

/* Patched Rise of Rome 1.0a game-screen resource bar (tpnl_inv, vtable
   0x00549d98, 0x004f7c00..0x004f83d0): the player's wood, food, gold and
   stone (attributes 1, 0, 3, 2) as whole numbers at x + 0x20, 0x63, 0xa6
   and 0xe9, each with a shadow one pixel down-left. It redraws at most
   every half second while the game runs (modes 4..7) and only when a
   value's whole part changed. */

typedef struct {
    int16_t attribute_count;   /* player +0x4c */
    const float *attributes;   /* player +0x50 */
} AoeResourcePlayerRor;

typedef struct {
    AoeUiPanelRor panel;
    void *picture;             /* +0xf4 (freed, never set here) */
    const AoeResourcePlayerRor *player; /* +0xf8 */
    int16_t cached_count;      /* +0xfc */
    int16_t *cached;           /* +0x100 */
    uint32_t last_ms;          /* +0x104 */
    uint32_t interval_ms;      /* +0x108, 500 */
    uintptr_t font;            /* +0x10c */
    uint32_t color, shadow;    /* +0x110 0xffffff, +0x114 0 */
    const int32_t *game_mode;  /* game +0x1b0 */
    /* The tooltip formats (strings 120046, 120045, 120047, 120048). */
    const char *(*string)(void *context, int32_t id);
    void *string_context;
} AoeResourcePanelRor;

const AoeUiPanelVtblRor *aoe_resource_panel_vtable_ror(void);

/* 0x004f7c00 constructor and setup (area, parent, font, player); 0 when
   setup fails. 0x004f7d10 destructor (deleting 0x004f7cd0). */
int aoe_resource_panel_init_ror(AoeResourcePanelRor *r, AoeUiSystemRor *system,
                                AoeUiAreaRor *area, AoeUiPanelRor *parent,
                                uintptr_t font,
                                const AoeResourcePlayerRor *player);
/* 0x004f81d0 the player (refreshes the cache and redraws); 0x004f81f0 the
   cache (resized when the attribute count changes). */
void aoe_resource_panel_set_player_ror(AoeResourcePanelRor *r,
                                       const AoeResourcePlayerRor *player);
void aoe_resource_panel_cache_ror(AoeResourcePanelRor *r);

#endif
