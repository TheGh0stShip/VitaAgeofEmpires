#ifndef VITA_AOE_LIVE_UNIT_PLACEMENT_H
#define VITA_AOE_LIVE_UNIT_PLACEMENT_H

#include "live_unit_core.h"
#include "object_update_list.h"

/* Semantic split of the original unit's +0x1c tile pointer, +0x20 attached
   live unit, and the node in the tile's +0x0c object list. Patched RoR
   0x004a8f40 reads the attached unit's state byte at +0x48. */
typedef struct {
    AoeObjectUpdateList *tile;
    AoeObjectUpdateNode *node;
    AoeLiveUnitCore *attached_unit;
} AoeLiveUnitPlacement;

typedef struct {
    void *context;
    /* RoR vtable +0xdc / +0xd8, invoked only for state 2 on a tile change.
       The underlying game effects are not reconstructed here. */
    void (*leave_tile)(void *context, AoeLiveUnitCore *unit);
    void (*enter_tile)(void *context, AoeLiveUnitCore *unit);
} AoeLiveUnitPlacementHooks;

/* Draw-offset tail of RoR 0x004ab160. Fractions are the clamped position's
   within-tile X/Y. surface_z is the shape-derived terrain height before the
   template elevation policy; unit->position.z is the policy-selected height.
   Mode 0 copies the primary pair, mode 1 uses surface_z for the secondary Y,
   and other modes retain the prior secondary pair, matching the executable.
   Integer conversion truncates and stores the original low 16 bits. */
int aoe_live_unit_draw_offsets_ror(AoeLiveUnitCore *unit,
                                   const DatMapGeometry *geometry,
                                   uint8_t elevation_shape,
                                   float fraction_x, float fraction_y,
                                   float surface_z);

/* Rise of Rome 0x004ab160 tile-list/coordinate/height path. `tile_lists`
   mirrors the original map's row[y][x]. The terrain array must use that
   same coordinate system; scenario-to-runtime orientation is not established
   merely by calling this routine. The caller retains both arrays and unit.
   No game hook is silently omitted for a state-2 tile change. */
int aoe_live_unit_place_ror(AoeLiveUnitCore *unit,
                            AoeLiveUnitPlacement *placement,
                            const ScnTile *terrain_tiles,
                            AoeObjectUpdateList *tile_lists,
                            const DatMapGeometry *geometry,
                            uint32_t width, uint32_t height,
                            float x, float y, float requested_z,
                            const AoeLiveUnitPlacementHooks *hooks);

#endif
