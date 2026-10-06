#ifndef VITA_AOE_LIVE_UNIT_VISION_H
#define VITA_AOE_LIVE_UNIT_VISION_H

#include "live_unit_placement.h"
#include "visibility_footprint.h"

#include <stddef.h>

/* Original RoR unit +0xd8/+0xdc hook dependencies. `owner_index` is player
   +0x4a, `active_index` is world +0x7c (-1 when no active player), and
   active_relations is active player's +0xb0 indexed by owner. A missing
   active player has no relation and cannot receive the tile-view byte.
   owner_grid->view_codes must be present exactly when active==owner. */
typedef struct {
    AoeVisibilityFootprintGrid *owner_grid;
    int16_t owner_index;
    int16_t active_index;
    const int32_t *active_relations;
    size_t relation_count;
    AoeVisibilityFootprintOps explored_ops;
    uint32_t last_new_cells;
} AoeLiveUnitVisionBinding;

/* Composes RoR 0x004ab160 placement with its original common visibility
   callbacks 0x004ac290/0x004ac380. The alternate rectangle-vision branch
   (hook flag != 0) is not taken by 0x004ab160; use the separate visibility
   change helper for state-method calls.
   The binding and referenced map/player arrays must remain valid throughout
   the call. This is still not the full live-unit factory or registration. */
int aoe_live_unit_place_ror_with_vision(
    AoeLiveUnitCore *unit, AoeLiveUnitPlacement *placement,
    const ScnTile *terrain_tiles, AoeObjectUpdateList *tile_lists,
    const DatMapGeometry *geometry, uint32_t width, uint32_t height,
    float x, float y, float requested_z,
    AoeLiveUnitVisionBinding *vision);

/* Same composition with the dynamic-class visibility mode selected by the
   caller. Type-80 units use alternate=1 while completion byte +0x1dd is zero;
   the common classes and completed buildings use zero. */
int aoe_live_unit_place_ror_with_vision_mode(
    AoeLiveUnitCore *unit, AoeLiveUnitPlacement *placement,
    const ScnTile *terrain_tiles, AoeObjectUpdateList *tile_lists,
    const DatMapGeometry *geometry, uint32_t width, uint32_t height,
    float x, float y, float requested_z,
    AoeLiveUnitVisionBinding *vision, int alternate);

/* RoR root unit +0xd8/+0xdc visibility hooks, usable by state transitions.
   `alternate` selects the original size-based rectangle rather than the
   radius table. This changes only visibility surfaces, not unit state or
   player/world registration. */
int aoe_live_unit_vision_change_ror(AoeLiveUnitCore *unit,
                                     AoeLiveUnitVisionBinding *vision,
                                     int entering, int alternate);

/* The same hooks with the radius argument of +0xd8/+0xdc(owner, mode,
   radius) (patched RoR 1.0a 0x004aaa10 enter via 0x00517860/0x00517b50 and
   0x004aab20 leave via 0x00517a40/0x00517d10; the "double explore" flag
   is owner +0x4a == world +0x7c there): a negative radius means the template's line of sight (effects
   pass an explicit radius when it changes). */
int aoe_live_unit_vision_radius_ror(AoeLiveUnitCore *unit,
                                    AoeLiveUnitVisionBinding *vision,
                                    int entering, int alternate,
                                    int32_t radius_override);

/* 0x004ad070 (type-80 +0xd8 override): an unfinished building (+0x1dd
   zero) always enters with the rectangle; a finished one keeps the
   caller's mode. Returns the mode passed on to 0x004aaa10. */
int aoe_live_unit_vision_building_mode_ror(int completed, int alternate);

#endif
