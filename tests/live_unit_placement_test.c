#include "live_unit_placement.h"

#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct {
    AoeLiveUnitPlacement *placement;
    AoeObjectUpdateList *old_tile;
    AoeObjectUpdateList *new_tile;
    char events[8];
    unsigned count;
} HookTrace;

static void leave_tile(void *context, AoeLiveUnitCore *unit) {
    HookTrace *trace = context;
    assert(unit && trace->placement->tile == trace->old_tile);
    assert(trace->old_tile->count == 0 && trace->new_tile->count == 1);
    trace->events[trace->count++] = 'L';
}

static void enter_tile(void *context, AoeLiveUnitCore *unit) {
    HookTrace *trace = context;
    assert(unit && trace->placement->tile == trace->new_tile);
    assert(trace->new_tile->count == 1);
    trace->events[trace->count++] = 'E';
}

int main(void) {
    ScnTile tiles[4] = {0};
    AoeObjectUpdateList lists[4] = {0};
    DatUnit def = {.present = 1, .elevation_mode = 0};
    DatMapGeometry geometry = {.tile_half_width = 32,
                               .tile_half_height = 16};
    AoeLiveUnitCore unit = {.unit_template = &def, .state = 2};
    AoeLiveUnitPlacement placement = {0};
    HookTrace trace = {.placement = &placement, .new_tile = &lists[0]};
    AoeLiveUnitPlacementHooks hooks = {&trace, leave_tile, enter_tile};
    for (unsigned i = 0; i < 4; ++i) tiles[i].elevation = 1;

    unit.position.z40 = 1.5f;
    assert(aoe_live_unit_draw_offsets_ror(&unit, &geometry, 0,
                                           0.25f, 0.5f, 1.5f));
    assert(unit.draw_left == 24 && unit.draw_top == -4 &&
           unit.draw_right == 24 && unit.draw_bottom == -4);
    def.elevation_mode = 1;
    unit.position.z40 = 3.0f;
    assert(aoe_live_unit_draw_offsets_ror(&unit, &geometry, 6,
                                           0.25f, 0.5f, 1.5f));
    assert(unit.draw_left == 24 && unit.draw_top == -12 &&
           unit.draw_right == 24 && unit.draw_bottom == 12);
    def.elevation_mode = 2;
    unit.draw_right = 111;
    unit.draw_bottom = -222;
    assert(aoe_live_unit_draw_offsets_ror(&unit, &geometry, 16,
                                           0.75f, 0.5f, 0.0f));
    assert(unit.draw_left == 40 && unit.draw_top == -20 &&
           unit.draw_right == 111 && unit.draw_bottom == -222);
    assert(!aoe_live_unit_draw_offsets_ror(&unit, &geometry, 17,
                                            0.0f, 0.0f, 0.0f));
    def.elevation_mode = 0;

    assert(aoe_live_unit_place_ror(&unit, &placement, tiles, lists, &geometry, 2, 2,
                                   -4.0f, -3.0f, 9.0f, &hooks));
    assert(placement.tile == &lists[0] && lists[0].count == 1);
    assert(lists[0].head->object == &unit);
    assert(unit.position.x38 == 0.0f && unit.position.y3c == 0.0f &&
           unit.position.z40 == 1.0f);
    assert(trace.count == 1 && trace.events[0] == 'E');

    assert(aoe_live_unit_place_ror(&unit, &placement, tiles, lists, &geometry, 2, 2,
                                   0.25f, 0.75f, 5.0f, 0));
    assert(trace.count == 1 && lists[0].count == 1);
    assert(unit.position.x38 == 0.25f && unit.position.y3c == 0.75f);

    trace.old_tile = &lists[0];
    trace.new_tile = &lists[3];
    assert(!aoe_live_unit_place_ror(&unit, &placement, tiles, lists, &geometry, 2, 2,
                                    1.5f, 1.5f, 0.0f, 0));
    assert(placement.tile == &lists[0] && lists[0].count == 1 &&
           lists[3].count == 0 && unit.position.x38 == 0.25f);
    assert(aoe_live_unit_place_ror(&unit, &placement, tiles, lists, &geometry, 2, 2,
                                   1.5f, 1.5f, 0.0f, &hooks));
    assert(placement.tile == &lists[3] && lists[0].count == 0 &&
           lists[3].count == 1 && lists[3].head->object == &unit);
    assert(trace.count == 3 && trace.events[1] == 'L' &&
           trace.events[2] == 'E');

    placement.attached_unit = &unit;
    assert(aoe_live_unit_place_ror(&unit, &placement, tiles, lists, &geometry, 2, 2,
                                   0.0f, 0.0f, 0.0f, &hooks));
    assert(placement.tile == &lists[3] && unit.position.x38 == 1.5f);
    placement.attached_unit = 0;
    unit.state = 1;
    assert(aoe_live_unit_place_ror(&unit, &placement, tiles, lists, &geometry, 2, 2,
                                   3.0f, 0.1f, 0.0f, 0));
    assert(placement.tile == &lists[1] && lists[1].count == 1);
    assert(unit.position.x38 < 2.0f && unit.position.x38 > 1.99f);
    assert(trace.count == 3);

    assert(!aoe_live_unit_place_ror(&unit, &placement, tiles, lists, &geometry, 2, 2,
                                    NAN, 0.0f, 0.0f, 0));
    assert(placement.tile == &lists[1] && lists[1].count == 1);
    for (unsigned i = 0; i < 4; ++i) aoe_object_update_list_clear(&lists[i]);
    return 0;
}
