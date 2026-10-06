#include "tile_map.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "aoe_ftol.h"
#include "aoe_stream.h"

void aoe_tilemap_set_shape_ror(AoeTileMapRor *m, uint8_t i, int16_t w, int16_t h, int16_t offset) {
    m->shapes[i].width = w;
    m->shapes[i].height = h;
    m->shapes[i].offset = offset;
}

void aoe_tilemap_default_shapes_ror(AoeTileMapRor *m) {
    /* Height selector per shape: 0 elevation, 1 tile, 2 tile + elevation. */
    static const struct {
        uint8_t height;
        int8_t offset;
    } k[19] = {{1, 0}, {0, -8}, {2, 8}, {1, -8}, {1, -8}, {0, -8}, {2, 8}, {0, -8}, {2, 8}, {0, -8},
               {2, 8}, {1, -8}, {1, -8}, {0, -8}, {2, 8}, {1, 8},  {1, 8}, {1, 0},  {1, 0}};
    int i;
    m->tile_w = 0x40;
    m->tile_h = 0x20;
    m->half_w = 0x20;
    m->half_h = 0x10;
    m->elevation_h = 0x10;
    for (i = 0; i < 19; ++i) {
        int16_t h = k[i].height == 0 ? m->elevation_h
                    : k[i].height == 1 ? m->tile_h
                                       : (int16_t)(m->tile_h + m->elevation_h);
        aoe_tilemap_set_shape_ror(m, (uint8_t)i, i >= 17 ? m->half_w : m->tile_w, h, k[i].offset);
    }
    /* Shape 0 is written with the constants 64 and 32. */
    aoe_tilemap_set_shape_ror(m, 0, 0x40, 0x20, 0);
}

AoeMapTileRor *aoe_tilemap_tiles_ror(const AoeTileMapRor *m) { return m->tiles; }

static AoeMapTileRor *at(const AoeTileMapRor *m, int32_t x, int32_t y) {
    return &m->tiles[(size_t)y * (size_t)m->width + (size_t)x];
}

static void set_elevation(AoeMapTileRor *t, uint8_t e) {
    t->terrain_elevation = (uint8_t)((t->terrain_elevation & 0x1f) | (uint8_t)(e << 5));
}

void aoe_tilemap_invalidate_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                                uint8_t flag) {
    int32_t x, y, t;
    if (x1 < x0) {
        t = x0; x0 = x1; x1 = t;
    }
    if (y1 < y0) {
        t = y0; y0 = y1; y1 = t;
    }
    if (x1 < 0 || y1 < 0 || x0 >= m->width || y0 >= m->height) return;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= m->width) x1 = m->width - 1;
    if (y1 >= m->height) y1 = m->height - 1;
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x) {
            at(m, x, y)->dirty = 0xcc;
            at(m, x, y)->draw_flag = flag;
        }
}

static int32_t clamp(int32_t v, int32_t size) {
    if (v < 0) return 0;
    return v > size - 1 ? size - 1 : v;
}

void aoe_tilemap_positions_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    int32_t x, y;
    x0 = clamp(x0, m->width);
    y0 = clamp(y0, m->height);
    x1 = clamp(x1, m->width);
    y1 = clamp(y1, m->height);
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x) {
            AoeMapTileRor *t = at(m, x, y);
            t->screen_x = (int16_t)((int16_t)(y + x) * m->half_w);
            t->screen_y = (int16_t)((int16_t)((int16_t)y - (int16_t)x) * m->half_h -
                                    aoe_tile_elevation_ror(t) * m->elevation_h);
            switch (t->shape) {
            case 2: case 6: case 8: case 10: case 14: case 15: case 16:
                t->screen_y = (int16_t)(t->screen_y - m->elevation_h);
                break;
            default:
                break;
            }
        }
}

/* The eight neighbours' elevations (north, south, west, east, NW, NE,
   SW, SE) present at (x, y); -1 where off the map. */
static void neighbours(const AoeTileMapRor *m, int32_t x, int32_t y, int e[8]) {
    int n = y > 0, s = y < m->height - 1, w = x > 0, ea = x < m->width - 1;
    e[0] = n ? aoe_tile_elevation_ror(at(m, x, y - 1)) : -1;
    e[1] = s ? aoe_tile_elevation_ror(at(m, x, y + 1)) : -1;
    e[2] = w ? aoe_tile_elevation_ror(at(m, x - 1, y)) : -1;
    e[3] = ea ? aoe_tile_elevation_ror(at(m, x + 1, y)) : -1;
    e[4] = n && w ? aoe_tile_elevation_ror(at(m, x - 1, y - 1)) : -1;
    e[5] = n && ea ? aoe_tile_elevation_ror(at(m, x + 1, y - 1)) : -1;
    e[6] = s && w ? aoe_tile_elevation_ror(at(m, x - 1, y + 1)) : -1;
    e[7] = s && ea ? aoe_tile_elevation_ror(at(m, x + 1, y + 1)) : -1;
}

int aoe_tilemap_step_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                         uint8_t target) {
    int32_t x, y;
    int changed = 0, i, e[8];
    if (x0 > 0) --x0;
    if (y0 > 0) --y0;
    if (x1 < m->width - 1) ++x1;
    if (y1 < m->height - 1) ++y1;
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x) {
            AoeMapTileRor *t = at(m, x, y);
            int el = aoe_tile_elevation_ror(t), hit = 0;
            neighbours(m, x, y, e);
            if (el < target) {
                for (i = 0; i < 8; ++i)
                    if (e[i] >= 0 && el + 1 < e[i]) hit = 1;
                if (hit) set_elevation(t, (uint8_t)(el + 1));
            } else if (el > target) {
                for (i = 0; i < 8; ++i)
                    if (e[i] >= 0 && e[i] < el - 1) hit = 1;
                if (hit) set_elevation(t, (uint8_t)(el - 1));
            }
            if (hit) changed = 1;
        }
    return changed;
}

void aoe_tilemap_smooth_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                            uint8_t limit) {
    static const int8_t dx[8] = {0, 0, -1, 1, -1, 1, -1, 1}, dy[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
    /* The original writes the neighbours in this order. */
    static const int8_t order[8] = {4, 0, 5, 3, 7, 1, 6, 2};
    int changed;
    do {
        int32_t x, y;
        changed = 0;
        if (x0 > 0) --x0;
        if (y0 > 0) --y0;
        if (x1 < m->width - 1) ++x1;
        if (y1 < m->height - 1) ++y1;
        if (y1 < y0) return;
        for (y = y0; y <= y1; ++y)
            for (x = x0; x <= x1; ++x) {
                AoeMapTileRor *t = at(m, x, y);
                int el = aoe_tile_elevation_ror(t), e[8], hi[8], i;
                neighbours(m, x, y, e);
                for (i = 0; i < 8; ++i) hi[i] = e[i] >= 0 && el < e[i];
                if (!((hi[3] && hi[2]) || (hi[0] && hi[1]))) continue;
                if (el < limit) {
                    set_elevation(t, (uint8_t)(el + 1));
                } else {
                    for (i = 0; i < 8; ++i)
                        if (hi[order[i]])
                            set_elevation(at(m, x + dx[order[i]], y + dy[order[i]]), (uint8_t)el);
                }
                changed = 1;
            }
    } while (changed);
}

static void square(const AoeTileMapRor *m, int32_t x, int32_t y, int32_t r, int32_t *x0, int32_t *y0,
                   int32_t *x1, int32_t *y1) {
    *x0 = x - r < 0 ? 0 : x - r;
    *x1 = x + r >= m->width ? m->width - 1 : x + r;
    *y0 = y - r < 0 ? 0 : y - r;
    *y1 = y + r >= m->height ? m->height - 1 : y + r;
}

int aoe_tilemap_paint_square_ror(AoeTileMapRor *m, int32_t x, int32_t y, int32_t r, int32_t terrain,
                                 const AoeTileMapOpsRor *ops) {
    int32_t x0, y0, x1, y1, i, j;
    square(m, x, y, r, &x0, &y0, &x1, &y1);
    for (j = y0; j <= y1; ++j)
        for (i = x0; i <= x1; ++i) ops->paint_cell(ops->context, i, j, terrain);
    return 1;
}

int aoe_tilemap_raise_square_ror(AoeTileMapRor *m, int32_t x, int32_t y, int32_t r, uint8_t elevation) {
    int32_t x0, y0, x1, y1, i, j;
    square(m, x, y, r, &x0, &y0, &x1, &y1);
    for (j = y0; j <= y1; ++j)
        for (i = x0; i <= x1; ++i) set_elevation(at(m, i, j), elevation);
    return 1;
}

typedef void (*AoeLineBrushRor)(AoeTileMapRor *m, int32_t x, int32_t y, int32_t r, int32_t value,
                                const AoeTileMapOpsRor *ops);

static void line_paint(AoeTileMapRor *m, int32_t x, int32_t y, int32_t r, int32_t value,
                       const AoeTileMapOpsRor *ops) {
    (void)aoe_tilemap_paint_square_ror(m, x, y, r, value, ops);
}
static void line_raise(AoeTileMapRor *m, int32_t x, int32_t y, int32_t r, int32_t value,
                       const AoeTileMapOpsRor *ops) {
    (void)ops;
    (void)aoe_tilemap_raise_square_ror(m, x, y, r, (uint8_t)value);
}

/* Shared body of 0x00446050 / 0x00446250 (clamped ends written back). */
static void brush_line(AoeTileMapRor *m, int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1,
                       int32_t r, int32_t value, AoeLineBrushRor brush,
                       const AoeTileMapOpsRor *ops) {
    double dx, dy, sx, sy, cx, cy;
    int32_t steps;
    if (*x0 < 0) *x0 = 0;
    if (*x1 >= m->width) *x1 = m->width - 1;
    if (*y0 < 0) *y0 = 0;
    if (*y1 >= m->height) *y1 = m->height - 1;
    dx = (double)(*x1 - *x0);
    dy = (double)(*y1 - *y0);
    steps = aoe_ftol32(sqrt(dx * dx + dy * dy));
    /* With no steps the original's 0/0 increments are never used. */
    sx = steps ? dx / (double)steps : 0.0;
    sy = steps ? dy / (double)steps : 0.0;
    cx = (double)*x0;
    cy = (double)*y0;
    brush(m, *x0, *y0, r, value, ops);
    for (int32_t i = steps; i > 0; --i) {
        cx += sx;
        cy += sy;
        brush(m, aoe_ftol32(cx), aoe_ftol32(cy), r, value, ops);
    }
    if ((double)*x1 != cx || (double)*y1 != cy) brush(m, *x1, *y1, r, value, ops);
}

/* 0x00446050 */
int aoe_tilemap_paint_line_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                               int32_t width, int32_t terrain, const AoeTileMapOpsRor *ops) {
    brush_line(m, &x0, &y0, &x1, &y1, width >> 1, terrain, line_paint, ops);
    return 1;
}

/* 0x00446250 */
int aoe_tilemap_raise_line_ror(AoeTileMapRor *m, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                               int32_t width, int32_t elevation,
                               void (*cleanup)(void *context, int32_t x0, int32_t y0,
                                               int32_t x1, int32_t y1, int32_t value),
                               void *context) {
    int32_t r = width >> 1, lx, hx, ly, hy;
    brush_line(m, &x0, &y0, &x1, &y1, r, elevation, line_raise, NULL);
    if (x0 < x1) { lx = x0 - r; hx = x1 + r; } else { lx = x1 - r; hx = x0 + r; }
    if (y0 < y1) { ly = y0 - r; hy = y1 + r; } else { ly = y1 - r; hy = y0 + r; }
    if (cleanup) cleanup(context, lx, ly, hx, hy, elevation);
    return 1;
}

/* 0x004433c0 */
int aoe_tilemap_footprint_corners_ror(const AoeTileMapRor *m, float size_x, float size_y,
                                      float x, float y, int16_t out[8]) {
    double hw = (double)m->half_w, hh = (double)m->half_h, t;
    float fx = (float)((double)x - (double)(int16_t)aoe_ftol32(x) + (double)size_x);
    float fy = (float)((double)y - (double)(int16_t)aoe_ftol32(y) + (double)size_y);
    double x2, fy2;
    out[0] = (int16_t)aoe_ftol32(((double)fy + (double)fx) * hw);
    t = (double)fy + 1.0;
    out[1] = (int16_t)aoe_ftol32((t - (double)fx) * hh);
    fx = (float)((double)fx - ((double)size_x + (double)size_x));
    out[2] = (int16_t)aoe_ftol32(((double)fy + (double)fx) * hw);
    out[3] = (int16_t)aoe_ftol32((t - (double)fx) * hh);
    fy2 = (double)fy - ((double)size_y + (double)size_y); /* kept unrounded */
    out[4] = (int16_t)aoe_ftol32((fy2 + (double)fx) * hw);
    t = fy2 + 1.0;
    out[5] = (int16_t)aoe_ftol32((t - (double)fx) * hh);
    x2 = (double)size_x + (double)size_x + (double)fx;
    out[6] = (int16_t)aoe_ftol32((fy2 + x2) * hw);
    out[7] = (int16_t)aoe_ftol32((t - x2) * hh);
    return 1;
}

int aoe_tilemap_load_old_ror(AoeTileMapRor *m, AoeOutcomeStream *s, const AoeTileMapOpsRor *ops) {
    int32_t w = aoe_stream_i32(s), h = aoe_stream_i32(s), x, y;
    if (s->failed || !ops->resize(ops->context, m, w, h)) return 0;
    for (y = 0; y < m->height; ++y)
        for (x = 0; x < m->height; ++x) {
            uint8_t terrain = aoe_stream_u8(s), elevation = aoe_stream_u8(s);
            AoeMapTileRor *t;
            (void)aoe_stream_u8(s);
            if (x >= m->width) continue; /* host guard: the original writes past the row */
            t = at(m, x, y);
            t->terrain_elevation = (uint8_t)((t->terrain_elevation & 0xe0) | (terrain & 0x1f));
            set_elevation(t, elevation);
        }
    ops->refresh_shapes(ops->context, m, 0, 0, (int16_t)(m->width - 1), (int16_t)(m->height - 1));
    ops->paint_rect(ops->context, m, NULL, 0, 0, 0, (int16_t)(m->width - 1), (int16_t)(m->height - 1), 0,
                    0, 0);
    return !s->failed;
}

int aoe_tilemap_new_filled_ror(AoeTileMapRor *m, void *a, int32_t world, int32_t terrain, int32_t w,
                               int32_t h, const AoeTileMapOpsRor *ops) {
    if (!ops->resize(ops->context, m, w, h)) return 0;
    if (world) ops->world_begin(ops->context, world);
    ops->paint_rect(ops->context, m, a, world, 0, 0, w - 1, h - 1, terrain, 1, 0);
    if (world) ops->world_end(ops->context, world);
    return 1;
}

void aoe_tilemap_pick_terrain_ror(uint8_t terrain, uint8_t *cell, const AoeTileMapOpsRor *ops) {
    int16_t border;
    if (*cell == terrain) return;
    border = ops->border_between(ops->context, terrain, *cell);
    if (border < 0 || border > 15 || !ops->border_used(ops->context, border)) *cell = terrain;
}

void aoe_tilemap_nothing_ror(AoeTileMapRor *m) { (void)m; }

void aoe_tilemap_invalidate_around_ror(AoeTileMapRor *m, float x, float y) {
    int32_t tx = (int32_t)x, ty = (int32_t)y;
    aoe_tilemap_invalidate_ror(m, tx - 2, ty - 2, tx + 1, ty + 1, 0);
}

int aoe_tilemap_generate_ror(AoeTileMapRor *m, int32_t world, int32_t w, int32_t h, int32_t a,
                             int32_t b, const AoeTileMapOpsRor *ops,
                             const AoeTileMapGenOpsRor *gen) {
    uint8_t saved = gen->world_flag(gen->context);
    int32_t seed;
    char line[64];
    int ok;
    gen->set_world_flag(gen->context, 3);
    seed = gen->game_seed(gen->context);
    if (seed == -1) seed = gen->rand(gen->context);
    gen->set_used_seed(gen->context, seed);
    snprintf(line, sizeof line, "Random Map Seed = %d", (int)seed);
    gen->log(gen->context, line);
    if (w < 0) w = m->width;
    if (h < 0) h = m->height;
    gen->srand(gen->context, (uint32_t)seed);
    ok = aoe_tilemap_new_filled_ror(m, NULL, world, 0, w, h, ops);
    gen->srand(gen->context, (uint32_t)seed);
    gen->rm_setup(gen->context, m, world, a, b);
    gen->srand(gen->context, (uint32_t)seed);
    gen->rm_generate(gen->context);
    gen->rm_finish(gen->context);
    snprintf(line, sizeof line, "After Random Map Generation random number is %d",
             (int)gen->rand(gen->context));
    gen->log(gen->context, line);
    gen->set_world_flag(gen->context, saved);
    return ok;
}

static void put32at(uint8_t *p, int32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)((uint32_t)v >> 8);
    p[2] = (uint8_t)((uint32_t)v >> 16);
    p[3] = (uint8_t)((uint32_t)v >> 24);
}

int aoe_tilemap_save_data_ror(const uint8_t image[AOE_MAP_RECORD_SIZE],
                              int32_t (*sound_id)(void *context, int border, int index),
                              void (*controller_save)(void *context, AoeOutcomeStream *s),
                              void *context, AoeOutcomeStream *s) {
    uint8_t *copy = calloc(1, AOE_MAP_RECORD_SIZE);
    int i;
    if (!copy) return 0;
    memcpy(copy, image, AOE_MAP_RECORD_SIZE);
    for (i = 0; i < AOE_MAP_TERRAINS; ++i) {
        size_t base = 0x8c + (size_t)i * 0x198;
        if (image[base] != 0) put32at(copy + base + 0x24, sound_id(context, 0, i));
    }
    for (i = 0; i < AOE_MAP_BORDERS; ++i) {
        size_t base = 0x338c + (size_t)i * 0x5a0;
        put32at(copy + base + 0x24, image[base] != 0 ? sound_id(context, 1, i) : -1);
    }
    aoe_stream_put(s, copy, AOE_MAP_RECORD_SIZE);
    controller_save(context, s);
    free(copy);
    return !s->failed;
}

/* 0x00444110 (0x004ed840 / deleting 0x004ed7e0) */
void aoe_map_release_owned_ror(AoeMapOwnedRor *m, const AoeMapOwnedOpsRor *ops) {
    int32_t i;
    if (m->zones) {
        ops->destroy(ops->context, AOE_MAP_OWNED_ZONES, m->zones);
        m->zones = NULL;
    }
    if (m->random_map) {
        ops->destroy(ops->context, AOE_MAP_OWNED_RANDOM_MAP, m->random_map);
        m->random_map = NULL;
    }
    if (m->raw04) ops->free_raw(ops->context, m->raw04);
    if (m->raw8db0) ops->free_raw(ops->context, m->raw8db0);
    if (m->rows8d8c) ops->free_raw(ops->context, m->rows8d8c);
    if (m->raw8db4) ops->free_raw(ops->context, m->raw8db4);
    for (i = 31; i >= 0; --i)
        if (m->terrain_shapes[i]) {
            ops->destroy(ops->context, AOE_MAP_OWNED_SHAPE, m->terrain_shapes[i]);
            m->terrain_shapes[i] = NULL;
        }
    for (i = 15; i >= 0; --i)
        if (m->border_shapes[i]) {
            ops->destroy(ops->context, AOE_MAP_OWNED_SHAPE, m->border_shapes[i]);
            m->border_shapes[i] = NULL;
        }
    if (m->masks) ops->destroy(ops->context, AOE_MAP_OWNED_MASKS, m->masks);
    if (m->v8dcc) ops->destroy(ops->context, AOE_MAP_OWNED_8DCC, m->v8dcc);
}

static float frac_i16(float v) {
    return v - (float)(int16_t)aoe_ftol32(v);
}

int aoe_map_clamp_footprint_ror(float half_w, float half_h, int32_t map_w,
                                int32_t map_h, float *x, float *y, int snap) {
    const float eps = 9.999999747378752e-06f; /* 0x00544578 */
    float x0, y0, x1, y1, max_x, max_y;
    if (!x || !y) return 0;
    x0 = *x - half_w; y0 = *y - half_h;
    x1 = *x + half_w; y1 = *y + half_h;
    max_x = (float)map_w - eps;
    max_y = (float)map_h - eps;
    if (x0 < 0.0f) { x1 += -x0; *x += -x0; x0 = 0.0f; }
    if (y0 < 0.0f) { y1 += -y0; *y += -y0; y0 = 0.0f; }
    if (max_x <= x1) { x0 += max_x - x1; *x += max_x - x1; x1 = max_x; }
    if (!(y1 < max_y)) { y0 += max_y - y1; *y += max_y - y1; }
    if (snap) {
        float a, b, c, d;
        if (x0 < 0.0f) x0 = -x0;
        if (y0 < 0.0f) y0 = -y0;
        if (x1 < 0.0f) x1 = -x1;
        if (y1 < 0.0f) y1 = -y1;
        a = frac_i16(x0);
        c = frac_i16(y0);
        b = 1.0f - frac_i16(x1);
        d = 1.0f - frac_i16(y1);
        if (!(b + a < 1.0f)) {
            if (a < b) *x -= 1.0f - b;
            else *x += 1.0f - a;
        }
        if (!(d + c < 1.0f)) {
            if (c > d) *y += 1.0f - c;
            else *y -= 1.0f - d;
        }
    }
    return 1;
}

int32_t aoe_tilemap_sweep_tile_ror(const AoeTileSweepOpsRor *o, int32_t x, int32_t y,
                                   int32_t terrain, int32_t delete_them, int32_t lo, int32_t hi) {
    int32_t player = terrain;
    int32_t obj, next, last = -1;
    if (!o) return -1;
    for (obj = o->first(o->context, x, y); obj; obj = next) {
        int16_t id, k, n;
        int listed = 0;
        next = o->next(o->context, obj);
        id = o->template_id(o->context, obj);
        if (lo <= id && id <= hi) continue;
        last = id;
        if (player >= 0) {
            n = o->player_class_count(o->context, player);
            for (k = n; k >= 0; --k)
                if (o->player_class(o->context, player, k) == id) { listed = 1; break; }
            if (!listed && !o->can_place(o->context, obj)) continue;
        }
        if (delete_them) o->destroy(o->context, obj); else o->remove(o->context, obj);
    }
    return last;
}

void aoe_tilemap_place_terrain_objects_ror(const AoeTerrainObjectsOpsRor *o, int32_t player,
                                           int32_t x, int32_t y, int32_t filter,
                                           int32_t delete_them, int32_t lo, int32_t hi) {
    uint8_t terrain;
    int16_t count, i;
    int32_t obj, next;
    if (!o) return;
    terrain = o->terrain_at(o->context, x, y);
    count = o->table_count(o->context, terrain);
    if (count <= 0) return;
    for (obj = o->first(o->context, x, y); obj; obj = next) {
        int16_t id;
        int go = 0;
        next = o->next(o->context, obj);
        if (o->owner(o->context, obj) != player) continue;
        id = o->template_id(o->context, obj);
        if (id >= lo && id <= hi) continue;
        if (filter < 0) {
            go = 1;
        } else {
            int16_t k, n = o->table_count(o->context, filter);
            for (k = n; k >= 0; --k)
                if (o->table_id(o->context, filter, k) == id) { go = 1; break; }
        }
        if (!go) continue;
        if (delete_them) o->destroy(o->context, obj); else o->remove(o->context, obj);
    }
    for (i = 0; i < count; ++i) {
        int16_t id = o->table_id(o->context, terrain, i);
        int32_t density, left;
        uint8_t mode;
        if (id == -1) continue;
        density = o->table_density(o->context, terrain, i);
        mode = o->table_mode(o->context, terrain, i);
        if (!o->player_has_template(o->context, player, id) || density <= 0) continue;
        for (left = density; left > 0; left -= 1000) {
            float px = 0.0f, py = 0.0f;
            if (mode == 0) {
                px = (float)o->rand(o->context) * 3.0518509447574615e-05f + (float)x;
                py = (float)o->rand(o->context) * 3.0518509447574615e-05f + (float)y;
            } else if (mode == 1) {
                px = (float)x + 0.5f;
                py = (float)y + 0.5f;
            }
            if ((left > 999 || (o->rand(o->context) * 1000) / 0x7fff < left) &&
                !o->can_place(o->context, player, id, px, py))
                o->create(o->context, player, id, px, py);
        }
    }
}

void aoe_tilemap_place_terrain_entry_ror(const AoeTerrainObjectsOpsRor *o, int32_t player,
                                         int32_t x, int32_t y, int32_t once) {
    uint8_t terrain;
    int16_t count, i;
    if (!o) return;
    terrain = o->terrain_at(o->context, x, y);
    count = o->table_count(o->context, terrain);
    for (i = 0; i < count; ++i) {
        int16_t id = o->table_id(o->context, terrain, i);
        int32_t density, left;
        uint8_t mode;
        if (id == -1) continue;
        density = o->table_density(o->context, terrain, i);
        mode = o->table_mode(o->context, terrain, i);
        if (!o->player_has_template(o->context, player, id)) continue;
        if (once == 0) {
            for (left = density; left > 0; left -= 1000) {
                float px = 0.0f, py = 0.0f;
                if (mode == 0) {
                    px = (float)o->rand(o->context) * 3.0518509447574615e-05f + (float)x;
                    py = (float)o->rand(o->context) * 3.0518509447574615e-05f + (float)y;
                } else {
                    px = (float)x + 0.5f;
                    py = (float)y + 0.5f;
                }
                if ((left > 999 || (o->rand(o->context) * 1000) / 0x7fff < left) &&
                    !o->can_place(o->context, player, id, px, py))
                    o->create(o->context, player, id, px, py);
            }
        } else if (once == 1) {
            float px, py;
            if (mode == 0) {
                px = (float)o->rand(o->context) * 3.0518509447574615e-05f + (float)x;
                py = (float)o->rand(o->context) * 3.0518509447574615e-05f + (float)y;
            } else {
                px = (float)x + 0.5f;
                py = (float)y + 0.5f;
            }
            if (!o->can_place(o->context, player, id, px, py)) {
                o->create(o->context, player, id, px, py);
                return;
            }
        }
    }
}
