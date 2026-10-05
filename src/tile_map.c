#include "tile_map.h"

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
