#include "span_region.h"

#include <stdlib.h>
#include <string.h>

/* ---- node pool ---- */

void aoe_pool_clear_stats_ror(AoeNodePoolRor *p) {
    if (!p) return;
    p->stat_2c = p->stat_30 = 0;
    p->blocks_added = 0;
    p->resets = 0;
}

int aoe_pool_grow_ror(AoeNodePoolRor *p, int32_t capacity) {
    uint8_t **blocks;
    int32_t *counts;
    if (!p || p->capacity > capacity) return 1;
    blocks = calloc((size_t)capacity, sizeof(*blocks));
    counts = calloc((size_t)capacity, sizeof(*counts));
    if (!blocks || !counts) {
        free(blocks);
        free(counts);
        return 0;
    }
    if (p->blocks) {
        if (p->block_count > 0) {
            memcpy(blocks, p->blocks, (size_t)p->block_count * sizeof(*blocks));
            memcpy(counts, p->block_counts, (size_t)p->block_count * sizeof(*counts));
        }
        free(p->blocks);
        free(p->block_counts);
    }
    p->block_counts = counts;
    p->blocks = blocks;
    p->capacity = capacity;
    return 1;
}

int aoe_pool_add_block_ror(AoeNodePoolRor *p, int32_t count) {
    int32_t i;
    if (!p || count < 0) return 0;
    if (p->block_count == p->capacity && !aoe_pool_grow_ror(p, p->block_count + 4))
        return 0;
    i = p->block_count++;
    p->block_counts[i] = count;
    p->blocks[i] = malloc((size_t)p->node_size * (size_t)(count ? count : 1));
    ++p->blocks_added;
    p->total += count;
    p->available += count;
    if (p->cur_block == -1) {
        p->cur_block = i;
        p->cur_index = 0;
    }
    return p->blocks[i] != NULL;
}

int aoe_pool_init_ror(AoeNodePoolRor *p, int32_t initial, int32_t per_block,
                      int32_t node_size) {
    if (!p) return 0;
    memset(p, 0, sizeof(*p));
    p->cur_block = -1;
    p->cur_index = -1;
    p->node_size = node_size < (int32_t)(2 * sizeof(void *))
                       ? (int32_t)(2 * sizeof(void *))
                       : node_size;
    p->capacity = 8;
    p->per_block = per_block;
    aoe_pool_clear_stats_ror(p);
    if (!aoe_pool_grow_ror(p, 8)) return 0;
    return aoe_pool_add_block_ror(p, initial);
}

void aoe_pool_free_ror(AoeNodePoolRor *p) {
    int32_t i;
    if (!p) return;
    for (i = 0; i < p->block_count; ++i) free(p->blocks[i]);
    free(p->blocks);
    free(p->block_counts);
    p->blocks = NULL;
    p->block_counts = NULL;
    p->block_count = 0;
}

void *aoe_pool_take_ror(AoeNodePoolRor *p) {
    void **node;
    if (!p) return NULL;
    if (p->available == 0 && !aoe_pool_add_block_ror(p, p->per_block)) return NULL;
    if (p->free_list) {
        node = p->free_list;
        p->free_list = node[0];
        --p->available;
        return node;
    }
    if (p->cur_block < 0) return NULL; /* port guard */
    node = (void **)(p->blocks[p->cur_block] +
                     (size_t)p->cur_index * (size_t)p->node_size);
    ++p->cur_index;
    --p->available;
    if (p->cur_index == p->block_counts[p->cur_block]) {
        ++p->cur_block;
        p->cur_index = 0;
        if (p->cur_block == p->block_count) p->cur_block = -1;
    }
    return node;
}

void aoe_pool_give_ror(AoeNodePoolRor *p, void *node) {
    void **n = node;
    if (!p || !n) return;
    n[0] = p->free_list;
    n[1] = NULL;
    p->free_list = n;
    ++p->available;
}

int32_t aoe_pool_give_chain_ror(AoeNodePoolRor *p, void *first, void *last) {
    void **n = first, **tail;
    int32_t count = 0;
    if (!p || !first) return 0;
    if (!last) {
        do {
            tail = n;
            ++count;
            ++p->available;
            n = tail[0];
        } while (n);
        tail[0] = p->free_list;
        p->free_list = first;
        return count;
    }
    do {
        ++count;
        ++p->available;
        if (n == last) break;
        n = n[0];
    } while (n);
    ((void **)last)[0] = p->free_list;
    p->free_list = first;
    return count;
}

void aoe_pool_reset_ror(AoeNodePoolRor *p) {
    if (!p) return;
    p->free_list = NULL;
    p->cur_block = 0;
    p->cur_index = 0;
    p->available = p->total;
    ++p->resets;
}

/* ---- span region ---- */

void aoe_span_region_set_bounds_ror(AoeSpanRegionRor *r, int32_t l, int32_t t,
                                    int32_t rr, int32_t b) {
    int32_t x_hi, y_lo;
    if (!r) return;
    x_hi = rr;
    if (rr < l) {
        x_hi = l;
        l = rr;
    }
    y_lo = t;
    if (b < t) {
        y_lo = b;
        b = t;
    }
    if (y_lo < 0) y_lo = 0;
    if (b < 0 || y_lo >= r->rows) return;
    if (r->rows <= b) b = r->rows - 1;
    if (l < 0) l = 0;
    if (x_hi < 0 || l >= r->cols) return;
    if (r->cols <= x_hi) x_hi = r->cols - 1;
    r->top = y_lo;
    r->bottom = b;
    r->left = l;
    r->right = x_hi;
}

int aoe_span_region_init_ror(AoeSpanRegionRor *r, int32_t w, int32_t h) {
    int32_t groups;
    if (!r) return 0;
    memset(r, 0, sizeof(*r));
    groups = (w + 0x1f) / 32;
    if (w + 0x1f < 0) groups = (w + 0x1f - 31) / 32; /* arithmetic shift */
    if (!aoe_pool_init_ror(&r->pool, groups * h, h << 4, (int32_t)sizeof(AoeSpanRor)))
        return 0;
    r->rows = h < 0x10 ? 0x10 : h;
    r->cols = w < 0x10 ? 0x10 : w;
    r->left = r->top = 0;
    r->right = r->bottom = 0;
    aoe_span_region_set_bounds_ror(r, 0, 0, r->cols - 1, r->rows - 1);
    r->first = calloc((size_t)r->rows, sizeof(*r->first));
    r->last = calloc((size_t)r->rows, sizeof(*r->last));
    r->min_x = calloc((size_t)r->rows, sizeof(*r->min_x));
    r->max_x = calloc((size_t)r->rows, sizeof(*r->max_x));
    r->count = calloc((size_t)r->rows, sizeof(*r->count));
    return r->first && r->last && r->min_x && r->max_x && r->count;
}

void aoe_span_region_free_ror(AoeSpanRegionRor *r) {
    if (!r) return;
    free(r->first);
    free(r->last);
    free(r->min_x);
    free(r->max_x);
    free(r->count);
    r->first = r->last = NULL;
    r->min_x = r->max_x = r->count = NULL;
    aoe_pool_free_ror(&r->pool);
}

void aoe_span_region_clear_ror(AoeSpanRegionRor *r) {
    if (!r) return;
    aoe_pool_reset_ror(&r->pool);
    memset(r->first, 0, (size_t)r->rows * sizeof(*r->first));
    memset(r->last, 0, (size_t)r->rows * sizeof(*r->last));
    memset(r->min_x, 0, (size_t)r->rows * sizeof(*r->min_x));
    memset(r->max_x, 0, (size_t)r->rows * sizeof(*r->max_x));
    memset(r->count, 0, (size_t)r->rows * sizeof(*r->count));
}

static AoeSpanRor *new_span(AoeSpanRegionRor *r) {
    return aoe_pool_take_ror(&r->pool);
}

static void drop_span(AoeSpanRegionRor *r, AoeSpanRor *s, int32_t y) {
    aoe_pool_give_ror(&r->pool, s);
    --r->count[y];
}

/* Clip a span to the bounds; 0 when nothing is left. */
static int clip(const AoeSpanRegionRor *r, int32_t *x0, int32_t *x1, int32_t y) {
    if (y < r->top || y > r->bottom) return 0;
    if (*x1 < *x0) {
        int32_t t = *x0;
        *x0 = *x1;
        *x1 = t;
    }
    if (*x1 < r->left || *x0 > r->right) return 0;
    if (*x0 < r->left) *x0 = r->left;
    if (r->right < *x1) *x1 = r->right;
    return 1;
}

void aoe_span_region_add_ror(AoeSpanRegionRor *r, int32_t x0, int32_t x1,
                             int32_t y) {
    AoeSpanRor *n, *s, *m;
    if (!r || !clip(r, &x0, &x1, y)) return;
    n = r->first[y];
    if (!n) {
        if (!(s = new_span(r))) return;
        s->next = s->prev = NULL;
        s->x0 = x0;
        s->x1 = x1;
        r->first[y] = r->last[y] = s;
        r->min_x[y] = x0;
        r->max_x[y] = x1;
        ++r->count[y];
        return;
    }
    if (r->min_x[y] < x0) {
        if (x1 < r->max_x[y]) {
            /* Inside the row's extent. */
            while (n->x1 < x0 - 1) n = n->next;
            if (x1 + 1 < n->x0) {
                if (!(s = new_span(r))) return;
                s->next = n;
                s->prev = n->prev;
                n->prev = s;
                s->prev->next = s;
                s->x0 = x0;
                s->x1 = x1;
                ++r->count[y];
                return;
            }
            if (x0 < n->x0) n->x0 = x0;
            if (n->x1 < x1 && (m = n->next) != NULL) {
                for (;;) {
                    AoeSpanRor *after;
                    if (x1 + 1 < m->x0) {
                        n->x1 = x1;
                        return;
                    }
                    if (x1 <= m->x1) break;
                    if (m->prev) m->prev->next = m->next;
                    if (m->next) m->next->prev = m->prev;
                    after = m->next;
                    drop_span(r, m, y);
                    m = after;
                    if (!m) return;
                }
                n->x1 = m->x1;
                n->next = m->next;
                drop_span(r, m, y);
                if (n->next) {
                    n->next->prev = n;
                    return;
                }
                r->last[y] = n;
            }
            return;
        }
        /* Reaching past the row's right end. */
        s = r->last[y];
        if (r->max_x[y] + 1 < x0) {
            if (!(m = new_span(r))) return;
            m->prev = s;
            m->next = NULL;
            s->next = m;
            m->x0 = x0;
            m->x1 = x1;
            r->max_x[y] = x1;
            r->last[y] = m;
            ++r->count[y];
            return;
        }
        r->max_x[y] = x1;
        if (x0 - 1 <= s->x1) {
            for (;;) {
                AoeSpanRor *p;
                if (s->x0 <= x0) {
                    s->x1 = x1;
                    return;
                }
                p = s->prev;
                if (p->x1 < x0 - 1) break;
                r->last[y] = p;
                p->next = NULL;
                drop_span(r, s, y);
                s = p;
            }
            s->x0 = x0;
            s->x1 = x1;
        }
        return;
    }
    /* Reaching the row's left end. */
    if (x1 < r->min_x[y] - 1) {
        if (!(s = new_span(r))) return;
        s->prev = NULL;
        s->next = n;
        n->prev = s;
        s->x0 = x0;
        s->x1 = x1;
        r->min_x[y] = x0;
        r->first[y] = s;
        ++r->count[y];
        return;
    }
    if (r->max_x[y] <= x1) {
        r->count[y] -= aoe_pool_give_chain_ror(&r->pool, n->next, NULL);
        n->next = n->prev = NULL;
        n->x0 = x0;
        n->x1 = x1;
        r->last[y] = n;
        r->max_x[y] = x1;
        r->min_x[y] = x0;
        return;
    }
    r->min_x[y] = x0;
    if (n->x0 <= x1 + 1) {
        for (;;) {
            if (x1 <= n->x1) {
                n->x0 = x0;
                return;
            }
            m = n->next;
            if (x1 + 1 < m->x0) break;
            r->first[y] = m;
            m->prev = NULL;
            drop_span(r, n, y);
            n = m;
        }
        n->x1 = x1;
        n->x0 = x0;
    }
}

void aoe_span_region_remove_ror(AoeSpanRegionRor *r, int32_t x0, int32_t x1,
                                int32_t y) {
    AoeSpanRor *first, *last, *n, *m;
    if (!r || !clip(r, &x0, &x1, y)) return;
    first = r->first[y];
    if (!first || r->min_x[y] > x1 || x0 > r->max_x[y]) return;
    if (x0 <= r->min_x[y] && r->max_x[y] <= x1) {
        aoe_pool_give_chain_ror(&r->pool, first, r->last[y]);
        r->first[y] = r->last[y] = NULL;
        r->count[y] = r->min_x[y] = r->max_x[y] = 0;
        return;
    }
    last = r->last[y];
    n = first;
    while (n->x1 < x0) n = n->next;
    if (n->x0 < x0) {
        if (x1 < n->x1) {
            if (!(m = new_span(r))) return;
            m->prev = n;
            m->next = n->next;
            n->next = m;
            if (m->next) m->next->prev = m;
            if (n == last) r->last[y] = m;
            m->x0 = x1 + 1;
            m->x1 = n->x1;
            n->x1 = x0 - 1;
            ++r->count[y];
            return;
        }
        n->x1 = x0 - 1;
        n = n->next;
    }
    while (n) {
        AoeSpanRor *after;
        if (n->x0 < x0 || x1 < n->x1) {
            if (n->x0 <= x1) n->x0 = x1 + 1;
            break;
        }
        if (n == first) first = n->next;
        if (n == last) last = n->prev;
        if (n->prev) n->prev->next = n->next;
        if (n->next) n->next->prev = n->prev;
        after = n->next;
        drop_span(r, n, y);
        n = after;
    }
    r->first[y] = first;
    r->last[y] = last;
    if (first) r->min_x[y] = first->x0; /* port guard */
    if (last) r->max_x[y] = last->x1;
}

static int16_t s16(const uint8_t *p) { return (int16_t)(p[0] | p[1] << 8); }

void aoe_span_region_add_outline_ror(AoeSpanRegionRor *r,
                                     const uint8_t *outline, int32_t width,
                                     int32_t height, int32_t x, int32_t y,
                                     int mirrored) {
    int32_t row, right = width - 1 + x;
    if (!r || !outline) return;
    for (row = y; row < height + y; ++row, outline += 4) {
        int16_t a = s16(outline), b = s16(outline + 2);
        if (row < r->top || row > r->bottom) continue;
        if (!mirrored) {
            if (a & 0x8000) continue;
            aoe_span_region_add_ror(r, (int32_t)((uint32_t)(a + x) & ~3u),
                                    (right - b) | 3, row);
        } else {
            if (b & 0x8000) continue;
            aoe_span_region_add_ror(r, (int32_t)((uint32_t)(b + x) & ~3u),
                                    (right - a) | 3, row);
        }
    }
}

static int32_t lo4(int32_t x) { return (int32_t)((uint32_t)x & ~3u); }

void aoe_span_region_add_line_ror(AoeSpanRegionRor *r, int32_t x0, int32_t y0,
                                  int32_t x1, int32_t y1) {
    int32_t dx, dy, step, err, y, y_end, x, start;
    if (!r) return;
    if (x0 == x1) {
        int32_t a = y0 < y1 ? y0 : y1, b = y0 < y1 ? y1 : y0;
        for (; a <= b; ++a) aoe_span_region_add_ror(r, lo4(x0), x0 | 3, a);
        return;
    }
    if (y0 == y1) {
        aoe_span_region_add_ror(r, lo4(x0), x1 | 3, y0);
        return;
    }
    y = y0;
    y_end = y1;
    if (x1 < x0) {
        int32_t t = x0;
        x0 = x1;
        x1 = t;
        y = y1;
        y_end = y0;
    }
    dx = x1 - x0 + 1;
    if (y < y_end) {
        step = 1;
        dy = y_end - y;
    } else {
        dy = y - y_end;
        step = -1;
    }
    ++dy;
    if (dy <= dx) {
        err = 0;
        start = x0;
        for (x = x0; x < x1; ++x) {
            err += dy;
            if (dx <= err) {
                aoe_span_region_add_ror(r, lo4(start), x | 3, y);
                y += step;
                err -= dx;
                start = x + 1;
            }
        }
        aoe_span_region_add_ror(r, lo4(start), x | 3, y);
        return;
    }
    err = 0;
    x = x0;
    for (;;) {
        aoe_span_region_add_ror(r, lo4(x), x | 3, y);
        if (y == y_end) break;
        y += step;
        err += dx;
        if (dy <= err) {
            ++x;
            err -= dy;
        }
    }
}

void aoe_span_region_add_list_ror(AoeSpanRegionRor *r, const uint8_t *list,
                                  int32_t x, int32_t y) {
    for (; list && *list != 0xff; list += 3)
        aoe_span_region_add_ror(r, list[1] + x, list[2] + x, list[0] + y);
}

void aoe_span_region_remove_list_ror(AoeSpanRegionRor *r, const uint8_t *list,
                                     int32_t x, int32_t y) {
    for (; list && *list != 0xff; list += 3)
        aoe_span_region_remove_ror(r, list[1] + x, list[2] + x, list[0] + y);
}

void aoe_span_region_union_ror(AoeSpanRegionRor *r, const AoeSpanRegionRor *a,
                               const AoeSpanRegionRor *b) {
    int32_t y;
    if (!r || !a || !b) return;
    aoe_span_region_clear_ror(r);
    if (r->rows != a->rows || r->rows != b->rows || r->cols != a->cols ||
        r->cols != b->cols)
        return;
    for (y = 0; y < r->rows; ++y) {
        const AoeSpanRor *p = a->first[y], *q = b->first[y];
        while (p || q) {
            if (p) {
                aoe_span_region_add_ror(r, lo4(p->x0), p->x1 | 3, y);
                p = p->next;
            }
            if (q) {
                aoe_span_region_add_ror(r, lo4(q->x0), q->x1 | 3, y);
                q = q->next;
            }
        }
    }
}
