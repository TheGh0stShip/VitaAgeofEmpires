#include "resource_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RP(p) ((AoeResourcePanelRor *)(p))

static uint32_t now_ms(const AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

static float attribute(const AoeResourcePanelRor *r, int i) {
    return r->player && r->player->attributes ? r->player->attributes[i] : 0.0f;
}

void aoe_resource_panel_cache_ror(AoeResourcePanelRor *r) {
    int16_t i;
    if (!r->player) return;
    if (!r->cached || r->cached_count != r->player->attribute_count) {
        free(r->cached);
        r->cached = NULL;
        r->cached_count = r->player->attribute_count;
        r->cached = calloc(r->cached_count > 0 ? (size_t)r->cached_count : 1,
                           sizeof *r->cached);
        if (!r->cached) return;
    }
    for (i = 0; i < r->cached_count; ++i)
        r->cached[i] = (int16_t)(int32_t)r->player->attributes[i];
}

void aoe_resource_panel_set_player_ror(AoeResourcePanelRor *r,
                                       const AoeResourcePlayerRor *player) {
    r->player = player;
    aoe_resource_panel_cache_ror(r);
    r->panel.vt->set_redraw(&r->panel, 1);
}

int aoe_resource_panel_init_ror(AoeResourcePanelRor *r, AoeUiSystemRor *system,
                                AoeUiAreaRor *area, AoeUiPanelRor *parent,
                                uintptr_t font,
                                const AoeResourcePlayerRor *player) {
    aoe_ui_panel_init_ror(&r->panel, system);
    r->font = font;
    r->cached_count = 0;
    r->cached = NULL;
    r->picture = NULL;
    r->last_ms = 0;
    r->player = NULL;
    r->shadow = 0;
    r->panel.vt = aoe_resource_panel_vtable_ror();
    r->interval_ms = 500;
    r->color = 0xffffff;
    if (!aoe_ui_panel_setup_ror(&r->panel, area, parent, 0, 0, 0, 0, 0)) {
        r->panel.field_d8 = 1;
        return 0;
    }
    aoe_resource_panel_set_player_ror(r, player);
    return 1;
}

static void destroy(AoeUiPanelRor *p) {
    AoeResourcePanelRor *r = RP(p);
    free(r->cached);
    r->cached = NULL;
    r->picture = NULL;
    aoe_ui_panel_release_ror(p);
}

/* +0x30 0x004f7db0. */
static const int kOrder[4] = {1, 0, 3, 2};
static const int32_t kX[4] = {0x20, 0x63, 0xa6, 0xe9};

static void draw(AoeUiPanelRor *p) {
    AoeResourcePanelRor *r = RP(p);
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    int i;
    if (!p->area || !p->visible || !p->active) {
        p->redraw = 0;
        return;
    }
    if (p->parent) p->parent->vt->draw_rect(p->parent, &p->clip);
    if (!r->player) {
        p->redraw = 0;
        return;
    }
    p->vt->begin_draw(p, 0);
    if (pf && pf->text_begin &&
        pf->text_begin(pf->context, p->area, &p->clip, r->font)) {
        for (i = 0; i < 4; ++i) {
            char n[24];
            int32_t len;
            snprintf(n, sizeof n, "%ld", (long)(int32_t)attribute(r, kOrder[i]));
            len = (int32_t)strlen(n);
            pf->text_out(pf->context, p->x + kX[i] - 1, p->y + 3, n, len,
                         r->shadow);
            pf->text_out(pf->context, p->x + kX[i], p->y + 2, n, len, r->color);
        }
        pf->text_end(pf->context);
    }
    p->vt->end_draw(p);
}

/* +0x4c 0x004f8110. */
static int32_t idle(AoeUiPanelRor *p) {
    AoeResourcePanelRor *r = RP(p);
    int32_t mode;
    uint32_t now;
    int16_t i;
    aoe_ui_panel_handle_idle_ror(p);
    mode = r->game_mode ? *r->game_mode : 0;
    if (mode != 4 && mode != 6 && mode != 7 && mode != 5) return 0;
    now = now_ms(p);
    if (now - r->last_ms < r->interval_ms) return 0;
    for (i = 0; i < r->cached_count; ++i) {
        if ((int16_t)(int32_t)r->player->attributes[i] != r->cached[i]) {
            aoe_resource_panel_cache_ror(r);
            p->vt->set_redraw(p, 1);
            break;
        }
    }
    r->last_ms = now_ms(p);
    return 0;
}

/* +0xd0 0x004f8280: the tooltip of the value under the pointer. */
static int get_help(AoeUiPanelRor *p, char *text, int32_t *page, int32_t x,
                    int32_t y) {
    AoeResourcePanelRor *r = RP(p);
    int32_t id, i;
    const char *fmt;
    if (!p->active || !p->vt->is_inside(p, x, y)) return 0;
    if (x < p->x + 0x37) id = 0x1d4ee, i = 1;
    else if (x < p->x + 0x6e) id = 0x1d4ed, i = 0;
    else if (x < p->x + 0xa5) id = 0x1d4ef, i = 3;
    else id = 0x1d4f0, i = 2;
    fmt = r->string ? r->string(r->string_context, id) : NULL;
    if (text) snprintf(text, 0x100, fmt ? fmt : "%d", (int)(int32_t)attribute(r, i));
    if (page) *page = -1;
    return 1;
}

static AoeUiPanelVtblRor vtable;
static int vtable_ready;

const AoeUiPanelVtblRor *aoe_resource_panel_vtable_ror(void) {
    if (!vtable_ready) {
        vtable = aoe_ui_panel_vtable_ror;
        vtable.destroy = destroy;
        vtable.draw = draw;
        vtable.handle_idle = idle;
        vtable.get_help = get_help;
        vtable_ready = 1;
    }
    return &vtable;
}
