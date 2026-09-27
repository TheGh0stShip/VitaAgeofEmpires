#ifndef VITA_AOE_LIVE_UNIT_SPRITE_BACKEND_H
#define VITA_AOE_LIVE_UNIT_SPRITE_BACKEND_H

#include "live_unit_draw.h"
#include "sprite_draw.h"

/* Portable form of the selected player/visibility graphic consumed at
   +0x26/+0x28 by RoR 0x004a6dd0 and patched 1.0a 0x004a55e0. */
typedef int (*AoeLiveUnitColorSourceRor)(
    void *context, const void *selected_graphic,
    uint8_t fallback_player_color, uint8_t *player_color,
    const uint8_t **transform_table);

/* Portable indexed implementation of the sprite-manager callback reached by
   live draw 0x004a8a60. The renderer argument must point to the destination
   AoeSpriteIndexSurface supplied by viewport traversal. */
typedef struct {
    DrsArchive *const *archives;
    size_t archive_count;
    AoeGraphicArtCache *cache;
    const uint8_t *shadow_table; /* 256 entries, or NULL when legal for art */
    int last_status;
    int32_t last_unit_id;
    const DatGraphic *last_graphic;
    uint64_t missing_skipped;
    uint8_t skip_missing_art;
    AoeLiveUnitColorSourceRor resolve_color_source;
    void *color_source_context;
    AoeSpriteFixedColorTableRor resolve_fixed_color_table;
    void *fixed_color_context;
} AoeLiveUnitIndexedBackend;

int aoe_live_unit_sprite_indexed_ror(
    void *context, void *renderer, const AoeLiveUnitCore *unit,
    const AoeSpriteRuntime *sprites, const void *graphic,
    uint8_t owner_color, uint8_t frame, int32_t draw_id,
    int16_t left, int16_t top, int16_t right, int16_t bottom);

#endif
