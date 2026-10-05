/* Patched RoR 1.0a game minimap layers (0x00518010..0x00518690,
   0x005085a0..0x00508690). */
#include "game_minimap.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *load_bitmap(void *c, const char *name, int32_t id, int32_t *w, int32_t *h) {
    static const int widths[7] = {2, 4, 6, 8, 6, 4, 2};
    uint8_t *b = calloc(8 * 7, 1);
    (void)c; (void)name; (void)id;
    *w = 8;
    *h = 7;
    for (int y = 0; y < 7; ++y) memset(b + y * 8 + (8 - widths[y]) / 2, 0xff, (size_t)widths[y]);
    return b;
}
static int32_t four(void *c) { (void)c; return 4; }
static int32_t mode;
static int32_t edit_mode(void *c) { (void)c; return mode; }
static float vx, vy;
static void set_view(void *c, const void *p, float x, float y) { (void)c; (void)p; vx = x; vy = y; }

static int32_t state = 4, comm, style = 1, orders = 1;
static int32_t gstate(void *c) { (void)c; return state; }
static int32_t gcomm(void *c) { (void)c; return comm; }
static int32_t gstyle(void *c) { (void)c; return style; }
static int gorders(void *c) { (void)c; return orders; }
static int32_t set_modes, last_set;
static void set_edit_mode(void *c, int32_t m, int32_t v) { (void)c; (void)v; ++set_modes; last_set = m; mode = m; }
static int cleared;
static void clear_selection(void *c, const void *p) { (void)c; (void)p; ++cleared; }
static int32_t order_slot;
static float ox, oy;
static uint8_t player_order(void *c, const void *p, int32_t slot, float x, float y) {
    (void)c; (void)p;
    order_slot = slot, ox = x, oy = y;
    return 1;
}
static float px_, py_;
static void player_point(void *c, const void *p, float x, float y) { (void)c; (void)p; px_ = x; py_ = y; }
static int frames;
static void redraw_frame(void *c, int32_t r) { (void)c; if (r) ++frames; }
static uint32_t now;
static uint32_t clock_ms(void *c) { (void)c; return now; }
static char world, player;

int main(void) {
    static AoeUiSystemRor sys;
    AoeUiPlatformRor pf = {0};
    AoeDiamondMapOpsRor ops = {0};
    AoeGameMinimapOpsRor gops = {0, gstate, gcomm, gstyle, gorders, set_edit_mode,
                                 clear_selection, player_order, player_point, redraw_frame};
    AoeUiAreaRor area = {(void *)1, 200, 100, NULL};
    static AoeGameMinimapRor g;
    AoeUiPanelRor *p = &g.map.panel;
    pf.now_ms = clock_ms;
    sys.platform = &pf;
    ops.load_bitmap = load_bitmap;
    ops.map_width = four;
    ops.map_height = four;
    ops.edit_mode = edit_mode;
    ops.set_view_center = set_view;
    aoe_game_minimap_init_ror(&g, &sys, &ops, &gops);
    assert(p->drag_slack == 0 && g.flash == 0);
    aoe_diamond_map_setup_ror(&g.map, &area, NULL, 100, 50, 8, 7, 0, NULL);
    assert(aoe_diamond_map_load_ror(&g.map, "map.bmp", 0xc4e1));
    g.map.player = &player;
    aoe_diamond_map_set_world_ror(&g.map, &world);

    /* Redraws pass to the game screen's frame while shown. */
    frames = 0;
    p->vt->set_redraw(p, 1);
    assert(frames == 1);

    /* Style 1: left clicks order the selection to the tile ((103, 53) is
       tile (1, 1)); edit modes 4 and 5 use the other orders and leave the
       mode. */
    assert(p->vt->actions[1](p, 103, 53, 0, 0) == 1 && !g.map.dragging);
    assert(p->vt->actions[4](p, 103, 53, 0, 0) == 1);
    assert(order_slot == 0xa8 && ox == 1.5f && oy == 1.5f);
    mode = 4;
    assert(p->vt->actions[4](p, 103, 53, 0, 0) == 1 && order_slot == 0xa0 && mode == 0);
    mode = 5;
    assert(p->vt->actions[4](p, 103, 53, 0, 0) == 1 && order_slot == 0xa4 && mode == 0);
    orders = 0;
    order_slot = 0;
    assert(p->vt->actions[4](p, 103, 53, 0, 0) == 0 && order_slot == 0);
    orders = 1;
    /* Right drags the view. */
    assert(p->vt->actions[6](p, 107, 53, 0, 0) == 1 && g.map.dragging && vx == 3.5f);
    assert(p->vt->actions[8](p, 100, 53, 0, 0) == 1 && vx == 0.5f);
    assert(p->vt->actions[9](p, 100, 53, 0, 0) == 1 && !g.map.dragging);
    /* A right-click leaves an edit mode, else clears the selection. */
    p->mouse_state = 1;
    mode = 3;
    assert(p->vt->actions[9](p, 103, 53, 0, 0) == 0 && mode == 0);
    assert(p->vt->actions[9](p, 103, 53, 0, 0) == 0 && cleared == 1);

    /* Style 2: the buttons swap; a right-click with the flag points. */
    style = 2;
    assert(p->vt->actions[1](p, 103, 53, 0, 0) == 1 && g.map.dragging && vx == 1.5f);
    assert(p->vt->actions[4](p, 103, 53, 0, 0) == 1 && !g.map.dragging);
    order_slot = 0;
    assert(p->vt->actions[9](p, 103, 53, 0, 0) == 1 && order_slot == 0xa8);
    assert(p->vt->actions[9](p, 107, 53, 0, 1) == 1 && px_ == 3.5f && py_ == 3.5f);

    /* Not while waiting on the network, nor outside play. */
    comm = 1;
    assert(p->vt->actions[1](p, 103, 53, 0, 0) == 0);
    comm = 0;
    state = 2;
    assert(p->vt->actions[1](p, 103, 53, 0, 0) == 0);
    state = 4;

    /* Presses only count on the map. */
    assert(p->vt->mouse_down(p, 1, 100, 50, 0, 0) == 0);

    p->vt->destroy(p);
    puts("game minimap ok");
    return 0;
}
