/* Patched RoR 1.0a game-screen resource bar (0x004f7c00..0x004f83d0). */
#include "resource_panel.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { int32_t x, y; uint32_t color; char text[24]; } Out;
static Out outs[16];
static int nouts;
static uint32_t now;
static uint32_t clock_ms(void *c) { (void)c; return now; }
static int tb(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r, uintptr_t f) {
    (void)c; (void)a; (void)r;
    assert(f == 9);
    return 1;
}
static void to(void *c, int32_t x, int32_t y, const char *t, int32_t n,
               uint32_t col) {
    (void)c;
    outs[nouts].x = x, outs[nouts].y = y, outs[nouts].color = col;
    snprintf(outs[nouts].text, sizeof outs[nouts].text, "%.*s", (int)n, t);
    ++nouts;
}
static void te(void *c) { (void)c; }
static const char *str(void *c, int32_t id) {
    (void)c;
    return id == 0x1d4ef ? "Gold: %d" : "?%d";
}

int main(void) {
    static AoeUiSystemRor sys;
    AoeUiPlatformRor pf = {0};
    AoeUiAreaRor area = {(void *)1, 640, 480};
    static AoeUiPanelRor parent;
    static AoeResourcePanelRor r;
    AoeUiPanelRor *p = &r.panel;
    float attrs[5] = {100.9f, 200.0f, 300.0f, 400.0f, 7.0f};
    AoeResourcePlayerRor player = {5, attrs};
    int32_t mode = 4, page = 0;
    char help[0x100];
    pf.now_ms = clock_ms;
    pf.text_begin = tb;
    pf.text_out = to;
    pf.text_end = te;
    sys.platform = &pf;
    aoe_ui_panel_init_ror(&parent, &sys);
    aoe_ui_panel_setup_ror(&parent, &area, NULL, 0, 0, 640, 480, 0);
    assert(aoe_resource_panel_init_ror(&r, &sys, &area, &parent, 9, &player));
    assert(r.interval_ms == 500 && r.color == 0xffffff && r.cached_count == 5 &&
           r.cached[0] == 100 && r.cached[4] == 7);
    r.game_mode = &mode;
    r.string = str;
    p->vt->set_fixed_position(p, 10, 0, 300, 20);

    /* Wood, food, gold, stone; shadow first. */
    p->vt->draw(p);
    assert(nouts == 8);
    assert(strcmp(outs[1].text, "200") == 0 && outs[1].x == 10 + 0x20 &&
           outs[1].y == 2 && outs[1].color == 0xffffff);
    assert(outs[0].x == 10 + 0x1f && outs[0].y == 3 && outs[0].color == 0);
    assert(strcmp(outs[3].text, "100") == 0 && outs[3].x == 10 + 0x63);
    assert(strcmp(outs[5].text, "400") == 0 && outs[5].x == 10 + 0xa6);
    assert(strcmp(outs[7].text, "300") == 0 && outs[7].x == 10 + 0xe9);

    /* Redraw only after half a second and a whole-number change. */
    p->redraw = 0;
    now = 1000;
    attrs[0] = 100.5f;
    p->vt->handle_idle(p);
    assert(!p->redraw && r.last_ms == 1000);
    attrs[0] = 101.0f;
    now = 1200;
    p->vt->handle_idle(p);
    assert(!p->redraw && r.last_ms == 1000);
    now = 1500;
    p->vt->handle_idle(p);
    assert(p->redraw && r.cached[0] == 101 && r.last_ms == 1500);
    p->redraw = 0;
    mode = 2;
    attrs[1] = 5.0f;
    now = 5000;
    p->vt->handle_idle(p);
    assert(!p->redraw && r.last_ms == 1500);

    /* The tooltip by column. */
    assert(p->vt->get_help(p, help, &page, 10 + 0xa0, 5));
    assert(strcmp(help, "Gold: 400") == 0 && page == -1);
    assert(!p->vt->get_help(p, help, &page, 400, 5));

    p->vt->destroy(p);
    puts("resource panel ok");
    return 0;
}
