#include "dib_util.h"
#include "le_bytes.h"

#include <stdlib.h>
#include <string.h>

static int32_t rd32(const uint8_t *p) {
    return (int32_t)((uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
                     (uint32_t)p[3] << 24);
}
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static void wr32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static void wr16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

void aoe_dib_layout_ror(const uint8_t *info, uint32_t *pitch, int32_t *height,
                        int32_t *direction) {
    int32_t h = rd32(info + 8);
    *pitch = ((uint32_t)rd32(info + 4) + 3u) & ~3u;
    *height = h;
    if (h < 0) {
        *height = -h;
        *direction = 1;
        return;
    }
    *direction = -1;
}

int aoe_dib_clip_ror(const uint8_t *dest_info, const int32_t *area, int32_t *x, int32_t *y,
                     int32_t *w, int32_t *h, int32_t *sx, int32_t *sy, int32_t *sw,
                     int32_t *sh, int flip_x, int flip_y) {
    int32_t x0, y0, x1, y1, step_x, step_y, d;
    if (!area) {
        x0 = 0;
        y0 = 0;
        x1 = rd32(dest_info + 4);
        y1 = rd32(dest_info + 8);
    } else {
        x0 = area[0];
        y0 = area[1];
        x1 = area[2] + 1;
        y1 = area[3] + 1;
    }
    if (y1 < 0) y1 = -y1;
    --x1;
    --y1;
    if (!(*x <= x1 && *y <= y1 && x0 <= *x - 1 + *w && y0 <= *h - 1 + *y && 0 < *w && 0 < *h))
        return 0;
    step_x = (*sw << 8) / *w;
    step_y = (*sh << 8) / *h;
    if (*x < x0) {
        d = x0 - *x;
        *x = x0;
        *w -= d;
        d = (d * 0x100 * step_x) >> 0x10;
        if (!flip_x) *sx += d;
        *sw -= d;
    }
    if (*y < y0) {
        d = y0 - *y;
        *y = y0;
        *h -= d;
        d = (d * 0x100 * step_y) >> 0x10;
        if (!flip_y) *sy += d;
        *sh -= d;
    }
    if (x1 < *w - 1 + *x) {
        d = *w - x1 - 1 + *x;
        *w -= d;
        d = (d * 0x100 * step_x) >> 0x10;
        *sw -= d;
        if (flip_x) *sx += d;
    }
    if (y1 < *h - 1 + *y) {
        d = *y - 1 + (*h - y1);
        *h -= d;
        d = (d * 0x100 * step_y) >> 0x10;
        *sh -= d;
        if (flip_y) *sy += d;
    }
    return 0 < *w && 0 < *h && 0 < *sw && 0 < *sh;
}

int aoe_dib_spans_ror(const uint8_t *info, AoeDibSpanRor **spans, const uint8_t *bits) {
    uint32_t pitch;
    int32_t height, direction, width = rd32(info + 4), r, any = 0;
    AoeDibSpanRor *t;
    aoe_dib_layout_ror(info, &pitch, &height, &direction);
    t = malloc((size_t)(height > 0 ? height : 1) * sizeof *t);
    *spans = t;
    if (!t) return 1;
    for (r = 0; r < height; ++r) {
        int32_t row = direction != 1 ? height - r - 1 : r;
        const uint8_t *line = bits + (size_t)row * pitch;
        int32_t first = width, last, i, empty = 1, solid = 1;
        for (i = 0; i < width; ++i) {
            if (line[i] == 0) {
                any = 1;
            } else if (empty) {
                empty = 0;
                first = i;
            }
        }
        last = first;
        if (!empty)
            for (i = first + 1; i < width; ++i)
                if (line[i] != 0) last = i;
        if (!empty)
            for (i = first; i <= last; ++i)
                if (line[i] == 0) {
                    solid = 0;
                    break;
                }
        t[r].empty = empty;
        t[r].solid = solid;
        t[r].first = first;
        t[r].last = last;
    }
    if (!any) {
        free(t);
        *spans = NULL;
    }
    return any;
}

uint8_t *aoe_dib_read_header_ror(const uint8_t *data, size_t size, size_t *pos,
                                 size_t *offset, size_t *block_size) {
    size_t start = *pos, colors, header_size;
    uint8_t head[40];
    int core;
    uint32_t bits_offset = 0;
    uint8_t *out;
    *offset = 0;
    if (size - start < 14) return NULL;
    if (rd16(data + start) == 0x4d42) {
        bits_offset = (uint32_t)rd32(data + start + 10);
        *pos = start + 14;
    }
    if (size - *pos < 40) return NULL;
    memcpy(head, data + *pos, 40);
    *pos += 40;
    core = rd32(head) == 0xc;
    if (core) {
        uint16_t w = rd16(head + 4), h = rd16(head + 6), planes = rd16(head + 8),
                 count = rd16(head + 10);
        memset(head, 0, sizeof head);
        wr32(head, 0x28);
        wr32(head + 4, w);
        wr32(head + 8, h);
        wr16(head + 12, planes);
        wr16(head + 14, count);
        *pos -= 0x1c;
    }
    colors = (size_t)(uint32_t)rd32(head + 32);
    if (colors == 0 && rd16(head + 14) < 9) colors = (size_t)1 << rd16(head + 14);
    if (rd32(head + 20) == 0)
        wr32(head + 20, (((uint32_t)rd16(head + 14) * (uint32_t)rd32(head + 4) + 0x1f) >> 3 &
                         0x1ffffffcu) *
                            (uint32_t)rd32(head + 8));
    header_size = (size_t)(uint32_t)rd32(head);
    *block_size = header_size + colors * 4;
    out = calloc(*block_size ? *block_size : 1, 1);
    if (!out) return NULL;
    memcpy(out, head, 40);
    if (colors) {
        uint8_t *pal = out + header_size;
        if (core) {
            size_t i;
            if (size - *pos < colors * 3) {
                free(out);
                return NULL;
            }
            for (i = 0; i < colors; ++i) {
                pal[4 * i] = data[*pos + 3 * i];
                pal[4 * i + 1] = data[*pos + 3 * i + 1];
                pal[4 * i + 2] = data[*pos + 3 * i + 2];
                pal[4 * i + 3] = 0;
            }
            *pos += colors * 3;
        } else {
            if (size - *pos < colors * 4) {
                free(out);
                return NULL;
            }
            memcpy(pal, data + *pos, colors * 4);
            *pos += colors * 4;
        }
    }
    if (bits_offset) {
        *offset = start + bits_offset;
        *pos = *offset;
    }
    return out;
}

uint8_t *aoe_dib_read_ror(const uint8_t *data, size_t size, size_t *block_size) {
    size_t pos = 0, offset, head_size, image;
    uint8_t *head = aoe_dib_read_header_ror(data, size, &pos, &offset, &head_size), *grown;
    if (!head) return NULL;
    image = (size_t)(uint32_t)rd32(head + 20);
    grown = realloc(head, head_size + image);
    if (!grown) {
        free(head);
        return NULL;
    }
    if (size - pos < image) image = size - pos;
    memcpy(grown + head_size, data + pos, image);
    *block_size = head_size + image;
    return grown;
}

void aoe_dib_free_ror(uint8_t *dib) { free(dib); }

void aoe_dib_remap8_ror(uint8_t *p, int32_t n, const uint8_t *table) {
    for (; n != 0; --n, ++p) *p = table[*p];
}

void aoe_dib_remap4_ror(uint8_t *p, int32_t n, const uint8_t *table) {
    for (; n != 0; --n, ++p) *p = (uint8_t)(table[*p >> 4] << 4 | table[*p & 0xf]);
}

void aoe_dib_remap_rle8_ror(uint8_t *p, const uint8_t *table) {
    for (;;) {
        uint8_t count = p[0], value = p[1];
        if (count != 0) {
            p[1] = table[value];
            p += 2;
            continue;
        }
        if (value == 0) {
            p += 2;
            continue;
        }
        if (value == 1) return;
        if (value == 2) {
            p += 4;
            continue;
        }
        {
            uint8_t n = value;
            p += 2;
            for (; n != 0; --n, ++p) *p = table[*p];
            if (value & 1) ++p;
        }
    }
}

void aoe_dib_nothing_ror(void) {}

void aoe_dib_copy_back_ror(uint8_t *dst, const uint8_t *src, int32_t n) {
    for (; n != 0; --n) dst[n - 1] = src[n - 1];
}

/* 0x0042e460 */
int aoe_dib_blit_scaled_ror(AoeDibBlitRor *bl) {
    uint32_t dpitch, spitch;
    int32_t dheight, sheight, ddir, sdir, i, row, drow, srow, sstep, dstep;
    int32_t xoff[1024];
    /* A span table whose first dword is 0 sends the original to the
       unscaled 0x42e1f0; the caller routes that case (-1 here). */
    if (bl->spans && bl->spans[0].empty == 0 && bl->spans[0].solid == 0 && bl->spans[0].first == 0 &&
        bl->spans[0].last == 0)
        return -1;
    if (bl->transparent < 0) bl->transparent = 0;
    if (bl->clip && !aoe_dib_clip_ror(bl->dest_info, NULL, &bl->dest_x, &bl->dest_y, &bl->w, &bl->h,
                                      &bl->sx, &bl->sy, &bl->sw, &bl->sh, bl->flip_x, bl->flip_y))
        return 0;
    if (bl->w <= 0 || bl->h <= 0 || bl->w > 1024) return 0;
    sstep = (bl->sw << 8) / bl->w;
    dstep = (bl->sh << 8) / bl->h;
    aoe_dib_layout_ror(bl->dest_info, &dpitch, &dheight, &ddir);
    aoe_dib_layout_ror(bl->src_info, &spitch, &sheight, &sdir);
    if (bl->flip_y) sdir = -sdir;
    drow = ddir == 1 ? bl->dest_y : dheight - 1 - bl->dest_y;
    if (!bl->flip_x) {
        for (i = 0; i < bl->w; ++i) xoff[i] = (int32_t)(((int64_t)bl->sx << 8) + (int64_t)i * sstep) >> 8;
    } else {
        for (i = 0; i < bl->w; ++i) xoff[bl->w - 1 - i] = (int32_t)(((int64_t)bl->sx << 8) + (int64_t)i * sstep) >> 8;
    }
    for (row = 0, srow = bl->sy << 8; row < bl->h; ++row, srow += dstep, drow += ddir) {
        int32_t sr = srow >> 8, sline = sdir == 1 ? sr : sheight - 1 - sr;
        uint8_t *d = bl->dest_bits + (size_t)drow * dpitch + bl->dest_x;
        const uint8_t *s = bl->src_bits + (size_t)sline * spitch;
        if (bl->spans && !bl->flip_x) {
            const AoeDibSpanRor *sp = &bl->spans[sr];
            int32_t from, to;
            if (sp->empty) continue;
            from = sp->first < bl->sx ? 0 : ((sp->first - bl->sx) * (bl->w << 8) / bl->sw) >> 8;
            to = ((sp->last - bl->sx + 1) * (bl->w << 8) / bl->sw) >> 8;
            if (to > bl->w) to = bl->w;
            if (from >= to) continue;
            if ((bl->w << 8) / bl->sw == 0x100 && sp->solid) {
                memcpy(d + from, s + xoff[from], (size_t)(to - from));
            } else {
                for (i = from; i < to; ++i) {
                    uint8_t px = s[xoff[i]];
                    if (px != bl->transparent) d[i] = px;
                }
            }
        } else {
            for (i = 0; i < bl->w; ++i) {
                uint8_t px = s[xoff[i]];
                if (px != bl->transparent) d[i] = px;
            }
        }
    }
    return 1;
}

/* 0x0042f190 */
uint8_t *aoe_dib_match_palette_ror(uint8_t *dib, const AoeDibPaletteOpsRor *o, int remap_transparent,
                                   int from_last) {
    uint32_t header = aoe_le_u32(dib), width = aoe_le_u32(dib + 4);
    int32_t height = (int32_t)aoe_le_u32(dib + 8), colours, image, bits_off, i, pal = o->palette_size;
    uint16_t bpp = aoe_le_u16(dib + 14);
    uint32_t compression = aoe_le_u32(dib + 16);
    uint8_t table[256], transparent = 0, repl = 0;
    uint8_t *rgb = dib + header;
    if (height < 0) height = -height;
    colours = (int32_t)aoe_le_u32(dib + 32);
    if (colours == 0 && bpp < 9) colours = 1 << bpp;
    image = (int32_t)aoe_le_u32(dib + 20);
    if (image == 0) image = (int32_t)(((width * bpp + 31) >> 3) & ~3u) * height;
    bits_off = compression == 3 ? (int32_t)header + 12 : (int32_t)header + colours * 4;
    if (remap_transparent) {
        int32_t r = rgb[2], g = rgb[1], b = rgb[0], step = 1, pass = 0;
        transparent = from_last ? 0 : dib[bits_off + (int32_t)((width + 3) & ~3u) * (height - 1)];
        for (;;) {
            r += step; g += step; b += step;
            if (r > 255 || g > 255 || b > 255 || r < 0 || g < 0 || b < 0) {
                if (pass) break;
                pass = 1; step = -1; r = rgb[2]; g = rgb[1]; b = rgb[0];
                continue;
            }
            repl = o->nearest(o->context, (uint8_t)r, (uint8_t)g, (uint8_t)b);
            if (repl) break;
        }
    }
    for (i = 0; i < colours && i < 256; ++i) {
        const uint8_t *c = rgb + i * 4;
        if (remap_transparent && i == transparent) { table[i] = 0; continue; }
        table[i] = o->nearest(o->context, c[2], c[1], c[0]);
        if (remap_transparent && table[i] == 0) table[i] = repl;
    }
    for (; i < 256; ++i) table[i] = (uint8_t)i;
    aoe_le_put_u32(dib + 32, (uint32_t)pal);
    if (colours < pal) {
        size_t size = header + (size_t)pal * 4 + (size_t)image;
        int32_t new_off = compression == 3 ? (int32_t)header + 12 : (int32_t)header + pal * 4;
        dib = o->regrow(o->context, dib, size);
        memmove(dib + new_off, dib + bits_off, (size_t)image);
        bits_off = new_off;
    } else if (colours > pal) {
        int32_t new_off = compression == 3 ? (int32_t)header + 12 : (int32_t)header + pal * 4;
        memmove(dib + new_off, dib + bits_off, (size_t)image);
        bits_off = new_off;
    }
    if (compression == 0) {
        if (bpp == 8) aoe_dib_remap8_ror(dib + bits_off, image, table);
        else aoe_dib_remap4_ror(dib + bits_off, image, table);
    } else if (compression == 1) {
        aoe_dib_remap_rle8_ror(dib + bits_off, table);
    } else if (compression == 2) {
        aoe_dib_nothing_ror();
    }
    rgb = dib + header;
    for (i = 0; i < pal; ++i) {
        uint8_t r, g, b;
        o->entry(o->context, i, &r, &g, &b);
        rgb[i * 4 + 2] = r; rgb[i * 4 + 1] = g; rgb[i * 4] = b; rgb[i * 4 + 3] = 0;
    }
    return dib;
}
