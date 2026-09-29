#include "unit_placement_check.h"

#include <assert.h>
#include <string.h>

int main(void) {
    ScnTile tiles[9];
    uint8_t shapes[9] = {0};
    uint8_t view[9] = {0};
    AoeObjectUpdateList objects[9] = {0};
    AoeRuntimeMap map = {
        .width = 3, .height = 3, .tiles = tiles, .shapes = shapes,
        .view_codes = view, .objects = objects
    };
    float restriction[4] = {1, 1, 1, 1};
    DatUnit candidate = {.present = 1, .size = {0.4f, 0.4f, 1.0f},
                         .placement_clearance = {0.5f, 0.5f},
                         .placement_side_terrain = {-1, -1},
                         .placement_terrain = {-1, -1}};
    DatUnit existing_template = {.present = 1,
                                 .size = {0.4f, 0.4f, 1.0f}};
    AoeLiveUnitCore existing = {.unit_template = &existing_template,
                                .unit_id = 77,
                                .position = {1.5f, 1.5f, 0},
                                .state = 2};
    AoeObjectUpdateNode node = {.object = &existing};
    AoeUnitPlacementCheckContext context = {&map, restriction, 4, 0};
    int32_t blocker = -1;
    memset(tiles, 0, sizeof(tiles));
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 1.5f, 1.5f, &blocker,
               0, 0, 0, 0, 1, 0) == AOE_PLACEMENT_OK);
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 0.2f, 1.5f, &blocker,
               0, 0, 0, 0, 1, 0) == AOE_PLACEMENT_OUT_OF_BOUNDS);
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 1.5f, 1.5f, &blocker,
               1, 0, 0, 0, 1, 0) == AOE_PLACEMENT_NOT_VISIBLE);
    view[4] = 0x0f;
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 1.5f, 1.5f, &blocker,
               1, 0, 0, 0, 1, 0) == AOE_PLACEMENT_OK);
    shapes[4] = 1;
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 1.5f, 1.5f, &blocker,
               0, 2, 0, 0, 1, 0) == AOE_PLACEMENT_SHAPE);
    shapes[4] = 0;
    restriction[0] = 0;
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 1.5f, 1.5f, &blocker,
               0, 0, 0, 0, 1, 0) == AOE_PLACEMENT_TERRAIN_RESTRICTED);
    restriction[0] = 1;
    candidate.placement_terrain[0] = 2;
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 1.5f, 1.5f, &blocker,
               0, 0, 0, 0, 1, 0) == AOE_PLACEMENT_TERRAIN_PAIR);
    candidate.placement_terrain[0] = -1;
    objects[4].head = &node;
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 1.5f, 1.5f, &blocker,
               0, 0, 0, 0, 1, 1) == AOE_PLACEMENT_OCCUPIED);
    assert(blocker == 77);
    existing_template.placement_overlap_mode = 1;
    candidate.placement_overlap_mode = 0;
    assert(aoe_unit_placement_check_ror(
               &candidate, &context, 1.5f, 1.5f, &blocker,
               0, 0, 0, 0, 1, 1) == AOE_PLACEMENT_OK);
    return 0;
}
