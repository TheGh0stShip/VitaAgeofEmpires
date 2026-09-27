#include "player_vision_state.h"
#include "live_unit_vision.h"

#include <assert.h>

int main(void) {
    AoeVisibilityMaskMap map = {0};
    AoePlayerVisionState player = {0};
    AoeExploredCellVector growth = {0};
    uint8_t view[16] = {0};
    int32_t relations[1] = {1};
    ScnTile terrain[16] = {0};
    AoeObjectUpdateList tile_lists[16] = {0};
    DatUnit definition = {.present = 1, .line_of_sight = 1.0f};
    DatMapGeometry geometry = {.tile_half_width = 32,
                               .tile_half_height = 16};
    AoeLiveUnitCore unit = {.unit_template = &definition, .state = 2};
    AoeLiveUnitPlacement placement = {0};
    AoeLiveUnitVisionBinding binding = {0};

    assert(aoe_visibility_mask_map_init(&map, 4, 4));
    assert(aoe_player_vision_state_init(&player, &map, 0));
    assert(player.owner_cells58.capacity == 400 &&
           player.active_cells6c.capacity == 400);
    assert(player.grid.counts[0] == -1 && map.masks[0] == 0);
    assert(player.grid.visible_mask == 1 &&
           player.grid.clear_visible_mask == ~1u &&
           player.grid.explored_mask == 0x10000u);
    player.grid.view_codes = view;
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
        &unit, &placement, terrain, tile_lists, &geometry, 4, 4,
        0.25f, 0.25f, 0.0f, &binding));
    assert(player.grid.explored_count == 4 && binding.last_new_cells == 4);
    assert(player.owner_cells58.count == 4 &&
           player.active_cells6c.count == 4);
    assert(player.owner_cells58.cells[0].x == 0 &&
           player.owner_cells58.cells[0].y == 0);
    assert(player.active_cells6c.cells[0].x == 0 &&
           player.active_cells6c.cells[0].y == 0);
    assert(map.masks[0] == 0x10001u && view[0] == 0x0f);
    assert(!player.owner_cells58.allocation_failed &&
           !player.active_cells6c.allocation_failed);

    assert(aoe_explored_cells_init(&growth));
    for (int i = 0; i < 401; ++i)
        assert(aoe_explored_cells_append(&growth, i, i + 1));
    assert(growth.capacity == 800 && growth.count == 401 &&
           growth.total_appends == 401 &&
           growth.cells[400].x == 400 && growth.cells[400].y == 401);
    aoe_explored_cells_reset(&growth);
    assert(growth.count == 0 && growth.total_appends == 0 &&
           growth.capacity == 800);
    aoe_explored_cells_destroy(&growth);

    aoe_object_update_list_clear(&tile_lists[0]);
    aoe_player_vision_state_destroy(&player);
    aoe_visibility_mask_map_destroy(&map);
    return 0;
}
