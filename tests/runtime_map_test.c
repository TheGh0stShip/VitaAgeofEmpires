#include "runtime_map.h"
#include "live_unit_vision.h"

#include <assert.h>

int main(void) {
    ScnTile scenario_tiles[9] = {0};
    ScnMap scenario = {.width = 3, .height = 3,
                       .tiles = scenario_tiles};
    DatMapGeometry geometry = {.tile_half_width = 32,
                               .tile_half_height = 16,
                               .elevation_height = 16};
    DatTerrain terrain = {0};
    DatTerrains terrains = {.count = 1, .terrains = &terrain};
    DatBorder border = {0};
    DatBorders borders = {.count = 1, .borders = &border};
    AoeRuntimeMap map = {0};
    AoePlayerVisionState player = {0};
    DatUnit definition = {.present = 1, .line_of_sight = 1.0f};
    AoeLiveUnitCore unit = {.unit_template = &definition, .state = 2};
    AoeLiveUnitPlacement placement = {0};
    int32_t relations[1] = {1};
    AoeLiveUnitVisionBinding binding = {0};
    scenario_tiles[1].elevation = 1;
    assert(aoe_runtime_map_from_scenario(&map, &scenario, &geometry,
                                          &terrains, &borders));
    assert(map.width == 3 && map.height == 3 && map.tiles[1].elevation == 1);
    assert(map.shapes[4] == 6 && map.borders[4] == 0);
    assert(map.projected_x[4] == 64 && map.projected_y[4] == -16);
    assert(map.objects[0].count == 0 && map.view_codes[0] == 0 &&
           map.masks.masks[0] == 0);
    assert(aoe_player_vision_state_init(&player, &map.masks, 0));
    assert(aoe_runtime_map_bind_player_vision(&map, &player, 1));
    binding.owner_grid = &player.grid;
    binding.owner_index = 0;
    binding.active_index = 0;
    binding.active_relations = relations;
    binding.relation_count = 1;
    binding.explored_ops = (AoeVisibilityFootprintOps){
        .owner_context = &player.owner_cells58,
        .record_owner = aoe_explored_cells_record,
        .active_context = &player.active_cells6c,
        .record_active = aoe_explored_cells_record
    };
    assert(aoe_live_unit_place_ror_with_vision(
        &unit, &placement, map.tiles, map.objects, &geometry,
        map.width, map.height,
        1.2f, 1.2f, 0.0f, &binding));
    assert(placement.tile == &map.objects[4] && map.objects[4].count == 1);
    assert(player.grid.explored_count == 9 &&
           player.owner_cells58.count == 9 &&
           player.active_cells6c.count == 9);
    assert(map.view_codes[4] == 0x0f && map.masks.masks[4] == 0x10001u);
    assert(unit.position.z40 > 0.79f && unit.position.z40 < 0.81f);
    assert(aoe_runtime_map_bind_player_vision(&map, &player, 0));
    assert(player.grid.view_codes == 0);
    assert(aoe_object_update_list_detach(placement.tile, &unit,
                                         placement.node));
    aoe_player_vision_state_destroy(&player);
    aoe_runtime_map_destroy(&map);
    scenario.width = 2;
    assert(!aoe_runtime_map_from_scenario(&map, &scenario, &geometry,
                                           &terrains, &borders));
    return 0;
}
