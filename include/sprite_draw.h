#ifndef VITA_AOE_SPRITE_DRAW_H
#define VITA_AOE_SPRITE_DRAW_H

#include "graphic_art.h"
#include "graphic_art_cache.h"
#include "graphic_effects.h"
#include "sprite_runtime.h"

typedef struct {
    uint8_t *rgba;
    size_t size;
    uint32_t width, height;
    size_t stride;
} AoeSpriteDrawSurface;

typedef struct {
    uint8_t *indices;
    size_t size;
    uint32_t width, height;
    size_t stride;
} AoeSpriteIndexSurface;

typedef void (*AoeSpriteSpecialPixel)(void *context,
                                       const SlpPixel *pixel,
                                       uint8_t rgba[4]);

typedef struct {
    const DatGraphic *graphic;
    AoeGraphicFrame frame;
    int32_t anchor_x, anchor_y;
} AoeSpriteDrawPart;

typedef const uint8_t *(*AoeSpriteFixedColorTableRor)(
    void *context, int16_t color_id);

/* RoR 0x004a6dd0 / 1.0a 0x004a55e0 selects the caller's table for a
   negative DAT color, otherwise the registered table for that fixed ID. */
const uint8_t *aoe_sprite_draw_transform_table_ror(
    const DatGraphic *graphic, const uint8_t *selected_table,
    AoeSpriteFixedColorTableRor fixed_table, void *fixed_context);

/* RoR 0x00415dc0 -> 0x00416190/0x004166e0 -> 0x004a7110 draw plan.
   It applies static-instance offsets to both coordinate pairs, selects the
   secondary pair for graphic layer < 11 and primary otherwise, and expands
   direct delta graphics with their angle filter and offsets. If parts is
   NULL, only the required count is returned. */
int aoe_sprite_draw_instance_plan_ror(
    const AoeSpriteInstance *instance, uint16_t angle,
    int16_t left, int16_t top, int16_t right, int16_t bottom,
    AoeSpriteDrawPart *parts, size_t capacity, size_t *part_count);

/* Resolves the anchor passed to the original graphic draw method. */
int aoe_sprite_draw_position(const AoeSpriteInstance *instance,
                              int32_t anchor_x, int32_t anchor_y,
                              int32_t *out_x, int32_t *out_y);

/* Original sprite draw dispatch: static RoR 0x00416190 / base 0x004e8a40
   adds instance X/Y offsets; animated RoR 0x004166e0 / base 0x004e9060
   does not. Selects the original angle/frame, loads SLP art, and blits it.
   Return values are AOE_GRAPHIC_ART_*; special pixel effects require the
   original palette resolver supplied by the caller. */
int aoe_sprite_draw_instance_rgba(const AoeSpriteInstance *instance,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  const Palette *palette,
                                  const AoeSpriteDrawSurface *surface,
                                  AoeSpriteSpecialPixel special,
                                  void *special_context);

/* Same draw path, borrowing decoded frames from a bounded cache. A frame
   larger than its budget falls back to the direct loader for correctness. */
int aoe_sprite_draw_instance_cached_rgba(const AoeSpriteInstance *instance,
                                         uint16_t angle, int32_t anchor_x,
                                         int32_t anchor_y,
                                         DrsArchive *const *archives,
                                         size_t archive_count,
                                         AoeGraphicArtCache *cache,
                                         const Palette *palette,
                                         const AoeSpriteDrawSurface *surface,
                                         AoeSpriteSpecialPixel special,
                                         void *special_context);

/* Original player-color OR transform for frames containing only ordinary and
   player-color pixels. Explicitly rejects shadows/outlines until their
   destination effects are ported. The nonnegative DAT graphic player-color
   field overrides the zero-based owning-unit color supplied here, matching
   RoR 0x004a6dd0. */
int aoe_sprite_draw_instance_player_cached_rgba(
                                  const AoeSpriteInstance *instance,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const Palette *palette,
                                  const AoeSpriteDrawSurface *surface,
                                  uint8_t player_color);

/* Indexed original-renderer path. Shadow lookup is applied to the prior
   destination index; pass NULL only for frames with no shadow pixels.
   Outlines are rejected until their exact original rules are translated. */
int aoe_sprite_draw_instance_cached_indexed(
                                  const AoeSpriteInstance *instance,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const AoeSpriteIndexSurface *surface,
                                  uint8_t player_color,
                                  const uint8_t shadow_table[256]);

/* Full two-anchor/delta form used by the live-unit draw path. */
int aoe_sprite_draw_instance_bounds_cached_indexed_ror(
                                  const AoeSpriteInstance *instance,
                                  uint16_t angle,
                                  int16_t left, int16_t top,
                                  int16_t right, int16_t bottom,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const AoeSpriteIndexSurface *surface,
                                  uint8_t player_color,
                                  const uint8_t selected_table[256],
                                  AoeSpriteFixedColorTableRor fixed_table,
                                  void *fixed_context);

/* RoR 0x00415dc0 / base 0x0044fce0: draw sprites in priority-list order.
   The original void dispatch continues after an unavailable sprite. This
   portable version also continues, then reports the first failure; later
   sprites may have changed the destination. */
int aoe_sprite_draw_runtime_rgba(const AoeSpriteRuntime *runtime,
                                 uint16_t angle, int32_t anchor_x,
                                 int32_t anchor_y,
                                 DrsArchive *const *archives,
                                 size_t archive_count,
                                 const Palette *palette,
                                 const AoeSpriteDrawSurface *surface,
                                 AoeSpriteSpecialPixel special,
                                 void *special_context);

int aoe_sprite_draw_runtime_cached_rgba(const AoeSpriteRuntime *runtime,
                                        uint16_t angle, int32_t anchor_x,
                                        int32_t anchor_y,
                                        DrsArchive *const *archives,
                                        size_t archive_count,
                                        AoeGraphicArtCache *cache,
                                        const Palette *palette,
                                        const AoeSpriteDrawSurface *surface,
                                        AoeSpriteSpecialPixel special,
                                        void *special_context);

/* Same continue-on-error list traversal for the indexed destination. */
int aoe_sprite_draw_runtime_cached_indexed(
                                  const AoeSpriteRuntime *runtime,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const AoeSpriteIndexSurface *surface,
                                  uint8_t player_color,
                                  const uint8_t shadow_table[256]);

int aoe_sprite_draw_runtime_bounds_cached_indexed_ror(
                                  const AoeSpriteRuntime *runtime,
                                  uint16_t angle,
                                  int16_t left, int16_t top,
                                  int16_t right, int16_t bottom,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const AoeSpriteIndexSurface *surface,
                                  uint8_t player_color,
                                  const uint8_t selected_table[256],
                                  AoeSpriteFixedColorTableRor fixed_table,
                                  void *fixed_context);

#endif
