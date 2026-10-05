#include "value_grid.h"

#include <stdlib.h>
#include <string.h>

/* The clockwise neighbour offsets from the top-left. */
static const int8_t ring_dx[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
static const int8_t ring_dy[8] = {-1, -1, -1, 0, 1, 1, 1, 0};

static int alloc_cells(AoeValueGridRor *g, uint8_t fill) {
    int32_t y;
    size_t n = (size_t)(g->width > 0 ? g->width : 0) *
               (size_t)(g->height > 0 ? g->height : 0);
    g->cells = malloc(n ? n : 1);
    g->rows = malloc((size_t)(g->height > 0 ? g->height : 1) *
                     sizeof *g->rows);
    if (!g->cells || !g->rows) return 0;
    memset(g->cells, fill, n);
    for (y = 0; y < g->height; ++y) g->rows[y] = g->cells + (size_t)y * g->width;
    return 1;
}

int aoe_value_grid_init_ror(AoeValueGridRor *g, int32_t w, int32_t h,
                            uint8_t fill) {
    memset(g, 0, sizeof *g);
    g->width = w;
    g->height = h;
    return alloc_cells(g, fill);
}

void aoe_value_grid_free_ror(AoeValueGridRor *g) {
    free(g->cells);
    free(g->rows);
    g->cells = NULL;
    g->rows = NULL;
}

AoeValueGridRor *aoe_value_grid_destroy_ror(AoeValueGridRor *g, int heap) {
    aoe_value_grid_free_ror(g);
    if (heap) {
        free(g);
        return NULL;
    }
    return g;
}

int aoe_value_grid_fill_ror(AoeValueGridRor *g, uint8_t value) {
    memset(g->cells, value, (size_t)g->width * (size_t)g->height);
    return 1;
}

void aoe_value_grid_save_ror(const AoeValueGridRor *g, AoeOutcomeStream *out) {
    int32_t y;
    aoe_stream_put_i32(out, g->width);
    aoe_stream_put_i32(out, g->height);
    aoe_stream_put_i32(out, g->origin_x);
    aoe_stream_put_i32(out, g->origin_y);
    aoe_stream_put_u8(out, g->cap);
    for (y = 0; y < g->height; ++y)
        aoe_stream_put(out, g->rows[y], (size_t)g->width);
}

int aoe_value_grid_load_ror(AoeValueGridRor *g, AoeOutcomeStream *in) {
    int32_t w = aoe_stream_i32(in), h = aoe_stream_i32(in), y;
    const uint8_t *row;
    g->origin_x = aoe_stream_i32(in);
    g->origin_y = aoe_stream_i32(in);
    g->cap = aoe_stream_u8(in);
    if (in->failed || w < 0 || h < 0) return 0;
    if (w != g->width || h != g->height) {
        aoe_value_grid_free_ror(g);
        g->width = w;
        g->height = h;
        if (!alloc_cells(g, 0)) return 0;
    }
    for (y = 0; y < h; ++y) {
        row = aoe_stream_take(in, (size_t)w);
        if (!row) return 0;
        memcpy(g->rows[y], row, (size_t)w);
    }
    return 1;
}

uint8_t aoe_value_grid_get_ror(const AoeValueGridRor *g, int32_t x, int32_t y) {
    return g->rows[y - g->origin_y][x - g->origin_x];
}

static int inside(const AoeValueGridRor *g, int32_t x, int32_t y) {
    return x >= 0 && y >= 0 && x < g->width && y < g->height;
}

int aoe_value_grid_set_ror(AoeValueGridRor *g, int32_t x, int32_t y,
                           uint8_t value) {
    x -= g->origin_x;
    y -= g->origin_y;
    if (!inside(g, x, y)) return 0;
    g->rows[y][x] = value;
    return 1;
}

int aoe_value_grid_set_box_ror(AoeValueGridRor *g, int32_t x0, int32_t y0,
                               int32_t x1, int32_t y1, uint8_t value) {
    int32_t x, y;
    x0 -= g->origin_x;
    x1 -= g->origin_x;
    y0 -= g->origin_y;
    y1 -= g->origin_y;
    if (!inside(g, x0, y0) || !inside(g, x1, y1)) return 0;
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x) g->rows[y][x] = value;
    return 1;
}

static void raise_cell(uint8_t *c, uint8_t cap, uint8_t amount) {
    uint32_t sum;
    if (*c >= cap) return;
    sum = (uint32_t)*c + amount;
    *c = sum >= cap ? (uint8_t)(cap - 1) : (uint8_t)sum;
}

static void lower_cell(uint8_t *c, uint8_t cap, uint8_t amount) {
    int32_t d;
    if (*c >= cap) return;
    d = (int32_t)*c - amount;
    *c = d < 0 ? 0 : (uint8_t)d;
}

int aoe_value_grid_raise_ror(AoeValueGridRor *g, int32_t x, int32_t y,
                             uint8_t amount) {
    x -= g->origin_x;
    y -= g->origin_y;
    if (!inside(g, x, y)) return 0;
    raise_cell(&g->rows[y][x], g->cap, amount);
    return 1;
}

int aoe_value_grid_lower_ror(AoeValueGridRor *g, int32_t x, int32_t y,
                             uint8_t amount) {
    x -= g->origin_x;
    y -= g->origin_y;
    if (!inside(g, x, y)) return 0;
    lower_cell(&g->rows[y][x], g->cap, amount);
    return 1;
}

/* The original clamps all four coordinates into 0..width-1; the port also
   keeps the rows below the height. */
static int32_t clamp_to(int32_t v, int32_t n) {
    if (v < 0) return 0;
    return v >= n ? n - 1 : v;
}

static void box_apply(AoeValueGridRor *g, int32_t x0, int32_t y0, int32_t x1,
                      int32_t y1, uint8_t amount,
                      void (*op)(uint8_t *, uint8_t, uint8_t)) {
    int32_t x, y;
    x0 = clamp_to(x0 - g->origin_x, g->width);
    y0 = clamp_to(y0 - g->origin_y, g->width);
    x1 = clamp_to(x1 - g->origin_x, g->width);
    y1 = clamp_to(y1 - g->origin_y, g->width);
    if (y0 >= g->height) y0 = g->height - 1;
    if (y1 >= g->height) y1 = g->height - 1;
    for (y = y0; y <= y1; ++y)
        for (x = x0; x <= x1; ++x) op(&g->rows[y][x], g->cap, amount);
}

int aoe_value_grid_raise_box_ror(AoeValueGridRor *g, int32_t x0, int32_t y0,
                                 int32_t x1, int32_t y1, uint8_t amount) {
    box_apply(g, x0, y0, x1, y1, amount, raise_cell);
    return 1;
}

int aoe_value_grid_lower_box_ror(AoeValueGridRor *g, int32_t x0, int32_t y0,
                                 int32_t x1, int32_t y1, uint8_t amount) {
    box_apply(g, x0, y0, x1, y1, amount, lower_cell);
    return 1;
}

void aoe_value_grid_set_cap_ror(AoeValueGridRor *g, uint8_t cap) {
    if (cap) g->cap = cap;
}

static int range_ok(const AoeValueGridRor *g, const int32_t from[2],
                    const int32_t to[2]) {
    return inside(g, from[0], from[1]) && inside(g, to[0], to[1]);
}

/* Is neighbour `dir` looked at, and is it off the grid and counted? */
static int looked_at(int dir, int32_t diagonals) {
    return diagonals || (dir & 1);
}

static int off_counted(int dir, int32_t off_edge, int32_t off_corner) {
    return (off_edge == 1 && (dir & 1)) || (off_corner == 1 && !(dir & 1));
}

int32_t aoe_value_grid_count_ror(AoeValueGridRor *g, const int32_t from[2],
                                 const int32_t to[2], int32_t threshold,
                                 int32_t off_edge, int32_t off_corner,
                                 int32_t diagonals) {
    int32_t x, y, nx, ny;
    int dir;
    g->count = 0;
    if (!range_ok(g, from, to)) return 0;
    for (x = from[0]; x <= to[0]; ++x)
        for (y = from[1]; y <= to[1]; ++y) {
            if (aoe_value_grid_get_ror(g, x, y) < threshold) continue;
            for (dir = 0; dir < 8; ++dir) {
                if (!looked_at(dir, diagonals)) continue;
                nx = x + ring_dx[dir];
                ny = y + ring_dy[dir];
                if (!inside(g, nx, ny)) {
                    if (off_counted(dir, off_edge, off_corner)) ++g->count;
                } else if (aoe_value_grid_get_ror(g, nx, ny) >= threshold) {
                    ++g->count;
                }
            }
        }
    return g->count;
}

int32_t aoe_value_grid_score_ror(const AoeValueGridRor *g,
                                 const int32_t from[2], const int32_t to[2],
                                 uint8_t threshold, uint8_t bonus,
                                 int32_t off_edge, int32_t off_corner,
                                 int32_t diagonals) {
    int32_t score = g->count, x, y, nx, ny;
    uint8_t cell;
    int dir;
    if (!range_ok(g, from, to)) return 0;
    for (x = from[0]; x <= to[0]; ++x)
        for (y = from[1]; y <= to[1]; ++y) {
            cell = aoe_value_grid_get_ror(g, x, y);
            if ((uint32_t)cell + bonus < threshold) continue;
            if (cell >= threshold)
                for (dir = 0; dir < 8; ++dir) {
                    if (!looked_at(dir, diagonals)) continue;
                    nx = x + ring_dx[dir];
                    ny = y + ring_dy[dir];
                    if (!inside(g, nx, ny) &&
                        off_counted(dir, off_edge, off_corner))
                        --score;
                    if (score < 1) break;
                }
            for (dir = 0; dir < 8; ++dir) {
                if (!looked_at(dir, diagonals)) continue;
                nx = x + ring_dx[dir];
                ny = y + ring_dy[dir];
                if (!inside(g, nx, ny)) {
                    if (off_counted(dir, off_edge, off_corner)) --score;
                } else if (aoe_value_grid_get_ror(g, nx, ny) >= threshold) {
                    score += 2;
                }
            }
        }
    return score;
}
