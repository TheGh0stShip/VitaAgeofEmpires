#ifndef VITA_AOE_UNIT_PLACEMENT_CHECK_H
#define VITA_AOE_UNIT_PLACEMENT_CHECK_H

#include "live_unit_core.h"
#include "runtime_map.h"

#include <stddef.h>
#include <stdint.h>

enum {
    AOE_PLACEMENT_OK = 0,
    AOE_PLACEMENT_TERRAIN_PAIR = 1,
    AOE_PLACEMENT_TERRAIN_RESTRICTED = 2,
    AOE_PLACEMENT_SHAPE = 3,
    AOE_PLACEMENT_RESERVED_4 = 4,
    AOE_PLACEMENT_NOT_VISIBLE = 5,
    AOE_PLACEMENT_OCCUPIED = 6,
    AOE_PLACEMENT_OUT_OF_BOUNDS = 7
};

typedef struct {
    const AoeRuntimeMap *map;
    const float *terrain_restriction;
    size_t terrain_restriction_count;
    int bypass_visibility; /* Original screen/world mode 7 branch. */
    /* The owner's vision (player +0x38, 0x00517670): when set, a tile
       counts as seen if its mask has the owner's visible or explored bit;
       otherwise the active player's view bytes are used. */
    const uint32_t *owner_tile_masks;
    uint32_t owner_visible_mask, owner_explored_mask;
} AoeUnitPlacementCheckContext;

/* Common unit-template vtable +0x20, release RoR 0x00443700. The boolean
   arguments retain the original call surface used by training and scenario
   editor placement. Return values are the executable's 0..7 reason codes;
   `blocking_unit_id` receives the first overlapping live unit's ID for code
   6. `use_clearance` chooses template +0x5c/+0x60 instead of +0x30/+0x34. */
int aoe_unit_placement_check_ror(
    const DatUnit *unit, const AoeUnitPlacementCheckContext *context,
    float x, float y, int32_t *blocking_unit_id,
    int require_visible, uint8_t shape_mode, int reserved_flag,
    int use_clearance, int honor_overlap_mode, int check_units);

#endif
