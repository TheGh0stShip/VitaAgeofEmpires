#include "shape.h"

#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

#define TYPE_SLP 0x736c7020u /* 'slp ' */
#define TYPE_SHP 0x73687020u /* 'shp ' */

static const uint8_t *base_of(const AoeShapeRor *s) { return s->slp ? s->slp : s->shp; }

/* Port guard: `n` bytes at `p` lie inside the loaded data. */
static int inside(const AoeShapeRor *s, const uint8_t *p, size_t n) {
    const uint8_t *b = base_of(s);
    return b && p >= b && (size_t)(p - b) + n <= s->size;
}

static void clear(AoeShapeRor *s) {
    s->shp = NULL;
    s->frame = NULL;
    s->shp_header = NULL;
    s->shp_table = NULL;
    s->slp = NULL;
}

static void with_suffix(char *out, size_t size, const char *name,
                        const char *suffix) {
    size_t n = strlen(name);
    if (n >= size) n = size - 1;
    memcpy(out, name, n);
    out[n] = '\0';
    /* The last three characters become the suffix (the original writes
       at length - 3 whatever the name). */
    if (n >= 3) memcpy(out + n - 3, suffix, 4);
}

void aoe_shape_load_ror(AoeShapeRor *s, const char *name, int32_t id,
                        const AoeShapeOpsRor *ops) {
    char path[0x104];
    uint8_t *data;
    uint32_t size = 0;
    if (!s) return;
    clear(s);
    s->owned = 0;
    s->size = 0;
    s->slp_frames = NULL;
    if (!ops) return;
    if ((ops->loose_files || id < 0) && name && ops->load_file) {
        with_suffix(path, sizeof(path), name, "SLP");
        if (ops->load_count) ++*ops->load_count;
        data = ops->load_file(ops->context, path, &size);
        if (data) {
            s->slp = data;
            s->owned = 1;
            s->slp_frames = data + 0x20;
            s->size = size;
        }
        if (!s->slp) {
            with_suffix(path, sizeof(path), name, "SHP");
            data = ops->load_file(ops->context, path, &size);
            if (data) {
                s->shp = data;
                s->shp_header = data;
                s->shp_table = data + 8;
                s->owned = 1;
                s->size = size;
            }
        }
    }
    if (!s->slp && !s->shp && id >= 0 && ops->load_resource) {
        int32_t owned = 0;
        if (ops->load_count) ++*ops->load_count;
        data = ops->load_resource(ops->context, TYPE_SLP, id, &size, &owned);
        if (data) {
            s->owned = owned;
            s->size = size;
            s->slp = data;
            s->slp_frames = data + 0x20;
            s->shp = NULL;
            return;
        }
        data = ops->load_resource(ops->context, TYPE_SHP, id, &size, &owned);
        s->owned = owned;
        s->size = size;
        s->shp = data;
        if (data && size >= 8 && memcmp(data, "1.10", 4) == 0) {
            s->shp_header = data;
            s->shp_table = data + 8;
        }
    }
}

static void drop(const AoeShapeRor *s, const uint8_t *data,
                 const AoeShapeOpsRor *ops) {
    if (s->owned == 1) free((void *)data);
    if (s->owned == 0 && ops && ops->release) ops->release(ops->context, data, s->size);
}

void aoe_shape_free_ror(AoeShapeRor *s, const AoeShapeOpsRor *ops) {
    if (!s) return;
    if (s->slp) drop(s, s->slp, ops);
    if (s->shp) drop(s, s->shp, ops);
    clear(s);
}

int aoe_shape_loaded_ror(const AoeShapeRor *s) {
    return s && (s->shp || s->slp);
}

static int32_t count_at(const AoeShapeRor *s, const uint8_t *header) {
    return inside(s, header + 4, 4) ? aoe_le_i32(header + 4) : 0;
}

int aoe_shape_bad_ror(AoeShapeRor *s, int32_t frame) {
    if (!s || (!s->shp && !s->slp)) return 1;
    if (s->shp) {
        if (!s->shp_header) return 1;
        if (frame >= 0 && count_at(s, s->shp_header) <= frame) return 1;
        if (memcmp(s->shp_header, "1.10", 4) != 0) return 1;
    }
    if (!s->slp || frame < 0 || frame < count_at(s, s->slp)) return 0;
    return 1;
}

int32_t aoe_shape_count_ror(AoeShapeRor *s) {
    const uint8_t *h;
    if (aoe_shape_bad_ror(s, -1)) return 0;
    h = s->slp ? s->slp : s->shp_header;
    return count_at(s, h);
}

static const uint8_t *slp_frame(const AoeShapeRor *s, int32_t frame) {
    const uint8_t *f = s->slp_frames + 0x20 * (size_t)frame;
    return inside(s, f, 0x20) ? f : NULL;
}

static const uint8_t *shp_frame(AoeShapeRor *s, int32_t frame) {
    const uint8_t *entry = s->shp_table + 8 * (size_t)frame, *f;
    if (!inside(s, entry, 4)) return NULL;
    f = s->shp + aoe_le_u32(entry);
    if (!inside(s, f, 0x18)) return NULL;
    s->frame = f;
    return f;
}

int aoe_shape_size_ror(AoeShapeRor *s, int32_t frame, int16_t *w, int16_t *h) {
    if (!s || !w || !h) return 0;
    if (s->slp) {
        const uint8_t *f = slp_frame(s, frame);
        if (!f) return 0;
        *w = (int16_t)aoe_le_u32(f + 0x10);
        *h = (int16_t)aoe_le_u32(f + 0x14);
        return 1;
    }
    if (frame >= 0 && !aoe_shape_bad_ror(s, frame)) {
        const uint8_t *f = shp_frame(s, frame);
        uint32_t v;
        if (!f) return 0;
        v = aoe_le_u32(f);
        *w = (int16_t)(v >> 16);
        *h = (int16_t)(v & 0xffff);
        return 1;
    }
    return 0;
}

int aoe_shape_bounds_ror(AoeShapeRor *s, int32_t frame, int32_t *min_x,
                         int32_t *min_y, int32_t *max_x, int32_t *max_y) {
    const uint8_t *f;
    if (!s || frame < 0 || aoe_shape_bad_ror(s, frame)) return 0;
    if (s->slp) {
        if (!(f = slp_frame(s, frame))) return 0;
        *min_x = -aoe_le_i32(f + 0x18);
        *min_y = -aoe_le_i32(f + 0x1c);
        *max_x = aoe_le_i32(f + 0x10) - aoe_le_i32(f + 0x18) - 1;
        *max_y = aoe_le_i32(f + 0x14) - aoe_le_i32(f + 0x1c) - 1;
        return 1;
    }
    if (!(f = shp_frame(s, frame))) return 0;
    *min_x = aoe_le_i32(f + 8);
    *min_y = aoe_le_i32(f + 0xc);
    *max_x = aoe_le_i32(f + 0x10);
    *max_y = aoe_le_i32(f + 0x14);
    return 1;
}

/* Skip one SHP row (odd n: n literal bytes, or a skip byte when n is 0;
   even nonzero: a fill byte; 0 ends the row). */
static const uint8_t *shp_skip_row(const AoeShapeRor *s, const uint8_t *p) {
    for (;;) {
        uint8_t b;
        if (!inside(s, p, 1)) return NULL;
        b = *p;
        if (b & 1) {
            p += (b >> 1) ? 1 + (b >> 1) : 2;
        } else if (b & 0xfe) {
            p += 2;
        } else {
            return p + 1;
        }
    }
}

static int slp_hit(const AoeShapeRor *s, int32_t x, int32_t y,
                   const uint8_t *f) {
    const uint8_t *data = s->slp, *p;
    int32_t width = aoe_le_i32(f + 0x10), height = aoe_le_i32(f + 0x14);
    int32_t col, left, right;
    const uint8_t *outline;
    x += aoe_le_i32(f + 0x18);
    y += aoe_le_i32(f + 0x1c);
    if (x < 0 || y < 0 || x >= width || y >= height) return 0;
    outline = data + aoe_le_u32(f + 4) + 4 * (size_t)y;
    if (!inside(s, outline, 4)) return 0;
    left = (int16_t)(outline[0] | outline[1] << 8);
    right = (int16_t)(outline[2] | outline[3] << 8);
    if (left < 0 || x < left || width - right < x) return 0;
    p = data + aoe_le_u32(f) + 4 * (size_t)y;
    if (!inside(s, p, 4)) return 0;
    p = data + aoe_le_u32(p);
    col = left;
    for (;;) {
        uint8_t b;
        int32_t n;
        if (width <= col || x < col) return 0;
        if (!inside(s, p, 1)) return 0;
        b = *p;
        switch (b & 0xf) {
        case 0: case 4: case 8: case 0xc:
            n = b >> 2;
            col += n;
            if (x < col) return 1;
            p += 1 + n;
            break;
        case 1: case 5: case 9: case 0xd:
            col += b >> 2;
            p += 1;
            break;
        case 2:
            if (!inside(s, p, 2)) return 0;
            n = (int32_t)((uint32_t)b << 4 | p[1]);
            col += n;
            if (x < col) return 1;
            p += 2 + n;
            break;
        case 3:
            if (!inside(s, p, 2)) return 0;
            col += (int32_t)((uint32_t)b << 4 | p[1]);
            p += 2;
            break;
        case 6: case 7: case 0xa: case 0xb: {
            const uint8_t *q = p + 1;
            n = b >> 4;
            if (!n) {
                if (!inside(s, p, 2)) return 0;
                n = p[1];
                q = p + 2;
            }
            col += n;
            if (x < col) return 1;
            if ((b & 0xf) == 6) p = q + n;
            else if ((b & 0xf) == 0xb) p = q;
            else p = q + 1;
            break;
        }
        default:
            return 0;
        }
    }
}

static int shp_hit(AoeShapeRor *s, int32_t x, int32_t y, const uint8_t *f) {
    int32_t min_x = aoe_le_i32(f + 8), min_y = aoe_le_i32(f + 0xc);
    int32_t max_x = aoe_le_i32(f + 0x10), max_y = aoe_le_i32(f + 0x14);
    const uint8_t *p = f + 0x18;
    int32_t col, row;
    if (y < min_y || y > max_y || x < min_x || x > max_x) return 0;
    for (row = min_y; row < y; ++row)
        if (!(p = shp_skip_row(s, p))) return 0;
    col = min_x;
    for (;;) {
        uint8_t b;
        int32_t n;
        if (!inside(s, p, 1)) return 0;
        b = *p;
        n = b >> 1;
        if (b & 1) {
            if (!n) {
                if (!inside(s, p, 2)) return 0;
                col += p[1];
                if (x < col) return 0;
                p += 2;
            } else {
                col += n;
                if (x < col) return 1;
                p += 1 + n;
            }
        } else {
            if (!n) return 0;
            col += n;
            if (x < col) return 1;
            p += 2;
        }
    }
}

int aoe_shape_hit_ror(AoeShapeRor *s, int32_t x, int32_t y, int32_t frame) {
    const uint8_t *f;
    if (!s || frame < 0 || aoe_shape_bad_ror(s, frame)) return 0;
    if (s->slp) {
        f = slp_frame(s, frame);
        return f ? slp_hit(s, x, y, f) : 0;
    }
    f = shp_frame(s, frame);
    return f ? shp_hit(s, x, y, f) : 0;
}

/* ---- SHP blitters ---- */

static uint8_t *row_at(AoeShapeSurfaceRor *d, int32_t x, int32_t y) {
    if (d->direction < 1) y = d->height - 1 - y;
    return d->bits + (ptrdiff_t)y * d->pitch + x;
}

static int32_t step(const AoeShapeSurfaceRor *d) {
    return d->direction < 1 ? -d->pitch : d->pitch;
}

/* mode 0 copy, 1 through `table`, 2 remap the destination. */
static int blit(const AoeShapeRor *s, AoeShapeSurfaceRor *d, int32_t x,
                int32_t y, const uint8_t *table, int mode) {
    const uint8_t *f = s ? s->frame : NULL, *p;
    int32_t rows, r;
    uint8_t *line;
    if (!f || !d || !d->bits) return 0;
    rows = aoe_le_i32(f + 0x14) - aoe_le_i32(f + 0xc) + 1;
    p = f + 0x18;
    line = row_at(d, x, y);
    for (r = 0; r < rows; ++r, line += step(d)) {
        uint8_t *o = line;
        for (;;) {
            uint8_t b, n;
            if (!inside(s, p, 1)) return 1;
            b = *p;
            n = b >> 1;
            if (b & 1) {
                if (!n) {
                    o += p[1];
                    p += 2;
                    continue;
                }
                if (mode == 2) {
                    for (uint8_t i = 0; i < n; ++i) o[i] = table[o[i]];
                } else if (mode == 1) {
                    for (uint8_t i = 0; i < n; ++i) o[i] = table[p[1 + i]];
                } else {
                    memcpy(o, p + 1, n);
                }
                o += n;
                p += 1 + n;
            } else {
                if (!n) {
                    p += 1;
                    break;
                }
                if (mode == 2) {
                    for (uint8_t i = 0; i < n; ++i) o[i] = table[o[i]];
                } else {
                    memset(o, mode == 1 ? table[p[1]] : p[1], n);
                }
                o += n;
                p += 2;
            }
        }
    }
    return 1;
}

int aoe_shape_blit_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                       int32_t x, int32_t y) {
    return blit(s, dst, x, y, NULL, 0);
}

int aoe_shape_blit_xlat_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                            int32_t x, int32_t y, const uint8_t table[256]) {
    return table ? blit(s, dst, x, y, table, 1) : 0;
}

int aoe_shape_blit_shadow_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                              int32_t x, int32_t y,
                              const uint8_t table[256]) {
    return table ? blit(s, dst, x, y, table, 2) : 0;
}

/* ---- clipped SHP blitters ---- */

static void put(uint8_t *o, int mode, const uint8_t *table, uint8_t value) {
    if (mode == 2) *o = table[*o];
    else if (mode == 1) *o = table[value];
    else *o = value;
}

static int blit_clipped(const AoeShapeRor *s, AoeShapeSurfaceRor *d,
                        int32_t x, int32_t y, int32_t cl, int32_t ct,
                        int32_t cr, int32_t cb, const uint8_t *table,
                        int mode) {
    const uint8_t *f = s ? s->frame : NULL, *p;
    int32_t row;
    uint8_t *line;
    if (!f || !d || !d->bits) return 0;
    p = f + 0x18;
    for (row = y; row < ct; ++row)
        if (!(p = shp_skip_row(s, p))) return 1;
    if (cb < ct) return 1;
    line = row_at(d, 0, ct);
    for (row = ct; row <= cb; ++row, line += step(d)) {
        int32_t col = x;
        for (;;) {
            uint8_t b, n;
            int32_t i;
            if (!inside(s, p, 1)) return 1;
            b = *p;
            n = b >> 1;
            if (b & 1) {
                if (!n) {
                    if (!inside(s, p, 2)) return 1;
                    col += p[1];
                    p += 2;
                    continue;
                }
                if (!inside(s, p, 1 + (size_t)n)) return 1;
                for (i = 0; i < n; ++i, ++col)
                    if (col >= cl && col <= cr) put(line + col, mode, table, p[1 + i]);
                p += 1 + n;
            } else {
                if (!n) {
                    p += 1;
                    break;
                }
                if (!inside(s, p, 2)) return 1;
                for (i = 0; i < n; ++i, ++col)
                    if (col >= cl && col <= cr) put(line + col, mode, table, p[1]);
                p += 2;
            }
        }
    }
    return 1;
}

int aoe_shape_blit_clipped_ror(const AoeShapeRor *s, AoeShapeSurfaceRor *dst,
                               int32_t x, int32_t y, int32_t cl, int32_t ct,
                               int32_t cr, int32_t cb) {
    return blit_clipped(s, dst, x, y, cl, ct, cr, cb, NULL, 0);
}

int aoe_shape_blit_clipped_xlat_ror(const AoeShapeRor *s,
                                    AoeShapeSurfaceRor *dst, int32_t x,
                                    int32_t y, int32_t cl, int32_t ct,
                                    int32_t cr, int32_t cb,
                                    const uint8_t table[256]) {
    return table ? blit_clipped(s, dst, x, y, cl, ct, cr, cb, table, 1) : 0;
}

int aoe_shape_blit_clipped_shadow_ror(const AoeShapeRor *s,
                                      AoeShapeSurfaceRor *dst, int32_t x,
                                      int32_t y, int32_t cl, int32_t ct,
                                      int32_t cr, int32_t cb,
                                      const uint8_t table[256]) {
    return table ? blit_clipped(s, dst, x, y, cl, ct, cr, cb, table, 2) : 0;
}

/* ---- drawing ---- */

int aoe_shape_draw_ror(AoeShapeRor *s, AoeShapeSurfaceRor *dst, int32_t x,
                       int32_t y, int32_t frame, uint8_t mode,
                       const uint8_t *table, const AoeShapeDrawOpsRor *ops) {
    const uint8_t *f;
    int32_t x0, y0, x1, y1, l, t, r, b;
    if (!s || !dst || frame < 0 || aoe_shape_bad_ror(s, frame)) return 0;
    if (s->slp) {
        int batched = ops && ops->batching && *ops->batching;
        f = slp_frame(s, frame);
        if (!f) return 0;
        if (ops && ops->draw_slp)
            ops->draw_slp(ops->context, dst, s, frame, x - aoe_le_i32(f + 0x18),
                          y - aoe_le_i32(f + 0x1c), table, batched);
        if (!batched && ops && ops->drawn) ++*ops->drawn;
        return 1;
    }
    if (!s->shp || !(f = shp_frame(s, frame))) return 0;
    x0 = aoe_le_i32(f + 8) + x;
    y0 = aoe_le_i32(f + 0xc) + y;
    x1 = aoe_le_i32(f + 0x10) + x;
    y1 = aoe_le_i32(f + 0x14) + y;
    l = x0 < dst->clip_left ? dst->clip_left : x0;
    t = y0 < dst->clip_top ? dst->clip_top : y0;
    r = dst->clip_right < x1 ? dst->clip_right : x1;
    b = dst->clip_bottom < y1 ? dst->clip_bottom : y1;
    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (dst->width <= r) r = dst->width - 1;
    if (dst->height <= b) b = dst->height - 1;
    if (l > r || t > b) return 0;
    if (l == x0 && t == y0 && r == x1 && b == y1) {
        if (mode == 1 && table) return aoe_shape_blit_xlat_ror(s, dst, x0, y0, table);
        if (mode == 2 && table) return aoe_shape_blit_shadow_ror(s, dst, x0, y0, table);
        if (mode > 2) return 0;
        return aoe_shape_blit_ror(s, dst, x0, y0);
    }
    if (mode == 1 && table)
        return aoe_shape_blit_clipped_xlat_ror(s, dst, x0, y0, l, t, r, b, table);
    if (mode == 2 && table)
        return aoe_shape_blit_clipped_shadow_ror(s, dst, x0, y0, l, t, r, b, table);
    if (mode > 2) return 0;
    return aoe_shape_blit_clipped_ror(s, dst, x0, y0, l, t, r, b);
}
