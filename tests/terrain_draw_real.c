#include "dat.h"
#include "drs.h"
#include "palette.h"
#include "scn.h"
#include "terrain.h"
#include "terrain_art.h"
#include "terrain_draw.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PLAN_BUCKETS = 65536 };

typedef struct {
    uint8_t used, layer_count;
    TerrainDrawLayer layers[5];
} SeenPlan;

typedef struct {
    size_t calls;
    int invalid;
} RealTileTrace;

static int trace_real_tile(void *opaque, uint32_t tile_x, uint32_t tile_y,
                           int32_t anchor_x, int32_t anchor_y,
                           uint8_t visibility_mode) {
    RealTileTrace *trace = opaque;
    (void)tile_x;
    (void)tile_y;
    (void)anchor_x;
    (void)anchor_y;
    ++trace->calls;
    if (visibility_mode == TERRAIN_VIEW_HIDDEN) trace->invalid = 1;
    return 1;
}

static int same_layers(const SeenPlan *seen, const TerrainDrawPlan *plan) {
    if (seen->layer_count != plan->layer_count) return 0;
    for (uint8_t i = 0; i < plan->layer_count; ++i) {
        if (seen->layers[i].kind != plan->layers[i].kind ||
            seen->layers[i].slot != plan->layers[i].slot ||
            seen->layers[i].frame != plan->layers[i].frame) return 0;
    }
    return 1;
}

static int record_plan(SeenPlan *seen, const TerrainDrawPlan *plan) {
    uint32_t hash = 2166136261u;
    for (uint8_t i = 0; i < plan->layer_count; ++i) {
        hash = (hash ^ plan->layers[i].kind) * 16777619u;
        hash = (hash ^ plan->layers[i].slot) * 16777619u;
        hash = (hash ^ plan->layers[i].frame) * 16777619u;
    }
    size_t index = hash & (PLAN_BUCKETS - 1);
    for (size_t probe = 0; probe < PLAN_BUCKETS; ++probe) {
        SeenPlan *bucket = &seen[index];
        if (!bucket->used) {
            bucket->used = 1;
            bucket->layer_count = plan->layer_count;
            memcpy(bucket->layers, plan->layers,
                   plan->layer_count * sizeof(TerrainDrawLayer));
            return 1;
        }
        if (same_layers(bucket, plan)) return 0;
        index = (index + 1) & (PLAN_BUCKETS - 1);
    }
    return -1;
}

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    uint8_t *bytes = NULL;
    long length;
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0 ||
        (length = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0)
        goto done;
    bytes = malloc((size_t)length);
    if (!bytes) goto done;
    if (fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        free(bytes);
        bytes = NULL;
        goto done;
    }
    *size = (size_t)length;
done:
    fclose(file);
    return bytes;
}

/* Real-asset audit, not a game executable. It walks original scenario tiles,
   blits each distinct frame sequence, and checks a caller-supplied mixed
   visibility grid when an original TILEEDGE.DAT is provided. */
int main(int argc, char **argv) {
    DatBlob blob = {0};
    DatIntro intro = {0};
    DatSounds sounds = {0};
    DatGraphics graphics = {0};
    DatTerrains terrains = {0};
    DatBorders borders = {0};
    DatMapGeometry geometry = {0};
    ScnMap map = {0};
    DrsArchive terrain_archive = {0}, border_archive = {0}, interface_archive = {0};
    DrsArchive *terrain_archives[1] = {&terrain_archive};
    DrsArchive *border_archives[1] = {&border_archive};
    TerrainArtCache terrain_art = {0};
    BorderArtCache border_art = {0};
    Palette palette = {0};
    uint8_t *palette_bytes = NULL;
    SeenPlan *seen = NULL;
    uint8_t *view_rgba = NULL;
    uint8_t *view_indices = NULL;
    uint8_t *edge_bytes = NULL, *tile_flags = NULL;
    uint8_t *first_clip = NULL, *second_clip = NULL;
    uint32_t *visibility_masks = NULL;
    size_t edge_size = 0;
    TileEdgeIndex edge_index = {0};
    uint8_t rgba[128 * 96 * 4];
    uint8_t pass_rgba[128 * 96 * 4];
    uint8_t pass_indices[128 * 96];
    uint8_t clip_bytes[128 * 96];
    uint64_t tiles = 0, empty = 0, layers = 0, unique = 0;
    size_t view_drawn = 0, view_pixels = 0, fog_drawn = 0, fog_pixels = 0;
    size_t object_tiles = 0;
    int ok = 0;
    if (argc != 6 && argc != 7) {
        fprintf(stderr, "usage: terrain_draw_real SCENARIO EMPIRES.DAT TERRAIN.DRS BORDER.DRS INTERFAC.DRS [TILEEDGE.DAT]\n");
        return 2;
    }
    if (!dat_load(argv[2], &blob) ||
        !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &map) ||
        !drs_open(&terrain_archive, argv[3]) ||
        !drs_open(&border_archive, argv[4]) ||
        !drs_open(&interface_archive, argv[5]) ||
        !terrain_art_load_direct(&terrain_art, &terrains, terrain_archives, 1) ||
        !border_art_load_direct(&border_art, &borders, border_archives, 1))
        goto done;
    const DrsEntry *palette_entry = drs_find(
        &interface_archive, drs_type("anib"), 50500);
    if (!palette_entry) goto done;
    if (argc == 7) {
        edge_bytes = read_file(argv[6], &edge_size);
        if (!edge_bytes || !tileedge_parse(edge_bytes, edge_size,
                                            &edge_index) ||
            map.width > SIZE_MAX / map.height)
            goto done;
        size_t tile_count = (size_t)map.width * map.height;
        visibility_masks = calloc(tile_count, sizeof(*visibility_masks));
        tile_flags = calloc(tile_count, 1);
        first_clip = calloc(640 * 480, 1);
        second_clip = calloc(640 * 480, 1);
        if (!visibility_masks || !tile_flags || !first_clip || !second_clip)
            goto done;
        for (uint32_t y = 0; y < map.height; ++y)
            for (uint32_t x = 0; x < map.width; ++x)
                visibility_masks[(size_t)y * map.width + x] =
                    x < map.width / 2 ? 3 :
                    y < map.height / 2 ? 2 : 0;
    }
    palette_bytes = malloc(palette_entry->size ? palette_entry->size : 1);
    seen = calloc(PLAN_BUCKETS, sizeof(*seen));
    view_rgba = calloc(640 * 480, 4);
    view_indices = malloc(640 * 480);
    if (!palette_bytes || !seen || !view_rgba || !view_indices ||
        !drs_read(&interface_archive, palette_entry, palette_bytes,
                  palette_entry->size) ||
        !palette_parse_jasc(palette_bytes, palette_entry->size, &palette))
        goto done;
    memset(clip_bytes, 255, sizeof(clip_bytes));
    TerrainClipMask full_clip = {clip_bytes, sizeof(clip_bytes), 128};
    TerrainClipMask no_clip = {0};
    TerrainSpritePassPlan normal_passes;
    TerrainSpritePassPlan dither_passes;
    if (!terrain_build_sprite_pass_plan(0, 1, 0, &normal_passes) ||
        !terrain_build_sprite_pass_plan(0x20, 0, 1, &dither_passes))
        goto done;

    for (uint32_t y = 0; y < map.height; ++y) {
        for (uint32_t x = 0; x < map.width; ++x) {
            const ScnTile *tile = &map.tiles[(size_t)y * map.width + x];
            uint8_t shape = terrain_elevation_shape(map.tiles, map.width,
                                                      map.height, x, y);
            uint8_t code = terrain_border_code(map.tiles, map.width,
                                                map.height, x, y,
                                                &terrains, &borders);
            TerrainDrawPlan plan;
            if (!terrain_build_draw_plan(&terrains, &borders, &terrain_art,
                                         &border_art, tile->terrain_id,
                                         shape, code, x, y, &plan)) goto done;
            tiles++;
            layers += plan.layer_count;
            if (!plan.layer_count) { empty++; continue; }
            int fresh = record_plan(seen, &plan);
            if (fresh < 0) goto done;
            if (!fresh) continue;
            unique++;
            memset(rgba, 0, sizeof(rgba));
            if (!terrain_blit_draw_plan_rgba(&plan, &terrain_art, &border_art,
                                              &palette, rgba, sizeof(rgba),
                                              128, 96, 128 * 4, 64, 48))
                goto done;
            memset(pass_rgba, 0, sizeof(pass_rgba));
            if (!terrain_blit_draw_plan_passes_rgba(
                    &plan, &terrain_art, &border_art, &palette,
                    &normal_passes, full_clip, no_clip,
                    pass_rgba, sizeof(pass_rgba), 128, 96, 128 * 4,
                    64, 48, 0, 0) ||
                memcmp(rgba, pass_rgba, sizeof(rgba)) != 0)
                goto done;
            memset(pass_indices, 0, sizeof(pass_indices));
            if (!terrain_blit_draw_plan_passes_indexed(
                    &plan, &terrain_art, &border_art,
                    &normal_passes, full_clip, no_clip,
                    pass_indices, sizeof(pass_indices), 128, 96, 128,
                    64, 48, 0, 0)) goto done;
            for (size_t i = 0; i < sizeof(pass_indices); ++i) {
                if (!pass_rgba[i * 4 + 3]) continue;
                PaletteColor color = palette.colors[pass_indices[i]];
                if (pass_rgba[i * 4] != color.r ||
                    pass_rgba[i * 4 + 1] != color.g ||
                    pass_rgba[i * 4 + 2] != color.b) goto done;
            }
            memset(pass_rgba, 0, sizeof(pass_rgba));
            if (!terrain_blit_draw_plan_passes_rgba(
                    &plan, &terrain_art, &border_art, &palette,
                    &dither_passes, no_clip, full_clip,
                    pass_rgba, sizeof(pass_rgba), 128, 96, 128 * 4,
                    64, 48, 0, 0))
                goto done;
            memset(pass_indices, 0, sizeof(pass_indices));
            if (!terrain_blit_draw_plan_passes_indexed(
                    &plan, &terrain_art, &border_art,
                    &dither_passes, no_clip, full_clip,
                    pass_indices, sizeof(pass_indices), 128, 96, 128,
                    64, 48, 0, 0)) goto done;
            for (size_t i = 0; i < sizeof(pass_indices); ++i) {
                if (!pass_rgba[i * 4 + 3]) continue;
                PaletteColor color = palette.colors[pass_indices[i]];
                if (pass_rgba[i * 4] != color.r ||
                    pass_rgba[i * 4 + 1] != color.g ||
                    pass_rgba[i * 4 + 2] != color.b) goto done;
            }
        }
    }
    {
        TerrainViewCamera camera;
        TerrainViewSurface surface = {view_rgba, 640 * 480 * 4,
                                      640, 480, 640 * 4, 0, 0, 640, 480};
        float camera_x, camera_y;
        if (!terrain_clamp_camera_tiles(map.width, map.height,
                                         map.width / 2.0f + 0.25f,
                                         map.height / 2.0f + 0.25f,
                                         &camera_x, &camera_y) ||
            !terrain_compute_view_camera(map.tiles, map.width, map.height,
                                         &geometry, camera_x, camera_y,
                                         0, 0, 640, 480, &camera) ||
            !terrain_draw_view_terrain_rgba(map.tiles, map.width, map.height,
                                            &geometry, &terrains, &borders,
                                            &terrain_art, &border_art,
                                            &palette, &camera, &surface,
                                            &view_drawn)) goto done;
        for (size_t i = 3; i < 640 * 480 * 4; i += 4)
            view_pixels += view_rgba[i] != 0;
        if (!view_drawn || !view_pixels) goto done;
        if (argc == 7) {
            TerrainViewVisibility runtime = {
                .tile_masks = visibility_masks,
                .tile_masks_count = (size_t)map.width * map.height,
                .tile_flags = tile_flags,
                .tile_flags_count = (size_t)map.width * map.height,
                .visible_mask = 1,
                .explored_mask = 2,
                .fog_enabled = 1,
                .tile_edges = &edge_index,
                .first = {first_clip, 640 * 480, 640},
                .second = {second_clip, 640 * 480, 640}
            };
            memset(view_rgba, 0, 640 * 480 * 4);
            if (!terrain_draw_view_visibility_rgba(
                    map.tiles, map.width, map.height, &geometry,
                    &terrains, &borders, &terrain_art, &border_art,
                    &palette, &camera, &surface, &runtime, &fog_drawn))
                goto done;
            TerrainViewIndexedSurface indexed_surface = {
                view_indices, 640 * 480, 640, 480, 640,
                0, 0, 640, 480
            };
            size_t indexed_drawn = 0;
            memset(view_indices, 0xaa, 640 * 480);
            RealTileTrace trace = {0};
            if (!terrain_draw_view_visibility_indexed_with_tiles(
                    map.tiles, map.width, map.height, &geometry,
                    &terrains, &borders, &terrain_art, &border_art,
                    &camera, &indexed_surface, &runtime, trace_real_tile,
                    &trace, &indexed_drawn, &object_tiles) ||
                indexed_drawn != fog_drawn || !object_tiles ||
                object_tiles != trace.calls || trace.invalid) goto done;
            for (size_t i = 0; i < 640 * 480; ++i) {
                if (!view_rgba[i * 4 + 3]) {
                    if (view_indices[i] != 0xaa) goto done;
                    continue;
                }
                PaletteColor color = palette.colors[view_indices[i]];
                if (view_rgba[i * 4] != color.r ||
                    view_rgba[i * 4 + 1] != color.g ||
                    view_rgba[i * 4 + 2] != color.b) goto done;
            }
            for (size_t i = 3; i < 640 * 480 * 4; i += 4)
                fog_pixels += view_rgba[i] != 0;
            if (!fog_drawn || !fog_pixels || fog_pixels >= view_pixels)
                goto done;
        }
    }
    printf("draw audit: %" PRIu64 " tiles, %" PRIu64 " layers, %" PRIu64
           " empty tiles, %" PRIu64 " distinct frame sequences blitted; "
           "%zu viewport tiles, %zu opaque viewport pixels; "
           "%zu visibility tiles, %zu visible/fog pixels\n",
           tiles, layers, empty, unique, view_drawn, view_pixels,
           fog_drawn, fog_pixels);
    if (object_tiles)
        printf("object composition seam: %zu projected non-hidden tiles\n",
               object_tiles);
    ok = 1;
done:
    if (!ok) fprintf(stderr, "real terrain draw audit failed\n");
    free(seen);
    free(edge_bytes);
    free(tile_flags);
    free(first_clip);
    free(second_clip);
    free(visibility_masks);
    free(view_rgba);
    free(view_indices);
    free(palette_bytes);
    border_art_free(&border_art);
    terrain_art_free(&terrain_art);
    drs_close(&interface_archive);
    drs_close(&border_archive);
    drs_close(&terrain_archive);
    scn_free_map(&map);
    dat_free_borders(&borders);
    dat_free_terrains(&terrains);
    dat_free_graphics(&graphics);
    dat_free_sounds(&sounds);
    dat_free_intro(&intro);
    dat_free(&blob);
    return ok ? 0 : 1;
}
