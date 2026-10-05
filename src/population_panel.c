#include "population_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PP(p) ((AoePopulationPanelRor *)(p))

static const AoeUiPlatformRor *platform(const AoeUiPanelRor *p) {
    return p->system ? p->system->platform : NULL;
}

static void free_buffer(AoePopulationPanelRor *pp) {
    pp->has_region = 0;
    if (pp->buffer) {
        aoe_surface_free_ror(pp->buffer);
        free(pp->buffer);
        pp->buffer = NULL;
    }
}

int aoe_population_panel_init_ror(AoePopulationPanelRor *pp,
                                  AoeUiSystemRor *system,
                                  const AoePopulationPanelOpsRor *ops,
                                  AoeUiAreaRor *area, AoeUiPanelRor *parent,
                                  uintptr_t font) {
    aoe_ui_panel_init_ror(&pp->panel, system);
    pp->panel.vt = aoe_population_panel_vtable_ror();
    pp->ops = ops;
    pp->panel.region = 0;
    pp->font = font;
    pp->color = 0xffffff;
    pp->shadow = 0;
    pp->flash_color = 0xffff;
    pp->player = NULL;
    pp->population = 0;
    pp->limit = 0;
    pp->flash = 0;
    pp->flash_seconds = 0;
    pp->buffer = NULL;
    pp->buffer_w = -1;
    pp->buffer_h = -1;
    pp->buffer_ready = 0;
    pp->has_region = 0;
    if (!aoe_ui_panel_setup_ror(&pp->panel, area, parent, 0, 0, 0, 0, 0)) {
        pp->panel.field_d8 = 1;
        return 0;
    }
    return 1;
}

static void destroy(AoeUiPanelRor *p) {
    free_buffer(PP(p));
    aoe_ui_panel_release_ror(p);
}

static float attribute(AoePopulationPanelRor *pp, int i) {
    return pp->player && pp->ops->attribute
               ? pp->ops->attribute(pp->ops->context, pp->player, i)
               : 0.0f;
}

/* Population and its limit (attributes 11; 4 + 11 capped by 32). */
static void numbers(AoePopulationPanelRor *pp, int32_t *pop, int32_t *limit) {
    int32_t cap;
    *pop = (int32_t)attribute(pp, 11);
    *limit = (int32_t)(attribute(pp, 4) + attribute(pp, 11));
    cap = (int32_t)attribute(pp, 32);
    if (cap < *limit) *limit = cap;
}

void aoe_population_panel_refresh_ror(AoePopulationPanelRor *pp) {
    int32_t pop, houses, cap;
    pp->player = pp->ops && pp->ops->player ? pp->ops->player(pp->ops->context) : NULL;
    pop = (int32_t)attribute(pp, 11);
    pp->population = pop;
    houses = (int32_t)attribute(pp, 4);
    pp->limit = pop + houses;
    cap = (int32_t)attribute(pp, 32);
    if (cap < pop + houses) pp->limit = cap;
}

void aoe_population_panel_set_rect_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                       int32_t w, int32_t h) {
    AoePopulationPanelRor *pp = PP(p);
    aoe_ui_panel_set_rect_ror(p, x, y, w, h);
    if (pp->buffer_w < p->width || pp->buffer_h < p->height ||
        (p->area && p->area->width <= pp->buffer_w + x)) {
        AoeDrawSystemRor *d = p->area && p->area->surface ? p->area->surface->system : NULL;
        free_buffer(pp);
        pp->buffer = calloc(1, sizeof *pp->buffer);
        if (pp->buffer && !aoe_surface_init_ror(pp->buffer, "time message panel", 0)) {
            free(pp->buffer);
            pp->buffer = NULL;
        }
        /* As the original, the attach result is not checked (it is 0 in
           GDI mode, where the surface keeps no bits). */
        if (pp->buffer) aoe_surface_attach_ror(pp->buffer, d, p->width, p->height, 0, 0);
        pp->buffer_w = p->width;
        pp->buffer_h = p->height;
        pp->buffer_rect[0] = 0;
        pp->buffer_rect[1] = 0;
        pp->buffer_rect[2] = p->width - 1;
        pp->buffer_rect[3] = p->height - 1;
        pp->has_region = 1;
    }
    if (!pp->buffer) {
        pp->buffer_w = -1;
        pp->buffer_h = -1;
    } else {
        memcpy(pp->source_rect, pp->buffer_rect, sizeof pp->source_rect);
    }
    p->need_restore = 1;
    p->vt->set_redraw(p, 1);
}

static void label(AoePopulationPanelRor *pp, char *out, size_t n) {
    char s[0x200];
    s[0] = '\0';
    if (pp->ops && pp->ops->string) pp->ops->string(pp->ops->context, 0x2b2a, s, sizeof s);
    snprintf(out, n, "%s%ld/%ld", s, (long)pp->population, (long)pp->limit);
}

/* The four shadows, then the text one pixel in, centred in each. */
static void shadowed(const AoeUiPlatformRor *pf, const char *text, int32_t l,
                     int32_t t, int32_t r, int32_t b, uint32_t shadow,
                     uint32_t color) {
    const AoeUiRectRor rs[4] = {{l, t, r - 2, b - 2}, {l, t + 2, r - 2, b},
                                {l + 2, t, r, b - 2}, {l + 2, t + 2, r, b}};
    const AoeUiRectRor m = {l + 1, t + 1, r - 1, b - 1};
    int k;
    for (k = 0; k < 4; ++k) pf->text_rect(pf->context, text, &rs[k], 0x21, shadow);
    pf->text_rect(pf->context, text, &m, 0x21, color);
}

int aoe_population_panel_render_ror(AoePopulationPanelRor *pp) {
    AoeUiPanelRor *p = &pp->panel;
    const AoeUiPlatformRor *pf = platform(p);
    AoeUiAreaRor buffer_area;
    AoeUiRectRor clip;
    char text[256];
    if (!pp->buffer) return 0;
    aoe_surface_soft_fill_ror(pp->buffer, pp->buffer_rect, 1);
    buffer_area.window = p->area ? p->area->window : NULL;
    buffer_area.width = pp->buffer_w;
    buffer_area.height = pp->buffer_h;
    buffer_area.surface = pp->buffer;
    clip.left = pp->buffer_rect[0];
    clip.top = pp->buffer_rect[1];
    clip.right = pp->buffer_rect[2];
    clip.bottom = pp->buffer_rect[3];
    if (!pf || !pf->text_begin || !pf->text_rect ||
        !pf->text_begin(pf->context, &buffer_area, &clip, pp->font))
        return 0;
    aoe_surface_clip_ror(pp->buffer, pp->buffer_rect);
    label(pp, text, sizeof text);
    shadowed(pf, text, 0, 0, p->width - 1, p->height - 1, pp->shadow,
             pp->flash == 1 ? pp->flash_color : pp->color);
    pf->text_end(pf->context);
    pp->source_rect[0] = 0;
    pp->source_rect[1] = 0;
    pp->source_rect[2] = p->width - 1;
    pp->source_rect[3] = p->height - 1;
    pp->dest_x = p->rect.left;
    pp->dest_y = p->rect.top;
    pp->dest_right = p->rect.left + p->width - 1;
    pp->dest_bottom = p->rect.top + p->height - 1;
    pp->buffer_ready = 1;
    return 1;
}

void aoe_population_panel_draw_ror(AoeUiPanelRor *p) {
    AoePopulationPanelRor *pp = PP(p);
    const AoeUiPlatformRor *pf = platform(p);
    const AoePopulationPanelOpsRor *ops = pp->ops;
    int changed = 0;
    int32_t pop, limit;
    if (!p->area || !p->visible || !p->active) return;
    ++p->field_e0;
    p->vt->begin_draw(p, 0);
    pp->player = ops && ops->player ? ops->player(ops->context) : NULL;
    if (pp->flash == 0 && pp->player && ops->limit_flag && ops->player_number &&
        ops->limit_flag(ops->context, ops->player_number(ops->context, pp->player)) > 0) {
        pp->flash = 1;
        pp->flash_seconds = 0;
        changed = 1;
    }
    if (pp->flash != 0) {
        pp->flash_seconds += ops->frame_seconds ? ops->frame_seconds(ops->context, pp->player) : 0;
        if (pp->flash_seconds > 1.0f) {
            changed = 1;
            pp->flash_seconds = 0;
            if (++pp->flash > 2) pp->flash = 0;
        }
    }
    numbers(pp, &pop, &limit);
    if (pp->buffer && (changed || p->need_restore || pp->population != pop ||
                       pp->limit != limit)) {
        pp->population = pop;
        pp->limit = limit;
        aoe_population_panel_render_ror(pp);
        if (p->need_restore) {
            p->need_restore = 0;
            p->vt->end_draw(p);
            return;
        }
    }
    if (pp->buffer_ready && pp->buffer) {
        aoe_surface_set_key_ror(pp->buffer, 1, 1);
        if (p->area->surface)
            aoe_surface_blit_ror(pp->buffer, p->area->surface, pp->dest_x, pp->dest_y,
                                 pp->source_rect, 1);
        aoe_surface_set_key_ror(pp->buffer, 0, 1);
        p->vt->end_draw(p);
        return;
    }
    if (pf && pf->text_begin && pf->text_rect &&
        pf->text_begin(pf->context, p->area, &p->clip, pp->font)) {
        char text[256];
        int32_t l = p->rect.left, t = p->rect.top;
        label(pp, text, sizeof text);
        shadowed(pf, text, l, t, p->width - 1 + l, p->height - 1 + t, pp->shadow,
                 pp->flash == 1 ? pp->shadow : pp->color);
        pf->text_end(pf->context);
    }
    p->vt->end_draw(p);
}

static void true_rect(AoeUiPanelRor *p, AoeUiRectRor *out) {
    AoePopulationPanelRor *pp = PP(p);
    if (pp->buffer && p->area && p->visible && p->active) {
        out->left = pp->dest_x;
        out->top = pp->dest_y;
        out->right = pp->dest_right;
        out->bottom = pp->dest_bottom;
        return;
    }
    *out = p->rect;
}

static int32_t idle(AoeUiPanelRor *p) {
    aoe_ui_panel_handle_idle_ror(p);
    return 0;
}

static AoeUiPanelVtblRor vtable;
static int vtable_ready;

const AoeUiPanelVtblRor *aoe_population_panel_vtable_ror(void) {
    if (!vtable_ready) {
        vtable = aoe_ui_panel_vtable_ror;
        vtable.destroy = destroy;
        vtable.set_rect = aoe_population_panel_set_rect_ror;
        vtable.draw = aoe_population_panel_draw_ror;
        vtable.handle_idle = idle;
        vtable.get_true_rect = true_rect;
        vtable_ready = 1;
    }
    return &vtable;
}
