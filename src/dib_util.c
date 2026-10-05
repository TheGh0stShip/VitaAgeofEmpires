#include "dib_util.h"

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
