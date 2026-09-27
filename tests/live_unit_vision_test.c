#include "live_unit_vision.h"

#include <assert.h>
#include <string.h>

typedef struct {
    AoeVisibilityFootprintGrid *grid;
    uint32_t calls;
    uint32_t observed_count;
} Trace;

static void record(void *context, int32_t x, int32_t y) {
    Trace *trace = context;
    assert(x >= 0 && x < 4 && y >= 0 && y < 4);
    assert(trace->grid->explored_count >= trace->observed_count);
    trace->observed_count = trace->grid->explored_count;
    trace->calls++;
}

int main(void) {
    ScnTile terrain[16] = {0};
    AoeObjectUpdateList lists[16] = {0};
    int8_t counts[16];
    uint32_t masks[16] = {0};
    uint8_t view[16] = {0};
    int32_t active_relations[2] = {1, 0};
    DatUnit def = {.present = 1, .line_of_sight = 1.5f};
    DatMapGeometry geometry = {.tile_half_width = 32,
                               .tile_half_height = 16};
    AoeLiveUnitCore unit = {.unit_template = &def, .state = 2};
    AoeLiveUnitPlacement placement = {0};
    AoeVisibilityFootprintGrid grid = {
        .width = 4, .height = 4, .counts = counts,
        .tile_masks = masks, .view_codes = view,
        .visible_mask = 1u, .clear_visible_mask = ~1u,
        .explored_mask = 0x10000u
    };
    Trace owner = {.grid = &grid}, active = {.grid = &grid};
    AoeLiveUnitVisionBinding vision = {
        .owner_grid = &grid, .owner_index = 0, .active_index = 0,
        .active_relations = active_relations, .relation_count = 2,
        .explored_ops = {
            .owner_context = &owner, .record_owner = record,
            .active_context = &active, .record_active = record
        }
    };
    for (unsigned i = 0; i < 16; ++i) counts[i] = -1;

    assert(aoe_live_unit_place_ror_with_vision(
        &unit, &placement, terrain, lists, &geometry, 4, 4,
        0.2f, 0.2f, 0.0f, &vision));
    assert(placement.tile == &lists[0] && lists[0].count == 1);
    assert(grid.explored_count == 4 && vision.last_new_cells == 4);
    assert(owner.calls == 4 && active.calls == 4 && view[0] == 0x0f &&
           counts[0] == 1 && masks[0] == 0x10001u);

    assert(aoe_live_unit_place_ror_with_vision(
        &unit, &placement, terrain, lists, &geometry, 4, 4,
        1.2f, 0.2f, 0.0f, &vision));
    assert(placement.tile == &lists[1] && lists[0].count == 0 &&
           lists[1].count == 1);
    assert(grid.explored_count == 6 && vision.last_new_cells == 2);
    assert(owner.calls == 6 && active.calls == 6 &&
           counts[0] == 1 && counts[2] == 1);
    assert(view[2] == 0x0f && masks[2] == 0x10001u);

    assert(aoe_live_unit_place_ror_with_vision(
        &unit, &placement, terrain, lists, &geometry, 4, 4,
        1.7f, 0.4f, 0.0f, &vision));
    assert(vision.last_new_cells == 0 && owner.calls == 6 &&
           active.calls == 6 &&
           lists[1].count == 1);

    vision.relation_count = 0;
    assert(!aoe_live_unit_place_ror_with_vision(
        &unit, &placement, terrain, lists, &geometry, 4, 4,
        2.2f, 0.2f, 0.0f, &vision));
    assert(placement.tile == &lists[1] && lists[1].count == 1 &&
           grid.explored_count == 6);
    vision.relation_count = 2;

    /* A different active player gets no tile-view writes and relation 0
       records each newly explored cell only once. */
    vision.active_index = 1;
    active_relations[0] = 0;
    grid.view_codes = 0;
    assert(aoe_live_unit_place_ror_with_vision(
        &unit, &placement, terrain, lists, &geometry, 4, 4,
        2.2f, 0.2f, 0.0f, &vision));
    assert(placement.tile == &lists[2] && vision.last_new_cells == 2);
    assert(grid.explored_count == 8 && owner.calls == 8 &&
           active.calls == 6);
    assert(view[3] == 0 && masks[3] == 0x10001u);

    /* RoR +0xd8/+0xdc alternate mode uses size-based rectangular bounds. */
    memset(counts, 0xff, sizeof(counts));
    memset(masks, 0, sizeof(masks));
    memset(view, 0, sizeof(view));
    grid.explored_count = 0;
    grid.view_codes = view;
    vision.active_index = 0;
    active_relations[0] = 1;
    owner.calls = active.calls = 0;
    owner.observed_count = active.observed_count = 0;
    unit.position = (AoeVisibilityUnitRef){2.0f, 2.0f, 0.0f};
    def.size[0] = def.size[1] = 1.2f;
    assert(aoe_live_unit_vision_change_ror(&unit, &vision, 1, 1));
    assert(vision.last_new_cells == 16 && grid.explored_count == 16);
    assert(owner.calls == 16 && active.calls == 16);
    assert(counts[0] == 1 && counts[15] == 1 && view[15] == 0x0f);
    assert(aoe_live_unit_vision_change_ror(&unit, &vision, 0, 1));
    assert(counts[0] == 0 && counts[15] == 0 && view[15] == 0x80);
    assert(masks[15] == 0x10000u && vision.last_new_cells == 0);
    def.size[0] = 0.0f;
    assert(aoe_live_unit_vision_change_ror(&unit, &vision, 1, 1));
    assert(vision.last_new_cells == 0 && owner.calls == 16);
    def.size[0] = 1.2f;

    for (unsigned i = 0; i < 16; ++i) aoe_object_update_list_clear(&lists[i]);
    return 0;
}
