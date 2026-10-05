#include "game_minimap.h"

#define GM(p) ((AoeGameMinimapRor *)(p))
#define GVT(g) ((const AoeGameMinimapVtblRor *)(g)->map.panel.vt)

static int running(const AoeGameMinimapRor *g, int with_5) {
    int32_t s = g->game_ops->state(g->game_ops->context);
    return s == 4 || s == 6 || s == 7 || (with_5 && s == 5);
}

static int waiting(const AoeGameMinimapRor *g) {
    return g->game_ops->comm_value(g->game_ops->context) != 0;
}

static int style2(const AoeGameMinimapRor *g) {
    return g->game_ops->mouse_style(g->game_ops->context) == 2;
}

void aoe_game_minimap_init_ror(AoeGameMinimapRor *g, AoeUiSystemRor *system,
                               const AoeDiamondMapOpsRor *ops,
                               const AoeGameMinimapOpsRor *game_ops) {
    aoe_diamond_map_init_ror(&g->map, system, ops);
    g->map.panel.vt = &aoe_game_minimap_vtable_ror()->base.base;
    g->map.panel.drag_slack = 0;
    g->game_ops = game_ops;
    g->flash_ms = 0;
    g->flash = 0;
}

static void set_redraw(AoeUiPanelRor *p, int32_t redraw) {
    AoeGameMinimapRor *g = GM(p);
    aoe_diamond_map_set_redraw_ror(p, redraw);
    if (redraw && p->visible && p->active && g->game_ops && g->game_ops->redraw_frame)
        g->game_ops->redraw_frame(g->game_ops->context, redraw);
}

static void draw_objects(AoeDiamondMapRor *m) {
    AoeGameMinimapRor *g = (AoeGameMinimapRor *)m;
    const AoeUiPlatformRor *pf = m->panel.system ? m->panel.system->platform : NULL;
    uint32_t now = pf && pf->now_ms ? pf->now_ms(pf->context) : 0;
    if (now - g->flash_ms > 0x14c) {
        g->flash = g->flash == 0;
        g->flash_ms = now;
    }
    aoe_diamond_map_draw_objects_ror(m);
}

static void draw_dot(AoeDiamondMapRor *m, int16_t x, int16_t y, uint8_t color,
                     int16_t shape, void *object) {
    AoeGameMinimapRor *g = (AoeGameMinimapRor *)m;
    if (m->phase == 4) color = g->flash ? 0x56 : 0xff;
    aoe_diamond_map_draw_dot_ror(m, x, y, color, shape, object);
}

static int32_t mouse_down(AoeUiPanelRor *p, uint8_t button, int32_t x, int32_t y,
                          int32_t c, int32_t s) {
    if (!aoe_diamond_map_tile_at_ror(&GM(p)->map, x, y, NULL, NULL)) return 0;
    return aoe_ui_panel_mouse_down_ror(p, button, x, y, c, s);
}

int32_t aoe_game_minimap_left_down_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                       int32_t c, int32_t s) {
    AoeGameMinimapRor *g = GM(p);
    (void)c, (void)s;
    if (waiting(g) || !running(g, 1)) return 0;
    if (style2(g)) {
        aoe_diamond_map_press_ror(&g->map, x, y);
        return 1;
    }
    if (running(g, 0)) aoe_ui_panel_capture_ror(p);
    return 1;
}

int32_t aoe_game_minimap_right_down_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                        int32_t c, int32_t s) {
    AoeGameMinimapRor *g = GM(p);
    (void)c, (void)s;
    if (waiting(g) || !running(g, 1)) return 0;
    if (style2(g)) {
        if (running(g, 0)) aoe_ui_panel_capture_ror(p);
        return 1;
    }
    aoe_diamond_map_press_ror(&g->map, x, y);
    return 1;
}

int32_t aoe_game_minimap_drag_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                  int32_t c, int32_t s) {
    AoeGameMinimapRor *g = GM(p);
    (void)c, (void)s;
    if (!running(g, 1)) return 0;
    if (!waiting(g)) {
        if (g->map.dragging) {
            aoe_diamond_map_drag_ror(&g->map, x, y);
            return 1;
        }
    } else if (!g->map.dragging) {
        aoe_ui_panel_release_capture_ror(p);
    } else {
        aoe_diamond_map_release_drag_ror(&g->map);
    }
    return 0;
}

int32_t aoe_game_minimap_left_up_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                     int32_t c, int32_t s) {
    AoeGameMinimapRor *g = GM(p);
    (void)c, (void)s;
    if (!running(g, 1)) return 0;
    if (g->map.dragging) {
        aoe_diamond_map_release_drag_ror(&g->map);
        return 1;
    }
    aoe_ui_panel_release_capture_ror(p);
    if (!waiting(g) && g->game_ops->state(g->game_ops->context) != 5 &&
        aoe_diamond_map_tile_at_ror(&g->map, x, y, NULL, NULL)) {
        int32_t mode = g->map.ops->edit_mode(g->map.ops->context);
        if (mode == 0) {
            if (!style2(g) && GVT(g)->order_here(g, x, y)) return 1;
        } else if (mode == 4) {
            GVT(g)->order_a0(g, x, y);
            return 1;
        } else if (mode == 5) {
            GVT(g)->order_a4(g, x, y);
            return 1;
        }
    }
    return 0;
}

int32_t aoe_game_minimap_right_up_ror(AoeUiPanelRor *p, int32_t x, int32_t y,
                                      int32_t c, int32_t s) {
    AoeGameMinimapRor *g = GM(p);
    const AoeGameMinimapOpsRor *ops = g->game_ops;
    int32_t state;
    (void)c;
    if (!running(g, 1)) return 0;
    if (g->map.dragging) {
        aoe_diamond_map_release_drag_ror(&g->map);
        return 1;
    }
    aoe_ui_panel_release_capture_ror(p);
    if (waiting(g) || (p->mouse_state != 1 && p->mouse_state != 2)) return 0;
    state = ops->state(ops->context);
    if (style2(g)) {
        if (g->map.ops->edit_mode(g->map.ops->context) == 0 && state != 5) {
            if (s != 0) {
                int16_t tx, ty;
                if (aoe_diamond_map_tile_at_ror(&g->map, x, y, &tx, &ty))
                    ops->player_point(ops->context, g->map.player, (float)tx + 0.5f,
                                      (float)ty + 0.5f);
                return 1;
            }
            if (GVT(g)->order_here(g, x, y)) return 1;
        }
    } else if (state != 7) {
        if (g->map.ops->edit_mode(g->map.ops->context) != 0) {
            ops->set_edit_mode(ops->context, 0, 0);
            return 0;
        }
        ops->clear_selection(ops->context, g->map.player);
    }
    return 0;
}

/* +0xf8 / +0xfc / +0x100: the player's order at the tile (0 when map
   orders are off); the last two return to edit mode 0. */
static uint8_t order(AoeGameMinimapRor *g, int32_t x, int32_t y, int32_t slot, int reset) {
    const AoeGameMinimapOpsRor *ops = g->game_ops;
    int16_t tx, ty;
    uint8_t r = 0;
    if (!ops->orders_enabled(ops->context)) return 0;
    if (aoe_diamond_map_tile_at_ror(&g->map, x, y, &tx, &ty))
        r = ops->player_order(ops->context, g->map.player, slot, (float)tx + 0.5f,
                              (float)ty + 0.5f);
    if (reset) ops->set_edit_mode(ops->context, 0, 0);
    return r;
}
static uint8_t order_here(AoeGameMinimapRor *g, int32_t x, int32_t y) { return order(g, x, y, 0xa8, 0); }
static uint8_t order_a0(AoeGameMinimapRor *g, int32_t x, int32_t y) { return order(g, x, y, 0xa0, 1); }
static uint8_t order_a4(AoeGameMinimapRor *g, int32_t x, int32_t y) { return order(g, x, y, 0xa4, 1); }

static AoeGameMinimapVtblRor vtable;
static int vtable_ready;

const AoeGameMinimapVtblRor *aoe_game_minimap_vtable_ror(void) {
    if (!vtable_ready) {
        vtable.base = *aoe_diamond_map_vtable_ror();
        vtable.base.base.set_redraw = set_redraw;
        vtable.base.base.mouse_down = mouse_down;
        vtable.base.base.actions[1] = aoe_game_minimap_left_down_ror;
        vtable.base.base.actions[3] = aoe_game_minimap_drag_ror;
        vtable.base.base.actions[4] = aoe_game_minimap_left_up_ror;
        vtable.base.base.actions[6] = aoe_game_minimap_right_down_ror;
        vtable.base.base.actions[8] = aoe_game_minimap_drag_ror;
        vtable.base.base.actions[9] = aoe_game_minimap_right_up_ror;
        vtable.base.draw_objects = draw_objects;
        vtable.base.draw_dot = draw_dot;
        vtable.order_here = order_here;
        vtable.order_a0 = order_a0;
        vtable.order_a4 = order_a4;
        vtable_ready = 1;
    }
    return &vtable;
}
