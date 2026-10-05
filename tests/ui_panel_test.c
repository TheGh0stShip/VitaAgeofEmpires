/* Patched RoR 1.0a panel framework 0x00452030..0x00454400. */
#include "ui_panel.h"

#include <assert.h>
#include <string.h>

typedef struct {
    int invalidates, fills, captures, releases, created, deleted;
    uint32_t now;
    int left_down, left_up, left_hold, left_drag, hover, right_down;
} Env;

static Env env;
static void invalidate(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r) {
    (void)c; (void)a; (void)r;
    ++env.invalidates;
}
static void fill(void *c, const AoeUiRectRor *r, uint8_t color) {
    (void)c; (void)r; (void)color;
    ++env.fills;
}
static void capture(void *c, AoeUiAreaRor *a) { (void)c; (void)a; ++env.captures; }
static void release(void *c) { (void)c; ++env.releases; }
static uint32_t now(void *c) { (void)c; return env.now; }
static void string(void *c, int32_t id, char *out, int size) {
    (void)c;
    if (id == 7) strncpy(out, "Help text", (size_t)size);
}
static uintptr_t create_palette(void *c, const char *name, int32_t id) {
    (void)c; (void)name;
    ++env.created;
    return (uintptr_t)(100 + id);
}
static void delete_palette(void *c, uintptr_t f) { (void)c; (void)f; ++env.deleted; }

static int32_t left_down(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)p; (void)x; (void)y; (void)c; (void)s;
    ++env.left_down;
    return 0;
}
static int32_t left_up(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)p; (void)x; (void)y; (void)c; (void)s;
    ++env.left_up;
    return 0;
}
static int32_t left_hold(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)p; (void)c; (void)s;
    assert(x == 15 && y == 25);
    ++env.left_hold;
    return 0;
}
static int32_t left_drag(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)p; (void)x; (void)y; (void)c; (void)s;
    ++env.left_drag;
    return 0;
}
static int32_t hover(AoeUiPanelRor *p, int32_t x, int32_t y, int32_t c, int32_t s) {
    (void)p; (void)x; (void)y; (void)c; (void)s;
    ++env.hover;
    return 0;
}

int main(void) {
    AoeUiPlatformRor pf = {NULL, invalidate, NULL, fill, NULL, capture,
                           release, now, NULL, NULL, string, create_palette,
                           delete_palette};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    static AoeUiPanelRor root, a, b, c;
    static AoeUiPanelVtblRor vt;
    AoeUiPanelRor *cycle[3];
    char text[64];
    int32_t page = 0, x, y;

    sys.platform = &pf;
    sys.enabled = 1;
    sys.app_active = 1;

    /* Fonts: shared by name, counted, deleted on the last release. */
    assert(aoe_ui_palette_ror(&sys, "arial", 3) == 103 && env.created == 1);
    assert(aoe_ui_palette_ror(&sys, "other", 3) == 103 && sys.palette_refs[0] == 2);
    assert(!strcmp(sys.palette_names[0], "arial.pal"));
    aoe_ui_palette_release_ror(&sys, 103);
    assert(!env.deleted);
    aoe_ui_palette_release_ror(&sys, 103);
    assert(env.deleted == 1 && !sys.palettes[0]);

    /* Root fills the area; a child at (10, 20) 100 x 50. */
    assert(aoe_ui_panel_init_named_ror(&root, &sys, "Root"));
    assert(sys.panel_count == 1 && root.hold_ms == 250 && root.redraw == 2);
    assert(root.vt->setup(&root, &area, NULL, 0, 0, 640, 480, 3));
    assert(root.rect.right == 639 && root.visible && root.color == 3);
    vt = aoe_ui_panel_vtable_ror;
    vt.actions[AOE_UI_LEFT_DOWN] = left_down;
    vt.actions[AOE_UI_LEFT_UP] = left_up;
    vt.actions[AOE_UI_LEFT_HOLD] = left_hold;
    vt.actions[AOE_UI_LEFT_DRAG] = left_drag;
    vt.actions[AOE_UI_HOVER] = hover;
    aoe_ui_panel_init_ror(&a, &sys);
    a.vt = &vt;
    assert(a.vt->setup(&a, &area, &root, 10, 20, 100, 50, 0));
    assert(a.rect.left == 10 && a.rect.bottom == 69 && root.first == a.node);
    aoe_ui_panel_init_ror(&b, &sys);
    assert(b.vt->setup(&b, &area, &root, 200, 20, 50, 50, 0));
    assert(root.last == b.node && a.node->next == b.node);

    /* Anchored layout: c sits right of a with a 5 pixel gap, centred
       otherwise. */
    aoe_ui_panel_init_ror(&c, &sys);
    c.area = &area;
    c.parent = &root;
    aoe_ui_panel_set_position_mode_ror(&c, 1, 5, 0, 0, 0, 0, 30, 0, 40, &a,
                                       NULL, NULL, NULL);
    assert(c.rect.left == 610 && c.width == 30 && c.rect.bottom == 479 &&
           c.height == 40);
    aoe_ui_panel_set_position_mode_ror(&c, 7, 5, 0, 0, 0, 0, 30, 0, 40, &a,
                                       NULL, NULL, NULL);
    assert(c.rect.left == a.rect.right + 1 + 5 && c.width == 30 &&
           c.rect.top == 0 && c.height == 40);

    /* Clipping to the parent. */
    b.clip_to_parent = 1;
    b.vt->set_rect(&b, 600, 460, 100, 100);
    assert(b.clip.right == 639 && b.clip.bottom == 479 && b.rect.right == 699);
    b.vt->set_rect(&b, 700, 500, 10, 10);
    assert(!b.visible);

    /* Mouse: a press inside a's rect goes to a; holding past 250 ms is a
       hold, moving past the slack drags, release ends it. */
    a.captured = 0;
    env.now = 1000;
    assert(root.vt->handle_message(&root, (void *)1, 0x201, 0, (25u << 16) | 15) == 1);
    assert(env.left_down == 1 && a.mouse_state == 1 && a.mouse_button == 1 &&
           a.down_x == 15);
    env.now = 1249;
    root.vt->handle_idle(&root);
    assert(a.mouse_state == 1);
    env.now = 1250;
    root.vt->handle_idle(&root);
    assert(a.mouse_state == 2 && env.left_hold == 1);
    assert(aoe_ui_panel_capture_ror(&a) && env.captures == 1 && sys.capture == &a);
    aoe_ui_panel_init_ror(&c, &sys);
    assert(!aoe_ui_panel_capture_ror(&c));
    assert(root.vt->handle_message(&root, (void *)1, 0x200, 0, (25u << 16) | 15) == 1);
    assert(env.left_drag == 0); /* within the slack */
    root.vt->handle_message(&root, (void *)1, 0x200, 0, (30u << 16) | 40);
    assert(env.left_drag == 1 && a.mouse_state == 3);
    root.vt->handle_message(&root, (void *)1, 0x202, 0, (30u << 16) | 40);
    assert(env.left_up == 1 && a.mouse_state == 0 && a.mouse_button == 0);
    aoe_ui_panel_release_capture_ror(&a);
    assert(!sys.capture && env.releases == 1);
    root.vt->handle_message(&root, (void *)1, 0x200, 0, (30u << 16) | 40);
    assert(env.hover == 1);
    assert(!root.vt->handle_message(&root, (void *)2, 0x201, 0, 0));

    /* Focus and tab order. */
    root.has_focus = 1;
    cycle[0] = &a;
    cycle[1] = &b;
    cycle[2] = &root;
    aoe_ui_panels_tab_cycle_ror(cycle, 3);
    assert(a.tab_next == &b && a.tab_prev == &root && b.tab_stop);
    aoe_ui_panel_set_current_ror(&root, &a);
    assert(a.has_focus && root.current == &a);
    assert(a.vt->key_down(&a, 9, 1, 0, 0, 0) == 1);
    assert(root.current == &b && !a.has_focus && b.has_focus);

    /* Z-order re-sorts the child list. */
    aoe_ui_panel_set_z_ror(&a, 1, 0);
    assert(a.z == 1 && root.last == a.node);
    aoe_ui_panel_set_z_ror(&a, 2, 0);
    assert(a.z == -1 && root.first == a.node);

    /* Help text and point helpers. */
    aoe_ui_panel_set_help_ror(&b, 7, 3);
    b.vt->set_rect(&b, 200, 20, 50, 50);
    assert(root.vt->get_help(&root, text, &page, 210, 30) &&
           !strcmp(text, "Help text") && page == 3);
    x = 0;
    y = 100;
    assert(aoe_ui_panel_clamp_point_ror(&a, &x, &y) && x == 11 && y == 68);
    assert(aoe_ui_point_in_rect_ror(10, 20, &a.rect));
    {
        int32_t xy[2], ctl, shf;
        aoe_ui_decode_mouse_ror(0xc, 0xfff0000au, xy, &ctl, &shf);
        assert(xy[0] == 10 && xy[1] == -16 && ctl == 8 && shf == 4);
    }

    /* Deactivation and teardown. */
    a.vt->set_active(&a, 0);
    assert(!a.vt->is_inside(&a, 15, 25));
    aoe_ui_panel_release_ror(&a);
    assert(root.first == b.node);
    aoe_ui_panel_release_ror(&b);
    aoe_ui_panel_release_ror(&root);
    assert(sys.panel_count == 0);
    return 0;
}
