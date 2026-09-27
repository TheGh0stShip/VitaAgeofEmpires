#include "scn.h"

#include <assert.h>

int main(void) {
    ScnTile tiles[9] = {0};
    ScnMap map = {.width = 3, .height = 3, .tiles = tiles};
    for (unsigned i = 0; i < 9; ++i) tiles[i].terrain_id = (uint8_t)i;
    assert(scn_runtime_tile_order_supported(&map));
    assert(scn_runtime_tile_at(&map, 0, 0)->terrain_id == 0);
    assert(scn_runtime_tile_at(&map, 2, 0)->terrain_id == 2);
    assert(scn_runtime_tile_at(&map, 0, 1)->terrain_id == 3);
    assert(scn_runtime_tile_at(&map, 2, 2)->terrain_id == 8);
    assert(!scn_runtime_tile_at(&map, 3, 0));
    assert(!scn_runtime_tile_at(&map, 0, 3));
    map.width = 2;
    assert(!scn_runtime_tile_order_supported(&map));
    assert(!scn_runtime_tile_at(&map, 1, 1));
    return 0;
}
