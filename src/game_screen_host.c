/* The port's Game Screen (integration support; see game_screen_host.h). */
#include "game_screen_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "draw_system.h"
#include "game_ror_flow.h"
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
    if (s->object_panel) {
        s->object_panel->panel.vt->destroy(&s->object_panel->panel);
        free(s->object_panel);
        s->object_panel = NULL;
    }
    free_shape(s, s->unit_icons);
    s->unit_icons = NULL;
    for (i = 0; i < 5; ++i) {
        free_shape(s, s->building_icons[i]);
        s->building_icons[i] = NULL;
    }
    if (s->map) {
        s->map->map.panel.vt->destroy(&s->map->map.panel);
        free(s->map);
        s->map = NULL;
    }
    if (s->resources) {
        s->resources->panel.vt->destroy(&s->resources->panel);
        free(s->resources);
        s->resources = NULL;
    }
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
    if (!s->world || !s->pixels || !s->rendered || !d) return;
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

/* The world drawn into the view's pixels, once per idle (the panel's
   draw may run several times a frame for its children's rectangles). */
static void render_view(AoeGameScreenHostRor *s) {
    if (s->world && s->pixels)
        s->rendered = aoe_demo_render_indexed(s->world, s->pixels, (size_t)s->view_w);
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
    render_view(s);
    /* The minimap every 0x14d ms (the screen's +0x5c0 interval). */
    if (s->map && now - s->map_time >= 0x14d) {
        s->map_time = now;
        s->map->map.panel.vt->set_redraw(&s->map->map.panel, 1);
    }
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
/* 0x004f81d0 on +0x4c0 (the view, map and object panel are not made). */
static void st_panels(void *c) {
    AoeGameScreenHostRor *s = GS(c);
    if (s->resources && s->resource_player.attribute_count)
        aoe_resource_panel_set_player_ror(s->resources, &s->resource_player);
}
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
    if (off == 0x4c0 && GS(c)->resources) aoe_resource_panel_set_colors_ror(GS(c)->resources, a, b);
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
    o.player_to_panels = st_panels;
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

static const char *ui_string(void *c, int32_t id) {
    static char text[256];
    aoe_ui_string_ror(GS(c)->ez.panel.system, id, text, sizeof text);
    return text;
}

/* AOE_GSC_RESOURCES: 0x004f7c00 with font record 11 (help 0x4e25), at the
   layout's place for +0x4c0 (0, 0, 0x10e, 0x14). */
static int make_resources(AoeGameScreenHostRor *s) {
    const AoeUiEzOpsRor *ez = s->ui->ez_ops;
    uintptr_t font = 0;
    int32_t fw = 0, fh = 0;
    AoeResourcePanelRor *r = calloc(1, sizeof *r);
    if (!r) return 0;
    if (ez && ez->font) ez->font(ez->context, 11, &font, &fw, &fh);
    if (!aoe_demo_player_attributes(s->world, 1, &s->resource_player.attribute_count,
                                    &s->resource_player.attributes))
        s->resource_player.attribute_count = 0;
    if (!aoe_resource_panel_init_ror(r, s->ez.panel.system, s->ez.panel.area, &s->ez.panel, font,
                                     s->resource_player.attribute_count ? &s->resource_player : NULL)) {
        r->panel.vt->destroy(&r->panel);
        free(r);
        return 0;
    }
    s->resources = r;
    r->game_mode = &s->game_state;
    r->string = ui_string;
    r->string_context = s;
    r->panel.vt->set_fixed_position(&r->panel, 0, 0, 0x10e, 0x14);
    r->panel.help_id = 0x4e25;
    return 1;
}

uint8_t *aoe_game_screen_host_bmp_ror(const uint8_t *data, uint32_t size, int32_t *width,
                                      int32_t *height) {
    uint32_t offset, header, w, h, stride, y;
    int32_t raw_h;
    uint8_t *out;
    if (!data || size < 54 || data[0] != 'B' || data[1] != 'M') return NULL;
    offset = (uint32_t)data[10] | (uint32_t)data[11] << 8 | (uint32_t)data[12] << 16 |
             (uint32_t)data[13] << 24;
    header = (uint32_t)data[14] | (uint32_t)data[15] << 8;
    w = (uint32_t)data[18] | (uint32_t)data[19] << 8 | (uint32_t)data[20] << 16 |
        (uint32_t)data[21] << 24;
    raw_h = (int32_t)((uint32_t)data[22] | (uint32_t)data[23] << 8 | (uint32_t)data[24] << 16 |
                      (uint32_t)data[25] << 24);
    if (header < 40 || data[28] != 8 || data[29] != 0 || w == 0 || w > 4096 || raw_h == 0 ||
        raw_h > 4096 || raw_h < -4096)
        return NULL;
    h = (uint32_t)(raw_h < 0 ? -raw_h : raw_h);
    stride = (w + 3) & ~3u;
    if (offset > size || (uint64_t)stride * h > size - offset) return NULL;
    out = malloc((size_t)w * h);
    if (!out) return NULL;
    for (y = 0; y < h; ++y) {
        uint32_t src = raw_h < 0 ? y : h - 1 - y;
        memcpy(out + (size_t)y * w, data + offset + (size_t)src * stride, w);
    }
    *width = (int32_t)w;
    *height = (int32_t)h;
    return out;
}

/* The minimap's calls the world does not answer. */
static uint8_t *mm_bitmap(void *c, const char *name, int32_t id, int32_t *w, int32_t *h) {
    AoeGameScreenHostRor *s = c;
    uint32_t size = 0;
    uint8_t *data, *bits;
    (void)name;
    if (!s->resource) return NULL;
    data = s->resource(s->resource_context, 0x62696e61u, id, &size);
    bits = aoe_game_screen_host_bmp_ror(data, size, w, h);
    free(data);
    return bits;
}
static int32_t mg_state(void *c) { return GS(c)->game_state; }
static int32_t mg_zero(void *c) { (void)c; return 0; }
/* Mouse style 2: the left button moves the view, the right one orders. */
static int32_t mg_style(void *c) { (void)c; return 2; }
static int mg_orders(void *c) { (void)c; return 1; }
static void mg_mode(void *c, int32_t mode, int32_t value) { (void)c; (void)mode; (void)value; }
static void mg_clear(void *c, const void *player) { (void)c; (void)player; }
static uint8_t mg_order(void *c, const void *player, int32_t slot, float x, float y) {
    (void)c; (void)player; (void)slot; (void)x; (void)y;
    return 0;
}
static void mg_point(void *c, const void *player, float x, float y) {
    (void)c; (void)player; (void)x; (void)y;
}
static void mg_frame(void *c, int32_t redraw) { GS(c)->ez.panel.vt->set_redraw(&GS(c)->ez.panel, redraw); }

/* AOE_GSC_MINIMAP: 0x005085a0, its setup 0x0042c4f0 (no buffer: the map
   makes its own), help 0x4e33, the world (0x0042c780), "map.bmp" 0xc4e1
   (0x0042c540) and the viewed player (0x0042cbb0), at the layout's place
   for +0x4bc. */
static int make_minimap(AoeGameScreenHostRor *s) {
    static const int32_t at640[4] = {0x19b, 0x16a, 0xdc, 0x72};
    AoeGameMinimapRor *g = calloc(1, sizeof *g);
    if (!g) return 0;
    aoe_demo_minimap_ops(s->world, &s->map_ops, mm_bitmap, s);
    s->map_game_ops = (AoeGameMinimapOpsRor){s, mg_state, mg_zero, mg_style, mg_orders, mg_mode,
                                             mg_clear, mg_order, mg_point, mg_frame};
    aoe_game_minimap_init_ror(g, s->ez.panel.system, &s->map_ops, &s->map_game_ops);
    if (g->map.panel.field_d8 ||
        !aoe_diamond_map_setup_ror(&g->map, s->ez.panel.area, &s->ez.panel, 0, 0, 0, 0, 0, NULL)) {
        g->map.panel.vt->destroy(&g->map.panel);
        free(g);
        return 0;
    }
    s->map = g;
    g->map.panel.help_id = 0x4e33;
    aoe_diamond_map_set_world_ror(&g->map, s->world);
    aoe_diamond_map_load_ror(&g->map, "map.bmp", 0xc4e1);
    aoe_diamond_map_set_player_ror(&g->map, s->map_ops.world_player(s->world, 1));
    g->map.panel.vt->set_fixed_position(&g->map.panel, at640[0], at640[1], at640[2], at640[3]);
    return 1;
}

/* A press on the world view (no child panel overlaps it; help mode
   first): with mouse style 2 the left button selects
   the unit drawn there (or clears the selection) and the right button
   orders the selection there. The original's view panel does this
   through its mouse handlers (0x0050f420's game view; not assembled). */
static int32_t gs_mouse_down(AoeUiPanelRor *p, uint8_t button, int32_t x, int32_t y, int32_t c,
                             int32_t sh) {
    AoeGameScreenHostRor *s = GS(p);
    int32_t vx = x - p->x - s->view_x, vy = y - p->y - s->view_y;
    if (!s->world || vx < 0 || vy < 0 || vx >= s->view_w || vy >= s->view_h ||
        (button != 1 && button != 2) || aoe_ui_ez_help_press_ror(&s->ez, button, x, y))
        return aoe_ui_ez_mouse_down_slot_ror(p, button, x, y, c, sh);
    if (button == 1) aoe_demo_select(s->world, aoe_demo_pick(s->world, vx, vy));
    else aoe_demo_order(s->world, vx, vy);
    render_view(s);
    p->vt->set_redraw(p, 1);
    return 1;
}

/* The translated methods' calls on the screen (the escape, menu and
   selection paths). */
#define SO(c) ((AoeGameScreenHostRor *)(c))
static int32_t so_zero(void *c) { (void)c; return 0; }
static int so_none_int(void *c) { (void)c; return 0; }
static void so_none(void *c) { (void)c; }
static int32_t so_edit_mode(void *c, int32_t *sub) {
    (void)c;
    if (sub) *sub = 0;
    return 0;
}
static void so_view_cancel(void *c, void *view) { (void)c; (void)view; }
static void so_set_mode(void *c, int32_t mode, int32_t sub) { (void)c; (void)mode; (void)sub; }
static void so_clear_selection(void *c) { aoe_demo_select(SO(c)->world, NULL); }
static void so_redraw(void *c, void *panel, int all) {
    (void)panel;
    (void)all;
    render_view(SO(c));
    SO(c)->ez.panel.vt->set_redraw(&SO(c)->ez.panel, 1);
}
static int32_t so_state(void *c) { return SO(c)->game_state; }
static void so_comm_pause(void *c, int32_t on, int32_t flag) { (void)c; (void)on; (void)flag; }
/* The dialogs the screen makes by name: the Menu Dialog (0x00433c30). */
static void so_open_dialog(void *c, const char *name) {
    AoeGameScreenHostRor *s = SO(c);
    if (strcmp(name, "Menu Dialog") == 0) {
        AoeMenuDialogRor *m = calloc(1, sizeof *m);
        if (m && !aoe_menu_dialog_init_ror(m, s->ui, &s->ez.panel)) {
            m->dialog.ez.panel.vt->destroy(&m->dialog.ez.panel);
            free(m);
        }
    }
}

static void screen_ops(AoeGameScreenHostRor *s) {
    AoeGameScreenOpsRor *o = &s->screen_ops;
    memset(o, 0, sizeof *o);
    o->context = s;
    o->escape_armed = so_none_int;
    o->escape_disarm = so_none;
    o->comm_waiting = so_zero;
    o->edit_mode = so_edit_mode;
    o->view_cancel = so_view_cancel;
    o->outline_reset = so_none;
    o->set_mode = so_set_mode;
    o->clear_selection = so_clear_selection;
    o->clear_block = so_none;
    o->redraw = so_redraw;
    o->game_state = so_state;
    o->run_commands = so_none;
    o->paused = so_none_int;
    o->comm_pause = so_comm_pause;
    o->open_dialog = so_open_dialog;
    s->screen.ops = o;
}

/* The key table's Escape and F10 (the screen's +0xb0 keys). */
static int32_t gs_key_action(AoeUiPanelRor *p, int32_t key, int32_t count, int32_t alt,
                             int32_t ctrl, int32_t shift) {
    AoeGameScreenHostRor *s = GS(p);
    (void)count;
    if (alt || ctrl || shift) return 0;
    if (key == 0x1b) {
        aoe_game_screen_escape_ror(&s->screen);
        return 1;
    }
    if (key == 0x79) {
        aoe_game_screen_menu_ror(&s->screen);
        return 1;
    }
    return 0;
}

/* The action's steps (0x00480fe0). */
static void leave_game(AoeGameScreenHostRor *s) {
    const AoeGameUiOpsRor *ops = s->ui->ops;
    AoeUiSystemRor *system = s->ez.panel.system;
    if (!ops || !ops->open_main_menu) return;
    ops->open_main_menu(ops->context);
    if (aoe_ui_system_show_named_ror(system, "Main Menu", 0))
        aoe_ui_system_close_ror(system, NAME);
}
static void act_step(void *c, int32_t id, int32_t a, int32_t b, int32_t arg) {
    AoeGameScreenHostRor *s = c;
    const AoeGameUiOpsRor *ops = s->ui->ops;
    (void)a; (void)b; (void)arg;
    switch (id) {
    case AOE_GS_ACT_SHOW_GAME_SCREEN:
        aoe_ui_system_show_named_ror(s->ez.panel.system, NAME, 0);
        break;
    case AOE_GS_ACT_LEAVE:
        leave_game(s);
        break;
    case AOE_GS_ACT_CLOSE:
        if (ops && ops->quit_program) ops->quit_program(ops->context);
        break;
    default:   /* restart, resign, load, resume, commands: not integrated */
        break;
    }
}
static const char *act_return_name(void *c) {
    AoeGameScreenHostRor *s = c;
    return s->action_from ? aoe_ui_panel_name_ror(s->action_from) : NULL;
}
static void act_remove(void *c, const char *name) { aoe_ui_system_close_ror(GS(c)->ez.panel.system, name); }
static int32_t act_base(void *c, void *from, int32_t code, int32_t a, int32_t b) {
    return aoe_ui_ez_action_slot_ror(&GS(c)->ez.panel, from, code, a, b);
}

static int32_t gs_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code, int32_t a,
                         int32_t b) {
    AoeGameScreenHostRor *s = GS(p);
    AoeGameScreenActionOpsRor o;
    memset(&o, 0, sizeof o);
    o.context = s;
    o.return_name = act_return_name;
    o.remove_screen = act_remove;
    o.global_7c0338 = &s->ui->quit_requested;
    o.field_730 = &s->field_730;
    o.step = act_step;
    o.base_action = act_base;
    s->action_from = from;
    return aoe_game_screen_action_ror(&o, from, code, a, b);
}

/* The object panel's calls (0x004f83d0's), over the world. */
static void *op_selected(void *c, const void *player) {
    AoeLiveUnit *u = NULL;
    (void)player;
    return aoe_demo_selection(GS(c)->world, &u, 1) ? u : NULL;
}
static int op_read(void *c, void *object, AoeInfoObjectRor *out) {
    return aoe_demo_info_object(GS(c)->world, object, out);
}
static float op_attribute(void *c, const void *player, int index) {
    int16_t n = 0;
    const float *v = NULL;
    int i;
    for (i = 0; i < 9; ++i)
        if (aoe_demo_world_player(GS(c)->world, i) == player) break;
    if (i == 9 || !aoe_demo_player_attributes(GS(c)->world, i, &n, &v) || index < 0 || index >= n)
        return 0.0f;
    return v[index];
}
static uint8_t op_civ(void *c, const void *player) { return aoe_demo_player_civ(GS(c)->world, player); }
static int op_nearest(void *c, const void *player, int16_t cls, float x, float y, uint8_t state,
                      void *unit, float *ox, float *oy) {
    (void)c; (void)player; (void)cls; (void)x; (void)y; (void)state; (void)unit; (void)ox; (void)oy;
    return 0;
}
static int op_zero(void *c) { (void)c; return 0; }
static char *op_load_string(void *c, int32_t id, char *buf, int32_t len) {
    aoe_ui_string_ror(GS(c)->ez.panel.system, id, buf, len);
    return buf;
}
static char *op_base_format(void *c, int32_t kind, int32_t index, int32_t sub, char *buf,
                            int32_t len) {
    (void)c; (void)kind; (void)index; (void)sub;
    if (len > 0) buf[0] = '\0';
    return buf;
}
/* The game's +0x28 formatter (0x004ff580), kind 0x69: a civilisation. */
static void op_civ_name(void *c, uint8_t civ, char *out, int size) {
    GameRorFlowStringOps ops = {c, op_load_string, op_base_format};
    game_ror_flow_format_message(&ops, 0x69, civ, 0, out, size);
}
static void op_string(void *c, int32_t id, char *out, int size) {
    aoe_ui_string_ror(GS(c)->ez.panel.system, id, out, size);
}
/* A frame onto the area; `remap` >= 0 draws the player-colour run
   (0x10..0x17) in the owner's run starting there (the original passes the
   owner's colour table, 0x0049fe00 mode 1). */
static void op_draw_shape(void *c, AoeUiAreaRor *area, void *shape, int32_t frame, int32_t x,
                          int32_t y, int32_t remap) {
    AoeGameScreenHostRor *s = GS(c);
    const AoeUiEzOpsRor *ez = s->ui->ez_ops;
    AoeDrawSurfaceRor *d = area ? area->surface : NULL;
    uint8_t table[256];
    int i;
    if (!shape || !d || !ez || !ez->shape_ops || !aoe_surface_lock_ror(d, 1)) return;
    for (i = 0; i < 256; ++i) table[i] = (uint8_t)i;
    if (remap >= 0)
        for (i = 0; i < 8; ++i) table[0x10 + i] = (uint8_t)(remap + i);
    aoe_shape_draw_ror(shape, &d->view, x, y, frame, remap >= 0 ? 1 : 0, remap >= 0 ? table : NULL,
                       ez->shape_ops);
    aoe_surface_unlock_ror(d);
}
static int32_t op_frames(void *c, void *shape) {
    (void)c;
    return shape ? aoe_shape_count_ror(shape) : 0;
}
static void *op_load_shape(void *c, const char *file, int32_t id) { return st_load(c, file, id); }
static void op_free_shape(void *c, void *shape) { free_shape(GS(c), shape); }

/* AOE_GSC_SHAPE (0, 3 + i) and AOE_GSC_OBJECT_PANEL: 0x004f83d0 with font
   record 7, the unit and building pictures and the viewed player, help
   0x19bb8 / 0x265ac, at the layout's place for +0x4c4 (5, 0x169, 0x7f,
   0x72). */
static int make_object_panel(AoeGameScreenHostRor *s) {
    const AoeUiEzOpsRor *ez = s->ui->ez_ops;
    AoeInfoPanelOpsRor *o = &s->object_ops;
    AoeInfoPanelRor *panel;
    uintptr_t font = 0;
    int32_t fw = 0, fh = 0, i;
    char name[32];
    s->unit_icons = st_load(s, "btnunit.shp", 0xc62a);
    for (i = 0; i < 5; ++i) {
        snprintf(name, sizeof name, "btnbldg%d.shp", (int)i);
        s->building_icons[i] = st_load(s, name, i == 4 ? 0xcf0d : 0xc610 + i);
    }
    memset(o, 0, sizeof *o);
    o->context = s;
    o->selected = op_selected;
    o->read = op_read;
    o->attribute = op_attribute;
    o->civilization = op_civ;
    o->nearest = op_nearest;
    o->multiplayer = op_zero;
    o->civ_name = op_civ_name;
    o->string = op_string;
    o->show_ids = op_zero;
    o->draw_shape = op_draw_shape;
    o->shape_frames = op_frames;
    o->load_shape = op_load_shape;
    o->free_shape = op_free_shape;
    if (ez && ez->font) ez->font(ez->context, 7, &font, &fw, &fh);
    panel = calloc(1, sizeof *panel);
    if (!panel) return 0;
    if (!aoe_info_panel_init_ror(panel, s->ez.panel.system, o, s->ez.panel.area, &s->ez.panel, font,
                                 fw, fh, s->unit_icons, s->building_icons,
                                 aoe_demo_world_player(s->world, 1))) {
        panel->panel.vt->destroy(&panel->panel);
        free(panel);
        return 0;
    }
    s->object_panel = panel;
    panel->panel.help_id = 0x19bb8;
    panel->panel.help_page = 0x265ac;
    panel->panel.vt->set_fixed_position(&panel->panel, 5, 0x169, 0x7f, 0x72);
    return 1;
}

static AoeUiEzVtblRor vtable = {
    AOE_UI_EZ_SLOTS_MD(gs_destroy, gs_draw, gs_idle, aoe_ui_panel_handle_paint_ror,
                       gs_key_action, aoe_ui_panel_zero_ror, gs_action,
                       aoe_ui_screen_focus_slot_ror, aoe_ui_ez_mouse_down_slot_ror),
    aoe_ui_ez_draw_background_slot_ror,
    aoe_ui_ez_set_reference_slot_ror,
};

int aoe_game_screen_host_init_ror(AoeGameScreenHostRor *s, AoeGameUiRor *ui, AoeUiAreaRor *area,
                                  AoeDemo *world,
                                  uint8_t *(*resource)(void *context, uint32_t type, int32_t id,
                                                       uint32_t *size),
                                  void *resource_context) {
    AoeUiEzRor *ez = &s->ez;
    int32_t r[4];
    memset(s, 0, sizeof *s);
    s->ui = ui;
    s->world = world;
    s->resource = resource;
    s->resource_context = resource_context;
    s->mouse_x = s->mouse_y = -1;
    vtable.base.mouse_move = gs_mouse_move;
    vtable.base.mouse_down = gs_mouse_down;
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
    s->game_state = 4;
    screen_ops(s);
    if (!make_resources(s) || !make_minimap(s) || !make_object_panel(s)) return 0;
    restyle(s);
    render_view(s);
    return 1;
}
