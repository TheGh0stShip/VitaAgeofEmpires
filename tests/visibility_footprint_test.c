#include "visibility_footprint.h"

#include <assert.h>
#include <string.h>

typedef struct {
    unsigned calls;
    int32_t last_x, last_y;
} Trace;

static void record(void *context, int32_t x, int32_t y) {
    Trace *trace = context;
    trace->calls++;
    trace->last_x = x;
    trace->last_y = y;
}

int main(void) {
    int8_t counts[25];
    uint32_t masks[25] = {0};
    uint8_t view[25] = {0};
    AoeVisibilityFootprintGrid grid = {
        .width = 5, .height = 5, .counts = counts,
        .tile_masks = masks, .view_codes = view,
        .visible_mask = 0x2, .clear_visible_mask = ~0x2u,
        .explored_mask = 0x200
    };
    Trace owner = {0}, active = {0};
    AoeVisibilityFootprintOps ops = {
        .owner_context = &owner, .record_owner = record,
        .active_context = &active, .record_active = record
    };
    uint32_t discovered = 999;
    int width;
    for (unsigned i = 0; i < 25; ++i) counts[i] = -1;

    assert(aoe_visibility_footprint_half_width(2, -2, &width) && width == 1);
    assert(aoe_visibility_footprint_half_width(2, 0, &width) && width == 2);
    assert(aoe_visibility_footprint_half_width(16, 14, &width) && width == 8);
    assert(aoe_visibility_footprint_half_width(50, 16, &width) && width == 3);
    assert(!aoe_visibility_footprint_half_width(2, 3, &width));

    assert(aoe_visibility_footprint_enter(&grid, 2, 2, 2, 0, &ops,
                                           &discovered));
    assert(discovered == 21 && grid.explored_count == 21 &&
           owner.calls == 21 && active.calls == 0);
    assert(counts[12] == 1 && masks[12] == 0x202 && view[12] == 0x0f);
    assert(counts[0] == -1 && masks[0] == 0 && view[0] == 0);
    assert(aoe_visibility_footprint_enter(&grid, 2, 2, 2, 0, &ops,
                                           &discovered));
    assert(discovered == 0 && grid.explored_count == 21 && owner.calls == 21);
    assert(counts[12] == 2);
    assert(aoe_visibility_footprint_leave(&grid, 2, 2, 2));
    assert(counts[12] == 1 && view[12] == 0x0f);
    assert(aoe_visibility_footprint_leave(&grid, 2, 2, 2));
    assert(counts[12] == 0 && masks[12] == 0x200 && view[12] == 0x80);
    assert(grid.explored_count == 21);

    /* Radius-one near a corner touches only four in-bounds cells. */
    assert(aoe_visibility_footprint_enter(&grid, 0, 0, 1, 1, &ops,
                                           &discovered));
    assert(discovered == 1 && owner.calls == 22 && active.calls == 1);
    assert(counts[0] == 1 && counts[1] == 1 && counts[5] == 1 &&
           counts[6] == 1);
    assert(grid.explored_count == 22);
    assert(aoe_visibility_footprint_leave(&grid, 0, 0, 1));

    counts[0] = -7;
    grid.suspended = 0;
    assert(aoe_visibility_footprint_enter(&grid, 0, 0, 0, 0, &ops,
                                           &discovered));
    assert(counts[0] == -7 && discovered == 0);
    grid.suspended = 1;
    assert(aoe_visibility_footprint_enter(&grid, 0, 0, 0, 0, &ops,
                                           &discovered));
    assert(counts[0] == -7 && discovered == 0);
    grid.suspended = 0;
    assert(!aoe_visibility_footprint_enter(&grid, 0, 0, 0, 0, 0,
                                            &discovered));
    assert(!aoe_visibility_footprint_leave(0, 0, 0, 0));

    /* The original alternate branch visits a clipped inclusive rectangle. */
    memset(counts, 0xff, sizeof(counts));
    memset(masks, 0, sizeof(masks));
    memset(view, 0, sizeof(view));
    owner = (Trace){0};
    active = (Trace){0};
    grid.explored_count = 0;
    assert(aoe_visibility_rectangle_enter(&grid, 1, 1, 3, 2, 1,
                                           &ops, &discovered));
    assert(discovered == 6 && grid.explored_count == 6);
    assert(owner.calls == 6 && active.calls == 6);
    assert(counts[6] == 1 && counts[13] == 1 && counts[0] == -1);
    assert(masks[6] == 0x202 && view[6] == 0x0f);
    assert(aoe_visibility_rectangle_leave(&grid, 1, 1, 3, 2));
    assert(counts[6] == 0 && masks[6] == 0x200 && view[6] == 0x80);
    assert(aoe_visibility_rectangle_enter(&grid, -2, -2, 1, 1, 0,
                                           &ops, &discovered));
    assert(discovered == 3 && counts[0] == 1 && counts[6] == 1);
    assert(aoe_visibility_rectangle_leave(&grid, -2, -2, 1, 1));
    assert(aoe_visibility_rectangle_enter(&grid, 3, 3, 1, 1, 0,
                                           &ops, &discovered));
    assert(discovered == 0);
    assert(!aoe_visibility_rectangle_enter(0, 0, 0, 1, 1, 0,
                                            &ops, &discovered));
    return 0;
}
