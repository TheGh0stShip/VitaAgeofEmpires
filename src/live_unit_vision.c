#include "live_unit_vision.h"

#include <limits.h>
#include <math.h>
#include <stdint.h>

static int checked_trunc(float value, int32_t *out) {
    if (!isfinite(value) || (double)value < (double)INT32_MIN ||
        (double)value >= 2147483648.0 || !out) return 0;
    *out = (int32_t)value;
    return 1;
}

int aoe_live_unit_vision_change_ror(AoeLiveUnitCore *unit,
                                     AoeLiveUnitVisionBinding *vision,
                                     int entering, int alternate) {
    return aoe_live_unit_vision_radius_ror(unit, vision, entering, alternate,
                                           -1);
}

int aoe_live_unit_vision_radius_ror(AoeLiveUnitCore *unit,
                                    AoeLiveUnitVisionBinding *vision,
                                    int entering, int alternate,
                                    int32_t radius_override) {
    AoeVisibilityFootprintGrid *grid;
    int double_explore, radius;
    int32_t x, y, x0, y0, x1, y1;
    float line_of_sight;
    if (!unit || !unit->unit_template || !vision ||
        !(grid = vision->owner_grid) || !grid->counts || !grid->tile_masks ||
        vision->owner_index < 0 || vision->active_index < -1 ||
        (!!grid->view_codes !=
         (vision->active_index == vision->owner_index))) return 0;
    if (vision->active_index >= 0 &&
        (!vision->active_relations ||
         (size_t)vision->owner_index >= vision->relation_count)) return 0;
    line_of_sight = unit->unit_template->line_of_sight;
    if (radius_override >= 0) {
        radius = radius_override;
    } else if (!isfinite(line_of_sight) || line_of_sight >= 2147483648.0f ||
               line_of_sight < (float)INT32_MIN ||
               !checked_trunc(line_of_sight, &radius)) {
        return 0;
    }
    vision->last_new_cells = 0;
    if (radius <= 0) return 1;
    double_explore = vision->active_index >= 0 &&
        vision->active_relations[vision->owner_index] == 1;
    if (!alternate) {
        if (!checked_trunc(unit->position.x38, &x) ||
            !checked_trunc(unit->position.y3c, &y)) return 0;
        return entering ? aoe_visibility_footprint_enter(
            grid, x, y, radius, double_explore, &vision->explored_ops,
            &vision->last_new_cells) :
            aoe_visibility_footprint_leave(grid, x, y, radius);
    }
    if (!(unit->unit_template->size[0] > 0.0f) ||
        !(unit->unit_template->size[1] > 0.0f)) return 1;
    if (!checked_trunc(unit->position.x38 - unit->unit_template->size[0],
                       &x0) ||
        !checked_trunc(unit->position.y3c - unit->unit_template->size[1],
                       &y0) ||
        !checked_trunc(unit->position.x38 + unit->unit_template->size[0] -
                       0.01f, &x1) ||
        !checked_trunc(unit->position.y3c + unit->unit_template->size[1] -
                       0.01f, &y1)) return 0;
    return entering ? aoe_visibility_rectangle_enter(
        grid, x0, y0, x1, y1, double_explore, &vision->explored_ops,
        &vision->last_new_cells) :
        aoe_visibility_rectangle_leave(grid, x0, y0, x1, y1);
}

typedef struct {
    AoeLiveUnitVisionBinding *binding;
    int alternate;
    int failed;
} VisionHooks;

static void leave_vision(void *context, AoeLiveUnitCore *unit) {
    VisionHooks *hooks = context;
    if (!aoe_live_unit_vision_change_ror(
            unit,hooks->binding,0,hooks->alternate)) hooks->failed = 1;
}

static void enter_vision(void *context, AoeLiveUnitCore *unit) {
    VisionHooks *hooks = context;
    if (!aoe_live_unit_vision_change_ror(
            unit,hooks->binding,1,hooks->alternate)) hooks->failed = 1;
}

int aoe_live_unit_place_ror_with_vision_mode(
    AoeLiveUnitCore *unit, AoeLiveUnitPlacement *placement,
    const ScnTile *terrain_tiles, AoeObjectUpdateList *tile_lists,
    const DatMapGeometry *geometry, uint32_t width, uint32_t height,
    float x, float y, float requested_z,
    AoeLiveUnitVisionBinding *vision, int alternate) {
    VisionHooks context = {0};
    AoeLiveUnitPlacementHooks hooks;
    float line_of_sight;
    if (!unit || !unit->unit_template || !geometry || !vision ||
        (alternate != 0 && alternate != 1) || !vision->owner_grid ||
        !vision->owner_grid->counts || !vision->owner_grid->tile_masks ||
        vision->owner_grid->width != width ||
        vision->owner_grid->height != height ||
        vision->owner_index < 0 || vision->active_index < -1 ||
        (!!vision->owner_grid->view_codes !=
         (vision->active_index == vision->owner_index)))
        return 0;
    if (vision->active_index >= 0 &&
        (!vision->active_relations ||
         (size_t)vision->owner_index >= vision->relation_count)) return 0;
    line_of_sight = unit->unit_template->line_of_sight;
    if (!isfinite(line_of_sight) ||
        (double)line_of_sight >= 2147483648.0 ||
        (double)line_of_sight < -2147483648.0 ||
        (unit->state == 2 && line_of_sight >= 1.0f &&
         !vision->explored_ops.record_owner)) return 0;
    context.binding = vision;
    context.alternate = alternate;
    if (unit->state == 2 && line_of_sight >= 1.0f &&
        vision->active_index >= 0 &&
        vision->active_relations[vision->owner_index] == 1 &&
        !vision->explored_ops.record_active)
        return 0;
    hooks = (AoeLiveUnitPlacementHooks){&context, leave_vision, enter_vision};
    vision->last_new_cells = 0;
    if (!aoe_live_unit_place_ror(unit, placement, terrain_tiles, tile_lists,
                                 geometry,
                                 width, height, x, y, requested_z, &hooks))
        return 0;
    return !context.failed;
}

int aoe_live_unit_place_ror_with_vision(
    AoeLiveUnitCore *unit, AoeLiveUnitPlacement *placement,
    const ScnTile *terrain_tiles, AoeObjectUpdateList *tile_lists,
    const DatMapGeometry *geometry, uint32_t width, uint32_t height,
    float x, float y, float requested_z,
    AoeLiveUnitVisionBinding *vision) {
    return aoe_live_unit_place_ror_with_vision_mode(
        unit,placement,terrain_tiles,tile_lists,geometry,width,height,
        x,y,requested_z,vision,0);
}
