#include "live_unit_placement.h"

#include <math.h>
#include <stdint.h>

static int truncated_word(float value, int16_t *out) {
    int32_t integer;
    uint16_t word;
    if (!out || !isfinite(value) || (double)value < (double)INT32_MIN ||
        (double)value >= 2147483648.0) return 0;
    integer = (int32_t)value;
    word = (uint16_t)(uint32_t)integer;
    *out = word <= INT16_MAX ? (int16_t)word :
           (int16_t)((int32_t)word - 65536);
    return 1;
}

int aoe_live_unit_draw_offsets_ror(AoeLiveUnitCore *unit,
                                   const DatMapGeometry *geometry,
                                   uint8_t elevation_shape,
                                   float fraction_x, float fraction_y,
                                   float surface_z) {
    float baseline, diagonal, vertical;
    int16_t left, top, right, bottom;
    if (!unit || !unit->unit_template || !geometry ||
        geometry->tile_half_width <= 0 || geometry->tile_half_height <= 0 ||
        elevation_shape > 16 || !isfinite(fraction_x) ||
        !isfinite(fraction_y) || !isfinite(surface_z) ||
        !isfinite(unit->position.z40) ||
        !(fraction_x >= 0.0f && fraction_x < 1.0f) ||
        !(fraction_y >= 0.0f && fraction_y < 1.0f)) return 0;
    switch (elevation_shape) {
    case 2: case 6: case 8: case 10: case 14: case 15: case 16:
        baseline = 1.0f;
        break;
    default:
        baseline = 0.0f;
        break;
    }
    diagonal = fraction_x + fraction_y;
    vertical = baseline + fraction_y + 1.0f - fraction_x;
    if (!truncated_word(diagonal * (float)geometry->tile_half_width, &left) ||
        !truncated_word((vertical - unit->position.z40) *
                        (float)geometry->tile_half_height, &top)) return 0;
    right = unit->draw_right;
    bottom = unit->draw_bottom;
    if (unit->unit_template->elevation_mode == 0) {
        right = left;
        bottom = top;
    } else if (unit->unit_template->elevation_mode == 1) {
        if (!truncated_word(diagonal * (float)geometry->tile_half_width,
                            &right) ||
            !truncated_word((vertical - surface_z) *
                            (float)geometry->tile_half_height, &bottom))
            return 0;
    }
    unit->draw_left = left;
    unit->draw_top = top;
    unit->draw_right = right;
    unit->draw_bottom = bottom;
    return 1;
}

int aoe_live_unit_place_ror(AoeLiveUnitCore *unit,
                            AoeLiveUnitPlacement *placement,
                            const ScnTile *terrain_tiles,
                            AoeObjectUpdateList *tile_lists,
                            const DatMapGeometry *geometry,
                            uint32_t width, uint32_t height,
                            float x, float y, float requested_z,
                            const AoeLiveUnitPlacementHooks *hooks) {
    AoeObjectUpdateList *next_tile;
    AoeObjectUpdateNode *next_node = 0;
    float surface, next_z, fraction_x, fraction_y;
    uint32_t tile_x, tile_y;
    uint8_t shape;
    AoeLiveUnitCore projected;
    if (!unit || !unit->unit_template || !placement || !terrain_tiles ||
        !tile_lists || !geometry || !width || !height || width > INT32_MAX ||
        height > INT32_MAX || (size_t)width > SIZE_MAX / height ||
        !isfinite(x) || !isfinite(y) || !isfinite(requested_z)) return 0;

    /* Original +0x20 guard returns before changing coordinates or tile. */
    if (placement->attached_unit) return 1;
    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;
    if (x >= (float)width) x = (float)width - 0.001f;
    if (y >= (float)height) y = (float)height - 0.001f;
    tile_x = (uint32_t)x;
    tile_y = (uint32_t)y;
    fraction_x = x - (float)tile_x;
    fraction_y = y - (float)tile_y;
    next_tile = &tile_lists[(size_t)tile_y * width + tile_x];
    shape = terrain_elevation_shape(terrain_tiles, width, height,
                                    tile_x, tile_y);

    if (!terrain_surface_height_at(terrain_tiles, width, height, x, y,
                                   &surface) ||
        !terrain_unit_surface_z(unit->unit_template, requested_z, surface,
                                &next_z)) return 0;
    projected = *unit;
    projected.position = (AoeVisibilityUnitRef){x, y, next_z};
    if (!aoe_live_unit_draw_offsets_ror(&projected, geometry, shape,
                                         fraction_x, fraction_y, surface))
        return 0;
    if (next_tile != placement->tile) {
        if (unit->state == 2 && (!hooks || !hooks->enter_tile ||
                                  (placement->tile && !hooks->leave_tile)))
            return 0;
        /* The original removes the old node before prepending. Prepending
           first leaves the old link intact if allocation fails; callbacks
           still observe the original post-relink state. */
        if (!aoe_object_update_list_attach(next_tile, unit, &next_node))
            return 0;
        if (placement->tile &&
            !aoe_object_update_list_detach(placement->tile, unit,
                                           placement->node)) {
            aoe_object_update_list_detach(next_tile, unit, next_node);
            return 0;
        }
        if (placement->tile && unit->state == 2)
            hooks->leave_tile(hooks->context, unit);
        unit->position = (AoeVisibilityUnitRef){x, y, next_z};
        unit->draw_left = projected.draw_left;
        unit->draw_top = projected.draw_top;
        unit->draw_right = projected.draw_right;
        unit->draw_bottom = projected.draw_bottom;
        placement->tile = next_tile;
        placement->node = next_node;
        if (unit->state == 2) hooks->enter_tile(hooks->context, unit);
    } else {
        unit->position = (AoeVisibilityUnitRef){x, y, next_z};
        unit->draw_left = projected.draw_left;
        unit->draw_top = projected.draw_top;
        unit->draw_right = projected.draw_right;
        unit->draw_bottom = projected.draw_bottom;
    }
    return 1;
}
