#include "runtime_map.h"
#include "live_unit_vision.h"

#include <assert.h>
#include <string.h>

typedef struct {
    unsigned calls;
    int fail;
    uint32_t x, y;
    uint8_t old_terrain;
    int16_t filter_terrain;
} PaintTrace;

static int paint_transition(void *context, uint32_t x, uint32_t y,
                            uint8_t old_terrain,
                            int16_t filter_terrain) {
    PaintTrace *trace = context;
    ++trace->calls;
    trace->x = x;
    trace->y = y;
    trace->old_terrain = old_terrain;
    trace->filter_terrain = filter_terrain;
    return !trace->fail;
}

int main(void) {
    ScnTile scenario_tiles[9] = {0};
    ScnMap scenario = {.width = 3, .height = 3,
                       .tiles = scenario_tiles};
    DatMapGeometry geometry = {.tile_half_width = 32,
                               .tile_half_height = 16,
                               .elevation_height = 16};
    DatTerrain terrain[2] = {{0}};
    DatTerrains terrains = {.count = 2, .terrains = terrain};
    DatBorder border = {0};
    DatBorders borders = {.count = 1, .borders = &border};
    AoeRuntimeMap map = {0};
    AoePlayerVisionState player = {0};
    DatUnit definition = {.present = 1, .line_of_sight = 1.0f};
    AoeLiveUnitCore unit = {.unit_template = &definition, .state = 2};
    AoeLiveUnitPlacement placement = {0};
    int32_t relations[1] = {1};
    AoeLiveUnitVisionBinding binding = {0};
    PaintTrace paint = {0};
    AoeRuntimeTerrainPaintOps paint_ops = {&paint, paint_transition};
    scenario_tiles[1].elevation = 1;
    assert(aoe_runtime_map_from_scenario(&map, &scenario, &geometry,
                                          &terrains, &borders));
    assert(map.width == 3 && map.height == 3 && map.tiles[1].elevation == 1);
    assert(map.shapes[4] == 6 && map.borders[4] == 0);
    assert(map.projected_x[4] == 64 && map.projected_y[4] == -16);
    assert(map.objects[0].count == 0 && map.view_codes[0] == 0 &&
           map.cache_flags[0] == 0 && map.draw_flags[0] == 0 &&
           map.masks.masks[0] == 0);
    memset(map.draw_flags, 0x7e, 9);
    assert(aoe_runtime_map_invalidate_ror(&map, 2, 1, -1, 4, 3));
    for (size_t i = 0; i < 9; ++i) {
        if (i < 3)
            assert(map.cache_flags[i] == 0 && map.draw_flags[i] == 0x7e);
        else
            assert(map.cache_flags[i] == 0xcc && map.draw_flags[i] == 3);
    }
    memset(map.cache_flags, 0, 9);
    assert(aoe_runtime_map_invalidate_ror(&map, -4, -3, -2, 8, 0));
    for (size_t i = 0; i < 9; ++i) assert(map.cache_flags[i] == 0);
    terrain[1].enabled = 1;
    for (unsigned shape = 0; shape < 19; ++shape)
        terrain[1].elevation_frames[shape].frame_count = 1;
    terrain[1].elevation_frames[6].frame_count = 0;
    assert(aoe_runtime_map_paint_terrain_ror(
        &map, &terrains, &borders, 1, 1, 1, 1, 1, 0, &paint_ops));
    assert(paint.calls == 0 && map.tiles[4].terrain_id == 0);
    terrain[1].elevation_frames[6].frame_count = 1;
    assert(aoe_runtime_map_paint_terrain_ror(
        &map, &terrains, &borders, 1, 1, 1, 1, 1, 0, &paint_ops));
    assert(paint.calls == 1 && paint.x == 1 && paint.y == 1 &&
           paint.old_terrain == 0 && paint.filter_terrain == 0 &&
           map.tiles[4].terrain_id == 1);
    for (size_t i = 0; i < 9; ++i)
        assert(map.cache_flags[i] == 0xcc && map.draw_flags[i] == 0);
    map.tiles[4].terrain_id = 0;
    map.cache_flags[4] = 0;
    paint.fail = 1;
    assert(!aoe_runtime_map_paint_terrain_ror(
        &map, &terrains, &borders, 1, 1, 1, 1, 1, 0, &paint_ops));
    assert(paint.calls == 2 && map.tiles[4].terrain_id == 1 &&
           map.cache_flags[4] == 0);
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
