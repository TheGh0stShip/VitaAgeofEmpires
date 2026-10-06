#include "terrain_draw.h"

#include "terrain.h"

#include <string.h>

static void append_layer(TerrainDrawPlan *plan, uint8_t kind, uint8_t slot,
                         uint32_t frame) {
    TerrainDrawLayer *layer = &plan->layers[plan->layer_count++];
    layer->kind = kind;
    layer->slot = slot;
    layer->frame = frame;
}

int terrain_build_sprite_pass_plan(uint8_t tile_flags, int first_active,
                                    int second_active,
                                    TerrainSpritePassPlan *plan) {
    TerrainSpritePassPlan selected = {0};
    uint32_t fill = (uint32_t)(tile_flags & 0x0f) * 0x10001000u +
                    0x04000400u;
    if (!plan) return 0;
    selected.even_mask = 0x00ff00ffu;
    selected.odd_mask = 0xff00ff00u;
    if (tile_flags & 0x10) {
        selected.even_fill = 0x28002800u;
        selected.odd_fill = 0x00280028u;
        selected.passes[0] = (TerrainSpritePass){
            TERRAIN_SPRITE_CLIP_DEFAULT, 1
        };
        selected.count = 1;
    } else {
        if (tile_flags & 0x20) {
            selected.even_fill = 0x56005600u;
            selected.odd_fill = 0x00560056u;
        }
        if (tile_flags & 0x80)
            selected.even_fill = selected.odd_fill = fill;
        if (second_active)
            selected.passes[selected.count++] = (TerrainSpritePass){
                TERRAIN_SPRITE_CLIP_SECOND, 1
            };
        if (first_active)
            selected.passes[selected.count++] = (TerrainSpritePass){
                TERRAIN_SPRITE_CLIP_FIRST,
                (uint8_t)((tile_flags & 0xa0) != 0)
            };
    }
    *plan = selected;
    return 1;
}

int terrain_build_draw_plan(const DatTerrains *terrains,
                            const DatBorders *borders,
                            const TerrainArtCache *terrain_art,
                            const BorderArtCache *border_art,
                            uint8_t raw_terrain_id, uint8_t shape,
                            uint8_t border_code, uint32_t x, uint32_t y,
                            TerrainDrawPlan *plan) {
    uint8_t draw_id, border_id, pattern;
    const DatBorder *border = NULL;
    const BorderArtEntry *border_sprite = NULL;
    uint32_t base_frame, border_frame;
    int draw_base = 1;
    int have_border = 0;
    if (!plan || !terrains || !borders || !terrain_art || !border_art ||
        !terrains->terrains || !borders->borders ||
        !terrain_art->entries || !border_art->entries ||
        shape > 16 || x > 32767 || y > 32767 ||
        terrain_art->count < terrains->count ||
        border_art->count < borders->count ||
        !terrain_resolve_draw_id(terrains, raw_terrain_id, &draw_id)) return 0;
    memset(plan, 0, sizeof(*plan));
    plan->draw_terrain_id = draw_id;
    plan->shape = shape;
    plan->border_code = border_code;
    if (!terrains->terrains[draw_id].enabled ||
        !terrain_art->entries[draw_id].frames) return 1;

    border_id = border_code & 15;
    pattern = border_code >> 4;
    if (border_id && border_id < borders->count &&
        borders->borders[border_id].enabled &&
        border_art->entries[border_id].frames) {
        border = &borders->borders[border_id];
        border_sprite = &border_art->entries[border_id];
        if (border->border_style == 0) {
            if (terrain_select_border_frame(border, shape, pattern, x, y,
                                            &border_frame) &&
                border_frame < border_sprite->frame_count) {
                have_border = 1;
                draw_base = (uint8_t)border->draw_base != 0;
            }
        } else if (pattern != 0) {
            have_border = 1;
        }
    }
    if (draw_base && terrain_select_frame(&terrains->terrains[draw_id],
                                          shape, x, y, &base_frame) &&
        base_frame < terrain_art->entries[draw_id].frame_count)
        append_layer(plan, TERRAIN_DRAW_TERRAIN, draw_id, base_frame);
    if (!have_border) return 1;
    if (border->border_style == 0) {
        append_layer(plan, TERRAIN_DRAW_BORDER, border_id, border_frame);
        return 1;
    }
    /* Style 1's four independent cardinal components are drawn in the
       original order: mask bits 1, 4, 8, 2 -> patterns 1, 3, 2, 4. */
    static const uint8_t masks[4] = {1, 4, 8, 2};
    static const uint8_t patterns[4] = {1, 3, 2, 4};
    for (unsigned i = 0; i < 4; ++i) {
        if (!(pattern & masks[i])) continue;
        if (terrain_select_border_frame(border, shape, patterns[i], x, y,
                                        &border_frame) &&
            border_frame < border_sprite->frame_count)
            append_layer(plan, TERRAIN_DRAW_BORDER, border_id, border_frame);
    }
    return 1;
}

int terrain_blit_draw_plan_rgba(const TerrainDrawPlan *plan,
                                const TerrainArtCache *terrain_art,
                                const BorderArtCache *border_art,
                                const Palette *palette, uint8_t *rgba,
                                size_t rgba_size, uint32_t width,
                                uint32_t height, size_t stride,
                                int32_t anchor_x, int32_t anchor_y) {
    const SlpFrame *frames[5];
    int32_t draw_x[5], draw_y[5];
    if (!plan || !terrain_art || !border_art || !palette || !rgba ||
        plan->layer_count > 5 || !terrain_art->entries ||
        !border_art->entries || !width || !height || !stride ||
        (size_t)width > stride / 4 ||
        (size_t)width * 4 > rgba_size ||
        (size_t)(height - 1) >
            (rgba_size - (size_t)width * 4) / stride)
        return 0;
    for (uint8_t i = 0; i < plan->layer_count; ++i) {
        const TerrainDrawLayer *layer = &plan->layers[i];
        const TerrainArtCache *cache;
        const TerrainArtEntry *art;
        const SlpFrame *frame;
        int64_t x, y;
        if (layer->kind == TERRAIN_DRAW_TERRAIN) cache = terrain_art;
        else if (layer->kind == TERRAIN_DRAW_BORDER) cache = border_art;
        else return 0;
        if (layer->slot >= cache->count) return 0;
        art = &cache->entries[layer->slot];
        if (!art->frames || layer->frame >= art->frame_count) return 0;
        frame = &art->frames[layer->frame];
        if (!frame->pixels || !frame->width || !frame->height ||
            frame->width > SIZE_MAX / frame->height) return 0;
        for (size_t j = 0; j < (size_t)frame->width * frame->height; ++j)
            if (frame->pixels[j].kind != SLP_COLOR &&
                frame->pixels[j].kind != SLP_TRANSPARENT) return 0;
        x = (int64_t)anchor_x - frame->hotspot_x;
        y = (int64_t)anchor_y - frame->hotspot_y;
        if (x < INT32_MIN || x > INT32_MAX ||
            y < INT32_MIN || y > INT32_MAX) return 0;
        frames[i] = frame;
        draw_x[i] = (int32_t)x;
        draw_y[i] = (int32_t)y;
    }
    for (uint8_t i = 0; i < plan->layer_count; ++i)
        if (!slp_blit_terrain_rgba(frames[i], palette, rgba, rgba_size,
                                   width, height, stride, draw_x[i],
                                   draw_y[i])) return 0;
    return 1;
}

static int clip_mask_valid(TerrainClipMask mask, uint32_t width,
                           uint32_t height) {
    return mask.bytes && width <= mask.stride && width <= mask.size &&
           (size_t)(height - 1) <= (mask.size - width) / mask.stride;
}

int terrain_blit_draw_plan_passes_rgba(const TerrainDrawPlan *plan,
                                       const TerrainArtCache *terrain_art,
                                       const BorderArtCache *border_art,
                                       const Palette *palette,
                                       const TerrainSpritePassPlan *passes,
                                       TerrainClipMask first,
                                       TerrainClipMask second,
                                       uint8_t *rgba, size_t rgba_size,
                                       uint32_t width, uint32_t height,
                                       size_t stride, int32_t anchor_x,
                                       int32_t anchor_y,
                                       int32_t screen_origin_x,
                                       int32_t screen_origin_y) {
    const SlpFrame *frames[5];
    int64_t draw_x[5], draw_y[5];
    if (!plan || !terrain_art || !border_art || !palette || !passes || !rgba ||
        !terrain_art->entries || !border_art->entries ||
        plan->layer_count > 5 || passes->count > 2 || !width || !height ||
        (size_t)width > stride / 4 || (size_t)width * 4 > rgba_size ||
        (size_t)(height - 1) >
            (rgba_size - (size_t)width * 4) / stride)
        return 0;
    for (uint8_t i = 0; i < passes->count; ++i) {
        const TerrainSpritePass *pass = &passes->passes[i];
        if (pass->dither_mode > 1 ||
            (pass->clip != TERRAIN_SPRITE_CLIP_DEFAULT &&
             pass->clip != TERRAIN_SPRITE_CLIP_FIRST &&
             pass->clip != TERRAIN_SPRITE_CLIP_SECOND) ||
            (pass->clip == TERRAIN_SPRITE_CLIP_FIRST &&
             !clip_mask_valid(first, width, height)) ||
            (pass->clip == TERRAIN_SPRITE_CLIP_SECOND &&
             !clip_mask_valid(second, width, height)))
            return 0;
    }
    for (uint8_t i = 0; i < plan->layer_count; ++i) {
        const TerrainDrawLayer *layer = &plan->layers[i];
        const TerrainArtCache *cache;
        const TerrainArtEntry *art;
        const SlpFrame *frame;
        if (layer->kind == TERRAIN_DRAW_TERRAIN) cache = terrain_art;
        else if (layer->kind == TERRAIN_DRAW_BORDER) cache = border_art;
        else return 0;
        if (layer->slot >= cache->count) return 0;
        art = &cache->entries[layer->slot];
        if (!art->frames || layer->frame >= art->frame_count) return 0;
        frame = &art->frames[layer->frame];
        if (!frame->pixels || !frame->width || !frame->height ||
            frame->width > SIZE_MAX / frame->height) return 0;
        for (size_t j = 0; j < (size_t)frame->width * frame->height; ++j)
            if (frame->pixels[j].kind != SLP_COLOR &&
                frame->pixels[j].kind != SLP_TRANSPARENT) return 0;
        draw_x[i] = (int64_t)anchor_x - frame->hotspot_x;
        draw_y[i] = (int64_t)anchor_y - frame->hotspot_y;
        frames[i] = frame;
    }
    for (uint8_t i = 0; i < plan->layer_count; ++i) {
        const SlpFrame *frame = frames[i];
        for (uint8_t p = 0; p < passes->count; ++p) {
            const TerrainSpritePass *pass = &passes->passes[p];
            TerrainClipMask clip = pass->clip == TERRAIN_SPRITE_CLIP_FIRST ?
                                   first : second;
            for (uint32_t sy = 0; sy < frame->height; ++sy) {
                int64_t dy = draw_y[i] + sy;
                if (dy < 0 || dy >= height) continue;
                for (uint32_t sx = 0; sx < frame->width; ++sx) {
                    int64_t dx = draw_x[i] + sx;
                    const SlpPixel *pixel;
                    uint8_t index;
                    PaletteColor color;
                    uint8_t *out;
                    if (dx < 0 || dx >= width) continue;
                    pixel = &frame->pixels[(size_t)sy * frame->width + sx];
                    if (pixel->kind == SLP_TRANSPARENT) continue;
                    if (pass->clip != TERRAIN_SPRITE_CLIP_DEFAULT &&
                        !clip.bytes[(size_t)dy * clip.stride + (size_t)dx])
                        continue;
                    index = pixel->index;
                    if (pass->dither_mode) {
                        int64_t screen_x = (int64_t)screen_origin_x + dx;
                        int64_t screen_y = (int64_t)screen_origin_y + dy;
                        uint32_t bit = (uint32_t)((screen_x & 3) * 8);
                        uint32_t mask = screen_y & 1 ? passes->odd_mask :
                                                        passes->even_mask;
                        uint32_t fill = screen_y & 1 ? passes->odd_fill :
                                                        passes->even_fill;
                        index = (uint8_t)((index & (mask >> bit)) |
                                          (fill >> bit));
                    }
                    color = palette->colors[index];
                    out = rgba + (size_t)dy * stride + (size_t)dx * 4;
                    out[0] = color.r;
                    out[1] = color.g;
                    out[2] = color.b;
                    out[3] = 255;
                }
            }
        }
    }
    return 1;
}

/* Only color/transparent pixels: the load-time flag when present. */
static int frame_plain(const TerrainArtEntry *art, uint32_t index) {
    const SlpFrame *frame = &art->frames[index];
    if (art->plain_frames) return art->plain_frames[index];
    for (size_t j = 0; j < (size_t)frame->width * frame->height; ++j)
        if (frame->pixels[j].kind != SLP_COLOR &&
            frame->pixels[j].kind != SLP_TRANSPARENT) return 0;
    return 1;
}

int terrain_blit_draw_plan_passes_indexed(const TerrainDrawPlan *plan,
                                          const TerrainArtCache *terrain_art,
                                          const BorderArtCache *border_art,
                                          const TerrainSpritePassPlan *passes,
                                          TerrainClipMask first,
                                          TerrainClipMask second,
                                          uint8_t *indices,
                                          size_t indices_size,
                                          uint32_t width, uint32_t height,
                                          size_t stride,
                                          int32_t anchor_x, int32_t anchor_y,
                                          int32_t screen_origin_x,
                                          int32_t screen_origin_y) {
    const SlpFrame *frames[5];
    int64_t draw_x[5], draw_y[5];
    if (!plan || !terrain_art || !border_art || !passes || !indices ||
        !terrain_art->entries || !border_art->entries ||
        plan->layer_count > 5 || passes->count > 2 || !width || !height ||
        (size_t)width > stride || (size_t)width > indices_size ||
        (size_t)(height - 1) >
            (indices_size - (size_t)width) / stride)
        return 0;
    for (uint8_t i = 0; i < passes->count; ++i) {
        const TerrainSpritePass *pass = &passes->passes[i];
        if (pass->dither_mode > 1 ||
            (pass->clip != TERRAIN_SPRITE_CLIP_DEFAULT &&
             pass->clip != TERRAIN_SPRITE_CLIP_FIRST &&
             pass->clip != TERRAIN_SPRITE_CLIP_SECOND) ||
            (pass->clip == TERRAIN_SPRITE_CLIP_FIRST &&
             !clip_mask_valid(first, width, height)) ||
            (pass->clip == TERRAIN_SPRITE_CLIP_SECOND &&
             !clip_mask_valid(second, width, height)))
            return 0;
    }
    for (uint8_t i = 0; i < plan->layer_count; ++i) {
        const TerrainDrawLayer *layer = &plan->layers[i];
        const TerrainArtCache *cache;
        const TerrainArtEntry *art;
        const SlpFrame *frame;
        if (layer->kind == TERRAIN_DRAW_TERRAIN) cache = terrain_art;
        else if (layer->kind == TERRAIN_DRAW_BORDER) cache = border_art;
        else return 0;
        if (layer->slot >= cache->count) return 0;
        art = &cache->entries[layer->slot];
        if (!art->frames || layer->frame >= art->frame_count) return 0;
        frame = &art->frames[layer->frame];
        if (!frame->pixels || !frame->width || !frame->height ||
            frame->width > SIZE_MAX / frame->height) return 0;
        if (!frame_plain(art, layer->frame)) return 0;
        draw_x[i] = (int64_t)anchor_x - frame->hotspot_x;
        draw_y[i] = (int64_t)anchor_y - frame->hotspot_y;
        frames[i] = frame;
    }
    /* Same pixel order and results as a per-pixel bounds test: the visible
       column range is clipped once per layer and the dither selectors once
       per row. Frame and viewport sizes are bounded far below 2^31, so the
       clipped coordinates fit 32-bit arithmetic (cheap on 32-bit ARM). */
    for (uint8_t i = 0; i < plan->layer_count; ++i) {
        const SlpFrame *frame = frames[i];
        int64_t x_lo = draw_x[i] < 0 ? -draw_x[i] : 0;
        int64_t x_hi = (int64_t)width - draw_x[i];
        int64_t y_lo = draw_y[i] < 0 ? -draw_y[i] : 0;
        int64_t y_hi = (int64_t)height - draw_y[i];
        uint32_t sx0, sx1, sy0, sy1;
        if (x_hi > frame->width) x_hi = frame->width;
        if (y_hi > frame->height) y_hi = frame->height;
        if (x_lo >= x_hi || y_lo >= y_hi) continue;
        sx0 = (uint32_t)x_lo;
        sx1 = (uint32_t)x_hi;
        sy0 = (uint32_t)y_lo;
        sy1 = (uint32_t)y_hi;
        for (uint8_t p = 0; p < passes->count; ++p) {
            const TerrainSpritePass *pass = &passes->passes[p];
            TerrainClipMask clip = pass->clip == TERRAIN_SPRITE_CLIP_FIRST ?
                                   first : second;
            int clipped = pass->clip != TERRAIN_SPRITE_CLIP_DEFAULT;
            for (uint32_t sy = sy0; sy < sy1; ++sy) {
                size_t dy = (size_t)(draw_y[i] + sy);
                size_t dx0 = (size_t)(draw_x[i] + sx0);
                const SlpPixel *src =
                    &frame->pixels[(size_t)sy * frame->width + sx0];
                uint8_t *dst = indices + dy * stride + dx0;
                const uint8_t *mask_row =
                    clipped ? clip.bytes + dy * clip.stride + dx0 : NULL;
                uint32_t n = sx1 - sx0;
                if (!pass->dither_mode && frame->row_spans &&
                    frame->row_spans[2 * sy] != SLP_ROW_MIXED) {
                    /* Transparent outside one color run. */
                    uint32_t lo = frame->row_spans[2 * sy];
                    uint32_t hi = frame->row_spans[2 * sy + 1];
                    if (lo < sx0) lo = sx0;
                    if (hi > sx1) hi = sx1;
                    if (!clipped) {
                        for (uint32_t x = lo; x < hi; ++x)
                            dst[x - sx0] = src[x - sx0].index;
                    } else {
                        for (uint32_t x = lo; x < hi; ++x) {
                            uint32_t k = x - sx0;
                            dst[k] = mask_row[k] ? src[k].index : dst[k];
                        }
                    }
                } else if (!pass->dither_mode) {
                    for (uint32_t k = 0; k < n; ++k) {
                        if (src[k].kind == SLP_TRANSPARENT) continue;
                        if (clipped && !mask_row[k]) continue;
                        dst[k] = src[k].index;
                    }
                } else {
                    int64_t screen_y = (int64_t)screen_origin_y + (int64_t)dy;
                    uint32_t mask = screen_y & 1 ? passes->odd_mask :
                                                    passes->even_mask;
                    uint32_t fill = screen_y & 1 ? passes->odd_fill :
                                                    passes->even_fill;
                    uint32_t phase =
                        (uint32_t)(((int64_t)screen_origin_x + (int64_t)dx0) &
                                   3);
                    for (uint32_t k = 0; k < n; ++k) {
                        uint32_t bit = ((phase + k) & 3) * 8;
                        if (src[k].kind == SLP_TRANSPARENT) continue;
                        if (clipped && !mask_row[k]) continue;
                        dst[k] = (uint8_t)((src[k].index & (mask >> bit)) |
                                           (fill >> bit));
                    }
                }
            }
        }
    }
    return 1;
}

typedef struct {
    const ScnTile *tiles;
    uint32_t map_width, map_height;
    const DatMapGeometry *geometry;
    const DatTerrains *terrains;
    const DatBorders *borders;
    const TerrainArtCache *terrain_art;
    const BorderArtCache *border_art;
    const Palette *palette;
    const TerrainViewCamera *camera;
    const TerrainViewSurface *surface;
    uint8_t *clip_rgba;
    size_t clip_size;
    int indexed;
    const TerrainViewVisibility *visibility;
    TerrainViewTileDraw draw_tile;
    void *draw_tile_context;
    uint8_t edge_table[256];
    size_t drawn, object_tiles;
    int failed;
} TerrainViewDrawContext;

static void draw_view_tile(uint32_t x, uint32_t y, void *opaque) {
    TerrainViewDrawContext *ctx = opaque;
    const ScnTile *tile;
    TerrainDrawPlan plan;
    uint8_t shape, border_code;
    int32_t anchor_x, anchor_y;
    int64_t clip_x, clip_y;
    size_t tile_index = (size_t)y * ctx->map_width + x;
    if (ctx->failed) return;
    tile = &ctx->tiles[tile_index];
    shape = terrain_elevation_shape(ctx->tiles, ctx->map_width,
                                    ctx->map_height, x, y);
    if (shape >= 19 ||
        !terrain_project_tile_view(ctx->tiles, ctx->map_width,
                                   ctx->map_height, x, y, ctx->geometry,
                                   ctx->camera->camera_pixel_x,
                                   ctx->camera->camera_pixel_y,
                                   &anchor_x, &anchor_y)) {
        ctx->failed = 1;
        return;
    }
    /* 0x00514c10 tests the projected anchor against its viewport rectangle
       using DAT tile-size height indexed by elevation shape (tile byte +4). */
    if ((int64_t)anchor_x >
            (int64_t)ctx->surface->clip_left + ctx->surface->clip_width - 1 ||
        (int64_t)anchor_y >
            (int64_t)ctx->surface->clip_top + ctx->surface->clip_height - 1 ||
        (int64_t)ctx->surface->clip_left >=
            (int64_t)anchor_x + ctx->geometry->tile_width ||
        (int64_t)ctx->surface->clip_top >=
            (int64_t)anchor_y + ctx->terrains->tile_sizes[shape].height)
        return;
    if (ctx->visibility) {
        const TerrainViewVisibility *runtime = ctx->visibility;
        uint8_t mode = terrain_view_visibility_code(
            runtime->tile_masks[tile_index], runtime->visible_mask,
            runtime->explored_mask, runtime->bypass_exploration_gate,
            runtime->fog_enabled);
        uint8_t visible_pattern = 0, explored_pattern = 0;
        TileEdgeViewPlan edge_plan;
        TileEdgeCoveragePlane first = runtime->first;
        TileEdgeCoveragePlane second = runtime->second;
        if (mode == TERRAIN_VIEW_HIDDEN) return;
        if (runtime->skip_terrain) {
            if (ctx->draw_tile) {
                if (!ctx->draw_tile(ctx->draw_tile_context, x, y, anchor_x,
                                    anchor_y, mode)) {
                    ctx->failed = 1;
                    return;
                }
                ++ctx->object_tiles;
            }
            return;
        }
        if ((mode == TERRAIN_VIEW_VISIBLE && runtime->fog_enabled &&
             !terrain_visibility_edge_pattern(
                 runtime->tile_masks, ctx->map_width, ctx->map_height,
                 x, y, runtime->visible_mask, ctx->edge_table,
                 &visible_pattern)) ||
            (!runtime->bypass_exploration_gate &&
             !terrain_visibility_edge_pattern(
                 runtime->tile_masks, ctx->map_width, ctx->map_height,
                 x, y, runtime->explored_mask, ctx->edge_table,
                 &explored_pattern)) ||
            !tileedge_build_view_plan(mode, runtime->fog_enabled,
                                      runtime->bypass_exploration_gate,
                                      visible_pattern, explored_pattern,
                                      &edge_plan)) {
            ctx->failed = 1;
            return;
        }
        clip_x = (int64_t)anchor_x - ctx->surface->clip_left;
        clip_y = (int64_t)anchor_y - ctx->surface->clip_top;
        if (shape >= TILEEDGE_SHAPES ||
            clip_x < INT32_MIN || clip_x > INT32_MAX ||
            clip_y < INT32_MIN || clip_y > INT32_MAX ||
            !tileedge_apply_view_plan(runtime->tile_edges,
                                      runtime->black_edges, shape,
                                      &edge_plan, (int32_t)clip_x,
                                      (int32_t)clip_y, first, second,
                                      ctx->surface->clip_width,
                                      ctx->surface->clip_height)) {
            ctx->failed = 1;
            return;
        }
        TerrainSpritePassPlan passes;
        int first_active = edge_plan.first_side != TILEEDGE_SIDE_NONE &&
            runtime->tile_edges->offsets[shape][edge_plan.tile_pattern]
                                        [edge_plan.first_side] != 0;
        int second_active = edge_plan.second_side != TILEEDGE_SIDE_NONE &&
            runtime->tile_edges->offsets[shape][edge_plan.tile_pattern]
                                        [edge_plan.second_side] != 0;
        if (!terrain_build_sprite_pass_plan(runtime->tile_flags[tile_index],
                                             first_active, second_active,
                                             &passes)) {
            ctx->failed = 1;
            return;
        }
        border_code = terrain_border_code(ctx->tiles, ctx->map_width,
                                          ctx->map_height, x, y,
                                          ctx->terrains, ctx->borders);
        if (!terrain_build_draw_plan(ctx->terrains, ctx->borders,
                                     ctx->terrain_art, ctx->border_art,
                                     tile->terrain_id, shape, border_code,
                                     x, y, &plan)) {
            ctx->failed = 1;
            return;
        }
        if (plan.layer_count && passes.count) {
            TerrainClipMask first_mask = {first.bytes, first.size,
                                          first.stride};
            TerrainClipMask second_mask = {second.bytes, second.size,
                                           second.stride};
            int blit_ok = ctx->indexed ?
                terrain_blit_draw_plan_passes_indexed(
                    &plan, ctx->terrain_art, ctx->border_art, &passes,
                    first_mask, second_mask, ctx->clip_rgba, ctx->clip_size,
                    ctx->surface->clip_width, ctx->surface->clip_height,
                    ctx->surface->stride, (int32_t)clip_x, (int32_t)clip_y,
                    (int32_t)ctx->surface->clip_left,
                    (int32_t)ctx->surface->clip_top) :
                terrain_blit_draw_plan_passes_rgba(
                    &plan, ctx->terrain_art, ctx->border_art, ctx->palette,
                    &passes, first_mask, second_mask, ctx->clip_rgba,
                    ctx->clip_size, ctx->surface->clip_width,
                    ctx->surface->clip_height, ctx->surface->stride,
                    (int32_t)clip_x, (int32_t)clip_y,
                    (int32_t)ctx->surface->clip_left,
                    (int32_t)ctx->surface->clip_top);
            if (!blit_ok) {
                ctx->failed = 1;
                return;
            }
            ++ctx->drawn;
        }
        if (ctx->draw_tile) {
            if (!ctx->draw_tile(ctx->draw_tile_context, x, y, anchor_x,
                                anchor_y, mode)) {
                ctx->failed = 1;
                return;
            }
            ++ctx->object_tiles;
        }
        return;
    }
    border_code = terrain_border_code(ctx->tiles, ctx->map_width,
                                      ctx->map_height, x, y,
                                      ctx->terrains, ctx->borders);
    if (!terrain_build_draw_plan(ctx->terrains, ctx->borders,
                                 ctx->terrain_art, ctx->border_art,
                                 tile->terrain_id, shape, border_code,
                                 x, y, &plan)) {
        ctx->failed = 1;
        return;
    }
    if (!plan.layer_count) return;
    clip_x = (int64_t)anchor_x - ctx->surface->clip_left;
    clip_y = (int64_t)anchor_y - ctx->surface->clip_top;
    if (clip_x < INT32_MIN || clip_x > INT32_MAX ||
        clip_y < INT32_MIN || clip_y > INT32_MAX ||
        !terrain_blit_draw_plan_rgba(&plan, ctx->terrain_art, ctx->border_art,
                                      ctx->palette, ctx->clip_rgba,
                                      ctx->clip_size, ctx->surface->clip_width,
                                      ctx->surface->clip_height,
                                      ctx->surface->stride,
                                      (int32_t)clip_x, (int32_t)clip_y)) {
        ctx->failed = 1;
        return;
    }
    ++ctx->drawn;
}

int terrain_draw_view_terrain_rgba(const ScnTile *tiles, uint32_t map_width,
                                   uint32_t map_height,
                                   const DatMapGeometry *geometry,
                                   const DatTerrains *terrains,
                                   const DatBorders *borders,
                                   const TerrainArtCache *terrain_art,
                                   const BorderArtCache *border_art,
                                   const Palette *palette,
                                   const TerrainViewCamera *camera,
                                   const TerrainViewSurface *surface,
                                   size_t *drawn_tiles) {
    TerrainViewDrawContext ctx = {0};
    size_t offset, visited;
    if (!tiles || !geometry || !terrains || !borders || !terrain_art ||
        !border_art || !palette || !camera || !surface || !surface->rgba ||
        !map_width || !map_height || !surface->width || !surface->height ||
        !surface->stride || !surface->clip_width || !surface->clip_height ||
        (size_t)surface->width > surface->stride / 4 ||
        (size_t)surface->width * 4 > surface->rgba_size ||
        (size_t)(surface->height - 1) >
            (surface->rgba_size - (size_t)surface->width * 4) /
            surface->stride ||
        surface->clip_left >= surface->width ||
        surface->clip_top >= surface->height ||
        surface->clip_width > surface->width - surface->clip_left ||
        surface->clip_height > surface->height - surface->clip_top ||
        camera->view_left != (int64_t)surface->clip_left ||
        camera->view_top != (int64_t)surface->clip_top ||
        camera->view_width != (int64_t)surface->clip_width ||
        camera->view_height != (int64_t)surface->clip_height)
        return 0;
    offset = (size_t)surface->clip_top * surface->stride +
             (size_t)surface->clip_left * 4;
    ctx.tiles = tiles;
    ctx.map_width = map_width;
    ctx.map_height = map_height;
    ctx.geometry = geometry;
    ctx.terrains = terrains;
    ctx.borders = borders;
    ctx.terrain_art = terrain_art;
    ctx.border_art = border_art;
    ctx.palette = palette;
    ctx.camera = camera;
    ctx.surface = surface;
    ctx.clip_rgba = surface->rgba + offset;
    ctx.clip_size = surface->rgba_size - offset;
    if (!terrain_walk_view_tiles(map_width, map_height,
                                 camera->start_x, camera->start_y,
                                 camera->diagonal_count,
                                 camera->diagonal_length,
                                 draw_view_tile, &ctx, &visited) || ctx.failed)
        return 0;
    if (drawn_tiles) *drawn_tiles = ctx.drawn;
    return 1;
}

static int view_coverage_valid(TileEdgeCoveragePlane plane, uint32_t width,
                               uint32_t height) {
    return plane.bytes && width <= plane.stride && width <= plane.size &&
           (size_t)(height - 1) <= (plane.size - width) / plane.stride;
}

static int terrain_draw_view_visibility_common(
    const ScnTile *tiles, uint32_t map_width, uint32_t map_height,
    const DatMapGeometry *geometry, const DatTerrains *terrains,
    const DatBorders *borders, const TerrainArtCache *terrain_art,
    const BorderArtCache *border_art, const Palette *palette,
    const TerrainViewCamera *camera, const TerrainViewSurface *surface,
    const TerrainViewVisibility *visibility, size_t *drawn_tiles,
    int indexed, TerrainViewTileDraw draw_tile, void *draw_tile_context,
    size_t *object_tiles) {
    TerrainViewDrawContext ctx = {0};
    size_t tile_count, offset, visited;
    size_t pixel_size = indexed ? 1 : 4;
    if (!tiles || !geometry || !terrains || !borders || !terrain_art ||
        !border_art || (!indexed && !palette) || !camera || !surface ||
        !surface->rgba ||
        !visibility || !visibility->tile_masks || !visibility->tile_flags ||
        !visibility->tile_edges || !visibility->tile_edges->bytes ||
        !map_width || !map_height || map_width > SIZE_MAX / map_height ||
        !surface->width || !surface->height || !surface->stride ||
        !surface->clip_width || !surface->clip_height ||
        (size_t)surface->width > surface->stride / pixel_size ||
        (size_t)surface->width > SIZE_MAX / pixel_size ||
        (size_t)surface->width * pixel_size > surface->rgba_size ||
        (size_t)(surface->height - 1) >
            (surface->rgba_size - (size_t)surface->width * pixel_size) /
            surface->stride ||
        surface->clip_left >= surface->width ||
        surface->clip_top >= surface->height ||
        surface->clip_width > surface->width - surface->clip_left ||
        surface->clip_height > surface->height - surface->clip_top ||
        camera->view_left != (int64_t)surface->clip_left ||
        camera->view_top != (int64_t)surface->clip_top ||
        camera->view_width != (int64_t)surface->clip_width ||
        camera->view_height != (int64_t)surface->clip_height ||
        visibility->first.bytes == visibility->second.bytes ||
        !view_coverage_valid(visibility->first, surface->clip_width,
                             surface->clip_height) ||
        !view_coverage_valid(visibility->second, surface->clip_width,
                             surface->clip_height))
        return 0;
    tile_count = (size_t)map_width * map_height;
    if (visibility->tile_masks_count < tile_count ||
        visibility->tile_flags_count < tile_count ||
        !terrain_build_visibility_edge_table(ctx.edge_table))
        return 0;
    offset = (size_t)surface->clip_top * surface->stride +
             (size_t)surface->clip_left * pixel_size;
    ctx.tiles = tiles;
    ctx.map_width = map_width;
    ctx.map_height = map_height;
    ctx.geometry = geometry;
    ctx.terrains = terrains;
    ctx.borders = borders;
    ctx.terrain_art = terrain_art;
    ctx.border_art = border_art;
    ctx.palette = palette;
    ctx.indexed = indexed;
    ctx.camera = camera;
    ctx.surface = surface;
    ctx.visibility = visibility;
    ctx.draw_tile = draw_tile;
    ctx.draw_tile_context = draw_tile_context;
    ctx.clip_rgba = surface->rgba + offset;
    ctx.clip_size = surface->rgba_size - offset;
    for (uint32_t y = 0; y < surface->clip_height; ++y) {
        memset(visibility->first.bytes + (size_t)y * visibility->first.stride,
               0, surface->clip_width);
        memset(visibility->second.bytes + (size_t)y * visibility->second.stride,
               0, surface->clip_width);
    }
    if (!terrain_walk_view_tiles(map_width, map_height,
                                 camera->start_x, camera->start_y,
                                 camera->diagonal_count,
                                 camera->diagonal_length,
                                 draw_view_tile, &ctx, &visited) || ctx.failed)
        return 0;
    if (drawn_tiles) *drawn_tiles = ctx.drawn;
    if (object_tiles) *object_tiles = ctx.object_tiles;
    return 1;
}

int terrain_draw_view_visibility_rgba(const ScnTile *tiles,
                                      uint32_t map_width,
                                      uint32_t map_height,
                                      const DatMapGeometry *geometry,
                                      const DatTerrains *terrains,
                                      const DatBorders *borders,
                                      const TerrainArtCache *terrain_art,
                                      const BorderArtCache *border_art,
                                      const Palette *palette,
                                      const TerrainViewCamera *camera,
                                      const TerrainViewSurface *surface,
                                      const TerrainViewVisibility *visibility,
                                      size_t *drawn_tiles) {
    return terrain_draw_view_visibility_common(
        tiles, map_width, map_height, geometry, terrains, borders,
        terrain_art, border_art, palette, camera, surface, visibility,
        drawn_tiles, 0, NULL, NULL, NULL);
}

int terrain_draw_view_visibility_indexed(
    const ScnTile *tiles, uint32_t map_width, uint32_t map_height,
    const DatMapGeometry *geometry, const DatTerrains *terrains,
    const DatBorders *borders, const TerrainArtCache *terrain_art,
    const BorderArtCache *border_art, const TerrainViewCamera *camera,
    const TerrainViewIndexedSurface *surface,
    const TerrainViewVisibility *visibility, size_t *drawn_tiles) {
    TerrainViewSurface raw;
    if (!surface) return 0;
    raw.rgba = surface->indices;
    raw.rgba_size = surface->indices_size;
    raw.width = surface->width;
    raw.height = surface->height;
    raw.stride = surface->stride;
    raw.clip_left = surface->clip_left;
    raw.clip_top = surface->clip_top;
    raw.clip_width = surface->clip_width;
    raw.clip_height = surface->clip_height;
    return terrain_draw_view_visibility_common(
        tiles, map_width, map_height, geometry, terrains, borders,
        terrain_art, border_art, NULL, camera, &raw, visibility,
        drawn_tiles, 1, NULL, NULL, NULL);
}

int terrain_draw_view_visibility_indexed_with_tiles(
    const ScnTile *tiles, uint32_t map_width, uint32_t map_height,
    const DatMapGeometry *geometry, const DatTerrains *terrains,
    const DatBorders *borders, const TerrainArtCache *terrain_art,
    const BorderArtCache *border_art, const TerrainViewCamera *camera,
    const TerrainViewIndexedSurface *surface,
    const TerrainViewVisibility *visibility, TerrainViewTileDraw draw_tile,
    void *draw_tile_context, size_t *drawn_tiles, size_t *object_tiles) {
    TerrainViewSurface raw;
    if (!surface || !draw_tile) return 0;
    raw.rgba = surface->indices;
    raw.rgba_size = surface->indices_size;
    raw.width = surface->width;
    raw.height = surface->height;
    raw.stride = surface->stride;
    raw.clip_left = surface->clip_left;
    raw.clip_top = surface->clip_top;
    raw.clip_width = surface->clip_width;
    raw.clip_height = surface->clip_height;
    return terrain_draw_view_visibility_common(
        tiles, map_width, map_height, geometry, terrains, borders,
        terrain_art, border_art, NULL, camera, &raw, visibility,
        drawn_tiles, 1, draw_tile, draw_tile_context, object_tiles);
}
