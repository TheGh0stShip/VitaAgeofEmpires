#include "color_table.h"
#include "live_unit_sprite_backend.h"
#include "palette.h"
#include "scenario_scene.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t colors[AOE_SCENARIO_RUNTIME_PLAYERS];
    uint8_t active_player;
} SceneResolver;

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    uint8_t *bytes = NULL;
    long length;
    if (!file || fseek(file, 0, SEEK_END) != 0 ||
        (length = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0)
        goto done;
    bytes = malloc((size_t)length);
    if (!bytes || fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        free(bytes);
        bytes = NULL;
        goto done;
    }
    *size = (size_t)length;
done:
    if (file) fclose(file);
    return bytes;
}

static int resolve_unit(void *opaque, const AoeLiveUnit *unit,
                        AoeLiveUnitViewportStateRor *state) {
    const SceneResolver *resolver = opaque;
    int owner;
    if (!resolver || !unit || !state) return 0;
    owner = unit->runtime_owner_index;
    if (owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    memset(state, 0, sizeof(*state));
    state->active_player = resolver->active_player;
    state->owner_color = resolver->colors[owner];
    return 1;
}

static int resolve_color(void *opaque, const void *selected,
                         uint8_t fallback, uint8_t *color,
                         const uint8_t **table) {
    AoeLiveUnitIndexedBackend *backend = opaque;
    (void)selected;
    if (!backend || !color || !table) return 0;
    *color = fallback;
    *table = backend->shadow_table;
    return 1;
}

static int write_ppm(const char *path, const Palette *palette,
                     const uint8_t *indices, uint32_t width,
                     uint32_t height, size_t stride) {
    FILE *file = fopen(path, "wb");
    int ok = file && fprintf(file, "P6\n%u %u\n255\n", width, height) > 0;
    for (uint32_t y = 0; ok && y < height; ++y)
        for (uint32_t x = 0; ok && x < width; ++x) {
            PaletteColor c = palette->colors[indices[(size_t)y * stride + x]];
            uint8_t rgb[3] = {c.r, c.g, c.b};
            ok = fwrite(rgb, 1, 3, file) == 3;
        }
    if (file && fclose(file) != 0) ok = 0;
    return ok;
}

static int select_dense_camera(const AoeScenarioRuntimeRor *runtime,
                               float *camera_x, float *camera_y,
                               size_t *nearby) {
    size_t best = 0;
    int best_owners = 0;
    float best_x = 0.0f, best_y = 0.0f;
    if (!runtime || !camera_x || !camera_y || !nearby) return 0;
    for (int p = 1; p < AOE_SCENARIO_RUNTIME_PLAYERS; ++p)
        for (uint32_t i = 0; i < runtime->players[p].unit_count; ++i) {
            const AoeLiveUnit *candidate = &runtime->players[p].units[i];
            size_t count = 0;
            unsigned owner_bits = 0;
            if (!candidate->constructed) continue;
            for (int q = 1; q < AOE_SCENARIO_RUNTIME_PLAYERS; ++q)
                for (uint32_t j = 0; j < runtime->players[q].unit_count; ++j) {
                    const AoeLiveUnit *unit = &runtime->players[q].units[j];
                    float dx, dy;
                    if (!unit->constructed) continue;
                    dx = unit->core.position.x38 - candidate->core.position.x38;
                    dy = unit->core.position.y3c - candidate->core.position.y3c;
                    if (dx >= -12.0f && dx <= 12.0f &&
                        dy >= -12.0f && dy <= 12.0f) {
                        ++count;
                        owner_bits |= 1u << q;
                    }
                }
            int owners = 0;
            for (unsigned bits = owner_bits; bits; bits >>= 1)
                owners += bits & 1u;
            if (owners > best_owners || (owners == best_owners && count > best)) {
                best_owners = owners;
                best = count;
                best_x = candidate->core.position.x38;
                best_y = candidate->core.position.y3c;
            }
        }
    if (!best) return 0;
    *camera_x = best_x;
    *camera_y = best_y;
    *nearby = best;
    return 1;
}

int main(int argc, char **argv) {
    DatBlob blob = {0}; DatIntro intro = {0}; DatSounds sounds = {0};
    DatGraphics graphics = {0}; DatTerrains terrains = {0};
    DatBorders borders = {0}; DatMapGeometry geometry = {0};
    DatRandomMaps random_maps = {0}; DatEffects effects = {0}; DatTechs techs = {0};
    DatCivilizations civs = {0}; ScnMap scenario = {0};
    AoeScenarioRuntimeRor runtime = {0};
    DrsArchive exp_graphics = {0}, base_graphics = {0};
    DrsArchive terrain_drs = {0}, border_drs = {0};
    DrsArchive exp_interface = {0}, base_interface = {0};
    DrsArchive *graphic_archives[6] = {
        &exp_graphics, &base_graphics, &exp_interface, &base_interface,
        &terrain_drs, &border_drs
    };
    DrsArchive *terrain_archives[1] = {&terrain_drs};
    DrsArchive *border_archives[1] = {&border_drs};
    DrsArchive *interface_archives[2] = {&exp_interface, &base_interface};
    TerrainArtCache terrain_art = {0}; BorderArtCache border_art = {0};
    AoeGraphicArtCache graphic_cache = {0}; AoeColorTableSet color_tables = {0};
    Palette palette = {0}; DrsMatch palette_match; uint8_t shadow[256];
    uint8_t *palette_bytes = NULL, *edge_bytes = NULL;
    uint8_t *pixels = NULL, *first = NULL, *second = NULL;
    size_t edge_size = 0; TileEdgeIndex edges = {0};
    TerrainViewCamera camera; AoeScenarioSceneResultRor result;
    const uint32_t width = 960, height = 544;
    SceneResolver resolver = {.colors = {8, 0, 1, 2, 3, 4, 5, 6, 7},
                              .active_player = 1};
    int ok = 0;
    if (argc != 12 && argc != 13) {
        fprintf(stderr, "usage: scenario_scene_tool SCN EMPIRES.DAT "
                "EXP_GRAPHICS.DRS BASE_GRAPHICS.DRS TERRAIN.DRS BORDER.DRS "
                "EXP_INTERFAC.DRS BASE_INTERFAC.DRS TILEEDGE.DAT COL_DIR "
                "OUT.ppm [--auto-dense]\n");
        return 2;
    }
    if (!dat_load(argv[2], &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !dat_parse_techs(&blob, &civs, &techs) ||
        !scn_load_map(argv[1], (uint8_t)intro.terrain_count, &scenario) ||
        !drs_open(&exp_graphics, argv[3]) || !drs_open(&base_graphics, argv[4]) ||
        !drs_open(&terrain_drs, argv[5]) || !drs_open(&border_drs, argv[6]) ||
        !drs_open(&exp_interface, argv[7]) || !drs_open(&base_interface, argv[8]) ||
        !terrain_art_load_direct(&terrain_art, &terrains, terrain_archives, 1) ||
        !border_art_load_direct(&border_art, &borders, border_archives, 1) ||
        !drs_find_first(interface_archives, 2, drs_type("anib"), 50500,
                        &palette_match) ||
        !aoe_shadow_table_load(interface_archives, 2, shadow) ||
        !aoe_color_table_set_load_directory(&intro, argv[10], &color_tables) ||
        !aoe_color_table_set_load_drs_fallback(
            &intro, interface_archives, 2, &color_tables) ||
        !aoe_graphic_art_cache_init(&graphic_cache, 128, 64u * 1024u * 1024u) ||
        !aoe_scenario_runtime_build_ror(&runtime, &scenario, &intro, &civs, &graphics,
            &terrains, &borders, &geometry, &techs, &effects, 1) ||
        !aoe_scenario_runtime_set_active_player_ror(&runtime, 1)) goto done;
    palette_bytes = malloc(palette_match.entry->size);
    edge_bytes = read_file(argv[9], &edge_size);
    pixels = calloc((size_t)width * height, 1);
    first = calloc((size_t)width * height, 1);
    second = calloc((size_t)width * height, 1);
    if (!palette_bytes || !edge_bytes || !pixels || !first || !second ||
        !drs_read(palette_match.archive, palette_match.entry, palette_bytes,
                  palette_match.entry->size) ||
        !palette_parse_jasc(palette_bytes, palette_match.entry->size, &palette) ||
        !tileedge_parse(edge_bytes, edge_size, &edges)) goto done;
    float camera_x = scenario.player_details[0].camera_x;
    float camera_y = scenario.player_details[0].camera_y;
    size_t nearby = 0;
    if (argc == 13 && (strcmp(argv[12], "--auto-dense") != 0 ||
        !select_dense_camera(&runtime, &camera_x, &camera_y, &nearby)))
        goto done;
    if (!terrain_clamp_camera_tiles(runtime.map.width, runtime.map.height,
            camera_x, camera_y, &camera_x, &camera_y) ||
        !terrain_compute_view_camera(runtime.map.tiles, runtime.map.width,
            runtime.map.height, &geometry, camera_x, camera_y, 0, 0,
            (int32_t)width, (int32_t)height, &camera)) goto done;
    TerrainViewIndexedSurface surface = {
        pixels, (size_t)width * height, width, height, width,
        0, 0, width, height
    };
    AoeLiveUnitIndexedBackend backend = {
        .archives = graphic_archives, .archive_count = 6,
        .cache = &graphic_cache, .shadow_table = shadow,
        .skip_missing_art = 1,
        .resolve_fixed_color_table = aoe_color_table_set_resolve,
        .fixed_color_context = &color_tables
    };
    backend.resolve_color_source = resolve_color;
    backend.color_source_context = &backend;
    AoeLiveUnitDrawRorOps ops = {
        .context = &backend, .sprite = aoe_live_unit_sprite_indexed_ror
    };
    AoeScenarioSceneDrawRor draw = {
        .geometry = &geometry, .terrains = &terrains, .borders = &borders,
        .terrain_art = &terrain_art, .border_art = &border_art,
        .camera = &camera, .surface = &surface, .tile_edges = &edges,
        .first_clip = {first, (size_t)width * height, width},
        .second_clip = {second, (size_t)width * height, width},
        /* The original exploration-gate bypass is used for an asset/render
           audit before saved scenario fog history is reconstructed. */
        .fog_enabled = 0, .bypass_exploration_gate = 1,
        .resolve_unit = resolve_unit,
        .resolve_unit_context = &resolver, .draw_ops = &ops
    };
    if (!aoe_scenario_runtime_draw_scene_ror(&runtime, &draw, &result)) {
        fprintf(stderr, "scene draw failed at camera %.2f,%.2f; graphic status %d, "
                "unit %d, graphic %s slp %d\n", camera_x, camera_y,
                backend.last_status, backend.last_unit_id,
                backend.last_graphic ? backend.last_graphic->name : "(null)",
                backend.last_graphic ? backend.last_graphic->slp_id : -1);
        goto done;
    }
    if (!write_ppm(argv[11], &palette, pixels, width, height, width)) goto done;
    printf("scene camera %.2f,%.2f%s%zu; %zu terrain tiles, %zu object tiles, "
           "%zu/%zu units drawn; "
           "cache hits=%" PRIu64 " misses=%" PRIu64
           " unavailable-skipped=%" PRIu64 " -> %s\n",
           camera_x, camera_y, argc == 13 ? " nearby=" : "", nearby,
           result.terrain_tiles, result.object_tiles, result.units.drawn,
           result.units.visited, graphic_cache.hits, graphic_cache.misses,
           backend.missing_skipped, argv[11]);
    ok = 1;
done:
    if (!ok) fprintf(stderr, "real scenario scene render failed\n");
    aoe_scenario_runtime_destroy_ror(&runtime);
    aoe_graphic_art_cache_clear(&graphic_cache);
    free(second); free(first); free(pixels); free(edge_bytes); free(palette_bytes);
    border_art_free(&border_art); terrain_art_free(&terrain_art);
    drs_close(&base_interface); drs_close(&exp_interface);
    drs_close(&border_drs); drs_close(&terrain_drs);
    drs_close(&base_graphics); drs_close(&exp_graphics);
    scn_free_map(&scenario); dat_free_civilizations(&civs);
    dat_free_techs(&techs);
    dat_free_effects(&effects); dat_free_random_maps(&random_maps);
    dat_free_borders(&borders); dat_free_terrains(&terrains);
    dat_free_graphics(&graphics); dat_free_sounds(&sounds);
    dat_free_intro(&intro); dat_free(&blob);
    return ok ? 0 : 1;
}
