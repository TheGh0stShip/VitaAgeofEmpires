/* The port's Game Screen (integration support; see game_screen_host.h). */
#include "game_screen_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_place.h"
#include "command_production.h"
#include "main_view.h"
#include "draw_system.h"
#include "game_ror_flow.h"
#include "player_costs.h"
#include "research_manager.h"
#include "tribe_player_commands.h"
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

/* A command button by its screen offset (+0x4c8 + 4 i, i 0..16). */
static AoeCommandButtonRor *button_at(AoeGameScreenHostRor *s, uint32_t off) {
    if (off < 0x4c8 || off > 0x508 || (off - 0x4c8) % 4) return NULL;
    return s->buttons[(off - 0x4c8) / 4];
}

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
static void gs_destroy(AoeUiPanelRor *p) {
    AoeGameScreenHostRor *s = GS(p);
    int i;
    if (s->object_panel) {
        s->object_panel->panel.vt->destroy(&s->object_panel->panel);
        free(s->object_panel);
        s->object_panel = NULL;
    }
    for (i = 0; i < 17; ++i) {
        if (!s->buttons[i]) continue;
        s->buttons[i]->button.panel.vt->destroy(&s->buttons[i]->button.panel);
        free(s->buttons[i]);
        s->buttons[i] = NULL;
    }
    free(s->train_list);
    s->train_list = NULL;
    free(s->research_list);
    s->research_list = NULL;
    free_shape(s, s->unit_icons);
    s->unit_icons = NULL;
    free_shape(s, s->tech_icons);
    s->tech_icons = NULL;
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
static void draw_place_preview(AoeGameScreenHostRor *s);
static void render_view(AoeGameScreenHostRor *s) {
    if (s->world && s->pixels) {
        s->rendered = aoe_demo_render_indexed(s->world, s->pixels, (size_t)s->view_w);
        if (s->rendered) draw_place_preview(s);
        /* The drag box (the view's kind-1 draw node) from the press to the
           pointer, in the selection box's colour. */
        if (s->rendered && s->drag_kind == 1 && s->mouse_x >= 0)
            aoe_demo_draw_box(s->world, s->pixels, (size_t)s->view_w, s->press_x, s->press_y,
                              s->mouse_x - s->ez.panel.x - s->view_x,
                              s->mouse_y - s->ez.panel.y - s->view_y, 0x4a);
    }
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
    /* 0x00519e00: a move with the button held draws the kind-1 box from the
       press point and continues the drag. */
    if (s->pressing && s->world) {
        int32_t vx = x - p->x - s->view_x, vy = y - p->y - s->view_y;
        if (vx != s->press_x || vy != s->press_y) s->drag_kind = 1;
    }
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
    AoeCommandButtonRor *b = button_at(GS(c), off);
    if (b) aoe_ui_button_set_bevel_ror(&b->button, type, k);
}
static void st_button_picture(void *c, uint16_t off, int32_t state, void *pic, int16_t frame) {
    AoeCommandButtonRor *b = button_at(GS(c), off);
    if (b) aoe_ui_button_set_picture_ror(&b->button, (int16_t)state, pic, frame);
}
static int32_t st_button_id(void *c, uint16_t off) {
    AoeCommandButtonRor *b = button_at(GS(c), off);
    return b ? aoe_ui_button_id_ror(&b->button) : 0;
}
static void st_button_colors(void *c, uint16_t off, int32_t state, uint32_t a, uint32_t b) {
    AoeCommandButtonRor *btn = button_at(GS(c), off);
    if (btn) aoe_ui_button_set_text_color_ror(&btn->button, state, a, b);
}
static void st_border(void *c, int32_t i, void *shape) {
    if (i >= 0 && i < 17 && GS(c)->buttons[i]) GS(c)->buttons[i]->background = shape;
}
static void st_colors(void *c, uint16_t off, uint32_t a, uint32_t b) {
    if (off == 0x4c0 && GS(c)->resources) aoe_resource_panel_set_colors_ror(GS(c)->resources, a, b);
}
static void *st_load(void *c, const char *name, int32_t id) {
    const AoeUiEzOpsRor *ez = GS(c)->ui->ez_ops;
    return ez && ez->load_shape ? ez->load_shape(ez->context, name, id) : NULL;
}
static void st_free(void *c, void *shape) { free_shape(GS(c), shape); }
static void st_redraw(void *c) { GS(c)->ez.panel.vt->set_redraw(&GS(c)->ez.panel, 1); }

static void rebuild_buttons(AoeGameScreenHostRor *s);
static void st_rebuild(void *c) { rebuild_buttons(GS(c)); }

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
    o.frame_4b0 = s->frame_4b0;
    o.frame_4b2 = s->frame_4b2;
    o.frame_4b4 = s->frame_4b4;
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
    o.rebuild_buttons = st_rebuild;
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
static void selection_changed(AoeGameScreenHostRor *s);
/* ---- building placement, edit mode 7 (0x15 places adjacent buildings, not yet) ---- */
static void so_set_mode(void *c, int32_t mode, int32_t sub);

typedef struct {
    AoeGameScreenHostRor *s;
    const DatUnit *t;      /* the unit to place (game +0x3fc) */
    float fx, fy;          /* the tile centre under the pointer */
    int ok;                /* placeable there */
    int16_t corners[8];    /* 0x004433c0's offsets about the tile */
} PlaceCtx;

static const DatUnit *place_template(AoeGameScreenHostRor *s) {
    const AoePlayerTemplates *ts = aoe_demo_player_templates(s->world, 1);
    return ts && s->place_unit >= 0 ? aoe_player_templates_get(ts, (uint16_t)s->place_unit) : NULL;
}

/* The preview under the pointer (0x00511080) over the demo's world. */
static int pv_has_template(void *c) { return ((PlaceCtx *)c)->t != NULL; }
static void pv_mouse(void *c, int32_t *x, int32_t *y) {
    *x = ((PlaceCtx *)c)->s->mouse_x;
    *y = ((PlaceCtx *)c)->s->mouse_y;
}
static int pv_pick_terrain(void *c, int32_t vx, int32_t vy) {
    AoeDemo *w = ((PlaceCtx *)c)->s->world;
    float fx, fy;
    return !aoe_demo_pick(w, vx, vy) && aoe_demo_screen_to_map(w, vx, vy, &fx, &fy);
}
static int pv_tile_at(void *c, int32_t vx, int32_t vy, int32_t *tx, int32_t *ty) {
    float fx, fy;
    if (!aoe_demo_screen_to_map(((PlaceCtx *)c)->s->world, vx, vy, &fx, &fy)) return 0;
    *tx = (int32_t)fx;
    *ty = (int32_t)fy;
    return 1;
}
static void pv_corners(void *c, float fx, float fy, int16_t out[8]) {
    PlaceCtx *p = c;
    p->fx = fx;
    p->fy = fy;
    aoe_demo_footprint_corners(p->s->world, p->t, fx, fy, out);
    memcpy(p->corners, out, sizeof p->corners);
}
static int pv_can_place(void *c, float fx, float fy) {
    PlaceCtx *p = c;
    p->ok = aoe_demo_can_place(p->s->world, 1, p->s->place_unit, fx, fy);
    return p->ok;
}
static uint32_t pv_flags(void *c) { return ((PlaceCtx *)c)->t->type; }          /* +0x04 */
static int pv_has_graphic(void *c) { return ((PlaceCtx *)c)->t->standing_graphic >= 0; }
static uint8_t pv_facing(void *c) { return ((PlaceCtx *)c)->t->selection_flags; } /* +0xa8 */
static uint8_t pv_shape(void *c) { return ((PlaceCtx *)c)->t->selection_color; }  /* +0xa9 */
static int pv_relocating(void *c) { (void)c; return 0; }
/* The view's +0x34 preview as the port draws it: the footprint outline
   about the tile, the selection box's healthy colour (0x4a) when the
   building may be placed and its lost colour (0x97) when not. The
   building's picture at the midpoint is not drawn yet. */
static void pv_draw(void *c, int32_t cx, int32_t cy, int relocated, uint32_t facing,
                    uint32_t shape) {
    PlaceCtx *p = c;
    int32_t sx, sy;
    (void)cx; (void)cy; (void)relocated; (void)facing; (void)shape;
    if (!p->s->pixels || !aoe_demo_map_to_screen(p->s->world, p->fx, p->fy, &sx, &sy)) return;
    aoe_demo_draw_footprint(p->s->world, p->s->pixels, (size_t)p->s->view_w, sx, sy, p->corners,
                            p->ok ? 0x4a : 0x97);
}
static void draw_place_preview(AoeGameScreenHostRor *s) {
    PlaceCtx p = {s, place_template(s), 0.0f, 0.0f, 0, {0}};
    AoeMainViewEditorPreviewOps o = {&p,          pv_has_template, pv_mouse,  pv_pick_terrain,
                                     pv_tile_at,  pv_corners,      pv_can_place, pv_flags,
                                     pv_has_graphic, pv_facing,    pv_shape,  pv_relocating,
                                     pv_draw};
    int32_t code = 0, py = 0, left = s->ez.panel.x + s->view_x, top = s->ez.panel.y + s->view_y;
    if (s->edit_mode != 7 || !p.t || s->mouse_x < 0) return;
    aoe_main_view_editor_preview(&o, left, top, left + s->view_w - 1, top + s->view_h - 1, &code,
                                 &py);
}

/* A click placing the building (0x00508d50) over the demo's world. */
static int pc_base_click(void *c, int32_t x, int32_t y, int32_t flag) {
    PlaceCtx *p = c;
    (void)flag;
    aoe_demo_select(p->s->world, aoe_demo_pick(p->s->world, x, y));
    selection_changed(p->s);
    return 1;
}
static uint8_t pc_pick(void *c, int32_t x, int32_t y, float point[2]) {
    AoeDemo *w = ((PlaceCtx *)c)->s->world;
    if (aoe_demo_pick(w, x, y)) return '4';
    return aoe_demo_screen_to_map(w, x, y, &point[0], &point[1]) ? '3' : 0;
}
/* 0x0051b070 keeps the point on the map: the pick yields on-map points. */
static void pc_snap(void *c, float point[2]) { (void)c; (void)point; }
/* The template's +0x24: the tile centre (0x00508bb0). */
static void pc_adjust(void *c, float point[2]) {
    (void)c;
    point[0] = (float)(int32_t)point[0] + 0.5f;
    point[1] = (float)(int32_t)point[1] + 0.5f;
}
static int pc_blocked(void *c, float x, float y) {
    PlaceCtx *p = c;
    return !aoe_demo_can_place(p->s->world, 1, p->s->place_unit, x, y);
}
/* The game's +0x10(0, 0); the buttons back to the first page as a new
   selection would (0x00482290). */
static void pc_game_10(void *c) {
    so_set_mode(((PlaceCtx *)c)->s, 0, 0);
    selection_changed(((PlaceCtx *)c)->s);
}
/* 0x004f10e0: the selected villagers (class 4) build `id` there: opcode
   0x66 for player 1, run by the world. */
static int pc_build(void *c, int16_t id, float x, float y) {
    PlaceCtx *p = c;
    AoeLiveUnit *sel[AOE_DEMO_SELECTION];
    int32_t ids[AOE_DEMO_SELECTION];
    uint8_t cmd[AOE_COMMAND_PLACE_HEADER_ROR + 4 * AOE_DEMO_SELECTION];
    int n = aoe_demo_selection(p->s->world, sel, AOE_DEMO_SELECTION), i, count = 0;
    size_t size;
    for (i = 0; i < n; ++i) {
        const DatUnit *t = sel[i]->core.unit_template;
        if (t && t->unit_class == 4 && sel[i]->runtime_owner_index == 1)
            ids[count++] = aoe_demo_unit_id(sel[i]);
    }
    if (!count) return 0;
    size = aoe_command_encode_place_ror(cmd, sizeof cmd, 1, x, y, id, 0, ids, (uint8_t)count);
    return size && aoe_demo_command_place(p->s->world, cmd, size) == 1;
}
static void place_click(AoeGameScreenHostRor *s, int32_t vx, int32_t vy) {
    PlaceCtx p = {s, place_template(s), 0.0f, 0.0f, 0, {0}};
    AoeMainViewPlaceOpsRor o = {&p,       1,        s->edit_mode, s->place_unit, p.t != NULL,
                                pc_base_click, pc_pick, pc_snap,  pc_adjust,     pc_blocked,
                                pc_game_10, pc_build};
    aoe_main_view_place_click_ror(&o, vx, vy, 1);
}

static int32_t gs_mouse_down(AoeUiPanelRor *p, uint8_t button, int32_t x, int32_t y, int32_t c,
                             int32_t sh) {
    AoeGameScreenHostRor *s = GS(p);
    int32_t vx = x - p->x - s->view_x, vy = y - p->y - s->view_y;
    if (!s->world || vx < 0 || vy < 0 || vx >= s->view_w || vy >= s->view_h ||
        (button != 1 && button != 2) || aoe_ui_ez_help_press_ror(&s->ez, button, x, y))
        return aoe_ui_ez_mouse_down_slot_ror(p, button, x, y, c, sh);
    if (s->edit_mode == 7 && s->place_unit >= 0) {
        /* 0x00508d50 places on a left click; a right click resets the mode
           (0x00519ed0: any sub-mode but 0 and 0x14 resets it). */
        if (button == 1) {
            place_click(s, vx, vy);
        } else {
            so_set_mode(s, 0, 0);
            selection_changed(s);
        }
        render_view(s);
        p->vt->set_redraw(p, 1);
        return 1;
    }
    if (button == 1) {
        /* 0x00519d50: the press is captured; the selection follows on the
           release (0x00519ed0 / 0x00515d30), or a drag box (0x00519e00). */
        s->press_x = vx;
        s->press_y = vy;
        s->pressing = 1;
        s->drag_kind = 0;
    } else {
        aoe_demo_order(s->world, vx, vy);
    }
    render_view(s);
    p->vt->set_redraw(p, 1);
    return 1;
}

/* The left release (0x00519ed0 edit mode 0): a dragged box selects the
   area (0x00515d30 kind 2 -> 0x005114d0), a plain release the point;
   Shift adds to the selection. */
static int32_t gs_mouse_up(AoeUiPanelRor *p, uint8_t button, int32_t x, int32_t y, int32_t c,
                           int32_t sh) {
    AoeGameScreenHostRor *s = GS(p);
    int32_t vx = x - p->x - s->view_x, vy = y - p->y - s->view_y, x0, y0, x1, y1;
    if (button != 1 || !s->pressing || !s->world)
        return aoe_ui_panel_mouse_up_ror(p, button, x, y, c, sh);
    s->pressing = 0;
    if (s->drag_kind == 1) {
        x0 = s->press_x < vx ? s->press_x : vx;
        x1 = s->press_x < vx ? vx : s->press_x;
        y0 = s->press_y < vy ? s->press_y : vy;
        y1 = s->press_y < vy ? vy : s->press_y;
    } else {
        x0 = x1 = vx;
        y0 = y1 = vy;
    }
    s->drag_kind = 0;
    aoe_demo_select_area(s->world, x0, y0, x1, y1, key_held(p, 0x10));
    selection_changed(s);
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
    if (sub) *sub = SO(c)->edit_sub;
    return SO(c)->edit_mode;
}
static void so_view_cancel(void *c, void *view) { (void)c; (void)view; }
/* The game's +0x10 mode change (+0x1b4, +0x1b8). */
static void so_set_mode(void *c, int32_t mode, int32_t sub) {
    SO(c)->edit_mode = mode;
    SO(c)->edit_sub = sub;
}
static void so_clear_selection(void *c) {
    aoe_demo_select(SO(c)->world, NULL);
    selection_changed(SO(c));
}
static void rebuild_buttons(AoeGameScreenHostRor *s);
static void so_refresh_buttons(void *c) { rebuild_buttons(SO(c)); }
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

/* ---- the command buttons ---- */

/* A shape frame clipped to `clip` (0x004f75b0's draws); `remap` >= 0 as
   the object panel's; disabled pictures are drawn as they are (the grey
   table 0x00540680 is not loaded). */
static void cb_draw_shape(void *c, AoeUiAreaRor *area, void *shape, int32_t frame, int32_t x,
                          int32_t y, const AoeUiRectRor *clip, int32_t remap, int disabled) {
    AoeGameScreenHostRor *s = c;
    const AoeUiEzOpsRor *ez = s->ui->ez_ops;
    AoeDrawSurfaceRor *d = area ? area->surface : NULL;
    AoeShapeSurfaceRor saved;
    uint8_t table[256];
    int i;
    (void)disabled;
    if (!shape || !d || !ez || !ez->shape_ops || !aoe_surface_lock_ror(d, 1)) return;
    saved = d->view;
    if (clip) {
        if (clip->left > d->view.clip_left) d->view.clip_left = clip->left;
        if (clip->top > d->view.clip_top) d->view.clip_top = clip->top;
        if (clip->right < d->view.clip_right) d->view.clip_right = clip->right;
        if (clip->bottom < d->view.clip_bottom) d->view.clip_bottom = clip->bottom;
    }
    for (i = 0; i < 256; ++i) table[i] = (uint8_t)i;
    if (remap >= 0)
        for (i = 0; i < 8; ++i) table[0x10 + i] = (uint8_t)(remap + i);
    aoe_shape_draw_ror(shape, &d->view, x, y, frame, remap >= 0 ? 1 : 0, remap >= 0 ? table : NULL,
                       ez->shape_ops);
    d->view.clip_left = saved.clip_left;
    d->view.clip_top = saved.clip_top;
    d->view.clip_right = saved.clip_right;
    d->view.clip_bottom = saved.clip_bottom;
    aoe_surface_unlock_ror(d);
}
/* The viewed player's colour (its colour record +0x26): player 1's
   colour 0. */
static int32_t cb_player_color(void *c) {
    (void)c;
    return 0;
}

/* AOE_GSC_BUTTON / _FIXED / _TEXT: seventeen 0x004f7420 buttons (font
   record 11, border +0x47c for 0..11 and +0x484 for the rest, text
   colours (0, 0, 0xffffff), sound 1); 12..16 take the states 0xd, 0xe,
   0xf, 0x15 and 0x18 with help 0x1d4cf, 0x1d4d1, 0x1d4cd, 0x1d4db and
   0x1d4dc; 12..14 draw type 3, style 3, their state's name (kind 0x66)
   and bevel 3; 15 and 16 draw type 3, bevel 4, style 3 and the texts
   0xfa9 and 0x1023 (16's help line string 0x1024). */
static int make_buttons(AoeGameScreenHostRor *s) {
    static const int32_t ids[5] = {0xd, 0xe, 0xf, 0x15, 0x18};
    static const int32_t helps[5] = {0x1d4cf, 0x1d4d1, 0x1d4cd, 0x1d4db, 0x1d4dc};
    const AoeUiEzOpsRor *ez = s->ui->ez_ops;
    uintptr_t font = 0;
    int32_t fw = 0, fh = 0, i;
    char text[0x200];
    GameRorFlowStringOps fmt = {s, op_load_string, op_base_format};
    s->button_ops.context = s;
    s->button_ops.draw_shape = cb_draw_shape;
    s->button_ops.player_color = cb_player_color;
    if (ez && ez->font) ez->font(ez->context, 11, &font, &fw, &fh);
    for (i = 0; i < 17; ++i) {
        AoeCommandButtonRor *b = calloc(1, sizeof *b);
        if (!b) return 0;
        if (!aoe_command_button_init_ror(b, s->ez.panel.system, ez ? ez->button_ops : NULL,
                                         &s->button_ops, s->ez.panel.area, &s->ez.panel, NULL,
                                         font, fw, fh,
                                         i < 12 ? s->shapes[AOE_GSP_BORDER_A]
                                                : s->shapes[AOE_GSP_BORDER_B])) {
            b->button.panel.vt->destroy(&b->button.panel);
            free(b);
            return 0;
        }
        s->buttons[i] = b;
        aoe_ui_button_set_text_color_ror(&b->button, 0, 0, 0xffffff);
        aoe_ui_button_set_sound_id_ror(&b->button, 1);
        if (i < AOE_GAME_SCREEN_BUTTONS) s->screen.buttons[i] = b;
        else s->screen.more_buttons[i - AOE_GAME_SCREEN_BUTTONS] = b;
    }
    for (i = 0; i < 5; ++i) {
        aoe_ui_button_set_id_ror(&s->buttons[12 + i]->button, 0, ids[i], 0);
        s->buttons[12 + i]->button.panel.help_id = helps[i];
        s->buttons[12 + i]->button.panel.help_page = -1;
    }
    for (i = 12; i < 15; ++i) {
        AoeUiButtonRor *b = &s->buttons[i]->button;
        aoe_ui_button_set_draw_type_ror(b, 3);
        s->buttons[i]->style = 3;
        game_ror_flow_format_message(&fmt, 0x66, aoe_ui_button_id_ror(b), 0, text, sizeof text);
        aoe_ui_button_set_text_ror(b, 0, text);
        aoe_ui_button_set_bevel_ror(b, 3, s->ez.bevel);
    }
    for (i = 15; i < 17; ++i) {
        AoeUiButtonRor *b = &s->buttons[i]->button;
        aoe_ui_button_set_draw_type_ror(b, 3);
        aoe_ui_button_set_bevel_ror(b, 4, s->ez.bevel);
        s->buttons[i]->style = 3;
        aoe_ui_button_set_text_id_ror(b, 0, i == 15 ? 0xfa9 : 0x1023);
    }
    aoe_ui_string_ror(s->ez.panel.system, 0x1024, text, sizeof text);
    aoe_command_button_set_help_ror(s->buttons[16], text);
    return 1;
}

/* The translated screen methods' calls on a button. */
#define CB(b) ((AoeCommandButtonRor *)(b))
static void so_set_active(void *c, void *panel, int on) {
    (void)c;
    if (panel) ((AoeUiPanelRor *)panel)->vt->set_active(panel, on);
}
static void so_help(void *c, void *panel, int32_t id1, int32_t id2) {
    (void)c;
    if (!panel) return;
    ((AoeUiPanelRor *)panel)->help_id = id1;
    ((AoeUiPanelRor *)panel)->help_page = id2;
}
static int32_t so_button_disabled(void *c, void *b) { (void)c; return CB(b)->button.disabled; }
static void so_button_set_disabled(void *c, void *b, int32_t d) {
    (void)c;
    aoe_ui_button_set_disabled_ror(&CB(b)->button, d);
}
static int32_t so_button_id(void *c, void *b) { (void)c; return aoe_ui_button_id_ror(&CB(b)->button); }
static int32_t so_button_value(void *c, void *b) {
    (void)c;
    return aoe_ui_button_value_ror(&CB(b)->button);
}
static void so_button_drop_press(void *c, void *b) { (void)c; aoe_ui_button_cancel_ror(&CB(b)->button); }
static void so_button_reset(void *c, void *b) { (void)c; aoe_command_button_show_count_ror(CB(b), 0); }
static void so_button_picture(void *c, void *b, void *picture, int32_t frame) {
    (void)c;
    aoe_ui_button_set_picture_ror(&CB(b)->button, 0, picture, (int16_t)frame);
}
static void so_button_state(void *c, void *b, int32_t id, int32_t value) {
    (void)c;
    aoe_ui_button_set_id_ror(&CB(b)->button, 0, id, value);
}
static void so_button_lines(void *c, void *b, const char *l1, const char *l2) {
    (void)c;
    aoe_ui_button_set_texts_ror(&CB(b)->button, 0, l1, l2);
}
static void so_button_help_line(void *c, void *b, const char *text) {
    (void)c;
    aoe_command_button_set_help_ror(CB(b), text);
}
static int32_t so_button_used(void *c, void *b) { (void)c; return CB(b)->field_3d0; }
static void so_button_set_used(void *c, void *b, int32_t used) { (void)c; CB(b)->field_3d0 = used; }
static void so_button_fields(void *c, void *b, int32_t f2c8, int32_t f2cc, int32_t hotkey) {
    (void)c;
    CB(b)->player_colors = f2c8;
    CB(b)->background_frame = f2cc;
    CB(b)->button.hotkey = hotkey;
}
/* The game's +0x28 formatter (0x004ff580). */
static void so_format(void *c, int32_t kind, int32_t a, int32_t b, char *out, int32_t n) {
    GameRorFlowStringOps fmt = {c, op_load_string, op_base_format};
    game_ror_flow_format_message(&fmt, kind, a, b, out, n);
}
static const char *so_string(void *c, int32_t id) {
    static char text[0x200];
    aoe_ui_string_ror(SO(c)->ez.panel.system, id, text, sizeof text);
    return text;
}
/* The message lines are not made yet: a message is dropped. */
static void so_message(void *c, void *panel, int32_t style, const char *text, uint8_t color) {
    (void)c; (void)panel; (void)style; (void)text; (void)color;
}

/* The current unit (the viewed player's +0x134): the first selected. */
static AoeLiveUnit *current_unit(AoeGameScreenHostRor *s) {
    AoeLiveUnit *u = NULL;
    return aoe_demo_selection(s->world, &u, 1) ? u : NULL;
}

/* Training and research at the current building: the cost check
   0x004f15f0 (multiplier 1.0, population flag 0) and the tribe commands
   0x64 (0x004f1410 -> 0x004e9490) and 0x65 (0x004f1200 -> 0x004e9530)
   with request -1. Research affordability is the tech's costs against
   the player's resources (0x004f1850 is not translated). */
static int so_instant(void *c) { (void)c; return 0; }
static void so_instant_train(void *c, int16_t id, int32_t arg) { (void)c; (void)id; (void)arg; }
static int so_can_afford(void *c, int16_t id, int16_t *failing) {
    AoeGameScreenHostRor *s = SO(c);
    const AoePlayerTemplates *t = aoe_demo_player_templates(s->world, 1);
    AoeResearchOwnerRor owner;
    int ok = 0;
    if (!t || !aoe_demo_research_owner(s->world, 1, &owner)) return 0;
    if (!aoe_player_cost_check_ror(owner.resources, aoe_player_templates_get(t, (uint16_t)id), 1.0f,
                                   0, failing, &ok))
        return 0;
    return ok;
}
static void so_place_unit(void *c, int16_t id) { SO(c)->place_unit = id; } /* game +0x3fc */
/* The game's +0x10 (0x004ff420: the state and substate, 0x00419970) as
   the screen's "action": 0x00484820 asks 7 to place a building, or 0x15
   when the template's +0x16c byte (adjacent mode, walls) is set. */
static void so_action(void *c, int32_t action, int32_t arg) { so_set_mode(c, action, arg); }
static int so_template_flag(void *c, int16_t id) {
    const AoePlayerTemplates *ts = aoe_demo_player_templates(SO(c)->world, 1);
    const DatUnit *t = ts && id >= 0 ? aoe_player_templates_get(ts, (uint16_t)id) : NULL;
    return t && t->building_adjacent_mode != 0;
}
static void production(AoeGameScreenHostRor *s, uint8_t opcode, int16_t item) {
    uint8_t cmd[AOE_COMMAND_PRODUCTION_SIZE_ROR];
    AoeLiveUnit *b = current_unit(s);
    size_t n;
    if (!b) return;
    n = aoe_command_encode_production_ror(cmd, sizeof cmd, opcode, aoe_demo_unit_id(b), 1, item, -1);
    if (n) aoe_demo_command_production(s->world, cmd, n);
}
static void so_train_at_current(void *c, int16_t id) { production(SO(c), AOE_COMMAND_TRAIN_ROR, id); }
static void so_research_at_current(void *c, int16_t id) {
    production(SO(c), AOE_COMMAND_RESEARCH_ROR, id);
}
static int so_can_research(void *c, int16_t id, int16_t *failing) {
    const DatTechs *techs = aoe_demo_techs(SO(c)->world);
    AoeResearchOwnerRor owner;
    int k;
    *failing = -1;
    if (!techs || id < 0 || id >= techs->count || !aoe_demo_research_owner(SO(c)->world, 1, &owner))
        return 0;
    for (k = 0; k < 3; ++k) {
        const DatTechCost *cost = &techs->techs[id].costs[k];
        if (cost->resource_type < 0 || !cost->paid) continue;
        if (cost->resource_type >= owner.resources->count ||
            owner.resources->values[cost->resource_type] < (float)cost->amount) {
            *failing = cost->resource_type;
            return 0;
        }
    }
    return 1;
}

static void screen_button_ops(AoeGameScreenOpsRor *o) {
    o->set_active = so_set_active;
    o->help = so_help;
    o->button_disabled = so_button_disabled;
    o->button_set_disabled = so_button_set_disabled;
    o->button_id = so_button_id;
    o->button_value = so_button_value;
    o->button_drop_press = so_button_drop_press;
    o->button_reset = so_button_reset;
    o->button_picture = so_button_picture;
    o->button_state = so_button_state;
    o->button_lines = so_button_lines;
    o->button_help_line = so_button_help_line;
    o->button_used = so_button_used;
    o->button_set_used = so_button_set_used;
    o->button_fields = so_button_fields;
    o->format = so_format;
    o->string = so_string;
    o->message = so_message;
    o->instant = so_instant;
    o->instant_train = so_instant_train;
    o->can_afford = so_can_afford;
    o->set_place_unit = so_place_unit;
    o->action = so_action;
    o->template_flag = so_template_flag;
    o->train_at_current = so_train_at_current;
    o->research_at_current = so_research_at_current;
    o->can_research = so_can_research;
}

/* ---- the layout (0x0047f000) ---- */

/* The panel at a screen offset, NULL when it is not made. */
static AoeUiPanelRor *panel_at(AoeGameScreenHostRor *s, uint16_t off) {
    AoeCommandButtonRor *b = button_at(s, off);
    if (b) return &b->button.panel;
    switch (off) {
    case 0x4bc: return s->map ? &s->map->map.panel : NULL;
    case 0x4c0: return s->resources ? &s->resources->panel : NULL;
    case 0x4c4: return s->object_panel ? &s->object_panel->panel : NULL;
    default: return NULL;
    }
}
static int32_t lo_score(void *c, int i) { (void)c; (void)i; return -1; }
static void lo_none(void *c) { (void)c; }
static void lo_active(void *c, uint16_t off, int32_t v) {
    AoeUiPanelRor *p = panel_at(GS(c), off);
    if (p) p->vt->set_active(p, v);
}
static void lo_mode(void *c, uint16_t off, const AoeGameScreenModeRor *m) { (void)c; (void)off; (void)m; }
static void lo_rect(void *c, uint16_t off, int32_t x, int32_t y, int32_t w, int32_t h) {
    AoeGameScreenHostRor *s = GS(c);
    AoeUiPanelRor *p;
    if (off == 0x4b8) {   /* the view */
        s->view_x = x;
        s->view_y = y;
        s->view_w = w;
        s->view_h = h;
        return;
    }
    p = panel_at(s, off);
    if (p) p->vt->set_fixed_position(p, x, y, w, h);
}
static void lo_style(void *c, uint16_t off, uint8_t style) {
    AoeCommandButtonRor *b = button_at(GS(c), off);
    if (b) b->style = style;
}
/* +0x4ac = +0x480 (small) or +0x48c, +0x4b0 = 0 / 0xb, +0x4b2 = +0x4b4 =
   pages. */
static void lo_pages(void *c, int small, int16_t base, int16_t pages) {
    AoeGameScreenHostRor *s = GS(c);
    s->page_picture = small ? s->shapes[AOE_GSP_OTHER] : s->shapes[AOE_GSP_COMMANDS];
    s->frame_4b0 = base;
    s->frame_4b2 = s->frame_4b4 = pages;
}
static void restyle(AoeGameScreenHostRor *s);
static void lo_notify(void *c, int16_t player) {
    (void)player;
    restyle(GS(c));
}

static void layout(AoeGameScreenHostRor *s) {
    AoeGameScreenLayoutOpsRor o;
    memset(&o, 0, sizeof o);
    o.context = s;
    o.has_view = 1;
    o.width = s->ez.panel.width;
    o.height = s->ez.panel.height;
    o.screen_width = s->ez.panel.width;
    o.viewed_player = 1;
    o.score_active = lo_score;
    o.base_resize = lo_none;
    o.set_active = lo_active;
    o.position_mode = lo_mode;
    o.set_rect = lo_rect;
    o.button_style = lo_style;
    o.button_pages = lo_pages;
    o.notify = lo_notify;
    aoe_game_screen_layout_ror(&o);
}

/* ---- the buttons rebuilt (0x004822a0) ---- */

typedef struct {
    AoeGameScreenHostRor *s;
    const AoePlayerTemplates *templates;
    AoeResearchOwnerRor owner;
} ButtonsWorld;

static int tl_info(void *c, int16_t slot, uint8_t *kind, uint8_t *enabled, int16_t *trained_at,
                   int16_t *field_4c, const char **name) {
    const DatUnit *t = aoe_player_templates_get(((ButtonsWorld *)c)->templates, (uint16_t)slot);
    if (!t || !t->present) return 0;
    *kind = t->type;
    *enabled = t->enabled;
    *trained_at = t->train_location_id;
    *field_4c = t->icon;
    *name = t->name;
    return 1;
}
static void tl_costs(void *c, int16_t slot, int16_t types[3], int16_t amounts[3]) {
    ButtonsWorld *w = c;
    if (!aoe_player_cost_shown_ror(w->owner.resources,
                                   aoe_player_templates_get(w->templates, (uint16_t)slot), 1.0f,
                                   types, amounts))
        types[0] = types[1] = types[2] = -1;
}
static int32_t tl_help(void *c, int16_t slot) {
    const DatUnit *t = aoe_player_templates_get(((ButtonsWorld *)c)->templates, (uint16_t)slot);
    return t ? t->language_help_id : 0;
}
static int32_t tl_tree(void *c, int16_t slot) {
    const DatUnit *t = aoe_player_templates_get(((ButtonsWorld *)c)->templates, (uint16_t)slot);
    return t ? t->language_tree_id : 0;
}
static float tl_attribute_48(void *c) {
    const AoePlayerResources *r = ((ButtonsWorld *)c)->owner.resources;
    return r && r->count > 48 ? r->values[48] : 0.0f;
}
static int tl_owns(void *c, int16_t id) {
    return aoe_demo_owns_template(((ButtonsWorld *)c)->s->world, 1, id);
}

static int32_t bt_train_list(void *c, int which, int16_t building) {
    ButtonsWorld *w = c;
    AoeTribeTrainListOpsRor o = {w, (int16_t)w->templates->count, tl_info, tl_costs, tl_help,
                                 tl_tree, tl_attribute_48, tl_owns};
    AoeTribeTrainEntryRor *list = w->s->train_list;
    (void)which;
    aoe_tribe_player_train_list_ror(&o, building, &list, &w->s->train_count);
    w->s->train_list = list;
    return w->s->train_count;
}
static void help_string(void *c, int32_t id, char *out, int size) {
    aoe_ui_string_ror(((ButtonsWorld *)c)->s->ez.panel.system, id, out, size);
}
static void resource_name(void *c, int32_t type, char *out, int size) {
    GameRorFlowStringOps fmt = {((ButtonsWorld *)c)->s, op_load_string, op_base_format};
    game_ror_flow_format_message(&fmt, 100, type, 0, out, size);
}
/* A train entry as a unit button: the name (+0x0c), the help text
   (0x004834f0 with the creation text +0x0e), slot +0x15e, picture +0x4c,
   help IDs +0x0e + 100000 and the entry's tree text, hotkey +0xa0 and the
   player's colours. */
static void bt_train_entry(void *c, int which, int32_t i, AoeButtonEntryRor *e) {
    ButtonsWorld *w = c;
    const AoeTribeTrainEntryRor *t = (const AoeTribeTrainEntryRor *)w->s->train_list + i;
    const DatUnit *u = aoe_player_templates_get(w->templates, (uint16_t)t->id);
    AoeGameScreenHelpTextOpsRor ho = {w, help_string, resource_name};
    AoeGameScreenCostRor costs[3];
    int k;
    (void)which;
    memset(e, 0, sizeof *e);
    for (k = 0; k < 3; ++k) {
        costs[k].type = t->cost_type[k];
        costs[k].amount = t->cost_amount[k];
    }
    if (u) {
        help_string(w, u->language_name_id, e->name, sizeof e->name);
        aoe_game_screen_help_text_ror(&ho, u->language_creation_id, costs, 1, e->help_text,
                                      sizeof e->help_text);
    }
    e->slot = u ? (uint8_t)u->button_id : 0;
    e->frame = t->field_4c;
    e->id = t->id;
    e->help = u ? u->language_creation_id + 100000 : -1;
    e->help2 = t->tree_text;
    e->a = u ? u->hotkey_id : 0;
    e->b = 1;
}
static int32_t bt_research_list(void *c, int16_t building) {
    ButtonsWorld *w = c;
    AoeResearchListEntryRor *list = w->s->research_list;
    aoe_research_list_for_building_ror(&w->owner, building, &list, &w->s->research_count);
    w->s->research_list = list;
    return w->s->research_count;
}
/* A research entry as a tech button: the tech's name and help text
   (0x004eb6f3 / 0x004eb730), slot (0x004eb76d), picture (the entry's
   +0x06), help and tree text, hotkey (0x004eba87). */
static void bt_research_entry(void *c, int32_t i, AoeButtonEntryRor *e) {
    ButtonsWorld *w = c;
    const AoeResearchListEntryRor *r = (const AoeResearchListEntryRor *)w->s->research_list + i;
    const DatTechs *techs = w->owner.techs;
    const DatTech *t = r->id >= 0 && r->id < techs->count ? &techs->techs[r->id] : NULL;
    AoeGameScreenHelpTextOpsRor ho = {w, help_string, resource_name};
    AoeGameScreenCostRor costs[3];
    int k;
    memset(e, 0, sizeof *e);
    for (k = 0; k < 3; ++k) {
        costs[k].type = k < 2 ? r->cost_type[k] : -1;
        costs[k].amount = r->cost_amount[k];
    }
    if (t) {
        help_string(w, t->language_name, e->name, sizeof e->name);
        aoe_game_screen_help_text_ror(&ho, t->language_description, costs, 1, e->help_text,
                                      sizeof e->help_text);
    }
    e->slot = t ? (uint8_t)t->button_id : 0;
    e->frame = r->field_24;
    e->id = r->id;
    e->help = r->help;
    e->help2 = r->tree_text;
    e->a = aoe_tech_hotkey_ror(techs, r->id);
    e->b = 1;
}
static int bt_age_research(void *c, int32_t tech, AoeButtonEntryRor *e) {
    (void)c; (void)tech; (void)e;
    return 0;
}
static int32_t bt_queued_of(void *c, int16_t id) { (void)c; (void)id; return 0; }
/* 0x00483760 with the picture set: 0 +0x48c, 1 +0x490, 2 +0x494, 3 the
   civilisation's building set, 4 +0x4ac. */
static void bt_set(void *c, int set, int32_t index, int32_t frame, int32_t command, int32_t value,
                   int32_t help, int32_t help2, int32_t a, int32_t b, int32_t disabled,
                   const char *name, const char *help_text) {
    AoeGameScreenHostRor *s = ((ButtonsWorld *)c)->s;
    AoeGameScreenButtonRor btn;
    uint8_t style = 0;
    void *pictures[5];
    aoe_demo_player_icon_set(s->world, 1, &style);
    pictures[0] = s->shapes[AOE_GSP_COMMANDS];
    pictures[1] = s->tech_icons;
    pictures[2] = s->unit_icons;
    pictures[3] = s->building_icons[style < 5 ? style : 0];
    pictures[4] = s->page_picture;
    if (index < 0 || index >= AOE_GAME_SCREEN_BUTTONS) return;
    memset(&btn, 0, sizeof btn);
    btn.picture = set >= 0 && set < 5 ? pictures[set] : NULL;
    btn.index = (int16_t)index;
    btn.frame = frame;
    btn.command = command;
    btn.value = value;
    btn.help1 = help;
    btn.help2 = help2;
    btn.hotkey_id = a;
    btn.field_2c8 = b;
    btn.line2 = name;
    btn.help = help_text && help_text[0] ? help_text : NULL;
    btn.disabled = disabled;
    aoe_game_screen_set_button_ror(&s->screen, &btn);
}
static void bt_queue_flag(void *c, int on, int32_t n) { (void)c; (void)on; (void)n; }
static void bt_hide_unused(void *c) { aoe_game_screen_hide_unused_ror(&((ButtonsWorld *)c)->s->screen); }

static void rebuild_buttons(AoeGameScreenHostRor *s) {
    ButtonsWorld w;
    AoeGameScreenButtonsOpsRor o;
    AoeLiveUnit *sel[AOE_DEMO_SELECTION];
    AoeInfoObjectRor info;
    int n, i, all_3_9 = 1;
    uint8_t style = 0;
    int16_t count;
    const float *v = NULL;
    if (!s->buttons[0] || !s->world) return;
    memset(&w, 0, sizeof w);
    w.s = s;
    w.templates = aoe_demo_player_templates(s->world, 1);
    if (!w.templates || !aoe_demo_research_owner(s->world, 1, &w.owner)) return;
    n = aoe_demo_selection(s->world, sel, AOE_DEMO_SELECTION);
    memset(&info, 0, sizeof info);
    if (n) aoe_demo_info_object(s->world, sel[0], &info);
    for (i = 0; i < n; ++i) {
        AoeInfoObjectRor o2;
        if (!aoe_demo_info_object(s->world, sel[i], &o2) || o2.panel_kind < 3 || o2.panel_kind > 9)
            all_3_9 = 0;
    }
    aoe_demo_player_icon_set(s->world, 1, &style);
    for (i = 0; i < 17; ++i) s->buttons[i]->field_3d0 = 0;
    memset(&o, 0, sizeof o);
    o.context = &w;
    o.has_current_unit = n > 0;
    o.game_state = s->game_state;
    o.civ = style < 6 ? style : 0;
    o.count = &s->screen.button_pages;
    o.shown = &s->screen.shown_page;
    o.page = s->screen.button_page;
    o.edit_mode = s->edit_mode;
    o.field_1b8 = s->edit_sub;
    o.frame_4b0 = s->frame_4b0;
    o.frame_4b2 = s->frame_4b2;
    o.frame_4b4 = s->frame_4b4;
    o.current_is_ours = n > 0 && info.owner_number == 1;
    o.common_8f = n > 0 ? info.panel_kind : 0;
    o.selection_count = (int16_t)n;
    o.all_8f_3_to_9 = all_3_9;
    o.is_building = n > 0 && info.type == 'P';
    o.current_state = info.object_state;
    o.current_template = info.object_class;
    o.age = aoe_demo_player_attributes(s->world, 1, &count, &v) && count > 6 ? (int32_t)v[6] : 0;
    o.age_research = bt_age_research;
    o.train_list = bt_train_list;
    o.train_entry = bt_train_entry;
    o.research_list = bt_research_list;
    o.research_entry = bt_research_entry;
    o.queued_of = bt_queued_of;
    o.set = bt_set;
    o.queue_flag = bt_queue_flag;
    o.hide_unused = bt_hide_unused;
    aoe_game_screen_buttons_ror(&o);
    s->ez.panel.vt->set_redraw(&s->ez.panel, 1);
}

/* A new current object: the first button page rebuilt (0x00482290, as
   the player change 0x00481f70 ends). */
static void selection_changed(AoeGameScreenHostRor *s) {
    s->screen.button_page = 0;
    rebuild_buttons(s);
}

/* 0x00481410's handlers that are integrated: train (0x485520), a building
   to place (0x484820), research (0x4852f0), the build menu (0x4847e0), the
   next page (0x485140), the menu (0x4850b0) and Escape (0x484900). */
static void command_run(void *c, uint32_t handler, int32_t arg, int32_t arg2) {
    AoeGameScreenHostRor *s = c;
    switch (handler) {
    case 0x485520u: aoe_game_screen_train_button_ror(&s->screen, (int16_t)arg, arg2); break;
    case 0x484820u: aoe_game_screen_train_ror(&s->screen, (int16_t)arg); break; /* a building to place */
    case 0x4852f0u: aoe_game_screen_research_button_ror(&s->screen, (int16_t)arg); break;
    case 0x4847e0u: aoe_game_screen_build_menu_ror(&s->screen); break;
    case 0x485140u: aoe_game_screen_next_page_ror(&s->screen); break;
    case 0x4850b0u: aoe_game_screen_menu_ror(&s->screen); break;
    case 0x484900u: aoe_game_screen_escape_ror(&s->screen); break;
    default: break;
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
    o->refresh_buttons = so_refresh_buttons; /* the page requested by first/next page */
    o->game_state = so_state;
    o->run_commands = so_none;
    o->paused = so_none_int;
    o->comm_pause = so_comm_pause;
    o->open_dialog = so_open_dialog;
    screen_button_ops(o);
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
    case AOE_GS_ACT_COMMAND:
        aoe_game_screen_command_ror(1, a, b, arg, 0, command_run, s);
        break;
    case AOE_GS_ACT_PLAYER_CHANGED:   /* 0x00481f70 -> the first page, rebuilt */
        s->screen.button_page = 0;
        rebuild_buttons(s);
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
    for (int i = 0; i < 17; ++i) o.buttons[i] = s->buttons[i];
    o.panel_4c4 = s->object_panel;
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

/* AOE_GSC_SHAPE (0 units, 1 techs, 2 commands, 3 + i buildings) and AOE_GSC_OBJECT_PANEL: 0x004f83d0 with font
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
    s->tech_icons = st_load(s, "btntech.shp", 0xc629);
    s->shapes[AOE_GSP_COMMANDS] = st_load(s, "btncmd.shp", 0xc621);
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
    s->place_unit = -1;
    vtable.base.mouse_move = gs_mouse_move;
    vtable.base.mouse_down = gs_mouse_down;
    vtable.base.mouse_up = gs_mouse_up;
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
    if (!make_resources(s) || !make_minimap(s) || !make_object_panel(s) || !make_buttons(s))
        return 0;
    layout(s);   /* places the panels and restyles (0x004816d0) */
    rebuild_buttons(s);
    render_view(s);
    return 1;
}
