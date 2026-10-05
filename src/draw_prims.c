#include "draw_prims.h"

#include <string.h>

/* The offset of row y's first pixel. */
static int32_t row_offset(const AoeShapeSurfaceRor *s, int32_t y) {
    if (s->direction == 1) return s->pitch * y;
    return (s->height - y - 1) * s->pitch;
}

void aoe_draw_hline_ror(AoeShapeSurfaceRor *s, int32_t x, int32_t y,
                        int32_t len, uint8_t color) {
    if (!s->bits || x > s->clip_right || y < s->clip_top ||
        y > s->clip_bottom)
        return;
    if (x < s->clip_left) {
        len += x - s->clip_left;
        x = s->clip_left;
    }
    if (x + len - 1 > s->clip_right) len = s->clip_right - x + 1;
    if (len > 0) memset(s->bits + row_offset(s, y) + x, color, (size_t)len);
}

void aoe_draw_vline_ror(AoeShapeSurfaceRor *s, int32_t x, int32_t y,
                        int32_t len, uint8_t color) {
    uint8_t *p;
    int32_t step;
    if (!s->bits || x < s->clip_left || x > s->clip_right ||
        y > s->clip_bottom)
        return;
    if (y < s->clip_top) {
        len += y - s->clip_top;
        y = s->clip_top;
    }
    if (y + len - 1 > s->clip_bottom) len = s->clip_bottom - y + 1;
    if (len <= 0) return;
    step = s->direction == 1 ? s->pitch : -s->pitch;
    p = s->bits + row_offset(s, y) + x;
    for (; len > 0; --len, p += step) *p = color;
}

void aoe_draw_fill_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                       int32_t x1, int32_t y1, uint8_t color) {
    uint8_t *p;
    int32_t t, w, h, step;
    if (!s->bits) return;
    if (x1 < x0) t = x0, x0 = x1, x1 = t;
    if (y1 < y0) t = y0, y0 = y1, y1 = t;
    if (x0 > s->clip_right || x1 < s->clip_left || y0 > s->clip_bottom ||
        y1 < s->clip_top)
        return;
    if (x0 < s->clip_left) x0 = s->clip_left;
    if (x1 > s->clip_right) x1 = s->clip_right;
    if (y0 < s->clip_top) y0 = s->clip_top;
    if (y1 > s->clip_bottom) y1 = s->clip_bottom;
    w = x1 - x0 + 1;
    h = y1 - y0 + 1;
    if (w <= 0 || h <= 0) return;
    step = s->direction == 1 ? s->pitch : -s->pitch;
    p = s->bits + row_offset(s, y0) + x0;
    for (; h > 0; --h, p += step) memset(p, color, (size_t)w);
}

/* The corners in order; zero when the box is empty. */
static int order(int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1) {
    int32_t t;
    if (*x1 < *x0) t = *x0, *x0 = *x1, *x1 = t;
    if (*y1 < *y0) t = *y0, *y0 = *y1, *y1 = t;
    return *x1 - *x0 + 1 > 0 && *y1 - *y0 + 1 > 0;
}

void aoe_draw_rect_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                       int32_t x1, int32_t y1, uint8_t color) {
    int32_t w, h;
    if (!order(&x0, &y0, &x1, &y1)) return;
    w = x1 - x0 + 1;
    h = y1 - y0 + 1;
    aoe_draw_hline_ror(s, x0, y0, w, color);
    aoe_draw_hline_ror(s, x0, y1, w, color);
    aoe_draw_vline_ror(s, x0, y0, h, color);
    aoe_draw_vline_ror(s, x1, y0, h, color);
}

void aoe_draw_bevel1_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                         int32_t x1, int32_t y1, uint8_t a, uint8_t b) {
    int32_t dw, dh;
    if (!order(&x0, &y0, &x1, &y1)) return;
    dw = x1 - x0;
    dh = y1 - y0;
    aoe_draw_hline_ror(s, x0 + 1, y0, dw, a);
    aoe_draw_vline_ror(s, x1, y0 + 1, dh, a);
    aoe_draw_hline_ror(s, x0, y1, dw + 1, b);
    aoe_draw_vline_ror(s, x0, y0, dh, b);
}

void aoe_draw_bevel2_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                         int32_t x1, int32_t y1, const uint8_t c[4]) {
    int32_t dw, dh;
    if (!order(&x0, &y0, &x1, &y1)) return;
    dw = x1 - x0;
    dh = y1 - y0;
    aoe_draw_hline_ror(s, x0 + 1, y0, dw, c[0]);
    aoe_draw_vline_ror(s, x1, y0 + 1, dh - 1, c[0]);
    aoe_draw_hline_ror(s, x0 + 2, y0 + 1, dw - 2, c[1]);
    aoe_draw_vline_ror(s, x1 - 1, y0 + 2, dh - 3, c[1]);
    aoe_draw_hline_ror(s, x0, y1, dw + 1, c[3]);
    aoe_draw_vline_ror(s, x0, y0, dh, c[3]);
    aoe_draw_hline_ror(s, x0 + 1, y1 - 1, dw - 1, c[2]);
    aoe_draw_vline_ror(s, x0 + 1, y0 + 1, dh - 2, c[2]);
}

void aoe_draw_bevel3_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                         int32_t x1, int32_t y1, const uint8_t c[6]) {
    int32_t dw, dh;
    if (!order(&x0, &y0, &x1, &y1)) return;
    dw = x1 - x0;
    dh = y1 - y0;
    aoe_draw_hline_ror(s, x0 + 1, y0, dw, c[0]);
    aoe_draw_vline_ror(s, x1, y0 + 1, dh - 1, c[0]);
    aoe_draw_hline_ror(s, x0 + 2, y0 + 1, dw - 2, c[1]);
    aoe_draw_vline_ror(s, x1 - 1, y0 + 2, dh - 3, c[1]);
    aoe_draw_hline_ror(s, x0 + 3, y0 + 2, dw - 4, c[2]);
    aoe_draw_vline_ror(s, x1 - 2, y0 + 3, dh - 5, c[2]);
    aoe_draw_hline_ror(s, x0, y1, dw + 1, c[5]);
    aoe_draw_vline_ror(s, x0, y0, dh, c[5]);
    aoe_draw_hline_ror(s, x0 + 1, y1 - 1, dw - 1, c[4]);
    aoe_draw_vline_ror(s, x0 + 1, y0 + 1, dh - 2, c[4]);
    aoe_draw_hline_ror(s, x0 + 2, y1 - 2, dw - 3, c[3]);
    aoe_draw_vline_ror(s, x0 + 2, y0 + 2, dh - 4, c[3]);
}

void aoe_draw_bevel2_flat_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                              int32_t x1, int32_t y1, const uint8_t c[4]) {
    int32_t dw, dh;
    if (!order(&x0, &y0, &x1, &y1)) return;
    dw = x1 - x0;
    dh = y1 - y0;
    aoe_draw_hline_ror(s, x0 + 1, y0, dw, c[0]);
    aoe_draw_vline_ror(s, x1, y0 + 1, dh - 1, c[0]);
    aoe_draw_hline_ror(s, x0 + 1, y0 + 1, dw - 1, c[1]);
    aoe_draw_vline_ror(s, x1 - 1, y0 + 2, dh - 2, c[1]);
    aoe_draw_hline_ror(s, x0, y1, dw + 1, c[3]);
    aoe_draw_vline_ror(s, x0, y0, dh, c[3]);
}

void aoe_draw_bevel3_flat_ror(AoeShapeSurfaceRor *s, int32_t x0, int32_t y0,
                              int32_t x1, int32_t y1, const uint8_t c[6]) {
    int32_t dw, dh;
    if (!order(&x0, &y0, &x1, &y1)) return;
    dw = x1 - x0;
    dh = y1 - y0;
    aoe_draw_hline_ror(s, x0 + 1, y0, dw, c[0]);
    aoe_draw_vline_ror(s, x1, y0 + 1, dh - 1, c[0]);
    aoe_draw_hline_ror(s, x0 + 2, y0 + 1, dw - 2, c[1]);
    aoe_draw_vline_ror(s, x1 - 1, y0 + 2, dh - 3, c[1]);
    aoe_draw_hline_ror(s, x0 + 2, y0 + 2, dw - 3, c[2]);
    aoe_draw_vline_ror(s, x1 - 2, y0 + 3, dh - 4, c[2]);
    aoe_draw_hline_ror(s, x0, y1, dw + 1, c[5]);
    aoe_draw_vline_ror(s, x0, y0, dh, c[5]);
    aoe_draw_hline_ror(s, x0 + 1, y1 - 1, dw - 1, c[4]);
    aoe_draw_vline_ror(s, x0 + 1, y0 + 1, dh - 2, c[4]);
}

void aoe_draw_set_shade_ror(AoeShapeSurfaceRor *s, const uint8_t *table) {
    s->shade = table;
}

void aoe_draw_shade_ror(AoeShapeSurfaceRor *s, int32_t left, int32_t top,
                        int32_t right, int32_t bottom) {
    uint8_t *row;
    int32_t stride, x, y;
    if (!s->shade || !s->bits) return;
    if (left < s->clip_left) left = s->clip_left;
    if (top < s->clip_top) top = s->clip_top;
    if (right > s->clip_right) right = s->clip_right;
    if (bottom > s->clip_bottom) bottom = s->clip_bottom;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= s->width) right = s->width - 1;
    if (bottom >= s->height) bottom = s->height - 1;
    if (left > right || top > bottom) return;
    stride = s->pitch * s->direction;
    row = s->bits + left +
          (s->direction < 1 ? top - s->height + 1 : top) * stride;
    for (y = top; y <= bottom; ++y, row += stride)
        for (x = 0; x <= right - left; ++x) row[x] = s->shade[row[x]];
}
