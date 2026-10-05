#include "random_map.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

static uint8_t terrain_at(AoeRmModuleRor *m, int32_t x, int32_t y) {
    return *aoe_rm_tile_ror(m, x, y) & 0x1f;
}

static uint8_t elevation_at(AoeRmModuleRor *m, int32_t x, int32_t y) {
    return *aoe_rm_tile_ror(m, x, y) >> 5;
}

static void set_elevation(AoeRmModuleRor *m, int32_t x, int32_t y, uint8_t h) {
    uint8_t *t = aoe_rm_tile_ror(m, x, y);
    *t = (uint8_t)((*t & 0x1f) | (h << 5));
}

static void destroy_module(AoeRmModuleRor *m) { aoe_rm_module_release_ror(m); }

/* ---- Path module ---- */

static int path_run(AoeRmModuleRor *m) {
    AoeRmPathModuleRor *p = (AoeRmPathModuleRor *)m;
    int32_t i;
    aoe_rm_path_setup_ror(p);
    (void)aoe_rng_next(m->rng);
    for (i = 0; i < p->count; ++i) aoe_rm_path_walk_ror(p);
    return 1;
}

static const AoeRmModuleVtblRor path_vt = {path_run, destroy_module};

AoeRmPathModuleRor *aoe_rm_path_module_create_ror(AoeRmControllerRor *c,
                                                   int32_t count,
                                                   int32_t length,
                                                   int32_t variance) {
    AoeRmPathModuleRor *p = calloc(1, sizeof(*p));
    int32_t i;
    if (!p) return NULL;
    aoe_rm_module_init_ror(&p->base, &path_vt, c->base.map, &c->base, 1, NULL);
    p->count = count;
    p->f34 = 2;
    p->length = length;
    p->variance = variance;
    p->water_clear = 2;
    p->path_clear = 2;
    p->zone_count = c->terrain_zone_count;
    for (i = 0; i < c->terrain_zone_count && i < AOE_RM_SLOTS; ++i) {
        p->zone_x[i] = c->terrain_zones[i].x;
        p->zone_y[i] = c->terrain_zones[i].y;
        p->zone_r[i] = c->terrain_zones[i].radius;
    }
    p->base.priority = 1.75f;
    return p;
}

void aoe_rm_path_clear_ror(AoeRmPathModuleRor *p, int32_t x, int32_t y,
                           int32_t r) {
    AoeRmModuleRor *m = &p->base;
    int32_t bw1 = m->width / 3 - 1, bh1 = m->height / 3 - 1;
    int32_t x0 = x - r, y0 = y - r, x1 = x + r, y1 = r + y, bx, by;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > bw1) x1 = bw1;
    if (x1 > bh1) y1 = bh1;
    for (by = y0; by <= y1; ++by) {
        if (by >= m->height) break; /* past the grid rows (memory only) */
        for (bx = x0; bx <= x1; ++bx)
            if (m->grid_rows[by][bx]) {
                m->grid_rows[by][bx] = 0;
                aoe_rm_node_unlink_ror(&m->node_rows[by][bx]);
            }
    }
}

void aoe_rm_path_setup_ror(AoeRmPathModuleRor *p) {
    AoeRmModuleRor *m = &p->base;
    int32_t bw = m->width / 3, bh = m->height / 3, bx, by, i;
    aoe_rm_nodes_unlink_all_ror(m);
    aoe_rm_list_init_ror(&p->open);
    memset(m->grid, 1, (size_t)m->width * (size_t)m->height);
    for (by = 0; by < bh; ++by)
        for (bx = 0; bx < bw; ++bx) {
            int uniform = 1, water = 0, ty, tx;
            int32_t level = -1;
            for (ty = 0; ty < 3; ++ty)
                for (tx = 0; tx < 3; ++tx) {
                    uint8_t b = *aoe_rm_tile_ror(m, bx * 3 + tx, by * 3 + ty);
                    uint8_t t = b & 0x1f;
                    if (t == 0 || t == 6) {
                        if (level < 0) level = b >> 5;
                        else if (level != (b >> 5)) uniform = 0;
                    } else {
                        uniform = 0;
                        if (t == 1 || t == 4 || t == 0x16) water = 1;
                    }
                }
            if (uniform) {
                uint8_t *cell = &m->grid_rows[by][bx];
                if (*cell) {
                    *cell = (uint8_t)(level + 1);
                    aoe_rm_list_push_ror(m, &p->open, bx, by, 0, 0.0f);
                }
            } else {
                if (water) aoe_rm_path_clear_ror(p, bx, by, p->water_clear);
                m->grid_rows[by][bx] = 0;
            }
        }
    for (i = 0; i < p->zone_count && i < AOE_RM_SLOTS; ++i)
        aoe_rm_path_clear_ror(p, p->zone_x[i] / 3, p->zone_y[i] / 3,
                              p->zone_r[i] / 3 + 2);
    for (by = 0; by < bh; ++by)
        for (bx = 0; bx < bw; ++bx) {
            uint8_t *cell = &m->grid_rows[by][bx];
            if (*cell && (by < 1 || !m->grid_rows[by - 1][bx]) &&
                (bx < 1 || !cell[-1]) &&
                (bh - 1 <= by || !m->grid_rows[by + 1][bx]) &&
                (bw - 1 <= bx || !cell[1])) {
                *cell = 0;
                aoe_rm_node_unlink_ror(&m->node_rows[by][bx]);
            }
        }
    aoe_rm_list_shuffle_ror(m, &p->open);
}

int aoe_rm_path_step_ror(AoeRmPathModuleRor *p, int32_t dir, int32_t *x,
                         int32_t *y, uint32_t value) {
    AoeRmModuleRor *m = &p->base;
    int32_t nx = *x, ny = *y;
    if (dir > 3) dir -= (int32_t)(((uint32_t)dir - 1u) / 3u) * 3;
    if (dir < 0) dir += (int32_t)((2u - (uint32_t)dir) / 3u) * 3;
    switch (dir) {
    case 0: if (ny < 1) return 0; --ny; break;
    case 1: if (m->width / 3 - 1 <= nx) return 0; ++nx; break;
    case 2: if (m->height / 3 - 1 <= ny) return 0; ++ny; break;
    case 3: if (nx < 1) return 0; --nx; break;
    default: break;
    }
    if (m->grid_rows[ny][nx] != value) return 0;
    *x = nx;
    *y = ny;
    return 1;
}

void aoe_rm_path_walk_ror(AoeRmPathModuleRor *p) {
    AoeRmModuleRor *m = &p->base;
    AoeRmNodeRor path;
    int32_t var = p->variance, len, x, y, dir, k, px = -1, py = -1;
    uint32_t value, dummy;
    int first = 1;
    len = aoe_rm_rand_scaled_ror(m, var * 2) + p->length - var;
    if (!p->open.next) return;
    if (len < 3) return;
    aoe_rm_list_init_ror(&path);
    do {
        aoe_rm_list_pop_ror(&p->open, &x, &y, &dummy);
        if (m->grid_rows[y][x]) break;
        if (!p->open.next) return;
    } while (1);
    value = m->grid_rows[y][x];
    aoe_rm_list_push_ror(m, &path, x, y, 0, 0.0f);
    m->grid_rows[y][x] = 0;
    dir = aoe_rm_rand_scaled_ror(m, 4);
    for (k = 0; k < len; ++k) {
        int32_t r = aoe_rm_rand_scaled_ror(m, 100);
        int stuck;
        if (r < 18) dir = dir < 1 ? 3 : dir - 1;
        else if (r < 36) dir = dir < 3 ? dir + 1 : 0;
        stuck = !aoe_rm_path_step_ror(p, dir, &x, &y, value) &&
                !aoe_rm_path_step_ror(p, dir + 1, &x, &y, value) &&
                !aoe_rm_path_step_ror(p, dir - 1, &x, &y, value) &&
                (!first || !aoe_rm_path_step_ror(p, dir - 2, &x, &y, value));
        first = 0;
        if (stuck) break;
        aoe_rm_list_push_ror(m, &path, x, y, 0, 0.0f);
        m->grid_rows[y][x] = 0;
    }
    while (path.next) {
        aoe_rm_list_pop_ror(&path, &x, &y, &dummy);
        if (px == -1) {
            px = x;
            py = y;
        }
        if (m->map->paint_line)
            m->map->paint_line(m->map->context, px * 3 + 1, py * 3 + 1,
                               x * 3 + 1, y * 3 + 1, 1, 0x10);
        if (m->map->vt20)
            m->map->vt20(m->map->context, px * 3 + 1, py * 3 + 1, x * 3 + 1,
                         y * 3 + 1, 0, 0);
        px = x;
        py = y;
        aoe_rm_path_clear_ror(p, x, y, p->path_clear);
    }
}

/* ---- Shallows module ---- */

static int shallows_run(AoeRmModuleRor *m) {
    AoeRmShallowsModuleRor *s = (AoeRmShallowsModuleRor *)m;
    int32_t i, j, n, a, b, round;
    aoe_rm_nodes_unlink_all_ror(m);
    for (i = 0; i < s->count; ++i)
        for (j = i + 1; j < s->count; ++j) aoe_rm_shallows_connect_ror(s, i, j);
    n = s->count;
    s->count = n + 2;
    a = n + 1;
    b = n;
    for (round = 0; round < 2; ++round) {
        int32_t k, slots[2] = {a, b};
        for (k = 0; k < 2; ++k) {
            int32_t x, y;
            uint8_t t;
            do {
                x = aoe_rm_rand_scaled_ror(m, m->width - 1);
                y = aoe_rm_rand_scaled_ror(m, m->height - 1);
                t = terrain_at(m, x, y);
            } while (t == 1 || t == 0x16 || t == 4);
            if (slots[k] < AOE_RM_SLOTS) {
                s->xy[slots[k]][0] = x;
                s->xy[slots[k]][1] = y;
            }
        }
        if (a < AOE_RM_SLOTS) aoe_rm_shallows_connect_ror(s, a, b);
    }
    return 1;
}

static const AoeRmModuleVtblRor shallows_vt = {shallows_run, destroy_module};

int aoe_rm_shallows_create_ror(AoeRmControllerRor *c) {
    AoeRmShallowsModuleRor *s;
    int32_t i;
    if (!c) return 0;
    s = calloc(1, sizeof(*s));
    if (!s) return 0;
    aoe_rm_module_init_ror(&s->base, &shallows_vt, c->base.map, &c->base, 1, NULL);
    s->base.priority = 1.1f;
    for (i = 0; i < c->land_count && i < AOE_RM_SLOTS; ++i) {
        s->xy[i][0] = c->lands[i].x;
        s->xy[i][1] = c->lands[i].y;
    }
    s->count = c->land_count;
    return 1;
}

static int is_open_water(uint8_t t) { return t == 1 || t == 0x16; }

void aoe_rm_shallows_connect_ror(AoeRmShallowsModuleRor *s, int32_t a,
                                 int32_t b) {
    AoeRmModuleRor *m = &s->base;
    AoeRmNodeRor marks;
    float costs[40];
    int32_t gx = s->xy[b][0], gy = s->xy[b][1], x, y, w = m->width, h = m->height;
    int done = 0;
    uint32_t dummy;
    memset(costs, 0, sizeof costs);
    aoe_rm_list_init_ror(&marks);
    costs[0] = 1.0f;
    costs[2] = 1.0f;
    costs[1] = 7.0f;
    costs[0x16] = 14.0f;
    costs[4] = 2.0f;
    /* The search's result is not checked: a failed search leaves the goal
       cell at 2 and the trace below stops at once. */
    (void)aoe_rm_find_path_ror(m, s->xy[a][0], s->xy[a][1], &gx, &gy, costs);
    if (gx != s->xy[b][0] || gy != s->xy[b][1] || gx < 0 || w <= gx || gy < 0 || h <= gy)
        return;
    x = gx;
    y = gy;
    for (;;) {
        int32_t px = x, py = y;
        switch (m->grid_rows[y][x]) {
        case 6: py = y + 1; break;
        case 7: py = y - 1; break;
        case 8: py = y + 1; px = x + 1; break;
        case 4: px = x + 1; break;
        case 9: py = y - 1; px = x + 1; break;
        case 10: py = y + 1; px = x - 1; break;
        case 5: px = x - 1; break;
        case 11: py = y - 1; px = x - 1; break;
        default: done = 1; break;
        }
        if (px < 0 || w <= px || py < 0 || h <= py || done) {
            int32_t tx, ty;
            while (aoe_rm_list_pop_ror(&marks, &tx, &ty, &dummy)) {
                uint8_t *t = aoe_rm_tile_ror(m, tx, ty);
                *t = (uint8_t)((*t & 0xe4) | 4);
            }
            aoe_rm_list_detach_ror(&marks);
            return;
        }
        if (is_open_water(terrain_at(m, px, py))) {
            int32_t x0, y0, x1, y1, ix, iy;
            x0 = px - aoe_rm_rand_scaled_ror(m, 2) - 1;
            if (x0 < 0) x0 = 0;
            y0 = py - aoe_rm_rand_scaled_ror(m, 2) - 1;
            if (y0 < 0) y0 = 0;
            x1 = aoe_rm_rand_scaled_ror(m, 2) + 1 + px;
            if (w <= x1) x1 = w - 1;
            y1 = aoe_rm_rand_scaled_ror(m, 3) + 1 + py;
            if (h <= y1) y1 = h - 1;
            for (iy = y0; iy <= y1; ++iy)
                for (ix = x0; ix <= x1; ++ix)
                    if (is_open_water(terrain_at(m, ix, iy)))
                        aoe_rm_list_push_ror(m, &marks, ix, iy, 0, 0.0f);
        } else {
            if (px > 0 && is_open_water(terrain_at(m, px - 1, py)))
                aoe_rm_list_push_ror(m, &marks, px - 1, py, 0, 0.0f);
            if (px < w - 1 && is_open_water(terrain_at(m, px + 1, py)))
                aoe_rm_list_push_ror(m, &marks, px + 1, py, 0, 0.0f);
            if (py > 0 && is_open_water(terrain_at(m, px, py - 1)))
                aoe_rm_list_push_ror(m, &marks, px, py - 1, 0, 0.0f);
            if (py < h - 1 && is_open_water(terrain_at(m, px, py + 1)))
                aoe_rm_list_push_ror(m, &marks, px, py + 1, 0, 0.0f);
        }
        m->grid_rows[y][x] = 0xff;
        x = px;
        y = py;
    }
}

/* ---- Elevation module ---- */

int aoe_rm_elev_check_ror(AoeRmModuleRor *m, int32_t x, int32_t y,
                          uint8_t terrain, uint8_t min, int32_t r) {
    int32_t x0 = x - (r & 0xff), y0 = y - (r & 0xff), x1 = x + (r & 0xff),
            y1 = (r & 0xff) + y, ix, iy;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (m->width <= x1) x1 = m->width - 1;
    if (m->height <= y1) y1 = m->height - 1;
    for (iy = y0; iy <= y1; ++iy)
        for (ix = x0; ix <= x1; ++ix) {
            uint8_t b = *aoe_rm_tile_ror(m, ix, iy);
            if (terrain == 0xff) return 0;
            if ((b & 0x1f) != terrain || (b >> 5) < min) return 0;
        }
    return 1;
}

int aoe_rm_elev_count_ror(AoeRmModuleRor *m, uint8_t height, int32_t x,
                          int32_t y, int32_t r, uint8_t terrain, uint8_t min) {
    int32_t x0, y0, x1, y1, ix, iy;
    uint8_t count = 0;
    if (r > 0) {
        x0 = x - r; y0 = y - r; x1 = x + r; y1 = y + r;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (m->width <= x1) x1 = m->width - 1;
        if (m->height <= y1) y1 = m->height - 1;
        for (iy = y0; iy <= y1; ++iy)
            for (ix = x0; ix <= x1; ++ix) {
                uint8_t b = *aoe_rm_tile_ror(m, ix, iy);
                if (terrain == 0xff || (b & 0x1f) != terrain || (b >> 5) < min)
                    return 0;
            }
    }
    x0 = x - 2; y0 = y - 2; x1 = x + 2; y1 = y + 2;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (m->width <= x1) x1 = m->width - 1;
    if (m->height <= y1) y1 = m->height - 1;
    for (iy = y0; iy <= y1; ++iy)
        for (ix = x0; ix <= x1; ++ix)
            if ((*aoe_rm_tile_ror(m, ix, iy) >> 5) == height) ++count;
    return count;
}

void aoe_rm_zone_mask_ror(AoeRmModuleRor *m, const AoeRmZoneRor *zones,
                          int32_t count) {
    int32_t x, y, i;
    memset(m->grid, 0, (size_t)m->width * (size_t)m->height);
    for (y = 0; y < m->height; ++y)
        for (x = 0; x < m->width; ++x) {
            int32_t sum = 0;
            if (count >= 1) {
                for (i = 0; i < count; ++i) {
                    int32_t dx = x - zones[i].x, dy = y - zones[i].y;
                    int32_t d = aoe_ftol32(sqrt((double)(dy * dy + dx * dx)));
                    int32_t left = zones[i].radius - d;
                    if (left > 0) sum += zones[i].weight * left;
                }
                if (sum >= 0x65) {
                    m->grid_rows[y][x] = 0x65;
                    continue;
                }
            }
            m->grid_rows[y][x] = (uint8_t)sum;
        }
}

/* 0x0046f4d0: the node chain is built through every grid node but the
   head is not attached to it (node 0's prev points at the head); random
   nodes are then linked in front. Picking node 0 attaches the rest of the
   chain through its unlink. */
static void elev_seed(AoeRmModuleRor *m, AoeRmNodeRor *head) {
    int32_t n = m->width * m->height, i, picks;
    AoeRmNodeRor *nodes = m->nodes;
    nodes[0].prev = head;
    nodes[0].next = n > 1 ? &nodes[1] : NULL;
    nodes[n - 1].next = NULL;
    nodes[n - 1].prev = n > 1 ? &nodes[n - 2] : head;
    for (i = 1; i < n - 1; ++i) {
        nodes[i].next = &nodes[i + 1];
        nodes[i].prev = &nodes[i - 1];
    }
    picks = n / 16;
    for (i = 0; i < picks; ++i) {
        int32_t x = aoe_rm_rand_scaled_ror(m, m->width - 1);
        int32_t y = aoe_rm_rand_scaled_ror(m, m->height - 1);
        aoe_rm_node_link_ror(head, &m->node_rows[y][x]);
    }
}

static void elev_unlink_band(AoeRmModuleRor *m, int32_t x, int32_t y, int32_t r) {
    int32_t x0 = x - r, y0 = y - r, x1 = x + r, iy, ix;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (m->width <= x1) x1 = m->width - 1;
    for (iy = y0; iy <= x1; ++iy) {
        if (iy >= m->height) break; /* only on non-square maps */
        for (ix = x0; ix <= x1; ++ix) aoe_rm_node_unlink_ror(&m->node_rows[iy][ix]);
    }
}

int aoe_rm_elev_grow_ror(AoeRmElevationModuleRor *e, const AoeRmLayerRor *l) {
    AoeRmModuleRor *m = &e->base;
    AoeRmNodeRor lists[AOE_RM_SLOTS], seed;
    int32_t max_x = m->width - 1, max_y = m->height - 1, clumps = l->f08;
    int32_t radius, placed = 0, k, i;
    uint8_t height = (uint8_t)l->height, terrain = (uint8_t)l->f14,
            min = (uint8_t)l->f10;
    int32_t r = l->f0c;
    uint32_t dummy;
    int done;
    if (clumps > AOE_RM_SLOTS) clumps = AOE_RM_SLOTS;
    for (i = 0; i < clumps; ++i) aoe_rm_list_init_ror(&lists[i]);
    memset(&seed, 0, sizeof seed);
    elev_seed(m, &seed);
    radius = aoe_ftol32(sqrt((double)l->tiles / (double)l->f08)) / 2;
    if (radius < 2) radius = 2;
    k = 0;
    while (k < clumps) {
        int32_t x, y;
        if (!aoe_rm_list_pop_ror(&seed, &x, &y, &dummy)) break;
        if (m->grid_rows[y][x] == 0 && aoe_rm_elev_check_ror(m, x, y, terrain, min, r)) {
            AoeRmNodeRor *lp = &lists[k];
            elev_unlink_band(m, x, y, radius);
            set_elevation(m, x, y, height);
            if (x > 0) aoe_rm_list_push_ror(m, lp, x - 1, y, 0, 0.0f);
            if (y > 0) aoe_rm_list_push_ror(m, lp, x, y - 1, 0, 0.0f);
            if (x < m->width - 1) aoe_rm_list_push_ror(m, lp, x + 1, y, 0, 0.0f);
            if (y < m->height - 1) aoe_rm_list_push_ror(m, lp, x, y + 1, 0, 0.0f);
            ++placed;
            ++k;
        }
    }
    do {
        done = 1;
        if (clumps < 1) return 1;
        for (i = 0; i < clumps; ++i) {
            int32_t x, y;
            if (placed < l->tiles && aoe_rm_list_pop_ror(&lists[i], &x, &y, &dummy)) {
                uint8_t *cell = &m->grid_rows[y][x];
                done = 0;
                if (aoe_rm_rand_scaled_ror(m, 100) < (int32_t)*cell) {
                    *cell = 0x65;
                } else {
                    int count = aoe_rm_elev_count_ror(m, height, x, y, r, terrain, min);
                    if (elevation_at(m, x, y) == min && count) {
                        float prio = (float)(250 - count * 15);
                        set_elevation(m, x, y, height);
                        if (x > 0 && elevation_at(m, x - 1, y) == min)
                            aoe_rm_list_push_ror(m, &lists[i], x - 1, y, 0,
                                (float)((double)aoe_rm_rand_scaled_ror(m, 100) + prio));
                        if (x < max_x && elevation_at(m, x + 1, y) == min)
                            aoe_rm_list_push_ror(m, &lists[i], x + 1, y, 0,
                                (float)((double)aoe_rm_rand_scaled_ror(m, 100) + prio));
                        if (y > 0 && elevation_at(m, x, y - 1) == min)
                            aoe_rm_list_push_ror(m, &lists[i], x, y - 1, 0,
                                (float)((double)aoe_rm_rand_scaled_ror(m, 100) + prio));
                        if (y < max_y && elevation_at(m, x, y + 1) == min)
                            aoe_rm_list_push_ror(m, &lists[i], x, y + 1, 0,
                                (float)((double)aoe_rm_rand_scaled_ror(m, 100) + prio));
                        ++placed;
                    }
                }
            }
        }
    } while (!done);
    for (i = 0; i < clumps; ++i) aoe_rm_list_detach_ror(&lists[i]);
    return 1;
}

static int elevation_run(AoeRmModuleRor *m) {
    AoeRmElevationModuleRor *e = (AoeRmElevationModuleRor *)m;
    int32_t i, x, y;
    aoe_rm_nodes_unlink_all_ror(m);
    if (e->layer_count > 0)
        for (y = 0; y < m->height; ++y)
            for (x = 0; x < m->width; ++x)
                set_elevation(m, x, y, (uint8_t)e->layers[0].f10);
    aoe_rm_zone_mask_ror(m, e->zones, e->zone_count);
    for (i = 0; i < e->layer_count && i < AOE_RM_SLOTS; ++i) {
        aoe_rm_elev_grow_ror(e, &e->layers[i]);
        if (m->map->fix_elevation)
            m->map->fix_elevation(m->map->context, 0, 0, m->width - 1,
                                  m->height - 1, 8);
    }
    return 1;
}

static const AoeRmModuleVtblRor elevation_vt = {elevation_run, destroy_module};

AoeRmElevationModuleRor *aoe_rm_elevation_module_create_ror(AoeRmControllerRor *c) {
    AoeRmElevationModuleRor *e = calloc(1, sizeof(*e));
    if (!e) return NULL;
    aoe_rm_module_init_ror(&e->base, &elevation_vt, c->base.map, &c->base, 1, NULL);
    memcpy(e->layers, c->layers, sizeof(e->layers));
    e->layer_count = c->layer_count;
    memcpy(e->zones, c->elevation_zones, sizeof(e->zones));
    e->zone_count = c->elevation_zone_count;
    e->base.priority = 1.5f;
    return e;
}

/* ---- Land module ---- */

int aoe_rm_land_score_ror(AoeRmLandModuleRor *l, int32_t land, int32_t x,
                          int32_t y) {
    AoeRmModuleRor *m = &l->base;
    const AoeRmLandWorkRor *w = &l->lands[land & 0xff];
    int32_t r = w->f14, ix, iy, count = 0;
    uint8_t id = w->id;
    /* The original also computes max(2, ftol(r * K)) and drops it. */
    if (m->grid_rows[y][x] != (uint32_t)l->land_count) return 0;
    for (iy = y - r; iy <= r + y; ++iy) {
        if (iy < 0 || iy >= m->height) continue; /* the +3 terms never apply */
        for (ix = x - r; ix <= r + x; ++ix) {
            uint8_t c;
            if (ix < 0 || ix >= m->width) continue;
            c = m->grid_rows[iy][ix];
            if (c == id) {
                if (iy >= y - 2 && iy <= y + 2 && ix >= x - 2 && ix <= x + 2) ++count;
            } else if ((int32_t)c < l->land_count) {
                return 0;
            }
        }
    }
    return count & 0xff;
}

int aoe_rm_land_penalty_ror(AoeRmLandModuleRor *l, int32_t x, int32_t y,
                            int32_t land) {
    const AoeRmLandWorkRor *w = &l->lands[land];
    int32_t x0 = w->bounds[0], y0 = w->bounds[1], x1 = w->bounds[2],
            y1 = w->bounds[3], a, b, mx, m2, ey, fy, v;
    if (!w->avoid) return 0;
    a = x0 - x;
    b = x - x1;
    mx = a > b ? a : b;
    a += x0;
    b += l->base.width - x1;
    m2 = a > b ? a : b;
    if (m2 > 0) {
        ey = y0 - y + m2;
        fy = m2 - y1 + y;
    } else {
        ey = y0 - y;
        fy = y - y1;
    }
    if (ey > fy) fy = ey;
    if (mx < 0) mx = 0;
    if (fy < 0) fy = 0;
    v = (fy + mx) * w->avoid;
    return v >= 100 ? 0x65 : (v & 0xff);
}

void aoe_rm_land_grow_ror(AoeRmLandModuleRor *l) {
    AoeRmModuleRor *m = &l->base;
    AoeRmNodeRor lists[AOE_RM_SLOTS];
    int32_t sizes[256], i, max_x = m->width - 1, max_y = m->height - 1;
    uint8_t free_id = (uint8_t)l->land_count;
    uint32_t dummy;
    int done;
    memset(sizes, 0, sizeof sizes);
    for (i = 0; i < l->land_count && i < AOE_RM_SLOTS; ++i) {
        AoeRmLandWorkRor *w = &l->lands[i];
        int32_t x0 = w->x - w->radius, y0 = w->y - w->radius,
                x1 = w->x + w->radius, y1 = w->y + w->radius, ix, iy;
        uint32_t value = aoe_rm_float_bits((float)i);
        aoe_rm_list_init_ror(&lists[i]);
        sizes[i] = 0;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 >= m->width) x1 = m->width - 1;
        if (y1 >= m->height) y1 = m->height - 1;
        if (m->map->set_terrain)
            m->map->set_terrain(m->map->context, 0, 0, x0, y0, x1, y1,
                                (uint8_t)w->terrain, 1, 0);
        sizes[w->id] = (y1 - y0 + 1) * (x1 - x0 + 1);
        for (iy = y0; iy <= y1; ++iy)
            for (ix = x0; ix <= x1; ++ix) m->grid_rows[iy][ix] = w->id;
        if (x0 > 0)
            for (iy = y0; iy <= y1; ++iy)
                aoe_rm_list_push_ror(m, &lists[i], x0 - 1, iy, value, 0.0f);
        if (y0 > 0)
            for (ix = x0; ix <= x1; ++ix)
                aoe_rm_list_push_ror(m, &lists[i], ix, y0 - 1, value, 0.0f);
        if (x1 < m->width - 1)
            for (iy = y0; iy <= y1; ++iy)
                aoe_rm_list_push_ror(m, &lists[i], x1 + 1, iy, value, 0.0f);
        if (y1 < m->height - 1)
            for (ix = x0; ix <= x1; ++ix)
                aoe_rm_list_push_ror(m, &lists[i], ix, y1 + 1, value, 0.0f);
    }
    do {
        done = 1;
        for (i = 0; i < l->land_count && i < AOE_RM_SLOTS; ++i) {
            AoeRmLandWorkRor *w = &l->lands[i];
            int32_t x, y, pen, cnt;
            if (sizes[i] >= w->area) continue;
            if (!aoe_rm_list_pop_ror(&lists[i], &x, &y, &dummy)) continue;
            done = 0;
            pen = aoe_rm_land_penalty_ror(l, x, y, i);
            if (pen > aoe_rm_rand_scaled_ror(m, 100)) {
                m->grid_rows[y][x] = 0xff;
                continue;
            }
            cnt = aoe_rm_land_score_ror(l, i, x, y);
            if (m->grid_rows[y][x] != free_id || cnt <= 0) continue;
            {
                uint8_t *t = aoe_rm_tile_ror(m, x, y);
                *t = (uint8_t)(((*t ^ (uint8_t)w->terrain) & 0x1f) ^ *t);
            }
            m->grid_rows[y][x] = w->id;
#define LAND_PUSH(nx, ny)                                                          \
    aoe_rm_list_push_ror(m, &lists[i], nx, ny, 0,                                  \
        (float)(aoe_rm_rand_scaled_ror(m, 100) - cnt * w->weight + 0xfa))
            if (x > 0 && m->grid_rows[y][x - 1] == free_id) LAND_PUSH(x - 1, y);
            if (x < max_x && m->grid_rows[y][x + 1] == free_id) LAND_PUSH(x + 1, y);
            if (y > 0 && m->grid_rows[y - 1][x] == free_id) LAND_PUSH(x, y - 1);
            if (y < max_y && m->grid_rows[y + 1][x] == free_id) LAND_PUSH(x, y + 1);
#undef LAND_PUSH
            ++sizes[i];
        }
    } while (!done);
    for (i = 0; i < l->land_count && i < AOE_RM_SLOTS; ++i) {
        AoeRmLandWorkRor *w = &l->lands[i];
        uint8_t id = w->id;
        int32_t x, y;
        while (aoe_rm_list_pop_ror(&lists[i], &x, &y, &dummy)) {
            if ((x >= 1 && m->grid_rows[y][x - 1] == id && x < max_x &&
                 m->grid_rows[y][x + 1] == id) ||
                (y >= 1 && m->grid_rows[y - 1][x] == id && y < max_y &&
                 m->grid_rows[y + 1][x] == id)) {
                uint8_t *t = aoe_rm_tile_ror(m, x, y);
                *t = (uint8_t)(((*t ^ (uint8_t)w->terrain) & 0x1f) ^ *t);
            }
        }
    }
    for (i = 0; i < l->land_count && i < AOE_RM_SLOTS; ++i) aoe_rm_list_detach_ror(&lists[i]);
}

static int land_run(AoeRmModuleRor *m) {
    AoeRmLandModuleRor *l = (AoeRmLandModuleRor *)m;
    aoe_rm_nodes_unlink_all_ror(m);
    memset(m->grid, (uint8_t)l->land_count, (size_t)m->width * (size_t)m->height);
    if (m->map->set_terrain)
        m->map->set_terrain(m->map->context, 0, 0, 0, 0, m->width - 1,
                            m->height - 1, l->base_terrain, 1, 0);
    aoe_rm_land_grow_ror(l);
    if (m->map->fix_terrain)
        m->map->fix_terrain(m->map->context, 0, 0, m->width, m->height,
                            l->base_terrain);
    return 1;
}

static const AoeRmModuleVtblRor land_vt = {land_run, destroy_module};

AoeRmLandModuleRor *aoe_rm_land_module_create_ror(AoeRmControllerRor *c) {
    AoeRmLandModuleRor *l = calloc(1, sizeof(*l));
    if (!l) return NULL;
    aoe_rm_module_init_ror(&l->base, &land_vt, c->base.map, &c->base, 1, NULL);
    l->base.priority = 1.0f;
    memcpy(l->lands, c->lands, sizeof(l->lands));
    l->land_count = c->land_count;
    l->base_terrain = c->f145c;
    memcpy(l->bounds, c->bounds, sizeof(l->bounds));
    l->avoid = c->avoid;
    return l;
}

/* ---- Terrain module ---- */

int aoe_rm_is_water_ror(uint8_t terrain) {
    return terrain == 1 || terrain == 4 || terrain == 0x16;
}

void aoe_rm_beaches_ror(AoeRmModuleRor *m) {
    int32_t x, y, max_x = m->width - 1, max_y = m->height - 1;
    for (y = 0; y < m->height; ++y)
        for (x = 0; x < m->width; ++x) {
            int near = 0;
            if (aoe_rm_is_water_ror(terrain_at(m, x, y))) continue;
            if ((y > 0 && aoe_rm_is_water_ror(terrain_at(m, x, y - 1))) ||
                (y < max_y && aoe_rm_is_water_ror(terrain_at(m, x, y + 1))))
                near = 1;
            if (x > 0 &&
                (aoe_rm_is_water_ror(terrain_at(m, x - 1, y)) ||
                 (y > 0 && aoe_rm_is_water_ror(terrain_at(m, x - 1, y - 1))) ||
                 (y < max_y && aoe_rm_is_water_ror(terrain_at(m, x - 1, y + 1)))))
                near = 1;
            if ((x < max_x &&
                 (aoe_rm_is_water_ror(terrain_at(m, x + 1, y)) ||
                  (y > 0 && aoe_rm_is_water_ror(terrain_at(m, x + 1, y - 1))) ||
                  (y < max_y && aoe_rm_is_water_ror(terrain_at(m, x + 1, y + 1))))) ||
                near) {
                uint8_t *t = aoe_rm_tile_ror(m, x, y);
                *t = (uint8_t)((*t & 0xe2) | 2);
            }
        }
}

int aoe_rm_terrain_check_ror(AoeRmModuleRor *m, uint8_t terrain, int32_t x,
                             int32_t y, int32_t r, uint8_t base) {
    int32_t x0, y0, x1, y1, ix, iy;
    uint8_t count = 1;
    if (r > 0) {
        uint8_t alias = m->map->terrain_alias
                            ? m->map->terrain_alias(m->map->context, base)
                            : base;
        x0 = x - r; y0 = y - r; x1 = x + r; y1 = y + r;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (m->width <= x1) x1 = m->width - 1;
        if (m->height <= y1) y1 = m->height - 1;
        for (iy = y0; iy <= y1; ++iy)
            for (ix = x0; ix <= x1; ++ix) {
                uint8_t t = terrain_at(m, ix, iy);
                if (t != base && t != alias && t != terrain) return 0;
            }
    }
    x0 = x - 2; y0 = y - 2; x1 = x + 2; y1 = y + 2;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (m->width <= x1) x1 = m->width - 1;
    if (m->height <= y1) y1 = m->height - 1;
    for (iy = y0; iy <= y1; ++iy)
        for (ix = x0; ix <= x1; ++ix)
            if (terrain_at(m, ix, iy) == terrain) ++count;
    return count;
}

static void terrain_seed(AoeRmModuleRor *m, AoeRmNodeRor *head) {
    int32_t n = m->width * m->height, i, picks;
    AoeRmNodeRor *nodes = m->nodes;
    nodes[0].prev = head;
    nodes[0].next = n > 1 ? &nodes[1] : NULL;
    head->next = &nodes[0];
    nodes[n - 1].next = NULL;
    if (n > 1) nodes[n - 1].prev = &nodes[n - 2];
    for (i = 1; i < n - 1; ++i) {
        nodes[i].next = &nodes[i + 1];
        nodes[i].prev = &nodes[i - 1];
    }
    picks = n / 8;
    for (i = 0; i < picks; ++i) {
        int32_t x = aoe_rm_rand_scaled_ror(m, m->width - 1);
        int32_t y = aoe_rm_rand_scaled_ror(m, m->height - 1);
        aoe_rm_node_link_ror(head, &m->node_rows[y][x]);
    }
}

static void terrain_unlink_square(AoeRmModuleRor *m, int32_t x, int32_t y, int32_t r) {
    int32_t x0 = x - r, y0 = y - r, x1 = x + r, y1 = r + y, ix, iy;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (m->width <= x1) x1 = m->width - 1;
    if (m->height <= y1) y1 = m->height - 1;
    for (iy = y0; iy <= y1; ++iy)
        for (ix = x0; ix <= x1; ++ix) aoe_rm_node_unlink_ror(&m->node_rows[iy][ix]);
}

static uint8_t zone_of(AoeRmTerrainModuleRor *t, int32_t x, int32_t y) {
    AoeRmMapRor *map = t->base.map;
    return map->zone_at ? map->zone_at(map->context, t->zone_map, x, y) : 0;
}

int aoe_rm_terrain_grow_ror(AoeRmTerrainModuleRor *t,
                            const AoeRmTerrainWorkRor *e) {
    AoeRmModuleRor *m = &t->base;
    AoeRmNodeRor lists[AOE_RM_SLOTS], seed;
    char zone_id[100], zone_done[100];
    int32_t max_x = m->width - 1, max_y = m->height - 1, placed = 0, k = 0,
            zones_done = 0, radius, i, j;
    uint8_t terrain = (uint8_t)e->terrain, base = (uint8_t)e->f14;
    uint32_t dummy;
    int done;
    if (e->flag == 2)
        for (i = 0; i < t->zone_count && i < 100; ++i) {
            zone_id[i] = (char)zone_of(t, t->zones[i].x, t->zones[i].y);
            zone_done[i] = 0;
        }
    for (i = 0; i < AOE_RM_SLOTS; ++i) aoe_rm_list_init_ror(&lists[i]);
    memset(&seed, 0, sizeof seed);
    terrain_seed(m, &seed);
    radius = aoe_ftol32(sqrt((double)e->tiles / (double)e->clumps)) * 2;
    if (radius < 2) radius = 2;
#define CLAIM(x, y)                                                            \
    do {                                                                       \
        AoeRmNodeRor *lp_ = &lists[k < AOE_RM_SLOTS ? k : AOE_RM_SLOTS - 1];    \
        uint8_t *tile_ = aoe_rm_tile_ror(m, x, y);                             \
        terrain_unlink_square(m, x, y, radius);                                \
        *tile_ = (uint8_t)(((*tile_ ^ terrain) & 0x1f) ^ *tile_);              \
        if (x > 0) aoe_rm_list_push_ror(m, lp_, x - 1, y, 0, 0.0f);            \
        if (y > 0) aoe_rm_list_push_ror(m, lp_, x, y - 1, 0, 0.0f);            \
        if (x < max_x) aoe_rm_list_push_ror(m, lp_, x + 1, y, 0, 0.0f);        \
        if (y < max_y) aoe_rm_list_push_ror(m, lp_, x, y + 1, 0, 0.0f);        \
        ++placed;                                                              \
        ++k;                                                                   \
    } while (0)
    while (k < e->clumps) {
        int32_t x, y;
        uint8_t z;
        if (!aoe_rm_list_pop_ror(&seed, &x, &y, &dummy)) break;
        if (terrain_at(m, x, y) != base ||
            !aoe_rm_terrain_check_ror(m, terrain, x, y, e->f10, base) ||
            (e->flag != 0 && m->grid_rows[y][x] != 0))
            continue;
        CLAIM(x, y);
        z = zone_of(t, x, y);
        if (e->flag == 2)
            for (i = 0; i < t->zone_count && i < 100; ++i)
                if (zone_id[i] == (char)z && !zone_done[i]) {
                    zone_done[i] = 1;
                    ++zones_done;
                }
    }
    if (e->flag == 2 && zones_done < t->zone_count) {
        do {
            int32_t x, y;
            uint8_t z;
            if (!aoe_rm_list_pop_ror(&seed, &x, &y, &dummy)) break;
            if (terrain_at(m, x, y) != base ||
                !aoe_rm_terrain_check_ror(m, terrain, x, y, e->f10, base) ||
                m->grid_rows[y][x] >= 0x1e)
                continue;
            z = zone_of(t, x, y);
            for (i = 0; i < t->zone_count && i < 100; ++i)
                if (zone_id[i] == (char)z && !zone_done[i]) break;
            if (i != t->zone_count) {
                ++zones_done;
                zone_done[i] = 1;
                CLAIM(x, y);
            }
        } while (zones_done < t->zone_count);
    }
#undef CLAIM
    do {
        done = 1;
        if (k < 1) break;
        for (j = 0; j < k && j < AOE_RM_SLOTS; ++j) {
            int32_t x, y, cnt;
            uint8_t *tile;
            float prio;
            if (placed >= e->tiles) continue;
            if (!aoe_rm_list_pop_ror(&lists[j], &x, &y, &dummy)) continue;
            done = 0;
            if (e->flag != 0 &&
                !((int32_t)m->grid_rows[y][x] <= aoe_rm_rand_scaled_ror(m, 100)))
                continue;
            tile = aoe_rm_tile_ror(m, x, y);
            cnt = aoe_rm_terrain_check_ror(m, terrain, x, y, e->f10, base);
            if ((*tile & 0x1f) != base || !cnt) continue;
            prio = (float)(0xfa - cnt * e->f18);
            if (e->flag != 0) prio = (float)((double)m->grid_rows[y][x] + (double)prio);
            *tile = (uint8_t)(((*tile ^ terrain) & 0x1f) ^ *tile);
#define TERRAIN_PUSH(nx, ny)                                                      \
    aoe_rm_list_push_ror(m, &lists[j], nx, ny, 0,                                 \
        (float)((double)aoe_rm_rand_scaled_ror(m, 100) + (double)prio))
            if (x > 0 && terrain_at(m, x - 1, y) == base) TERRAIN_PUSH(x - 1, y);
            if (x < max_x && terrain_at(m, x + 1, y) == base) TERRAIN_PUSH(x + 1, y);
            if (y > 0 && terrain_at(m, x, y - 1) == base) TERRAIN_PUSH(x, y - 1);
            if (y < max_y && terrain_at(m, x, y + 1) == base) TERRAIN_PUSH(x, y + 1);
#undef TERRAIN_PUSH
            ++placed;
        }
    } while (!done);
    for (j = 0; j < e->clumps && j < AOE_RM_SLOTS; ++j) {
        int32_t x, y;
        while (aoe_rm_list_pop_ror(&lists[j], &x, &y, &dummy)) {
            /* The vertical half of the original test compares the row
               with the last row the wrong way round, so only the
               horizontal fill ever applies. */
            if (x > 0 && terrain_at(m, x - 1, y) == terrain && x < max_x &&
                terrain_at(m, x + 1, y) == terrain) {
                uint8_t *tile = aoe_rm_tile_ror(m, x, y);
                *tile = (uint8_t)(((*tile ^ terrain) & 0x1f) ^ *tile);
            }
        }
    }
    for (i = 0; i < AOE_RM_SLOTS; ++i) aoe_rm_list_detach_ror(&lists[i]);
    return 1;
}

static int terrain_run(AoeRmModuleRor *m) {
    AoeRmTerrainModuleRor *t = (AoeRmTerrainModuleRor *)m;
    float table[99];
    int32_t i;
    aoe_rm_zone_mask_ror(m, t->zones, t->zone_count);
    for (i = 0; i < 99; ++i) table[i] = (float)i;
    t->zone_map = m->map->zones ? m->map->zones(m->map->context, table, 99) : NULL;
    for (i = 0; i < t->terrain_count && i < AOE_RM_SLOTS; ++i)
        aoe_rm_terrain_grow_ror(t, &t->terrains[i]);
    aoe_rm_beaches_ror(m);
    return 1;
}

static const AoeRmModuleVtblRor terrain_vt = {terrain_run, destroy_module};

AoeRmTerrainModuleRor *aoe_rm_terrain_module_create_ror(AoeRmControllerRor *c) {
    AoeRmTerrainModuleRor *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    aoe_rm_module_init_ror(&t->base, &terrain_vt, c->base.map, &c->base, 1, NULL);
    t->base.priority = 2.0f;
    memcpy(t->terrains, c->terrains, sizeof(t->terrains));
    t->terrain_count = c->terrain_count;
    memcpy(t->zones, c->terrain_zones, sizeof(t->zones));
    t->zone_count = c->terrain_zone_count;
    return t;
}

/* ---- Object module ---- */

static int32_t player_of(int32_t f1c) { return f1c < 0 ? 0 : f1c; }

void aoe_rm_object_fill_ror(AoeRmObjectModuleRor *o, int32_t x, int32_t y,
                            int32_t r, uint8_t value) {
    AoeRmModuleRor *m = &o->base;
    int32_t x0 = x - r, y0 = y - r, x1 = x + r, y1 = y + r, ix, iy;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (m->width <= x1) x1 = m->width - 1;
    if (m->height <= y1) y1 = m->height - 1;
    for (iy = y0; iy <= y1; ++iy)
        for (ix = x0; ix <= x1; ++ix) m->grid_rows[iy][ix] = value;
}

void aoe_rm_object_gather_ror(AoeRmObjectModuleRor *o, AoeRmNodeRor *head,
                              int32_t x, int32_t y, int32_t r) {
    AoeRmModuleRor *m = &o->base;
    AoeRmNodeRor *prev = head;
    int32_t x0, y0, x1, y1, ix, iy, wx, hy, picks;
    head->next = head->prev = NULL;
    if (x < 0 || y < 0 || r < 0) {
        x0 = 0;
        y0 = 0;
        x1 = m->width - 1;
        y1 = m->height - 1;
    } else {
        x0 = x - r; y0 = y - r; x1 = x + r; y1 = y + r;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (m->width <= x1) x1 = m->width - 1;
        if (m->height <= y1) y1 = m->height - 1;
    }
    for (iy = y0; iy <= y1; ++iy)
        for (ix = x0; ix <= x1; ++ix)
            if (m->grid_rows[iy][ix]) {
                AoeRmNodeRor *n = &m->node_rows[iy][ix];
                aoe_rm_node_link_ror(prev, n);
                prev = n;
            }
    wx = x1 - x0 - 1;
    hy = y1 - y0 - 1;
    if (wx > 1 && hy > 1 && (picks = hy * wx / 4) > 0)
        while (picks-- > 0) {
            int32_t rx = aoe_rm_rand_scaled_ror(m, wx) + x0;
            int32_t ry = aoe_rm_rand_scaled_ror(m, hy) + y0;
            if (m->grid_rows[ry][rx]) aoe_rm_node_link_ror(head, &m->node_rows[ry][rx]);
        }
}

/* The tile's centre when the unit size has a fraction. */
static void spot(AoeRmObjectModuleRor *o, int32_t player, int32_t unit,
                 int32_t x, int32_t y, float *fx, float *fy) {
    float sx = 0.0f, sy = 0.0f;
    *fx = (float)x;
    *fy = (float)y;
    if (o->world->unit_size) o->world->unit_size(o->world->context, player, unit, &sx, &sy);
    if ((double)sx - (double)(int16_t)aoe_ftol32(sx) > 0.0) *fx = (float)((double)*fx + 0.5);
    if ((double)sy - (double)(int16_t)aoe_ftol32(sy) > 0.0) *fy = (float)((double)*fy + 0.5);
}

static int refused(AoeRmObjectModuleRor *o, int32_t player, int32_t unit,
                   float fx, float fy, int32_t flag) {
    return o->world->blocked &&
           o->world->blocked(o->world->context, player, unit, fx, fy, flag);
}

static void create(AoeRmObjectModuleRor *o, int32_t player, int32_t unit,
                   float fx, float fy) {
    if (o->world->create) o->world->create(o->world->context, player, unit, fx, fy);
}

static int32_t cluster_count(AoeRmObjectModuleRor *o, const AoeRmObjectWorkRor *e) {
    int32_t n = e->rec.f0c + (aoe_rm_rand_scaled_ror(&o->base, e->rec.f10 * 2) - e->rec.f10);
    return n < 1 ? 1 : n;
}

static void cluster_scatter(AoeRmObjectModuleRor *o, AoeRmObjectWorkRor *e,
                            int32_t x, int32_t y, int32_t player) {
    AoeRmModuleRor *m = &o->base;
    AoeRmNodeRor list;
    int32_t count, tx, ty;
    uint32_t dummy;
    aoe_rm_list_init_ror(&list);
    aoe_rm_object_gather_ror(o, &list, x, y, e->rec.f18);
    count = cluster_count(o, e);
    while (count > 0 && aoe_rm_list_pop_ror(&list, &tx, &ty, &dummy)) {
        float fx, fy;
        if (e->rec.f04 >= 0 && terrain_at(m, tx, ty) != (uint8_t)e->rec.f04) continue;
        spot(o, player, e->rec.unit, tx, ty, &fx, &fy);
        if (!refused(o, player, e->rec.unit, fx, fy, 0)) {
            create(o, player, e->rec.unit, fx, fy);
            --count;
        }
    }
    aoe_rm_list_detach_ror(&list);
}

static void cluster_grow(AoeRmObjectModuleRor *o, AoeRmObjectWorkRor *e,
                         int32_t x, int32_t y, int32_t player) {
    AoeRmModuleRor *m = &o->base;
    AoeRmNodeRor list;
    int32_t r = e->rec.f18, x0, y0, x1, y1, count, ix, iy, tx, ty;
    uint32_t dummy;
    aoe_rm_list_init_ror(&list);
    aoe_rm_node_link_ror(&list, &m->node_rows[y][x]);
    x0 = x - r; y0 = y - r; x1 = x + r; y1 = y + r;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (m->width <= x1) x1 = m->width - 1;
    if (m->height <= y1) y1 = m->height - 1;
    for (iy = y0; iy <= y1; ++iy)
        for (ix = x0; ix <= x1; ++ix) m->grid_rows[iy][ix] = 1;
    count = cluster_count(o, e);
    while (count > 0 && aoe_rm_list_pop_ror(&list, &tx, &ty, &dummy)) {
        float fx, fy;
        spot(o, player, e->rec.unit, tx, ty, &fx, &fy);
        if (o->world->adjust) o->world->adjust(o->world->context, player, e->rec.unit, &fx, &fy);
        if (refused(o, player, e->rec.unit, fx, fy, 0)) continue;
        create(o, player, e->rec.unit, fx, fy);
        --count;
        m->grid_rows[ty][tx] = 0;
        if (x0 < tx && m->grid_rows[ty][tx - 1])
            aoe_rm_list_push_ror(m, &list, tx - 1, ty, 0, (float)aoe_rng_next(m->rng));
        if (tx < x1 && m->grid_rows[ty][tx + 1])
            aoe_rm_list_push_ror(m, &list, tx + 1, ty, 0, (float)aoe_rng_next(m->rng));
        if (y0 < ty && m->grid_rows[ty - 1][tx])
            aoe_rm_list_push_ror(m, &list, tx, ty - 1, 0, (float)aoe_rng_next(m->rng));
        if (ty < y1 && m->grid_rows[ty + 1][tx])
            aoe_rm_list_push_ror(m, &list, tx, ty + 1, 0, (float)aoe_rng_next(m->rng));
    }
    aoe_rm_list_detach_ror(&list);
}

static void put(AoeRmObjectModuleRor *o, AoeRmObjectWorkRor *e, int32_t x,
                int32_t y, int32_t player, float fx, float fy) {
    terrain_unlink_square(&o->base, x, y, e->flag);
    switch (e->rec.f08) {
    case 0: create(o, player, e->rec.unit, fx, fy); break;
    case 1: cluster_scatter(o, e, x, y, player); break;
    case 2: cluster_grow(o, e, x, y, player); break;
    default: break;
    }
}

/* Lands -1 and -2: random open tiles of the whole map. */
static void place_anywhere(AoeRmObjectModuleRor *o, AoeRmObjectWorkRor *e,
                           int away) {
    AoeRmModuleRor *m = &o->base;
    AoeRmNodeRor list;
    int32_t player = player_of(e->rec.f1c), remaining = e->rec.f14, x, y, k;
    uint32_t dummy;
    memset(m->grid, 1, (size_t)m->width * (size_t)m->height);
    if (away)
        for (k = 0; k < o->land_count && k < AOE_RM_SLOTS; ++k)
            aoe_rm_object_fill_ror(o, o->lands[k].x, o->lands[k].y, e->rec.f24, 0);
    aoe_rm_list_init_ror(&list);
    aoe_rm_object_gather_ror(o, &list, -1, -1, -1);
    while (remaining > 0 && aoe_rm_list_pop_ror(&list, &x, &y, &dummy)) {
        float fx, fy;
        if (e->rec.f04 >= 0 && terrain_at(m, x, y) != (uint8_t)e->rec.f04) continue;
        spot(o, player, e->rec.unit, x, y, &fx, &fy);
        if (refused(o, player, e->rec.unit, fx, fy, 0)) continue;
        put(o, e, x, y, player, fx, fy);
        --remaining;
    }
    aoe_rm_list_detach_ror(&list);
}

static void place_near(AoeRmObjectModuleRor *o, AoeRmObjectWorkRor *e,
                       int32_t lx, int32_t ly, int32_t r, int32_t reach,
                       int32_t land) {
    AoeRmModuleRor *m = &o->base;
    AoeRmNodeRor list;
    int32_t x0 = lx - r, y0 = ly - r, x1 = lx + r, y1 = ly + r, k, player,
            remaining, tx, ty;
    void *zones;
    uint8_t home;
    uint32_t dummy;
    float sx, sy;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (m->width <= x1) x1 = m->width - 1;
    if (m->height <= y1) y1 = m->height - 1;
    memset(m->grid, 1, (size_t)m->width * (size_t)m->height);
    for (k = 0; k < o->land_count && k < AOE_RM_SLOTS; ++k)
        aoe_rm_object_fill_ror(o, o->lands[k].x, o->lands[k].y, r, 0);
    aoe_rm_list_init_ror(&list);
    aoe_rm_object_gather_ror(o, &list, lx, ly, reach);
    player = e->rec.f1c > -1 ? e->rec.f1c
                             : (o->lands[land].player > 0 ? o->lands[land].player : 0);
    if (!o->world->unit_size ||
        !o->world->unit_size(o->world->context, player, e->rec.unit, &sx, &sy)) {
        aoe_rm_list_detach_ror(&list); /* the original returns with it linked */
        return;
    }
    zones = o->world->unit_zones ? o->world->unit_zones(o->world->context, player, e->rec.unit) : NULL;
    home = o->world->zone_at ? o->world->zone_at(o->world->context, zones, lx, ly) : 0;
    remaining = e->rec.f14;
    while (remaining > 0 && aoe_rm_list_pop_ror(&list, &tx, &ty, &dummy)) {
        float fx, fy;
        uint8_t z = o->world->zone_at ? o->world->zone_at(o->world->context, zones, tx, ty) : 0;
        if (home != z) continue;
        if (e->rec.f04 >= 0 && terrain_at(m, tx, ty) != (uint8_t)e->rec.f04) continue;
        if (!(tx <= x0 || tx >= x1 || ty <= y0 || ty >= y1)) continue;
        spot(o, player, e->rec.unit, tx, ty, &fx, &fy);
        if (!refused(o, player, e->rec.unit, fx, fy, 1)) {
            put(o, e, tx, ty, player, fx, fy);
            --remaining;
        } else if (e->rec.f08 == 0 && !list.next) {
            /* The last candidate is placed even when refused. */
            create(o, player, e->rec.unit, fx, fy);
        }
    }
    aoe_rm_list_detach_ror(&list);
}

void aoe_rm_object_place_ror(AoeRmObjectModuleRor *o, AoeRmObjectWorkRor *e) {
    AoeRmModuleRor *m = &o->base;
    int32_t k;
    if (e->rec.f09) {
        e->rec.f14 = m->height * m->width * e->rec.f14 / 0x5100;
        if (e->rec.f14 < 1) e->rec.f14 = 1;
    }
    if (e->rec.land < 0) {
        if (e->rec.land == -1) place_anywhere(o, e, 0);
        else if (e->rec.land == -2) place_anywhere(o, e, 1);
        return;
    }
    for (k = 0; k < o->land_count && k < AOE_RM_SLOTS; ++k)
        if (o->lands[k].id == e->rec.land)
            place_near(o, e, o->lands[k].x, o->lands[k].y, e->rec.f24, e->rec.f28, k);
}

void aoe_rm_object_add_ror(AoeRmObjectModuleRor *o, int32_t unit,
                           int32_t count, int32_t flag) {
    AoeRmObjectWorkRor *e;
    if (!o || o->object_count >= AOE_RM_SLOTS) return;
    e = &o->objects[o->object_count];
    memset(e, 0, sizeof(*e));
    e->rec.unit = unit;
    e->rec.f04 = -1;
    e->rec.f0c = 1;
    e->rec.f10 = 1;
    e->rec.f14 = count;
    e->rec.f18 = 2;
    e->rec.land = -1;
    e->rec.f28 = 1;
    e->flag = flag;
    ++o->object_count;
}

static int object_run(AoeRmModuleRor *m) {
    AoeRmObjectModuleRor *o = (AoeRmObjectModuleRor *)m;
    int32_t i;
    if (o->clear && o->world->clear_all) o->world->clear_all(o->world->context);
    aoe_rm_nodes_unlink_all_ror(m);
    memset(o->counters, 0, sizeof o->counters);
    for (i = 0; i < o->object_count && i < AOE_RM_SLOTS; ++i)
        aoe_rm_object_place_ror(o, &o->objects[i]);
    if (o->world->zones_reset) o->world->zones_reset(o->world->context);
    return 1;
}

static const AoeRmModuleVtblRor object_vt = {object_run, destroy_module};
static const AoeRmWorldOpsRor no_world;

AoeRmObjectModuleRor *aoe_rm_object_module_create_ror(AoeRmControllerRor *c,
                                                       int with_work,
                                                       uint8_t clear) {
    AoeRmObjectModuleRor *o = calloc(1, sizeof(*o));
    if (!o) return NULL;
    aoe_rm_module_init_ror(&o->base, &object_vt, c->base.map, &c->base, 1, NULL);
    o->world = c->world ? c->world : &no_world;
    if (with_work) {
        memcpy(o->objects, c->objects, sizeof(o->objects));
        o->object_count = c->object_count;
        memcpy(o->lands, c->object_lands, sizeof(o->lands));
        o->land_count = c->object_land_count;
    }
    o->base.priority = 3.0f;
    o->clear = clear;
    return o;
}

/* ---- Generation ---- */

int aoe_rm_generate_ror(AoeRmControllerRor *c) {
    int ok;
    int32_t x, y;
    if (!c) return 0;
    aoe_rm_terrain_module_create_ror(c);
    aoe_rm_land_module_create_ror(c);
    aoe_rm_object_module_create_ror(c, 1, 1);
    aoe_rm_elevation_module_create_ror(c);
    ok = aoe_rm_module_run_children_ror(&c->base);
    if (c->ror)
        for (y = 0; y < c->base.height; ++y)
            for (x = 0; x < c->base.width; ++x) {
                uint8_t *t = aoe_rm_tile_ror(&c->base, x, y);
                if ((*t & 0x1f) == 0x10) *t &= 0xe0;
            }
    return ok;
}

int aoe_rm_ror_setup_ror(AoeRmControllerRor *c, AoeRmMapRor *map,
                         int32_t object_arg, int32_t map_index,
                         int32_t players, int teams) {
    int32_t w, count = 0, size = 0, var;
    if (!aoe_rm_controller_setup_ror(c, map, object_arg, map_index, players, teams))
        return 0;
    if (aoe_rng_next(c->base.rng) < 0x2000) return 1;
    w = c->base.width;
    if (w < 0x49) { count = 1; size = 8; }
    else if (w < 0x61) { count = 4; size = 10; }
    else if (w < 0x79) { count = 6; size = 10; }
    else if (w < 0x91) { count = 8; size = 10; }
    else if (w < 0xc9) { count = 9; size = 10; }
    else { count = 10; size = 10; } /* 251+: the original reads stale stack */
    switch (c->map_index) {
    case 0: count = 0; size = 3; break;
    case 1: count -= 5; break;
    case 2: count -= 3; break;
    case 3: case 4: size += 2; break;
    case 7: count += 3; size += 4; break;
    default: break;
    }
    if (size < 3) size = 3;
    var = size - 3 < 1 ? 0 : (size - 3) / 2;
    size -= var;
    if (size > 0 && count > 0) aoe_rm_path_module_create_ror(c, count, size, var);
    return 1;
}
