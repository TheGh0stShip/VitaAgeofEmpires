#include "live_unit_sprite_backend.h"

int aoe_live_unit_sprite_indexed_ror(
    void *context, void *renderer, const AoeLiveUnitCore *unit,
    const AoeSpriteRuntime *sprites, const void *graphic,
    uint8_t owner_color, uint8_t frame, int32_t draw_id,
    int16_t left, int16_t top, int16_t right, int16_t bottom) {
    AoeLiveUnitIndexedBackend *backend = context;
    const AoeSpriteIndexSurface *surface = renderer;
    const uint8_t *transform_table;
    uint8_t player_color = owner_color;
    (void)draw_id; /* Original exposes this through global draw context. */
    if (!backend || !unit || !sprites || !surface || !surface->indices ||
        !backend->archives || !backend->archive_count) return 0;
    backend->last_unit_id = unit->unit_id;
    backend->last_graphic = graphic;
    transform_table = backend->shadow_table;
    if (graphic && backend->resolve_color_source &&
        !backend->resolve_color_source(backend->color_source_context,
            graphic, owner_color, &player_color, &transform_table)) {
        backend->last_status = AOE_GRAPHIC_ART_INVALID;
        return 0;
    }
    if (surface->queue) {
        /* 0x004a8b16: the sprite-list row key is the tile screen y plus
           unit +0x2e, the same sum as `bottom` within screen range. */
        surface->queue->row_key = bottom;
        surface->queue->skip_missing_art = backend->skip_missing_art;
    }
    backend->last_status = aoe_sprite_draw_runtime_bounds_cached_indexed_ror(
        sprites, frame, left, top, right, bottom,
        backend->archives, backend->archive_count, backend->cache,
        surface, player_color, transform_table,
        backend->resolve_fixed_color_table, backend->fixed_color_context);
    if (backend->last_status == AOE_GRAPHIC_ART_MISSING &&
        backend->skip_missing_art) {
        ++backend->missing_skipped;
        return 1;
    }
    return backend->last_status == AOE_GRAPHIC_ART_OK;
}
