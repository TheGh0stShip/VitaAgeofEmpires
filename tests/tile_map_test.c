#include "tile_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_stream.h"

static int painted, refreshed, rects, begins, ends;
static int32_t last_terrain, last_x1;

static int t_resize(void *c, AoeTileMapRor *m, int32_t w, int32_t h) {
    (void)c;
    free(m->tiles);
    m->tiles = calloc((size_t)(w * h), sizeof *m->tiles);
    m->width = w;
    m->height = h;
    return m->tiles != NULL;
}
static void t_refresh(void *c, AoeTileMapRor *m, int32_t a, int32_t b, int32_t x1, int32_t y1) {
    (void)c; (void)m; (void)a; (void)b; (void)x1; (void)y1; ++refreshed;
}
static void t_rect(void *c, AoeTileMapRor *m, void *a, int32_t b, int32_t x0, int32_t y0, int32_t x1,
                   int32_t y1, int32_t terrain, int32_t cc, int32_t d) {
    (void)c; (void)m; (void)a; (void)b; (void)x0; (void)y0; (void)y1; (void)cc; (void)d;
    ++rects; last_terrain = terrain; last_x1 = x1;
}
static void t_cell(void *c, int32_t x, int32_t y, int32_t terrain) { (void)c; (void)x; (void)y; (void)terrain; ++painted; }
static void t_begin(void *c, int32_t w) { (void)c; (void)w; ++begins; }
static void t_end(void *c, int32_t w) { (void)c; (void)w; ++ends; }
static int16_t t_border(void *c, uint8_t a, uint8_t b) { (void)c; return (int16_t)(a == 1 && b == 2 ? 3 : -1); }
static int t_used(void *c, int16_t b) { (void)c; return b == 3; }

static uint8_t E(AoeTileMapRor *m, int x, int y) { return aoe_tile_elevation_ror(&m->tiles[y * m->width + x]); }
static void setE(AoeTileMapRor *m, int x, int y, int e) {
    AoeMapTileRor *t = &m->tiles[y * m->width + x];
    t->terrain_elevation = (uint8_t)((t->terrain_elevation & 0x1f) | (e << 5));
}

int main(void) {
    AoeTileMapOpsRor ops = {NULL, t_resize, t_refresh, t_rect, t_cell, t_begin, t_end, t_border, t_used};
    AoeTileMapRor m;
    uint8_t buf[64], cell;
    AoeOutcomeStream s = {buf, sizeof buf, 0, 0};
    int i;
    memset(&m, 0, sizeof m);

    aoe_tilemap_default_shapes_ror(&m);
    assert(m.shapes[0].width == 64 && m.shapes[0].height == 32 && m.shapes[0].offset == 0);
    assert(m.shapes[1].height == 16 && m.shapes[1].offset == -8 && m.shapes[2].height == 48);
    assert(m.shapes[2].offset == 8 && m.shapes[15].height == 32 && m.shapes[15].offset == 8);
    assert(m.shapes[17].width == 32 && m.shapes[18].offset == 0 && m.half_h == 16);

    assert(aoe_tilemap_new_filled_ror(&m, NULL, 1, 4, 5, 5, &ops));
    assert(begins == 1 && ends == 1 && last_terrain == 4 && last_x1 == 4);
    assert(aoe_tilemap_tiles_ror(&m) == m.tiles);

    /* Invalidate: swapped corners, clamped. */
    aoe_tilemap_invalidate_ror(&m, 7, 1, 3, -2, 5);
    assert(m.tiles[1 * 5 + 4].dirty == 0xcc && m.tiles[0 * 5 + 3].draw_flag == 5);
    assert(m.tiles[2 * 5 + 4].dirty == 0 && m.tiles[0].dirty == 0);
    aoe_tilemap_invalidate_ror(&m, 6, 6, 9, 9, 1); /* outside */

    /* Positions with elevation and a raised shape. */
    setE(&m, 1, 2, 2);
    m.tiles[2 * 5 + 1].shape = 2;
    aoe_tilemap_positions_ror(&m, -1, -1, 9, 9);
    assert(m.tiles[2 * 5 + 1].screen_x == 3 * 32);
    assert(m.tiles[2 * 5 + 1].screen_y == (2 - 1) * 16 - 2 * 16 - 16);
    assert(m.tiles[4].screen_y == -4 * 16);

    /* Step down towards 0: only the tile itself next to a two-step drop moves. */
    setE(&m, 1, 2, 0);
    setE(&m, 2, 2, 3);
    assert(aoe_tilemap_step_ror(&m, 2, 2, 2, 2, 0));
    assert(E(&m, 2, 2) == 2);
    assert(!aoe_tilemap_step_ror(&m, 0, 0, 0, 0, 0));
    /* Raising: a tile below the target beside a much higher tile. */
    setE(&m, 2, 2, 3);
    assert(aoe_tilemap_step_ror(&m, 1, 1, 1, 1, 5));
    assert(E(&m, 1, 1) == 1 && E(&m, 1, 2) == 1);

    /* Smooth: a pit between two higher tiles is filled up to the limit,
       above it the neighbours come down. */
    for (i = 0; i < 25; ++i) m.tiles[i].terrain_elevation = 0;
    setE(&m, 1, 2, 2);
    setE(&m, 3, 2, 2);
    aoe_tilemap_smooth_ror(&m, 2, 2, 2, 2, 7);
    assert(E(&m, 2, 2) == 2);
    for (i = 0; i < 25; ++i) m.tiles[i].terrain_elevation = 0;
    setE(&m, 1, 2, 2);
    setE(&m, 3, 2, 2);
    aoe_tilemap_smooth_ror(&m, 2, 2, 2, 2, 0);
    assert(E(&m, 1, 2) == 0 && E(&m, 3, 2) == 0 && E(&m, 2, 2) == 0);

    assert(aoe_tilemap_paint_square_ror(&m, 0, 0, 1, 3, &ops) && painted == 4);
    assert(aoe_tilemap_raise_square_ror(&m, 4, 4, 2, 5) && E(&m, 2, 2) == 5 && E(&m, 1, 4) == 0);

    /* Old map data: 2x2, terrain / elevation / ignored per tile. */
    aoe_stream_put_i32(&s, 2);
    aoe_stream_put_i32(&s, 2);
    for (i = 0; i < 4; ++i) {
        aoe_stream_put_u8(&s, (uint8_t)(i + 1));
        aoe_stream_put_u8(&s, (uint8_t)i);
        aoe_stream_put_u8(&s, 9);
    }
    s.size = s.pos;
    s.pos = 0;
    rects = 0;
    assert(aoe_tilemap_load_old_ror(&m, &s, &ops));
    assert(m.width == 2 && aoe_tile_terrain_ror(&m.tiles[3]) == 4 && E(&m, 1, 1) == 3);
    assert(refreshed == 1 && rects == 1 && last_terrain == 0);

    cell = 2;
    aoe_tilemap_pick_terrain_ror(1, &cell, &ops);
    assert(cell == 2);
    aoe_tilemap_pick_terrain_ror(5, &cell, &ops);
    assert(cell == 5);
    aoe_tilemap_nothing_ror(&m);
    free(m.tiles);
    return 0;
}
