/* Patched RoR 1.0a game-screen clock and countdown line
   (0x004fac10..0x004fb710). */
#include "timer_panel.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { AoeUiRectRor r; uint32_t color, format; char text[48]; } Rect;
static Rect rects[16];
static int nrects;
static uint32_t now;
static uint32_t clock_ms(void *c) { (void)c; return now; }
static int tb(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r, uintptr_t f) {
    (void)c; (void)a; (void)r; (void)f;
    return 1;
}
static void te(void *c) { (void)c; }
static void trect(void *c, const char *t, const AoeUiRectRor *r, uint32_t f, uint32_t col) {
    (void)c;
    if (nrects < 16) {
        rects[nrects].r = *r;
        rects[nrects].color = col;
        rects[nrects].format = f;
        snprintf(rects[nrects].text, sizeof rects[nrects].text, "%s", t);
    }
    ++nrects;
}

static char players[3];
static int16_t colors[3] = {0, 0, 2};
static const void *world_player(void *c, int32_t i) { (void)c; return i >= 1 && i <= 2 ? &players[i] : NULL; }
static int16_t number(void *c, const void *p) { (void)c; return (int16_t)((const char *)p - players); }
static int16_t color(void *c, const void *p) { (void)c; return colors[(const char *)p - players]; }
static uint32_t game_ms;
static uint32_t world_ms(void *c) { (void)c; return game_ms; }
static float speed(void *c) { (void)c; return 1.5f; }
static float years(void *c) { (void)c; return 12.7f; }
static int32_t wonder(void *c, int16_t n) { (void)c; return n == 2 ? 1998 : 0; }
static float artifacts(void *c, const void *p) { (void)c; (void)p; return -1.0f; }
static float ruins(void *c, const void *p) { (void)c; (void)p; return 41.0f; }
/* LANGUAGE.ENG 11300..11303. */
static void string(void *c, int32_t id, char *out, int n) {
    (void)c;
    const char *s = id == 0x2c24 ? "%d Years" : id == 0x2c25 ? "Wonder: %d Years"
                  : id == 0x2c26 ? "Artifacts: %d Years" : id == 0x2c27 ? "Ruins: %d Years" : "";
    snprintf(out, (size_t)n, "%s", s);
}

static int parent_redraws;
static void parent_set_redraw(AoeUiPanelRor *p, int32_t r) { (void)p; if (r) ++parent_redraws; }

int main(void) {
    static AoeUiSystemRor sys;
    AoeUiPlatformRor pf = {0};
    AoeTimerPanelOpsRor ops = {0, world_player, number, color, world_ms, speed, years,
                               wonder, artifacts, ruins, string};
    AoeUiAreaRor area = {(void *)1, 640, 480, NULL};
    static AoeUiPanelRor parent;
    static AoeUiPanelVtblRor parent_vt;
    static AoeTimerPanelRor t;
    AoeUiPanelRor *p = &t.panel;
    pf.now_ms = clock_ms;
    pf.text_begin = tb;
    pf.text_end = te;
    pf.text_rect = trect;
    sys.platform = &pf;
    aoe_ui_panel_init_ror(&parent, &sys);
    aoe_ui_panel_setup_ror(&parent, &area, NULL, 0, 0, 640, 480, 0);
    parent_vt = *parent.vt;
    parent_vt.set_redraw = parent_set_redraw;
    parent.vt = &parent_vt;
    assert(aoe_timer_panel_init_ror(&t, &sys, &ops, &area, &parent, 6));
    /* Positioned (a new buffer, without bits here), then drawn straight
       onto the area without it. */
    p->vt->set_fixed_position(p, 10, 20, 200, 14);
    assert(t.buffer && t.buffer_w == 200 && p->need_restore);
    aoe_surface_free_ror(t.buffer);
    free(t.buffer);
    t.buffer = NULL;

    /* Real time since the refresh: 3725 s = 01:02:05, with the speed. */
    now = 1000;
    aoe_timer_panel_refresh_ror(&t, now);
    aoe_timer_panel_set_ror(&t, 1, 0);
    assert(t.kind == 1 && p->active && t.color == 0xffffff && t.value == 0);
    now = 1000 + 3725 * 1000 + 999;
    p->vt->handle_idle(p);
    assert(t.value == 3725 && parent_redraws >= 1);
    p->vt->draw(p);
    assert(nrects == 5 && strcmp(rects[4].text, "01:02:05 (1.5)") == 0 &&
           rects[4].format == 0x20 && rects[4].color == 0xffffff && rects[0].color == 0);
    assert(rects[4].r.left == p->rect.left + 1 && rects[0].r.right == p->rect.left + 199 - 2);
    /* Game time. */
    aoe_timer_panel_set_ror(&t, 2, 0);
    game_ms = 59999;
    p->vt->handle_idle(p);
    nrects = 0;
    p->vt->draw(p);
    assert(strcmp(rects[4].text, "00:00:59 (1.5)") == 0);

    /* Player 2's wonder (colour 2, yellow), rounded up to five, right
       aligned; player 1's colour 0 is blue on grey. */
    aoe_timer_panel_set_ror(&t, 4, 2);
    assert(t.kind == 2 && t.color == 0xffff && t.shadow == 0 && t.value == 1998);
    nrects = 0;
    p->vt->draw(p);
    assert(strcmp(rects[4].text, "Wonder: 2000 Years") == 0 && rects[4].format == 0x22);
    aoe_timer_panel_set_ror(&t, 5, 1);
    assert(t.color == 0xff0000 && t.shadow == 0xb3b3b3 && t.value == 0); /* -1: none */
    aoe_timer_panel_set_ror(&t, 6, 1);
    nrects = 0;
    p->vt->draw(p);
    assert(t.value == 41 && strcmp(rects[4].text, "Ruins: 45 Years") == 0 &&
           rects[0].color == 0xb3b3b3);
    aoe_timer_panel_set_ror(&t, 3, 0);
    nrects = 0;
    p->vt->draw(p);
    assert(strcmp(rects[4].text, "15 Years") == 0);
    /* Setting the same mode and player does nothing; mode 0 hides. */
    parent_redraws = 0;
    aoe_timer_panel_set_ror(&t, 3, 0);
    assert(t.player == NULL && parent_redraws == 0);
    aoe_timer_panel_set_ror(&t, 0, 0);
    assert(!p->active);

    p->vt->destroy(p);
    puts("timer panel ok");
    return 0;
}
