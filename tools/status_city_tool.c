/* The status-art city (status/README.md): a scene described by a plan
   file and drawn by the port's own scenario runtime and scene renderer,
   so terrain, borders, player colours, draw order and construction stages
   all come from the translated engine rather than from hand placement.

   The plan is a text file, one directive per line (# comments):
     map N                     square N x N map of grass (first)
     fill T X0 Y0 X1 Y1        terrain T over the inclusive tile rectangle
     player P CIV AGE          player P (1..8): civilization, starting age
                               (1 Tool, 2 Bronze, 3 Iron, 4 post-Iron)
     view W H                  viewport size in pixels
     camera X Y                camera tile (the view centre)
     object P ID X Y [ROT]     a unit of player P (0 Gaia); type-80 units
                               are completed through add-build-work
                               (0x004ad7b0); ROT is a facing in degrees
     foundation P ID X Y PCT   a type-80 unit left at PCT percent of its
                               build time (the construction stage follows
                               0x004ae880); it is the order target
     target                    the most recent object is the order target
     order P ID X Y            a unit created at X,Y and ordered onto the
                               target through the order step (0x00405f20)
     ticks N                   N world updates of 0.05 s
     terrain T SLP N REPL      terrain slot T draws SLP (N flat frames in a
                               row) instead of its own art; REPL is its
                               replacement terrain (-1 none)
     border A B ID             terrain A borders a neighbouring terrain B
                               with border ID (-1 none)

   usage: status_city_tool PLAN EMPIRES.DAT EXP_GRAPHICS.DRS
          BASE_GRAPHICS.DRS TERRAIN.DRS BORDER.DRS EXP_INTERFAC.DRS
          BASE_INTERFAC.DRS TILEEDGE.DAT COL_DIR OUT.ppm */
#include "color_table.h"
#include "live_unit_sprite_backend.h"
#include "live_unit_state.h"
#include "palette.h"
#include "scenario_scene.h"
#include "scn.h"
#include "world_combat.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 8192
#define MAX_FOUNDATIONS 64

typedef struct {
    uint8_t colors[AOE_SCENARIO_RUNTIME_PLAYERS];
    uint8_t active_player;
} SceneResolver;

typedef struct {
    AoeLiveUnit *unit;
    float percent;
} Foundation;

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    uint8_t *bytes = NULL;
    long length;
    if (!file || fseek(file, 0, SEEK_END) != 0 ||
        (length = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0)
        goto done;
    bytes = malloc((size_t)length + 1);
    if (!bytes || fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        free(bytes);
        bytes = NULL;
        goto done;
    }
    bytes[length] = 0;
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
    if (unit->core.class_code == 0x19) owner = unit->doppel.source_owner;
    if (owner < 0 || owner >= AOE_SCENARIO_RUNTIME_PLAYERS) return 0;
    memset(state, 0, sizeof(*state));
    state->active_player = resolver->active_player;
    state->owner_color = resolver->colors[owner];
    if (unit->core.class_code == 0x19) {
        state->type25_controller =
            unit->doppel.source ? &unit->doppel.source->core : NULL;
        state->type25_hidden_players = unit->doppel.hidden_players;
    }
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
                     uint32_t height) {
    FILE *file = fopen(path, "wb");
    int ok = file && fprintf(file, "P6\n%u %u\n255\n", width, height) > 0;
    for (uint32_t y = 0; ok && y < height; ++y)
        for (uint32_t x = 0; ok && x < width; ++x) {
            PaletteColor c = palette->colors[indices[(size_t)y * width + x]];
            uint8_t rgb[3] = {c.r, c.g, c.b};
            ok = fwrite(rgb, 1, 3, file) == 3;
        }
    if (file && fclose(file) != 0) ok = 0;
    return ok;
}

/* The plan's lines, NUL-terminated in place. */
static size_t split_lines(char *text, char **lines, size_t max) {
    size_t n = 0;
    char *p = text;
    while (*p && n < max) {
        char *end = strpbrk(p, "\r\n");
        char *hash;
        if (end) *end = 0;
        hash = strchr(p, '#');
        if (hash) *hash = 0;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p) lines[n++] = p;
        if (!end) break;
        p = end + 1;
    }
    return n;
}

/* An in-memory scenario: the square grass map and the players, in the
   fields the runtime build reads (allied diplomacy, no objects). */
static int make_map(ScnMap *m, uint32_t n) {
    memset(m, 0, sizeof *m);
    if (n < 8 || n > 255) return 0;
    m->width = m->height = n;
    m->tiles = calloc((size_t)n * n, sizeof *m->tiles);
    if (!m->tiles) return 0;
    m->player_count = 8;
    for (int p = 0; p < 8; ++p) {
        m->player_setup[p].enabled = 1;
        m->player_setup[p].civilization_id = 13;
        m->player_details[p].diplomacy_count = 8;
        m->resources[p].food = m->resources[p].wood = 1000.0f;
        m->resources[p].gold = m->resources[p].stone = 1000.0f;
        m->starting_age[p] = 3;
        for (int k = 0; k < 20; ++k) m->tech_toggles[p][k] = 1;
    }
    return 1;
}

static int fill(ScnMap *m, int t, int x0, int y0, int x1, int y1) {
    if (!m->tiles || t < 0 || t > 31 || x0 < 0 || y0 < 0 || x1 >= (int)m->width ||
        y1 >= (int)m->height || x0 > x1 || y0 > y1)
        return 0;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            m->tiles[(size_t)y * m->width + x].terrain_id = (uint8_t)t;
    return 1;
}

static void face(AoeLiveUnit *u, float degrees) {
    (void)aoe_live_unit_low_state_ror(&u->core, u->graphic.current_graphic,
                                      degrees * 3.14159265f / 180.0f);
}

int main(int argc, char **argv) {
    DatBlob blob = {0}; DatIntro intro = {0}; DatSounds sounds = {0};
    DatGraphics graphics = {0}; DatTerrains terrains = {0};
    DatBorders borders = {0}; DatMapGeometry geometry = {0};
    DatRandomMaps random_maps = {0}; DatEffects effects = {0}; DatTechs techs = {0};
    DatCivilizations civs = {0}; ScnMap scenario = {0};
    AoeScenarioRuntimeRor runtime = {0};
    AoeWorldMotionRor motion; AoeWorldCombatRor combat;
    DrsArchive exp_graphics = {0}, base_graphics = {0};
    DrsArchive terrain_drs = {0}, border_drs = {0};
    DrsArchive exp_interface = {0}, base_interface = {0};
    DrsArchive *graphic_archives[6] = {
        &exp_graphics, &base_graphics, &exp_interface, &base_interface,
        &terrain_drs, &border_drs
    };
    /* Terrain art may come from the graphics archives too (the plan can
       put the base game's Road tile, SLP 530 of graphics.drs, on a slot). */
    DrsArchive *terrain_archives[3] = {&terrain_drs, &base_graphics, &exp_graphics};
    DrsArchive *border_archives[1] = {&border_drs};
    DrsArchive *interface_archives[2] = {&exp_interface, &base_interface};
    TerrainArtCache terrain_art = {0}; BorderArtCache border_art = {0};
    AoeGraphicArtCache graphic_cache = {0}; AoeColorTableSet color_tables = {0};
    Palette palette = {0}; DrsMatch palette_match; uint8_t shadow[256];
    uint8_t *palette_bytes = NULL, *edge_bytes = NULL, *plan = NULL;
    uint8_t *pixels = NULL, *first = NULL, *second = NULL;
    size_t edge_size = 0, plan_size = 0, line_count = 0; TileEdgeIndex edges = {0};
    TerrainViewCamera camera; AoeScenarioSceneResultRor result;
    static char *lines[MAX_LINES];
    Foundation foundations[MAX_FOUNDATIONS];
    size_t foundation_count = 0, created = 0, orders = 0, ticks = 0, failed = 0;
    AoeLiveUnit *target = NULL;
    uint32_t width = 960, height = 544, map_size = 0;
    float camera_x = -1.0f, camera_y = -1.0f;
    SceneResolver resolver = {.colors = {8, 0, 1, 2, 3, 4, 5, 6, 7},
                              .active_player = 1};
    int ok = 0, world = 0;
    memset(&motion, 0, sizeof motion);
    memset(&combat, 0, sizeof combat);
    if (argc != 12) {
        fprintf(stderr, "usage: status_city_tool PLAN EMPIRES.DAT "
                "EXP_GRAPHICS.DRS BASE_GRAPHICS.DRS TERRAIN.DRS BORDER.DRS "
                "EXP_INTERFAC.DRS BASE_INTERFAC.DRS TILEEDGE.DAT COL_DIR "
                "OUT.ppm\n");
        return 2;
    }
    plan = read_file(argv[1], &plan_size);
    if (!plan) {
        fprintf(stderr, "cannot read the plan %s\n", argv[1]);
        return 2;
    }
    line_count = split_lines((char *)plan, lines, MAX_LINES);
    if (!dat_load(argv[2], &blob) || !dat_parse_intro(&blob, &intro) ||
        !dat_parse_sounds(&blob, &intro, &sounds) ||
        !dat_parse_graphics(&blob, &sounds, &graphics) ||
        !dat_parse_terrains(&blob, &intro, &graphics, &terrains) ||
        !dat_parse_borders(&blob, &terrains, &borders) ||
        !dat_parse_map_geometry(&blob, &terrains, &borders, &geometry) ||
        !dat_parse_random_maps(&blob, &geometry, &random_maps) ||
        !dat_parse_effects(&blob, &random_maps, &effects) ||
        !dat_parse_civilizations(&blob, &effects, &civs) ||
        !dat_parse_techs(&blob, &civs, &techs)) {
        fprintf(stderr, "the DAT failed to load\n");
        goto done;
    }
    /* Pass 1: the map, the terrain, the players, the view and the DAT
       terrain adjustments (before the terrain art is loaded). */
    for (size_t i = 0; i < line_count; ++i) {
        char word[32];
        int a, b, c, d, e;
        float fx, fy;
        if (sscanf(lines[i], "%31s", word) != 1) continue;
        if (!strcmp(word, "map")) {
            if (sscanf(lines[i], "%*s %d", &a) != 1 || scenario.tiles ||
                !make_map(&scenario, (uint32_t)a))
                goto plan_error;
            map_size = (uint32_t)a;
        } else if (!strcmp(word, "fill")) {
            if (sscanf(lines[i], "%*s %d %d %d %d %d", &a, &b, &c, &d, &e) != 5 ||
                !fill(&scenario, a, b, c, d, e))
                goto plan_error;
        } else if (!strcmp(word, "player")) {
            if (sscanf(lines[i], "%*s %d %d %d", &a, &b, &c) != 3 || a < 1 ||
                a > 8 || b < 0 || b > 16 || c < 1 || c > 4 || !scenario.tiles)
                goto plan_error;
            scenario.player_setup[a - 1].civilization_id = (uint32_t)b;
            scenario.starting_age[a - 1] = c;
        } else if (!strcmp(word, "view")) {
            if (sscanf(lines[i], "%*s %d %d", &a, &b) != 2 || a < 64 || b < 64 ||
                a > 4096 || b > 4096)
                goto plan_error;
            width = (uint32_t)a;
            height = (uint32_t)b;
        } else if (!strcmp(word, "camera")) {
            if (sscanf(lines[i], "%*s %f %f", &fx, &fy) != 2) goto plan_error;
            camera_x = fx;
            camera_y = fy;
        } else if (!strcmp(word, "terrain")) {
            DatTerrain *t;
            if (sscanf(lines[i], "%*s %d %d %d %d", &a, &b, &c, &d) != 4 || a < 0 ||
                a >= terrains.count || b < 0 || c < 1 || c > 255 || d < -1 || d > 31)
                goto plan_error;
            t = &terrains.terrains[a];
            t->enabled = 1;
            t->slp_id = b;
            t->replacement_id = (int16_t)d;
            t->frame_grid[0] = 1;
            t->frame_grid[1] = (int16_t)c;
            for (unsigned k = 0; k < 19; ++k) {
                t->elevation_frames[k].frame_count = (int16_t)c;
                t->elevation_frames[k].angle_count = 1;
                t->elevation_frames[k].frame_start = 0;
            }
        } else if (!strcmp(word, "border")) {
            if (sscanf(lines[i], "%*s %d %d %d", &a, &b, &c) != 3 || a < 0 ||
                a >= terrains.count || b < 0 || b > 31 || c < -1 || c > 15)
                goto plan_error;
            terrains.terrains[a].borders[b] = (int16_t)c;
        } else if (strcmp(word, "object") && strcmp(word, "foundation") &&
                   strcmp(word, "target") && strcmp(word, "order") &&
                   strcmp(word, "ticks")) {
            goto plan_error;
        }
        continue;
    plan_error:
        fprintf(stderr, "plan line %zu is invalid: %s\n", i + 1, lines[i]);
        goto done;
    }
    if (!scenario.tiles) {
        fprintf(stderr, "the plan has no map\n");
        goto done;
    }
    if (!drs_open(&exp_graphics, argv[3]) || !drs_open(&base_graphics, argv[4]) ||
        !drs_open(&terrain_drs, argv[5]) || !drs_open(&border_drs, argv[6]) ||
        !drs_open(&exp_interface, argv[7]) || !drs_open(&base_interface, argv[8]) ||
        !terrain_art_load_direct(&terrain_art, &terrains, terrain_archives, 3) ||
        !border_art_load_direct(&border_art, &borders, border_archives, 1) ||
        !drs_find_first(interface_archives, 2, drs_type("anib"), 50500,
                        &palette_match) ||
        !aoe_shadow_table_load(interface_archives, 2, shadow) ||
        !aoe_color_table_set_load_directory(&intro, argv[10], &color_tables) ||
        !aoe_color_table_set_load_drs_fallback(
            &intro, interface_archives, 2, &color_tables) ||
        !aoe_graphic_art_cache_init(&graphic_cache, 128, 64u * 1024u * 1024u)) {
        fprintf(stderr, "an archive or the terrain art failed to load\n");
        goto done;
    }
    if (!aoe_scenario_runtime_build_ror(&runtime, &scenario, &intro, &civs,
                                        &graphics, &terrains, &borders,
                                        &geometry, &techs, &effects, NULL, 1) ||
        !aoe_scenario_runtime_set_active_player_ror(&runtime, 1) ||
        !aoe_world_motion_init_ror(&motion, &runtime) ||
        !aoe_world_combat_init_ror(&combat, &motion)) {
        fprintf(stderr, "the runtime or the world failed to build\n");
        goto done;
    }
    world = 1;
    /* Pass 2: the objects, the orders and the world time. */
    for (size_t i = 0; i < line_count; ++i) {
        char word[32];
        int p, id, n;
        float x, y, rot, pct;
        AoeLiveUnit *u = NULL;
        if (sscanf(lines[i], "%31s", word) != 1) continue;
        if (!strcmp(word, "object")) {
            int fields = sscanf(lines[i], "%*s %d %d %f %f %f", &p, &id, &x, &y, &rot);
            if (fields < 4 || p < 0 || p > 8 || id < 0) goto plan_error2;
            if (!aoe_scenario_runtime_editor_create_ror(&runtime, (uint8_t)p, (uint16_t)id,
                                                        x, y, 0.0f, &u) || !u) {
                fprintf(stderr, "plan line %zu: object %d of player %d could not be "
                        "created at %.1f,%.1f\n", i + 1, id, p, x, y);
                ++failed;
                continue;
            }
            ++created;
            if (u->core.unit_template && u->core.unit_template->type == 80) {
                int completed = 0;
                if (!aoe_scenario_runtime_add_build_work_ror(&runtime, u, 1.0e9f,
                                                             &completed) || !completed) {
                    fprintf(stderr, "plan line %zu: object %d was not completed\n",
                            i + 1, id);
                    goto done;
                }
            }
            if (fields == 5) face(u, rot);
            target = u;
        } else if (!strcmp(word, "foundation")) {
            if (sscanf(lines[i], "%*s %d %d %f %f %f", &p, &id, &x, &y, &pct) != 5 ||
                p < 0 || p > 8 || id < 0 || !(pct >= 0.0f && pct <= 100.0f) ||
                foundation_count == MAX_FOUNDATIONS)
                goto plan_error2;
            if (!aoe_scenario_runtime_editor_create_ror(&runtime, (uint8_t)p, (uint16_t)id,
                                                        x, y, 0.0f, &u) || !u ||
                !u->core.unit_template || u->core.unit_template->type != 80 ||
                !u->building_update) {
                fprintf(stderr, "plan line %zu: foundation %d of player %d could not be "
                        "created at %.1f,%.1f\n", i + 1, id, p, x, y);
                goto done;
            }
            ++created;
            if (pct >= 100.0f) {
                int completed = 0;
                if (!aoe_scenario_runtime_add_build_work_ror(&runtime, u, 1.0e9f,
                                                             &completed) || !completed)
                    goto done;
            }
            foundations[foundation_count].unit = u;
            foundations[foundation_count].percent = pct;
            ++foundation_count;
            target = u;
        } else if (!strcmp(word, "target")) {
            if (!target) goto plan_error2;
        } else if (!strcmp(word, "order")) {
            int16_t code = 0;
            if (sscanf(lines[i], "%*s %d %d %f %f", &p, &id, &x, &y) != 4 || p < 0 ||
                p > 8 || id < 0 || !target)
                goto plan_error2;
            if (!aoe_scenario_runtime_editor_create_ror(&runtime, (uint8_t)p, (uint16_t)id,
                                                        x, y, 0.0f, &u) || !u) {
                fprintf(stderr, "plan line %zu: worker %d could not be created at "
                        "%.1f,%.1f\n", i + 1, id, x, y);
                ++failed;
                continue;
            }
            ++created;
            if (!aoe_world_combat_order_ror(&combat, u, target, target->core.position.x38,
                                            target->core.position.y3c,
                                            target->core.position.z40, &code)) {
                fprintf(stderr, "plan line %zu: the order failed\n", i + 1);
                ++failed;
                continue;
            }
            ++orders;
        } else if (!strcmp(word, "ticks")) {
            if (sscanf(lines[i], "%*s %d", &n) != 1 || n < 0 || n > 100000)
                goto plan_error2;
            for (int k = 0; k < n; ++k)
                if (!aoe_world_combat_update_world_ror(&combat, 0.05f)) {
                    fprintf(stderr, "plan line %zu: the world update failed\n", i + 1);
                    goto done;
                }
            ticks += (size_t)n;
        }
        continue;
    plan_error2:
        fprintf(stderr, "plan line %zu is invalid: %s\n", i + 1, lines[i]);
        goto done;
    }
    /* The foundations' stages: the elapsed build time (+0x1d4) is set to
       the requested share of the build time and one world update selects
       the construction angle (0x004ae880). */
    for (size_t k = 0; k < foundation_count; ++k) {
        Foundation *f = &foundations[k];
        if (f->percent < 100.0f && f->unit->building_update)
            f->unit->building_update->elapsed_build_time =
                f->percent / 100.0f * (float)f->unit->core.unit_template->train_time;
    }
    if (foundation_count && !aoe_world_combat_update_world_ror(&combat, 0.001f)) goto done;

    palette_bytes = malloc(palette_match.entry->size);
    edge_bytes = read_file(argv[9], &edge_size);
    pixels = calloc((size_t)width * height, 1);
    first = calloc((size_t)width * height, 1);
    second = calloc((size_t)width * height, 1);
    if (!palette_bytes || !edge_bytes || !pixels || !first || !second ||
        !drs_read(palette_match.archive, palette_match.entry, palette_bytes,
                  palette_match.entry->size) ||
        !palette_parse_jasc(palette_bytes, palette_match.entry->size, &palette) ||
        !tileedge_parse(edge_bytes, edge_size, &edges)) {
        fprintf(stderr, "the palette or TILEEDGE failed to load\n");
        goto done;
    }
    /* The water colours cycle at run time (0x004843d0); the shipped palette
       holds black in the cycled slots, so the first cycle step is applied. */
    palette_water_cycle_apply(&palette, 0);
    if (camera_x < 0.0f) camera_x = (float)map_size / 2.0f;
    if (camera_y < 0.0f) camera_y = (float)map_size / 2.0f;
    if (!terrain_clamp_camera_tiles(runtime.map.width, runtime.map.height,
            camera_x, camera_y, &camera_x, &camera_y) ||
        !terrain_compute_view_camera(runtime.map.tiles, runtime.map.width,
            runtime.map.height, &geometry, camera_x, camera_y, 0, 0,
            (int32_t)width, (int32_t)height, &camera)) {
        fprintf(stderr, "the camera is invalid\n");
        goto done;
    }
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
    if (!write_ppm(argv[11], &palette, pixels, width, height)) goto done;
    printf("city: %zu objects created, %zu failed, %zu orders, %zu ticks; camera "
           "%.2f,%.2f view %ux%u; %zu terrain tiles, %zu object tiles, %zu/%zu units "
           "drawn, %" PRIu64 " art skipped -> %s\n",
           created, failed, orders, ticks, camera_x, camera_y, width, height,
           result.terrain_tiles, result.object_tiles, result.units.drawn,
           result.units.visited, backend.missing_skipped, argv[11]);
    for (size_t k = 0; k < foundation_count; ++k)
        printf("foundation %d: %g%%, state %d, construction angle %d\n",
               (int)foundations[k].unit->core.unit_template->id, (double)foundations[k].percent,
               (int)foundations[k].unit->core.state,
               (int)foundations[k].unit->building_update->angle);
    ok = failed == 0;
done:
    if (!ok) fprintf(stderr, "status city render failed\n");
    if (world) {
        aoe_world_combat_destroy_ror(&combat);
        aoe_world_motion_destroy_ror(&motion);
    }
    aoe_scenario_runtime_destroy_ror(&runtime);
    aoe_graphic_art_cache_clear(&graphic_cache);
    free(second); free(first); free(pixels); free(edge_bytes); free(palette_bytes);
    free(plan);
    free(scenario.tiles);
    border_art_free(&border_art); terrain_art_free(&terrain_art);
    drs_close(&base_interface); drs_close(&exp_interface);
    drs_close(&border_drs); drs_close(&terrain_drs);
    drs_close(&base_graphics); drs_close(&exp_graphics);
    dat_free_civilizations(&civs); dat_free_techs(&techs);
    dat_free_effects(&effects); dat_free_random_maps(&random_maps);
    dat_free_borders(&borders); dat_free_terrains(&terrains);
    dat_free_graphics(&graphics); dat_free_sounds(&sounds);
    dat_free_intro(&intro); dat_free(&blob);
    return ok ? 0 : 1;
}
