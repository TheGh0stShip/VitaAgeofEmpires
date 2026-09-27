#ifndef VITA_AOE_RUNTIME_VIEWPORT_H
#define VITA_AOE_RUNTIME_VIEWPORT_H

#include "live_unit_viewport.h"
#include "runtime_map.h"
#include "terrain_draw.h"

typedef int (*AoeRuntimeViewportEntryResolveRor)(
    void *context, const AoeLiveUnit *unit, AoeTileObjectDrawEntry *entry);

/* Caller-owned state for composing runtime-map tile lists during the
   visibility-aware terrain walk. Scratch capacity must cover the largest
   tile list. The optional entry resolver supplies class-specific alternate
   graphic fields that are not part of the common live-unit constructor. */
typedef struct {
    const AoeRuntimeMap *map;
    AoeTileObjectDrawEntry *scratch;
    size_t scratch_capacity;
    void *renderer;
    AoeLiveUnitViewportResolveRor resolve_unit;
    void *resolve_unit_context;
    AoeRuntimeViewportEntryResolveRor resolve_entry;
    void *resolve_entry_context;
    const AoeLiveUnitDrawRorOps *draw_ops;
    AoeLiveUnitViewportResult result;
    size_t tiles;
    size_t nonempty_tiles;
} AoeRuntimeViewportRor;

/* TerrainViewTileDraw implementation joining the recovered RoR viewport
   tile order to each runtime tile's original object-list order and then to
   live-unit draw. The tile +9 value 0x80 selects the alternate object pass,
   matching RoR 0x00451fd0's caller. */
int aoe_runtime_viewport_draw_tile_ror(void *context, uint32_t tile_x,
                                       uint32_t tile_y, int32_t anchor_x,
                                       int32_t anchor_y,
                                       uint8_t visibility_mode);

#endif
