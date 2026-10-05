#include "timer_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TP(p) ((AoeTimerPanelRor *)(p))

static const AoeUiPlatformRor *platform(const AoeUiPanelRor *p) {
    return p->system ? p->system->platform : NULL;
}

static uint32_t now_ms(const AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = platform(p);
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

static void free_buffer(AoeTimerPanelRor *t) {
    t->has_region = 0;
    if (t->buffer) {
        aoe_surface_free_ror(t->buffer);
        free(t->buffer);
        t->buffer = NULL;
    }
}

int aoe_timer_panel_init_ror(AoeTimerPanelRor *t, AoeUiSystemRor *system,
                             const AoeTimerPanelOpsRor *ops, AoeUiAreaRor *area,
                             AoeUiPanelRor *parent, uintptr_t font) {
    aoe_ui_panel_init_ror(&t->panel, system);
    t->panel.vt = aoe_timer_panel_vtable_ror();
    t->ops = ops;
    t->panel.region = 0;
    t->font = font;
    t->color = 0xffffff;
    t->shadow = 0;
    t->mode = 0;
    t->kind = 0;
    t->drawn_mode = 0;
    t->drawn_kind = 0;
    t->player = NULL;
    t->start_ms = 0;
    t->value = 0;
    t->last_value = 0;
    t->shown = 0;
    t->drawn_shown = -1;
    t->buffer = NULL;
    t->buffer_w = -1;
    t->buffer_h = -1;
    t->buffer_ready = 0;
    t->has_region = 0;
    if (!aoe_ui_panel_setup_ror(&t->panel, area, parent, 0, 0, 0, 0, 0)) {
        t->panel.field_d8 = 1;
        return 0;
    }
    return 1;
}

static void destroy(AoeUiPanelRor *p) {
    free_buffer(TP(p));
    aoe_ui_panel_release_ror(p);
}

void aoe_timer_panel_refresh_ror(AoeTimerPanelRor *t, uint32_t now) {
    t->start_ms = now;
}

void aoe_timer_panel_set_ror(AoeTimerPanelRor *t, int32_t mode, int32_t index) {
    AoeUiPanelRor *p = &t->panel;
    const AoeTimerPanelOpsRor *ops = t->ops;
    if (t->mode == mode &&
        (!t->player || ops->player_number(ops->context, t->player) == index))
        return;
    t->mode = mode;
    t->player = index < 1 || !ops->world_player ? NULL : ops->world_player(ops->context, index);
    t->shadow = 0;
    if (!t->player) {
        t->color = 0xffffff;
    } else {
        switch (ops->player_color ? ops->player_color(ops->context, t->player) : -1) {
        case 0: t->color = 0xff0000; t->shadow = 0xb3b3b3; break;
        case 1: t->color = 0x2139c6; break;
        case 2: t->color = 0xffff; break;
        case 3: t->color = 0x4f73a3; break;
        case 4: t->color = 0xf77f3; break;
        case 5: t->color = 0xc700; break;
        case 6: t->color = 0x7b90a2; break;
        case 7: t->color = 0xadad00; break;
        default: t->color = 0xffffff; break;
        }
    }
    t->kind = mode == 1 || mode == 2 ? 1 : 2;
    p->vt->set_active(p, mode != 0);
    p->vt->set_redraw(p, 1);
    if (p->parent) p->parent->vt->set_redraw(p->parent, 1);
    p->vt->handle_idle(p);
}

static void set_rect(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t w, int32_t h) {
    AoeTimerPanelRor *t = TP(p);
    aoe_ui_panel_set_rect_ror(p, x, y, w, h);
    if (t->buffer_w < p->width || t->buffer_h < p->height ||
        (p->area && p->area->width <= t->buffer_w + x)) {
        AoeDrawSystemRor *d = p->area && p->area->surface ? p->area->surface->system : NULL;
        free_buffer(t);
        t->buffer = calloc(1, sizeof *t->buffer);
        if (t->buffer && !aoe_surface_init_ror(t->buffer, "time message panel", 0)) {
            free(t->buffer);
            t->buffer = NULL;
        }
        /* As the original, the attach result is not checked. */
        if (t->buffer) aoe_surface_attach_ror(t->buffer, d, p->width, p->height, 0, 0);
        t->buffer_w = p->width;
        t->buffer_h = p->height;
        t->buffer_rect[0] = 0;
        t->buffer_rect[1] = 0;
        t->buffer_rect[2] = p->width - 1;
        t->buffer_rect[3] = p->height - 1;
        t->has_region = 1;
    }
    if (!t->buffer) {
        t->buffer_w = -1;
        t->buffer_h = -1;
    } else {
        memcpy(t->source_rect, t->buffer_rect, sizeof t->source_rect);
    }
    p->need_restore = 1;
    p->vt->set_redraw(p, 1);
}

static int32_t round5(int32_t v) { return (int32_t)(((uint32_t)v + 4u) / 5u * 5u); }

/* The line and its DrawText format (0x20 left, 0x22 right). */
static uint32_t text(AoeTimerPanelRor *t, char *out, size_t n) {
    const AoeTimerPanelOpsRor *ops = t->ops;
    char fmt[0x200];
    int32_t id;
    if (t->kind == 1) {
        int32_t s = t->value, m = 0, h = 0;
        if (s > 59) {
            m = s / 60;
            s %= 60;
            if (m > 59) {
                h = m / 60;
                m %= 60;
            }
        }
        snprintf(out, n, "%02ld:%02ld:%02ld (%3.1f)", (long)h, (long)m, (long)s,
                 (double)(ops->world_speed ? ops->world_speed(ops->context) : 0.0f));
        return 0x20;
    }
    switch (t->mode) {
    case 4: id = 0x2c25; break;
    case 5: id = 0x2c26; break;
    case 6: id = 0x2c27; break;
    default: id = 0x2c24; break;
    }
    fmt[0] = '\0';
    if (ops->string) ops->string(ops->context, id, fmt, sizeof fmt);
    /* The string is the format, as the original's sprintf. */
    snprintf(out, n, fmt, round5(t->value));
    return 0x22;
}

static void shadowed(const AoeUiPlatformRor *pf, const char *s, int32_t l, int32_t top,
                     int32_t r, int32_t b, uint32_t format, uint32_t shadow,
                     uint32_t color) {
    const AoeUiRectRor rs[4] = {{l, top, r - 2, b - 2}, {l, top + 2, r - 2, b},
                                {l + 2, top, r, b - 2}, {l + 2, top + 2, r, b}};
    const AoeUiRectRor m = {l + 1, top + 1, r - 1, b - 1};
    int k;
    for (k = 0; k < 4; ++k) pf->text_rect(pf->context, s, &rs[k], format, shadow);
    pf->text_rect(pf->context, s, &m, format, color);
}

int aoe_timer_panel_render_ror(AoeTimerPanelRor *t) {
    AoeUiPanelRor *p = &t->panel;
    const AoeUiPlatformRor *pf = platform(p);
    AoeUiAreaRor buffer_area;
    AoeUiRectRor clip;
    char line[256];
    uint32_t format;
    if (!t->buffer) return 0;
    aoe_surface_soft_fill_ror(t->buffer, t->buffer_rect, 1);
    buffer_area.window = p->area ? p->area->window : NULL;
    buffer_area.width = t->buffer_w;
    buffer_area.height = t->buffer_h;
    buffer_area.surface = t->buffer;
    clip.left = t->buffer_rect[0];
    clip.top = t->buffer_rect[1];
    clip.right = t->buffer_rect[2];
    clip.bottom = t->buffer_rect[3];
    if (!pf || !pf->text_begin || !pf->text_rect ||
        !pf->text_begin(pf->context, &buffer_area, &clip, t->font))
        return 0;
    aoe_surface_clip_ror(t->buffer, t->buffer_rect);
    format = text(t, line, sizeof line);
    shadowed(pf, line, 0, 0, p->width - 1, p->height - 1, format, t->shadow, t->color);
    pf->text_end(pf->context);
    t->source_rect[0] = 0;
    t->source_rect[1] = 0;
    t->source_rect[2] = p->width - 1;
    t->source_rect[3] = p->height - 1;
    t->dest_rect[0] = p->rect.left;
    t->dest_rect[1] = p->rect.top;
    t->dest_rect[2] = p->rect.left + p->width - 1;
    t->dest_rect[3] = p->rect.top + p->height - 1;
    t->drawn_shown = t->shown;
    t->drawn_mode = t->mode;
    t->drawn_kind = t->kind;
    t->buffer_ready = 1;
    return 1;
}

void aoe_timer_panel_draw_ror(AoeUiPanelRor *p) {
    AoeTimerPanelRor *t = TP(p);
    const AoeUiPlatformRor *pf = platform(p);
    if (!p->area || !p->visible || !p->active) return;
    ++p->field_e0;
    p->vt->begin_draw(p, 0);
    t->shown = t->kind == 1 ? t->value : round5(t->value);
    if (t->buffer && (t->shown != t->drawn_shown || t->mode != t->drawn_mode ||
                      t->kind != t->drawn_kind || p->need_restore)) {
        aoe_timer_panel_render_ror(t);
        if (p->need_restore) {
            p->need_restore = 0;
            p->vt->end_draw(p);
            return;
        }
    }
    if (t->buffer_ready && t->buffer) {
        aoe_surface_set_key_ror(t->buffer, 1, 1);
        if (p->area->surface)
            aoe_surface_blit_ror(t->buffer, p->area->surface, t->dest_rect[0],
                                 t->dest_rect[1], t->source_rect, 1);
        aoe_surface_set_key_ror(t->buffer, 0, 1);
        p->vt->end_draw(p);
        return;
    }
    if (pf && pf->text_begin && pf->text_rect &&
        pf->text_begin(pf->context, p->area, &p->clip, t->font)) {
        char line[256];
        uint32_t format = text(t, line, sizeof line);
        int32_t l = p->rect.left, top = p->rect.top;
        shadowed(pf, line, l, top, p->width - 1 + l, p->height - 1 + top, format,
                 t->shadow, t->color);
        pf->text_end(pf->context);
    }
    p->vt->end_draw(p);
}

int32_t aoe_timer_panel_idle_ror(AoeUiPanelRor *p) {
    AoeTimerPanelRor *t = TP(p);
    const AoeTimerPanelOpsRor *ops = t->ops;
    float f;
    aoe_ui_panel_handle_idle_ror(p);
    switch (t->mode) {
    case 0:
        return 0;
    case 1:
        t->value = (int32_t)((now_ms(p) - t->start_ms) / 1000u);
        break;
    case 2:
        t->value = (int32_t)((ops->world_ms ? ops->world_ms(ops->context) : 0) / 1000u);
        break;
    case 3:
        t->value = (int32_t)(ops->world_years ? ops->world_years(ops->context) : 0.0f);
        break;
    case 4:
        t->value = t->player && ops->wonder_years
                       ? ops->wonder_years(ops->context,
                                           ops->player_number(ops->context, t->player))
                       : 0;
        break;
    case 5:
        f = t->player && ops->artifact_years ? ops->artifact_years(ops->context, t->player)
                                             : -1.0f;
        t->value = f == -1.0f ? 0 : (int32_t)f;
        break;
    case 6:
        f = t->player && ops->ruin_years ? ops->ruin_years(ops->context, t->player) : -1.0f;
        t->value = f == -1.0f ? 0 : (int32_t)f;
        break;
    default:
        break;
    }
    if (t->value != t->last_value) {
        p->vt->set_redraw(p, 1);
        if (p->parent) p->parent->vt->set_redraw(p->parent, 1);
        t->last_value = t->value;
    }
    return 0;
}

static void true_rect(AoeUiPanelRor *p, AoeUiRectRor *out) {
    AoeTimerPanelRor *t = TP(p);
    if (t->buffer && p->area && p->visible && p->active) {
        out->left = t->dest_rect[0];
        out->top = t->dest_rect[1];
        out->right = t->dest_rect[2];
        out->bottom = t->dest_rect[3];
        return;
    }
    *out = p->rect;
}

static AoeUiPanelVtblRor vtable;
static int vtable_ready;

const AoeUiPanelVtblRor *aoe_timer_panel_vtable_ror(void) {
    if (!vtable_ready) {
        vtable = aoe_ui_panel_vtable_ror;
        vtable.destroy = destroy;
        vtable.set_rect = set_rect;
        vtable.draw = aoe_timer_panel_draw_ror;
        vtable.handle_idle = aoe_timer_panel_idle_ror;
        vtable.get_true_rect = true_rect;
        vtable_ready = 1;
    }
    return &vtable;
}
