#include "dat.h"
#include "drs.h"
#include "palette.h"
#include "scn.h"
#include "terrain.h"

#include <stdio.h>
#include <stdlib.h>

static int write_minimap(const char *path, const ScnMap *map,
                         const DatTerrains *terrains, const Palette *palette) {
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    int ok = fprintf(file, "P6\n%u %u\n255\n", map->width, map->height) > 0;
    for (uint32_t y = 0; ok && y < map->height; ++y) for (uint32_t x = 0;
         ok && x < map->width; ++x) {
        uint8_t id = map->tiles[(size_t)y * map->width + x].terrain_id;
        uint8_t shape = terrain_elevation_shape(map->tiles, map->width,
                                                map->height, x, y);
        uint8_t index = terrain_minimap_palette_index(&terrains->terrains[id],
                                                       shape);
        PaletteColor color = palette->colors[index];
        uint8_t rgb[3] = {color.r, color.g, color.b};
        ok = fwrite(rgb, 1, sizeof(rgb), file) == sizeof(rgb);
    }
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int read_palette(const char *path, Palette *palette) {
    DrsArchive archive;
    const DrsEntry *entry;
    uint8_t *bytes;
    int ok;
    if (!drs_open(&archive, path)) return 0;
    entry = drs_find(&archive, drs_type("anib"), 50500);
    if (!entry) { drs_close(&archive); return 0; }
    bytes = malloc(entry->size ? entry->size : 1);
    if (!bytes) { drs_close(&archive); return 0; }
    ok = drs_read(&archive, entry, bytes, entry->size) &&
         palette_parse_jasc(bytes, entry->size, palette);
    free(bytes);
    drs_close(&archive);
    return ok;
}

int main(int argc, char **argv) {
    DatBlob blob;
    DatIntro intro;
    ScnMap map;
    unsigned counts[256] = {0};
    unsigned shape_counts[17] = {0};
    if (argc != 3 && argc != 5) {
        fprintf(stderr, "usage: scn_tool SCENARIO EMPIRES.DAT [INTERFAC.DRS OUTPUT.ppm]\n");
        return 2;
    }
    if (!dat_load(argv[2], &blob)) {
        fprintf(stderr, "cannot load VER 3.7 game data\n");
        return 1;
    }
    if (!dat_parse_intro(&blob, &intro)) {
        fprintf(stderr, "cannot read terrain count\n");
        dat_free(&blob);
        return 1;
    }
    if (!scn_load_map(argv[1], (uint8_t)intro.terrain_count, &map)) {
        fprintf(stderr, "cannot parse a valid 1.10/1.11 scenario with 1.15 body\n");
        dat_free_intro(&intro);
        dat_free(&blob);
        return 1;
    }
    printf("scenario %s: %u x %u tiles, %u players, body version %.2f, "
           "map at 0x%zx\n", map.version, map.width, map.height,
           map.player_count, map.player_data_version, map.map_offset);
    printf("fixed player data at 0x%zx\n", map.player_data_fixed_offset);
    for (unsigned i = 0; i < 9; ++i) {
        uint32_t low = 0, middle = 0, high = 0;
        for (uint32_t j = 0; j < map.player_units[i].count; ++j) {
            uint8_t state = map.player_units[i].units[j].state;
            if (state <= 2) ++low;
            else if (state <= 6) ++middle;
            else ++high;
        }
        printf("player %u: %u placed units (states 0..2 %u, 3..6 %u, "
               "7+ %u)\n", i, map.player_units[i].count,
               low, middle, high);
        if (high)
            for (uint32_t j = 0; j < map.player_units[i].count; ++j) {
                const ScnUnit *unit = &map.player_units[i].units[j];
                if (unit->state >= 7)
                    printf("  high-state record %u: object %u state %u "
                           "spawn %u\n", j, unit->object_id, unit->state,
                           unit->spawn_id);
            }
    }
    for (unsigned i = 0; i < 16; ++i)
        if (map.player_setup[i].enabled)
            printf("setup %u: %s, civilization %u, human %u, field3 %u\n", i,
                   map.player_setup[i].name,
                   map.player_setup[i].civilization_id,
                   map.player_setup[i].is_human,
                   map.player_setup[i].unknown);
    if (map.briefing_bitmap.dib)
        printf("briefing bitmap: %u x %u, %zu DIB bytes\n",
               map.briefing_bitmap.width, map.briefing_bitmap.height,
               map.briefing_bitmap.dib_size);
    for (size_t i = 0; i < (size_t)map.width * map.height; ++i)
        ++counts[map.tiles[i].terrain_id];
    for (uint32_t y = 0; y < map.height; ++y)
        for (uint32_t x = 0; x < map.width; ++x)
            ++shape_counts[terrain_elevation_shape(map.tiles, map.width,
                                                    map.height, x, y)];
    for (unsigned i = 0; i < intro.terrain_count; ++i)
        if (counts[i]) printf("terrain %u: %u tiles\n", i, counts[i]);
    for (unsigned i = 0; i < 17; ++i)
        if (shape_counts[i]) printf("elevation shape %u: %u tiles\n",
                                    i, shape_counts[i]);
    if (argc == 5) {
        DatSounds sounds;
        DatGraphics graphics;
        DatTerrains terrains;
        DatBorders borders;
        DatMapGeometry geometry;
        Palette palette;
        int ok = 0;
        if (dat_parse_sounds(&blob, &intro, &sounds)) {
            if (dat_parse_graphics(&blob, &sounds, &graphics)) {
                if (dat_parse_terrains(&blob, &intro, &graphics, &terrains)) {
                    if (dat_parse_borders(&blob, &terrains, &borders)) {
                        unsigned border_counts[16] = {0};
                        int valid = dat_parse_map_geometry(
                            &blob, &terrains, &borders, &geometry);
                        int32_t min_x = INT32_MAX, min_y = INT32_MAX;
                        int32_t max_x = INT32_MIN, max_y = INT32_MIN;
                        for (uint32_t y = 0; valid && y < map.height; ++y)
                            for (uint32_t x = 0; x < map.width; ++x) {
                                int32_t projected_x, projected_y;
                                uint8_t code = terrain_border_code(
                                    map.tiles, map.width, map.height, x, y,
                                    &terrains, &borders);
                                ++border_counts[code & 15];
                                if (!terrain_project_tile(
                                        map.tiles, map.width, map.height, x, y,
                                        &geometry, &projected_x, &projected_y)) {
                                    valid = 0;
                                    break;
                                }
                                if (projected_x < min_x) min_x = projected_x;
                                if (projected_x > max_x) max_x = projected_x;
                                if (projected_y < min_y) min_y = projected_y;
                                if (projected_y > max_y) max_y = projected_y;
                            }
                        for (unsigned i = 1; i < 16; ++i)
                            if (border_counts[i])
                                printf("border %u: %u tiles\n", i,
                                       border_counts[i]);
                        if (valid)
                            printf("projected tile origins: x %d..%d, y %d..%d\n",
                                   min_x, max_x, min_y, max_y);
                        ok = valid && read_palette(argv[3], &palette) &&
                             write_minimap(argv[4], &map, &terrains, &palette);
                        dat_free_borders(&borders);
                    }
                    dat_free_terrains(&terrains);
                }
                dat_free_graphics(&graphics);
            }
            dat_free_sounds(&sounds);
        }
        if (!ok) {
            fprintf(stderr, "cannot render terrain-color minimap\n");
            scn_free_map(&map);
            dat_free_intro(&intro);
            dat_free(&blob);
            return 1;
        }
        printf("wrote minimap: %s\n", argv[4]);
    }
    scn_free_map(&map);
    dat_free_intro(&intro);
    dat_free(&blob);
    return 0;
}
