/* Patched RoR 1.0a 0x00512400 / 0x00513ea0. */
#include "view_terrain_pass.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char lg[8192];
static void add(const char *fmt, ...) { va_list ap; size_t n = strlen(lg); va_start(ap, fmt); vsnprintf(lg + n, sizeof lg - n, fmt, ap); va_end(ap); }
static int count(const char *s) { int n = 0; const char *p = lg; size_t l = strlen(s); while ((p = strstr(p, s))) { ++n; p += l; } return n; }

/* A 4 x 3 map: tile (x, y) at screen (10 x, 10 y). */
static AoeViewTileRor tiles[3][4];
static uint32_t vis[3][4];
static int blits, mouse, q_state = 4, q_edit, q_in_view = 1, q_pixel = 1, q_id = 7, q_cursor = 1;
static uint32_t now = 1000;

static void step(void *c, int id, int32_t a, int32_t b, int32_t d) { (void)c; add("s%d(%d,%d,%d) ", id, (int)a, (int)b, (int)d); }
static void tile(void *c, int32_t x, int32_t y, AoeViewTileRor *t) { (void)c; *t = tiles[y][x]; }
static void tile_store(void *c, int32_t x, int32_t y, const AoeViewTileRor *t) { (void)c; tiles[y][x] = *t; }
static uint32_t visibility(void *c, int32_t x, int32_t y) { (void)c; return vis[y][x]; }
static int16_t shape_height(void *c, uint8_t s) { (void)c; return (int16_t)(10 + s); }
static uint32_t neighbor(void *c, int32_t x, int32_t y, uint32_t m) { (void)c; add("nb(%d,%d,%x) ", (int)x, (int)y, (unsigned)m); return (uint32_t)(x + y) & 3; }
static int16_t repl(void *c, uint8_t t) { (void)c; return t == 3 ? 9 : -1; }
static void counted(void *c, uint8_t t) { (void)c; add("cnt%d ", t); }
static void uncounted(void *c) { (void)c; add("uncnt "); }
static const void *outline(void *c, uint8_t s, uint32_t e, int w) { (void)c; (void)s; return (e == 2 && w == 1) ? NULL : (const void *)(size_t)(0x100 + e * 2 + w); }
static const void *edge_outline(void *c, uint8_t s, uint32_t e) { (void)c; (void)s; return (const void *)(size_t)(0x200 + e); }
static void radd(void *c, const void *ol, int32_t x, int32_t y) { (void)c; add("add%x(%d,%d) ", (unsigned)(size_t)ol, (int)x, (int)y); }
static void rrem(void *c, const void *ol, int32_t x, int32_t y) { (void)c; add("rem%x(%d,%d) ", (unsigned)(size_t)ol, (int)x, (int)y); }
static void draw_tile(void *c, int32_t x, int32_t y, uint8_t id, int32_t sx, int32_t sy, uint8_t fog, int a, int b) { (void)c; add("tile(%d,%d,%d,%d,%d,%x,%d,%d) ", (int)x, (int)y, id, (int)sx, (int)sy, fog, a, b); }
static void draw_objects(void *c, int32_t sx, int32_t sy, int f) { (void)c; add("obj(%d,%d,%d) ", (int)sx, (int)sy, f); }
static int mouse_global(void *c) { (void)c; return mouse; }
static int blits_active(void *c) { (void)c; if (blits > 0) { --blits; return 1; } return 0; }
static int32_t game_state(void *c) { (void)c; return q_state; }
static int32_t edit_mode(void *c) { (void)c; return q_edit; }
static int yes(void *c) { (void)c; return 1; }
static uint32_t time_ms(void *c) { (void)c; return now; }
static int32_t frame_count(void *c, int32_t s) { (void)c; (void)s; return 3; }
static void draw_mark(void *c, const AoeViewMarkRor *m) { (void)c; add("mark(%d,%d) ", (int)m->shape, (int)m->field_c); }
static void free_mark(void *c, AoeViewMarkRor *m) { (void)c; add("free(%d) ", (int)m->shape); }
static int cursor_in_view(void *c, int32_t dx, int32_t dy, int32_t *x, int32_t *y) { (void)c; *x = 5 + dx; *y = 6 + dy; return q_in_view; }
static int pixel_object(void *c, int32_t x, int32_t y, int32_t *id) { (void)c; add("pix(%d,%d) ", (int)x, (int)y); *id = q_id; return q_pixel; }
static int object_cursor(void *c, int32_t id, int32_t *f, int32_t *s) { (void)c; *f = 20 + id; *s = 300 + id; return q_cursor; }

static AoeViewTerrainPassOpsRor ops = {NULL, step, tile, tile_store, visibility, shape_height, neighbor, repl, counted, uncounted, outline, edge_outline, radd, rrem, draw_tile, draw_objects, mouse_global, blits_active, game_state, edit_mode, yes, yes, yes, time_ms, frame_count, draw_mark, free_mark, cursor_in_view, pixel_object, object_cursor};

static void reset_map(void) {
    int x, y;
    memset(tiles, 0, sizeof tiles);
    for (y = 0; y < 3; ++y) for (x = 0; x < 4; ++x) { tiles[y][x].x = (int16_t)(10 * x); tiles[y][x].y = (int16_t)(10 * y); tiles[y][x].shape = (uint8_t)x; tiles[y][x].terrain = (uint8_t)(x | (y << 5)); vis[y][x] = 3; }
}

/* 0x00513ea0 recorder. */
static int t_enabled = 1, t_border_enabled = 1, t_per_edge, t_frame = 4, t_bframe = 6, t_frames_ok = 1, sync_e, sync_v;
static uint8_t t_draws_terrain = 1;
static int te(void *c, uint8_t t) { (void)c; (void)t; return t_enabled; }
static const void *ts(void *c, uint8_t t) { (void)c; (void)t; return "T"; }
static int be(void *c, uint8_t t) { (void)c; (void)t; return t_border_enabled; }
static const void *bs(void *c, uint8_t t) { (void)c; (void)t; return "B"; }
static int bpe(void *c, uint8_t t) { (void)c; (void)t; return t_per_edge; }
static uint8_t bdt(void *c, uint8_t t) { (void)c; (void)t; return t_draws_terrain; }
static int16_t bf(void *c, uint8_t t, uint8_t s, uint8_t bsh, int16_t x, int16_t y) { (void)c; add("bf(%d,%d,%d,%d,%d) ", t, s, bsh, x, y); return (int16_t)t_bframe; }
static int bfs(void *c, uint8_t t, uint8_t s, uint8_t bsh, int16_t x, int16_t y, int32_t out[4]) { (void)c; (void)t; (void)s; (void)bsh; (void)x; (void)y; out[0] = 11; out[1] = -1; out[2] = 13; out[3] = 14; return t_frames_ok; }
static int16_t tf(void *c, uint8_t t, uint8_t s, int16_t x, int16_t y) { (void)c; (void)s; (void)x; (void)y; add("tf%d ", t); return (int16_t)t_frame; }
static uint32_t tvis(void *c, int16_t x, int16_t y) { (void)c; return vis[y][x]; }
static void blit(void *c, int32_t sx, int32_t sy, const void *s, int32_t f, uint8_t fog, uint8_t fl, int a, int b) { (void)c; add("blit(%d,%d,%s,%d,%x,%x,%d,%d) ", (int)sx, (int)sy, (const char *)s, (int)f, fog, fl, a, b); }
static void dirty(void *c, int16_t x, int16_t y) { (void)c; add("dirty(%d,%d) ", x, y); }
static AoeViewTileDrawOpsRor tops = {NULL, 8, te, ts, be, bs, bpe, bdt, bf, bfs, tf, 0, 7, tvis, 1, 2, &sync_e, &sync_v, blit, dirty, 3};

/* 0x005118c0 recorder: a 4 x 3 map, tiles 10 wide, half 5 x 4, step 3. */
static uint8_t fogv[3][4];
static void wstep(void *c, int id, int32_t x, int32_t y, int32_t a, int32_t b, int32_t d) { (void)c; add("w%d(%d,%d,%d,%d,%d) ", id, (int)x, (int)y, (int)a, (int)b, (int)d); }
static void *tile_ref(void *c, int32_t x, int32_t y) { (void)c; return &tiles[y][x]; }
static uint8_t fog_value(void *c, int32_t x, int32_t y) { (void)c; return fogv[y][x]; }
static void shape_size(void *c, uint8_t s, int16_t *w, int16_t *h) { (void)c; *w = 10; *h = (int16_t)(10 + s); }
static int area_q(void *c, int32_t out[4]) { (void)c; out[0] = 1; out[1] = 0; out[2] = 2; out[3] = 1; return 1; }
static int objs[4][4]; /* kind per tile; 0 none */
static void *pick_object(void *c, void *tile, int32_t sx, int32_t sy, int32_t x, int32_t y, const int16_t *pt, void *arg, int16_t *osx, int16_t *osy, void *pref, int fogged) {
    (void)c; (void)tile; (void)pt; (void)arg; add("pk(%d,%d,%d,%d,%d,%d) ", (int)x, (int)y, (int)sx, (int)sy, pref ? 1 : 0, fogged);
    if (!objs[y][x]) return NULL; *osx = (int16_t)sx; *osy = (int16_t)sy; return &objs[y][x]; }
static uint8_t object_kind(void *c, void *obj) { (void)c; return (uint8_t)*(int *)obj; }
static void object_point(void *c, void *obj, float *x, float *y) { (void)c; *x = (float)*(int *)obj; *y = 0.5f; }
static int terrain_hit(void *c, void *tile, int32_t sx, int32_t sy, int32_t x, int32_t y, const int16_t *pt) { (void)c; (void)tile; (void)sy; (void)x; (void)y; return pt[0] >= sx && pt[0] < sx + 10; }
static void elevation_point(void *c, int32_t dx, int32_t dy, void *tile, float *x, float *y) { (void)c; (void)tile; *x = dx / 10.0f; *y = dy / 10.0f; }
static AoeViewWalkOpsRor wops = {NULL, wstep, tile, tile_store, tile_ref, fog_value, shape_size, repl, counted, uncounted, area_q, pick_object, object_kind, object_point, terrain_hit, elevation_point};

static void walk_tests(void) {
    AoeViewTerrainPassRor v;
    void *res; float ox = 0, oy = 0; int16_t osx = 0, osy = 0, pt[3] = {25, 0, 7};
    int x, y;
    memset(&v, 0, sizeof v); reset_map();
    for (y = 0; y < 3; ++y) for (x = 0; x < 4; ++x) fogv[y][x] = 0x0f;
    v.start_x = 0; v.start_y = 0; v.cols = -2; v.rows = -5; /* 2 tiles per row, 2 rows */
    v.map_w = 4; v.map_h = 3; v.terrain_count = 8; v.fog_edges = 1; v.has_visibility = 1;
    v.tile_w = 10; v.half_w = 5; v.half_h = 4; v.elevation_step = 3; v.scroll_x = 20; v.scroll_y = 8;
    v.clip[0] = -100; v.clip[1] = -100; v.clip[2] = 100; v.clip[3] = 100; v.count_terrains = 1;
    /* Mode 10 counts the terrains of the walked tiles (0, 0), (1, 1), then row 1 from (0, 1): (0, 1), (1, 2); (1, 1) is fogged out and skipped. */
    fogv[1][1] = 0; lg[0] = '\0';
    assert(aoe_main_view_walk_ror(&v, &wops, 10, 2, pt, NULL, &res, &ox, &oy, &osx, &osy) == '2' && v.mode == 10 && v.min_kind == 2);
    assert(strcmp(lg, "uncnt w0(0,0,0,0,0) cnt0 cnt0 cnt1 w1(0,0,0,0,0) ") == 0);
    /* Mode 0x1e draws the objects of every visible tile at the screen position (the fogged tile forced visible). */
    lg[0] = '\0';
    assert(aoe_main_view_walk_ror(&v, &wops, 0x1e, 0, pt, NULL, &res, &ox, &oy, &osx, &osy) == '2');
    assert(strcmp(lg, "w0(0,0,0,0,0) w2(0,0,20,8,0) w2(1,1,30,8,0) w2(0,1,25,12,0) w2(1,2,35,12,0) ") == 0);
    /* '(' picks the terrain under x = 25: tile (1, 1) at sx 30 misses, (0, 1) at 25 hits (elevation shape 1: y - 3). */
    tiles[1][0].terrain = 1 << 5; lg[0] = '\0'; res = (void *)1;
    assert(aoe_main_view_walk_ror(&v, &wops, '(', 0, pt, NULL, &res, &ox, &oy, &osx, &osy) == '3');
    assert(res == &tiles[1][0] && osx == 25 && osy == 9 && ox == 0.0f && oy == 1.0f + (7 - 9) / 10.0f);
    /* ')' picks the object of the highest kind; the preferred object met again returns at once. */
    objs[0][0] = 2; objs[1][1] = 3; objs[2][1] = 1; fogv[1][1] = 0x80; lg[0] = '\0'; res = NULL;
    assert(aoe_main_view_walk_ror(&v, &wops, ')', 0, pt, NULL, &res, &ox, &oy, &osx, &osy) == '4' && res == &objs[1][1] && ox == 3.0f);
    assert(strcmp(lg, "w0(0,0,0,0,0) pk(0,0,20,8,0,0) pk(1,1,30,8,0,1) pk(0,1,25,12,0,0) pk(1,2,35,12,0,0) ") == 0);
    res = &objs[2][1]; lg[0] = '\0';
    assert(aoe_main_view_walk_ror(&v, &wops, ')', 0, pt, NULL, &res, &ox, &oy, &osx, &osy) == '4' && res == &objs[1][1]);
    assert(strcmp(lg, "w0(0,0,0,0,0) pk(0,0,20,8,1,0) pk(1,1,30,8,1,1) pk(0,1,25,12,1,0) pk(1,2,35,12,1,0) ") == 0);
    /* '<' finds the tile at the given map point; mode 14 keeps to the area. */
    ox = 1.0f; oy = 2.0f; lg[0] = '\0';
    assert(aoe_main_view_walk_ror(&v, &wops, '<', 0, pt, NULL, &res, &ox, &oy, &osx, &osy) == '3' && res == &tiles[2][1] && osx == 35 && osy == 12 - 2 * 3);
    lg[0] = '\0';
    assert(aoe_main_view_walk_ror(&v, &wops, 14, 0, pt, NULL, &res, &ox, &oy, &osx, &osy) == '2' && strcmp(lg, "w0(0,0,0,0,0) ") == 0);
}

int main(void) {
    AoeViewTerrainPassRor v;
    AoeViewMarkRor m1, m2;
    AoeViewTileRor t;
    memset(&v, 0, sizeof v);
    reset_map();
    v.start_x = 1; v.start_y = 0; v.cols = -3; v.rows = -5; /* 2 tiles per row, 2 rows */
    v.map_w = 4; v.map_h = 3; v.terrain_count = 8; v.fog_edges = 1; v.has_visibility = 1;
    v.explored_mask = 1; v.visible_mask = 2; v.tile_w = 10; v.half_h = 4;
    v.clip[0] = -100; v.clip[1] = -100; v.clip[2] = 100; v.clip[3] = 100; v.count_terrains = 1;
    v.timers_active = 1; mouse = 1; blits = 1;
    /* Row 0 from (1, 0): (1, 0) visible, (2, 1) explored only; row 1 steps x back to (0, 0): (0, 0) visible, (1, 1) unseen. */
    vis[1][2] = 1; vis[1][1] = 0; vis[2][2] = 0; v.hover_messages = 1;
    lg[0] = '\0';
    assert(aoe_game_view_terrain_pass_ror(&v, &ops, 10, 3, 4) == 0 && v.mode == 10);
    assert(strncmp(lg, "s0(0,0,0) uncnt s3(0,0,0) cnt1 nb(1,0,2) nb(1,0,1) s7(0,0,0) add102(10,0) add103(10,0) rem201(10,0) rem201(10,0) tile(1,0,1,10,0,f,1,1) s8(0,0,0) s5(1,0,0) obj(10,0,0) s4(0,40,0) s6(0,0,0) s5(0,0,0) ", 30) == 0);
    assert(count("tile(") == 3 && strstr(lg, "cnt2 nb(2,1,1) add100(20,10) rem203(20,10) tile(2,1,2,20,10,80,0,1) s8(0,0,0) s5(1,0,0) s6(1,0,0) obj(20,14,1) ") && count("obj(") == 4);
    /* The unseen tile: only its objects, in rows 0..5 and fogged. The blit step between the rows. */
    assert(strstr(lg, "s2(0,0,0) cnt0 nb(0,0,2) ") && strstr(lg, "s5(1,0,0) s4(0,5,0) s6(1,0,0) obj(10,14,0) s4(0,40,0) ") && tiles[1][1].last_fog == 0);
    assert(tiles[0][1].last_fog == 0x0f && tiles[0][1].edge_visible == 1 && tiles[0][1].edge_explored == 1 && tiles[1][2].last_fog == 0x80 && tiles[1][2].edge_explored == 3);
    /* The tail in order; the hovered object 7 gets cursor 27 and message 307. */
    assert(strstr(lg, "s1(0,0,0) s5(1,0,0) s5(0,0,0) s13(0,0,0) s14(0,0,0) s15(0,0,0) s1(0,0,0) pix(2,2) s27(27,0,0) s28(307,0,0) s16(3,4,0) s17(0,0,0) s3(0,0,0) s21(3,4,0) s22(0,0,0) s23(0,0,0) s24(0,0,0) s25(0,0,0) s26(0,0,0) "));
    assert(v.hover_on == 1 && v.hover_id == 7 && v.hover_time == 1000);
    /* Incremental: unchanged tiles are skipped unless the highlight covers them (region 1); a dirty flag redraws with both passes. */
    v.incremental = 1; v.highlight_on = 1; v.rect1[0] = 10; v.rect1[1] = 0; v.rect1[2] = 15; v.rect1[3] = 5; v.rect2[0] = -1;
    v.timers_active = 0; mouse = 0; v.drag_kind = 2; q_edit = 7; tiles[1][2].flags |= 0x40;
    m1.shape = 5; m1.kind = 2; m1.frame = 500; m1.interval = 100; m1.field_c = 1; m1.next = &m2; m1.prev = NULL;
    m2.shape = 6; m2.kind = 1; m2.frame = 0; m2.interval = 100; m2.field_c = 0; m2.next = NULL; m2.prev = &m1;
    v.marks = &m1; q_pixel = 0;
    lg[0] = '\0';
    aoe_game_view_terrain_pass_ror(&v, &ops, 11, 0, 0);
    assert(strstr(lg, "s7(1,0,0) add102(10,0) ") && strstr(lg, "tile(1,0,1,10,0,f,1,1) ") && strstr(lg, "tile(0,0,0,0,0,f,1,1) ") && count("tile(") == 3);
    assert(strstr(lg, "tile(2,1,2,20,10,80,1,1) ") && !strstr(lg, "tile(2,1,2,20,10,80,0,1) ") && (tiles[1][2].flags & 0x40) == 0);
    /* The drag box, the preview, the kind-2 mark wrapping twice (freed), the kind-1 mark started and drawn, 0x00515ff0. */
    assert(strstr(lg, "s9(0,0,0) s10(0,0,0) free(5) mark(6,0) s12(0,0,0) s5(0,0,0) s13(0,0,0) s14(0,0,0) s7(0,0,0) s15(0,0,0) ") && v.marks == &m2 && m2.prev == NULL && m2.frame == 1000);
    assert(strstr(lg, "pix(5,6) s27(0,0,0) s29(0,0,0) s16(0,0,0) s18(0,0,0) s19(10,15,0) s19(10,15,1) s19(10,15,2) s19(10,15,3) s19(10,15,4) s19(10,15,5) s20(0,0,0) s21(0,0,0) ") && v.hover_on == 0 && v.hover_id == INT32_MIN);
    /* The tile drawer. */
    memset(&t, 0, sizeof t); t.shape = 2; t.border = 0x31; t.flags = 0; lg[0] = '\0';
    assert(aoe_game_view_draw_tile_ror(&tops, &t, 9, 100, 50, 1, 1, 0, 1, 1) == 0 && lg[0] == '\0'); /* unknown terrain */
    lg[0] = '\0'; t_draws_terrain = 0;
    assert(aoe_game_view_draw_tile_ror(&tops, &t, 1, 100, 50, 1, 1, 0x0f, 1, 1) == 0 && strcmp(lg, "bf(1,2,3,1,1) bf(1,2,3,1,1) blit(100,50,B,6,f,0,1,1) dirty(0,1) dirty(1,2) ") == 0);
    lg[0] = '\0'; t_draws_terrain = 1; t_per_edge = 1; vis[1][1] = 0;
    assert(aoe_game_view_draw_tile_ror(&tops, &t, 1, 100, 50, 1, 1, 0x80, 0, 1) == 1 && sync_e == 1 && sync_v == 0 && strcmp(lg, "tf1 blit(100,50,T,4,80,0,0,1) blit(100,50,B,11,80,0,0,1) blit(100,50,B,13,80,0,0,1) blit(100,50,B,14,80,0,0,1) dirty(0,1) dirty(1,2) ") == 0);
    lg[0] = '\0'; t.border = 0; t.flags = 0x40;
    assert(aoe_game_view_draw_tile_ror(&tops, &t, 1, 100, 50, 2, 2, 0, 1, 1) == 1 && strcmp(lg, "tf1 blit(100,50,T,4,0,40,1,1) dirty(1,2) ") == 0);
    walk_tests();
    return 0;
}
