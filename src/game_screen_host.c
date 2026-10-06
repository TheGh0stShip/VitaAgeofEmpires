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

static void free_shape(AoeGameScreenHostRor *s, void *shape) {
    const AoeUiEzOpsRor *ez = s->ui->ez_ops;
    if (shape && ez && ez->free_shape) ez->free_shape(ez->context, shape);
}

static void gs_destroy(AoeUiPanelRor *p) {
    AoeGameScreenHostRor *s = GS(p);
    int i;
    for (i = 0; i < AOE_GSP_SHAPES; ++i) {
        free_shape(s, s->shapes[i]);
        s->shapes[i] = NULL;
    }
    free(s->pixels);
    s->pixels = NULL;
    aoe_ui_ez_release_ror(&s->ez);
}

static uint8_t *surface_row(AoeDrawSurfaceRor *d, int32_t y) {
    if (d->rows) return d->rows[y];
    if (d->view.direction == 1) return d->view.bits + (size_t)y * (size_t)d->view.pitch;
    return d->view.bits + (size_t)(d->view.height - 1 - y) * (size_t)d->view.pitch;
}

/* The translated draw's (0x00484110) calls on this panel. */
static AoeDrawSurfaceRor *area_surface(const AoeUiPanelRor *p) {
    return p->area ? p->area->surface : NULL;
}
static int d_zero(void *c) { (void)c; return 0; }
static int32_t d_redraw_level(void *c) { return ((AoeUiPanelRor *)c)->redraw; }
static void d_begin(void *c, int32_t clear) {
    AoeUiPanelRor *p = c;
    p->vt->begin_draw(p, clear);
}
static void d_end(void *c) {
    AoeUiPanelRor *p = c;
    p->vt->end_draw(p);
}
static void d_get_clip(void *c, int32_t r[4]) {
    const AoeUiPanelRor *p = c;
    r[0] = p->clip.left;
    r[1] = p->clip.top;
    r[2] = p->clip.right;
    r[3] = p->clip.bottom;
}
static void d_set_clip(void *c, const int32_t r[4]) {
    AoeUiPanelRor *p = c;
    p->clip.left = r[0];
    p->clip.top = r[1];
    p->clip.right = r[2];
    p->clip.bottom = r[3];
}
static int d_lock(void *c, const char *who) {
    AoeDrawSurfaceRor *d = area_surface(c);
    (void)who;
    return d && aoe_surface_lock_ror(d, 1) != NULL;
}
static void d_unlock(void *c, const char *who) {
    AoeDrawSurfaceRor *d = area_surface(c);
    (void)who;
    if (d) aoe_surface_unlock_ror(d);
}
static int d_view_rect(void *c, int32_t r[4]) { (void)c; (void)r; return 0; }
static void d_box(void *c, int32_t l, int32_t t, int32_t r, int32_t b, uint8_t color) {
    (void)c; (void)l; (void)t; (void)r; (void)b; (void)color;
}
static int d_has_shape(void *c) { return GS(c)->shapes[AOE_GSP_GAME] != NULL; }
static void d_shape(void *c, int32_t frame, int32_t x, int32_t y) {
    AoeGameScreenHostRor *s = GS(c);
    AoeDrawSurfaceRor *d = area_surface(c);
    const AoeUiEzOpsRor *ez = s->ui->ez_ops;
    if (d && ez && ez->shape_ops)
        aoe_shape_draw_ror(s->shapes[AOE_GSP_GAME], &d->view, s->ez.panel.x + x, s->ez.panel.y + y,
                           frame, 0, NULL, ez->shape_ops);
}

/* The screen's draw (0x00484110: the bars), then the world view copied
   into the area inside the panel's clip (the view is a child panel in
   the original, drawn after its parent). */
static void gs_draw(AoeUiPanelRor *p) {
    AoeGameScreenHostRor *s = GS(p);
    AoeDrawSurfaceRor *d = p->area ? p->area->surface : NULL;
    int32_t x0, y0, x1, y1, y;
    const AoeGameScreenDrawOpsRor ops = {p, d_zero, d_redraw_level, d_begin, d_end,
                                         d_get_clip, d_set_clip, d_lock, d_unlock,
                                         d_view_rect, d_box, d_has_shape, d_shape};
    aoe_game_screen_draw_ror(&s->screen, &ops);
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

/* The style's (0x004816d0) calls. The panels it tells about the player
   and the buttons it styles are not assembled yet, so those calls do
   nothing; the screen's own settings and shapes are real. */
static void st_none(void *c) { (void)c; }
static void st_info(void *c, const char *name, int32_t id) { aoe_ui_ez_set_info_ror(&GS(c)->ez, name, id); }
static void st_popup(void *c, const char *name, int32_t id) { aoe_ui_ez_set_popup_ror(&GS(c)->ez, name, id); }
static void st_pictures(void *c, const char *name, int32_t id) {
    aoe_ui_ez_set_button_pictures_ror(&GS(c)->ez, name, id);
}
static void st_bevel(void *c, const uint8_t k[6]) { aoe_ui_ez_set_bevel_colors_ror(&GS(c)->ez, k); }
static void st_button_bevel(void *c, uint16_t off, int32_t type, const uint8_t k[6]) {
    (void)c; (void)off; (void)type; (void)k;
}
static void st_button_picture(void *c, uint16_t off, int32_t state, void *pic, int16_t frame) {
    (void)c; (void)off; (void)state; (void)pic; (void)frame;
}
static int32_t st_button_id(void *c, uint16_t off) { (void)c; (void)off; return 0; }
static void st_button_colors(void *c, uint16_t off, int32_t state, uint32_t a, uint32_t b) {
    (void)c; (void)off; (void)state; (void)a; (void)b;
}
static void st_border(void *c, int32_t i, void *shape) { (void)c; (void)i; (void)shape; }
static void st_colors(void *c, uint16_t off, uint32_t a, uint32_t b) {
    (void)c; (void)off; (void)a; (void)b;
}
static void *st_load(void *c, const char *name, int32_t id) {
    const AoeUiEzOpsRor *ez = GS(c)->ui->ez_ops;
    return ez && ez->load_shape ? ez->load_shape(ez->context, name, id) : NULL;
}
static void st_free(void *c, void *shape) { free_shape(GS(c), shape); }
static void st_redraw(void *c) { GS(c)->ez.panel.vt->set_redraw(&GS(c)->ez.panel, 1); }

/* 0x004816d0 for the viewed player (the port plays as player 1). */
static void restyle(AoeGameScreenHostRor *s) {
    AoeGameScreenStyleOpsRor o;
    uint8_t style = 0;
    memset(&o, 0, sizeof o);
    o.context = s;
    o.width = s->screen.width;
    o.height = s->screen.height;
    o.style = &s->style;
    o.style_width = &s->style_width;
    o.player_id = 1;
    if (!aoe_demo_player_icon_set(s->world, 1, &style)) style = 0;
    o.player_style = style;
    o.player_to_panels = st_none;
    o.mode_changed = st_none;
    o.store_player = st_none;
    o.age = st_none;
    o.set_info = st_info;
    o.set_popup = st_popup;
    o.set_button_pictures = st_pictures;
    o.set_bevel_colors = st_bevel;
    o.button_bevel = st_button_bevel;
    o.button_picture = st_button_picture;
    o.button_id = st_button_id;
    o.button_text_colors = st_button_colors;
    o.button_border = st_border;
    o.text_colors = st_colors;
    o.load_shape = st_load;
    o.free_shape = st_free;
    o.rebuild_buttons = st_none;
    o.redraw = st_redraw;
    aoe_game_screen_style_ror(&o, s->shapes, &s->page_picture);
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
    s->screen.width = ez->panel.width;
    s->screen.height = ez->panel.height;
    s->screen.self = &ez->panel;
    s->style = -1;
    restyle(s);
    return 1;
}
