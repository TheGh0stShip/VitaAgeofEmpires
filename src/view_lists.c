#include "view_lists.h"

#include <stdlib.h>
#include <string.h>

void aoe_view_mark_add_ror(AoeViewMarkRor **list, int32_t shape, int32_t field_c, int32_t x,
                           int32_t y, int32_t field_18, int32_t frames, int32_t field_2c,
                           int32_t kind, int32_t interval) {
    AoeViewMarkRor *m = malloc(sizeof *m);
    if (!m) return;
    m->prev = NULL;
    m->next = *list;
    if (*list) (*list)->prev = m;
    *list = m;
    m->shape = shape;
    m->field_c = field_c;
    m->x = x;
    m->y = y;
    m->field_18 = field_18;
    m->frames = frames;
    m->field_2c = field_2c;
    m->kind = kind;
    m->interval = interval;
    m->frame = 0;
}

void aoe_view_marks_free_ror(AoeViewMarkRor **list) {
    AoeViewMarkRor *m = *list;
    while (m) {
        AoeViewMarkRor *next = m->next;
        free(m);
        m = next;
    }
    *list = NULL;
}

void aoe_view_marks_drop_once_ror(AoeViewMarkRor **list) {
    AoeViewMarkRor *m = *list;
    while (m) {
        AoeViewMarkRor *next = m->next;
        if (m->kind == 1) {
            if (m->prev) m->prev->next = m->next;
            if (m->next) m->next->prev = m->prev;
            if (!m->prev) *list = m->next;
            free(m);
        }
        m = next;
    }
}

void aoe_draw_rows_set_clip_ror(AoeDrawRowsRor *d, int32_t l, int32_t t, int32_t r, int32_t b) {
    if (l > r || t > b) return;
    d->clip_left = l;
    d->clip_right = r;
    d->clip_top = t;
    d->clip_bottom = b;
}

void aoe_draw_rows_set_window_ror(AoeDrawRowsRor *d, int32_t a, int32_t b) {
    int32_t lo = a, hi = b;
    if (b < a) {
        lo = b;
        hi = a;
    }
    d->row_lo = lo < 0 ? 0 : lo;
    d->row_hi = hi > d->rows ? d->rows : hi;
}

AoeDrawRowNodeRor *aoe_draw_rows_find_ror(const AoeDrawRowsRor *d, int32_t id, int32_t y0,
                                          int32_t y1, int32_t key) {
    int32_t lo = y0, hi = y1, y;
    if (y1 < y0) {
        lo = y1;
        hi = y0;
    }
    if (lo < 0) lo = 0;
    if (hi > d->rows) hi = d->rows;
    if (lo > d->rows || hi < 0) return NULL;
    for (y = lo; y <= hi; ++y) {
        AoeDrawRowNodeRor *n;
        for (n = d->heads[y]; n; n = n->next)
            if (n->id == id && n->key == key) return n;
    }
    return NULL;
}

void aoe_draw_rows_reset_ror(AoeDrawRowsRor *d) {
    memset(d->table, 0, (size_t)(d->table_count & 0x3fffffff) * sizeof *d->table);
    memset(d->heads, 0, (size_t)(d->rows + 1) * sizeof *d->heads);
    memset(d->tails, 0, (size_t)(d->rows + 1) * sizeof *d->tails);
    aoe_pool_reset_ror(&d->pool);
}

AoeDrawRowNodeRor *aoe_draw_rows_take_ror(AoeDrawRowsRor *d) { return aoe_pool_take_ror(&d->pool); }

void aoe_draw_rows_free_ror(AoeDrawRowsRor *d) {
    aoe_pool_free_ror(&d->pool);
    free(d->table);
    free(d->heads);
    free(d->tails);
    d->table = NULL;
    d->heads = d->tails = NULL;
}

void aoe_view_clamp_point_ror(float point[2], int32_t map_w, int32_t map_h) {
    if (point[0] < 0.0f)
        point[0] = 0.0f;
    else if ((float)map_w <= point[0])
        point[0] = (float)map_w - 1.0f;
    if (point[1] < 0.0f)
        point[1] = 0.0f;
    else if ((float)map_h <= point[1])
        point[1] = (float)map_h - 1.0f;
}

void aoe_view_timers_rewind_ror(AoeViewTimerRor *timers, int32_t count, int32_t kind) {
    int32_t i;
    for (i = 0; i < count; ++i)
        if (timers[i].active == 1 && timers[i].kind == kind) timers[i].time -= timers[i].period;
}

void aoe_view_timer_add_ror(AoeViewTimerRor *timers, int32_t count, int32_t *active,
                            int32_t index, int32_t duration, int32_t flag, int32_t match,
                            uint32_t now) {
    AoeViewTimerRor *slot = NULL;
    int32_t i;
    for (i = 0; i < count; ++i) {
        if (timers[i].active == 0)
            slot = &timers[i];
        else if (match != 0 && timers[i].kind == match)
            timers[i].time -= timers[i].period;
    }
    if (!slot) return;
    slot->active = 1;
    slot->field_4 = index;
    slot->time = (int32_t)now;
    slot->period = duration;
    slot->kind = flag;
    ++*active;
}

void aoe_view_timers_tick_ror(AoeViewTimerRor *timers, int32_t count, int32_t *active,
                              uint8_t *(*object_flags)(void *context, int32_t index),
                              void *context, uint32_t now) {
    int32_t i;
    for (i = 0; i < count; ++i) {
        AoeViewTimerRor *t = &timers[i];
        uint8_t *flags;
        uint32_t elapsed;
        if (!t->active) continue;
        flags = object_flags(context, t->field_4);
        if (!flags) {
            t->active = 0;
            --*active;
            continue;
        }
        elapsed = now - (uint32_t)t->time;
        if ((uint32_t)t->period < elapsed) {
            t->active = 0;
            --*active;
        } else if ((elapsed & 0x100) == 0) {
            *flags |= (uint8_t)t->kind;
            continue;
        }
        *flags &= (uint8_t)~(uint8_t)t->kind;
    }
}

void aoe_view_geometry_ror(AoeViewGeometryRor *g) {
    if (g->has_map && g->tile_w != 0 && g->tile_h != 0) {
        g->field_11e = 0;
        g->field_120 = 0;
        g->across = (int16_t)(g->width / g->tile_w);
        g->down = (int16_t)(g->height / g->tile_h);
        g->centre_x = (int16_t)((int16_t)(g->width / 2) - g->half_tile_w + g->origin_x);
        g->centre_y = (int16_t)((int16_t)(g->height / 2) - g->half_tile_h + g->origin_y);
        return;
    }
    g->down = g->across = 0;
    g->centre_x = g->centre_y = 0;
    g->field_122 = g->field_124 = 0;
    g->field_11e = g->field_120 = 0;
}

/* Node dwords at +0x0c..+0x28 live in `body`, coords +0x30.. in `tail`. */
static void node_put32(AoeDrawRowNodeRor *n, int32_t offset, int32_t v) {
    memcpy(n->body + (offset - 0x0c), &v, sizeof(v));
}

int32_t aoe_draw_row_node_dword_ror(const AoeDrawRowNodeRor *n, int32_t offset) {
    int32_t v;
    memcpy(&v, n->body + (offset - 0x0c), sizeof(v));
    return v;
}

int16_t aoe_draw_row_node_coord_ror(const AoeDrawRowNodeRor *n, int32_t index) {
    int16_t v;
    memcpy(&v, n->tail + index * 2, sizeof(v));
    return v;
}

static void node_sub16(AoeDrawRowNodeRor *n, int32_t index, int32_t by) {
    int16_t v = (int16_t)(aoe_draw_row_node_coord_ror(n, index) - (int16_t)by);
    memcpy(n->tail + index * 2, &v, sizeof(v));
}

/* 0x00516510 */
AoeDrawRowNodeRor *aoe_draw_rows_add_ror(AoeDrawRowsRor *d, int32_t kind, int32_t row,
                                         const int16_t coords[8], int32_t depth,
                                         int32_t field_1c, int32_t id) {
    int32_t index = d->table_base + row;
    AoeDrawRowNodeRor *n;
    if (depth > d->row_hi || depth < d->row_lo || index < 0 || index >= d->table_count)
        return NULL;
    n = aoe_draw_rows_take_ror(d);
    if (!n) return NULL;
    n->free_link = d->table[index];
    d->table[index] = n;
    n->next = NULL;
    n->id = id;
    memset(n->body, 0, sizeof(n->body));
    node_put32(n, 0x10, depth);
    node_put32(n, 0x1c, field_1c);
    n->key = kind;
    memcpy(n->tail, coords, 8 * sizeof(int16_t));
    return n;
}

/* 0x00516660 */
void aoe_draw_rows_scroll_ror(AoeDrawRowsRor *d, int32_t dx, int32_t dy) {
    int32_t count = d->table_count, i;
    if (dy > 0) {
        int32_t n = dy < count ? dy : count;
        for (i = 0; i < count - n; ++i) d->table[i] = d->table[i + n];
        for (i = count - n; i < count; ++i) d->table[i] = NULL;
    } else if (dy < 0) {
        int32_t n = -dy < count ? -dy : count;
        for (i = count - 1; i >= n; --i) d->table[i] = d->table[i - n];
        for (i = 0; i < n; ++i) d->table[i] = NULL;
    }
    for (i = 0; i < count; ++i) {
        AoeDrawRowNodeRor *n;
        for (n = d->table[i]; n; n = n->free_link) {
            switch (n->key) {
            case 0:
                node_put32(n, 0x14, aoe_draw_row_node_dword_ror(n, 0x14) - dx);
                node_put32(n, 0x18, aoe_draw_row_node_dword_ror(n, 0x18) - dy);
                break;
            case 1:
                node_sub16(n, 5, dy);
                node_sub16(n, 7, dy);
                /* fall through */
            case 2:
                node_sub16(n, 4, dx);
                node_sub16(n, 6, dx);
                /* fall through */
            case 3:
            case 4:
                node_sub16(n, 0, dx);
                node_sub16(n, 1, dy);
                node_sub16(n, 2, dx);
                node_sub16(n, 3, dy);
                break;
            default:
                break;
            }
        }
    }
}
