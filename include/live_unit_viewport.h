#ifndef VITA_AOE_LIVE_UNIT_VIEWPORT_H
#define VITA_AOE_LIVE_UNIT_VIEWPORT_H

#include "live_unit_draw.h"
#include "live_unit_factory.h"
#include "tile_object_draw.h"

typedef struct {
    const AoeLiveUnitCore *type25_controller;
    uint32_t type25_hidden_players;
    uint8_t active_player;
    uint8_t owner_color;
    uint8_t template_draw_flag;
    uint8_t screen_mode;
    uint8_t screen_submode;
} AoeLiveUnitViewportStateRor;

typedef int (*AoeLiveUnitViewportResolveRor)(
    void *context, const AoeLiveUnit *unit,
    AoeLiveUnitViewportStateRor *state);

typedef struct {
    size_t visited;
    size_t drawn;
    size_t failed;
} AoeLiveUnitViewportResult;

/* Connects RoR tile traversal 0x00451fd0 to live draw 0x004a8a60. Entries
   must reference AoeLiveUnit objects created by aoe_live_unit_construct_ror;
   core is their first member and has the address retained by the tile list.
   The selected graphic, renderer, and projected anchor remain unchanged. */
int aoe_live_unit_viewport_draw_ror(
    const AoeTileObjectDrawEntry *entries, size_t count, int alternate_pass,
    void *renderer, int32_t screen_x, int32_t screen_y,
    AoeLiveUnitViewportResolveRor resolve, void *resolve_context,
    const AoeLiveUnitDrawRorOps *ops, AoeLiveUnitViewportResult *result);

#endif
