#include "visibility_footprint.h"

#include <limits.h>
#include <stddef.h>

/* Positive-row half-widths for radii 0..16. Negative rows mirror positive
   rows in the original 0x00560b60..0x005619b8 data; each original row is
   (signed y offset, -half_width, +half_width), ending in y=999. */
static const uint8_t half_widths[] = {
    0,
    1,1,
    2,2,1,
    3,3,2,1,
    4,4,3,3,1,
    5,5,5,4,3,2,
    6,6,6,5,5,4,2,
    7,7,7,6,6,5,4,2,
    8,8,8,7,7,6,5,4,2,
    9,9,9,8,8,8,7,6,5,2,
    10,10,10,10,9,9,8,7,6,5,3,
    11,11,11,11,10,10,9,9,8,7,5,3,
    12,12,12,12,11,11,10,10,9,8,7,5,3,
    13,13,13,13,12,12,12,11,10,9,8,7,6,3,
    14,14,14,14,13,13,13,12,12,11,10,9,8,6,3,
    15,15,15,15,14,14,14,13,13,12,11,10,9,8,6,3,
    16,16,16,16,15,15,15,14,14,13,13,12,11,10,8,6,3
};
static const uint8_t offsets[17] = {
    0,1,3,6,10,15,21,28,36,45,55,66,78,91,105,120,136
};

int aoe_visibility_footprint_half_width(int radius, int row, int *half_width) {
    if (!half_width || radius < 0) return 0;
    if (radius > 16) radius = 16;
    if (row < -radius || row > radius) return 0;
    *half_width = half_widths[offsets[radius] + (row < 0 ? -row : row)];
    return 1;
}

static int valid_grid(const AoeVisibilityFootprintGrid *grid) {
    return grid && grid->width && grid->height &&
           grid->width <= INT32_MAX && grid->height <= INT32_MAX &&
           (size_t)grid->width <= SIZE_MAX / grid->height &&
           grid->counts && grid->tile_masks;
}

static void apply_cell(AoeVisibilityFootprintGrid *grid, size_t index,
                       int32_t x, int32_t y, int entering,
                       int double_explore, const AoeVisibilityFootprintOps *ops,
                       uint32_t *discovered) {
    int8_t value = (int8_t)(uint8_t)((uint8_t)grid->counts[index] +
                                    (entering ? 1u : (uint8_t)-1));
    if (entering) {
        if (value == 0) {
            ++*discovered;
            grid->explored_count++;
            ops->record_owner(ops->owner_context, x, y);
            if (double_explore)
                ops->record_active(ops->active_context, x, y);
            value = 1;
            grid->tile_masks[index] |= grid->explored_mask;
        }
        if (value == 1) {
            if (grid->view_codes) grid->view_codes[index] = 0x0f;
            grid->tile_masks[index] |= grid->visible_mask;
        }
        if (value == -6) value = -7;
    } else {
        if (value == -1) value = 0;
        if (value == 0) {
            if (grid->view_codes) grid->view_codes[index] = 0x80;
            grid->tile_masks[index] &= grid->clear_visible_mask;
        }
    }
    grid->counts[index] = value;
}

static int apply(AoeVisibilityFootprintGrid *grid, int32_t center_x,
                 int32_t center_y, int radius, int entering,
                 int double_explore, const AoeVisibilityFootprintOps *ops,
                 uint32_t *new_cells) {
    uint32_t discovered = 0;
    if (!valid_grid(grid) || radius < 0 ||
        (entering && (!ops || !ops->record_owner || !new_cells ||
                      (double_explore && !ops->record_active)))) return 0;
    if (grid->suspended) {
        if (new_cells) *new_cells = 0;
        return 1;
    }
    if (radius > 16) radius = 16;
    for (int dy = -radius; dy <= radius; ++dy) {
        int64_t y = (int64_t)center_y + dy;
        int half;
        if (y < 0 || y >= grid->height ||
            !aoe_visibility_footprint_half_width(radius, dy, &half)) continue;
        for (int dx = -half; dx <= half; ++dx) {
            int64_t x = (int64_t)center_x + dx;
            if (x < 0 || x >= grid->width) continue;
            apply_cell(grid, (size_t)y * grid->width + (size_t)x,
                       (int32_t)x, (int32_t)y, entering, double_explore,
                       ops, &discovered);
        }
    }
    if (new_cells) *new_cells = discovered;
    return 1;
}

int aoe_visibility_footprint_enter(AoeVisibilityFootprintGrid *grid,
                                    int32_t center_x, int32_t center_y,
                                    int radius, int double_explore,
                                    const AoeVisibilityFootprintOps *ops,
                                    uint32_t *new_cells) {
    return apply(grid, center_x, center_y, radius, 1, double_explore,
                 ops, new_cells);
}

int aoe_visibility_footprint_leave(AoeVisibilityFootprintGrid *grid,
                                    int32_t center_x, int32_t center_y,
                                    int radius) {
    return apply(grid, center_x, center_y, radius, 0, 0, 0, 0);
}

static int apply_rectangle(AoeVisibilityFootprintGrid *grid,
                           int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                           int entering, int double_explore,
                           const AoeVisibilityFootprintOps *ops,
                           uint32_t *new_cells) {
    uint32_t discovered = 0;
    if (!valid_grid(grid) ||
        (entering && (!ops || !ops->record_owner || !new_cells ||
                      (double_explore && !ops->record_active)))) return 0;
    if (grid->suspended || x0 >= (int32_t)grid->width ||
        y0 >= (int32_t)grid->height || x1 < 0 || y1 < 0 ||
        x0 > x1 || y0 > y1) {
        if (new_cells) *new_cells = 0;
        return 1;
    }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= (int32_t)grid->width) x1 = (int32_t)grid->width - 1;
    if (y1 >= (int32_t)grid->height) y1 = (int32_t)grid->height - 1;
    for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x)
            apply_cell(grid, (size_t)y * grid->width + (size_t)x,
                       x, y, entering, double_explore, ops, &discovered);
    if (new_cells) *new_cells = discovered;
    return 1;
}

int aoe_visibility_rectangle_enter(AoeVisibilityFootprintGrid *grid,
                                    int32_t x0, int32_t y0,
                                    int32_t x1, int32_t y1,
                                    int double_explore,
                                    const AoeVisibilityFootprintOps *ops,
                                    uint32_t *new_cells) {
    return apply_rectangle(grid, x0, y0, x1, y1, 1, double_explore,
                           ops, new_cells);
}

int aoe_visibility_rectangle_leave(AoeVisibilityFootprintGrid *grid,
                                    int32_t x0, int32_t y0,
                                    int32_t x1, int32_t y1) {
    return apply_rectangle(grid, x0, y0, x1, y1, 0, 0, 0, 0);
}
