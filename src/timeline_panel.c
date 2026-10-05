#include "timeline_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define O (t->ops)
#define C (t->ops->context)

void aoe_timeline_init_ror(AoeTimelineRor *t, const AoeTimelineOpsRor *ops) {
    memset(t, 0, sizeof *t);
    t->ops = ops;
    t->field_490 = 2;
    t->color_494 = 0xffffff;
    t->color_49c = 0xffff;
    t->fill_color = 0xff;
    t->enabled = 1;
    t->tick_every = 0xc;
}

void aoe_timeline_release_ror(AoeTimelineRor *t) {
    int i;
    while (t->marks) {
        AoeTimelineMarkRor *m = t->marks;
        t->marks = m->next;
        free(m);
    }
    if (t->background) {
        O->shape_free(C, t->background);
        t->background = NULL;
    }
    if (t->icons) {
        O->shape_free(C, t->icons);
        t->icons = NULL;
    }
    for (i = 0; i < 3; ++i) O->font_close(C, &t->fonts[i]);
}

void aoe_timeline_set_border_ror(AoeTimelineRor *t, int32_t style, uint8_t c0, uint8_t c1,
                                 uint8_t c2, uint8_t c3, uint8_t c4, uint8_t c5) {
    t->border = style;
    t->border_colors[0] = c0;
    t->border_colors[1] = c1;
    t->border_colors[2] = c2;
    t->border_colors[3] = c3;
    t->border_colors[4] = c4;
    t->border_colors[5] = c5;
}

void aoe_timeline_set_font_ror(AoeTimelineRor *t, int which, const AoeTimelineFontRor *font) {
    if (which)
        t->font_b = *font;
    else
        t->font_a = *font;
    t->redraw = 1;
}

void aoe_timeline_set_fill_ror(AoeTimelineRor *t, int32_t fill, uint8_t color) {
    t->fill = fill;
    t->fill_color = color;
}

static int set_shape(AoeTimelineRor *t, void **slot, int16_t *w, int16_t *h, const char *name) {
    int16_t l = 0, tp = 0, r = 0, b = 0;
    if (*slot) {
        O->shape_free(C, *slot);
        *slot = NULL;
        *w = 0;
        *h = 0;
    }
    if (!name || !*name) return 0;
    *slot = O->shape_load(C, name);
    O->shape_box(C, *slot, &l, &tp, &r, &b);
    *w = (int16_t)(r - l + 1);
    *h = (int16_t)(b - tp + 1);
    return 1;
}

int aoe_timeline_set_background_ror(AoeTimelineRor *t, const char *name) {
    return set_shape(t, &t->background, &t->background_w, &t->background_h, name);
}

int aoe_timeline_set_icons_ror(AoeTimelineRor *t, const char *name) {
    return set_shape(t, &t->icons, &t->icon_w, &t->icon_h, name);
}

int aoe_timeline_setup_ror(AoeTimelineRor *t, int32_t x, int32_t y, int32_t w, int32_t h,
                           const AoeTimelineFontRor *font_a, const AoeTimelineFontRor *font_b,
                           const char *background, const char *icons, int32_t fill,
                           uint8_t fill_color, int32_t outline, uint8_t outline_color) {
    /* The original ignores the base setup's result here. */
    (void)O->panel_setup(C, x, y, w, h);
    t->x = x;
    t->y = y;
    t->width = w;
    t->height = h;
    t->fill = fill;
    t->fill_color = fill_color;
    t->outline = outline;
    t->outline_color = outline_color;
    if (outline) t->border = 1;
    aoe_timeline_set_font_ror(t, 0, font_a);
    aoe_timeline_set_font_ror(t, 1, font_b);
    aoe_timeline_set_background_ror(t, background);
    aoe_timeline_set_icons_ror(t, icons);
    aoe_timeline_set_fill_ror(t, 1, O->palette_index(C, 0));
    return 1;
}

AoeTimelineRor *aoe_timeline_new_ror(const AoeTimelineOpsRor *ops, int32_t parent_w,
                                     int32_t parent_h, int32_t x, int32_t y, int32_t w, int32_t h,
                                     const AoeTimelineFontRor *font_a,
                                     const AoeTimelineFontRor *font_b, int32_t base_w,
                                     int32_t base_h) {
    AoeTimelineRor *t;
    if (base_w == 0 || base_h == 0) return NULL; /* host guard: the original divides */
    t = malloc(sizeof *t);
    if (!t) return NULL;
    aoe_timeline_init_ror(t, ops);
    aoe_timeline_setup_ror(t, parent_w * x / base_w, parent_h * y / base_h, parent_w * w / base_w,
                           parent_h * h / base_h, font_a, font_b, NULL, NULL, 0, 0, 0, 0);
    aoe_timeline_layout_ror(t);
    return t;
}

void aoe_timeline_layout_ror(AoeTimelineRor *t) {
    int32_t n = O->player_count(C) - 1, p, wide;
    t->samples = 0;
    for (p = 1; p <= n; ++p) {
        const AoePlayerHistoryRor *h = O->history(C, p);
        if (h && t->samples < h->count) t->samples = h->count;
    }
    wide = t->width - 0x31;
    if (t->samples == 0) {
        t->samples = 1;
        t->enabled = 0;
    }
    t->skip_ratio = 0.0f;
    t->scale = (float)wide / (float)t->samples;
    if (t->scale < 1.0f) {
        t->scale = 1.0f;
        t->columns = wide;
        t->compress = 1;
        t->skip_total = t->samples - wide;
        /* host guard: the original divides by a zero skip count only when
           scale is NaN, which cannot reach here */
        t->skip_ratio = (float)wide / (float)t->skip_total;
    } else {
        t->columns = t->samples;
    }
    t->axis_w = (int32_t)((double)t->columns * (double)t->scale) + 4;
    t->axis_h = t->height - 4 * t->font_a.char_h - 0x1a;
    t->left = t->x + 0x1b;
    t->cur_x = t->left;
    t->top = t->y + 0xf + 2 * t->font_a.char_h;
    t->cur_y = t->top;
    t->graph_w = t->axis_w - 2;
    t->graph_h = t->axis_h - 3;
    if (t->columns < 0xc)
        t->tick_every = 1;
    else
        t->tick_every = (int32_t)((double)(t->columns - 1) * (double)(1.0f / 12.0f));
}

void aoe_timeline_add_tick_ror(AoeTimelineRor *t, int32_t sample, int32_t total, int32_t x,
                               int32_t show) {
    AoeTimelineMarkRor *m = calloc(1, sizeof *m);
    if (!m) return; /* the original does not check */
    m->sample = sample;
    m->x = x;
    m->kind = 3;
    m->value = total;
    m->show = show;
    m->next = t->marks;
    t->marks = m;
}

void aoe_timeline_add_event_ror(AoeTimelineRor *t, uint8_t player,
                                const AoePlayerHistoryRor *history, uint8_t color,
                                int32_t sample, int32_t x, int32_t y, int32_t length) {
    uint8_t kind = 0;
    AoeTimelineMarkRor *m;
    if (!aoe_history_find_ror(history, sample, &kind)) return;
    m = calloc(1, sizeof *m);
    if (!m) return;
    m->sample = sample;
    m->x = x;
    m->y = y;
    m->length = length;
    m->player = player;
    m->color = color;
    m->kind = kind;
    m->next = t->marks;
    t->marks = m;
}

int aoe_timeline_place_ror(AoeTimelineRor *t, AoeTimelineMarkRor *m, int16_t frame) {
    int32_t icon_x = 0, icon_y = 0, text_x = 0, text_y = 0, frames, len, px;
    int fits = t->icons != NULL;
    m->show = 0;
    frames = t->icons ? O->shape_frames(C, t->icons) : 0;
    if (frames == 0) fits = 0;
    if (frames < frame) {
        fits = 0;
    } else if (fits) {
        int16_t w = 0, h = 0;
        int32_t half, x0;
        O->shape_size(C, t->icons, frame, &w, &h);
        if (w > 0) t->icon_w = w;
        if (h > 0) t->icon_h = h;
        if (t->icon_h > m->length) fits = 0;
        half = t->icon_w / 2;
        if (m->x + half > t->graph_w + t->left) fits = 0;
        x0 = m->x - half;
        if (x0 < t->left) {
            fits = 0;
        } else if (fits) {
            icon_x = x0;
            /* y is doubled in the original */
            icon_y = m->length / 2 + m->y * 2 - t->icon_h / 2;
        }
    }
    switch (m->kind) {
    case 0:
    case 2:
        len = (int32_t)strlen(m->text);
        m->text_len = len;
        px = t->font_b.char_w * len;
        if (px + 2 + m->x < t->left + t->graph_w)
            text_x = m->x + 2;
        else
            text_x = t->left - px - 3 + t->graph_w;
        text_y = m->length + m->y - t->font_b.char_h - 2;
        if (text_y <= t->top)
            text_y = t->top + 2;
        else if (t->graph_h + t->top <= text_y)
            text_y = t->graph_h - t->font_b.char_h - 3 + t->top;
        break;
    case 1:
        len = (int32_t)strlen(m->text);
        m->text_len = len;
        px = t->font_b.char_w * len;
        if (px + 2 + m->x < t->left + t->graph_w)
            text_x = m->x + 2;
        else
            text_x = t->left - px - 8 + t->graph_w;
        if (t->font_b.char_h + 2 + m->y < t->graph_h + t->top)
            text_y = m->y + 2;
        else
            text_y = t->graph_h - t->font_b.char_h - 2 + t->top;
        break;
    case 6:
    case 8: {
        int32_t cap = m->kind == 6 ? 2 : 5;
        len = (int32_t)strlen(m->text);
        if (t->left + t->graph_w < len * t->font_b.char_w + m->x)
            m->text_len = t->font_b.char_w
                              ? (t->left - m->x + t->graph_w) / t->font_b.char_w
                              : 0; /* host guard on the division */
        else
            m->text_len = len;
        if (m->text_len > cap) m->text_len = cap;
        if (t->graph_h + t->top < t->font_b.char_h + t->icon_h + 1 + icon_y)
            icon_y = t->graph_h - t->font_b.char_h - t->icon_h - 1 + t->top;
        text_y = t->icon_h + 1 + icon_y;
        text_x = icon_x;
        break;
    }
    default:
        break;
    }
    m->icon_x = icon_x;
    m->text_x = text_x;
    m->icon_y = icon_y;
    m->text_y = text_y;
    if (fits) m->show = 1;
    return fits;
}

static void tick_texts(AoeTimelineMarkRor *m) {
    int32_t hours = 0, minutes = 0;
    snprintf(m->text, sizeof m->text, "%ld", (long)m->value);
    m->text_len = (int32_t)strlen(m->text);
    if (m->sample * 15 > 59) {
        minutes = m->sample * 15 / 60;
        if (minutes > 59) {
            hours = minutes / 60;
            minutes %= 60;
        }
    }
    snprintf(m->text2, sizeof m->text2, "%1ld:%02ld", (long)hours, (long)minutes);
    m->text2_len = (int32_t)strlen(m->text2);
}

void aoe_timeline_draw_mark_ror(AoeTimelineRor *t, AoeTimelineMarkRor *m, int16_t frame) {
    char buf[50];
    uint8_t black;
    int fits;
    (void)O->palette_index(C, 0xffffff);
    black = O->palette_index(C, 0);
    switch (m->kind) {
    case 0:
    case 1:
    case 2:
        if (O->string(C, 0x106a + m->kind, buf, sizeof buf))
            snprintf(m->text, sizeof m->text, "%s", buf);
        else
            m->text[0] = 0;
        break;
    case 3:
        tick_texts(m);
        break;
    case 4:
    case 5:
    case 7:
        m->text[0] = 0;
        break;
    case 8:
        /* "%ld/%1d" of +0x18 / +0x1c is formatted and then replaced by
           the tick texts, as in the original. */
        tick_texts(m);
        break;
    default:
        break;
    }
    fits = aoe_timeline_place_ror(t, m, frame);
    if (m->kind <= 2)
        O->vline(C, m->x, m->y, m->length, black);
    else if (fits)
        O->shape_draw(C, t->icons, m->icon_x, m->icon_y, frame);
}

void aoe_timeline_draw_background_ror(AoeTimelineRor *t) {
    if (t->background) {
        int32_t cols, rows, r, c;
        if (!O->lock(C, "tpnl_tml::draw_background")) return;
        if (t->background_w >= t->width) {
            cols = 1;
        } else {
            cols = (int16_t)(t->width / t->background_w);
            if (cols * t->background_w < t->width) ++cols;
        }
        /* The original leaves the row count uninitialised when the tile
           is shorter than the panel (its test is inverted); the port
           tiles the whole height (host-only difference). */
        if (t->background_h <= 0) {
            rows = 0;
        } else {
            rows = (int16_t)(t->height / t->background_h);
            if (rows * t->background_h < t->height) ++rows;
        }
        for (r = 0; r < (int16_t)rows; ++r)
            for (c = 0; c < (int16_t)cols; ++c)
                O->shape_draw(C, t->background, t->background_w * c + t->x,
                              t->background_h * r + t->y, 0);
        O->unlock(C, "tpnl_tml::draw_background");
        return;
    }
    if (t->fill) {
        if (!O->lock(C, "tpnl_tml::draw_background2")) return;
        O->box(C, t->x, t->y, t->width - 1 + t->x, t->height - 1 + t->y, t->fill_color);
        O->unlock(C, "tpnl_tml::draw_background2");
        return;
    }
    if (!O->has_parent(C)) return;
    if (t->border > 1 && t->border < 8)
        O->parent_background(C, 1);
    else if (!O->parent_field_84(C))
        O->parent_background(C, 0);
}

void aoe_timeline_draw_border_ror(AoeTimelineRor *t) {
    static const char who[] = "tpnl_tml::draw_border";
    const uint8_t *c = t->border_colors;
    uint8_t ring[6];
    int32_t x1 = t->width - 1 + t->x, y1 = t->height - 1 + t->y;
    int count = 0, i;
    if (!t->border) return;
    if (O->lock(C, who)) {
        switch (t->border) {
        case 1:
            O->outline(C, t->x, t->y, x1, y1, t->outline_color);
            break;
        case 2: ring[0] = c[5]; ring[1] = c[0]; count = 1; break;
        case 5: ring[0] = c[0]; ring[1] = c[5]; count = 1; break;
        case 3: ring[0] = c[5]; ring[1] = c[4]; ring[2] = c[1]; ring[3] = c[0]; count = 2; break;
        case 6: ring[0] = c[0]; ring[1] = c[1]; ring[2] = c[4]; ring[3] = c[5]; count = 2; break;
        case 4:
            for (i = 0; i < 6; ++i) ring[i] = c[5 - i];
            count = 3;
            break;
        case 7:
            for (i = 0; i < 6; ++i) ring[i] = c[i];
            count = 3;
            break;
        default:
            break;
        }
        if (count) O->rings(C, t->x, t->y, x1, y1, count, ring);
    }
    O->unlock(C, who);
}

void aoe_timeline_draw_axis_ror(AoeTimelineRor *t) {
    static const char who[] = "tpnl_tml::draw_axis";
    uint8_t white = O->palette_index(C, 0xffffff);
    int32_t X = t->x, Y = t->y, fh = t->font_a.char_h;
    char buf[100];
    if (O->lock(C, who)) {
        O->box(C, X + 0x19, Y + 0xc + 2 * fh + t->axis_h, X + 0x18 + t->axis_w,
               Y + 2 * fh + 0xd + t->axis_h, white);
        O->box(C, X + 0x19, Y + 0xd + 2 * fh, X + 0x18 + t->axis_w, Y + 0xe + 2 * fh, white);
        O->box(C, X + 0x19, Y + 0xd + 2 * fh, X + 0x1a, Y + 2 * fh + 0xd + t->axis_h, white);
        O->box(C, X + 0x17 + t->axis_w, Y + 0xd + 2 * fh, X + 0x18 + t->axis_w,
               Y + 2 * fh + 0xd + t->axis_h, white);
    }
    O->unlock(C, who);
    if (!O->dc_lock(C, who)) return;
    if (O->string(C, 0x2c57, buf, sizeof buf))
        O->text(C, 0, X + 8, Y + 5, buf, (int32_t)strlen(buf), 0xffffff);
    if (O->string(C, 0x2c56, buf, sizeof buf)) {
        uint32_t len = (uint32_t)strlen(buf);
        O->text(C, 0, t->axis_w / 2 - (int32_t)((len * (uint32_t)t->font_a.char_w) >> 1) + t->left,
                t->height + Y - 5 - fh, buf, (int32_t)len, 0xffffff);
    }
    O->dc_unlock(C, who);
}

static uint32_t player_rgb(int16_t color) {
    static const uint32_t rgb[8] = {0xc40000, 200,      0xeaea,   0x215a8c,
                                    0x80ff,   0x8000,   0x808080, 0x808040};
    return (color >= 0 && color < 8) ? rgb[color] : 0xffffff;
}

void aoe_timeline_draw_graph_ror(AoeTimelineRor *t) {
    static const char who[] = "tpnl_tml::draw_timeline";
    int32_t tick = 0, extra = 0, skip_adj = 0, run = 0, skipped = 0, drawn = 0, step_bias = 0, i;
    if (!t->enabled) return;
    if (O->lock(C, who)) {
        t->cur_x = t->left;
        for (i = 0; i < t->samples; ++i) {
            int32_t n = O->player_count(C) - 1, p, need, step, width, col;
            float total = 0.0f, next_total;
            int has_event = 0;
            uint8_t kind;
            if (drawn >= t->columns) break;
            /* These returns leave the area locked, as the original does. */
            if (n <= 0) return;
            for (p = 1; p <= n; ++p) {
                const AoePlayerHistoryRor *h = O->history(C, p);
                float v = (float)aoe_history_get_ror(h, 0, i);
                if (aoe_history_find_ror(h, i, &kind)) has_event = 1;
                total = v + total;
            }
            if (total == 0.0f) return;
            step = (int32_t)t->skip_ratio;
            need = (int32_t)((double)(skipped + 1) * (double)t->skip_ratio) + step * step_bias -
                   skip_adj;
            if (t->compress && !has_event && run >= need && skipped < t->skip_total) {
                run = 0;
                --step_bias;
                ++skipped;
                if (need > step) ++skip_adj;
                continue;
            }
            ++run;
            ++drawn;
            if (i == t->columns - 1) {
                next_total = total;
            } else {
                next_total = 0.0f;
                for (p = 1; p <= n; ++p)
                    next_total = (float)aoe_history_get_ror(O->history(C, p), 0, i + 1) + next_total;
                if (next_total == 0.0f) return;
            }
            if (tick == t->tick_every - 1 || i == 0) {
                aoe_timeline_add_tick_ror(t, i, (int32_t)total, t->cur_x, 1);
                tick = 0;
            } else {
                ++tick;
            }
            {
                int32_t a = (int32_t)((double)(i + 1) * (double)t->scale), b = (int32_t)t->scale,
                        c = b * (i + 1);
                if (c < a - extra) {
                    width = b - c - extra + a;
                    ++extra;
                } else {
                    width = b;
                }
            }
            for (col = 0; col < width; ++col) {
                int32_t cum = 0;
                float frac = 0.0f;
                t->cur_y = t->top;
                n = O->player_count(C) - 1;
                for (p = 0; p < n; ++p) {
                    const AoePlayerHistoryRor *hist = O->history(C, p + 1);
                    uint8_t idx = O->palette_index(C, player_rgb(O->player_color(C, p + 1)));
                    float f = (float)aoe_history_get_ror(hist, 0, i);
                    int32_t H = t->graph_h;
                    int32_t h = (int32_t)((double)f / (double)total * (double)H);
                    if (f >= 1.0f && h < 3) h = 3;
                    if (t->scale != 1.0f) {
                        double next = i == t->columns - 1
                                          ? (double)f
                                          : (double)aoe_history_get_ror(hist, 0, i + 1);
                        int32_t hn;
                        double exact;
                        float stored;
                        H = t->graph_h;
                        hn = (int32_t)(next / (double)next_total * (double)H);
                        exact = (double)col / (double)t->scale * (double)(hn - h) + (double)h;
                        stored = (float)exact;
                        h = (int32_t)exact;
                        if (f >= 1.0f && h < 3) h = 3;
                        frac = (float)(((double)stored + (double)frac) - (double)h);
                        if ((double)frac > 1.0) {
                            frac = frac - 1.0f;
                            ++h;
                        }
                    }
                    if (p == n - 1 && h + cum != H) h = H - cum;
                    O->vline(C, t->cur_x, t->cur_y, h, idx);
                    if (col == 0)
                        aoe_timeline_add_event_ror(t, (uint8_t)(p + 1), hist, idx, i, t->cur_x,
                                                   t->cur_y, h);
                    t->cur_y += h;
                    cum += h;
                }
                ++t->cur_x;
            }
        }
    }
    O->unlock(C, who);
}

void aoe_timeline_draw_marks_ror(AoeTimelineRor *t) {
    static const char who[] = "tpnl_tml::draw_special_events";
    AoeTimelineMarkRor *m;
    if (!t->enabled) return;
    (void)O->palette_index(C, 0xffffff);
    (void)O->palette_index(C, 0);
    if (O->lock(C, who))
        for (m = t->marks; m; m = m->next)
            if (m->kind <= 8)
                aoe_timeline_draw_mark_ror(t, m, (int16_t)(m->kind <= 3 ? -1 : m->kind - 4));
    O->unlock(C, who);
    if (!O->dc_lock(C, who)) return;
    for (m = t->marks; m; m = m->next) {
        switch (m->kind) {
        case 6:
        case 8:
            if (!m->show) break;
            /* fall through */
        case 0:
        case 1:
        case 2:
            O->text(C, 1, m->text_x, m->text_y, m->text, m->text_len, 0);
            break;
        case 3: {
            int32_t off = m->sample != 0 ? -2 : 2;
            uint32_t len = (uint32_t)strlen(m->text);
            O->text(C, 1, m->x - (int32_t)(len >> 1) * t->font_a.char_w + off,
                    t->y + 8 + t->font_a.char_h, m->text, (int32_t)len, 0xffffff);
            len = (uint32_t)strlen(m->text2);
            O->text(C, 1, m->x - (int32_t)(len >> 1) * t->font_a.char_w + off,
                    t->height + t->y - 2 * t->font_a.char_h - 7, m->text2, (int32_t)len, 0xffffff);
            break;
        }
        default:
            break;
        }
    }
    O->dc_unlock(C, who);
}

void aoe_timeline_draw_names_ror(AoeTimelineRor *t) {
    static const char who[] = "tpnl_tml::draw_civilization_name";
    int32_t n, p, y0;
    float total = 0.0f;
    if (!t->enabled) return;
    if (!O->dc_lock(C, who)) return;
    n = O->player_count(C) - 1;
    /* Without players or samples the original returns with the device
       context still locked. */
    if (n <= 0) return;
    for (p = 1; p <= n; ++p) total = (float)aoe_history_get_ror(O->history(C, p), 0, 0) + total;
    if (total == 0.0f) return;
    y0 = t->top;
    for (p = 1; p <= n; ++p) {
        char civ[100], name[100], line[204];
        const char *pn = O->player_name(C, p);
        int32_t h = (int32_t)((double)aoe_history_get_ror(O->history(C, p), 0, 0) / (double)total *
                              (double)t->graph_h);
        int32_t y;
        civ[0] = 0;
        O->format(C, 0x69, O->player_civ(C, p), 0, civ, sizeof civ);
        snprintf(name, sizeof name, "%s", pn ? pn : "");
        snprintf(line, sizeof line, "%s - %s", name, civ);
        y = h / 2 - t->font_b.char_h / 2 + y0;
        if (y <= t->top) y = t->top + 2;
        if (t->graph_h + t->top <= t->font_b.char_h + y)
            y = t->graph_h - t->font_b.char_h - 2 + t->top;
        O->text(C, 1, t->left + 3, y, line, (int32_t)strlen(line), 0);
        y0 += h;
    }
    O->dc_unlock(C, who);
}

void aoe_timeline_draw_ror(AoeTimelineRor *t) {
    t->redraw = 0;
    if (!O->draw_begin(C)) return;
    aoe_timeline_draw_background_ror(t);
    aoe_timeline_draw_border_ror(t);
    aoe_timeline_draw_axis_ror(t);
    aoe_timeline_draw_graph_ror(t);
    aoe_timeline_draw_marks_ror(t);
    aoe_timeline_draw_names_ror(t);
    O->draw_end(C);
}
