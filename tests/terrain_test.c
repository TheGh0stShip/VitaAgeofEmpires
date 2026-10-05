#include "terrain.h"

#include <assert.h>
#include <math.h>
#include <string.h>

enum { NW, N, NE, W, C, E, SW, S, SE };

typedef struct {
    uint32_t x[16], y[16];
    size_t count;
} TileVisits;

static void record_tile(uint32_t x, uint32_t y, void *context) {
    TileVisits *visits = context;
    assert(visits->count < 16);
    visits->x[visits->count] = x;
    visits->y[visits->count] = y;
    ++visits->count;
}

static void check(int a, int b, int av, int bv, uint8_t expected) {
    ScnTile tiles[9];
    memset(tiles, 0, sizeof(tiles));
    for (int i = 0; i < 9; ++i) tiles[i].elevation = 1;
    if (a >= 0) tiles[a].elevation = (uint8_t)av;
    if (b >= 0) tiles[b].elevation = (uint8_t)bv;
    assert(terrain_elevation_shape(tiles, 3, 3, 1, 1) == expected);
}

/* Center elevation 1 with `raised` at 2 and `low_a`/`low_b` at 0. */
static void check3(int raised, int low_a, int low_b, uint8_t expected) {
    ScnTile tiles[9];
    memset(tiles, 0, sizeof(tiles));
    for (int i = 0; i < 9; ++i) tiles[i].elevation = 1;
    tiles[raised].elevation = 2;
    if (low_a >= 0) tiles[low_a].elevation = 0;
    if (low_b >= 0) tiles[low_b].elevation = 0;
    assert(terrain_elevation_shape(tiles, 3, 3, 1, 1) == expected);
}

static void check_border(int style, int a, int b, uint8_t expected) {
    ScnTile tiles[9] = {0};
    DatTerrain terrain[2] = {0};
    DatBorder border[2] = {0};
    DatTerrains terrains = {0};
    DatBorders borders = {0};
    for (unsigned i = 0; i < 32; ++i) terrain[0].borders[i] = -1;
    terrain[0].enabled = 1;
    terrain[0].borders[1] = 1;
    border[1].enabled = 1;
    border[1].border_style = (int16_t)style;
    terrains.count = 2;
    terrains.terrains = terrain;
    borders.count = 2;
    borders.borders = border;
    if (a >= 0) tiles[a].terrain_id = 1;
    if (b >= 0) tiles[b].terrain_id = 1;
    assert(terrain_border_code(tiles, 3, 3, 1, 1,
                               &terrains, &borders) == expected);
    if (a >= 0) {
        border[1].enabled = 0;
        assert(terrain_border_code(tiles, 3, 3, 1, 1,
                                   &terrains, &borders) == 0);
    }
}

int main(void) {
    {
        DatTerrain entries[3] = {0};
        DatTerrains terrains = {0};
        terrains.count = 3;
        terrains.terrains = entries;
        entries[0].enabled = 1;
        entries[1].replacement_id = 0;
        entries[2].replacement_id = -1;
        assert(terrain_paint_target_available(&terrains, 0));
        assert(terrain_paint_target_available(&terrains, 1));
        assert(!terrain_paint_target_available(&terrains, 2));
        entries[1].replacement_id = 2;
        assert(!terrain_paint_target_available(&terrains, 1));
        entries[1].replacement_id = 3;
        assert(!terrain_paint_target_available(&terrains, 1));
        entries[1].replacement_id = 0;
        entries[0].enabled = 2;
        assert(terrain_paint_target_available(&terrains, 0));
        assert(!terrain_paint_target_available(&terrains, 1));
        assert(!terrain_paint_target_available(&terrains, 3));
        assert(!terrain_paint_target_available(NULL, 0));
    }
    {
        DatTerrain entries[3] = {0};
        DatTerrains terrains = {0};
        uint8_t draw_id = 255;
        terrains.count = 3;
        terrains.terrains = entries;
        entries[0].replacement_id = -1;
        entries[1].replacement_id = 2;
        entries[2].replacement_id = 0;
        assert(terrain_resolve_draw_id(&terrains, 0, &draw_id));
        assert(draw_id == 0);
        assert(terrain_resolve_draw_id(&terrains, 1, &draw_id));
        assert(draw_id == 2); /* Do not follow slot 2's replacement. */
        entries[1].replacement_id = 3;
        assert(!terrain_resolve_draw_id(&terrains, 1, &draw_id));
        assert(!terrain_resolve_draw_id(&terrains, 3, &draw_id));
        assert(!terrain_resolve_draw_id(&terrains, 0, NULL));
        assert(!terrain_resolve_draw_id(NULL, 0, &draw_id));
    }
    {
        DatTerrain terrain = {0};
        uint16_t first = 65535, count = 65535;
        terrain.elevation_frames[0].frame_start = 0;
        terrain.elevation_frames[0].frame_count = 9;
        terrain.elevation_frames[0].angle_count = 1;
        terrain.elevation_frames[1].frame_start = 9;
        terrain.elevation_frames[1].frame_count = 1;
        terrain.elevation_frames[1].angle_count = 1;
        assert(terrain_shape_frame_span(&terrain, 0, &first, &count));
        assert(first == 0 && count == 9);
        assert(terrain_shape_frame_span(&terrain, 1, &first, &count));
        assert(first == 9 && count == 1);
        assert(!terrain_shape_frame_span(&terrain, 2, &first, &count));
        assert(!terrain_shape_frame_span(&terrain, 17, &first, &count));
        terrain.elevation_frames[1].frame_start = -1;
        assert(!terrain_shape_frame_span(&terrain, 1, &first, &count));
        assert(!terrain_shape_frame_span(NULL, 0, &first, &count));
        assert(!terrain_shape_frame_span(&terrain, 0, NULL, &count));
    }
    {
        DatTerrain terrain = {0};
        uint32_t frame = UINT32_MAX;
        terrain.elevation_frames[0].frame_count = 9;
        terrain.elevation_frames[0].angle_count = 1;
        terrain.elevation_frames[0].frame_start = 0;
        terrain.elevation_frames[1].frame_count = 1;
        terrain.elevation_frames[1].angle_count = 1;
        terrain.elevation_frames[1].frame_start = 9;
        terrain.frame_grid[0] = 3;
        terrain.frame_grid[1] = 3;
        for (uint32_t y = 0; y < 9; ++y)
            for (uint32_t x = 0; x < 9; ++x) {
                assert(terrain_select_frame(&terrain, 0, x, y, &frame));
                assert(frame == (y % 3) * 3 + x % 3);
                assert(terrain_select_frame(&terrain, 1, x, y, &frame));
                assert(frame == 9);
            }
        terrain.elevation_frames[0].angle_count = 2;
        terrain.elevation_frames[0].frame_start = 5;
        assert(terrain_select_frame(&terrain, 0, 2, 1, &frame));
        assert(frame == 15); /* 5 + (1*3+2)*2 */
        terrain.frame_grid[0] = 4;
        terrain.frame_grid[1] = 4;
        assert(terrain_select_frame(&terrain, 0, 3, 3, &frame));
        assert(frame == 5); /* Variant 15 exceeds nine-frame span. */
        terrain.frame_grid[0] = 0;
        terrain.frame_grid[1] = 0;
        assert(terrain_select_frame(&terrain, 0, 3, 3, &frame));
        assert(frame == 5);
        assert(!terrain_select_frame(&terrain, 17, 0, 0, &frame));
        assert(!terrain_select_frame(&terrain, 0, 32768, 0, &frame));
        assert(!terrain_select_frame(NULL, 0, 0, 0, &frame));
        assert(!terrain_select_frame(&terrain, 0, 0, 0, NULL));
        terrain.elevation_frames[0].frame_count = 0;
        assert(!terrain_select_frame(&terrain, 0, 0, 0, &frame));
    }
    {
        DatBorder border = {0};
        uint32_t frame = UINT32_MAX;
        border.frames[0][8] = (DatTerrainFrame){3, 1, 40}; /* Pattern 9. */
        border.frames[0][10] = (DatTerrainFrame){4, 1, 50}; /* Pattern 11. */
        border.frames[1][0] = (DatTerrainFrame){1, 1, 60}; /* Pattern 13. */
        assert(terrain_select_border_frame(&border, 0, 9, 5, 7, &frame));
        assert(frame == 42); /* Style 0, pattern 9 varies by x. */
        assert(terrain_select_border_frame(&border, 0, 11, 5, 7, &frame));
        assert(frame == 53); /* Style 0, pattern 11 varies by y. */
        assert(terrain_select_border_frame(&border, 0, 13, 5, 7, &frame));
        assert(frame == 60); /* Flattened table crosses a row. */
        border.border_style = 1;
        border.frames[0][0] = (DatTerrainFrame){3, 2, 70};
        border.frames[0][1] = (DatTerrainFrame){4, 1, 80};
        assert(terrain_select_border_frame(&border, 0, 1, 5, 7, &frame));
        assert(frame == 74); /* Style 1, pattern 1 varies by x. */
        assert(terrain_select_border_frame(&border, 0, 2, 5, 7, &frame));
        assert(frame == 83); /* Style 1, pattern 2 varies by y. */
        assert(!terrain_select_border_frame(&border, 0, 0, 5, 7, &frame));
        assert(!terrain_select_border_frame(&border, 17, 1, 5, 7, &frame));
        assert(!terrain_select_border_frame(&border, 0, 16, 5, 7, &frame));
        assert(!terrain_select_border_frame(&border, 0, 1, 32768, 7, &frame));
        assert(!terrain_select_border_frame(NULL, 0, 1, 5, 7, &frame));
        border.frames[0][0].frame_count = 0;
        assert(!terrain_select_border_frame(&border, 0, 1, 5, 7, &frame));
    }
    {
        DatTerrain terrain = {0};
        const uint8_t expected[17] = {
            22, 33, 11, 11, 33, 33, 11, 33, 33,
            33, 11, 11, 33, 33, 11, 33, 11
        };
        terrain.map_colors[0] = 11;
        terrain.map_colors[1] = 22;
        terrain.map_colors[2] = 33;
        for (uint8_t shape = 0; shape < 17; ++shape)
            assert(terrain_minimap_palette_index(&terrain, shape) ==
                   expected[shape]);
        assert(terrain_minimap_palette_index(&terrain, 17) == 0);
        assert(terrain_minimap_palette_index(NULL, 0) == 0);
    }
    check(-1, -1, 0, 0, 0);
    check(N, E, 2, 2, 14);
    check(W, S, 2, 2, 13);
    check(N, W, 2, 2, 16);
    check(E, S, 2, 2, 15);
    check(N, -1, 2, 0, 6);
    check(E, -1, 2, 0, 8);
    check(W, -1, 2, 0, 5);
    check(S, -1, 2, 0, 7);
    /* Raised diagonals, as RoR 0x004493a0 / patched 0x004484f0 return
       them (a lone raised corner gives 10/9/11/12; with the two far-side
       cardinals lowered, 2/1/3/4). */
    check(NE, -1, 2, 0, 10);
    check(SW, -1, 2, 0, 9);
    check(NW, -1, 2, 0, 11);
    check(SE, -1, 2, 0, 12);
    check3(NE, W, S, 2);
    check3(SW, N, E, 1);
    check3(NW, E, S, 3);
    check3(SE, N, W, 4);
    /* One lowered far-side cardinal is not enough. */
    check3(SW, N, -1, 9);
    /* Under the old (wrong) rules this layout gave 3: the raised SW corner
       tests N and E, not E and S. */
    check3(SW, E, S, 9);
    ScnTile tiles[9] = {0};
    assert(terrain_elevation_shape(tiles, 3, 3, 3, 0) == 0);
    assert(terrain_elevation_shape(NULL, 3, 3, 1, 1) == 0);
    check_border(0, -1, -1, 0);
    check_border(0, N, W, 0x11);
    check_border(0, N, E, 0x21);
    check_border(0, S, W, 0x31);
    check_border(0, S, E, 0x41);
    check_border(0, NW, -1, 0x51);
    check_border(0, NE, -1, 0x61);
    check_border(0, SW, -1, 0x71);
    check_border(0, SE, -1, 0x81);
    check_border(0, N, -1, 0x91);
    check_border(0, S, -1, 0xa1);
    check_border(0, W, -1, 0xb1);
    check_border(0, E, -1, 0xc1);
    check_border(1, N, E, 0x91);
    check_border(1, N, S, 0x31);
    check_border(1, NW, -1, 0x01);
    {
        ScnTile projected_tiles[9] = {0};
        DatMapGeometry geometry = {0};
        int32_t screen_x = -1, screen_y = -1;
        geometry.tile_half_width = 32;
        geometry.tile_half_height = 16;
        geometry.elevation_height = 16;
        assert(terrain_project_tile(projected_tiles, 3, 3, 1, 1, &geometry,
                                    &screen_x, &screen_y));
        assert(screen_x == 64 && screen_y == 0);
        assert(terrain_project_tile_view(projected_tiles, 3, 3, 1, 1,
                                         &geometry, 40, -10,
                                         &screen_x, &screen_y));
        assert(screen_x == 24 && screen_y == 10);
        projected_tiles[C].elevation = 1;
        assert(terrain_project_tile(projected_tiles, 3, 3, 1, 1, &geometry,
                                    &screen_x, &screen_y));
        assert(screen_x == 64 && screen_y == -16);
        projected_tiles[N].elevation = 2;
        assert(terrain_elevation_shape(projected_tiles, 3, 3, 1, 1) == 6);
        assert(terrain_project_tile(projected_tiles, 3, 3, 1, 1, &geometry,
                                    &screen_x, &screen_y));
        assert(screen_x == 64 && screen_y == -32);
        assert(terrain_project_tile_view(projected_tiles, 3, 3, 1, 1,
                                         &geometry, 40, -10,
                                         &screen_x, &screen_y));
        assert(screen_x == 24 && screen_y == -22);
        assert(!terrain_project_tile_view(projected_tiles, 3, 3, 1, 1,
                                          &geometry, INT32_MIN, 0,
                                          &screen_x, &screen_y));
        assert(terrain_project_tile(projected_tiles, 3, 3, 2, 0, &geometry,
                                    &screen_x, &screen_y));
        assert(screen_x == 64 && screen_y == -32);
        assert(!terrain_project_tile(projected_tiles, 3, 3, 3, 0, &geometry,
                                     &screen_x, &screen_y));
    }
    {
        static const uint32_t expected_x[] = {2, 3, 1, 2, 3, 1, 2, 0, 1};
        static const uint32_t expected_y[] = {1, 2, 1, 2, 3, 2, 3, 2, 3};
        TileVisits visits = {0};
        size_t count = 99;
        assert(terrain_walk_view_tiles(4, 4, 2, 1, 4, 3,
                                       record_tile, &visits, &count));
        assert(count == 9 && visits.count == 9);
        for (size_t i = 0; i < count; ++i) {
            assert(visits.x[i] == expected_x[i]);
            assert(visits.y[i] == expected_y[i]);
        }
        visits.count = 0;
        assert(terrain_walk_view_tiles(2, 2, -1, -1, 1, 3,
                                       record_tile, &visits, &count));
        assert(count == 2 && visits.count == 2);
        assert(visits.x[0] == 0 && visits.y[0] == 0);
        assert(visits.x[1] == 1 && visits.y[1] == 1);
        assert(!terrain_walk_view_tiles(0, 4, 0, 0, 2, 3,
                                        record_tile, &visits, &count));
    }
    {
        ScnTile tiles[9] = {0};
        DatMapGeometry geometry = {0};
        TerrainViewCamera camera = {0};
        int32_t anchor_x, anchor_y;
        float clamped_x, clamped_y;
        geometry.tile_width = 64;
        geometry.tile_height = 32;
        geometry.tile_half_width = 32;
        geometry.tile_half_height = 16;
        geometry.elevation_height = 16;
        assert(terrain_clamp_camera_tiles(3, 3, -1.0f, 4.0f,
                                          &clamped_x, &clamped_y));
        assert(clamped_x == 0.0f && clamped_y == 2.0f);
        assert(terrain_clamp_camera_tiles(3, 3, NAN, INFINITY,
                                          &clamped_x, &clamped_y));
        assert(clamped_x == 0.0f && clamped_y == 2.0f);
        assert(!terrain_clamp_camera_tiles(0, 3, 0.0f, 0.0f,
                                           &clamped_x, &clamped_y));
        assert(terrain_compute_view_camera(tiles, 3, 3, &geometry,
                                           1.25f, 1.5f, 0, 0, 640, 480,
                                           &camera));
        assert(camera.camera_pixel_x == -200);
        assert(camera.camera_pixel_y == -204);
        assert(camera.start_x == 2 && camera.start_y == -14);
        assert(camera.diagonal_count == 42 && camera.diagonal_length == 15);
        assert(terrain_project_tile_view(tiles, 3, 3, 1, 1, &geometry,
                                         camera.camera_pixel_x,
                                         camera.camera_pixel_y,
                                         &anchor_x, &anchor_y));
        assert(anchor_x == 264 && anchor_y == 204);
        assert(terrain_compute_view_camera(tiles, 3, 3, &geometry,
                                           1.03125f, 1.0f, 0, 0, 640, 480,
                                           &camera));
        assert(camera.camera_pixel_x == -223);
        assert(camera.camera_pixel_y == -208); /* -0.5 truncates to zero. */
        tiles[C].elevation = 1;
        tiles[N].elevation = 2;
        assert(terrain_compute_view_camera(tiles, 3, 3, &geometry,
                                           1.25f, 1.5f, 0, 0, 640, 480,
                                           &camera));
        assert(camera.camera_pixel_y == -220);
        assert(!terrain_compute_view_camera(tiles, 3, 3, &geometry,
                                            3.0f, 1.0f, 0, 0, 640, 480,
                                            &camera));
    }
    assert(terrain_view_visibility_code(0x02, 0x02, 0x04, 0, 1) ==
           TERRAIN_VIEW_VISIBLE);
    assert(terrain_view_visibility_code(0x04, 0x02, 0x04, 0, 1) ==
           TERRAIN_VIEW_EXPLORED_FOG);
    assert(terrain_view_visibility_code(0x04, 0x02, 0x04, 0, 0) ==
           TERRAIN_VIEW_VISIBLE);
    assert(terrain_view_visibility_code(0, 0x02, 0x04, 0, 1) ==
           TERRAIN_VIEW_HIDDEN);
    assert(terrain_view_visibility_code(0, 0x02, 0x04, 1, 1) ==
           TERRAIN_VIEW_EXPLORED_FOG);
    assert(terrain_view_visibility_code(0, 0x02, 0x04, 1, 0) ==
           TERRAIN_VIEW_VISIBLE);
    {
        uint8_t table[256], pattern = 255;
        uint32_t tile_masks[9] = {0};
        assert(terrain_build_visibility_edge_table(table));
        assert(table[0] == 0 && table[0x80] == 34);
        assert(table[0xf0] == 46 && table[0xff] == 46);
        assert(terrain_visibility_edge_pattern(tile_masks, 3, 3, 1, 1,
                                               1, table, &pattern));
        assert(pattern == 46);
        for (unsigned i = 0; i < 9; ++i) tile_masks[i] = 1;
        assert(terrain_visibility_edge_pattern(tile_masks, 3, 3, 1, 1,
                                               1, table, &pattern));
        assert(pattern == 0);
        tile_masks[N] = 0;
        assert(terrain_visibility_edge_pattern(tile_masks, 3, 3, 1, 1,
                                               1, table, &pattern));
        assert(pattern == 34);
        assert(!terrain_visibility_edge_pattern(tile_masks, 3, 3, 3, 1,
                                                1, table, &pattern));
    }
    {
        static const float low_sum[17] = {
            0.0f, 0.5f, 0.25f, 0.5f, 0.25f, 0.75f, 0.5f, 0.5f,
            0.25f, 0.25f, 0.0f, 0.25f, 0.0f, 0.75f, 0.5f, 0.5f, 0.75f
        };
        static const float high_sum[17] = {
            0.0f, 0.25f, 0.5f, 0.25f, 0.5f, 0.25f, 0.5f, 0.5f,
            0.75f, 0.0f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f, 0.75f, 0.5f
        };
        ScnTile tiles[9] = {0};
        float offset = -1.0f, surface = -1.0f;
        for (unsigned shape = 0; shape < 17; ++shape) {
            assert(terrain_shape_surface_offset((uint8_t)shape,
                                                0.25f, 0.5f, &offset));
            assert(offset == low_sum[shape]);
            assert(terrain_shape_surface_offset((uint8_t)shape,
                                                0.75f, 0.5f, &offset));
            assert(offset == high_sum[shape]);
        }
        assert(!terrain_shape_surface_offset(17, 0.25f, 0.5f, &offset));
        assert(!terrain_shape_surface_offset(0, 1.0f, 0.5f, &offset));
        assert(!terrain_shape_surface_offset(0, 0.5f, 0.5f, NULL));
        assert(offset == 0.5f);
        for (unsigned i = 0; i < 9; ++i) tiles[i].elevation = 1;
        assert(terrain_surface_height_at(tiles, 3, 3, 1.25f, 1.5f,
                                         &surface));
        assert(surface == 1.0f);
        tiles[N].elevation = 2;
        assert(terrain_surface_height_at(tiles, 3, 3, 1.25f, 1.5f,
                                         &surface));
        assert(surface == 1.5f); /* Shape 6: 1 - fractional Y. */
        assert(terrain_surface_height_at(tiles, 3, 3, -1.0f, 9.0f,
                                         &surface));
        assert(surface >= 1.0f && surface <= 2.0f);
        assert(!terrain_surface_height_at(tiles, 0, 3, 0.0f, 0.0f,
                                          &surface));
        assert(!terrain_surface_height_at(tiles, 3, 3, NAN, 0.0f,
                                          &surface));
        {
            DatUnit unit = {.present = 1};
            float placed_z = -99.0f;
            assert(terrain_unit_surface_z(&unit, 5.0f, 1.5f, &placed_z));
            assert(placed_z == 1.5f);
            unit.elevation_mode = 1;
            assert(terrain_unit_surface_z(&unit, 0.0f, 1.5f, &placed_z));
            assert(placed_z == 1.5f);
            assert(terrain_unit_surface_z(&unit, 3.0f, 1.5f, &placed_z));
            assert(placed_z == 3.0f);
            unit.elevation_mode = 2;
            assert(terrain_unit_surface_z(&unit, 0.0f, 1.5f, &placed_z));
            assert(placed_z == 0.0f);
            unit.present = 0;
            assert(!terrain_unit_surface_z(&unit, 0.0f, 1.5f, &placed_z));
            assert(placed_z == 0.0f);
        }
    }
    return 0;
}
