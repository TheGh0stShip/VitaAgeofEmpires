/* Patched RoR 1.0a button panel 0x0045ebf0..0x00460700. */
#include "ui_button.h"
#include "draw_system.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int actions[8], plays, commands, paints;
    int32_t last_id;
} Env;
static Env env;

static int32_t parent_action(AoeUiPanelRor *p, AoeUiPanelRor *from, int32_t code,
                             int32_t id, int32_t value) {
    (void)p; (void)from; (void)value;
    ++env.actions[code];
    env.last_id = id;
    return 0;
}
static void play(void *c, void *s) { (void)c; (void)s; ++env.plays; }
static void command(void *c, AoeUiAreaRor *a, int32_t id, int32_t v) {
    (void)c; (void)a; (void)v;
    ++env.commands;
    env.last_id = id;
}
static void paint(void *c, AoeUiAreaRor *a) { (void)c; (void)a; ++env.paints; }
static void *sound_by_id(void *c, int32_t id) { (void)c; return id == 4 ? (void *)&env : NULL; }
static int shape_hit(void *c, void *pic, int32_t x, int32_t y, int16_t f) {
    (void)c; (void)pic; (void)f;
    return x < 5 && y < 5;
}
static void string(void *c, int32_t id, char *out, int size) {
    (void)c;
    if (id == 9) strncpy(out, "Top\nBottom", (size_t)size);
}

static char outs[8][40];
static int nouts;
static int t_begin(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r, uintptr_t f) {
    (void)c; (void)a; (void)r; (void)f;
    return 1;
}
static void t_out(void *c, int32_t x, int32_t y, const char *t, int32_t n,
                  uint32_t color) {
    (void)c;
    if (nouts < 8)
        snprintf(outs[nouts], sizeof outs[0], "%d,%d,%.*s,%x", (int)x, (int)y,
                 (int)n, t, (unsigned)color);
    ++nouts;
}
static void t_end(void *c) { (void)c; }
static int32_t t_width(void *c, uintptr_t f, const char *t, int32_t n) {
    (void)c; (void)f; (void)t;
    return n * 6;
}

int main(void) {
    AoeUiPlatformRor pf = {0};
    static AoeUiSystemRor sys;
    AoeUiAreaRor area = {(void *)1, 640, 480};
    AoeUiButtonOpsRor ops = {NULL, NULL, NULL, play, sound_by_id, paint,
                             command, shape_hit};
    static AoeUiPanelRor root;
    static AoeUiPanelVtblRor root_vt;
    static AoeUiButtonRor a, b, c;
    AoeUiButtonRor *group[2] = {&a, &b};
    const char *l1, *l2;
    uint32_t col, sh;

    pf.string = string;
    sys.platform = &pf;
    sys.enabled = 1;
    aoe_ui_panel_init_ror(&root, &sys);
    root_vt = aoe_ui_panel_vtable_ror;
    root_vt.action = parent_action;
    root.vt = &root_vt;
    root.vt->setup(&root, &area, NULL, 0, 0, 640, 480, 0);

    aoe_ui_button_init_ror(&a, &sys, &ops);
    assert(a.type == 1 && a.state_count == 1 && a.frames[8] == -1 &&
           a.text_color[3] == 0xffffff && a.text_x == -1 && a.sound_id == -1);
    assert(aoe_ui_button_setup_ror(&a, &area, &root, 10, 10, 80, 20, 3, &env, 1, 77));
    assert(a.ids[0] == 77 && a.notify == 1 && a.draw_type == 3 && a.sound == &env);

    /* Click: press notifies 2, release 3 then 1 (clicked). */
    assert(root.vt->mouse_down(&root, 1, 20, 15, 0, 0) == 1);
    assert(a.pressed && env.actions[2] == 1 && env.plays == 1 && a.panel.captured);
    a.panel.vt->mouse_move(&a.panel, 200, 200, 0, 0);
    assert(!a.pressed && env.actions[3] == 1);
    a.panel.vt->mouse_move(&a.panel, 20, 15, 0, 0);
    assert(a.pressed && env.actions[2] == 2);
    a.panel.vt->mouse_up(&a.panel, 1, 20, 15, 0, 0);
    assert(!a.pressed && env.actions[3] == 2 && env.actions[1] == 1 &&
           env.last_id == 77 && env.paints == 1 && !a.panel.captured);

    /* Command notification and cycling states. */
    a.notify = 0;
    aoe_ui_button_set_cycle_ror(&a, 3);
    aoe_ui_button_set_id_ror(&a, 1, 88, 0);
    aoe_ui_button_click_ror(&a);
    assert(a.state == 1 && env.commands == 1 && env.last_id == 88);
    aoe_ui_button_click_ror(&a);
    aoe_ui_button_click_ror(&a);
    assert(a.state == 0);
    aoe_ui_button_set_state_by_id_ror(&a, 88);
    assert(aoe_ui_button_state_ror(&a) == 1 && aoe_ui_button_id_ror(&a) == 88);

    /* Radio group: selecting b clears a. */
    aoe_ui_button_init_ror(&b, &sys, &ops);
    aoe_ui_button_setup_ror(&b, &area, &root, 100, 10, 80, 20, 5, NULL, 1, 1);
    aoe_ui_button_set_group_ror(&b, group, 2);
    a.pressed = 1;
    aoe_ui_button_select_ror(&b);
    assert(!a.pressed && a.state == 0 && b.pressed && b.state == 1 && b.type == 2);

    /* Texts, colours, hotkeys and Enter. */
    assert(aoe_ui_button_set_text_id_ror(&b, 2, 9));
    assert(aoe_ui_button_texts_ror(&b, 2, &l1, &l2) && !strcmp(l1, "Top") &&
           !strcmp(l2, "Bottom"));
    aoe_ui_button_set_text_color_ror(&b, 1, 5, 6);
    aoe_ui_button_colors_ror(&b, -1, &col, &sh);
    assert(col == 5 && sh == 6);
    b.hotkey = 'Q';
    b.type = 1;
    assert(b.panel.vt->key_down(&b.panel, 'Q', 1, 0, 0, 0) == 1 && b.held_key == 'Q');
    memset(env.actions, 0, sizeof(env.actions));
    assert(b.panel.vt->handle_message(&b.panel, (void *)1, 0x101, 'Q', 0) == 1);
    assert(!b.held_key && env.actions[1] == 1);
    assert(b.panel.vt->key_action(&b.panel, 0x0d, 1, 0, 0, 0) == 1 && b.held_key == 0x0d);
    aoe_ui_button_cancel_ror(&b);
    assert(!b.held_key && !b.pressed);

    /* Shape hit test and disabling. */
    aoe_ui_button_init_ror(&c, &sys, &ops);
    aoe_ui_button_setup_ror(&c, &area, &root, 300, 300, 40, 40, 2, NULL, 1, 0);
    c.rect_hit = 0;
    aoe_ui_button_set_picture_ror(&c, 0, (void *)&env, 2);
    assert(aoe_ui_button_hit_test_ror(&c, 302, 302) && !aoe_ui_button_hit_test_ror(&c, 330, 330));
    aoe_ui_button_set_disabled_ror(&c, 1);
    assert(!c.panel.handles_mouse && !c.panel.vt->actions[AOE_UI_LEFT_DOWN](&c.panel, 0, 0, 0, 0));
    aoe_ui_button_set_sound_id_ror(&c, 4);
    c.panel.vt->stop_sound(&c.panel);
    assert(!c.sound && c.panel.vt->restart_sound(&c.panel) && c.sound == &env);

    aoe_ui_button_release_ror(&c);

    /* Drawing a type-3 button: centred text with its shadow one pixel
       down-left, then the bevel; pressed, both shift right and up and the
       bevel inverts. */
    {
        static uint8_t bits[640 * 40];
        static uint8_t *rows[40];
        static AoeDrawSystemRor d;
        static AoeDrawSurfaceRor surf;
        static AoeUiButtonRor btn;
        int i;
        d.type = 1;
        surf.system = &d;
        surf.view.bits = bits;
        surf.view.width = 640;
        surf.view.height = 40;
        surf.view.pitch = 640;
        surf.view.direction = 1;
        surf.view.clip_right = 639;
        surf.view.clip_bottom = 39;
        for (i = 0; i < 40; ++i) rows[i] = bits + 640 * i;
        surf.rows = rows;
        area.surface = &surf;
        pf.text_begin = t_begin;
        pf.text_out = t_out;
        pf.text_end = t_end;
        pf.text_width = t_width;
        aoe_ui_button_init_ror(&btn, &sys, &ops);
        assert(aoe_ui_button_setup_ror(&btn, &area, &root, 10, 10, 80, 20, 3,
                                       NULL, 1, 1));
        btn.text1[0] = "Go";
        btn.font_height = 10;
        btn.text_color[0] = 0x11;
        btn.text_shadow[0] = 0x22;
        btn.bevel_type = 2;
        memcpy(btn.bevel, (const uint8_t[6]){1, 2, 3, 4, 5, 6}, 6);
        btn.panel.vt->draw(&btn.panel);
        /* x = 10 - 12/2 + 40 = 44, y = 10 - 5 + 10 = 15. */
        assert(nouts == 2 && !strcmp(outs[0], "43,16,Go,22") &&
               !strcmp(outs[1], "44,15,Go,11"));
        /* Bevel type 2 unpressed: top/right in bevel[0], bottom/left in
           bevel[5]. */
        assert(bits[10 * 640 + 11] == 1 && bits[29 * 640 + 10] == 6);
        nouts = 0;
        btn.pressed = 1;
        memset(bits, 0, sizeof bits);
        btn.panel.vt->draw(&btn.panel);
        assert(!strcmp(outs[1], "45,14,Go,11"));
        assert(bits[10 * 640 + 11] == 6 && bits[29 * 640 + 10] == 1);
        btn.pressed = 0;
        area.surface = NULL;
        btn.text1[0] = NULL;
        aoe_ui_button_release_ror(&btn);
    }
    aoe_ui_button_release_ror(&b);
    aoe_ui_button_release_ror(&a);
    aoe_ui_panel_release_ror(&root);
    return 0;
}
