#include "unit_placement_check.h"

#include <limits.h>
#include <math.h>

static int terrain_matches(uint8_t terrain, int16_t a, int16_t b) {
    return (a >= 0 && terrain == (uint8_t)a) ||
           (b >= 0 && terrain == (uint8_t)b);
}

static int allowed_shape(uint8_t shape, uint8_t mode) {
    if (mode == 1)
        return shape == 0 || shape == 5 || shape == 6 ||
               shape == 7 || shape == 11;
    if (mode == 2) return shape == 0;
    return 1;
}

int aoe_unit_placement_check_ror(
    const DatUnit *unit, const AoeUnitPlacementCheckContext *context,
    float x, float y, int32_t *blocking_unit_id,
    int require_visible, uint8_t shape_mode, int reserved_flag,
    int use_clearance, int honor_overlap_mode, int check_units) {
    const AoeRuntimeMap *map;
    float half_x, half_y;
    int32_t min_x, max_x, min_y, max_y;
    int side_a = 0, side_b = 0, saw_visible = 0;
    (void)reserved_flag; /* Passed by the original ABI but unread. */
    if (!unit || !context || !(map = context->map) || !map->tiles ||
        !map->shapes || !map->objects || !map->width || !map->height ||
        !context->terrain_restriction || !context->terrain_restriction_count ||
        !isfinite(x) || !isfinite(y))
        return AOE_PLACEMENT_OUT_OF_BOUNDS;
    half_x = use_clearance ? unit->placement_clearance[0] : unit->size[0];
    half_y = use_clearance ? unit->placement_clearance[1] : unit->size[1];
    if (!isfinite(half_x) || !isfinite(half_y) || half_x < 0.0f ||
        half_y < 0.0f || x - half_x < 0.0f || y - half_y < 0.0f ||
        (float)map->width <= x + half_x - 0.001f ||
        (float)map->height <= y + half_y - 0.001f)
        return AOE_PLACEMENT_OUT_OF_BOUNDS;
    min_x = (int32_t)(x - half_x);
    max_x = (int32_t)(x + half_x - 0.001f);
    min_y = (int32_t)(y - half_y);
    max_y = (int32_t)(y + half_y - 0.001f);

    if (unit->placement_terrain[0] >= 0 || unit->placement_terrain[1] >= 0) {
        int32_t center_x = (int32_t)x, center_y = (int32_t)y;
        uint8_t terrain = map->tiles[(size_t)center_y * map->width + center_x]
                              .terrain_id;
        if (!terrain_matches(terrain, unit->placement_terrain[0],
                             unit->placement_terrain[1]))
            return AOE_PLACEMENT_TERRAIN_PAIR;
    }
    if (unit->placement_side_terrain[0] >= 0 ||
        unit->placement_side_terrain[1] >= 0) {
        for (int32_t px = min_x; px <= max_x; ++px) {
            int32_t ys[2] = {min_y - 1, max_y + 1};
            for (unsigned k = 0; k < 2; ++k) if (ys[k] >= 0 &&
                    (uint32_t)ys[k] < map->height) {
                uint8_t t = map->tiles[(size_t)ys[k] * map->width + px].terrain_id;
                side_a |= unit->placement_side_terrain[0] >= 0 &&
                          t == (uint8_t)unit->placement_side_terrain[0];
                side_b |= unit->placement_side_terrain[1] >= 0 &&
                          t == (uint8_t)unit->placement_side_terrain[1];
            }
        }
        for (int32_t py = min_y; py <= max_y; ++py) {
            int32_t xs[2] = {min_x - 1, max_x + 1};
            for (unsigned k = 0; k < 2; ++k) if (xs[k] >= 0 &&
                    (uint32_t)xs[k] < map->width) {
                uint8_t t = map->tiles[(size_t)py * map->width + xs[k]].terrain_id;
                side_a |= unit->placement_side_terrain[0] >= 0 &&
                          t == (uint8_t)unit->placement_side_terrain[0];
                side_b |= unit->placement_side_terrain[1] >= 0 &&
                          t == (uint8_t)unit->placement_side_terrain[1];
            }
        }
        if (!side_a && !side_b) return AOE_PLACEMENT_TERRAIN_PAIR;
    }

    for (int32_t py = min_y; py <= max_y; ++py) {
        for (int32_t px = min_x; px <= max_x; ++px) {
            size_t index = (size_t)py * map->width + px;
            uint8_t terrain = map->tiles[index].terrain_id;
            if (terrain >= context->terrain_restriction_count ||
                context->terrain_restriction[terrain] <= 0.0f)
                return AOE_PLACEMENT_TERRAIN_RESTRICTED;
            if (require_visible &&
                (context->bypass_visibility ||
                 (context->owner_tile_masks
                      ? (context->owner_tile_masks[index] &
                         (context->owner_visible_mask |
                          context->owner_explored_mask)) != 0
                      : map->view_codes && map->view_codes[index])))
                saw_visible = 1;
            if (shape_mode && !allowed_shape(map->shapes[index], shape_mode))
                return AOE_PLACEMENT_SHAPE;
        }
    }
    if (require_visible == 1 && !saw_visible)
        return AOE_PLACEMENT_NOT_VISIBLE;
    if (!check_units || (!honor_overlap_mode && unit->size[2] <= 0.0f) ||
        (half_x <= 0.0f && half_y <= 0.0f))
        return AOE_PLACEMENT_OK;

    for (int32_t py = min_y; py <= max_y; ++py) {
        for (int32_t px = min_x; px <= max_x; ++px) {
            AoeObjectUpdateNode *node =
                map->objects[(size_t)py * map->width + px].head;
            for (; node; node = node->next) {
                AoeLiveUnitCore *other = node->object;
                const DatUnit *other_template;
                float dx, dy;
                if (!other || other->state >= 7 ||
                    !(other_template = other->unit_template))
                    continue;
                if (honor_overlap_mode &&
                    other_template->placement_overlap_mode == 1 &&
                    unit->placement_overlap_mode == 0)
                    continue;
                if (other_template->size[0] <= 0.0f ||
                    other_template->size[1] <= 0.0f ||
                    ((other_template->size[2] > 0.0f) !=
                     (unit->size[2] > 0.0f)))
                    continue;
                dx = other->position.x38 - x;
                dy = other->position.y3c - y;
                if (dx < 0.0f) dx = -dx;
                if (dy < 0.0f) dy = -dy;
                if (dx < other_template->size[0] + half_x &&
                    dy < other_template->size[1] + half_y) {
                    if (blocking_unit_id) *blocking_unit_id = other->unit_id;
                    return AOE_PLACEMENT_OCCUPIED;
                }
            }
        }
    }
    return AOE_PLACEMENT_OK;
}
