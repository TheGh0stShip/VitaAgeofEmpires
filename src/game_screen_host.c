/* The port's Game Screen (integration support; see game_screen_host.h). */
#include "game_screen_host.h"

#include <stdlib.h>
#include <string.h>

#include "draw_system.h"
#include "ui_ez_slots.h"

#define GS(p) ((AoeGameScreenHostRor *)(p))

static const char *const NAME = "Game Screen";

/* Edge scrolling: the pointer within this many pixels of a screen edge. */
enum { EDGE = 4 };

void aoe_game_screen_host_view_rect_ror(int32_t width, int32_t height, int32_t rect[4]) {
    /* 0x0047f000 without the overlay: (0, 0x14, W, vh), vh 0x14e / 0x1c6 /
       0x26e for the 640 / 800 / 1024 layouts. */
    int32_t W = 640, vh = 0x14e;
    if (width >= 0x400 && height >= 0x300) {
        W = 0x400;
        vh = 0x26e;
    } else if (width >= 800 && height >= 600) {
        W = 800;
        vh = 0x1c6;
    }
    rect[0] = 0;
    rect[1] = 0x14;
    rect[2] = W;
    rect[3] = vh;
}

static void gs_destroy(AoeUiPanelRor *p) {
    AoeGameScreenHostRor *s = GS(p);
    free(s->pixels);
    s->pixels = NULL;
    aoe_ui_ez_release_ror(&s->ez);
}

static uint8_t *surface_row(AoeDrawSurfaceRor *d, int32_t y) {
    if (d->rows) return d->rows[y];
    if (d->view.direction == 1) return d->view.bits + (size_t)y * (size_t)d->view.pitch;
    return d->view.bits + (size_t)(d->view.height - 1 - y) * (size_t)d->view.pitch;
}

/* The screen's background, then the world view copied into the area
   inside the panel's clip. */
static void gs_draw(AoeUiPanelRor *p) {
    AoeGameScreenHostRor *s = GS(p);
    AoeDrawSurfaceRor *d = p->area ? p->area->surface : NULL;
    int32_t x0, y0, x1, y1, y;
    aoe_ui_ez_draw_slot_ror(p);
    if (!s->world || !s->pixels || !d) return;
    if (!aoe_demo_render_indexed(s->world, s->pixels, (size_t)s->view_w)) return;
    if (!aoe_surface_lock_ror(d, 1)) return;
    x0 = p->x + s->view_x;
    y0 = p->y + s->view_y;
    x1 = x0 + s->view_w;
    y1 = y0 + s->view_h;
    if (x0 < p->clip.left) x0 = p->clip.left;
    if (y0 < p->clip.top) y0 = p->clip.top;
    if (x1 > p->clip.right + 1) x1 = p->clip.right + 1;
    if (y1 > p->clip.bottom + 1) y1 = p->clip.bottom + 1;
    if (x1 > d->view.width) x1 = d->view.width;
    if (y1 > d->view.height) y1 = d->view.height;
    for (y = y0; y < y1 && x0 < x1; ++y)
        memcpy(surface_row(d, y) + x0,
               s->pixels + (size_t)(y - p->y - s->view_y) * (size_t)s->view_w +
                   (x0 - p->x - s->view_x),
               (size_t)(x1 - x0));
    aoe_surface_unlock_ror(d);
}

static int key_held(const AoeUiPanelRor *p, int vk) {
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    return pf && pf->key_down && pf->key_down(pf->context, vk);
}

static uint32_t now_ms(const AoeUiPanelRor *p) {
    const AoeUiPlatformRor *pf = p->system ? p->system->platform : NULL;
    return pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
}

/* The world advanced by the time since the last idle (at most a quarter
   second), the view scrolled, and the screen redrawn. */
static int32_t gs_idle(AoeUiPanelRor *p) {
    AoeGameScreenHostRor *s = GS(p);
    uint32_t now = now_ms(p);
    float dt = s->last_ms ? (float)(now - s->last_ms) / 1000.0f : 0.0f;
    AoeDemoInput in;
    memset(&in, 0, sizeof in);
    s->last_ms = now;
    if (dt > 0.25f) dt = 0.25f;
    if (key_held(p, 0x25)) in.scroll_x -= 1.0f;   /* VK_LEFT */
    if (key_held(p, 0x27)) in.scroll_x += 1.0f;   /* VK_RIGHT */
    if (key_held(p, 0x26)) in.scroll_y -= 1.0f;   /* VK_UP */
    if (key_held(p, 0x28)) in.scroll_y += 1.0f;   /* VK_DOWN */
    if (s->mouse_x >= 0 && p->area) {
        if (s->mouse_x < EDGE) in.scroll_x = -1.0f;
        else if (s->mouse_x >= p->area->width - EDGE) in.scroll_x = 1.0f;
        if (s->mouse_y < EDGE) in.scroll_y = -1.0f;
        else if (s->mouse_y >= p->area->height - EDGE) in.scroll_y = 1.0f;
    }
    if (s->world && dt > 0.0f) aoe_demo_update(s->world, &in, dt);
    p->vt->set_redraw(p, 1);
    return aoe_ui_panel_handle_idle_ror(p);
}

static int32_t gs_mouse_move(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t ctrl, int32_t shift) {
    AoeGameScreenHostRor *s = GS(p);
    s->mouse_x = x;
    s->mouse_y = y;
    return aoe_ui_panel_mouse_move_ror(p, x, y, ctrl, shift);
}

static AoeUiEzVtblRor vtable = {
    AOE_UI_EZ_SLOTS_MD(gs_destroy, gs_draw, gs_idle, aoe_ui_panel_handle_paint_ror,
                       aoe_ui_ez_no_key_action_ror, aoe_ui_panel_zero_ror,
                       aoe_ui_ez_action_slot_ror, aoe_ui_screen_focus_slot_ror,
                       aoe_ui_ez_mouse_down_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

int aoe_game_screen_host_init_ror(AoeGameScreenHostRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area,
                                  AoeDemo *world) {
    AoeUiEzRor *ez = &s->ez;
    int32_t r[4];
    memset(s, 0, sizeof *s);
    s->ui = ui;
    s->world = world;
    s->mouse_x = s->mouse_y = -1;
    vtable.base.mouse_move = gs_mouse_move;
    aoe_ui_screen_init_ror(ez, ui->system, ui->ez_ops, NAME);
    ez->panel.vt = &vtable.base;
    if (!aoe_ui_screen_setup_ror(ez, area, "", 0xc388, 1)) {
        ez->panel.field_d8 = 1;
        return 0;
    }
    aoe_game_screen_host_view_rect_ror(area->width, area->height, r);
    s->view_x = r[0];
    s->view_y = r[1];
    s->view_w = r[2];
    s->view_h = r[3];
    s->pixels = calloc((size_t)s->view_w * (size_t)s->view_h, 1);
    return 1;
}
