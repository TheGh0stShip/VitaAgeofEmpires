#include "scenario_scene.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct { size_t resolved, sprites; } Trace;

static int resolve_unit(void *opaque, const AoeLiveUnit *unit,
                        AoeLiveUnitViewportStateRor *state) {
    Trace *trace = opaque;
    assert(unit->constructed);
    memset(state, 0, sizeof(*state));
    state->active_player = 1;
    state->owner_color = 0;
    ++trace->resolved;
    return 1;
}

static int draw_sprite(void *opaque, void *renderer,
                       const AoeLiveUnitCore *unit,
                       const AoeSpriteRuntime *sprites, const void *graphic,
                       uint8_t owner_color, uint8_t frame, int32_t draw_id,
                       int16_t left, int16_t top, int16_t right,
                       int16_t bottom) {
    Trace *trace = opaque;
    AoeSpriteIndexSurface *surface = renderer;
    (void)unit; (void)sprites; (void)graphic; (void)owner_color;
    (void)frame; (void)draw_id; (void)left; (void)top;
    (void)right; (void)bottom;
    assert(surface && surface->width == 64 && surface->height == 32);
    ++trace->sprites;
    return 1;
}

int main(void) {
    AoeScenarioRuntimeRor runtime = {0};
    AoeLiveUnit unit = {0};
    DatUnit unit_template = {.draw_class = 5};
    DatTerrain terrain = {0};
    DatBorder border = {0};
    TerrainArtEntry terrain_entry = {0}, border_entry = {0};
    DatTerrains terrains = {.count = 1, .terrains = &terrain};
    DatBorders borders = {.count = 1, .borders = &border};
    TerrainArtCache terrain_art = {.count = 1, .entries = &terrain_entry};
    BorderArtCache border_art = {.count = 1, .entries = &border_entry};
    DatMapGeometry geometry = {
        .tile_width = 64, .tile_height = 32,
        .tile_half_width = 32, .tile_half_height = 16,
        .elevation_height = 16
    };
    TerrainViewCamera camera = {
        .start_x = 0, .start_y = 0,
        .diagonal_count = 1, .diagonal_length = 1,
        .view_width = 64, .view_height = 32
    };
    uint8_t pixels[64 * 32] = {0};
    uint8_t first[64 * 32], second[64 * 32];
    uint8_t edge_byte = 0xff;
    TileEdgeIndex edges = {.bytes = &edge_byte, .size = 1};
    TerrainViewIndexedSurface surface = {
        pixels, sizeof(pixels), 64, 32, 64, 0, 0, 64, 32
    };
    Trace trace = {0};
    AoeLiveUnitDrawRorOps ops = {.context = &trace, .sprite = draw_sprite};
    AoeScenarioSceneDrawRor draw = {
        .geometry = &geometry, .terrains = &terrains, .borders = &borders,
        .terrain_art = &terrain_art, .border_art = &border_art,
        .camera = &camera, .surface = &surface, .tile_edges = &edges,
        .first_clip = {first, sizeof(first), 64},
        .second_clip = {second, sizeof(second), 64},
        .bypass_exploration_gate = 1,
        .resolve_unit = resolve_unit, .resolve_unit_context = &trace,
        .draw_ops = &ops
    };
    AoeScenarioSceneResultRor result = {0};

    terrains.tile_sizes[0].height = 32;
    runtime.map.width = runtime.map.height = 1;
    runtime.map.tiles = calloc(1, sizeof(*runtime.map.tiles));
    runtime.map.view_codes = calloc(1, 1);
    runtime.map.draw_flags = calloc(1, 1);
    runtime.map.objects = calloc(1, sizeof(*runtime.map.objects));
    runtime.map.masks.width = runtime.map.masks.height = 1;
    runtime.map.masks.masks = calloc(1, sizeof(uint32_t));
    assert(runtime.map.tiles && runtime.map.view_codes &&
           runtime.map.draw_flags && runtime.map.objects &&
           runtime.map.masks.masks);
    runtime.map.masks.masks[0] = 1u << 1;
    runtime.players[1].vision.grid.visible_mask = 1u << 1;
    runtime.players[1].vision.grid.explored_mask = 1u << 17;
    runtime.active_player = 1;
    runtime.initialized = 1;
    unit.constructed = 1;
    unit.core.unit_template = &unit_template;
    unit.core.state = 2;
    unit.core.class_code = 10;
    unit.graphic.current_graphic = (void *)1;
    unit.placement.tile = &runtime.map.objects[0];
    assert(aoe_object_update_list_attach(&runtime.map.objects[0], &unit.core,
                                         &unit.placement.node));
    assert(aoe_scenario_runtime_draw_scene_ror(&runtime, &draw, &result));
    assert(result.object_tiles == 1 && result.terrain_tiles == 0);
    assert(result.units.visited == 1 && result.units.drawn == 1 &&
           result.units.failed == 0 && trace.resolved == 1 &&
           trace.sprites == 1);
    aoe_runtime_map_destroy(&runtime.map);
    free(runtime.draw_scratch);
    return 0;
}
