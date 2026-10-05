#include "mouse_pointer.h"

#include <assert.h>
#include <string.h>

static uint32_t now = 1000;
static int32_t cx = 100, cy = 80;
static int windowed_flag;
static int blts, fasts, invalidates, dirties, areas, freed, cursor_state = -1, locks, draws;
static AoeMouseRectRor last_dst;
static void *last_dst_surface;
static int32_t fast_x, fast_y, set_x, set_y, fail_hr;
static char last_load[64];
static int clipper_obj, clipper_sets;
static int area_store[20];

static uint32_t t_time(void *c) { (void)c; return now; }
static int t_windowed(void *c) { (void)c; return windowed_flag; }
static void t_pos(void *c, int32_t *x, int32_t *y) { (void)c; *x = cx; *y = cy; }
static void t_wrect(void *c, AoeMouseRectRor *r) { (void)c; r->left = 10; r->top = 20; r->right = 110; r->bottom = 220; }
static void t_ssize(void *c, int32_t *w, int32_t *h) { (void)c; *w = 640; *h = 480; }
static void t_setpos(void *c, int32_t x, int32_t y) { (void)c; set_x = x; set_y = y; }
static void t_cursor(void *c, int a) { (void)c; cursor_state = a; }
static void t_inval(void *c, const AoeMouseRectRor *r) { (void)c; (void)r; ++invalidates; }
static void t_dirty(void *c, const AoeMouseRectRor *r) { (void)c; (void)r; ++dirties; }
static int32_t t_blt(void *c, void *dst, const AoeMouseRectRor *dr, void *src, const AoeMouseRectRor *sr) {
    (void)c; (void)src; (void)sr;
    ++blts;
    last_dst = *dr;
    last_dst_surface = dst;
    return fail_hr;
}
static int32_t t_fast(void *c, void *dst, int32_t x, int32_t y, void *src, const AoeMouseRectRor *r) {
    (void)c; (void)dst; (void)src; (void)r;
    ++fasts; fast_x = x; fast_y = y;
    return 0;
}
static int32_t t_getclip(void *c, void *s, void **clip) { (void)c; (void)s; *clip = &clipper_obj; return 0; }
static int32_t t_setclip(void *c, void *s, void *clip) { (void)c; (void)s; (void)clip; ++clipper_sets; return 0; }
static void t_sync(void *c) { (void)c; }
static void *t_area_new(void *c, const char *n, int32_t w, int32_t h) {
    (void)c; (void)n; assert(w == 64 && h == 48);
    return &area_store[areas++];
}
static void t_area_free(void *c, void *a) { (void)c; (void)a; ++freed; }
static void *t_area_surface(void *c, void *a) { (void)c; return a; }
static int t_lock(void *c, void *a, const char *who) { (void)c; (void)a; (void)who; ++locks; return 1; }
static void t_unlock(void *c, void *a, const char *who) { (void)c; (void)a; (void)who; }
static void t_clear(void *c, void *a, int32_t w, int32_t h) { (void)c; (void)a; (void)w; (void)h; }
static int shape_obj;
static void *t_shape_load(void *c, const char *f, int32_t r) { (void)c; (void)r; strcpy(last_load, f); return &shape_obj; }
static int t_shape_ok(void *c, void *s) { (void)c; (void)s; return 1; }
static int32_t t_frames(void *c, void *s) { (void)c; (void)s; return 2; }
static void t_frame(void *c, void *s, int32_t i, AoeMouseFrameRor *f) {
    (void)c; (void)s;
    f->width = i ? 20 : 32; f->height = i ? 30 : 32; f->hot_x = i ? 10 : 0; f->hot_y = i ? 15 : 0;
}
static void t_draw(void *c, void *s, void *a, int32_t x, int32_t y, int32_t fr) {
    (void)c; (void)s; (void)a; (void)x; (void)y; (void)fr; ++draws;
}
static void t_shape_free(void *c, void *s) { (void)c; (void)s; }

int main(void) {
    AoeMouseOpsRor ops = {NULL, t_time, t_windowed, t_pos, t_wrect, t_ssize, t_setpos, t_cursor,
                          t_inval, t_dirty, t_blt, t_fast, t_getclip, t_setclip, t_sync, t_area_new,
                          t_area_free, t_area_surface, t_lock, t_unlock, t_clear, t_shape_load,
                          t_shape_ok, t_frames, t_frame, t_draw, t_shape_free};
    static AoeMousePtrRor m;
    int back, primary, area, parea;
    AoeMouseHostRor host = {&area, &back, &primary, &parea, 640, 480, 0};

    assert(aoe_mouse_error_ror(0) == 0 && aoe_mouse_error_ror((int32_t)0x887600fau) == 10);
    assert(aoe_mouse_error_ror((int32_t)0x80004001u) == 17 && aoe_mouse_error_ror(5) == -1);
    assert(aoe_mouse_error_ror((int32_t)0x88760238u) == -1);

    aoe_mouse_init_ror(&m, 0, &ops);
    assert(m.last_x == -32767 && m.resource == -1 && m.last_time == 1000 - 36);

    host.gdi_only = 1;
    assert(!aoe_mouse_setup_ror(&m, 1, &host, "mcursors", 51000, 10, &ops));
    host.gdi_only = 0;
    assert(aoe_mouse_setup_ror(&m, 0, &host, "x", 0, 10, &ops) && aoe_mouse_global_ror == &m);
    aoe_mouse_init_ror(&m, 0, &ops);
    assert(!aoe_mouse_setup_ror(&m, 1, &host, "mcursors", 51000, 17, &ops));
    assert(aoe_mouse_setup_ror(&m, 1, &host, "mcursors", 51000, 10, &ops));
    assert(m.ready && cursor_state == 0 && !strcmp(last_load, "mcursors.slp") && m.resource == 51000);
    assert(areas == 11 && m.max_w == 64 && m.max_h == 48 && m.frame_count == 2 && draws == 2);
    assert(m.normal_w == 32 && m.normal_right == 32);

    aoe_mouse_set_frame_ror(&m, 1, 1, &ops);
    assert(m.game_frame == 1 && m.game_hot_x == 10 && m.game_right == 10 && m.game_below == 15);
    aoe_mouse_set_frame_ror(&m, 1, 2, &ops); /* out of range */
    assert(m.game_frame == 1);

    /* Clipped at the left and top. */
    cx = -5;
    cy = 3;
    assert(aoe_mouse_update_ror(&m, &ops) && m.on_screen);
    assert(m.dest.left == 0 && m.clip.left == 5 && m.src.right == 27 && m.dest.right == 27);
    assert(m.dest.top == 3 && m.clip.top == 0 && m.dest.bottom == 35);
    /* In the game region the game frame is used. */
    m.region_set = 1;
    aoe_mouse_set_region_on_ror(&m, 1);
    aoe_mouse_set_region_ror(&m, 0, 0, 640, 400);
    assert(aoe_mouse_in_game_ror(&m) && aoe_mouse_region_on_ror(&m));
    cx = 630;
    cy = 100;
    assert(aoe_mouse_update_ror(&m, &ops) && m.frame == 1);
    assert(m.dest.left == 620 && m.dest.right == 640 && m.dest.top == 85 && m.dest.bottom == 115);
    m.saved = 1;
    m.drawn_frame = 1;
    assert(!aoe_mouse_update_ror(&m, &ops));
    m.saved = 0;

    /* Draw: shown, forced; save then BltFast with the clipper off and back. */
    aoe_mouse_show_ror(&m, &ops);
    assert(m.visible);
    blts = fasts = clipper_sets = 0;
    m.last_x = -32767;
    aoe_mouse_draw_ror(&m, 1, &ops);
    assert(blts == 1 && fasts == 1 && fast_x == 620 && fast_y == 85 && clipper_sets == 2);
    assert(m.saved && m.saved_surface == &back && m.drawn && m.drawn_frame == 1 && invalidates == 0);
    /* Too soon in the game region and not forced: nothing. */
    aoe_mouse_draw_ror(&m, 0, &ops);
    assert(blts == 1);

    /* Poll onto the primary: restores to the back and primary, then saves. */
    now += 40;
    cx = 300;
    blts = 0;
    aoe_mouse_poll_ror(&m, &ops);
    assert(blts == 3 && m.saved_surface == &primary && last_dst_surface == &area_store[0]);
    /* A failing blit in the poll keeps the mapped error. */
    now += 40;
    cx = 310;
    fail_hr = (int32_t)0x887600fau;
    m.saved = 0;
    aoe_mouse_poll_ror(&m, &ops);
    assert(m.error == 10);
    fail_hr = 0;

    /* Restore from the back area in the game region keeps the save. */
    m.saved = 1;
    m.saved_surface = &back;
    blts = 0;
    aoe_mouse_restore_ror(&m, &ops);
    assert(blts == 1 && m.saved && m.saved_surface == &primary);
    m.saved_surface = &back;
    aoe_mouse_hide_ror(&m, &ops);
    assert(!m.visible && !m.saved && !m.drawn && dirties == 1 && cursor_state == 0);

    assert(aoe_mouse_detach_ror(&m) && m.detached && !m.host);
    assert(!aoe_mouse_detach_ror(&m));
    assert(aoe_mouse_attach_ror(&m, &host, &ops) && !m.detached && m.draw_surface == &back);
    assert(!strcmp(last_load, "mcursors.slp"));

    aoe_mouse_center_ror(&ops);
    assert(set_x == 320 && set_y == 240);
    windowed_flag = 1;
    aoe_mouse_center_ror(&ops);
    assert(set_x == 60 && set_y == 120);
    cx = 0xffff;
    aoe_mouse_update_ror(&m, &ops);
    assert(m.x == -1);

    aoe_mouse_destroy_ror(&m, &ops);
    assert(freed == 11 && aoe_mouse_global_ror == NULL && m.frame_count == 0);
    return 0;
}
