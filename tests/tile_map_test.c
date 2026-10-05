#include "tile_map.h"

#include <assert.h>
#include <stdio.h>
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

static int32_t g_seed = -1, g_used, g_rand = 41, g_srands, g_flag = 7, g_setup, g_gen, g_fin, g_flag_during;
static char g_log[2][64];
static int g_logs;
static int32_t gs(void *c) { (void)c; return g_seed; }
static void gu(void *c, int32_t v) { (void)c; g_used = v; }
static int32_t gr(void *c) { (void)c; return g_rand++; }
static void gsr(void *c, uint32_t v) { (void)c; (void)v; ++g_srands; }
static void gl(void *c, const char *l) { (void)c; snprintf(g_log[g_logs++ & 1], 64, "%s", l); }
static uint8_t gf(void *c) { (void)c; return (uint8_t)g_flag; }
static void gsf(void *c, uint8_t v) { (void)c; g_flag = v; }
static void gsetup(void *c, AoeTileMapRor *m, int32_t w, int32_t a, int32_t b) { (void)c; (void)m; (void)w; (void)a; (void)b; ++g_setup; g_flag_during = g_flag; }
static void ggen(void *c) { (void)c; ++g_gen; }
static void gfin(void *c) { (void)c; ++g_fin; }

static int32_t map_sound(void *c, int border, int i) { (void)c; return border ? -1 : 40 + i; }
static void map_ctl(void *c, AoeOutcomeStream *s) { (void)c; aoe_stream_put_u8(s, 0xab); }

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
    aoe_tilemap_invalidate_around_ror(&m, 0.9f, 4.5f);
    assert(m.tiles[2 * 5 + 1].dirty == 0xcc && m.tiles[2 * 5 + 2].dirty == 0 && m.tiles[4 * 5].dirty == 0xcc);

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
    {
        AoeTileMapGenOpsRor gen = {NULL, gs, gu, gr, gsr, gl, gf, gsf, gsetup, ggen, gfin};
        assert(aoe_tilemap_generate_ror(&m, 0, -1, -1, 1, 2, &ops, &gen));
        assert(g_used == 41 && !strcmp(g_log[0], "Random Map Seed = 41"));
        assert(!strcmp(g_log[1], "After Random Map Generation random number is 42"));
        assert(g_srands == 3 && g_setup == 1 && g_gen == 1 && g_fin == 1 && g_flag_during == 3);
        assert(g_flag == 7 && m.width == 2);
        g_seed = 9;
        assert(aoe_tilemap_generate_ror(&m, 0, 4, 3, 1, 2, &ops, &gen) && g_used == 9);
        assert(m.width == 4 && m.height == 3);
    }
    {
        static uint8_t image[AOE_MAP_RECORD_SIZE], out[AOE_MAP_RECORD_SIZE + 8];
        AoeOutcomeStream o = {out, sizeof out, 0, 0};
        image[0x8c] = 1;                 /* terrain 0 enabled */
        image[0x8c + 0x198 + 0x24] = 9;  /* terrain 1 disabled: slot kept */
        image[0x338c + 0x5a0] = 1;       /* border 1 enabled */
        image[0x33b0] = 5;               /* border 0 disabled: -1 */
        image[0x8dcf] = 0x77;
        assert(aoe_tilemap_save_data_ror(image, map_sound, map_ctl, NULL, &o));
        assert(o.pos == AOE_MAP_RECORD_SIZE + 1 && out[AOE_MAP_RECORD_SIZE] == 0xab);
        assert(out[0xb0] == 40 && out[0xb1] == 0 && out[0xb0 + 0x198] == 9);
        assert(out[0x33b0] == 0xff && out[0x33b3] == 0xff && out[0x33b0 + 0x5a0] == 0xff);
        assert(out[0x8dcf] == 0x77 && image[0xb0] == 0);
    }
    free(m.tiles);
    return 0;
}
