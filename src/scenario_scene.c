#include "scenario_scene.h"

#include <stdlib.h>
#include <string.h>

static int ensure_scratch(AoeScenarioRuntimeRor *runtime, size_t needed) {
    AoeTileObjectDrawEntry *next;
    if (needed <= runtime->draw_scratch_capacity) return 1;
    if (needed > SIZE_MAX / sizeof(*next)) return 0;
    next = realloc(runtime->draw_scratch, needed * sizeof(*next));
    if (!next) return 0;
    runtime->draw_scratch = next;
    runtime->draw_scratch_capacity = needed;
    return 1;
}

int aoe_scenario_runtime_draw_scene_ror(
    AoeScenarioRuntimeRor *runtime, const AoeScenarioSceneDrawRor *draw,
    AoeScenarioSceneResultRor *result) {
    AoeRuntimeViewportRor viewport = {0};
    TerrainViewVisibility visibility = {0};
    AoeSpriteIndexSurface sprite_surface;
    AoePlayerVisionState *active;
    AoeScenarioSceneResultRor next = {0};
    size_t tile_count, max_objects = 0;
    if (!runtime || !runtime->initialized || runtime->active_player < 0 ||
        runtime->active_player >= AOE_SCENARIO_RUNTIME_PLAYERS || !draw ||
        !draw->geometry || !draw->terrains || !draw->borders ||
        !draw->terrain_art || !draw->border_art || !draw->camera ||
        !draw->surface || !draw->surface->indices || !draw->tile_edges ||
        !draw->resolve_unit || !draw->draw_ops || !draw->draw_ops->sprite ||
        !runtime->map.tiles || !runtime->map.objects ||
        !runtime->map.draw_flags || !runtime->map.masks.masks)
        return 0;
    tile_count = (size_t)runtime->map.width * runtime->map.height;
    for (size_t i = 0; i < tile_count; ++i)
        if (runtime->map.objects[i].count > max_objects)
            max_objects = runtime->map.objects[i].count;
    if (!ensure_scratch(runtime, max_objects)) return 0;

    active = &runtime->players[runtime->active_player].vision;
    visibility.tile_masks = runtime->map.masks.masks;
    visibility.tile_masks_count = tile_count;
    visibility.tile_flags = runtime->map.draw_flags;
    visibility.tile_flags_count = tile_count;
    visibility.visible_mask = active->grid.visible_mask;
    visibility.explored_mask = active->grid.explored_mask;
    visibility.fog_enabled = draw->fog_enabled;
    visibility.bypass_exploration_gate = draw->bypass_exploration_gate;
    visibility.tile_edges = draw->tile_edges;
    visibility.black_edges = draw->black_edges;
    visibility.first = draw->first_clip;
    visibility.second = draw->second_clip;

    sprite_surface.indices = draw->surface->indices;
    sprite_surface.size = draw->surface->indices_size;
    sprite_surface.width = draw->surface->width;
    sprite_surface.height = draw->surface->height;
    sprite_surface.stride = draw->surface->stride;
    sprite_surface.queue = NULL;
    if (!draw->immediate_sprites) {
        /* View sprite list rows -200 .. height + 249 (0x00512536). */
        aoe_sprite_draw_queue_reset(&runtime->sprite_queue, -200,
                                    (int32_t)draw->surface->height + 249);
        sprite_surface.queue = &runtime->sprite_queue;
    }
    viewport.map = &runtime->map;
    viewport.scratch = runtime->draw_scratch;
    viewport.scratch_capacity = runtime->draw_scratch_capacity;
    viewport.renderer = &sprite_surface;
    viewport.resolve_unit = draw->resolve_unit;
    viewport.resolve_unit_context = draw->resolve_unit_context;
    viewport.resolve_entry = draw->resolve_entry;
    viewport.resolve_entry_context = draw->resolve_entry_context;
    viewport.draw_ops = draw->draw_ops;
    if (!terrain_draw_view_visibility_indexed_with_tiles(
            runtime->map.tiles, runtime->map.width, runtime->map.height,
            draw->geometry, draw->terrains, draw->borders,
            draw->terrain_art, draw->border_art, draw->camera, draw->surface,
            &visibility, aoe_runtime_viewport_draw_tile_ror, &viewport,
            &next.terrain_tiles, &next.object_tiles))
        return 0;
    if (sprite_surface.queue) {
        AoeSpriteIndexSurface target = sprite_surface;
        target.queue = NULL;
        if (aoe_sprite_draw_queue_flush(&runtime->sprite_queue, &target) !=
            AOE_GRAPHIC_ART_OK)
            return 0;
    }
    next.units = viewport.result;
    if (result) *result = next;
    return 1;
}
