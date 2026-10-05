#include "terrain.h"

#include <limits.h>

int terrain_shape_surface_offset(uint8_t shape, float fraction_x,
                                 float fraction_y, float *offset) {
    float x = fraction_x, y = fraction_y, sum = x + y, value;
    if (!offset || shape > 16 || !(x >= 0.0f && x < 1.0f) ||
        !(y >= 0.0f && y < 1.0f)) return 0;
    switch (shape) {
    case 0: value = 0.0f; break;
    case 1: value = sum < 1.0f ? y : 1.0f - x; break;
    case 2: value = sum < 1.0f ? x : 1.0f - y; break;
    case 3: value = 1.0f - (x > y ? x : y); break;
    case 4: value = x < y ? x : y; break;
    case 5: value = 1.0f - x; break;
    case 6: value = 1.0f - y; break;
    case 7: value = y; break;
    case 8: value = x; break;
    case 9: value = y > x ? y - x : 0.0f; break;
    case 10: value = x > y ? x - y : 0.0f; break;
    case 11: value = sum < 1.0f ? 1.0f - sum : 0.0f; break;
    case 12: value = sum > 1.0f ? sum - 1.0f : 0.0f; break;
    case 13: value = sum >= 1.0f ? y : 1.0f - x; break;
    case 14: value = sum >= 1.0f ? x : 1.0f - y; break;
    case 15: value = x > y ? x : y; break;
    default: value = 1.0f - (x < y ? x : y); break;
    }
    *offset = value;
    return 1;
}

int terrain_surface_height_at(const ScnTile *tiles, uint32_t width,
                              uint32_t height, float x, float y,
                              float *surface_height) {
    uint32_t tile_x, tile_y, truncated_x, truncated_y;
    float fraction_x, fraction_y, offset;
    uint8_t shape;
    if (!tiles || !width || !height || !surface_height ||
        x != x || y != y || width > INT32_MAX || height > INT32_MAX ||
        (size_t)width > SIZE_MAX / height) return 0;
    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;
    if (x >= (float)width) x = (float)width - 0.001f;
    if (y >= (float)height) y = (float)height - 0.001f;
    truncated_x = (uint32_t)x;
    truncated_y = (uint32_t)y;
    fraction_x = x - (float)truncated_x;
    fraction_y = y - (float)truncated_y;
    tile_x = truncated_x < width ? truncated_x : width - 1;
    tile_y = truncated_y < height ? truncated_y : height - 1;
    shape = terrain_elevation_shape(tiles, width, height, tile_x, tile_y);
    if (!terrain_shape_surface_offset(shape, fraction_x, fraction_y,
                                      &offset)) return 0;
    *surface_height = (float)tiles[(size_t)tile_y * width + tile_x].elevation +
                      offset;
    return 1;
}

int terrain_unit_surface_z(const DatUnit *unit_template, float requested_z,
                           float surface_height, float *unit_z) {
    if (!unit_template || !unit_template->present || !unit_z ||
        requested_z != requested_z || surface_height != surface_height)
        return 0;
    if (unit_template->elevation_mode == 0)
        *unit_z = surface_height;
    else if (unit_template->elevation_mode == 1 &&
             requested_z < surface_height)
        *unit_z = surface_height;
    else
        *unit_z = requested_z;
    return 1;
}

int terrain_paint_target_available(const DatTerrains *terrains, uint8_t id) {
    const DatTerrain *terrain;
    int16_t replacement;
    if (!terrains || !terrains->terrains || id >= terrains->count) return 0;
    terrain = &terrains->terrains[id];
    if (terrain->enabled) return 1;
    replacement = terrain->replacement_id;
    return replacement >= 0 && (uint16_t)replacement < terrains->count &&
           terrains->terrains[replacement].enabled == 1;
}

int terrain_resolve_draw_id(const DatTerrains *terrains, uint8_t raw_id,
                            uint8_t *draw_id) {
    int16_t replacement;
    if (!terrains || !terrains->terrains || !draw_id ||
        raw_id >= terrains->count) return 0;
    replacement = terrains->terrains[raw_id].replacement_id;
    if (replacement == -1) {
        *draw_id = raw_id;
        return 1;
    }
    if (replacement < 0 || (uint16_t)replacement >= terrains->count ||
        replacement > UINT8_MAX) return 0;
    *draw_id = (uint8_t)replacement;
    return 1;
}

int terrain_shape_frame_span(const DatTerrain *terrain, uint8_t shape,
                             uint16_t *first_frame, uint16_t *frame_count) {
    const DatTerrainFrame *span;
    if (!terrain || !first_frame || !frame_count || shape > 16) return 0;
    span = &terrain->elevation_frames[shape];
    if (span->frame_start < 0 || span->frame_count <= 0 ||
        span->angle_count <= 0)
        return 0;
    *first_frame = (uint16_t)span->frame_start;
    *frame_count = (uint16_t)span->frame_count;
    return 1;
}

int terrain_select_frame(const DatTerrain *terrain, uint8_t shape,
                         uint32_t x, uint32_t y, uint32_t *frame_index) {
    const DatTerrainFrame *span;
    uint32_t variant = 0;
    uint32_t rows, columns;
    if (!terrain || !frame_index || shape > 16 || x > 32767 || y > 32767)
        return 0;
    span = &terrain->elevation_frames[shape];
    if (span->frame_count <= 0 || span->angle_count <= 0 ||
        span->frame_start < 0) return 0;
    if (span->frame_count > 1) {
        rows = (uint8_t)terrain->frame_grid[0];
        columns = (uint8_t)terrain->frame_grid[1];
        if (rows > 1 || columns > 1) {
            uint32_t row = rows ? y % rows : 0;
            uint32_t column = columns ? x % columns : 0;
            variant = row * columns + column;
        }
        if (variant >= (uint32_t)span->frame_count) variant = 0;
    }
    *frame_index = (uint32_t)span->frame_start +
                   variant * (uint32_t)span->angle_count;
    return 1;
}

int terrain_select_border_frame(const DatBorder *border, uint8_t shape,
                                uint8_t pattern, uint32_t x, uint32_t y,
                                uint32_t *frame_index) {
    const DatTerrainFrame *span;
    uint32_t variant = 0;
    uint32_t flat_index;
    if (!border || !frame_index || shape > 16 || pattern == 0 ||
        pattern > 15 || x > 32767 || y > 32767) return 0;
    flat_index = (uint32_t)shape * 12 + pattern - 1;
    span = &border->frames[flat_index / 12][flat_index % 12];
    if (span->frame_count <= 0 || span->angle_count <= 0 ||
        span->frame_start < 0) return 0;
    if (span->frame_count > 1) {
        if ((border->border_style == 0 && (pattern == 11 || pattern == 12)) ||
            (border->border_style == 1 && (pattern == 2 || pattern == 3)))
            variant = y % (uint32_t)span->frame_count;
        else if ((border->border_style == 0 && (pattern == 9 || pattern == 10)) ||
                 (border->border_style == 1 && (pattern == 1 || pattern == 4)))
            variant = x % (uint32_t)span->frame_count;
    }
    *frame_index = (uint32_t)span->frame_start +
                   variant * (uint32_t)span->angle_count;
    return 1;
}

uint8_t terrain_minimap_palette_index(const DatTerrain *terrain, uint8_t shape) {
    /* Both executables dispatch shape 0..16 through this identical table.
       The entries select DAT map_colors[0..2], not SLP sprite frames. */
    static const uint8_t color_slot[17] = {
        1, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 0, 2, 0
    };
    if (!terrain || shape >= sizeof(color_slot)) return 0;
    return terrain->map_colors[color_slot[shape]];
}

static int elevation(const ScnTile *tiles, uint32_t width, uint32_t height,
                     int x, int y, int fallback) {
    if (x < 0 || y < 0 || (uint32_t)x >= width || (uint32_t)y >= height)
        return fallback;
    return tiles[(size_t)y * width + (uint32_t)x].elevation;
}

uint8_t terrain_elevation_shape(const ScnTile *tiles, uint32_t width,
                                uint32_t height, uint32_t x, uint32_t y) {
    int h, n, e, w, s, ne, sw, nw, se;
    if (!tiles || !width || !height || x >= width || y >= height)
        return 0;
    h = tiles[(size_t)y * width + x].elevation;
    n = elevation(tiles, width, height, (int)x, (int)y - 1, h);
    e = elevation(tiles, width, height, (int)x + 1, (int)y, h);
    w = elevation(tiles, width, height, (int)x - 1, (int)y, h);
    s = elevation(tiles, width, height, (int)x, (int)y + 1, h);
    ne = elevation(tiles, width, height, (int)x + 1, (int)y - 1, h);
    sw = elevation(tiles, width, height, (int)x - 1, (int)y + 1, h);
    nw = elevation(tiles, width, height, (int)x - 1, (int)y - 1, h);
    se = elevation(tiles, width, height, (int)x + 1, (int)y + 1, h);

    /* Branch order matches the original: cardinal pairs outrank single
       sides, which outrank the diagonal/elevation-corner combinations. */
    if (n == h + 1 && e == h + 1) return 14;
    if (w == h + 1 && s == h + 1) return 13;
    if (n == h + 1 && w == h + 1) return 16;
    if (e == h + 1 && s == h + 1) return 15;
    if (n == h + 1) return 6;
    if (e == h + 1) return 8;
    if (w == h + 1) return 5;
    if (s == h + 1) return 7;
    /* Diagonals (RoR disc 0x004493a0, patched 1.0a 0x004484f0, base 1.0c
       0x0044a0f0 return the same sequence): each raised corner tests the
       two cardinals on the far side of the tile. */
    if (ne == h + 1) return w == h - 1 && s == h - 1 ? 2 : 10;
    if (sw == h + 1) return n == h - 1 && e == h - 1 ? 1 : 9;
    if (nw == h + 1) return e == h - 1 && s == h - 1 ? 3 : 11;
    if (se == h + 1) return n == h - 1 && w == h - 1 ? 4 : 12;
    return 0;
}

static uint8_t border_neighbor(const ScnTile *tiles, uint32_t width,
                               uint32_t height, int x, int y, uint8_t center,
                               const DatTerrain *terrain,
                               const DatBorders *borders) {
    uint8_t neighbor;
    int16_t id;
    if (x < 0 || y < 0 || (uint32_t)x >= width || (uint32_t)y >= height)
        return center;
    neighbor = tiles[(size_t)y * width + (uint32_t)x].terrain_id;
    if (neighbor == center) return center;
    if (neighbor >= 32) return center;
    id = terrain->borders[neighbor];
    if (id < 0 || (uint16_t)id >= borders->count ||
        !borders->borders[id].enabled)
        return center;
    return neighbor;
}

static int border_id(const DatTerrain *terrain, uint8_t neighbor) {
    return terrain->borders[neighbor];
}

uint8_t terrain_border_code(const ScnTile *tiles, uint32_t width,
                            uint32_t height, uint32_t x, uint32_t y,
                            const DatTerrains *terrains,
                            const DatBorders *borders) {
    uint8_t center, n, s, w, e, nw, ne, sw, se;
    const DatTerrain *terrain;
    const DatBorder *border;
    int id, pattern;
    if (!tiles || !terrains || !terrains->terrains || !borders ||
        !borders->borders || !width || !height || x >= width || y >= height)
        return 0;
    center = tiles[(size_t)y * width + x].terrain_id;
    if (center >= terrains->count) return 0;
    terrain = &terrains->terrains[center];
    n = border_neighbor(tiles, width, height, (int)x, (int)y - 1,
                        center, terrain, borders);
    s = border_neighbor(tiles, width, height, (int)x, (int)y + 1,
                        center, terrain, borders);
    w = border_neighbor(tiles, width, height, (int)x - 1, (int)y,
                        center, terrain, borders);
    e = border_neighbor(tiles, width, height, (int)x + 1, (int)y,
                        center, terrain, borders);
    nw = border_neighbor(tiles, width, height, (int)x - 1, (int)y - 1,
                         center, terrain, borders);
    ne = border_neighbor(tiles, width, height, (int)x + 1, (int)y - 1,
                         center, terrain, borders);
    sw = border_neighbor(tiles, width, height, (int)x - 1, (int)y + 1,
                         center, terrain, borders);
    se = border_neighbor(tiles, width, height, (int)x + 1, (int)y + 1,
                         center, terrain, borders);
    /* The original picks the first different normalized neighbor in this
       order, then tests which of the eight map to that border slot. */
    uint8_t candidates[8] = {n, s, w, e, nw, ne, sw, se};
    uint8_t chosen = center;
    for (unsigned i = 0; i < 8; ++i)
        if (candidates[i] != center) { chosen = candidates[i]; break; }
    if (chosen == center) return 0;
    id = border_id(terrain, chosen);
    if (id <= 0 || id > 15 || (uint16_t)id >= borders->count)
        return 0;
    border = &borders->borders[id];
    if (!border->enabled) return 0;
    if (border->border_style == 1) {
        pattern = (border_id(terrain, n) == id ? 1 : 0) |
                  (border_id(terrain, s) == id ? 2 : 0) |
                  (border_id(terrain, w) == id ? 4 : 0) |
                  (border_id(terrain, e) == id ? 8 : 0);
    } else if (border_id(terrain, n) == id &&
               border_id(terrain, w) == id) pattern = 1;
    else if (border_id(terrain, n) == id &&
             border_id(terrain, e) == id) pattern = 2;
    else if (border_id(terrain, s) == id &&
             border_id(terrain, w) == id) pattern = 3;
    else if (border_id(terrain, s) == id &&
             border_id(terrain, e) == id) pattern = 4;
    else if (border_id(terrain, n) == id) pattern = 9;
    else if (border_id(terrain, s) == id) pattern = 10;
    else if (border_id(terrain, w) == id) pattern = 11;
    else if (border_id(terrain, e) == id) pattern = 12;
    else if (border_id(terrain, nw) == id) pattern = 5;
    else if (border_id(terrain, ne) == id) pattern = 6;
    else if (border_id(terrain, sw) == id) pattern = 7;
    else if (border_id(terrain, se) == id) pattern = 8;
    else return 0;
    return (uint8_t)((pattern << 4) | id);
}

int terrain_project_tile(const ScnTile *tiles, uint32_t width, uint32_t height,
                         uint32_t x, uint32_t y, const DatMapGeometry *geometry,
                         int32_t *screen_x, int32_t *screen_y) {
    uint8_t shape;
    int32_t projected_y;
    if (!tiles || !geometry || !screen_x || !screen_y ||
        !width || !height || x >= width || y >= height ||
        geometry->tile_half_width <= 0 || geometry->tile_half_height <= 0 ||
        geometry->elevation_height <= 0 || width > 32767 || height > 32767)
        return 0;
    shape = terrain_elevation_shape(tiles, width, height, x, y);
    *screen_x = (int32_t)(x + y) * geometry->tile_half_width;
    projected_y = ((int32_t)y - (int32_t)x) * geometry->tile_half_height -
                  (int32_t)tiles[(size_t)y * width + x].elevation *
                  geometry->elevation_height;
    switch (shape) {
    case 2: case 6: case 8: case 10: case 14: case 15: case 16:
        projected_y -= geometry->elevation_height;
        break;
    default:
        break;
    }
    *screen_y = projected_y;
    return 1;
}

int terrain_project_tile_view(const ScnTile *tiles, uint32_t width,
                              uint32_t height, uint32_t x, uint32_t y,
                              const DatMapGeometry *geometry,
                              int32_t camera_x, int32_t camera_y,
                              int32_t *screen_x, int32_t *screen_y) {
    int32_t world_x, world_y;
    int64_t view_x, view_y;
    if (!screen_x || !screen_y ||
        !terrain_project_tile(tiles, width, height, x, y, geometry,
                              &world_x, &world_y)) return 0;
    view_x = (int64_t)world_x - camera_x;
    view_y = (int64_t)world_y - camera_y;
    if (view_x < INT32_MIN || view_x > INT32_MAX ||
        view_y < INT32_MIN || view_y > INT32_MAX) return 0;
    *screen_x = (int32_t)view_x;
    *screen_y = (int32_t)view_y;
    return 1;
}

int terrain_walk_view_tiles(uint32_t width, uint32_t height,
                            int32_t start_x, int32_t start_y,
                            uint32_t diagonal_count, uint32_t diagonal_length,
                            TerrainTileVisitor visit, void *context,
                            size_t *visited_count) {
    size_t count = 0;
    int64_t diagonal_x = start_x, diagonal_y = start_y;
    if (!width || !height || !visit || !visited_count) return 0;
    for (uint32_t diagonal = 0; diagonal < diagonal_count; ++diagonal) {
        int64_t x = diagonal_x, y = diagonal_y;
        for (uint32_t step = 0; step < diagonal_length; ++step, ++x, ++y) {
            if (x < 0 || y < 0) continue;
            if (x >= width || y >= height) break;
            if (count == SIZE_MAX) return 0;
            visit((uint32_t)x, (uint32_t)y, context);
            ++count;
        }
        if (diagonal & 1) ++diagonal_y;
        else --diagonal_x;
    }
    *visited_count = count;
    return 1;
}

int terrain_clamp_camera_tiles(uint32_t width, uint32_t height,
                               float requested_x, float requested_y,
                               float *camera_x, float *camera_y) {
    float max_x, max_y;
    if (!camera_x || !camera_y || !width || !height ||
        width > 32767 || height > 32767) return 0;
    max_x = (float)(width - 1);
    max_y = (float)(height - 1);
    *camera_x = !(requested_x >= 0.0f) ? 0.0f :
                requested_x > max_x ? max_x : requested_x;
    *camera_y = !(requested_y >= 0.0f) ? 0.0f :
                requested_y > max_y ? max_y : requested_y;
    return 1;
}

uint8_t terrain_view_visibility_code(uint32_t tile_mask,
                                      uint32_t visible_mask,
                                      uint32_t explored_mask,
                                      int bypass_exploration_gate,
                                      int fog_enabled) {
    if (tile_mask & visible_mask) return TERRAIN_VIEW_VISIBLE;
    if ((tile_mask & explored_mask) || bypass_exploration_gate)
        return fog_enabled ? TERRAIN_VIEW_EXPLORED_FOG :
                             TERRAIN_VIEW_VISIBLE;
    return TERRAIN_VIEW_HIDDEN;
}

int terrain_build_visibility_edge_table(uint8_t table[256]) {
    uint8_t next = 0;
    if (!table) return 0;
    for (unsigned mask = 0; mask < 256; ++mask) {
        int valid = !(((mask & 0x80) && (mask & 0x09)) ||
                      ((mask & 0x40) && (mask & 0x0c)) ||
                      ((mask & 0x20) && (mask & 0x06)) ||
                      ((mask & 0x10) && (mask & 0x03)));
        table[mask] = valid ? next++ : 0xff;
    }
    for (unsigned mask = 0; mask < 256; ++mask) {
        unsigned canonical;
        if (table[mask] != 0xff) continue;
        canonical = mask & 0xf0;
        if (!(mask & (0x80 | 0x10))) canonical |= mask & 0x01;
        if (!(mask & (0x20 | 0x10))) canonical |= mask & 0x02;
        if (!(mask & (0x20 | 0x40))) canonical |= mask & 0x04;
        if (!(mask & (0x80 | 0x40))) canonical |= mask & 0x08;
        table[mask] = table[canonical];
    }
    return next == 47;
}

int terrain_visibility_edge_pattern(const uint32_t *tile_masks,
                                     uint32_t width, uint32_t height,
                                     uint32_t x, uint32_t y,
                                     uint32_t owner_mask,
                                     const uint8_t table[256],
                                     uint8_t *pattern) {
    uint8_t mask = 0;
    if (!tile_masks || !table || !pattern || !width || !height ||
        width > 32767 || height > 32767 || x >= width || y >= height)
        return 0;
    if (x > 0 && !(tile_masks[(size_t)y * width + x - 1] & owner_mask))
        mask |= 0x10;
    if (x + 1 < width &&
        !(tile_masks[(size_t)y * width + x + 1] & owner_mask))
        mask |= 0x40;
    if (y > 0 && !(tile_masks[(size_t)(y - 1) * width + x] & owner_mask))
        mask |= 0x80;
    if (y + 1 < height &&
        !(tile_masks[(size_t)(y + 1) * width + x] & owner_mask))
        mask |= 0x20;
    if (x > 0 && y > 0 &&
        !(tile_masks[(size_t)(y - 1) * width + x - 1] & owner_mask))
        mask |= 0x01;
    if (x > 0 && y + 1 < height &&
        !(tile_masks[(size_t)(y + 1) * width + x - 1] & owner_mask))
        mask |= 0x02;
    if (x + 1 < width && y + 1 < height &&
        !(tile_masks[(size_t)(y + 1) * width + x + 1] & owner_mask))
        mask |= 0x04;
    if (x + 1 < width && y > 0 &&
        !(tile_masks[(size_t)(y - 1) * width + x + 1] & owner_mask))
        mask |= 0x08;
    *pattern = table[mask];
    return 1;
}

int terrain_compute_view_camera(const ScnTile *tiles, uint32_t width,
                                uint32_t height, const DatMapGeometry *geometry,
                                float camera_tile_x, float camera_tile_y,
                                int32_t view_left, int32_t view_top,
                                int32_t view_width, int32_t view_height,
                                TerrainViewCamera *camera) {
    TerrainViewCamera result;
    uint32_t tile_x, tile_y, columns, rows;
    int32_t world_x, world_y;
    int64_t view_x, view_y, pixel_x, pixel_y, start_x, start_y;
    double fraction_x, fraction_y, projected_x, projected_y;
    int64_t extra_columns, extra_rows;
    if (!tiles || !geometry || !camera || !width || !height ||
        width > 32767 || height > 32767 ||
        !(camera_tile_x >= 0 && camera_tile_x < width) ||
        !(camera_tile_y >= 0 && camera_tile_y < height) ||
        view_left < INT16_MIN || view_left > INT16_MAX ||
        view_top < INT16_MIN || view_top > INT16_MAX ||
        view_width <= 0 || view_width > INT16_MAX ||
        view_height <= 0 || view_height > INT16_MAX ||
        geometry->tile_width <= 0 || geometry->tile_height <= 0 ||
        geometry->tile_width != geometry->tile_half_width * 2 ||
        geometry->tile_height != geometry->tile_half_height * 2)
        return 0;
    columns = (uint32_t)(view_width / geometry->tile_width);
    rows = (uint32_t)(view_height / geometry->tile_height);
    /* The original __ftol uses x87 truncation toward zero. In the admitted
       nonnegative map domain, C's integer conversion has the same rule. */
    tile_x = (uint32_t)camera_tile_x;
    tile_y = (uint32_t)camera_tile_y;
    if (!terrain_project_tile(tiles, width, height, tile_x, tile_y, geometry,
                              &world_x, &world_y)) return 0;
    if (tiles[(size_t)tile_y * width + tile_x].elevation > 7) return 0;
    fraction_x = (double)camera_tile_x - tile_x;
    fraction_y = (double)camera_tile_y - tile_y;
    projected_x = (fraction_x * 0.5 + fraction_y * 0.5) *
                  geometry->tile_width;
    projected_y = (fraction_y * 0.5 - fraction_x * 0.5) *
                  geometry->tile_height;
    if (projected_x < INT32_MIN || projected_x > INT32_MAX ||
        projected_y < INT32_MIN || projected_y > INT32_MAX) return 0;
    view_x = (int64_t)view_left + view_width / 2 -
             geometry->tile_half_width - (int64_t)projected_x;
    view_y = (int64_t)view_top + view_height / 2 -
             2 * geometry->tile_half_height - (int64_t)projected_y;
    if (view_x < INT16_MIN || view_x > INT16_MAX ||
        view_y < INT16_MIN || view_y > INT16_MAX) return 0;
    pixel_x = (int64_t)world_x - view_x;
    pixel_y = (int64_t)world_y +
              (int64_t)tiles[(size_t)tile_y * width + tile_x].elevation *
              geometry->elevation_height - view_y;
    extra_columns = columns / 2 + 2;
    extra_rows = rows / 2 + 1;
    start_x = (int64_t)tile_x - extra_columns + extra_rows;
    start_y = (int64_t)tile_y - extra_columns - extra_rows;
    if (pixel_x < INT32_MIN || pixel_x > INT32_MAX ||
        pixel_y < INT32_MIN || pixel_y > INT32_MAX ||
        start_x < INT16_MIN || start_x > INT16_MAX ||
        start_y < INT16_MIN || start_y > INT16_MAX) return 0;
    result.camera_pixel_x = (int32_t)pixel_x;
    result.camera_pixel_y = (int32_t)pixel_y;
    result.start_x = (int32_t)start_x;
    result.start_y = (int32_t)start_y;
    result.diagonal_count = rows * 2 + 12;
    result.diagonal_length = columns + 5;
    result.view_left = view_left;
    result.view_top = view_top;
    result.view_width = view_width;
    result.view_height = view_height;
    *camera = result;
    return 1;
}
