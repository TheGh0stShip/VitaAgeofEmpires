/* Patched RoR 1.0a game-screen edit toolbar (0x00473d70..0x00474a40). */
#include "edit_toolbar.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int32_t game_state = 4, edit_mode, comm;
static int32_t set_modes[8], nset_modes;
static int marked, cleared_sel, cleared_hl, closed;
static int16_t placed = -1, painted = -1;
static int32_t elev_delta, ex0, ey1;
static int32_t removed_code = -2;
static AoeUiPanelRor dialog;
static const char *dialog_name = "Object List Dialog";

static void set_edit_mode(void *c, int32_t m, int32_t v) {
    (void)c;
    assert(v == 0);
    set_modes[nset_modes++] = m;
    edit_mode = m;
}
static void set_state(void *c, int32_t s) { (void)c; game_state = s; }
static int32_t state(void *c) { (void)c; return game_state; }
static int32_t mode(void *c) { (void)c; return edit_mode; }
static int32_t comm_value(void *c) { (void)c; return comm; }
static void mark_world(void *c) { (void)c; ++marked; }
static void clear_selection(void *c) { (void)c; ++cleared_sel; }
static void clear_highlight(void *c) { (void)c; ++cleared_hl; }
static AoeUiPanelRor *dialog_parent;
static AoeUiPanelRor *open_object_list(void *c, AoeUiPanelRor *parent) {
    (void)c;
    dialog_parent = parent;
    return &dialog;
}
static const char *name(void *c, AoeUiPanelRor *d) { (void)c; assert(d == &dialog); return dialog_name; }
static int16_t choice(void *c, AoeUiPanelRor *d) { (void)c; (void)d; return 83; }
static void close_dialog(void *c, AoeUiPanelRor *d) { (void)c; assert(d == &dialog); ++closed; }
static void place(void *c, int16_t id) { (void)c; placed = id; }
static void paint(void *c, int16_t id) { (void)c; painted = id; }
static int view_tiles(void *c, int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1) {
    (void)c;
    *x0 = 3, *y0 = 4, *x1 = 5, *y1 = 6;
    return 1;
}
static void adjust(void *c, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t d) {
    (void)c; (void)y0; (void)x1;
    ex0 = x0, ey1 = y1, elev_delta = d;
}
static int32_t remove_selected(void *c) { (void)c; return removed_code; }

static int view_redraws;
static void view_set_redraw(AoeUiPanelRor *p, int32_t r) { (void)p; if (r == 2) ++view_redraws; }

int main(void) {
    static AoeUiSystemRor sys;
    AoeUiPlatformRor pf = {0};
    AoeEditToolbarOpsRor ops = {0};
    AoeUiAreaRor area = {(void *)1, 640, 480, NULL};
    static AoeUiPanelRor screen, view, map;
    static AoeUiPanelVtblRor view_vt;
    static AoeEditToolbarRor t;
    AoeUiPanelRor *p = &t.panel;
    sys.platform = &pf;
    ops.set_edit_mode = set_edit_mode;
    ops.set_state = set_state;
    ops.state = state;
    ops.edit_mode = mode;
    ops.comm_value = comm_value;
    ops.mark_world = mark_world;
    ops.clear_selection = clear_selection;
    ops.clear_highlight = clear_highlight;
    ops.open_object_list = open_object_list;
    ops.dialog_name = name;
    ops.dialog_choice = choice;
    ops.close_dialog = close_dialog;
    ops.set_place_object = place;
    ops.set_paint_terrain = paint;
    ops.view_tiles = view_tiles;
    ops.adjust_elevation = adjust;
    ops.remove_selected = remove_selected;
    aoe_ui_panel_init_ror(&screen, &sys);
    aoe_ui_panel_setup_ror(&screen, &area, NULL, 0, 0, 640, 480, 0);
    aoe_ui_panel_init_ror(&view, &sys);
    view_vt = *view.vt;
    view_vt.set_redraw = view_set_redraw;
    view.vt = &view_vt;
    map.vt = &view_vt;

    /* No main view: the setup fails. */
    assert(!aoe_edit_toolbar_init_ror(&t, &sys, &ops, NULL, &area, &screen, 0, 0, 400, 30,
                                      7, 8, 12, NULL, &screen, NULL, &map));
    p->vt->destroy(p);
    memset(&t, 0, sizeof t);

    edit_mode = 0x12;
    assert(aoe_edit_toolbar_init_ror(&t, &sys, &ops, NULL, &area, &screen, 0, 0, 400, 30,
                                     7, 8, 12, NULL, &screen, &view, &map));
    assert(strcmp(t.game->text1[0], "Game") == 0 && strcmp(t.game->text1[1], "<Game>") == 0);
    assert(t.move->panel.x - p->x == 0x68 && t.del->panel.x - p->x == 0x9c &&
           t.cmd->panel.x - p->x == 0x13a && t.cmd->panel.width == 0x32 &&
           t.cmd->panel.height == 0x1e && t.cmd->state_count == 2);
    assert(!t.terr->panel.active && !t.elev->panel.active && t.game->panel.active);
    assert(t.game->group_count == 6 && t.game->draw_type == 3 && t.game->font == 7);
    /* Synced for mode 0x12: Move down, and leaving 0x12 cleared the
       selection. */
    assert(t.move->pressed && !t.game->pressed && cleared_sel == 1);

    /* The buttons switch modes. */
    p->vt->action(p, &t.game->panel, 1, 0, 0);
    p->vt->action(p, &t.del->panel, 1, 0, 0);
    p->vt->action(p, &t.elev->panel, 1, 0, 0);
    p->vt->action(p, &t.move->panel, 1, 0, 0);
    assert(nset_modes == 4 && set_modes[0] == 0 && set_modes[1] == 0xe &&
           set_modes[2] == 3 && set_modes[3] == 0x12);
    assert(p->vt->action(p, &t.game->panel, 2, 0, 0) == 0); /* not a click */
    assert(p->vt->action(p, NULL, 1, 0, 0) == 0);
    /* Cmd pauses and resumes. */
    p->vt->action(p, &t.cmd->panel, 1, 0, 0);
    assert(game_state == 6 && marked == 1);
    p->vt->action(p, &t.cmd->panel, 1, 0, 0);
    assert(game_state == 4 && marked == 1);

    /* Obj opens the object list over the screen; choosing places. */
    p->vt->action(p, &t.obj->panel, 1, 0, 0);
    assert(t.dialog == &dialog && dialog_parent == &screen);
    cleared_sel = 0;
    p->vt->action(p, &dialog, 1, 0, 0);
    assert(placed == 83 && closed == 1 && !t.dialog && edit_mode == 1 && cleared_sel == 1);
    /* The terrain list paints; a cancel resyncs the buttons. */
    t.dialog = &dialog;
    dialog_name = "Terrain List Dialog";
    p->vt->action(p, &dialog, 1, 0, 0);
    assert(painted == 83 && edit_mode == 2 && closed == 2);
    t.dialog = &dialog;
    p->vt->action(p, &dialog, 2, 0, 0);
    assert(closed == 3 && cleared_hl == 1 && t.terr->pressed);

    /* Keys: PageUp / PageDown in elevation mode; Delete in move mode. */
    edit_mode = 3;
    assert(p->vt->key_action(p, 0x21, 0, 0, 0, 0) == 1);
    assert(elev_delta == 1 && ex0 == 3 && ey1 == 6 && view_redraws == 2);
    assert(p->vt->key_action(p, 0x22, 0, 0, 0, 0) == 1 && elev_delta == -1);
    edit_mode = 0;
    assert(p->vt->key_action(p, 0x21, 0, 0, 0, 0) == 0);
    edit_mode = 0x12;
    view_redraws = 0;
    removed_code = 0;
    assert(p->vt->key_action(p, 0x2e, 0, 0, 0, 0) == 1 && view_redraws == 1);
    removed_code = 0x14;
    assert(p->vt->key_action(p, 0x2e, 0, 0, 0, 0) == 1 && view_redraws == 1);
    /* Not while waiting on the network, nor outside play. */
    comm = 1;
    assert(p->vt->key_action(p, 0x2e, 0, 0, 0, 0) == 0);
    comm = 0;
    game_state = 2;
    assert(p->vt->key_action(p, 0x2e, 0, 0, 0, 0) == 0);

    p->vt->destroy(p);
    puts("edit toolbar ok");
    return 0;
}
