#include "tile_map.h"

#include <assert.h>
#include <math.h>
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

static int32_t cl[5], cl_calls;
static void t_cleanup(void *c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t v) {
    (void)c; cl[0] = x0; cl[1] = y0; cl[2] = x1; cl[3] = y1; cl[4] = v; ++cl_calls;
}
static void test_lines(const AoeTileMapOpsRor *ops) {
    AoeTileMapRor m;
    memset(&m, 0, sizeof(m));
    assert(t_resize(NULL, &m, 8, 8));
    /* Horizontal 0..3 width 1 (radius 0): start, 3 steps, end equal: 4 cells. */
    painted = 0;
    assert(aoe_tilemap_paint_line_ror(&m, 0, 2, 3, 2, 1, 5, ops) == 1);
    assert(painted == 4);
    /* (0,0)->(3,4): 5 steps, then (3,4) once more (5 * 0.6 rounds off). */
    painted = 0;
    assert(aoe_tilemap_paint_line_ror(&m, 0, 0, 3, 4, 0, 5, ops) == 1);
    assert(painted == 6 || painted == 7);
    /* Same point: one brush only; end clamped to the map. */
    painted = 0;
    assert(aoe_tilemap_paint_line_ror(&m, 7, 7, 20, 7, 1, 5, ops) == 1);
    assert(painted == 1);
    /* Width 3 (radius 1) elevation line from (5,1) to (2,1) with the
       clean-up over the widened box. */
    assert(aoe_tilemap_raise_line_ror(&m, 5, 1, 2, 1, 3, 2, t_cleanup, NULL) == 1);
    assert(E(&m, 1, 0) == 2 && E(&m, 6, 2) == 2 && E(&m, 0, 1) == 0 && E(&m, 7, 1) == 0);
    assert(E(&m, 3, 3) == 0);
    assert(cl_calls == 1 && cl[0] == 1 && cl[1] == 0 && cl[2] == 6 && cl[3] == 2 && cl[4] == 2);
    free(m.tiles);
    painted = 0;
}


static int owned_log[64][2], owned_n;
static void owned_destroy(void *c, int kind, void *o) { (void)c; owned_log[owned_n][0] = kind; owned_log[owned_n++][1] = (int)(intptr_t)o; }
static void owned_free(void *c, void *p) { (void)c; owned_log[owned_n][0] = 100; owned_log[owned_n++][1] = (int)(intptr_t)p; }

static void test_map_release_owned(void) {
    AoeMapOwnedOpsRor ops = {NULL, owned_destroy, owned_free};
    AoeMapOwnedRor m;
    memset(&m, 0, sizeof m);
    m.zones = (void *)1; m.random_map = (void *)2; m.raw04 = (void *)3; m.rows8d8c = (void *)4;
    m.terrain_shapes[0] = (void *)10; m.terrain_shapes[31] = (void *)11;
    m.border_shapes[2] = (void *)20; m.border_shapes[9] = (void *)21;
    m.masks = (void *)30; m.v8dcc = (void *)31;
    aoe_map_release_owned_ror(&m, &ops);
    assert(owned_n == 10);
    assert(owned_log[0][0] == AOE_MAP_OWNED_ZONES && owned_log[1][0] == AOE_MAP_OWNED_RANDOM_MAP);
    assert(owned_log[2][0] == 100 && owned_log[2][1] == 3 && owned_log[3][1] == 4);
    assert(owned_log[4][1] == 11 && owned_log[5][1] == 10 && owned_log[6][1] == 21 && owned_log[7][1] == 20);
    assert(owned_log[8][0] == AOE_MAP_OWNED_MASKS && owned_log[9][0] == AOE_MAP_OWNED_8DCC);
    assert(!m.zones && !m.random_map && !m.terrain_shapes[31] && !m.border_shapes[9]);
    assert(m.raw04 && m.masks && m.v8dcc);
}

static void clamp_footprint(void) {
    float x = 0.25f, y = 9.9f;
    /* 0x00443140: pushed inside a 10x10 map, no snap */
    assert(aoe_map_clamp_footprint_ror(0.5f, 0.5f, 10, 10, &x, &y, 0) == 1);
    assert(x == 0.5f && fabsf(y - (9.5f - 1e-5f)) < 1e-4f);
    /* snap: 2.25..3.75 spans whole tiles' fractions under one: kept;
       2.75..3.25 (fractions .75 / 1 - .25, a tie) moves right by .25 */
    x = 3.0f; y = 5.0f;
    aoe_map_clamp_footprint_ror(0.75f, 0.5f, 10, 10, &x, &y, 1);
    /* (y 4.5..5.5: fractions .5 / .5 sum to one, a tie: up by .5) */
    assert(x == 3.0f && y == 4.5f);
    y = 5.2f;
    aoe_map_clamp_footprint_ror(0.25f, 0.1f, 10, 10, &x, &y, 1);
    assert(fabsf(x - 3.25f) < 1e-6f && y == 5.2f);
    /* y 4.6..5.2: low fraction .6 below 1 - .2, so up by .2 */
    x = 5.5f; y = 4.9f;
    aoe_map_clamp_footprint_ror(0.5f, 0.3f, 10, 10, &x, &y, 1);
    assert(x == 5.5f && fabsf(y - 4.7f) < 1e-5f);
}


/* Tile objects 1..4 with template IDs 10, 20, 30, 40; player 1 lists 30. */
static int32_t sw_next_of[5] = {0, 2, 3, 4, 0}; static int16_t sw_id[5] = {0, 10, 20, 30, 40};
static int sw_removed[5], sw_destroyed[5], sw_place[5] = {0, 1, 1, 0, 0};
static int32_t sw_first(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return 1; }
static int32_t sw_next(void *c, int32_t o) { (void)c; return sw_next_of[o]; }
static int16_t sw_tid(void *c, int32_t o) { (void)c; return sw_id[o]; }
static int16_t sw_pcount(void *c, int32_t p) { (void)c; (void)p; return 0; }
static int16_t sw_pclass(void *c, int32_t p, int16_t i) { (void)c; (void)p; (void)i; return 30; }
static int sw_can(void *c, int32_t o) { (void)c; return sw_place[o]; }
static void sw_remove(void *c, int32_t o) { (void)c; sw_removed[o]++; }
static void sw_destroy(void *c, int32_t o) { (void)c; sw_destroyed[o]++; }
static void sweep_tests(void) {
    AoeTileSweepOpsRor o = {NULL, sw_first, sw_next, sw_tid, sw_pcount, sw_pclass, sw_can, sw_remove, sw_destroy};
    /* No player: everything outside 20..20 goes (removed). */
    assert(aoe_tilemap_sweep_tile_ror(&o, 1, 1, -1, 0, 20, 20) == 40);
    assert(sw_removed[1] == 1 && sw_removed[2] == 0 && sw_removed[3] == 1 && sw_removed[4] == 1);
    /* Player 1: 10 (placeable) and 30 (listed) go, deleted; 40 stays. */
    assert(aoe_tilemap_sweep_tile_ror(&o, 1, 1, 1, 1, 20, 20) == 40);
    assert(sw_destroyed[1] == 1 && sw_destroyed[3] == 1 && sw_destroyed[4] == 0 && sw_destroyed[2] == 0);
}

static int32_t to_next[6] = {0, 2, 3, 0, 0, 0}; static int16_t to_tid[6] = {0, 10, 20, 30, 0, 0}; static int32_t to_owner[6] = {0, 1, 1, 2, 0, 0};
static int to_removed[6], to_created, to_rand_i; static int32_t to_rands[8] = {0, 0x7fff, 0x4000, 0, 0x7fff, 0, 0, 0};
static float to_cx[8], to_cy[8]; static int16_t to_cid[8];
static int32_t to_first(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return 1; }
static int32_t to_nxt(void *c, int32_t o) { (void)c; return to_next[o]; }
static int16_t to_t(void *c, int32_t o) { (void)c; return to_tid[o]; }
static int32_t to_own(void *c, int32_t o) { (void)c; return to_owner[o]; }
static uint8_t to_terrain(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return 4; }
static int16_t to_count(void *c, int32_t t) { (void)c; return t == 4 ? 2 : 0; }
static int16_t to_id(void *c, int32_t t, int16_t i) { (void)c; (void)t; return i == 0 ? 300 : -1; }
static int16_t to_den(void *c, int32_t t, int16_t i) { (void)c; (void)t; (void)i; return 1500; }
static uint8_t to_mode(void *c, int32_t t, int16_t i) { (void)c; (void)t; (void)i; return 0; }
static int to_has(void *c, int32_t p, int16_t id) { (void)c; (void)p; return id == 300; }
static int to_can(void *c, int32_t p, int16_t id, float x, float y) { (void)c; (void)p; (void)id; (void)x; (void)y; return 0; }
static void to_create(void *c, int32_t p, int16_t id, float x, float y) { (void)c; (void)p; if (to_created < 8) { to_cid[to_created] = id; to_cx[to_created] = x; to_cy[to_created] = y; } ++to_created; }
static void to_remove(void *c, int32_t o) { (void)c; to_removed[o]++; }
static void to_destroy(void *c, int32_t o) { (void)c; to_removed[o] += 10; }
static int32_t to_rand(void *c) { (void)c; return to_rands[to_rand_i++ & 7]; }
static void terrain_objects_tests(void) {
    AoeTerrainObjectsOpsRor o = {NULL, to_first, to_nxt, to_t, to_own, to_terrain, to_count, to_id, to_den, to_mode, to_has, to_can, to_create, to_remove, to_destroy, to_rand};
    /* Player 1 owns objects 1 (id 10) and 2 (id 20); 20..20 is kept; object 3 is player 2's.
       Density 1500: the first at a random point (rands 0, 0x7fff -> x+0, y+1), the second
       (500 left) placed when rand*1000/0x7fff (0x4000 -> 500) is below 500: no. */
    aoe_tilemap_place_terrain_objects_ror(&o, 1, 6, 7, -1, 0, 20, 20);
    assert(to_removed[1] == 1 && to_removed[2] == 0 && to_removed[3] == 0);
    assert(to_created == 1 && to_cid[0] == 300 && to_cx[0] == 6.0f && to_cy[0] > 7.99f && to_cy[0] <= 8.0f);
}

static void terrain_entry_tests(void) {
    AoeTerrainObjectsOpsRor o = {NULL, to_first, to_nxt, to_t, to_own, to_terrain, to_count, to_id, to_den, to_mode, to_has, to_can, to_create, to_remove, to_destroy, to_rand};
    int before = to_created;
    to_rand_i = 0;
    aoe_tilemap_place_terrain_entry_ror(&o, 1, 6, 7, 1); /* one attempt, no sweep */
    assert(to_created == before + 1 && to_removed[1] == 1);
    to_rand_i = 0;
    aoe_tilemap_place_terrain_entry_ror(&o, 1, 6, 7, 0); /* full density, same rands as the sweep test */
    assert(to_created == before + 2);
}

static int16_t pb_between(void *c, uint8_t a, uint8_t b) { (void)c; return a == 1 && b == 2 ? 3 : a == 1 && b == 4 ? 20 : -1; }
static int pb_used(void *c, int16_t b) { (void)c; return b == 3; }
static int16_t pb_style = 0;
static int16_t pb_stylef(void *c, int16_t b) { (void)c; (void)b; return pb_style; }
static void elevation_border_tests(void) {
    static AoeMapTileRor tiles[9];
    AoeTileMapRor m; AoeTileMapOpsRor ops = {0}; float a, b; int i;
    memset(&m, 0, sizeof m); m.width = 3; m.height = 3; m.tiles = tiles; m.tile_w = 64; m.tile_h = 32;
    ops.border_between = pb_between; ops.border_used = pb_used;
    /* Flat tile at its centre: a = u + v - ..., b = (2v - 1 + 2u) / 2. */
    tiles[0].shape = 0;
    aoe_tilemap_elevation_point_ror(&m, 32, 16, &tiles[0], &a, &b);
    assert(fabsf(b - 0.5f) < 1e-6f && fabsf(a - 0.5f) < 1e-6f);
    tiles[0].shape = 7;  /* h = 2v + 2u - 1 = 1 */
    aoe_tilemap_elevation_point_ror(&m, 32, 16, &tiles[0], &a, &b);
    assert(fabsf(b - 1.0f) < 1e-6f && fabsf(a - 0.0f) < 1e-6f);
    tiles[0].shape = 10;  /* h = -v */
    aoe_tilemap_elevation_point_ror(&m, 0, 16, &tiles[0], &a, &b);
    assert(fabsf(b - (-0.25f)) < 1e-6f && fabsf(a - 0.25f) < 1e-6f);
    /* Borders: terrain 1 everywhere, terrain 2 to the north and west of the centre -> NW corner shape 1. */
    for (i = 0; i < 9; ++i) tiles[i].terrain_elevation = 1;
    tiles[1].terrain_elevation = 2; tiles[3].terrain_elevation = 2;
    aoe_tilemap_pick_border_ror(&m, 1, 1, pb_stylef, &ops);
    assert(tiles[4].border == 0x13);
    pb_style = 1;  /* flag style: north (1) and west (4) */
    aoe_tilemap_pick_border_ror(&m, 1, 1, pb_stylef, &ops);
    assert(tiles[4].border == 0x53);
    pb_style = 0; tiles[1].terrain_elevation = 1; tiles[3].terrain_elevation = 1; tiles[8].terrain_elevation = 2;
    aoe_tilemap_pick_border_ror(&m, 1, 1, pb_stylef, &ops);
    assert(tiles[4].border == 0x83);
    tiles[8].terrain_elevation = 4;  /* border 20 is outside 0..15 */
    aoe_tilemap_pick_border_ror(&m, 1, 1, pb_stylef, &ops);
    assert(tiles[4].border == 0);
    tiles[8].terrain_elevation = 1;
    tiles[4].border = 9; aoe_tilemap_pick_border_ror(&m, 1, 1, pb_stylef, &ops); assert(tiles[4].border == 0);
    /* A corner tile reuses itself for the missing neighbours. */
    tiles[1].terrain_elevation = 2; aoe_tilemap_pick_border_ror(&m, 0, 0, pb_stylef, &ops); assert(tiles[0].border == 0xc3);
}
int main(void) {
    clamp_footprint();
    AoeTileMapOpsRor ops = {NULL, t_resize, t_refresh, t_rect, t_cell, t_begin, t_end, t_border, t_used};
    test_lines(&ops);
    {
        AoeTileMapRor fm;
        int16_t o[8];
        memset(&fm, 0, sizeof(fm));
        fm.half_w = 32;
        fm.half_h = 16;
        /* (2.25, 3.5) with a 0.5 x 0.5 footprint: fx 0.75, fy 1.0. */
        assert(aoe_tilemap_footprint_corners_ror(&fm, 0.5f, 0.5f, 2.25f, 3.5f, o) == 1);
        assert(o[0] == 56 && o[1] == 20);   /* (1.75*32, 1.25*16) */
        assert(o[2] == 24 && o[3] == 36);   /* fx -0.25 */
        assert(o[4] == -8 && o[5] == 20);   /* fy 0.0 */
        assert(o[6] == 24 && o[7] == 4);    /* fx 0.75 again */
    }
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
    test_map_release_owned();
    sweep_tests();
    terrain_objects_tests();
    terrain_entry_tests();
    elevation_border_tests();
    return 0;
}
