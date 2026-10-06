#include "slp.h"

#include <stdlib.h>
#include <string.h>

static uint16_t u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int available(size_t size, size_t offset, size_t amount) {
    return offset <= size && amount <= size - offset;
}

/* The SLP layout as the executable reads it (patched RoR 1.0a): the
   shape loader keeps the frame records at +0x20 (0x0049f5f0); each record
   is 0x20 bytes: +0x00 the row command-offset table, +0x04 the row
   outline table, +0x10 width, +0x14 height, +0x18 / +0x1c the hotspot
   (the size 0x0049fa00, the bounds 0x0049fac0, the pixel test 0x0049fb90
   and the draw 0x0049fe00, which passes them to the rasterizer
   0x005409a0). The frame count is the 32-bit value at +0x04 (0x0049fa90).
   An outline entry is two 16-bit edges (left, right); a row whose left
   edge has bit 15 set (0x8000) is not drawn (the rasterizer's
   `test eax, 0x8000`). The executable does not check the version text;
   every SLP in the installed archives starts with "2.0N", which this
   reader requires. */
int slp_frame_count(const uint8_t *data, size_t size, uint32_t *count) {
    uint32_t n;
    if (!data || !count || !available(size, 0, 32) ||
        memcmp(data, "2.0N", 4) != 0) return 0;
    n = u32(data + 4);
    if (!n || n > (size - 32) / 32) return 0;
    *count = n;
    return 1;
}

void slp_frame_summarize(SlpFrame *frame) {
    uint8_t mask = 0;
    if (!frame || !frame->pixels) return;
    free(frame->row_spans);
    frame->row_spans = malloc((size_t)frame->height * 2 * sizeof(uint16_t));
    for (uint32_t y = 0; y < frame->height; ++y) {
        const SlpPixel *row = &frame->pixels[(size_t)y * frame->width];
        uint32_t lo = 0, hi, x;
        while (lo < frame->width && row[lo].kind == SLP_TRANSPARENT) ++lo;
        hi = lo;
        while (hi < frame->width && row[hi].kind == SLP_COLOR) ++hi;
        x = hi;
        while (x < frame->width && row[x].kind == SLP_TRANSPARENT) ++x;
        for (uint32_t i = 0; i < frame->width; ++i)
            mask |= (uint8_t)(1u << row[i].kind);
        if (frame->row_spans) {
            int simple = x == frame->width && hi < SLP_ROW_MIXED;
            frame->row_spans[2 * y] = simple ? (uint16_t)lo : SLP_ROW_MIXED;
            frame->row_spans[2 * y + 1] =
                simple ? (uint16_t)hi : SLP_ROW_MIXED;
        }
    }
    frame->kind_mask = mask;
    frame->kinds_known = 1;
}

void slp_free_frame(SlpFrame *frame) {
    if (!frame) return;
    free(frame->pixels);
    free(frame->row_spans);
    memset(frame, 0, sizeof(*frame));
}

int slp_blit_terrain_rgba(const SlpFrame *frame, const Palette *palette,
                          uint8_t *rgba, size_t rgba_size,
                          uint32_t width, uint32_t height, size_t stride,
                          int32_t x, int32_t y) {
    size_t pixels;
    if (!frame || !frame->pixels || !palette || !rgba || !frame->width ||
        !frame->height || !width || !height || !stride ||
        frame->width > SIZE_MAX / frame->height ||
        (size_t)width > stride / 4 ||
        (size_t)width * 4 > rgba_size ||
        (size_t)(height - 1) >
            (rgba_size - (size_t)width * 4) / stride) return 0;
    pixels = (size_t)frame->width * frame->height;
    for (size_t i = 0; i < pixels; ++i)
        if (frame->pixels[i].kind != SLP_TRANSPARENT &&
            frame->pixels[i].kind != SLP_COLOR) return 0;
    for (uint32_t sy = 0; sy < frame->height; ++sy) {
        int64_t dy = (int64_t)y + sy;
        if (dy < 0 || dy >= height) continue;
        for (uint32_t sx = 0; sx < frame->width; ++sx) {
            int64_t dx = (int64_t)x + sx;
            const SlpPixel *pixel;
            PaletteColor color;
            uint8_t *out;
            if (dx < 0 || dx >= width) continue;
            pixel = &frame->pixels[(size_t)sy * frame->width + sx];
            if (pixel->kind == SLP_TRANSPARENT) continue;
            color = palette->colors[pixel->index];
            out = rgba + (size_t)dy * stride + (size_t)dx * 4;
            out[0] = color.r;
            out[1] = color.g;
            out[2] = color.b;
            out[3] = 255;
        }
    }
    return 1;
}

int slp_blit_graphic_rgba(const SlpFrame *frame, const Palette *palette,
                          uint8_t *rgba, size_t rgba_size,
                          uint32_t width, uint32_t height, size_t stride,
                          int32_t anchor_x, int32_t anchor_y,
                          int mirror_horizontal,
                          void (*apply_special)(void *context,
                                                const SlpPixel *pixel,
                                                uint8_t rgba[4]),
                          void *context) {
    if (!frame || !frame->pixels || !palette || !rgba ||
        !frame->width || !frame->height || !width || !height || !stride ||
        frame->width > SIZE_MAX / frame->height ||
        (size_t)width > stride / 4 ||
        (size_t)width * 4 > rgba_size ||
        (size_t)(height - 1) >
            (rgba_size - (size_t)width * 4) / stride)
        return 0;
    size_t count = (size_t)frame->width * frame->height;
    for (size_t i = 0; i < count; ++i) {
        uint8_t kind = frame->pixels[i].kind;
        if (kind > SLP_OUTLINE2 ||
            (kind >= SLP_PLAYER && !apply_special))
            return 0;
    }
    int64_t left = mirror_horizontal ?
        (int64_t)anchor_x + frame->hotspot_x - frame->width :
        (int64_t)anchor_x - frame->hotspot_x;
    int64_t top = (int64_t)anchor_y - frame->hotspot_y;
    for (uint32_t y = 0; y < frame->height; ++y) {
        int64_t dy = top + y;
        if (dy < 0 || dy >= height) continue;
        for (uint32_t x = 0; x < frame->width; ++x) {
            int64_t dx = left + x;
            if (dx < 0 || dx >= width) continue;
            uint32_t sx = mirror_horizontal ? frame->width - 1 - x : x;
            const SlpPixel *pixel =
                &frame->pixels[(size_t)y * frame->width + sx];
            if (pixel->kind == SLP_TRANSPARENT) continue;
            uint8_t *out = rgba + (size_t)dy * stride + (size_t)dx * 4;
            if (pixel->kind == SLP_COLOR) {
                PaletteColor color = palette->colors[pixel->index];
                out[0] = color.r;
                out[1] = color.g;
                out[2] = color.b;
                out[3] = 255;
            } else {
                apply_special(context, pixel, out);
            }
        }
    }
    return 1;
}

int slp_blit_graphic_indexed(const SlpFrame *frame,
                             uint8_t *indices, size_t indices_size,
                             uint32_t width, uint32_t height, size_t stride,
                             int32_t anchor_x, int32_t anchor_y,
                             int mirror_horizontal, uint8_t player_color,
                             const uint8_t shadow_table[256]) {
    if (!frame || !frame->pixels || !indices || !frame->width ||
        !frame->height || !width || !height || !stride ||
        frame->width > SIZE_MAX / frame->height ||
        (size_t)width > stride || (size_t)width > indices_size ||
        (size_t)(height - 1) >
            (indices_size - (size_t)width) / stride)
        return 0;
    if (frame->kinds_known) {
        if (frame->kind_mask & ~((1u << SLP_TRANSPARENT) | (1u << SLP_COLOR) |
                                 (1u << SLP_PLAYER) | (1u << SLP_SHADOW)))
            return 0;
        if ((frame->kind_mask & (1u << SLP_SHADOW)) && !shadow_table)
            return 0;
    } else {
        size_t count = (size_t)frame->width * frame->height;
        for (size_t i = 0; i < count; ++i) {
            uint8_t kind = frame->pixels[i].kind;
            if (kind > SLP_SHADOW ||
                (kind == SLP_SHADOW && !shadow_table))
                return 0;
        }
    }
    int64_t left = mirror_horizontal ?
        (int64_t)anchor_x + frame->hotspot_x - frame->width :
        (int64_t)anchor_x - frame->hotspot_x;
    int64_t top = (int64_t)anchor_y - frame->hotspot_y;
    uint8_t bank = (uint8_t)(((unsigned)player_color + 1u) * 16u);
    /* Visible destination columns [x0, x1) and rows [y0, y1) in frame
       coordinates; each pixel is written exactly as a per-pixel bounds
       test would. */
    int64_t x0 = left < 0 ? -left : 0, x1 = (int64_t)width - left;
    int64_t y0 = top < 0 ? -top : 0, y1 = (int64_t)height - top;
    if (x1 > frame->width) x1 = frame->width;
    if (y1 > frame->height) y1 = frame->height;
    if (x0 >= x1 || y0 >= y1) return 1;
    for (uint32_t y = (uint32_t)y0; y < (uint32_t)y1; ++y) {
        const SlpPixel *row = &frame->pixels[(size_t)y * frame->width];
        uint8_t *dest = indices + (size_t)(top + y) * stride +
                        (size_t)(left + x0);
        uint32_t n = (uint32_t)(x1 - x0);
        if (frame->row_spans &&
            frame->row_spans[2 * y] != SLP_ROW_MIXED) {
            /* Transparent outside one color run: copy the visible part. */
            uint32_t lo = frame->row_spans[2 * y];
            uint32_t hi = frame->row_spans[2 * y + 1];
            if (mirror_horizontal) {
                uint32_t mlo = frame->width - hi, mhi = frame->width - lo;
                lo = mlo;
                hi = mhi;
            }
            if (lo < (uint32_t)x0) lo = (uint32_t)x0;
            if (hi > (uint32_t)x1) hi = (uint32_t)x1;
            if (lo >= hi) continue;
            dest += lo - (uint32_t)x0;
            if (!mirror_horizontal) {
                for (uint32_t x = lo; x < hi; ++x) *dest++ = row[x].index;
            } else {
                for (uint32_t x = lo; x < hi; ++x)
                    *dest++ = row[frame->width - 1 - x].index;
            }
            continue;
        }
        for (uint32_t k = 0; k < n; ++k, ++dest) {
            uint32_t x = (uint32_t)x0 + k;
            uint32_t sx = mirror_horizontal ? frame->width - 1 - x : x;
            const SlpPixel *pixel = &row[sx];
            if (pixel->kind == SLP_COLOR) *dest = pixel->index;
            else if (pixel->kind == SLP_PLAYER)
                *dest = (uint8_t)(pixel->index | bank);
            else if (pixel->kind == SLP_SHADOW)
                *dest = shadow_table[*dest];
        }
    }
    return 1;
}

static int emit(SlpFrame *frame, uint32_t y, uint32_t *x, uint32_t end,
                uint32_t count, uint8_t kind, const uint8_t *colors) {
    if (count > end - *x) return 0;
    for (uint32_t i = 0; i < count; ++i) {
        SlpPixel *p = &frame->pixels[(size_t)y * frame->width + *x + i];
        p->kind = kind;
        p->index = colors ? colors[i] : 0;
    }
    *x += count;
    return 1;
}

/* A run length from a command byte: the 12-bit form (low nibble 2 / 3)
   keeps the byte's high nibble above the next byte (0x00541100 /
   0x0054114b: shl ecx, 4 then mov cl, [esi]); the counted forms (6, 7, 0xa,
   0xb) take the high nibble or, when it is 0, the next byte. 0 when the
   data ends. */
static int long_count(const uint8_t *data, size_t size, size_t *pos, uint8_t op,
                      uint32_t *n) {
    if (!available(size, *pos, 1)) return 0;
    *n = ((uint32_t)(op & 0xf0) << 4) | data[(*pos)++];
    return 1;
}
static int nibble_count(const uint8_t *data, size_t size, size_t *pos, uint8_t op,
                        uint32_t *n) {
    *n = op >> 4;
    if (*n) return 1;
    if (!available(size, *pos, 1)) return 0;
    *n = data[(*pos)++];
    return 1;
}

/* 0x00541b60 (for 0x4e): the pixel count of the command at `pos`, which
   is consumed without drawing. Its table (0x00540600 / 0x00540640) gives
   0 for an end-of-row byte and for the extended forms other than 0x1e
   (1) and 0x2e (the next byte), whose entries are int3 traps. */
static int skipped_command(const uint8_t *data, size_t size, size_t *pos, uint32_t *n) {
    uint8_t op;
    if (!available(size, *pos, 1)) return 0;
    op = data[(*pos)++];
    switch (op & 15) {
    case 0: case 4: case 8: case 12:
        *n = op >> 2;
        if (!available(size, *pos, *n)) return 0;
        *pos += *n;
        return 1;
    case 1: case 5: case 9: case 13: *n = op >> 2; return 1;
    case 2:
        if (!long_count(data, size, pos, op, n) || !available(size, *pos, *n)) return 0;
        *pos += *n;
        return 1;
    case 3: return long_count(data, size, pos, op, n);
    case 6:
        if (!nibble_count(data, size, pos, op, n) || !available(size, *pos, *n)) return 0;
        *pos += *n;
        return 1;
    case 7: case 10:
        if (!nibble_count(data, size, pos, op, n) || !available(size, *pos, 1)) return 0;
        ++*pos;
        return 1;
    case 11: return nibble_count(data, size, pos, op, n);
    case 14:
        if (op == 0x1e) { *n = 1; return 1; }
        if (op == 0x2e) {
            if (!available(size, *pos, 1)) return 0;
            *n = data[(*pos)++];
            return 1;
        }
        return 0;   /* an int3 entry in the original */
    default:
        *n = 0;     /* 0x?f */
        return 1;
    }
}

/* One row as the sprite rasterizer (0x005409a0, its command loop
   0x005410a0 dispatching on the low nibble through 0x0053f440) reads it:
     0, 4, 8, 0xc  colours, count = byte >> 2, then that many indices
     1, 5, 9, 0xd  transparent, count = byte >> 2 (no further byte)
     2 / 3         colours / transparent, the 12-bit count (long_count)
     6             player colours, counted, then that many indices
     7             one index repeated, counted
     0xa           one player-colour index repeated, counted
     0xb           shadow, counted (no index)
     0xe           by the high nibble (0x0053f4c0): 0x1e one outline pixel,
                   0x2e outline pixels (count in the next byte), 0x3e
                   nothing, 0x4e the next command's pixels left
                   transparent (0x00541b60); the others are int3 traps
     0xf           end of the row.
   The installed archives use none of the 0xe forms and no transparent run
   of count 0 (all 742,629 drawn rows of the 16,718 frames end exactly at
   width - right edge under these rules). */
static int decode_row(const uint8_t *data, size_t size, size_t pos,
                      SlpFrame *frame, uint32_t y, uint32_t left,
                      uint32_t right) {
    uint32_t x = left, end = frame->width - right;
    /* Every command consumes bytes or finishes a row; the limit also prevents
       malformed data from spending unbounded time in this loop. */
    for (size_t commands = 0; commands < size && pos < size; ++commands) {
        uint8_t op = data[pos++];
        uint32_t n = 0;
        switch (op & 15) {
        case 15:
            return x == end;
        case 0: case 4: case 8: case 12:
            n = op >> 2;
            if (!available(size, pos, n) || !emit(frame, y, &x, end, n, SLP_COLOR, data + pos))
                return 0;
            pos += n;
            break;
        case 1: case 5: case 9: case 13:
            if (!emit(frame, y, &x, end, op >> 2, SLP_TRANSPARENT, NULL)) return 0;
            break;
        case 2:
            if (!long_count(data, size, &pos, op, &n) || !available(size, pos, n) ||
                !emit(frame, y, &x, end, n, SLP_COLOR, data + pos))
                return 0;
            pos += n;
            break;
        case 3:
            if (!long_count(data, size, &pos, op, &n) ||
                !emit(frame, y, &x, end, n, SLP_TRANSPARENT, NULL))
                return 0;
            break;
        case 6:
            if (!nibble_count(data, size, &pos, op, &n) || !available(size, pos, n) ||
                !emit(frame, y, &x, end, n, SLP_PLAYER, data + pos))
                return 0;
            pos += n;
            break;
        case 7: case 10: {
            uint8_t kind = (op & 15) == 7 ? SLP_COLOR : SLP_PLAYER;
            if (!nibble_count(data, size, &pos, op, &n) || !available(size, pos, 1) ||
                n > end - x)
                return 0;
            for (uint32_t i = 0; i < n; ++i) {
                SlpPixel *p = &frame->pixels[(size_t)y * frame->width + x + i];
                p->kind = kind;
                p->index = data[pos];
            }
            x += n;
            ++pos;
            break;
        }
        case 11:
            if (!nibble_count(data, size, &pos, op, &n) ||
                !emit(frame, y, &x, end, n, SLP_SHADOW, NULL))
                return 0;
            break;
        default:   /* 0xe */
            switch (op >> 4) {
            case 1:
                if (!emit(frame, y, &x, end, 1, SLP_OUTLINE1, NULL)) return 0;
                break;
            case 2:
                if (!available(size, pos, 1)) return 0;
                n = data[pos++];
                if (!emit(frame, y, &x, end, n, SLP_OUTLINE1, NULL)) return 0;
                break;
            case 3:
                break;
            case 4:
                if (!skipped_command(data, size, &pos, &n) ||
                    !emit(frame, y, &x, end, n, SLP_TRANSPARENT, NULL))
                    return 0;
                break;
            default:
                return 0;   /* an int3 entry in the original */
            }
            break;
        }
    }
    return 0;
}

int slp_decode_frame(const uint8_t *data, size_t size, uint32_t frame_index,
                     SlpFrame *frame) {
    uint32_t count, width, height, outline, commands;
    size_t off;
    if (!frame) return 0;
    memset(frame, 0, sizeof(*frame));
    if (!slp_frame_count(data, size, &count) || frame_index >= count) return 0;
    off = 32 + (size_t)frame_index * 32;
    commands = u32(data + off);
    outline = u32(data + off + 4);
    width = u32(data + off + 16);
    height = u32(data + off + 20);
    if (!width || !height || width > 4096 || height > 4096 ||
        (uint64_t)width * height > 4000000 ||
        !available(size, outline, (size_t)height * 4) ||
        !available(size, commands, (size_t)height * 4)) return 0;
    frame->pixels = calloc((size_t)width * height, sizeof(SlpPixel));
    if (!frame->pixels) return 0;
    frame->width = width;
    frame->height = height;
    frame->hotspot_x = (int32_t)u32(data + off + 24);
    frame->hotspot_y = (int32_t)u32(data + off + 28);

    for (uint32_t y = 0; y < height; ++y) {
        uint16_t left = u16(data + outline + (size_t)y * 4);
        uint16_t right = u16(data + outline + (size_t)y * 4 + 2);
        uint32_t pos = u32(data + commands + (size_t)y * 4);
        if (left == 0x8000 || right == 0x8000) continue;
        if (left > width || right > width - left || pos >= size ||
            !decode_row(data, size, pos, frame, y, left, right)) {
            slp_free_frame(frame);
            return 0;
        }
    }
    slp_frame_summarize(frame);
    return 1;
}
