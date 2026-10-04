#include "pathfinder_search.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "aoe_ftol.h"

/* Direction masks 0x00557b34 (index 8 = no arrival direction) and the
   quarter shifts 0x00557b30. The occupancy span masks 0x00557b58 equal
   ((1 << 2 * count) - 1) << 2 * start for all 96 entries. */
static const uint32_t direction_masks[9] = {56, 112, 224, 193, 131,
                                            7,  14,  28,  0};
static const uint8_t quarter_shift[4] = {0, 2, 4, 6};

void aoe_unit_path_clear_ror(AoeUnitPathRor *path) {
    if (!path) return;
    path->count = 0;
    path->cursor = 0;
}

int aoe_unit_path_prepend_ror(AoeUnitPathRor *path,
                              const AoePathWaypointRor *point) {
    AoePathWaypointRor *grown;
    if (!path || !point || path->count < 0 || path->count > 1000) return 0;
    grown = malloc((size_t)(path->count + 1) * sizeof(*grown));
    if (!grown) return 0;
    grown[0] = *point;
    if (path->count)
        memcpy(grown + 1, path->points, (size_t)path->count * sizeof(*grown));
    free(path->points);
    path->points = grown;
    ++path->count;
    return 1;
}

void aoe_unit_path_free_ror(AoeUnitPathRor *path) {
    if (!path) return;
    free(path->points);
    path->points = NULL;
    path->count = path->cursor = 0;
}

typedef struct {
    AoePathfinderRor *p;
    const int32_t *table;
    const AoePathRequestRor *r;
    const AoePathWorldOps *ops;
    AoeLiveUnit *unit;
    int32_t self_lifted;
    int32_t target_id;     /* +0x11dc4c: -1 when the target did not resolve */
    int32_t goal_radius;
    int32_t e[4], q[4]; /* tile and quarter footprint offsets */
    int host_error;
} Search;

static int32_t cell(int32_t x, int32_t y) {
    return x * AOE_PATH_GRID_SIDE_ROR + y;
}

static int cell_ok(int32_t index) {
    return index >= 0 && index < AOE_PATH_GRID_CELLS_ROR;
}

/* (state & 0xf8) == generation ? state & 7 : `other`. */
static uint8_t arrival(const Search *s, int32_t index, uint8_t other) {
    uint8_t b = s->p->state[index];
    return (uint8_t)((b & 0xf8) == s->p->generation ? (b & 7) : other);
}

static uint32_t occupancy_dword(Search *s, int32_t index) {
    const uint8_t *o = s->p->occupancy;
    if (index < 0 || index + 4 >
        AOE_PATH_OCCUPANCY_BYTES_ROR + AOE_PATH_OCCUPANCY_PAD_ROR) {
        s->host_error = 1;
        return 0;
    }
    return (uint32_t)o[index] | (uint32_t)o[index + 1] << 8 |
           (uint32_t)o[index + 2] << 16 | (uint32_t)o[index + 3] << 24;
}

static uint8_t occupancy_byte(Search *s, int32_t index) {
    if (index < 0 || index >= AOE_PATH_OCCUPANCY_BYTES_ROR) {
        s->host_error = 1;
        return 0;
    }
    return s->p->occupancy[index];
}

static int terrain_passes(Search *s, int32_t terrain) {
    if (terrain < 0 || terrain >= s->ops->terrain_cost_count) {
        s->host_error = 1;
        return 0;
    }
    return s->ops->terrain_costs[terrain] > 0.0f;
}

/* 1 passable, 0xfe blocked. */
static int terrain_sweep(Search *s, int32_t x_lo, int32_t x_hi, int32_t y_lo,
                         int32_t y_hi) {
    uint8_t xb_hi = (uint8_t)x_hi, yb_hi = (uint8_t)y_hi;
    if (xb_hi >= s->ops->map_width || yb_hi >= s->ops->map_height) return 0xfe;
    /* Byte-sized loop counters, compared unsigned (ja/jbe). */
    for (uint8_t yb = (uint8_t)y_lo; yb <= yb_hi; ++yb) {
        for (uint8_t xb = (uint8_t)x_lo; xb <= xb_hi; ++xb) {
            uint8_t b5, b6;
            int32_t terrain;
            if (s->r->x1 == xb && s->r->y1 == yb) continue;
            if (!s->ops->tile(s->ops->context, xb, yb, &b5, &b6)) {
                s->host_error = 1;
                return 0xfe;
            }
            terrain = b5 & 0x1f;
            if (terrain == s->ops->free_terrain_a ||
                terrain == s->ops->free_terrain_b)
                continue;
            if (terrain_passes(s, terrain)) continue;
            if (!(b6 & 0x0f)) return 0xfe;
            if (!terrain_passes(s, s->ops->overlay_terrain(s->ops->context,
                                                           b6 & 0x0f)))
                return 0xfe;
            if (xb == 0xff) break;
        }
        if (yb == 0xff) break;
    }
    return 1;
}

/* 1 clear, 0xfe blocked, 0 touches the target rectangle. */
static int occupancy_sweep(Search *s, int32_t qx_lo, int32_t qx_hi,
                           int32_t qy_lo, int32_t qy_hi) {
    const int32_t *t = s->p->target_rect;
    int32_t n = qy_hi - qy_lo - 1;
    if (n < 0) return 1;
    if (n < 24 && (qx_lo > t[2] || qy_lo > t[3] || qx_hi < t[0] ||
                   qy_hi < t[1])) {
        uint32_t start = (uint32_t)(qy_lo & 3), count = (uint32_t)n + 1;
        uint64_t mask = ((count >= 32 ? ~0ull : ((1ull << (2 * count)) - 1))
                         << (2 * start));
        uint32_t lo = (uint32_t)mask, hi = (uint32_t)(mask >> 32);
        for (int32_t x = qx_lo; x < qx_hi; ++x) {
            int32_t index = x * AOE_PATH_GRID_SIDE_ROR + (qy_lo >> 2);
            if (occupancy_dword(s, index) & lo) return 0xfe;
            if (hi && (occupancy_dword(s, index + 4) & hi)) return 0xfe;
            if (s->host_error) return 0xfe;
        }
        return 1;
    }
    {
        int touched = 0;
        for (int32_t x = qx_lo; x < qx_hi; ++x)
            for (int32_t y = qy_lo; y < qy_hi; ++y) {
                uint8_t b = occupancy_byte(
                    s, x * AOE_PATH_GRID_SIDE_ROR + (y >> 2));
                if (s->host_error) return 0xfe;
                if (!((b >> quarter_shift[y & 3]) & 3)) continue;
                if (s->target_id == -1 || t[0] > x || x > t[2] ||
                    t[1] > y || y > t[3])
                    return 0xfe;
                touched = 1;
            }
        return touched ? 0 : 1;
    }
}

static void heap_sift_up(AoePathfinderRor *p, int32_t i) {
    AoePathHeapEntryRor saved;
    int32_t parent;
    if (i < 1) return;
    saved = p->heap[i];
    parent = i / 2;
    if (saved.cost >= p->heap[parent].cost) return;
    do {
        p->heap[i] = p->heap[parent];
        i /= 2;
        parent /= 2;
    } while (saved.cost < p->heap[parent].cost);
    p->heap[i] = saved;
}

static int heap_push(Search *s, uint8_t x, uint8_t y, int32_t cost) {
    AoePathfinderRor *p = s->p;
    if ((uint32_t)p->heap_size + 1 >= p->heap_capacity) {
        s->host_error = 1;
        return 0;
    }
    ++p->heap_size;
    p->heap[p->heap_size].x = x;
    p->heap[p->heap_size].y = y;
    p->heap[p->heap_size].cost = cost;
    heap_sift_up(p, p->heap_size);
    return 1;
}

/* Pop: the last entry moves to the root; the sift loop only runs while
   the child index stays below size / 2 (small heaps are not re-sifted). */
static void heap_pop(AoePathfinderRor *p) {
    AoePathHeapEntryRor last = p->heap[p->heap_size];
    int32_t i = 1, size;
    p->heap[1] = last;
    size = --p->heap_size;
    if (size / 2 > 1) {
        int32_t child;
        do {
            child = i * 2;
            if (child < size && p->heap[child].cost > p->heap[child + 1].cost)
                ++child;
            if (last.cost < p->heap[child].cost) break;
            p->heap[i] = p->heap[child];
            i = child;
        } while (child < p->heap_size / 2);
        if (i != 1) p->heap[i] = last;
    }
}

static void cleanup_group(Search *s) {
    const AoePathWorldOps *ops = s->ops;
    for (uint32_t i = 0; i < s->unit->group_unit_count; ++i) {
        AoeLiveUnit *m = ops->resolve(ops->context, s->unit->group_unit_ids[i]);
        if (m && m->core.unit_id != s->unit->core.unit_id &&
            ops->member_flag_85(ops->context, m) == 1 &&
            !ops->attached(ops->context, m))
            ops->restore_obstruction(ops->context, m);
    }
}

static void finish_self(Search *s) {
    if (s->self_lifted == 1 && !s->ops->attached(s->ops->context, s->unit))
        s->ops->restore_obstruction(s->ops->context, s->unit);
}

static void collect_nearby(Search *s, int32_t x, int32_t y) {
    AoePathfinderRor *p = s->p;
    int32_t x0 = x - 1, x1 = x + 1, y0 = y - 1, y1 = y + 1;
    if (x0 < 0) {
        x0 = 0;
        x1 = x + 2;
    } else if (p->width <= x1) {
        x0 = x - 2;
        x1 = p->width - 1;
    }
    if (y0 < 0) {
        y0 = 0;
        y1 = y + 2;
    } else if (p->height <= y1) {
        y0 = y - 2;
        y1 = p->height - 1;
    }
    for (int32_t ix = x0; ix <= x1; ++ix)
        for (int32_t iy = y0; iy <= y1; ++iy) {
            int32_t count = 0, id;
            uint32_t k;
            id = s->ops->query(s->ops->context, s->r->query_radius,
                               s->r->query_filter, ix, iy, &count);
            if (id == -1) continue;
            for (k = 0; k < p->nearby_count && k < p->nearby_capacity; ++k)
                if (p->nearby_ids[k] == id) break;
            if (k < p->nearby_count && k < p->nearby_capacity) continue;
            if (p->nearby_count == p->nearby_capacity) {
                int32_t *grown = malloc(((size_t)p->nearby_count + 1) *
                                        sizeof(*grown));
                if (!grown) {
                    s->host_error = 1;
                    return;
                }
                if (p->nearby_capacity)
                    memcpy(grown, p->nearby_ids,
                           p->nearby_capacity * sizeof(*grown));
                free(p->nearby_ids);
                p->nearby_ids = grown;
                p->nearby_capacity = p->nearby_count + 1;
            }
            p->nearby_ids[p->nearby_count++] = id;
        }
}

/* 0x0045a970: walk back from heap[1] through the arrival directions. */
static int build_path(Search *s) {
    AoePathfinderRor *p = s->p;
    AoeUnitPathRor *path = s->ops->unit_path(s->ops->context, s->unit);
    uint8_t step = (uint8_t)s->r->step, x, y, d, prev = 0xff;
    AoePathWaypointRor wp;
    int guard = 0;
    if (!path) {
        s->host_error = 1;
        return 0;
    }
    aoe_unit_path_clear_ror(path);
    x = p->heap[1].x;
    y = p->heap[1].y;
    d = arrival(s, cell(x, y), 0xff);
    wp.x = (float)((double)x + p->offset_x);
    wp.y = (float)((double)y + p->offset_y);
    wp.z = s->unit->core.position.z40;
    wp.direction = d;
    path->cursor = 0;
    for (;;) {
        if (s->r->x1 == x && s->r->y1 == y)
            return aoe_unit_path_prepend_ror(path, &wp);
        if (d != prev && !aoe_unit_path_prepend_ror(path, &wp)) return 0;
        if (s->r->query_filter != -1) {
            collect_nearby(s, x, y);
            if (s->host_error) return 0;
        }
        switch (d) {
        case 0: x += step; y += step; break;
        case 1: y += step; break;
        case 2: x -= step; y += step; break;
        case 3: x -= step; break;
        case 4: x -= step; y -= step; break;
        case 5: y -= step; break;
        case 6: x += step; y -= step; break;
        case 7: x += step; break;
        default: break;
        }
        prev = d;
        d = arrival(s, cell(x, y), 0xff);
        wp.x = (float)((double)x + p->offset_x);
        wp.y = (float)((double)y + p->offset_y);
        wp.direction = d;
        /* Host guard: the original loops forever on a broken chain. */
        if (++guard > AOE_PATH_GRID_CELLS_ROR) {
            s->host_error = 1;
            return 0;
        }
    }
}

static void setup_offsets(Search *s) {
    const DatUnit *t = s->unit->core.unit_template;
    double s0 = t->size[0], s1 = t->size[1];
    double a, b, c, d;
    float a_f, c_f, d_f;
    a = ((double)s->p->offset_x + 100.0f) - s0;
    a_f = (float)a;
    s->e[0] = aoe_ftol32(a + 4.999999873689376e-05) - 100;
    b = s0 + s->p->offset_x;
    s->e[1] = aoe_ftol32(b + 100.00005340576172) - 100;
    c = ((double)s->p->offset_y + 100.0f) - s1;
    c_f = (float)c;
    s->e[2] = aoe_ftol32(c + 4.999999873689376e-05) - 100;
    d = s1 + s->p->offset_y;
    d_f = (float)d;
    s->e[3] = aoe_ftol32(d + 100.00005340576172) - 100;
    s->q[0] = -400 - aoe_ftol32((double)a_f * -4.0);
    s->q[1] = -400 - aoe_ftol32((b + 100.0) * -4.0);
    s->q[2] = -400 - aoe_ftol32((double)c_f * -4.0);
    s->q[3] = -400 - aoe_ftol32(((double)d_f + 100.0) * -4.0);
}

static int ops_valid(const AoePathWorldOps *o) {
    return o && o->resolve && o->speed && o->unit_path && o->terrain_costs &&
           o->tile && o->overlay_terrain && o->lift_obstruction &&
           o->restore_obstruction && o->centered && o->retry_count &&
           o->member_busy && o->member_rate && o->member_field_130 &&
           o->member_flag_85 && o->attached && o->footprint_adjust &&
           o->query && o->owner_id && o->owner_kind;
}

static void *path_unit(void *context, void *unit) { (void)context; return unit; }

typedef struct {
    const AoePathWorldOps *ops;
} PrecheckBridge;

static float bridge_speed(void *context, void *unit) {
    const PrecheckBridge *b = context;
    return b->ops->speed(b->ops->context, unit);
}

static void bridge_clear(void *context, void *unit) {
    const PrecheckBridge *b = context;
    aoe_unit_path_clear_ror(b->ops->unit_path(b->ops->context, unit));
}

AoePathResultRor aoe_path_find_ror(AoePathfinderRor *p,
                                   const int32_t table[256],
                                   const AoePathRequestRor *r,
                                   const AoePathWorldOps *ops,
                                   int32_t *returned, float *distance) {
    Search s;
    PrecheckBridge bridge;
    AoePathPrecheckOps pre;
    AoeLiveUnit *unit;
    AoePathPrecheckRor first;
    int32_t h, budget, iter = 0, d16, dist;
    float floor_x, floor_y;
    (void)path_unit;
    if (!p || !table || !r || !returned || !ops_valid(ops) || !r->unit ||
        !r->unit->core.unit_template || !p->state || !p->cost || !p->heap ||
        p->heap_capacity < 2 || !p->occupancy || !p->penalty)
        return AOE_PATH_RESULT_ERROR_ROR;
    unit = r->unit;
    bridge.ops = ops;
    pre.context = &bridge;
    pre.speed = bridge_speed;
    pre.clear_unit_path = bridge_clear;
    *returned = 0;
    first = aoe_path_precheck_ror(p, table, r->x1, r->y1, r->x2, r->y2, unit,
                                  r->range, r->mode, distance, &pre);
    if (first == AOE_PATH_PRECHECK_FAIL_ROR) return AOE_PATH_RESULT_FAIL_ROR;
    if (first == AOE_PATH_PRECHECK_IN_RANGE_ROR) {
        *returned = 1;
        return AOE_PATH_RESULT_IN_RANGE_ROR;
    }
    if (!cell_ok(cell(r->x1, r->y1)) || !cell_ok(cell(r->x2, r->y2)))
        return AOE_PATH_RESULT_ERROR_ROR;
    memset(&s, 0, sizeof(s));
    s.p = p;
    s.table = table;
    s.r = r;
    s.ops = ops;
    s.unit = unit;
    p->nearby_count = 0;

    /* Target rectangle in quarter tiles. */
    p->target_rect[0] = r->x2 * 4;
    p->target_rect[1] = r->y2 * 4;
    p->target_rect[2] = r->x2 * 4 + 3;
    p->target_rect[3] = r->y2 * 4 + 3;
    s.target_id = r->target_id;
    if (r->target_id != -1) {
        AoeLiveUnit *t = ops->resolve(ops->context, r->target_id);
        if (!t) {
            s.target_id = -1;
        } else {
            if (!t->core.unit_template) return AOE_PATH_RESULT_ERROR_ROR;
            const AoeVisibilityUnitRef *tp = &t->core.position;
            const float *ts = t->core.unit_template->size;
            p->target_rect[0] = aoe_ftol32(((double)tp->x38 - ts[0]) * 4.0);
            p->target_rect[1] = aoe_ftol32(((double)tp->y3c - ts[1]) * 4.0);
            p->target_rect[2] = aoe_ftol32(((double)ts[0] + tp->x38) * 4.0);
            p->target_rect[3] = aoe_ftol32(((double)ts[1] + tp->y3c) * 4.0);
        }
    }
    p->target_id = s.target_id;
    s.self_lifted = ops->lift_obstruction(ops->context, unit);

    /* Sub-tile offset: tile centre when the unit sits centred there. */
    floor_y = (float)(floor((double)unit->core.position.y3c) + 0.5f);
    floor_x = (float)(floor((double)unit->core.position.x38) + 0.5f);
    if (ops->centered(ops->context, unit, floor_x, floor_y)) {
        p->offset_x = p->offset_y = 0.5f;
    } else {
        p->offset_x = (float)((double)unit->core.position.x38 -
                              floor((double)unit->core.position.x38));
        p->offset_y = (float)((double)unit->core.position.y3c -
                              floor((double)unit->core.position.y3c));
    }

    /* Moving group members become temporary obstructions after ten
       retries. */
    if (ops->retry_count(ops->context, unit) >= 10)
        for (uint32_t i = 0; i < unit->group_unit_count; ++i) {
            AoeLiveUnit *m = ops->resolve(ops->context,
                                          unit->group_unit_ids[i]);
            uint8_t busy;
            float rate;
            if (!m || m->core.unit_id == unit->core.unit_id) continue;
            /* Both virtuals run before either result is tested. */
            busy = ops->member_busy(ops->context, m);
            rate = ops->member_rate(ops->context, m);
            if (busy || (!isnan(rate) && rate != 0.0f) ||
                ops->member_field_130(ops->context, m) > 0.0f)
                ops->lift_obstruction(ops->context, m);
        }

    /* Unit footprint box in quarter tiles. */
    {
        const float *us = unit->core.unit_template->size;
        const AoeVisibilityUnitRef *up = &unit->core.position;
        int32_t low[2], high[2];
        low[0] = aoe_ftol32(((double)up->x38 - us[0]) * 4.0);
        if (low[0] < 0) low[0] = 0;
        low[1] = aoe_ftol32(((double)up->y3c - us[1]) * 4.0);
        if (low[1] < 0) low[1] = 0;
        high[0] = aoe_ftol32(((double)us[0] + up->x38) * 4.0);
        if (high[0] > p->width * 4) high[0] = p->width * 4;
        high[1] = aoe_ftol32(((double)us[1] + up->y3c) * 4.0);
        if (high[1] > p->height * 4) high[1] = p->height * 4;
        ops->footprint_adjust(ops->context, unit, low, high);
    }

    p->closest = -1;
    if (p->generation >= 0xf0) {
        p->generation = 0;
        memset(p->state, 0xff, AOE_PATH_GRID_CELLS_ROR);
    } else {
        p->generation = (uint8_t)(p->generation + 8);
    }
    p->heap_size = 1;
    p->expanded = 0;
    h = aoe_path_octile_ror(table, r->x1, r->y1, r->x2, r->y2);
    p->cost[cell(r->x1, r->y1)] = h;
    p->heap[1].x = (uint8_t)r->x1;
    p->heap[1].y = (uint8_t)r->y1;
    p->heap[1].cost = h;
    p->heap[0].cost = (int32_t)0x88ca6c00; /* -2000000000 sentinel */
    p->best_x = (uint8_t)r->x1;
    p->best_y = (uint8_t)r->y1;
    p->best_distance = h;
    /* fild step; fcomp range: below, equal or unordered uses the range. */
    s.goal_radius = !((double)r->step > (double)r->range)
                        ? aoe_ftol32((double)r->range * 16.0f)
                        : r->step << 4;
    setup_offsets(&s);

    /* Node budget. */
    {
        int32_t dx = r->x2 - r->x1, dy = r->y2 - r->y1;
        d16 = aoe_ftol32(sqrt((double)(dy * dy) + (double)(dx * dx)) * 16.0);
        dist = d16 / 16;
        budget = dist > 10 ? (dist * r->budget_base) / 100
                           : (dist * r->budget_base) / 10;
        if (ops->owner_id(ops->context, unit) == 0 && budget > 100)
            budget = 100;
        if (ops->owner_kind(ops->context, unit) == 1)
            budget = (dist * 2500) / 100;
    }

    while (p->heap_size > 0) {
        int32_t X, Y, eu, node, nx, ny;
        uint32_t bit = 1;
        AoePathResultRor reached = AOE_PATH_RESULT_FAIL_ROR;
        if ((r->budget_base != -1 && r->budget_limited == 1 &&
             iter > budget) ||
            iter > 20000)
            break;
        X = p->heap[1].x;
        Y = p->heap[1].y;
        {
            int32_t ex = r->x2 - X, ey = r->y2 - Y;
            eu = aoe_ftol32(sqrt((double)(ex * ex) + (double)(ey * ey)) *
                            16.0);
        }
        if (p->closest == -1 || eu < p->closest) p->closest = eu;
        if (eu < s.goal_radius) {
            reached = AOE_PATH_RESULT_GOAL_ROR;
            goto found;
        }
        if (eu < p->best_distance) {
            p->best_x = (uint8_t)X;
            p->best_y = (uint8_t)Y;
            p->best_distance = eu;
        }
        ++p->expanded;
        heap_pop(p);
        node = cell(X, Y);
        nx = X;
        ny = Y;
        for (int dir = 0; dir < 8; ++dir, bit <<= 1) {
            int32_t ncell, nc, parent_cost, o1, o2, step_cost;
            int32_t x_lo, x_hi, y_lo, y_hi, qx_lo, qx_hi, qy_lo, qy_hi;
            int32_t mx, Mx, my, My;
            int tres, penalty_flag = 0;
            float penalty_hp = 0.0f;
            uint8_t parent_dir;
            switch (dir) {
            case 0: nx -= r->step; ny -= r->step; break;
            case 1: nx += r->step; break;
            case 2: nx += r->step; break;
            case 3: ny += r->step; break;
            case 4: ny += r->step; break;
            case 5: nx -= r->step; break;
            case 6: nx -= r->step; break;
            case 7: ny -= r->step; break;
            }
            if (bit & direction_masks[arrival(&s, node, 8)]) continue;
            if (nx < 0 || ny < 0 || nx >= p->width || ny >= p->height) continue;
            if (nx == r->x1 && ny == r->y1) continue;
            ncell = cell(nx, ny);
            if (!cell_ok(ncell)) return AOE_PATH_RESULT_ERROR_ROR;
            parent_cost = p->cost[node];
            o1 = aoe_path_octile_ror(table, X, Y, r->x2, r->y2);
            o2 = aoe_path_octile_ror(table, nx, ny, r->x2, r->y2);
            {
                int32_t sx = nx - X, sy = ny - Y;
                step_cost = aoe_ftol32(sqrt((double)(sx * sx) +
                                            (double)(sy * sy)) * -16.0);
            }
            nc = o2 - step_cost - o1 + parent_cost;
            if ((p->state[ncell] & 0xf8) == p->generation &&
                p->cost[ncell] <= nc)
                continue;
            mx = X < nx ? X : nx;
            Mx = X < nx ? nx : X;
            my = Y < ny ? Y : ny;
            My = Y < ny ? ny : Y;
            x_lo = mx + s.e[0];
            x_hi = Mx + s.e[1];
            y_lo = my + s.e[2];
            y_hi = My + s.e[3];
            qx_lo = mx * 4 + s.q[0];
            qx_hi = Mx * 4 + s.q[1];
            qy_lo = my * 4 + s.q[2];
            qy_hi = My * 4 + s.q[3];
            /* Negative low bounds move up by exactly one. */
            if (x_lo < 0) ++x_lo;
            if (y_lo < 0) ++y_lo;
            if (qx_lo < 0) ++qx_lo;
            if (qy_lo < 0) ++qy_lo;
            tres = terrain_sweep(&s, x_lo, x_hi, y_lo, y_hi);
            if (s.host_error) return AOE_PATH_RESULT_ERROR_ROR;
            if (tres == 1 && r->check_occupancy == 1) {
                tres = occupancy_sweep(&s, qx_lo, qx_hi, qy_lo, qy_hi);
                if (s.host_error) return AOE_PATH_RESULT_ERROR_ROR;
            }
            if (tres == 0xfe) {
                int found_blocking = 0;
                if (r->query_filter == -1) continue;
                for (int32_t ix = nx - 1 < 0 ? 0 : nx - 1;
                     ix <= nx + 1 && ix < p->width && !found_blocking; ++ix)
                    for (int32_t iy = ny - 1 < 0 ? 0 : ny - 1;
                         iy <= ny + 1 && iy < p->height && !found_blocking;
                         ++iy) {
                        int32_t count = 0;
                        int32_t id = ops->query(ops->context, r->query_radius,
                                                r->query_filter, ix, iy,
                                                &count);
                        if (id == -1) {
                            if (count > 0) found_blocking = 1;
                        } else {
                            AoeLiveUnit *u = ops->resolve(ops->context, id);
                            if (u) {
                                penalty_flag = 1;
                                penalty_hp = u->core.hit_points;
                            }
                        }
                    }
                if (found_blocking) continue;
            }
            nc += (int32_t)p->penalty[ncell] << 4;
            if (penalty_flag == 1) nc += penalty_hp >= 10.0f ? 0x3c : 0x10;
            parent_dir = arrival(&s, node, 0xff);
            if ((uint8_t)dir != parent_dir) ++nc;
            if (tres == 0) nc = 0;
            p->cost[ncell] = nc;
            p->state[ncell] = (uint8_t)((dir & 7) | p->generation);
            if (!heap_push(&s, (uint8_t)nx, (uint8_t)ny, nc))
                return AOE_PATH_RESULT_ERROR_ROR;
            if (tres == 0) {
                reached = AOE_PATH_RESULT_TOUCH_ROR;
                goto found;
            }
        }
        ++iter;
        continue;
    found:
        if (r->mode == 1 && !build_path(&s)) {
            if (s.host_error) return AOE_PATH_RESULT_ERROR_ROR;
            aoe_unit_path_clear_ror(ops->unit_path(ops->context, unit));
            cleanup_group(&s);
            finish_self(&s);
            return AOE_PATH_RESULT_FAIL_ROR;
        }
        if (distance) *distance = (float)p->cost[cell(X, Y)];
        cleanup_group(&s);
        finish_self(&s);
        *returned = 1;
        return reached;
    }

    /* Budget or heap exhausted: head toward the best node. */
    if (!heap_push(&s, p->best_x, p->best_y, 0))
        return AOE_PATH_RESULT_ERROR_ROR;
    if (r->mode == 1 && !build_path(&s)) {
        if (s.host_error) return AOE_PATH_RESULT_ERROR_ROR;
        aoe_unit_path_clear_ror(ops->unit_path(ops->context, unit));
        cleanup_group(&s);
        finish_self(&s);
        return AOE_PATH_RESULT_FAIL_ROR;
    }
    if (distance) *distance = (float)p->best_distance;
    cleanup_group(&s);
    finish_self(&s);
    return AOE_PATH_RESULT_PARTIAL_ROR;
}

int aoe_path_fits_ror(AoePathfinderRor *p, const AoePathWorldOps *ops,
                      AoeLiveUnit *unit, float x, float y, int check_terrain) {
    AoePathRequestRor none;
    Search s;
    const DatUnit *t;
    double fy, sx, sy;
    float fx;
    int32_t ix, iy, qx_lo, qx_hi, qy_lo, qy_hi, registered, r;
    if (!p || !p->occupancy || !unit || !unit->core.unit_template ||
        !ops_valid(ops))
        return -1;
    t = unit->core.unit_template;
    memset(&none, 0, sizeof(none));
    none.x1 = none.y1 = -1; /* +0x11dc7c/+0x11dc80 reset: no start skip */
    memset(&s, 0, sizeof(s));
    s.p = p;
    s.r = &none;
    s.ops = ops;
    s.unit = unit;
    s.target_id = p->target_id;

    /* Quarter-tile footprint. */
    fx = (float)((double)x - floor((double)x));
    fy = (double)y - floor((double)y);
    sx = t->size[0];
    sy = t->size[1];
    ix = aoe_ftol32(x);
    iy = aoe_ftol32(y);
    qx_lo = 4 * ix - 400 - aoe_ftol32(((double)fx + 100.0 - sx) * -4.0);
    qy_lo = 4 * iy - 400 - aoe_ftol32((fy + 100.0 - sy) * -4.0);
    qx_hi = 4 * ix - 400 - aoe_ftol32((sx + fx + 100.0) * -4.0);
    qy_hi = 4 * iy - 400 - aoe_ftol32((sy + fy + 100.0) * -4.0);
    if (qx_lo < 0) ++qx_lo;
    if (qy_lo < 0) ++qy_lo;

    if (check_terrain &&
        terrain_sweep(&s, qx_lo >> 2, qx_hi >> 2, qy_lo >> 2, qy_hi >> 2) !=
            1)
        return s.host_error ? -1 : 0;
    registered = ops->lift_obstruction(ops->context, unit);
    r = occupancy_sweep(&s, qx_lo, qx_hi, qy_lo, qy_hi);
    if (registered && !ops->attached(ops->context, unit))
        ops->restore_obstruction(ops->context, unit);
    if (s.host_error) return -1;
    return r == 1;
}

static int obstruction_update(const AoeLiveUnit *unit,
                              AoeObstructionStateRor *state,
                              AoePathfinderRor *main_map,
                              AoePathfinderRor *secondary_map,
                              int32_t selector, int add) {
    const DatUnit *t;
    const AoeVisibilityUnitRef *pos;
    int32_t x_lo, y_lo, x_hi, y_hi;
    if (!unit || !unit->core.unit_template || !state) return -1;
    t = unit->core.unit_template;
    if (!(t->size[2] > 0.0f)) return 0;
    if (add ? state->placed == 1 : state->placed == 0) return 0;
    if ((selector != 1 && !main_map) || (selector != 0 && !secondary_map))
        return -1;
    pos = &unit->core.position;
    x_lo = aoe_ftol32(((double)pos->x38 - t->size[0]) * 4.0);
    if (x_lo < 0) x_lo = 0;
    y_lo = aoe_ftol32(((double)pos->y3c - t->size[1]) * 4.0);
    if (y_lo < 0) y_lo = 0;
    x_hi = aoe_ftol32(((double)pos->x38 + t->size[0]) * 4.0);
    y_hi = aoe_ftol32(((double)t->size[1] + pos->y3c) * 4.0);
    for (int32_t qx = x_lo; qx < x_hi; ++qx)
        for (int32_t qy = y_lo; qy < y_hi; ++qy) {
            if (selector != 1 &&
                !(add ? aoe_path_occupancy_add_ror(main_map, qx, qy)
                      : aoe_path_occupancy_remove_ror(main_map, qx, qy)))
                return -1;
            if (selector != 0 &&
                !(add ? aoe_path_occupancy_add_ror(secondary_map, qx, qy)
                      : aoe_path_occupancy_remove_ror(secondary_map, qx, qy)))
                return -1;
        }
    state->previous = state->placed;
    state->placed = (uint8_t)(add ? 1 : 0);
    return 1;
}

int aoe_unit_obstruction_restore_ror(const AoeLiveUnit *unit,
                                     AoeObstructionStateRor *state,
                                     AoePathfinderRor *main_map,
                                     AoePathfinderRor *secondary_map,
                                     int32_t selector) {
    return obstruction_update(unit, state, main_map, secondary_map, selector,
                              1);
}

int aoe_unit_obstruction_lift_ror(const AoeLiveUnit *unit,
                                  AoeObstructionStateRor *state,
                                  AoePathfinderRor *main_map,
                                  AoePathfinderRor *secondary_map,
                                  int32_t selector) {
    return obstruction_update(unit, state, main_map, secondary_map, selector,
                              0);
}
