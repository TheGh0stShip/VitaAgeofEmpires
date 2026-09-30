#include "runtime_map.h"

#include "terrain.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int aoe_runtime_map_from_scenario(AoeRuntimeMap *out,
                                   const ScnMap *scenario,
                                   const DatMapGeometry *geometry,
                                   const DatTerrains *terrains,
                                   const DatBorders *borders) {
    AoeRuntimeMap next = {0};
    size_t count;
    if (!out || !scenario || !geometry || !terrains ||
        !terrains->terrains || !borders || !borders->borders ||
        !scn_runtime_tile_order_supported(scenario)) return 0;
    next.width = scenario->width;
    next.height = scenario->height;
    count = (size_t)next.width * next.height;
    next.tiles = calloc(count, sizeof(*next.tiles));
    next.shapes = calloc(count, sizeof(*next.shapes));
    next.borders = calloc(count, sizeof(*next.borders));
    next.projected_x = calloc(count, sizeof(*next.projected_x));
    next.projected_y = calloc(count, sizeof(*next.projected_y));
    next.view_codes = calloc(count, sizeof(*next.view_codes));
    next.cache_flags = calloc(count, sizeof(*next.cache_flags));
    next.draw_flags = calloc(count, sizeof(*next.draw_flags));
    next.objects = calloc(count, sizeof(*next.objects));
    if (!next.tiles || !next.shapes || !next.borders ||
        !next.projected_x || !next.projected_y || !next.view_codes ||
        !next.cache_flags ||
        !next.draw_flags ||
        !next.objects ||
        !aoe_visibility_mask_map_init(&next.masks, next.width, next.height))
        goto fail;
    for (size_t i = 0; i < count; ++i) {
        /* Scenario loader keeps the low 5 terrain bits and upper 3
           elevation bits in runtime tile byte +5; third byte is ignored. */
        next.tiles[i].terrain_id = scenario->tiles[i].terrain_id & 0x1f;
        next.tiles[i].elevation = scenario->tiles[i].elevation & 0x07;
    }
    for (uint32_t y = 0; y < next.height; ++y)
        for (uint32_t x = 0; x < next.width; ++x) {
            size_t i = (size_t)y * next.width + x;
            next.shapes[i] = terrain_elevation_shape(next.tiles,
                                                     next.width, next.height,
                                                     x, y);
            next.borders[i] = terrain_border_code(next.tiles,
                                                  next.width, next.height,
                                                  x, y, terrains, borders);
            if (!terrain_project_tile(next.tiles, next.width, next.height,
                                      x, y, geometry, &next.projected_x[i],
                                      &next.projected_y[i])) goto fail;
        }
    *out = next;
    return 1;
fail:
    aoe_runtime_map_destroy(&next);
    return 0;
}

void aoe_runtime_map_destroy(AoeRuntimeMap *map) {
    size_t count;
    if (!map) return;
    count = (size_t)map->width * map->height;
    if (map->objects)
        for (size_t i = 0; i < count; ++i)
            aoe_object_update_list_clear(&map->objects[i]);
    free(map->objects);
    free(map->draw_flags);
    free(map->cache_flags);
    free(map->view_codes);
    free(map->projected_y);
    free(map->projected_x);
    free(map->borders);
    free(map->shapes);
    free(map->tiles);
    aoe_visibility_mask_map_destroy(&map->masks);
    memset(map, 0, sizeof(*map));
}

int aoe_runtime_map_bind_player_vision(AoeRuntimeMap *map,
                                       AoePlayerVisionState *player,
                                       int is_active_player) {
    if (!map || !player || !map->view_codes || !map->masks.masks ||
        !player->grid.counts || player->grid.width != map->width ||
        player->grid.height != map->height ||
        player->grid.tile_masks != map->masks.masks) return 0;
    player->grid.view_codes = is_active_player ? map->view_codes : 0;
    return 1;
}

int aoe_runtime_map_invalidate_ror(AoeRuntimeMap *map,
                                   int32_t x0, int32_t y0,
                                   int32_t x1, int32_t y1,
                                   uint8_t draw_flag) {
    int32_t swap;
    if (!map || !map->width || !map->height ||
        map->width > INT32_MAX || map->height > INT32_MAX ||
        !map->cache_flags || !map->draw_flags)
        return 0;
    if (x1 < x0) { swap = x0; x0 = x1; x1 = swap; }
    if (y1 < y0) { swap = y0; y0 = y1; y1 = swap; }
    if (x1 < 0 || y1 < 0 || x0 >= (int32_t)map->width ||
        y0 >= (int32_t)map->height)
        return 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= (int32_t)map->width) x1 = (int32_t)map->width - 1;
    if (y1 >= (int32_t)map->height) y1 = (int32_t)map->height - 1;
    for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x) {
            size_t index = (size_t)y * map->width + (uint32_t)x;
            map->cache_flags[index] = 0xcc;
            map->draw_flags[index] = draw_flag;
        }
    return 1;
}

int aoe_runtime_map_paint_terrain_ror(
    AoeRuntimeMap *map, const DatTerrains *terrains,
    const DatBorders *borders, int32_t x0, int32_t y0,
    int32_t x1, int32_t y1, uint8_t terrain_id,
    int16_t filter_terrain, const AoeRuntimeTerrainPaintOps *ops) {
    int32_t swap;
    const DatTerrain *target;
    x0 = (int16_t)x0;
    y0 = (int16_t)y0;
    x1 = (int16_t)x1;
    y1 = (int16_t)y1;
    if (!map || !map->tiles || !map->shapes || !map->borders ||
        !terrains || !borders || !ops || !ops->transition_cell ||
        !terrain_paint_target_available(terrains, terrain_id) ||
        terrain_id >= terrains->count || map->width > INT16_MAX ||
        map->height > INT16_MAX)
        return 0;
    target = &terrains->terrains[terrain_id];
    if (x1 < x0) { swap = x0; x0 = x1; x1 = swap; }
    if (y1 < y0) { swap = y0; y0 = y1; y1 = swap; }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= (int32_t)map->width) x1 = (int32_t)map->width - 1;
    if (y1 >= (int32_t)map->height) y1 = (int32_t)map->height - 1;
    if (x0 > x1 || y0 > y1) return 1;
    for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x) {
            size_t index = (size_t)y * map->width + (uint32_t)x;
            uint8_t shape = map->shapes[index];
            uint8_t old_terrain;
            if (shape >= 19 || target->elevation_frames[shape].frame_count <= 0)
                continue;
            old_terrain = map->tiles[index].terrain_id;
            map->tiles[index].terrain_id = terrain_id;
            ops->transition_cell(ops->context, (uint32_t)x, (uint32_t)y,
                                 old_terrain, filter_terrain);
        }
    if (x0 > 0) --x0;
    if (y0 > 0) --y0;
    if (x1 < (int32_t)map->width - 1) ++x1;
    if (y1 < (int32_t)map->height - 1) ++y1;
    for (int32_t y = y0; y <= y1; ++y)
        for (int32_t x = x0; x <= x1; ++x) {
            size_t index = (size_t)y * map->width + (uint32_t)x;
            map->borders[index] = terrain_border_code(
                map->tiles, map->width, map->height,
                (uint32_t)x, (uint32_t)y, terrains, borders);
        }
    return aoe_runtime_map_invalidate_ror(map, x0, y0, x1, y1, 0);
}
