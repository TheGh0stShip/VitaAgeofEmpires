#ifndef VITA_AOE_PLAYER_VISION_STATE_H
#define VITA_AOE_PLAYER_VISION_STATE_H

#include "visibility_footprint.h"

#include <stddef.h>

typedef struct {
    int32_t x, y;
} AoeExploredCell;

/* RoR 0x0045fb20 / 0x0045fa50: initial capacity 400, doubling on append.
   The original has two distinct instances at player +0x58 and +0x6c. */
typedef struct {
    AoeExploredCell *cells;
    size_t count, capacity;
    uint32_t total_appends;
    uint8_t allocation_failed;
} AoeExploredCellVector;

int aoe_explored_cells_init(AoeExploredCellVector *vector);
int aoe_explored_cells_append(AoeExploredCellVector *vector,
                              int32_t x, int32_t y);
void aoe_explored_cells_reset(AoeExploredCellVector *vector);
void aoe_explored_cells_destroy(AoeExploredCellVector *vector);
/* Callback for AoeVisibilityFootprintOps; inspect allocation_failed after
   a placement before continuing simulation. */
void aoe_explored_cells_record(void *context, int32_t x, int32_t y);

/* RoR 0x0051a890/0x0051aa20 (patched 1.0a 0x00517e30 / 0x00517f30, which
   also point the global row table at it, 0x00517fb0) shared zeroed
   32-bit tile-mask grid. */
typedef struct {
    uint32_t width, height;
    uint32_t *masks;
} AoeVisibilityMaskMap;

int aoe_visibility_mask_map_init(AoeVisibilityMaskMap *map,
                                 uint32_t width, uint32_t height);
void aoe_visibility_mask_map_destroy(AoeVisibilityMaskMap *map);

/* RoR disc 0x00519e10 (patched 0x005173d0) per-player count grid and owner masks, plus the two
   +0x58/+0x6c vectors from player constructor 0x0045c650. `view_codes`
   is a semantic row-major adapter for runtime tile +9 and should be set
   only while this player is the active player. */
typedef struct {
    AoeVisibilityFootprintGrid grid;
    int8_t *owned_counts;
    AoeExploredCellVector owner_cells58;
    AoeExploredCellVector active_cells6c;
} AoePlayerVisionState;

int aoe_player_vision_state_init(AoePlayerVisionState *state,
                                 AoeVisibilityMaskMap *shared,
                                 uint8_t owner_index);
void aoe_player_vision_state_destroy(AoePlayerVisionState *state);

#endif
