#include "edit_toolbar.h"

#include "draw_system.h"

#include <stdlib.h>
#include <string.h>

#define ET(p) ((AoeEditToolbarRor *)(p))

static AoeUiButtonRor *button(AoeEditToolbarRor *t, int32_t x, const char *label,
                              const char *down, void *sound, uintptr_t font,
                              int32_t fw, int32_t fh,
                              const AoeUiButtonOpsRor *ops) {
    AoeUiButtonRor *b = calloc(1, sizeof *b);
    if (!b) return NULL;
    aoe_ui_button_init_ror(b, t->panel.system, ops);
    aoe_ui_button_setup_ror(b, t->panel.area, &t->panel, x, 0, 0x32, 0x1e, 3, sound, 1, 0);
    aoe_ui_button_set_text_info_ror(b, label, font, fw, fh, -1, -1);
    aoe_ui_button_set_text_ror(b, 1, down);
    return b;
}

int aoe_edit_toolbar_init_ror(AoeEditToolbarRor *t, AoeUiSystemRor *system,
                              const AoeEditToolbarOpsRor *ops,
                              const AoeUiButtonOpsRor *button_ops,
                              AoeUiAreaRor *area, AoeUiPanelRor *parent,
                              int32_t x, int32_t y, int32_t w, int32_t h,
                              uintptr_t font, int32_t font_w, int32_t font_h,
                              void *sound, AoeUiPanelRor *screen,
                              AoeUiPanelRor *main_view, AoeUiPanelRor *map) {
    AoeUiButtonRor *group[6];
    int ok = 1, i;
    aoe_ui_panel_init_ror(&t->panel, system);
    t->panel.vt = aoe_edit_toolbar_vtable_ror();
    t->ops = ops;
    t->screen = screen;
    t->main_view = main_view;
    t->map = map;
    t->game = t->obj = t->del = t->move = t->terr = t->elev = t->cmd = NULL;
    t->dialog = NULL;
    if (!aoe_ui_panel_setup_ror(&t->panel, area, parent, x, y, w, h, 0)) ok = 0;
    if (!t->main_view) ok = 0;
    if (!ok) {
        t->panel.field_d8 = 1;
        return 0;
    }
#define MAKE(f, px, l, d) \
    if (!(t->f = button(t, px, l, d, sound, font, font_w, font_h, button_ops))) goto fail
    MAKE(game, 0, "Game", "<Game>");
    MAKE(obj, 0x34, "Obj", "<Obj>");
    MAKE(move, 0x68, "Move", "<Move>");
    MAKE(del, 0x9c, "Del", "<Del>");
    MAKE(terr, 0xd0, "Terr", "<Terr>");
    t->terr->panel.vt->set_active(&t->terr->panel, 0);
    MAKE(elev, 0x104, "Elev", "<Elev>");
    t->elev->panel.vt->set_active(&t->elev->panel, 0);
    group[0] = t->game;
    group[1] = t->obj;
    group[2] = t->del;
    group[3] = t->move;
    group[4] = t->terr;
    group[5] = t->elev;
    for (i = 0; i < 6; ++i) aoe_ui_button_set_group_ror(group[i], group, 6);
    MAKE(cmd, 0x13a, "Cmd", "<Cmd>");
#undef MAKE
    aoe_ui_button_set_cycle_ror(t->cmd, 2);
    if (ops && ops->edit_mode) {
        int32_t m = ops->edit_mode(ops->context);
        aoe_edit_toolbar_sync_ror(t, m, m);
    }
    return 1;
fail:
    t->panel.field_d8 = 1;
    return 0;
}

static void drop(AoeUiButtonRor **b) {
    if (*b) {
        (*b)->panel.vt->destroy(&(*b)->panel);
        free(*b);
        *b = NULL;
    }
}

static void destroy(AoeUiPanelRor *p) {
    AoeEditToolbarRor *t = ET(p);
    drop(&t->game);
    drop(&t->obj);
    drop(&t->move);
    drop(&t->del);
    drop(&t->terr);
    drop(&t->elev);
    drop(&t->cmd);
    aoe_ui_panel_release_ror(p);
}

static void clear_selection(AoeEditToolbarRor *t) {
    if (t->ops && t->ops->clear_selection) t->ops->clear_selection(t->ops->context);
}

void aoe_edit_toolbar_sync_ror(AoeEditToolbarRor *t, int32_t old_mode,
                               int32_t new_mode) {
    AoeUiButtonRor *b;
    switch (old_mode) {
    case 1: case 0x12:
        clear_selection(t);
        break;
    case 2: case 3:
        if (t->ops && t->ops->clear_highlight) t->ops->clear_highlight(t->ops->context);
        break;
    default:
        break;
    }
    switch (new_mode) {
    case 1: b = t->obj; break;
    case 2: b = t->terr; break;
    case 3: b = t->elev; break;
    case 0xe: b = t->del; break;
    case 0x12: b = t->move; break;
    default: b = t->game; break;
    }
    if (b) aoe_ui_button_select_ror(b);
    if (new_mode == 2 || new_mode == 3) clear_selection(t);
}

static void close_dialog(AoeEditToolbarRor *t) {
    if (t->dialog && t->ops->close_dialog) t->ops->close_dialog(t->ops->context, t->dialog);
    t->dialog = NULL;
}

int32_t aoe_edit_toolbar_action_ror(AoeUiPanelRor *p, AoeUiPanelRor *from,
                                    int32_t code, int32_t value, int32_t extra) {
    AoeEditToolbarRor *t = ET(p);
    const AoeEditToolbarOpsRor *ops = t->ops;
    (void)value, (void)extra;
    if (!from) return 0;
    if (code == 1 && t->game && from == &t->game->panel) {
        ops->set_edit_mode(ops->context, 0, 0);
        return 1;
    }
    if (code == 1 && t->obj && from == &t->obj->panel) {
        t->dialog = ops->open_object_list ? ops->open_object_list(ops->context, t->screen)
                                          : NULL;
        return 1;
    }
    if (code == 1 && t->move && from == &t->move->panel) {
        ops->set_edit_mode(ops->context, 0x12, 0);
        return 1;
    }
    if (code == 1 && t->del && from == &t->del->panel) {
        ops->set_edit_mode(ops->context, 0xe, 0);
        return 1;
    }
    if (code == 1 && t->elev && from == &t->elev->panel) {
        ops->set_edit_mode(ops->context, 3, 0);
        return 1;
    }
    if (code == 1 && t->cmd && from == &t->cmd->panel) {
        if (ops->state(ops->context) == 4) {
            ops->set_state(ops->context, 6);
            if (ops->mark_world) ops->mark_world(ops->context);
        } else {
            ops->set_state(ops->context, 4);
        }
        return 1;
    }
    if (!t->dialog || from != t->dialog) return 0;
    {
        const char *name = ops->dialog_name ? ops->dialog_name(ops->context, from) : "";
        if (strcmp(name, "Object List Dialog") == 0) {
            if (code == 1) {
                ops->set_place_object(ops->context, ops->dialog_choice(ops->context, from));
                close_dialog(t);
                ops->set_edit_mode(ops->context, 1, 0);
                clear_selection(t);
                return 1;
            }
        } else if (strcmp(name, "Terrain List Dialog") == 0) {
            if (code == 1) {
                ops->set_paint_terrain(ops->context, ops->dialog_choice(ops->context, from));
                close_dialog(t);
                ops->set_edit_mode(ops->context, 2, 0);
                return 1;
            }
        } else {
            return 1;
        }
    }
    if (code == 2) {
        int32_t m = ops->edit_mode(ops->context);
        close_dialog(t);
        aoe_edit_toolbar_sync_ror(t, m, m);
    }
    return 1;
}

static void redraw_views(AoeEditToolbarRor *t) {
    t->main_view->vt->set_redraw(t->main_view, 2);
    if (t->map) t->map->vt->set_redraw(t->map, 2);
}

int32_t aoe_edit_toolbar_key_ror(AoeUiPanelRor *p, int32_t key, int32_t a,
                                 int32_t b, int32_t c, int32_t d) {
    AoeEditToolbarRor *t = ET(p);
    const AoeEditToolbarOpsRor *ops = t->ops;
    int32_t state, mode;
    (void)a, (void)b, (void)c, (void)d;
    if (ops->comm_value && ops->comm_value(ops->context) != 0) return 0;
    state = ops->state(ops->context);
    if (state != 4 && state != 6 && state != 7) return 0;
    mode = ops->edit_mode(ops->context);
    if ((key == 0x21 || key == 0x22) && mode == 3) {
        int32_t x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        if (ops->view_tiles) ops->view_tiles(ops->context, &x0, &y0, &x1, &y1);
        if (ops->adjust_elevation)
            ops->adjust_elevation(ops->context, x0, y0, x1, y1, key == 0x21 ? 1 : -1);
        redraw_views(t);
        return 1;
    }
    if (key == 0x2e && mode == 0x12) {
        int32_t r = ops->remove_selected ? ops->remove_selected(ops->context) : -1;
        if (r == 0) t->main_view->vt->set_redraw(t->main_view, 2);
        return 1;
    }
    return 0;
}

static void draw(AoeUiPanelRor *p) {
    struct AoeDrawSurfaceRor *s;
    if (!p->area || !p->visible || !p->active) return;
    p->vt->begin_draw(p, 0);
    s = p->area->surface;
    if (s) {
        int32_t r[4] = {p->clip.left, p->clip.top, p->clip.right, p->clip.bottom};
        aoe_surface_fill_ror(s, r, 0);
    }
    p->vt->end_draw(p);
}

static AoeUiPanelVtblRor vtable;
static int vtable_ready;

const AoeUiPanelVtblRor *aoe_edit_toolbar_vtable_ror(void) {
    if (!vtable_ready) {
        vtable = aoe_ui_panel_vtable_ror;
        vtable.destroy = destroy;
        vtable.draw = draw;
        vtable.key_action = aoe_edit_toolbar_key_ror;
        vtable.action = aoe_edit_toolbar_action_ror;
        vtable_ready = 1;
    }
    return &vtable;
}
