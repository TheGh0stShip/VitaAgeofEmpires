#include "ui_message.h"

#include <string.h>

#define M(p) ((AoeUiMessageRor *)(p))

static uint32_t now(const AoeUiMessageRor *m) {
    const AoeUiPlatformRor *pf = m->panel.system ? m->panel.system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

static void set_rect4(AoeUiRectRor *r, int32_t l, int32_t t, int32_t rr, int32_t b) {
    r->left = l;
    r->top = t;
    r->right = rr;
    r->bottom = b;
}

/* x87 `sar 1`: floor division by two. */
static int32_t half(int32_t v) { return v >= 0 ? v / 2 : -((-v + 1) / 2); }

void aoe_ui_message_init_ror(AoeUiMessageRor *m, AoeUiSystemRor *system,
                             const AoeUiMessageOpsRor *ops, uintptr_t font,
                             int32_t font_w, int32_t font_h) {
    if (!m) return;
    memset(m, 0, sizeof(*m));
    aoe_ui_panel_init_ror(&m->panel, system);
    m->panel.vt = &aoe_ui_message_vtable_ror;
    m->ops = ops;
    m->font = font;
    m->font_w = font_w;
    m->font_h = font_h;
    m->buffer_w = m->buffer_h = -1;
    m->timeout_ms = 8000;
    m->align = 2;
    m->panel.handles_mouse = 0;
    m->panel.vt->set_active(&m->panel, 0);
}

static void free_image(AoeUiMessageRor *m) {
    if (m->image && m->ops && m->ops->image_free) m->ops->image_free(m->ops->context, m->image);
    m->image = NULL;
}

void aoe_ui_message_release_ror(AoeUiMessageRor *m) {
    if (!m) return;
    free_image(m);
    aoe_ui_panel_release_ror(&m->panel);
}

static void destroy(AoeUiPanelRor *p) { aoe_ui_message_release_ror(M(p)); }

void aoe_ui_message_set_layout_ror(AoeUiMessageRor *m, int32_t align,
                                   int32_t valign, int32_t wrap) {
    if (!m) return;
    m->align = align;
    m->valign = valign;
    m->wrap = wrap;
}

void aoe_ui_message_set_font_ror(AoeUiMessageRor *m, uintptr_t font,
                                 int32_t font_w, int32_t font_h) {
    if (!m || m->font == font) return;
    m->font = font;
    m->font_w = font_w;
    m->font_h = font_h;
    m->panel.need_restore = 1;
    m->panel.vt->set_redraw(&m->panel, 1);
}

static void shown(AoeUiMessageRor *m) {
    if (m->image) aoe_ui_message_render_ror(m);
    m->panel.vt->set_active(&m->panel, 1);
    m->panel.vt->set_redraw(&m->panel, 1);
    if (m->panel.parent) m->panel.parent->vt->set_redraw(m->panel.parent, 1);
}

static void take_text(AoeUiMessageRor *m, int32_t style, const char *text) {
    m->style = style;
    memset(m->text, 0, sizeof m->text);
    if (text) strncpy(m->text, text, 0x1ff);
    ++m->panel.field_e0;
    m->shown = 1;
    m->shown_ms = now(m);
}

static int same_text(const AoeUiMessageRor *m, int32_t style, const char *text) {
    return m->shown && m->style == style && text && !strncmp(text, m->text, 0x1ff);
}

void aoe_ui_message_show_ror(AoeUiMessageRor *m, int32_t style,
                             const char *text, uint8_t color, uint8_t shadow,
                             uintptr_t font, int32_t font_w, int32_t font_h) {
    if (!m) return;
    if (same_text(m, style, text) && (!font || font == m->font)) return;
    if (font) {
        m->font = font;
        m->font_w = font_w;
        m->font_h = font_h;
    }
    take_text(m, style, text);
    m->palette_colors = 1;
    m->color = color;
    m->shadow = shadow;
    shown(m);
}

void aoe_ui_message_show_rgb_ror(AoeUiMessageRor *m, int32_t style,
                                 const char *text, uint32_t rgb,
                                 uint32_t shadow_rgb, uintptr_t font,
                                 int32_t font_w, int32_t font_h, void *icon1,
                                 int32_t frame1, void *icon2, int32_t frame2) {
    if (!m) return;
    if (same_text(m, style, text) && (!font || font == m->font) &&
        m->icon[0] == icon1 && m->icon[1] == icon2 &&
        m->icon_frame[0] == frame1 && m->icon_frame[1] == frame2)
        return;
    if (font) {
        m->font = font;
        m->font_w = font_w;
        m->font_h = font_h;
    }
    m->icon[0] = icon1;
    m->icon_frame[0] = frame1;
    m->icon[1] = icon2;
    m->icon_frame[1] = frame2;
    take_text(m, style, text);
    m->palette_colors = 0;
    m->rgb = rgb;
    m->shadow_rgb = shadow_rgb;
    shown(m);
}

void aoe_ui_message_hide_ror(AoeUiMessageRor *m) {
    if (!m) return;
    if (m->shown) {
        m->shown = 0;
        m->panel.vt->set_active(&m->panel, 0);
        m->panel.vt->set_redraw(&m->panel, 1);
        if (m->panel.parent) m->panel.parent->vt->set_redraw(m->panel.parent, 1);
    }
    m->rendered = 0;
}

static void colors(const AoeUiMessageRor *m, uint32_t *text, uint32_t *shadow) {
    if (!m->palette_colors) {
        *text = m->rgb;
        *shadow = m->shadow_rgb;
    } else if (m->ops && m->ops->palette_rgb) {
        *text = m->ops->palette_rgb(m->ops->context, m->color);
        *shadow = m->ops->palette_rgb(m->ops->context, m->shadow);
    } else {
        *text = m->color;
        *shadow = m->shadow;
    }
}

/* The five DrawText passes: shadow at the four corners of a 2-pixel box,
   then the text between them. */
static void passes(const AoeUiMessageRor *m, void *image, int32_t x, int32_t y,
                   int32_t right, int32_t bottom, uint32_t format) {
    AoeUiRectRor r;
    uint32_t text, shadow;
    int opaque = m->style == 2;
    if (!m->ops || !m->ops->draw_text) return;
    colors(m, &text, &shadow);
    set_rect4(&r, x, y, right - 2, bottom - 2);
    m->ops->draw_text(m->ops->context, image, m->text, &r, format, shadow, opaque);
    set_rect4(&r, x, y + 2, right - 2, bottom);
    m->ops->draw_text(m->ops->context, image, m->text, &r, format, shadow, opaque);
    set_rect4(&r, x + 2, y, right, bottom - 2);
    m->ops->draw_text(m->ops->context, image, m->text, &r, format, shadow, opaque);
    set_rect4(&r, x + 2, y + 2, right, bottom);
    m->ops->draw_text(m->ops->context, image, m->text, &r, format, shadow, opaque);
    set_rect4(&r, x + 1, y + 1, right - 1, bottom - 1);
    m->ops->draw_text(m->ops->context, image, m->text, &r, format, text, opaque);
}

static void screen_rect(AoeUiMessageRor *m) {
    m->screen_rect.left = m->panel.rect.left + m->text_rect.left;
    m->screen_rect.top = m->text_rect.top + m->panel.rect.top;
    m->screen_rect.right = m->text_rect.right + m->panel.rect.left;
    m->screen_rect.bottom = m->text_rect.bottom + m->panel.rect.top;
}

int32_t aoe_ui_message_render_ror(AoeUiMessageRor *m) {
    const AoeUiMessageOpsRor *ops;
    int32_t w, h, bx, by, cx, cy, tw, th, x0 = 0, y0 = 0, i;
    uint32_t format;
    if (!m || !m->image) return 0;
    ops = m->ops;
    if (ops && ops->image_clear) ops->image_clear(ops->context, m->image, &m->buffer);
    if (!m->text[0]) {
        set_rect4(&m->text_rect, 0, 0, 1, 1);
        set_rect4(&m->screen_rect, m->panel.rect.left, m->panel.rect.top,
                  m->panel.rect.left + 1, m->panel.rect.top + 1);
        m->rendered = 1;
        return 1;
    }
    w = m->panel.width;
    h = m->panel.height;
    bx = m->buffer.left;
    by = m->buffer.top;
    if (!ops || !ops->extent || !ops->extent(ops->context, m->font, m->text, &cx, &cy)) {
        set_rect4(&m->text_rect, 0, 0, 1, 1);
        m->rendered = 0;
        screen_rect(m);
        return 0;
    }
    th = cy + 2;
    tw = cx + 2;
    if (m->style == 1) {
        format = 0x21;
        x0 = half(w - tw) - 1;
    } else if (m->style == 5) {
        format = 0x22;
        x0 = w - tw - 1;
    } else {
        int fit = !m->wrap || cx <= w - 2;
        if (fit) {
            tw = cx + 1;
            th = cy + 1;
            format = 0x20;
        } else {
            th = h;
            tw = w;
            format = 0x10;
        }
        if (m->align == 4) {
            format |= 1;
            if (fit) x0 = half(w - tw) - 1;
        } else if (m->align == 3) {
            format |= 2;
            if (fit) x0 = w - tw - 1;
        }
        if (m->valign == 4) {
            format |= 4;
            if (fit) y0 = half(h - th) - 1;
        } else if (m->valign == 1) {
            format |= 8;
            if (fit) y0 = h - th - 1;
        }
        if (y0 < 0) y0 = 0;
    }
    if (x0 < 0) x0 = 0;
    set_rect4(&m->text_rect, x0, y0, x0 + tw, y0 + th);
    if (m->buffer.right < x0 + tw) m->text_rect.right = m->buffer.right;
    if (m->buffer.bottom < y0 + th) m->text_rect.bottom = m->buffer.bottom;
    passes(m, m->image, bx, by, bx + w - 1, by + h - 1, format);
    for (i = 0; i < 2; ++i) {
        int32_t iw, hx, hy;
        if (!m->icon[i] || !ops->icon_frame ||
            !ops->icon_frame(ops->context, m->icon[i], m->icon_frame[i], &iw, &hx, &hy))
            continue;
        if (ops->draw_icon)
            ops->draw_icon(ops->context, m->image, m->icon[i], m->icon_frame[i],
                           hx - iw - 4 + m->text_rect.left, hy + 6 + m->text_rect.top);
        m->text_rect.left += -4 - iw;
    }
    m->rendered = 1;
    screen_rect(m);
    return 1;
}

static void set_rect(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t w, int32_t h) {
    AoeUiMessageRor *m = M(p);
    aoe_ui_panel_set_rect_ror(p, x, y, w, h);
    if (m->buffer_w < p->width || m->buffer_h < p->height ||
        (p->area && p->area->width <= m->buffer_w + x)) {
        free_image(m);
        m->image = m->ops && m->ops->image_create
                       ? m->ops->image_create(m->ops->context, p->width, p->height)
                       : NULL;
        m->buffer_w = p->width;
        m->buffer_h = p->height;
        set_rect4(&m->buffer, 0, 0, p->width - 1, p->height - 1);
    }
    if (!m->image) {
        m->buffer_w = m->buffer_h = -1;
    } else {
        m->text_rect = m->buffer;
    }
    p->need_restore = 1;
    p->vt->set_redraw(p, 1);
}

static void draw(AoeUiPanelRor *p) {
    AoeUiMessageRor *m = M(p);
    if (!p->area || !p->visible || !p->active || !m->shown) {
        p->redraw = 0;
        return;
    }
    p->vt->begin_draw(p, 0);
    if (m->rendered && m->image) {
        if (p->need_restore) {
            aoe_ui_message_render_ror(m);
            p->need_restore = 0;
        } else if (m->ops && m->ops->blit) {
            m->ops->blit(m->ops->context, m->image, m->screen_rect.left,
                         m->screen_rect.top, &m->text_rect);
        }
        p->vt->end_draw(p);
        return;
    }
    if (m->text[0]) {
        uint32_t format;
        int32_t cx = 0, cy = 0, w = p->width, h = p->height;
        if (m->ops && m->ops->extent) m->ops->extent(m->ops->context, m->font, m->text, &cx, &cy);
        if (m->style == 1) format = 0x21;
        else if (m->style == 5) format = 0x22;
        else {
            format = (m->wrap && cx > w - 2) ? 0x10 : 0x20;
            if (m->align == 4) format |= 1;
            else if (m->align == 3) format |= 2;
            if (m->valign == 4) format |= 4;
            else if (m->valign == 1) format |= 8;
        }
        passes(m, NULL, p->rect.left, p->rect.top, p->rect.left + w - 1,
               p->rect.top + h - 1, format);
    }
    p->vt->end_draw(p);
}

static int32_t idle(AoeUiPanelRor *p) {
    AoeUiMessageRor *m = M(p);
    if (p->active && m->shown && (m->style == 1 || m->style == 3 || m->style == 4) &&
        m->timeout_ms <= (int32_t)(now(m) - m->shown_ms))
        aoe_ui_message_hide_ror(m);
    return 0;
}

static void true_rect(AoeUiPanelRor *p, AoeUiRectRor *out) {
    AoeUiMessageRor *m = M(p);
    if (!out) return;
    *out = (m->image && p->area && p->visible && p->active && m->shown)
               ? m->screen_rect : p->rect;
}

static int32_t no_key_action(AoeUiPanelRor *p, int32_t a, int32_t b,
                             int32_t c, int32_t d, int32_t e) {
    (void)p; (void)a; (void)b; (void)c; (void)d; (void)e;
    return 0;
}

static int32_t no_mouse(AoeUiPanelRor *p, uint8_t b, int32_t x, int32_t y,
                        int32_t c, int32_t s) {
    (void)p; (void)b; (void)x; (void)y; (void)c; (void)s;
    return 0;
}

static int32_t no_move(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)p; (void)x; (void)y; (void)c; (void)s;
    return 0;
}

const AoeUiPanelVtblRor aoe_ui_message_vtable_ror = {
    destroy,
    aoe_ui_panel_setup_ror,
    aoe_ui_panel_set_rect_ltrb_ror,
    set_rect,
    aoe_ui_panel_set_color_ror,
    aoe_ui_panel_set_active_ror,
    aoe_ui_panel_set_position_mode_ror,
    aoe_ui_panel_set_fixed_position_ror,
    aoe_ui_panel_set_redraw_ror,
    aoe_ui_panel_redraw_overlaps_ror,
    aoe_ui_panel_begin_draw_ror,
    aoe_ui_panel_end_draw_ror,
    draw,
    aoe_ui_panel_draw_rect_ror,
    aoe_ui_panel_draw_offset_ror,
    aoe_ui_panel_draw_parent_rect_ror,
    aoe_ui_panel_draw_parent_offset_ror,
    aoe_ui_panel_validate_ror,
    aoe_ui_panel_handle_message_ror,
    idle,
    aoe_ui_panel_handle_size_ror,
    aoe_ui_panel_handle_paint_ror,
    aoe_ui_panel_key_down_ror,
    aoe_ui_panel_char_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    aoe_ui_panel_ignore_ror,
    /* Mouse slots +0x70 / +0x78 answer 0 (0x00405a00) and +0x74 0x004059e0:
       the panel never takes the mouse. */
    no_mouse,
    no_move,
    no_mouse,
    aoe_ui_panel_mouse_dbl_ror,
    {aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror, aoe_ui_panel_no_action_ror,
     aoe_ui_panel_no_action_ror},
    no_key_action,
    aoe_ui_panel_zero_ror,
    aoe_ui_panel_no_char_action_ror,
    aoe_ui_panel_action_ror,
    true_rect,
    aoe_ui_panel_is_inside_ror,
    aoe_ui_panel_set_focus_ror,
    aoe_ui_panel_set_tab_order_ror,
    aoe_ui_panel_get_help_ror,
    aoe_ui_panel_stop_sound_ror,
    aoe_ui_panel_restart_sound_ror,
    aoe_ui_panel_slot_e0_ror,
};
