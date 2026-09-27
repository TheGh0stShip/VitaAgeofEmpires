#include "terrain_draw.h"
#include "palette.h"

#include <assert.h>
#include <string.h>

static void put32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

typedef struct {
    size_t calls;
    size_t fail_on_call;
    uint8_t bad_mode;
} TileDrawTrace;

static int trace_tile_draw(void *opaque, uint32_t tile_x, uint32_t tile_y,
                           int32_t anchor_x, int32_t anchor_y,
                           uint8_t visibility_mode) {
    TileDrawTrace *trace = opaque;
    (void)tile_x;
    (void)tile_y;
    (void)anchor_x;
    (void)anchor_y;
    ++trace->calls;
    if (visibility_mode == TERRAIN_VIEW_HIDDEN) trace->bad_mode = 1;
    return trace->fail_on_call != trace->calls;
}

int main(void) {
    TerrainSpritePassPlan sprite_passes = {0};
    assert(terrain_build_sprite_pass_plan(0, 1, 1, &sprite_passes));
    assert(sprite_passes.count == 2 &&
           sprite_passes.passes[0].clip == TERRAIN_SPRITE_CLIP_SECOND &&
           sprite_passes.passes[0].dither_mode == 1 &&
           sprite_passes.passes[1].clip == TERRAIN_SPRITE_CLIP_FIRST &&
           sprite_passes.passes[1].dither_mode == 0 &&
           sprite_passes.even_mask == 0x00ff00ffu &&
           sprite_passes.odd_mask == 0xff00ff00u &&
           sprite_passes.even_fill == 0 && sprite_passes.odd_fill == 0);
    assert(terrain_build_sprite_pass_plan(0x20, 1, 0, &sprite_passes));
    assert(sprite_passes.count == 1 &&
           sprite_passes.passes[0].clip == TERRAIN_SPRITE_CLIP_FIRST &&
           sprite_passes.passes[0].dither_mode == 1 &&
           sprite_passes.even_fill == 0x56005600u &&
           sprite_passes.odd_fill == 0x00560056u);
    assert(terrain_build_sprite_pass_plan(0x83, 1, 1, &sprite_passes));
    assert(sprite_passes.count == 2 &&
           sprite_passes.even_fill == 0x34003400u &&
           sprite_passes.odd_fill == 0x34003400u);
    assert(terrain_build_sprite_pass_plan(0x10, 0, 0, &sprite_passes));
    assert(sprite_passes.count == 1 &&
           sprite_passes.passes[0].clip == TERRAIN_SPRITE_CLIP_DEFAULT &&
           sprite_passes.passes[0].dither_mode == 1 &&
           sprite_passes.even_fill == 0x28002800u &&
           sprite_passes.odd_fill == 0x00280028u);
    assert(terrain_build_sprite_pass_plan(0, 0, 0, &sprite_passes));
    assert(sprite_passes.count == 0);
    assert(!terrain_build_sprite_pass_plan(0, 0, 0, NULL));
    DatTerrain terrain_records[3] = {0};
    DatBorder border_records[3] = {0};
    TerrainArtEntry terrain_entries[3] = {0};
    BorderArtEntry border_entries[3] = {0};
    SlpFrame terrain_frames[9] = {0};
    SlpFrame border_frames[16] = {0};
    DatTerrains terrains = {.count = 3, .terrains = terrain_records};
    DatBorders borders = {.count = 3, .borders = border_records};
    TerrainArtCache terrain_art = {.count = 3, .entries = terrain_entries};
    BorderArtCache border_art = {.count = 3, .entries = border_entries};
    TerrainDrawPlan plan;

    terrain_records[0].enabled = 1;
    terrain_records[0].replacement_id = -1;
    terrain_records[0].frame_grid[0] = 3;
    terrain_records[0].frame_grid[1] = 3;
    terrain_records[0].elevation_frames[0] = (DatTerrainFrame){9, 1, 0};
    terrain_records[2].enabled = 1;
    terrain_records[2].replacement_id = 0;
    terrain_entries[0].frames = terrain_frames;
    terrain_entries[0].frame_count = 9;

    border_records[1].enabled = 1;
    border_records[1].border_style = 0;
    border_records[1].frames[0][0] = (DatTerrainFrame){1, 1, 7};
    border_entries[1].frames = border_frames;
    border_entries[1].frame_count = 16;
    assert(terrain_build_draw_plan(&terrains, &borders, &terrain_art,
                                   &border_art, 2, 0, 0x11, 2, 1, &plan));
    assert(plan.draw_terrain_id == 0 && plan.layer_count == 1);
    assert(plan.layers[0].kind == TERRAIN_DRAW_BORDER &&
           plan.layers[0].slot == 1 && plan.layers[0].frame == 7);

    border_records[1].draw_terrain = 1;
    assert(terrain_build_draw_plan(&terrains, &borders, &terrain_art,
                                   &border_art, 2, 0, 0x11, 2, 1, &plan));
    assert(plan.layer_count == 2);
    assert(plan.layers[0].kind == TERRAIN_DRAW_TERRAIN &&
           plan.layers[0].slot == 0 && plan.layers[0].frame == 5);
    assert(plan.layers[1].kind == TERRAIN_DRAW_BORDER &&
           plan.layers[1].frame == 7);

    border_records[2].enabled = 1;
    border_records[2].border_style = 1;
    for (unsigned i = 0; i < 4; ++i)
        border_records[2].frames[0][i] =
            (DatTerrainFrame){1, 1, (int16_t)(10 + i)};
    border_entries[2].frames = border_frames;
    border_entries[2].frame_count = 16;
    assert(terrain_build_draw_plan(&terrains, &borders, &terrain_art,
                                   &border_art, 2, 0, 0xf2, 2, 1, &plan));
    assert(plan.layer_count == 5);
    assert(plan.layers[0].kind == TERRAIN_DRAW_TERRAIN &&
           plan.layers[0].frame == 5);
    assert(plan.layers[1].frame == 10 && plan.layers[2].frame == 12 &&
           plan.layers[3].frame == 11 && plan.layers[4].frame == 13);
    for (unsigned i = 1; i < 5; ++i)
        assert(plan.layers[i].kind == TERRAIN_DRAW_BORDER &&
               plan.layers[i].slot == 2);

    border_entries[2].frames = 0;
    assert(terrain_build_draw_plan(&terrains, &borders, &terrain_art,
                                   &border_art, 2, 0, 0xf2, 2, 1, &plan));
    assert(plan.layer_count == 1 &&
           plan.layers[0].kind == TERRAIN_DRAW_TERRAIN);
    terrain_entries[0].frames = 0;
    assert(terrain_build_draw_plan(&terrains, &borders, &terrain_art,
                                   &border_art, 2, 0, 0x11, 2, 1, &plan));
    assert(plan.layer_count == 0);
    assert(!terrain_build_draw_plan(&terrains, &borders, &terrain_art,
                                    &border_art, 2, 17, 0, 2, 1, &plan));
    assert(!terrain_build_draw_plan(&terrains, &borders, &terrain_art,
                                    &border_art, 2, 0, 0, 2, 1, 0));
    {
        SlpPixel base_pixels[4] = {
            {1, SLP_COLOR}, {0, SLP_TRANSPARENT},
            {1, SLP_COLOR}, {1, SLP_COLOR}
        };
        SlpPixel edge_pixel = {2, SLP_COLOR};
        Palette palette = {0};
        uint8_t rgba[3 * 3 * 4];
        terrain_frames[0] = (SlpFrame){.width = 2, .height = 2,
                                      .hotspot_x = 1, .hotspot_y = 1,
                                      .pixels = base_pixels};
        border_frames[0] = (SlpFrame){.width = 1, .height = 1,
                                     .pixels = &edge_pixel};
        terrain_entries[0].frames = terrain_frames;
        border_entries[1].frames = border_frames;
        palette.colors[1] = (PaletteColor){10, 20, 30};
        palette.colors[2] = (PaletteColor){40, 50, 60};
        memset(&plan, 0, sizeof(plan));
        plan.layer_count = 2;
        plan.layers[0] = (TerrainDrawLayer){TERRAIN_DRAW_TERRAIN, 0, 0};
        plan.layers[1] = (TerrainDrawLayer){TERRAIN_DRAW_BORDER, 1, 0};
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_blit_draw_plan_rgba(&plan, &terrain_art, &border_art,
                                           &palette, rgba, sizeof(rgba),
                                           3, 3, 12, 1, 1));
        assert(rgba[0] == 10 && rgba[1] == 20 && rgba[2] == 30);
        assert(rgba[4] == 7); /* Transparent base pixel. */
        assert(rgba[16] == 40 && rgba[17] == 50 && rgba[18] == 60);
        uint8_t first_mask_bytes[9] = {0}, second_mask_bytes[9] = {0};
        TerrainClipMask first_mask = {first_mask_bytes, sizeof(first_mask_bytes), 3};
        TerrainClipMask second_mask = {second_mask_bytes, sizeof(second_mask_bytes), 3};
        base_pixels[1] = (SlpPixel){1, SLP_COLOR};
        palette.colors[0] = (PaletteColor){1, 2, 3};
        palette.colors[0x56] = (PaletteColor){70, 80, 90};
        plan.layer_count = 1;
        first_mask_bytes[1] = 255;
        second_mask_bytes[1] = 255;
        assert(terrain_build_sprite_pass_plan(0, 1, 1, &sprite_passes));
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_blit_draw_plan_passes_rgba(&plan, &terrain_art,
                                                  &border_art, &palette,
                                                  &sprite_passes, first_mask,
                                                  second_mask, rgba,
                                                  sizeof(rgba), 3, 3, 12,
                                                  1, 1, 0, 0));
        assert(rgba[4] == 10 && rgba[5] == 20 && rgba[6] == 30);
        {
            uint8_t indices[9], converted[sizeof(rgba)];
            /* The indexed path must preserve the exact pass order, clip,
               and screen-space dither phase of the existing RGBA path. */
            for (unsigned flags = 0; flags < 2; ++flags) {
                assert(terrain_build_sprite_pass_plan(flags ? 0x20 : 0,
                                                      1, 1, &sprite_passes));
                memset(indices, 0, sizeof(indices));
                assert(palette_expand_indexed_rgba(&palette, indices,
                       sizeof(indices), 3, 3, 3, rgba, sizeof(rgba), 12));
                assert(terrain_blit_draw_plan_passes_rgba(
                    &plan, &terrain_art, &border_art, &palette,
                    &sprite_passes, first_mask, second_mask, rgba,
                    sizeof(rgba), 3, 3, 12, 1, 1, 0, 0));
                assert(terrain_blit_draw_plan_passes_indexed(
                    &plan, &terrain_art, &border_art, &sprite_passes,
                    first_mask, second_mask, indices, sizeof(indices),
                    3, 3, 3, 1, 1, 0, 0));
                assert(palette_expand_indexed_rgba(&palette, indices,
                       sizeof(indices), 3, 3, 3, converted,
                       sizeof(converted), 12));
                assert(memcmp(converted, rgba, sizeof(rgba)) == 0);
            }
            first_mask.size = 7;
            memset(indices, 0x5a, sizeof(indices));
            assert(!terrain_blit_draw_plan_passes_indexed(
                &plan, &terrain_art, &border_art, &sprite_passes,
                first_mask, second_mask, indices, sizeof(indices),
                3, 3, 3, 1, 1, 0, 0));
            for (size_t i = 0; i < sizeof(indices); ++i)
                assert(indices[i] == 0x5a);
            first_mask.size = sizeof(first_mask_bytes);
        }
        assert(terrain_build_sprite_pass_plan(0, 1, 1, &sprite_passes));
        first_mask_bytes[1] = 0;
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_blit_draw_plan_passes_rgba(&plan, &terrain_art,
                                                  &border_art, &palette,
                                                  &sprite_passes, first_mask,
                                                  second_mask, rgba,
                                                  sizeof(rgba), 3, 3, 12,
                                                  1, 1, 0, 0));
        assert(rgba[4] == 1 && rgba[5] == 2 && rgba[6] == 3);
        first_mask_bytes[1] = 255;
        assert(terrain_build_sprite_pass_plan(0x20, 1, 0, &sprite_passes));
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_blit_draw_plan_passes_rgba(&plan, &terrain_art,
                                                  &border_art, &palette,
                                                  &sprite_passes, first_mask,
                                                  second_mask, rgba,
                                                  sizeof(rgba), 3, 3, 12,
                                                  1, 1, 0, 0));
        assert(rgba[4] == 70 && rgba[5] == 80 && rgba[6] == 90);
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_blit_draw_plan_passes_rgba(&plan, &terrain_art,
                                                  &border_art, &palette,
                                                  &sprite_passes, first_mask,
                                                  second_mask, rgba,
                                                  sizeof(rgba), 3, 3, 12,
                                                  1, 1, 1, 0));
        assert(rgba[4] == 10 && rgba[5] == 20 && rgba[6] == 30);
        first_mask.size = 7;
        memset(rgba, 7, sizeof(rgba));
        assert(!terrain_blit_draw_plan_passes_rgba(&plan, &terrain_art,
                                                   &border_art, &palette,
                                                   &sprite_passes, first_mask,
                                                   second_mask, rgba,
                                                   sizeof(rgba), 3, 3, 12,
                                                   1, 1, 0, 0));
        for (size_t i = 0; i < sizeof(rgba); ++i) assert(rgba[i] == 7);
        base_pixels[1] = (SlpPixel){0, SLP_TRANSPARENT};
        plan.layer_count = 2;
        edge_pixel.kind = SLP_SHADOW;
        memset(rgba, 7, sizeof(rgba));
        assert(!terrain_blit_draw_plan_rgba(&plan, &terrain_art, &border_art,
                                            &palette, rgba, sizeof(rgba),
                                            3, 3, 12, 1, 1));
        for (size_t i = 0; i < sizeof(rgba); ++i) assert(rgba[i] == 7);
        edge_pixel.kind = SLP_COLOR;
        plan.layers[1].frame = 16;
        assert(!terrain_blit_draw_plan_rgba(&plan, &terrain_art, &border_art,
                                            &palette, rgba, sizeof(rgba),
                                            3, 3, 12, 1, 1));
    }
    {
        ScnTile tiles[9] = {0};
        DatMapGeometry geometry = {0};
        DatTerrain local_terrain = {0};
        DatBorder local_border = {0};
        TerrainArtEntry local_art = {0}, local_border_art = {0};
        SlpFrame frames[9] = {0};
        SlpPixel pixel = {1, SLP_COLOR};
        Palette palette = {0};
        uint8_t rgba[128 * 96 * 4];
        uint8_t indices[128 * 96];
        TerrainViewCamera camera = {0};
        TerrainViewSurface surface = {rgba, sizeof(rgba), 128, 96, 128 * 4,
                                      10, 5, 118, 91};
        TerrainViewIndexedSurface indexed_surface = {
            indices, sizeof(indices), 128, 96, 128, 10, 5, 118, 91
        };
        DatTerrains local_terrains = {0};
        DatBorders local_borders = {0};
        TerrainArtCache local_cache = {0}, local_border_cache = {0};
        size_t drawn = 0;
        geometry.tile_width = 64;
        geometry.tile_height = 32;
        geometry.tile_half_width = 32;
        geometry.tile_half_height = 16;
        geometry.elevation_height = 16;
        local_terrain.enabled = 1;
        local_terrain.replacement_id = -1;
        local_terrain.frame_grid[0] = 3;
        local_terrain.frame_grid[1] = 3;
        local_terrain.elevation_frames[0] = (DatTerrainFrame){9, 1, 0};
        for (unsigned i = 0; i < 9; ++i)
            frames[i] = (SlpFrame){.width = 1, .height = 1, .pixels = &pixel};
        local_art.frames = frames;
        local_art.frame_count = 9;
        local_terrains.count = 1;
        local_terrains.terrains = &local_terrain;
        local_terrains.tile_sizes[0].height = 32;
        local_borders.count = 1;
        local_borders.borders = &local_border;
        local_cache.count = 1;
        local_cache.entries = &local_art;
        local_border_cache.count = 1;
        local_border_cache.entries = &local_border_art;
        palette.colors[1] = (PaletteColor){10, 20, 30};
        assert(terrain_compute_view_camera(tiles, 3, 3, &geometry,
                                           1.0f, 1.0f, 10, 5, 118, 91,
                                           &camera));
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_draw_view_terrain_rgba(tiles, 3, 3, &geometry,
                                              &local_terrains, &local_borders,
                                              &local_cache, &local_border_cache,
                                              &palette, &camera, &surface,
                                              &drawn));
        assert(drawn > 0);
        assert(rgba[((size_t)18 * 128 + 37) * 4] == 10);
        assert(rgba[0] == 7);
        uint8_t edge_bytes[448] = {0};
        TileEdgeIndex edge_index = {0};
        for (unsigned shape = 0; shape < TILEEDGE_SHAPES; ++shape)
            put32(edge_bytes + shape * 4, 68);
        put32(edge_bytes + 68, 444);
        put32(edge_bytes + 72, 444);
        edge_bytes[447] = 0xff; /* One pixel at the tile's anchor. */
        assert(tileedge_parse(edge_bytes, sizeof(edge_bytes), &edge_index));
        uint32_t visibility_masks[9];
        uint8_t tile_flags[9] = {0};
        uint8_t first_clip[118 * 91], second_clip[118 * 91];
        TerrainViewVisibility runtime = {
            .tile_masks = visibility_masks,
            .tile_masks_count = 9,
            .tile_flags = tile_flags,
            .tile_flags_count = 9,
            .visible_mask = 1,
            .explored_mask = 2,
            .tile_edges = &edge_index,
            .first = {first_clip, sizeof(first_clip), 118},
            .second = {second_clip, sizeof(second_clip), 118}
        };
        for (unsigned i = 0; i < 9; ++i) visibility_masks[i] = 1;
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_draw_view_visibility_rgba(
            tiles, 3, 3, &geometry, &local_terrains, &local_borders,
            &local_cache, &local_border_cache, &palette, &camera, &surface,
            &runtime, &drawn));
        assert(drawn > 0 && rgba[((size_t)18 * 128 + 37) * 4] == 10);
        size_t indexed_drawn = 0;
        memset(indices, 0xaa, sizeof(indices));
        assert(terrain_draw_view_visibility_indexed(
            tiles, 3, 3, &geometry, &local_terrains, &local_borders,
            &local_cache, &local_border_cache, &camera, &indexed_surface,
            &runtime, &indexed_drawn));
        assert(indexed_drawn == drawn);
        assert(indices[(size_t)18 * 128 + 37] == 1 && indices[0] == 0xaa);
        {
            uint8_t baseline[sizeof(indices)];
            TileDrawTrace trace = {0};
            size_t object_tiles = 0;
            memcpy(baseline, indices, sizeof(indices));
            memset(indices, 0xaa, sizeof(indices));
            assert(terrain_draw_view_visibility_indexed_with_tiles(
                tiles, 3, 3, &geometry, &local_terrains, &local_borders,
                &local_cache, &local_border_cache, &camera, &indexed_surface,
                &runtime, trace_tile_draw, &trace, &indexed_drawn,
                &object_tiles));
            assert(indexed_drawn == drawn && object_tiles == trace.calls &&
                   object_tiles > 0 && !trace.bad_mode);
            assert(memcmp(indices, baseline, sizeof(indices)) == 0);

            trace = (TileDrawTrace){.fail_on_call = 1};
            object_tiles = 99;
            assert(!terrain_draw_view_visibility_indexed_with_tiles(
                tiles, 3, 3, &geometry, &local_terrains, &local_borders,
                &local_cache, &local_border_cache, &camera, &indexed_surface,
                &runtime, trace_tile_draw, &trace, &indexed_drawn,
                &object_tiles));
            assert(trace.calls == 1 && object_tiles == 99);
        }
        for (unsigned i = 0; i < 9; ++i) visibility_masks[i] = 0;
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_draw_view_visibility_rgba(
            tiles, 3, 3, &geometry, &local_terrains, &local_borders,
            &local_cache, &local_border_cache, &palette, &camera, &surface,
            &runtime, &drawn));
        assert(drawn == 0 && rgba[((size_t)18 * 128 + 37) * 4] == 7);
        memset(indices, 0xaa, sizeof(indices));
        assert(terrain_draw_view_visibility_indexed(
            tiles, 3, 3, &geometry, &local_terrains, &local_borders,
            &local_cache, &local_border_cache, &camera, &indexed_surface,
            &runtime, &indexed_drawn));
        assert(indexed_drawn == 0);
        for (size_t i = 0; i < sizeof(indices); ++i)
            assert(indices[i] == 0xaa);
        {
            TileDrawTrace trace = {0};
            size_t object_tiles = 99;
            assert(terrain_draw_view_visibility_indexed_with_tiles(
                tiles, 3, 3, &geometry, &local_terrains, &local_borders,
                &local_cache, &local_border_cache, &camera, &indexed_surface,
                &runtime, trace_tile_draw, &trace, &indexed_drawn,
                &object_tiles));
            assert(indexed_drawn == 0 && object_tiles == 0 &&
                   trace.calls == 0);
        }
        for (unsigned i = 0; i < 9; ++i) visibility_masks[i] = 2;
        runtime.fog_enabled = 1;
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_draw_view_visibility_rgba(
            tiles, 3, 3, &geometry, &local_terrains, &local_borders,
            &local_cache, &local_border_cache, &palette, &camera, &surface,
            &runtime, &drawn));
        assert(drawn > 0 && rgba[((size_t)18 * 128 + 37) * 4 + 3] == 255);
        memset(indices, 0xaa, sizeof(indices));
        assert(terrain_draw_view_visibility_indexed(
            tiles, 3, 3, &geometry, &local_terrains, &local_borders,
            &local_cache, &local_border_cache, &camera, &indexed_surface,
            &runtime, &indexed_drawn));
        assert(indexed_drawn == drawn);
        for (size_t i = 0; i < sizeof(indices); ++i) {
            if (rgba[i * 4 + 3] != 255) continue;
            PaletteColor color = palette.colors[indices[i]];
            assert(rgba[i * 4] == color.r &&
                   rgba[i * 4 + 1] == color.g &&
                   rgba[i * 4 + 2] == color.b);
        }
        runtime.tile_masks_count = 8;
        memset(rgba, 7, sizeof(rgba));
        assert(!terrain_draw_view_visibility_rgba(
            tiles, 3, 3, &geometry, &local_terrains, &local_borders,
            &local_cache, &local_border_cache, &palette, &camera, &surface,
            &runtime, &drawn));
        assert(rgba[((size_t)18 * 128 + 37) * 4] == 7);
        local_terrains.tile_sizes[0].height = -1000;
        memset(rgba, 7, sizeof(rgba));
        assert(terrain_draw_view_terrain_rgba(tiles, 3, 3, &geometry,
                                              &local_terrains, &local_borders,
                                              &local_cache, &local_border_cache,
                                              &palette, &camera, &surface,
                                              &drawn));
        assert(drawn == 0 && rgba[((size_t)18 * 128 + 37) * 4] == 7);
        local_terrains.tile_sizes[0].height = 32;
        surface.clip_width = 119;
        assert(!terrain_draw_view_terrain_rgba(tiles, 3, 3, &geometry,
                                               &local_terrains, &local_borders,
                                               &local_cache, &local_border_cache,
                                               &palette, &camera, &surface,
                                               &drawn));
        surface.clip_left = 11;
        surface.clip_width = 117;
        assert(!terrain_draw_view_terrain_rgba(tiles, 3, 3, &geometry,
                                               &local_terrains, &local_borders,
                                               &local_cache, &local_border_cache,
                                               &palette, &camera, &surface,
                                               &drawn));
    }
    return 0;
}
