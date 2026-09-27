#include "sprite_draw.h"

#include <limits.h>
#include <stdlib.h>

static int add_i32(int32_t value, int32_t offset, int32_t *out) {
    int64_t sum = (int64_t)value + offset;
    if (!out || sum < INT32_MIN || sum > INT32_MAX) return 0;
    *out = (int32_t)sum;
    return 1;
}

const uint8_t *aoe_sprite_draw_transform_table_ror(
    const DatGraphic *graphic, const uint8_t *selected_table,
    AoeSpriteFixedColorTableRor fixed_table, void *fixed_context) {
    if (!graphic) return NULL;
    if (graphic->player_color < 0) return selected_table;
    return fixed_table ? fixed_table(fixed_context, graphic->player_color) :
                         selected_table;
}

int aoe_sprite_draw_instance_plan_ror(
    const AoeSpriteInstance *instance, uint16_t angle,
    int16_t left, int16_t top, int16_t right, int16_t bottom,
    AoeSpriteDrawPart *parts, size_t capacity, size_t *part_count) {
    const DatGraphic *root;
    int16_t animation_frame;
    int32_t primary_x = left, primary_y = top;
    int32_t secondary_x = right, secondary_y = bottom;
    size_t needed = 0;
    if (!instance || !part_count || !instance->graphic ||
        (instance->kind != 1 && instance->kind != 2)) return 0;
    root = instance->graphic;
    if (!root->present || !root->angle_count || !root->frame_count) return 0;
    if (angle >= root->angle_count) angle = 0;
    animation_frame = instance->kind == 2 ? instance->animation.frame : 0;
    if (instance->kind == 1 &&
        (!add_i32(primary_x, instance->offset_x, &primary_x) ||
         !add_i32(primary_y, instance->offset_y, &primary_y) ||
         !add_i32(secondary_x, instance->offset_x, &secondary_x) ||
         !add_i32(secondary_y, instance->offset_y, &secondary_y))) return 0;
    if (!root->delta_count || !root->deltas) {
        needed = 1;
        if (parts && capacity) {
            parts[0].graphic = root;
            parts[0].anchor_x = root->layer < 11 ? secondary_x : primary_x;
            parts[0].anchor_y = root->layer < 11 ? secondary_y : primary_y;
            if (!aoe_graphic_select_frame(root, angle, animation_frame,
                                           &parts[0].frame)) return 0;
        }
    } else {
        for (uint16_t i = 0; i < root->delta_count; ++i) {
            const DatGraphicDelta *delta = &root->deltas[i];
            const DatGraphic *graphic = delta->graphic;
            uint16_t delta_angle;
            int32_t x, y;
            if (delta->display_angle >= 0 &&
                (uint16_t)delta->display_angle != angle) continue;
            if (!graphic || !graphic->present || !graphic->angle_count)
                return 0;
            if (parts && needed < capacity) {
                delta_angle = (uint16_t)(((uint32_t)graphic->angle_count *
                                          angle) / root->angle_count);
                x = graphic->layer < 11 ? secondary_x : primary_x;
                y = graphic->layer < 11 ? secondary_y : primary_y;
                if (!add_i32(x, delta->offset_x, &x) ||
                    !add_i32(y, delta->offset_y, &y)) return 0;
                parts[needed].graphic = graphic;
                parts[needed].anchor_x = x;
                parts[needed].anchor_y = y;
                if (!aoe_graphic_select_frame(graphic, delta_angle,
                                               animation_frame,
                                               &parts[needed].frame)) return 0;
            }
            ++needed;
        }
    }
    *part_count = needed;
    return !parts || capacity >= needed;
}

int aoe_sprite_draw_position(const AoeSpriteInstance *instance,
                              int32_t anchor_x, int32_t anchor_y,
                              int32_t *out_x, int32_t *out_y) {
    if (!instance || !out_x || !out_y ||
        (instance->kind != 1 && instance->kind != 2))
        return 0;
    int64_t x = anchor_x, y = anchor_y;
    if (instance->kind == 1) {
        x += instance->offset_x;
        y += instance->offset_y;
    }
    if (x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX)
        return 0;
    *out_x = (int32_t)x;
    *out_y = (int32_t)y;
    return 1;
}

static int draw_instance_rgba(const AoeSpriteInstance *instance,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const Palette *palette,
                                  const AoeSpriteDrawSurface *surface,
                                  AoeSpriteSpecialPixel special,
                                  void *special_context,
                                  int player_only, uint8_t player_color) {
    if (!instance || !palette || !surface || !surface->rgba ||
        !archives || !archive_count)
        return AOE_GRAPHIC_ART_INVALID;
    AoeGraphicFrame selection;
    if (!aoe_sprite_instance_frame(instance, angle, &selection))
        return AOE_GRAPHIC_ART_INVALID;
    int32_t x, y;
    if (!aoe_sprite_draw_position(instance, anchor_x, anchor_y, &x, &y))
        return AOE_GRAPHIC_ART_INVALID;
    SlpFrame owned = {0};
    const SlpFrame *frame = NULL;
    int status = cache ? aoe_graphic_art_cache_get(cache, instance->graphic,
                                                   &selection, archives,
                                                   archive_count, &frame) :
                         AOE_GRAPHIC_ART_CACHE_LIMIT;
    if (status == AOE_GRAPHIC_ART_CACHE_LIMIT) {
        status = aoe_graphic_art_load_frame(instance->graphic, &selection,
                                            archives, archive_count, &owned);
        frame = &owned;
    }
    if (status != AOE_GRAPHIC_ART_OK)
        return status;
    if (player_only && !aoe_graphic_player_only(frame)) {
        slp_free_frame(&owned);
        return AOE_GRAPHIC_ART_INVALID;
    }
    if (player_only && instance->graphic->player_color >= 0)
        player_color = (uint8_t)instance->graphic->player_color;
    AoeGraphicPlayerEffects effects = {palette, player_color};
    if (player_only) {
        special = aoe_graphic_apply_player;
        special_context = &effects;
    }
    int ok = slp_blit_graphic_rgba(frame, palette, surface->rgba,
                                    surface->size, surface->width,
                                    surface->height, surface->stride,
                                    x, y,
                                    selection.mirror_horizontal,
                                    special, special_context);
    slp_free_frame(&owned);
    return ok ? AOE_GRAPHIC_ART_OK : AOE_GRAPHIC_ART_INVALID;
}

int aoe_sprite_draw_instance_cached_rgba(const AoeSpriteInstance *instance,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const Palette *palette,
                                  const AoeSpriteDrawSurface *surface,
                                  AoeSpriteSpecialPixel special,
                                  void *special_context) {
    return draw_instance_rgba(instance, angle, anchor_x, anchor_y,
            archives, archive_count, cache, palette, surface, special,
            special_context, 0, 0);
}

int aoe_sprite_draw_instance_player_cached_rgba(
                                  const AoeSpriteInstance *instance,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const Palette *palette,
                                  const AoeSpriteDrawSurface *surface,
                                  uint8_t player_color) {
    return draw_instance_rgba(instance, angle, anchor_x, anchor_y,
            archives, archive_count, cache, palette, surface, NULL, NULL,
            1, player_color);
}

int aoe_sprite_draw_instance_rgba(const AoeSpriteInstance *instance,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  const Palette *palette,
                                  const AoeSpriteDrawSurface *surface,
                                  AoeSpriteSpecialPixel special,
                                  void *special_context) {
    return aoe_sprite_draw_instance_cached_rgba(instance, angle,
            anchor_x, anchor_y, archives, archive_count, NULL, palette,
            surface, special, special_context);
}

int aoe_sprite_draw_runtime_cached_rgba(const AoeSpriteRuntime *runtime,
                                 uint16_t angle, int32_t anchor_x,
                                 int32_t anchor_y,
                                 DrsArchive *const *archives,
                                 size_t archive_count,
                                 AoeGraphicArtCache *cache,
                                 const Palette *palette,
                                 const AoeSpriteDrawSurface *surface,
                                 AoeSpriteSpecialPixel special,
                                 void *special_context) {
    if (!runtime)
        return AOE_GRAPHIC_ART_INVALID;
    size_t count = aoe_sprite_list_count(&runtime->list);
    int first_error = AOE_GRAPHIC_ART_OK;
    for (size_t i = 0; i < count; ++i) {
        const AoeSpriteInstance *instance = aoe_sprite_runtime_at(runtime, i);
        int status = aoe_sprite_draw_instance_cached_rgba(instance, angle,
                                                   anchor_x, anchor_y,
                                                   archives, archive_count,
                                                   cache,
                                                   palette, surface,
                                                   special,
                                                   special_context);
        if (status != AOE_GRAPHIC_ART_OK &&
            first_error == AOE_GRAPHIC_ART_OK)
            first_error = status;
    }
    return first_error;
}

int aoe_sprite_draw_runtime_rgba(const AoeSpriteRuntime *runtime,
                                 uint16_t angle, int32_t anchor_x,
                                 int32_t anchor_y,
                                 DrsArchive *const *archives,
                                 size_t archive_count,
                                 const Palette *palette,
                                 const AoeSpriteDrawSurface *surface,
                                 AoeSpriteSpecialPixel special,
                                 void *special_context) {
    return aoe_sprite_draw_runtime_cached_rgba(runtime, angle, anchor_x,
            anchor_y, archives, archive_count, NULL, palette, surface,
            special, special_context);
}

int aoe_sprite_draw_instance_cached_indexed(
                                  const AoeSpriteInstance *instance,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const AoeSpriteIndexSurface *surface,
                                  uint8_t player_color,
                                  const uint8_t shadow_table[256]) {
    if (!instance || !surface || !surface->indices ||
        !archives || !archive_count)
        return AOE_GRAPHIC_ART_INVALID;
    AoeGraphicFrame selection;
    int32_t x, y;
    if (!aoe_sprite_instance_frame(instance, angle, &selection) ||
        !aoe_sprite_draw_position(instance, anchor_x, anchor_y, &x, &y))
        return AOE_GRAPHIC_ART_INVALID;
    SlpFrame owned = {0};
    const SlpFrame *frame = NULL;
    int status = cache ? aoe_graphic_art_cache_get(cache, instance->graphic,
                    &selection, archives, archive_count, &frame) :
                    AOE_GRAPHIC_ART_CACHE_LIMIT;
    if (status == AOE_GRAPHIC_ART_CACHE_LIMIT) {
        status = aoe_graphic_art_load_frame(instance->graphic, &selection,
                    archives, archive_count, &owned);
        frame = &owned;
    }
    if (status != AOE_GRAPHIC_ART_OK)
        return status;
    if (instance->graphic->player_color >= 0)
        player_color = (uint8_t)instance->graphic->player_color;
    int ok = slp_blit_graphic_indexed(frame, surface->indices,
                    surface->size, surface->width, surface->height,
                    surface->stride, x, y, selection.mirror_horizontal,
                    player_color, shadow_table);
    slp_free_frame(&owned);
    return ok ? AOE_GRAPHIC_ART_OK : AOE_GRAPHIC_ART_INVALID;
}

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
                                  void *fixed_context) {
    AoeSpriteDrawPart stack_parts[16];
    AoeSpriteDrawPart *parts = stack_parts;
    size_t count = 0;
    int first_error = AOE_GRAPHIC_ART_OK;
    if (!instance || !surface || !surface->indices || !archives ||
        !archive_count ||
        !aoe_sprite_draw_instance_plan_ror(instance, angle, left, top,
                                            right, bottom,
                                            NULL, 0, &count))
        return AOE_GRAPHIC_ART_INVALID;
    if (count > sizeof(stack_parts) / sizeof(stack_parts[0])) {
        if (count > SIZE_MAX / sizeof(*parts)) return AOE_GRAPHIC_ART_INVALID;
        parts = malloc(count * sizeof(*parts));
        if (!parts) return AOE_GRAPHIC_ART_INVALID;
    }
    if (!aoe_sprite_draw_instance_plan_ror(instance, angle, left, top,
                                            right, bottom,
                                            parts, count, &count)) {
        if (parts != stack_parts) free(parts);
        return AOE_GRAPHIC_ART_INVALID;
    }
    for (size_t i = 0; i < count; ++i) {
        SlpFrame owned = {0};
        const SlpFrame *frame = NULL;
        int status = cache ? aoe_graphic_art_cache_get(
            cache, parts[i].graphic, &parts[i].frame,
            archives, archive_count, &frame) : AOE_GRAPHIC_ART_CACHE_LIMIT;
        if (status == AOE_GRAPHIC_ART_CACHE_LIMIT) {
            status = aoe_graphic_art_load_frame(parts[i].graphic,
                &parts[i].frame, archives, archive_count, &owned);
            frame = &owned;
        }
        if (status == AOE_GRAPHIC_ART_OK) {
            uint8_t color = parts[i].graphic->player_color >= 0 ?
                (uint8_t)parts[i].graphic->player_color : player_color;
            const uint8_t *table = aoe_sprite_draw_transform_table_ror(
                parts[i].graphic, selected_table, fixed_table, fixed_context);
            if (!slp_blit_graphic_indexed(frame, surface->indices,
                    surface->size, surface->width, surface->height,
                    surface->stride, parts[i].anchor_x, parts[i].anchor_y,
                    parts[i].frame.mirror_horizontal, color, table))
                status = AOE_GRAPHIC_ART_INVALID;
        }
        slp_free_frame(&owned);
        if (status != AOE_GRAPHIC_ART_OK &&
            first_error == AOE_GRAPHIC_ART_OK) first_error = status;
    }
    if (parts != stack_parts) free(parts);
    return first_error;
}

int aoe_sprite_draw_runtime_cached_indexed(
                                  const AoeSpriteRuntime *runtime,
                                  uint16_t angle, int32_t anchor_x,
                                  int32_t anchor_y,
                                  DrsArchive *const *archives,
                                  size_t archive_count,
                                  AoeGraphicArtCache *cache,
                                  const AoeSpriteIndexSurface *surface,
                                  uint8_t player_color,
                                  const uint8_t shadow_table[256]) {
    if (!runtime)
        return AOE_GRAPHIC_ART_INVALID;
    size_t count = aoe_sprite_list_count(&runtime->list);
    int first_error = AOE_GRAPHIC_ART_OK;
    for (size_t i = 0; i < count; ++i) {
        const AoeSpriteInstance *instance = aoe_sprite_runtime_at(runtime, i);
        int status = aoe_sprite_draw_instance_cached_indexed(instance,
                    angle, anchor_x, anchor_y, archives, archive_count,
                    cache, surface, player_color, shadow_table);
        if (status != AOE_GRAPHIC_ART_OK &&
            first_error == AOE_GRAPHIC_ART_OK)
            first_error = status;
    }
    return first_error;
}

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
                                  void *fixed_context) {
    int first_error = AOE_GRAPHIC_ART_OK;
    if (!runtime) return AOE_GRAPHIC_ART_INVALID;
    size_t count = aoe_sprite_list_count(&runtime->list);
    for (size_t i = 0; i < count; ++i) {
        const AoeSpriteInstance *instance = aoe_sprite_runtime_at(runtime, i);
        int status = aoe_sprite_draw_instance_bounds_cached_indexed_ror(
            instance, angle, left, top, right, bottom,
            archives, archive_count, cache, surface, player_color,
            selected_table, fixed_table, fixed_context);
        if (status != AOE_GRAPHIC_ART_OK &&
            first_error == AOE_GRAPHIC_ART_OK) first_error = status;
    }
    return first_error;
}
