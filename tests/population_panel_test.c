/* Patched RoR 1.0a game-screen population counter (0x004fa150..0x004fac10). */
#include "population_panel.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int32_t w, h; uint8_t *bits; } MemSurface;
static int32_t mem_create(void *c, void *d, int32_t w, int32_t h, int sys, void **out) {
    MemSurface *m = calloc(1, sizeof *m);
    (void)c; (void)d; (void)sys;
    m->w = w; m->h = h;
    m->bits = calloc((size_t)w, (size_t)h);
    *out = m;
    return 0;
}
static int32_t mem_describe(void *c, void *s, int32_t *pitch, uint32_t *caps) {
    (void)c;
    *pitch = ((MemSurface *)s)->w;
    *caps = 0x800;
    return 0;
}
static int32_t mem_fill(void *c, void *s, const int32_t *r, uint32_t color) {
    (void)c; (void)s; (void)r; (void)color;
    return 0;
}
static int32_t mem_lock(void *c, void *s, int wait, uint8_t **bits) {
    (void)c; (void)wait;
    *bits = ((MemSurface *)s)->bits;
    return 0;
}
static void mem_unlock(void *c, void *s, uint8_t *bits) { (void)c; (void)s; (void)bits; }
static void mem_release(void *c, void *o) {
    (void)c;
    free(((MemSurface *)o)->bits);
    free(o);
}
static int keys;
static int32_t mem_set_key(void *c, void *s, uint8_t color) {
    (void)c; (void)s;
    if (color == 1) ++keys;
    return 0;
}
static int blits, keyed_blits;
static int32_t mem_blt(void *c, void *d, const int32_t dr[4], void *s, const int32_t sr[4],
                       int keyed) {
    (void)c; (void)d; (void)dr; (void)s; (void)sr;
    ++blits;
    if (keyed) ++keyed_blits;
    return 0;
}

typedef struct { AoeUiRectRor r; uint32_t color; char text[32]; void *surface; } Rect;
static Rect rects[16];
static int nrects;
static void *text_surface;
static int tb(void *c, AoeUiAreaRor *a, const AoeUiRectRor *r, uintptr_t f) {
    (void)c; (void)r;
    assert(f == 6);
    text_surface = a->surface;
    return 1;
}
static void te(void *c) { (void)c; }
static void trect(void *c, const char *t, const AoeUiRectRor *r, uint32_t f, uint32_t col) {
    (void)c;
    assert(f == 0x21);
    if (nrects < 16) {
        rects[nrects].r = *r;
        rects[nrects].color = col;
        rects[nrects].surface = text_surface;
        snprintf(rects[nrects].text, sizeof rects[nrects].text, "%s", t);
    }
    ++nrects;
}

static char player_token;
static float attrs[40];
static int32_t limit_flag_value;
static const void *player(void *c) { (void)c; return &player_token; }
static int16_t number(void *c, const void *p) { (void)c; (void)p; return 2; }
static float attribute(void *c, const void *p, int i) { (void)c; (void)p; return attrs[i]; }
static int32_t limit_flag(void *c, int16_t n) { (void)c; assert(n == 2); return limit_flag_value; }
static float frame_seconds(void *c, const void *p) { (void)c; (void)p; return 0.6f; }
static void string(void *c, int32_t id, char *out, int n) {
    (void)c;
    snprintf(out, (size_t)n, "%s", id == 0x2b2a ? "pop: " : "");
}

int main(void) {
    static AoeUiSystemRor sys;
    AoeUiPlatformRor pf = {0};
    AoePopulationPanelOpsRor ops = {0, player, number, attribute, limit_flag,
                                    frame_seconds, string};
    static AoeDrawSystemRor d;
    static AoeDrawSurfaceRor surf;
    static uint8_t bits[64 * 64];
    static uint8_t *rows[64];
    static const AoeDrawOpsRor mops = {.create_surface = mem_create,
                                       .describe = mem_describe,
                                       .fill = mem_fill,
                                       .lock = mem_lock,
                                       .unlock = mem_unlock,
                                       .release = mem_release,
                                       .blt = mem_blt,
                                       .set_key = mem_set_key};
    AoeUiAreaRor area = {(void *)1, 640, 480, NULL};
    static AoeUiPanelRor parent;
    static AoePopulationPanelRor pp;
    AoeUiPanelRor *p = &pp.panel;
    AoeUiRectRor tr;
    int i;
    pf.text_begin = tb;
    pf.text_end = te;
    pf.text_rect = trect;
    sys.platform = &pf;
    attrs[11] = 7.9f;
    attrs[4] = 4.5f;
    attrs[32] = 50.0f;
    aoe_ui_panel_init_ror(&parent, &sys);
    aoe_ui_panel_setup_ror(&parent, &area, NULL, 0, 0, 640, 480, 0);

    /* Without a surface on the area the buffer has no system to join, but
       is kept, as the original keeps it; drop it to see the direct path.
       Without a buffer: the stored numbers (set by
       0x004fa480: 7, and 7 + 4 truncated apart) drawn straight onto the
       area, centred, four shadows then the text. */
    assert(aoe_population_panel_init_ror(&pp, &sys, &ops, &area, &parent, 6));
    /* Setup already sized it through the +0x0c slot: a 0 x 0 buffer. */
    assert(pp.color == 0xffffff && pp.flash_color == 0xffff && pp.buffer_w == 0);
    p->vt->set_rect(p, 10, 20, 40, 12);
    assert(pp.buffer && pp.buffer_w == 40 && p->need_restore);
    aoe_surface_free_ror(pp.buffer);
    free(pp.buffer);
    pp.buffer = NULL;
    p->vt->draw(p);
    assert(nrects == 5 && strcmp(rects[4].text, "pop: 0/0") == 0);
    aoe_population_panel_refresh_ror(&pp);
    assert(pp.population == 7 && pp.limit == 11);
    nrects = 0;
    p->vt->draw(p);
    assert(nrects == 5 && strcmp(rects[4].text, "pop: 7/11") == 0);
    assert(rects[0].color == 0 && rects[4].color == 0xffffff);
    assert(rects[0].r.left == 10 && rects[0].r.top == 20 && rects[0].r.right == 10 + 39 - 2 &&
           rects[0].r.bottom == 20 + 11 - 2);
    assert(rects[3].r.left == 12 && rects[3].r.top == 22 && rects[3].r.right == 49 &&
           rects[3].r.bottom == 31);
    assert(rects[4].r.left == 11 && rects[4].r.top == 21 && rects[4].r.right == 48);
    attrs[32] = 9.0f;
    aoe_population_panel_refresh_ror(&pp);
    assert(pp.limit == 9);
    attrs[32] = 50.0f;

    /* With a memory draw system: the text goes into the buffer and is
       copied out keyed; the first draw after sizing only renders. */
    d.type = 2;
    d.device = (void *)1;
    d.ops = &mops;
    surf.system = &d;
    surf.view.bits = bits;
    surf.view.width = 64;
    surf.view.height = 64;
    surf.view.pitch = 64;
    surf.view.direction = 1;
    surf.view.clip_right = 63;
    surf.view.clip_bottom = 63;
    for (i = 0; i < 64; ++i) rows[i] = bits + 64 * i;
    surf.rows = rows;
    mem_create(NULL, NULL, 64, 64, 0, &surf.surface);
    area.surface = &surf;
    pp.buffer_w = -1;
    p->vt->set_rect(p, 10, 20, 40, 12);
    assert(pp.buffer && pp.buffer_w == 40 && pp.buffer_h == 12 &&
           pp.buffer->view.width == 40);
    nrects = 0;
    p->vt->draw(p);
    /* The draw sums before truncating: 7.9 + 4.5 = 12.4 -> 12. */
    assert(nrects == 5 && rects[0].surface == pp.buffer && pp.buffer_ready &&
           strcmp(rects[4].text, "pop: 7/12") == 0);
    assert(rects[0].r.left == 0 && rects[0].r.right == 37 && rects[4].r.left == 1 &&
           rects[4].r.right == 38 && rects[4].color == 0xffffff);
    assert(!p->need_restore && pp.dest_x == p->rect.left && pp.dest_y == p->rect.top);
    nrects = 0;
    blits = keyed_blits = 0;
    p->vt->draw(p);
    assert(nrects == 0 && keys >= 1 && blits == 1 && keyed_blits == 1);
    p->vt->get_true_rect(p, &tr);
    assert(tr.left == pp.dest_x && tr.right == pp.dest_x + 39 && tr.bottom == pp.dest_y + 11);

    /* At the limit: lit (the lighting draw already counts its frame), dark
       after a world second, re-armed after another. Frames are 0.6 s. */
    limit_flag_value = 1;
    nrects = 0;
    p->vt->draw(p);
    assert(pp.flash == 1 && nrects == 5 && rects[4].color == 0xffff);
    nrects = 0;
    p->vt->draw(p); /* 1.2 s */
    assert(pp.flash == 2 && nrects == 5 && rects[4].color == 0xffffff);
    limit_flag_value = 0;
    nrects = 0;
    p->vt->draw(p); /* 0.6 s */
    assert(pp.flash == 2 && nrects == 0);
    p->vt->draw(p); /* 1.2 s */
    assert(pp.flash == 0 && nrects == 5);
    /* A changed number re-renders. */
    attrs[11] = 8.0f;
    nrects = 0;
    p->vt->draw(p);
    assert(nrects == 5 && strcmp(rects[4].text, "pop: 8/12") == 0);

    p->vt->destroy(p);
    mem_release(NULL, surf.surface);
    puts("population panel ok");
    return 0;
}
