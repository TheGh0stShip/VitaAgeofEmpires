#ifndef VITA_AOE_TILE_MAP_EDIT_H
#define VITA_AOE_TILE_MAP_EDIT_H

#include <stdint.h>

#include "player_pair_list.h"
#include "tile_map.h"

/* Patched Rise of Rome 1.0a map method 0x00447e40 (thiscall, 7 stack
   arguments, ret 0x1c): the elevation edit of a tile rectangle and the
   refresh of everything that hangs on elevation. Callers: the old map
   load 0x00444cb0 (0, 0, width - 1, height - 1, 0, 0, 0), the elevation
   clean-up 0x445570 (rect, 0, 0, 0) and the edit toolbar's PageUp /
   PageDown (view rect, 0, -1 / +1, 1).

   Map fields used: +0x08 width, +0x0c height, +0x8d8c row pointers
   (0x18-byte tiles: +0x04 shape, +0x05 terrain low 5 bits / elevation
   high 3, +0x10 object list with node +0x00 object and +0x04 next) and
   +0x8dc0 the world (+0x1d state, +0x40 players, +0x7c viewed player
   index). Here the tiles are the semantic AoeMapTileRor array; the
   object list, the world and the shape computation go through `ops`. */

typedef struct {
    void *context;
    /* 0x4484f0(x, y): the tile's elevation shape (stored at +0x04). */
    uint8_t (*shape)(void *context, int16_t x, int16_t y);
    /* Tile (x, y)'s object list (tile +0x10): the first node, NULL when
       empty; a node's next (+0x04) and object (+0x00). */
    void *(*first_node)(void *context, int16_t x, int16_t y);
    void *(*next_node)(void *context, void *node);
    void *(*node_object)(void *context, void *node);
    /* An object's position (+0x38, +0x3c) and its vtable +0x34(x, y, z),
       which places it again (its float result is dropped). */
    void (*position)(void *context, void *object, float *x, float *y);
    void (*place)(void *context, void *object, float x, float y, float z);
    /* World (map +0x8dc0) +0x1d, read for every tile; state 3 (random map
       generation) skips the touch list. */
    uint8_t (*world_state)(void *context);
    /* The viewed player (world +0x40 [world +0x7c]), read once on entry:
       its +0x6c pair list (0x0045eac0 appends). NULL for none (port). */
    AoePlayerPairListRor *(*viewed_list)(void *context);
} AoeTileMapEditOpsRor;

/* 0x00447e40(x0, y0, x1, y1, elevation, delta, apply). Only the low 16
   bits of the coordinates count. The rectangle is normalised (swapped
   when reversed), then x0 / y0 raised to 0 and x1 / y1 lowered to the
   last column / row (the other two ends are not clamped).

   With `apply` set: `delta` 0 writes `elevation` (above 8 taken as 8,
   whose << 5 leaves the 3-bit field 0) into every tile; otherwise each
   tile's elevation + delta (16-bit) is clamped to 0..0xff and its low 3
   bits stored (so 7 + 1 wraps to 0). The terrain bits stay.

   Then the rectangle grows by one tile on every side that is not at the
   map edge, and for each tile row by row: the shape from 0x4484f0, every
   object on it placed again at its own position with z 0, and, unless
   the world's state is 3, (x, y) appended to the viewed player's list.
   Finally the screen positions (0x00444aa0) and the invalidation
   (0x00444980 with flag 0) over the grown rectangle. Port: nothing for a
   NULL or empty map; a missing op is skipped. */
void aoe_tilemap_edit_elevation_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1,
                                    int32_t y1, uint8_t elevation, int16_t delta, uint8_t apply,
                                    const AoeTileMapEditOpsRor *ops);

#endif
