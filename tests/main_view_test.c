#include "main_view.h"
#include "le_bytes.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int redraws, last_level, layouts, attached, base_mode, base_value;
    int flag, notified, destroyed, highlight[4], state, edit_mode;
    void *last_panel, *last_area;
    char world_marker;
} Fake;

static Fake f;
static void redraw(void *c, void *p, int32_t l) { (void)c; f.redraws++; f.last_level = l; f.last_panel = p; }
static void layout(void *c) { (void)c; f.layouts++; }
static void attached(void *c, void *m) { (void)c; (void)m; f.attached++; }
static void *world(void *c, void *m) { (void)c; (void)m; return &f.world_marker; }
static void geometry(void *c, void *w, int16_t g[5])
{
    (void)c; assert(w == &f.world_marker);
    for (int i = 0; i < 5; i++) g[i] = (int16_t)(64 - i);
}
static void base_mode(void *c, int32_t m) { (void)c; f.base_mode = m; }
static void base_handler(void *c, int32_t v) { (void)c; f.base_value = v; }
static int gflag(void *c) { (void)c; return f.flag; }
static void notify(void *c, void *a) { (void)c; f.notified++; f.last_area = a; }
static void destroy(void *c, void *o) { (void)c; (void)o; f.destroyed++; }
static void highlight(void *c, int32_t a, int32_t b, int32_t d, int32_t e)
{ (void)c; f.highlight[0] = a; f.highlight[1] = b; f.highlight[2] = d; f.highlight[3] = e; }
static int32_t gstate(void *c) { (void)c; return f.state; }
static int32_t gmode(void *c) { (void)c; return f.edit_mode; }

static int busy, blits, last_blit[6];
static int surface_busy(void *c) { (void)c; return busy; }
static void surface_blit(void *c, int32_t x, int32_t y, const int32_t s[4])
{
    (void)c; blits++; last_blit[0] = x; last_blit[1] = y;
    memcpy(&last_blit[2], s, 4 * sizeof(int32_t));
}
static int32_t cur_x, cur_y; static int win;
static void cursor_pos(void *c, int32_t *x, int32_t *y) { (void)c; *x = cur_x; *y = cur_y; }
static int windowed(void *c) { (void)c; return win; }
static void to_client(void *c, int32_t *x, int32_t *y) { (void)c; *x -= 0x10010; *y -= 5; }
static int lock_ok, locks, unlocks, regions, codes, seen_left, seen_scroll;
static int32_t region[4]; static AoeMainViewRor *drawing;
static int lock_surface(void *c, void *s, const char *n)
{ (void)c; (void)s; assert(strcmp(n, "view::draw_view") == 0); locks++; return lock_ok; }
static void unlock_surface(void *c, void *s, const char *n) { (void)c; (void)s; (void)n; unlocks++; }
static void draw_region(void *c, int32_t l, int32_t t, int32_t r, int32_t b)
{
    (void)c; regions++; region[0] = l; region[1] = t; region[2] = r; region[3] = b;
    seen_left = drawing->left; seen_scroll = drawing->scroll_x;
}
static void draw_code(void *c, uint8_t code) { (void)c; codes = code; }
static uint8_t pick_result;
static uint8_t pick(void *c, uint8_t kind, int32_t arg, int32_t a[2], int32_t b[2],
                    int32_t *value, int32_t *o0, int32_t *o1, int16_t *h0, int16_t *h1)
{
    (void)c; (void)kind; assert(arg == 9);
    assert(a[0] == 11 && a[1] == 12 && b[0] == 11 && b[1] == 12);
    *o0 = 1; *o1 = 2; *h0 = 3; *h1 = 4;
    *value += 100;
    return pick_result;
}

static uint32_t clock_ms; static int recorded_slot; static uint32_t recorded_ms;
static uint32_t time_ms(void *c) { (void)c; clock_ms += 40; return clock_ms; }
static int32_t base_draw(void *c, int32_t a, int32_t b, int32_t d, int32_t e, int32_t g)
{ (void)c; return a + b + d + e + g; }
static void record_time(void *c, int32_t slot, uint32_t ms) { (void)c; recorded_slot = slot; recorded_ms = ms; }

static const AoeMainViewOpsRor ops = {
    NULL, redraw, layout, attached, world, geometry, base_mode, base_handler,
    gflag, notify, destroy, highlight, gstate, gmode,
    surface_busy, surface_blit, cursor_pos, windowed, to_client,
    lock_surface, unlock_surface, draw_region, draw_code, pick,
    time_ms, base_draw, record_time,
};

/* ---- batch: lifecycle, grid, helpers ---- */
static int life_log[64], life_n, base_ctor, base_dtor, deleted, overlay_freed, marks_freed, gclear, lflag;
static char shape_name_seen[64];
static int32_t pen_args[3];
static int tokens[32];
static void l_base_construct(void *c, const int32_t a[7]) { (void)c; assert(a[6] == 7); base_ctor++; }
static void l_base_destruct(void *c) { (void)c; base_dtor++; }
static void *l_stock(void *c, int32_t w) { (void)c; return &tokens[w]; }
static void *l_pen(void *c, int32_t s, int32_t w, uint32_t col)
{ (void)c; pen_args[0] = s; pen_args[1] = w; pen_args[2] = (int32_t)col; return &tokens[10]; }
static void *l_shape(void *c, const char *n) { (void)c; strcpy(shape_name_seen, n); return &tokens[11]; }
static int edges_init;
static void l_edges(void *c) { (void)c; edges_init++; }
static void *l_alloc(void *c, uint32_t n, uint32_t s) { (void)c; assert(n == 0x40 && s == 0x14); return &tokens[12]; }
static void l_release(void *c, AoeMainViewResourceRor w, void *r) { (void)c; (void)r; life_log[life_n++] = (int)w; }
static void l_overlay(void *c) { (void)c; overlay_freed = life_n; }
static void l_marks(void *c) { (void)c; marks_freed = life_n; }
static int l_gflag(void *c) { (void)c; return lflag; }
static void l_gclear(void *c) { (void)c; gclear = life_n; }
static void l_delete(void *c, void *s) { (void)c; (void)s; deleted++; }

static int lookup_built;
static void build_lookup(AoeViewGridRor *g) { lookup_built = g->col_count; }

static uint8_t node_flags;
static int find_hit, find_args[4];
static const uint8_t *find_node(void *c, int32_t id, int32_t y0, int32_t y1, int32_t key)
{ (void)c; find_args[0] = id; find_args[1] = y0; find_args[2] = y1; find_args[3] = key; return find_hit ? &node_flags : NULL; }

static char pass_log[4]; static int pass_n;
static void p1(void *c) { (void)c; pass_log[pass_n++] = '1'; }
static void p2(void *c) { (void)c; pass_log[pass_n++] = '2'; }
static void p3(void *c) { (void)c; pass_log[pass_n++] = 'b'; }

static const int32_t *stream_data; static uint32_t stream_pos;
static void stream_read(void *c, void *b, uint32_t n)
{ (void)c; memcpy(b, (const uint8_t *)stream_data + stream_pos, n); stream_pos += n; }

static void test_batch_lifecycle(void)
{
    AoeMainViewLifeOpsRor ops = {0};
    AoeMainViewLifeRor v;
    int32_t args[7] = {1, 2, 3, 4, 5, 6, 7};
    ops.base_construct = l_base_construct; ops.base_destruct = l_base_destruct;
    ops.stock_object = l_stock; ops.create_pen = l_pen; ops.load_shape = l_shape;
    ops.init_edges = l_edges; ops.alloc_zero = l_alloc; ops.release = l_release;
    ops.free_overlay = l_overlay; ops.free_marks = l_marks; ops.global_flag = l_gflag;
    ops.clear_globals = l_gclear; ops.delete_self = l_delete;

    /* 0x0050f420 */
    assert(aoe_main_view_construct_ror(&v, &ops, args, 99, "cursor") == 1);
    assert(base_ctor == 1 && v.owner_arg == 99 && v.point_x == -10000.0f && v.point_y == -10000.0f);
    assert(v.white_pen == &tokens[6] && v.null_brush == &tokens[5]);
    assert(pen_args[0] == 0 && pen_args[1] == 1 && pen_args[2] == 0xff);
    assert(strcmp(shape_name_seen, "cursor.shp") == 0 && edges_init == 1);
    assert(v.res[AOE_MAIN_VIEW_RES_LIST0 + 4] == &tokens[12] && v.list_count[4] == 0);

    /* 0x0050f240 / 0x0050f220 */
    v.res[AOE_MAIN_VIEW_RES_DRAW_ROWS1] = &tokens[13];
    v.res[AOE_MAIN_VIEW_RES_TIMERS] = &tokens[14];
    lflag = 1;
    assert(aoe_main_view_delete_ror(&v, &ops, 1) == &v);
    assert(life_log[0] == AOE_MAIN_VIEW_RES_SHAPE && overlay_freed == 1);
    assert(life_log[1] == AOE_MAIN_VIEW_RES_PEN && life_log[2] == AOE_MAIN_VIEW_RES_DRAW_ROWS1);
    assert(life_log[3] == AOE_MAIN_VIEW_RES_LIST0 && gclear == 8);
    assert(life_log[8] == AOE_MAIN_VIEW_RES_TIMERS && marks_freed == 9);
    assert(base_dtor == 2 && deleted == 1);
    assert(v.res[AOE_MAIN_VIEW_RES_SHAPE] == NULL && v.res[AOE_MAIN_VIEW_RES_PEN] == NULL);
    lflag = 0; gclear = -1;
    aoe_main_view_delete_ror(&v, &ops, 0);
    assert(deleted == 1 && gclear == -1 && base_dtor == 4);

    /* 0x00516800 / 0x005168d0 */
    {
        AoeViewGridRor g; AoeViewGridGlobalsRor gl;
        memset(&gl, 0, sizeof gl); gl.used[2] = 9;
        aoe_view_grid_init_ror(&g, &gl, 3, 4, build_lookup);
        assert(g.row_count == 3 && g.col_count == 5 && lookup_built == 5);
        assert(g.rows[2][4].capacity == 0x20 && g.rows[2][4].count == 0 && g.rows[2][4].items);
        assert(gl.buffers[2] == g.buffers[2] && gl.used[2] == 0 && gl.current == &g);
        aoe_view_grid_free_ror(&g, &gl);
        assert(gl.current == NULL && gl.buffers[0] == NULL);
    }

    /* 0x005160d0 */
    find_hit = 0;
    assert(aoe_game_view_has_draw_node_ror(find_node, NULL, 77, 1) == 0);
    assert(find_args[0] == 77 && find_args[1] == 6 && find_args[2] == 0x28 && find_args[3] == 0);
    find_hit = 1; node_flags = 1;
    assert(aoe_game_view_has_draw_node_ror(find_node, NULL, 77, 0) == 0);
    assert(aoe_game_view_has_draw_node_ror(find_node, NULL, 77, 1) == 1);
    node_flags = 2;
    assert(aoe_game_view_has_draw_node_ror(find_node, NULL, 77, 0) == 1);

    /* 0x00517820 */
    {
        const void *a[17], *b[17];
        for (int i = 0; i < 17; i++) { a[i] = &tokens[i]; b[i] = &tokens[i + 1]; }
        assert(aoe_view_footprint_table_ror(a, b, -3, 0) == &tokens[0]);
        assert(aoe_view_footprint_table_ror(a, b, 40, 0) == &tokens[16]);
        assert(aoe_view_footprint_table_ror(a, b, 5, 1) == &tokens[6]);
    }

    /* 0x00518700 */
    aoe_game_view_draw_pass_ror(NULL, p1, p2, p3);
    assert(pass_n == 3 && memcmp(pass_log, "12b", 3) == 0);

    /* 0x00517e90 */
    {
        static const int32_t s1[] = {2, 3, 1, 2, 3, 4, 5, 6};
        static const int32_t s2[] = {0, 3};
        AoeIntGridRor g = {0, 0, NULL};
        int32_t *cells;
        stream_data = s1; stream_pos = 0;
        aoe_int_grid_read_ror(&g, stream_read, NULL);
        assert(g.width == 2 && g.height == 3 && g.cells[5] == 6 && stream_pos == 32);
        cells = g.cells;
        stream_data = s1; stream_pos = 0;
        aoe_int_grid_read_ror(&g, stream_read, NULL);
        assert(g.cells == cells);
        stream_data = s2; stream_pos = 0;
        aoe_int_grid_read_ror(&g, stream_read, NULL);
        assert(g.width == 0 && g.height == 0 && g.cells == NULL && stream_pos == 8);
        free(cells);
    }
}

/* ---- batch 2 ---- */
static int ld_excl(void *c, int32_t p) { (void)c; return p == 2; }
static int32_t ld_score(void *c, int32_t p, int32_t cat) { (void)c; assert(cat == 9); static const int32_t sc[] = {0, 5, 99, 7, 7}; return sc[p]; }
static uint8_t ld_colour(void *c, int32_t p) { (void)c; return (uint8_t)(40 + p); }
static const char *ld_name(void *c, int32_t p) { (void)c; return p == 3 ? "Cleopatra" : "x"; }
static uint8_t ld_stance(void *c, int32_t a, int32_t b) { (void)c; return (a == 4 && b == 3) ? 1 : 0; }

static char rlog[128]; static int has_map_v, dbg_v; static uint32_t clock_v; static int32_t stat_v;
static uint32_t rec[16];
static uint32_t r_time(void *c) { (void)c; clock_v += 100; return clock_v; }
static int32_t r_stat(void *c, int32_t s) { (void)c; assert(s == 11); stat_v += 30; return stat_v; }
static void r_record(void *c, int32_t s, uint32_t ms) { (void)c; rec[s] = ms; }
static void r_adv(void *c) { (void)c; strcat(rlog, "A"); }
static int r_has(void *c) { (void)c; return has_map_v; }
static void r_begin(void *c, int32_t a) { (void)c; assert(a == 0); strcat(rlog, "B"); }
static void r_clear(void *c) { (void)c; strcat(rlog, "C"); }
static void r_end(void *c) { (void)c; strcat(rlog, "E"); }
static int r_dbg(void *c) { (void)c; return dbg_v; }
static void r_log(void *c, const char *t) { (void)c; strcat(rlog, t); }
static void r_render(void *c) { (void)c; strcat(rlog, "R"); }

static void test_batch2(void)
{
    /* 0x005116c0 */
    assert(aoe_main_view_unit_cursor_ror(2, 1, 0) == 5);
    assert(aoe_main_view_unit_cursor_ror(2, 0, 2) == 3);
    assert(aoe_main_view_unit_cursor_ror(2, 0, 3) == 2);
    assert(aoe_main_view_unit_cursor_ror(2, 0, 0) == 0);
    assert(aoe_main_view_unit_cursor_ror(3, 0, 1) == 3);
    assert(aoe_main_view_unit_cursor_ror(3, 0, 3) == 0);
    assert(aoe_main_view_unit_cursor_ror(4, 0, 1) == 4);
    assert(aoe_main_view_unit_cursor_ror(4, 0, 3) == 3);
    assert(aoe_main_view_unit_cursor_ror(4, 0, 0) == 2);
    assert(aoe_main_view_unit_cursor_ror(4, 1, 0) == 5);
    assert(aoe_main_view_unit_cursor_ror(9, 1, 1) == 1);

    /* 0x005176b0 */
    {
        int8_t c0[3], c1[3]; int8_t *rows[2] = {c0, c1};
        uint32_t t0[3] = {0xff, 0xff, 0xff}, t1[3] = {0xff, 0xff, 0xff}; uint32_t *tr[2] = {t0, t1};
        AoeVisibilityFillRor m = {rows, 3, 2, 5, 6, 0x1, ~0x1u, 0x100, ~0x100u, tr};
        aoe_visibility_fill_ror(&m, 0);
        assert(c1[2] == 0 && t1[2] == 0xfe && m.explored_count == 6);
        aoe_visibility_fill_ror(&m, 2);
        assert(c0[0] == 2 && t0[0] == 0x1ff);
        aoe_visibility_fill_ror(&m, -1);
        assert(c0[1] == -1 && t0[1] == 0xfe && m.explored_count == 0);
    }

    /* 0x0050cdf0 */
    {
        AoeLeaderOpsRor o = {NULL, ld_excl, ld_score, ld_colour, ld_name, ld_stance};
        AoeLeaderRor r;
        aoe_world_leader_ror(&o, 5, 9, &r);
        assert(r.score == 7 && r.colour == 43 && strcmp(r.name, "Cleopatra") == 0);
        assert(r.allies == 2); /* 1 and 2; 4 is hostile toward 3 */
        aoe_world_leader_ror(&o, 1, 9, &r);
        assert(r.score == 0 && r.colour == 41 && r.allies == 0);
    }

    /* 0x0051a210 */
    {
        AoeGameViewRenderOpsRor o = {NULL, r_time, r_stat, r_record, r_adv, r_has,
                                     r_begin, r_clear, r_end, r_dbg, r_log, r_render};
        has_map_v = 1; dbg_v = 1; clock_v = 0; stat_v = 0; rlog[0] = 0;
        aoe_game_view_render_ror(&o);
        assert(strcmp(rlog, "<vs>R<vf>\nA") == 0);
        assert(rec[6] == 200 - 30 - 100 && rec[12] == 30);
        has_map_v = 0; rlog[0] = 0;
        aoe_game_view_render_ror(&o);
        assert(strcmp(rlog, "BCEA") == 0);
    }
}

/* ---- batch 3 ---- */
static int16_t q_log[8]; static int q_n;
static int16_t q_query(void *c, int32_t k) { (void)c; q_log[q_n++] = (int16_t)k; return (int16_t)(k * 10); }
static char m_log[64]; static int32_t m_state, m_mode, m_comm, m_drag, m_args[6];
static int32_t m_gs(void *c) { (void)c; return m_state; }
static int32_t m_em(void *c) { (void)c; return m_mode; }
static int m_cw(void *c) { (void)c; return m_comm; }
static int m_dr(void *c) { (void)c; return m_drag; }
static void m_rel(void *c) { (void)c; strcat(m_log, "R"); }
static void m_fin(void *c, int32_t a, int32_t b) { (void)c; strcat(m_log, "F"); m_args[0] = a; m_args[1] = b; }
static void m_box(void *c, int32_t k, int32_t a, int32_t b, int32_t d, int32_t e)
{ (void)c; assert(k == 2); strcat(m_log, "B"); m_args[2] = a; m_args[3] = b; m_args[4] = d; m_args[5] = e; }
static void m_can(void *c) { (void)c; strcat(m_log, "C"); }
static void m_red(void *c, int32_t l) { (void)c; assert(l == 1); strcat(m_log, "D"); }

static void test_batch3(void)
{
    /* 0x00511790 */
    {
        AoeMainViewRor v; AoeMainViewTileRor r0[2] = {{100, 50, 0}, {130, 60, 5}};
        const AoeMainViewTileRor *rows[1] = {r0};
        int16_t off[16] = {0}; int16_t x, y;
        memset(&v, 0, sizeof v); v.origin_x = 10; v.origin_y = 20; v.geometry[4] = 7;
        off[5] = -4;
        assert(aoe_main_view_tile_point_ror(&v, rows, 2, 1, off, 0, 0, &x, &y, 1) == 1 && x == 90 && y == 30);
        assert(aoe_main_view_tile_point_ror(&v, rows, 2, 1, off, 1, 0, &x, &y, 0) == 1 && x == 120 && y == 36);
        /* slope 5: + 7/2 - 7 */
        assert(aoe_main_view_tile_point_ror(&v, rows, 2, 1, off, 1, 0, &x, &y, 1) == 1 && y == 32);
        r0[1].slope = 2; off[2] = 0;
        assert(aoe_main_view_tile_point_ror(&v, rows, 2, 1, off, 1, 0, &x, &y, 1) == 1 && y == 43);
        assert(aoe_main_view_tile_point_ror(&v, rows, 2, 1, off, 2, 0, &x, &y, 1) == 0 && x == 0 && y == 0);
    }

    /* 0x005150c0 */
    {
        int32_t o[4];
        assert(aoe_main_view_edge_query_ror(1, 0, q_query, NULL, o) == 0 && o[0] == -1 && o[3] == -1);
        assert(aoe_main_view_edge_query_ror(0, 15, q_query, NULL, o) == 0 && q_n == 0);
        assert(aoe_main_view_edge_query_ror(1, 15, q_query, NULL, o) == 1);
        assert(q_n == 4 && q_log[0] == 1 && q_log[1] == 4 && q_log[2] == 3 && q_log[3] == 2);
        assert(o[0] == 10 && o[1] == 20 && o[2] == 30 && o[3] == 40);
        assert(aoe_main_view_edge_query_ror(1, 2, q_query, NULL, o) == 1 && o[0] == -1 && o[3] == 40);
    }

    /* 0x00519360 */
    {
        AoeGameViewMouseOpsRor o = {NULL, m_gs, m_em, m_cw, m_dr, m_rel, m_fin, m_box, m_can, m_red};
        m_state = 3;
        assert(aoe_game_view_mouse_up_ror(&o, 1, 2, 3, 4, 5, 6) == 0 && m_log[0] == 0);
        m_state = 5; m_comm = 1; m_drag = 1;
        assert(aoe_game_view_mouse_up_ror(&o, 1, 2, 3, 4, 5, 6) == 0 && strcmp(m_log, "C") == 0);
        m_log[0] = 0; m_drag = 0;
        assert(aoe_game_view_mouse_up_ror(&o, 1, 2, 3, 4, 5, 6) == 0 && strcmp(m_log, "R") == 0);
        m_log[0] = 0; m_comm = 0; m_drag = 1;
        assert(aoe_game_view_mouse_up_ror(&o, 1, 2, 3, 4, 5, 6) == 1 && strcmp(m_log, "F") == 0);
        m_log[0] = 0; m_drag = 0; m_mode = 0;
        assert(aoe_game_view_mouse_up_ror(&o, 1, 2, 3, 4, 5, 6) == 1 && strcmp(m_log, "RBF") == 0);
        assert(m_args[0] == 3 && m_args[1] == 4 && m_args[2] == 1 && m_args[3] == 2 && m_args[4] == 5 && m_args[5] == 6);
        m_log[0] = 0; m_mode = 7;
        assert(aoe_game_view_mouse_up_ror(&o, 1, 2, 3, 4, 5, 6) == 1 && strcmp(m_log, "D") == 0);
        m_log[0] = 0; m_mode = 3;
        assert(aoe_game_view_mouse_up_ror(&o, 1, 2, 3, 4, 5, 6) == 1 && m_log[0] == 0);
    }
}

/* ---- batch 4 ---- */
static char d_log[64]; static int32_t d_args[5];
static void d_warp(void *c, int32_t x, int32_t y) { (void)c; strcat(d_log, "W"); d_args[0] = x; d_args[1] = y; }
static void d_sel(void *c, int32_t k, int32_t l, int32_t t, int32_t r, int32_t b)
{ (void)c; assert(k == 4); strcat(d_log, "S"); d_args[0] = l; d_args[1] = t; d_args[2] = r; d_args[3] = b; }
static void d_pt(void *c, int32_t a, int32_t b, int32_t x, int32_t y, int32_t f)
{ (void)c; assert(a == x && b == y && f == 0); strcat(d_log, "P"); d_args[0] = a; }
static void d_red(void *c, int32_t l) { (void)c; assert(l == 1); strcat(d_log, "D"); }
static void d_rm(void *c) { (void)c; strcat(d_log, "M"); }
static void d_rc(void *c) { (void)c; strcat(d_log, "R"); }

static void test_batch4(void)
{
    /* 0x00515d30 */
    AoeMainViewDragOpsRor o = {NULL, d_warp, d_sel, d_pt, d_red, d_rm, d_rc};
    uint8_t k = 7;
    aoe_main_view_end_drag_ror(&o, &k, 10, 20, 5, 30);
    assert(strcmp(d_log, "WMR") == 0 && k == 0 && d_args[0] == 10 && d_args[1] == 20);
    d_log[0] = 0; k = 2;
    aoe_main_view_end_drag_ror(&o, &k, 10, 20, 5, 30);
    assert(strcmp(d_log, "SDMR") == 0 && d_args[0] == 5 && d_args[1] == 20 && d_args[2] == 10 && d_args[3] == 30);
    d_log[0] = 0; k = 9;
    aoe_main_view_end_drag_ror(&o, &k, 10, 20, 5, 30);
    assert(strcmp(d_log, "PDR") == 0 && d_args[0] == 10);
    d_log[0] = 0; k = 3;
    aoe_main_view_end_drag_ror(&o, &k, 10, 20, 5, 30);
    assert(strcmp(d_log, "R") == 0 && k == 0);
}

/* ---- batch 5 ---- */
static int h_obj = 5, h_hit;
static void *h_pick(void *c, int32_t x, int32_t y) { (void)c; assert(x == 3 && y == 4); return h_hit ? &h_obj : NULL; }
static int32_t h_sid(void *c, void *o) { (void)c; return *(int *)o * 100; }
static int32_t h_page(void *c, void *o) { (void)c; return *(int *)o + 1; }
static const char *h_str(void *c, int32_t id) { (void)c; return id == 500 ? "Villager" : "?"; }
static int32_t h_base(void *c, char *t, int32_t *p, int32_t x, int32_t y) { (void)c; (void)x; (void)y; strcpy(t, "base"); *p = -7; return 9; }

static void test_batch5(void)
{
    /* 0x0051afd0 */
    AoeGameViewHelpOpsRor o = {NULL, h_pick, h_sid, h_page, h_str, h_base};
    char t[32]; int32_t pg;
    h_hit = 0;
    assert(aoe_game_view_help_ror(&o, t, sizeof t, &pg, 3, 4) == 9 && strcmp(t, "base") == 0 && pg == -7);
    h_hit = 1;
    assert(aoe_game_view_help_ror(&o, t, sizeof t, &pg, 3, 4) == 1 && strcmp(t, "Villager") == 0 && pg == 6);
}

/* ---- batch 6 ---- */
static float bp_x, bp_y; static uint8_t bp_res; static int32_t bm_x, bm_y; static int bp_calls;
static uint8_t bp_pick(void *c, uint8_t kind, int32_t arg, int32_t a[2], int32_t b[2],
                       int32_t *value, int32_t *o0, int32_t *o1, int16_t *h0, int16_t *h1)
{
    (void)c; (void)b; (void)h0; (void)h1; assert(kind == 0x28 && arg == 0 && *value == 0);
    assert(a[0] == bm_x && a[1] == bm_y); bp_calls++;
    memcpy(o0, &bp_x, 4); memcpy(o1, &bp_y, 4);
    return bp_res;
}
static void bm_mouse(void *c, int32_t *x, int32_t *y) { (void)c; *x = bm_x; *y = bm_y; }
static int16_t bm_brush(void *c) { (void)c; return 5; }

static void test_batch6(void)
{
    /* 0x00511310 */
    AoeMainViewOpsRor o = ops;
    AoeMainViewRor v;
    o.pick = bp_pick; o.game_mouse = bm_mouse; o.brush_size = bm_brush;
    memset(&v, 0, sizeof v); v.ops = &o;
    v.left = 0; v.top = 0; v.right = 100; v.bottom = 100;
    bm_x = 101; bm_y = 5; f.highlight[0] = 7;
    aoe_main_view_brush_highlight_ror(&v);
    assert(f.highlight[0] == -1 && f.highlight[3] == -1 && bp_calls == 0);
    bm_x = 100; bp_res = '4';
    aoe_main_view_brush_highlight_ror(&v);
    assert(f.highlight[0] == -1 && bp_calls == 1);
    bp_res = '3'; bp_x = 10.7f; bp_y = 20.2f; f.edit_mode = 0;
    aoe_main_view_brush_highlight_ror(&v);
    assert(f.highlight[0] == 8 && f.highlight[1] == 18 && f.highlight[2] == 12 && f.highlight[3] == 22);
    f.edit_mode = 0x13;
    aoe_main_view_brush_highlight_ror(&v);
    /* 10.7 -> 3*3+1 = 10, 20.2 -> 6*3+1 = 19, size 3 -> half 1 */
    assert(f.highlight[0] == 9 && f.highlight[1] == 18 && f.highlight[2] == 11 && f.highlight[3] == 20);
    f.edit_mode = 0;
}

/* ---- batch 7 ---- */
static int md_pending;
static void md_cap(void *c) { (void)c; strcat(m_log, "K"); }
static int md_pend(void *c) { (void)c; return md_pending; }
static void md_clr(void *c) { (void)c; strcat(m_log, "X"); }
static void md_mode(void *c, int32_t m, int32_t a) { (void)c; assert(m == 0x13 && a == 1); strcat(m_log, "G"); }
static int32_t md_box[5];
static void md_boxf(void *c, int32_t k, int32_t a, int32_t b, int32_t d, int32_t e)
{ (void)c; strcat(m_log, "B"); md_box[0] = k; md_box[1] = a; md_box[2] = b; md_box[3] = d; md_box[4] = e; }

static void test_batch7(void)
{
    /* 0x00519d50 */
    AoeGameViewMouseOpsRor o = {NULL, m_gs, m_em, m_cw, m_dr, m_rel, m_fin, md_boxf, m_can, m_red,
                                md_cap, md_pend, md_clr, md_mode};
    m_log[0] = 0; m_comm = 1; m_state = 5;
    assert(aoe_game_view_mouse_down_ror(&o, 1, 2, 3, 4) == 0 && m_log[0] == 0);
    m_comm = 0; m_state = 8;
    assert(aoe_game_view_mouse_down_ror(&o, 1, 2, 3, 4) == 0 && m_log[0] == 0);
    m_state = 4; m_mode = 0;
    assert(aoe_game_view_mouse_down_ror(&o, 1, 2, 3, 4) == 1 && strcmp(m_log, "K") == 0);
    m_log[0] = 0; m_mode = 0x13; md_pending = 1;
    assert(aoe_game_view_mouse_down_ror(&o, 1, 2, 3, 4) == 1 && strcmp(m_log, "XDGB") == 0);
    assert(md_box[0] == 9 && md_box[1] == 1 && md_box[2] == 2 && md_box[3] == 3 && md_box[4] == 4);
    m_log[0] = 0; md_pending = 0;
    assert(aoe_game_view_mouse_down_ror(&o, 1, 2, 3, 4) == 1 && strcmp(m_log, "GB") == 0);
}

/* ---- C3L: drag start ---- */
static char c3l_log[64];
static uint8_t c3l_pick_res; static float c3l_px, c3l_py; static int c3l_action_ok;
static float c3l_vx, c3l_vy; static int32_t c3l_act[5];
static uint8_t c3l_pick(void *c, uint8_t kind, int32_t arg, int32_t a[2], int32_t b[2],
                        int32_t *value, int32_t *o0, int32_t *o1, int16_t *h0, int16_t *h1)
{
    (void)c; (void)b; (void)h0; (void)h1; (void)a;
    assert(kind == 0x28 && arg == 0 && *value == 0);
    strcat(c3l_log, "P");
    memcpy(o0, &c3l_px, 4); memcpy(o1, &c3l_py, 4);
    return c3l_pick_res;
}
static void c3l_redraw(void *c, void *p, int32_t l) { (void)c; (void)p; assert(l == 1); strcat(c3l_log, "R"); }
static void c3l_cap(void *c) { (void)c; strcat(c3l_log, "C"); }
static void c3l_hide(void *c) { (void)c; strcat(c3l_log, "H"); }
static int c3l_action(void *c, int32_t a, int32_t b, int32_t d, int32_t e, int32_t g)
{ (void)c; c3l_act[0] = a; c3l_act[1] = b; c3l_act[2] = d; c3l_act[3] = e; c3l_act[4] = g; strcat(c3l_log, "A"); return c3l_action_ok; }
static void c3l_clear(void *c) { (void)c; strcat(c3l_log, "X"); }
static void c3l_centre(void *c, float x, float y) { (void)c; c3l_vx = x; c3l_vy = y; strcat(c3l_log, "V"); }

static void test_c3l_begin_drag(void)
{
    /* 0x005154e0 */
    AoeMainViewOpsRor o = ops;
    AoeMainViewDragStartOpsRor d = {NULL, c3l_cap, c3l_hide, c3l_action, c3l_clear, c3l_centre};
    AoeMainViewDragStateRor st;
    AoeMainViewRor v;
    o.pick = c3l_pick; o.redraw = c3l_redraw;
    memset(&v, 0, sizeof v); v.ops = &o;
    memset(&st, 0, sizeof st);

    /* Kind 3 off the map: nothing stored. */
    c3l_log[0] = 0; c3l_pick_res = '4';
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 3, 5, 6, 0, 0) == 0);
    assert(strcmp(c3l_log, "P") == 0 && st.kind == 0);
    /* Kind 3 on the map: single-tile highlight, capture, return 0. */
    c3l_log[0] = 0; c3l_pick_res = '3'; c3l_px = 7.9f; c3l_py = 3.2f;
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 3, 5, 6, 0, 0) == 0);
    assert(strcmp(c3l_log, "PC") == 0 && st.kind == 3);
    assert(f.highlight[0] == 7 && f.highlight[1] == 3 && f.highlight[2] == 7 && f.highlight[3] == 3);
    assert(st.press_x == 5 && st.cur_x == 5 && st.press_y == 6 && st.cur_y == 6);
    /* Kind 9: anchor recorded even when the action fails. */
    c3l_log[0] = 0; c3l_action_ok = 0; st.kind = 0;
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 9, 11, 12, 0, 0) == 0);
    assert(strcmp(c3l_log, "PA") == 0 && st.kind == 0 && st.anchor_x == 7 && st.anchor_y == 3);
    assert(c3l_act[0] == 11 && c3l_act[1] == 12 && c3l_act[2] == 11 && c3l_act[3] == 12 && c3l_act[4] == 1);
    c3l_log[0] = 0; c3l_action_ok = 1;
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 9, 11, 12, 0, 0) == 0);
    assert(strcmp(c3l_log, "PARC") == 0 && st.kind == 9);
    /* Kind 2: hide, selection cleared only without either keep flag. */
    c3l_log[0] = 0;
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 2, 1, 1, 0, 0) == 0);
    assert(strcmp(c3l_log, "CHXR") == 0);
    c3l_log[0] = 0;
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 2, 1, 1, 0, 1) == 0);
    assert(strcmp(c3l_log, "CHR") == 0);
    /* Kinds 6/8 centre on the map point + 0.5 and return 1. */
    c3l_log[0] = 0; c3l_px = 4.0f; c3l_py = 9.25f;
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 8, 1, 1, 0, 0) == 1);
    assert(strcmp(c3l_log, "CHPVR") == 0 && c3l_vx == 4.5f && c3l_vy == 9.75f);
    c3l_log[0] = 0; c3l_pick_res = '4';
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 6, 1, 1, 0, 0) == 0);
    assert(strcmp(c3l_log, "CHP") == 0);
    /* Kind 7 hides without picking. */
    c3l_log[0] = 0;
    assert(aoe_main_view_begin_drag_ror(&v, &st, &d, 7, 1, 1, 0, 0) == 0);
    assert(strcmp(c3l_log, "CH") == 0);
}

static int b8_kind;
static void b8_box(void *c, int32_t k, int32_t a, int32_t b, int32_t d, int32_t e)
{ (void)c; b8_kind = k; strcat(m_log, "B"); m_args[2] = a; m_args[3] = b; m_args[4] = d; m_args[5] = e; }
static char o_log[32]; static uint8_t o_pick_code;
static int o_on(void *c) { (void)c; return 1; }
static int o_sel_n;
static int o_sel(void *c) { (void)c; return o_sel_n; }
static uint8_t o_pick(void *c, int32_t x, int32_t y, int32_t out[5])
{ (void)c; strcat(o_log, "P"); out[0] = x; out[1] = y; out[3] = 77; return o_pick_code; }
static void o_clamp(void *c, int32_t out[5]) { (void)c; strcat(o_log, "K"); out[0] -= 1; }
static uint8_t o_order(void *c, int32_t a, int32_t x, int32_t y)
{ (void)c; strcat(o_log, "O"); assert(a == 77 && x == 9 && y == 20); return 5; }
static void o_red(void *c, int32_t l) { (void)c; assert(l == 1); strcat(o_log, "D"); }
static void o_mode(void *c, int32_t m, int32_t a) { (void)c; assert(m == 0 && a == 0); strcat(o_log, "M"); }
static int32_t s_mode; static char s_log[64]; static int32_t s_last[7];
static uint8_t s_pick(void *c, int32_t x, int32_t y, float *fx, float *fy)
{ (void)c; *fx = (float)x + 0.75f; *fy = (float)y + 0.5f; return x < 0 ? '1' : '3'; }
static int32_t s_em(void *c) { (void)c; return s_mode; }
static void s_stroke(void *c, int32_t slot, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t a, int32_t b)
{ (void)c; strcat(s_log, "S"); s_last[0] = slot; s_last[1] = x0; s_last[2] = y0; s_last[3] = x1; s_last[4] = y1; s_last[5] = a; s_last[6] = b; }
static void s_red(void *c, int32_t l) { (void)c; assert(l == 2); strcat(s_log, "D"); }

static void test_batch8(void)
{
    /* 0x00519e00 */
    {
        AoeGameViewMouseOpsRor o = {NULL, m_gs, m_em, m_cw, m_dr, m_rel, m_fin, b8_box, m_can, m_red};
        m_log[0] = 0; m_state = 3; m_comm = 0; m_drag = 0;
        assert(aoe_game_view_mouse_move_ror(&o, 0, 1, 2, 3, 4, 5, 6) == 0 && m_log[0] == 0);
        m_state = 6; m_comm = 1; m_drag = 1;
        assert(aoe_game_view_mouse_move_ror(&o, 0, 1, 2, 3, 4, 5, 6) == 0 && strcmp(m_log, "C") == 0);
        m_log[0] = 0; m_drag = 0;
        assert(aoe_game_view_mouse_move_ror(&o, 0, 1, 2, 3, 4, 5, 6) == 0 && strcmp(m_log, "R") == 0);
        m_log[0] = 0; m_comm = 0; m_drag = 1;
        assert(aoe_game_view_mouse_move_ror(&o, 1, 1, 2, 3, 4, 5, 6) == 1 && strcmp(m_log, "F") == 0);
        m_log[0] = 0; m_drag = 0;
        assert(aoe_game_view_mouse_move_ror(&o, 1, 1, 2, 3, 4, 5, 6) == 0 && m_log[0] == 0);
        assert(aoe_game_view_mouse_move_ror(&o, 0, 1, 2, 3, 4, 5, 6) == 1 && strcmp(m_log, "RBF") == 0);
        assert(b8_kind == 1 && m_args[0] == 3 && m_args[1] == 4 && m_args[2] == 1 && m_args[3] == 2 && m_args[4] == 5 && m_args[5] == 6);
    }
    /* 0x0051ad40 */
    {
        AoeGameViewOrderOpsRor o = {NULL, o_on, o_sel, o_pick, o_clamp, o_order, o_red, o_mode};
        o_sel_n = 0; o_log[0] = 0;
        assert(aoe_game_view_map_order_ror(&o, 10, 20) == 0 && o_log[0] == 0);
        o_sel_n = 1; o_pick_code = '2';
        assert(aoe_game_view_map_order_ror(&o, 10, 20) == 0 && strcmp(o_log, "PM") == 0);
        o_log[0] = 0; o_pick_code = '3';
        assert(aoe_game_view_map_order_ror(&o, 10, 20) == 5 && strcmp(o_log, "PKODM") == 0);
    }
    /* 0x00515e60 */
    {
        AoeMainViewBrushOpsRor o = {NULL, s_pick, s_em, 3, -1, 2, 1, s_stroke, s_red};
        int32_t px = 1, py = 1;
        assert(aoe_main_view_brush_stroke_ror(&o, &px, &py, 0, 0, -5, 7, 1) == 0 && px == 1);
        s_mode = 9;
        assert(aoe_main_view_brush_stroke_ror(&o, &px, &py, 0, 0, 4, 7, 1) == 1 && px == 4 && py == 7);
        assert(strcmp(s_log, "S") == 0 && s_last[0] == 0x10 && s_last[1] == 1 && s_last[3] == 4 && s_last[5] == 2 && s_last[6] == 3);
        s_log[0] = 0; s_mode = 10;
        assert(aoe_main_view_brush_stroke_ror(&o, &px, &py, 0, 0, 5, 8, 1) == 1 && s_log[0] == 0 && px == 5);
        s_mode = 9;
        assert(aoe_main_view_brush_stroke_ror(&o, &px, &py, 0, 0, 6, 8, 0) == 1 && s_log[0] == 0 && px == 6);
        s_mode = 0x13;
        assert(aoe_main_view_brush_stroke_ror(&o, &px, &py, 0, 0, 2, 3, 0) == 1 && strcmp(s_log, "SD") == 0);
        assert(s_last[0] == 0x20 && s_last[1] == 6 && s_last[2] == 8 && s_last[5] == 0 && s_last[6] == 1);
    }
}

/* ---- C3L: pixel picking ---- */
/* A 12-wide, 2-row frame. Row 0: outline (1, 2); skip 2, colour 3,
   skip 1, shadow 2 (via next byte), end. Row 1: outline -32768. */
static uint8_t c3l_shape[64];
static AoeShapeHitFrameRor c3l_frame = {0, 8, 12, 2};
static void c3l_build_shape(void)
{
    static const uint8_t cmds[] = {0x09, 0x0c, 1, 2, 3, 0x05, 0x0b, 2, 0x0f};
    memset(c3l_shape, 0, sizeof c3l_shape);
    c3l_shape[0] = 16; c3l_shape[4] = 16;             /* command table */
    c3l_shape[8] = 1; c3l_shape[10] = 2;              /* row 0 outline */
    c3l_shape[12] = 0x00; c3l_shape[13] = 0x80;       /* row 1 outline */
    memcpy(&c3l_shape[16], cmds, sizeof cmds);
}

typedef struct { const void *next; int32_t id, key; uint32_t flags; int16_t l, t, r, b, f14, f18; } C3lNode;
static C3lNode c3l_nodes[4];
static const void *c3l_heads[0x29];
static const void *c3l_head(void *c, int32_t row) { (void)c; assert(row >= 0 && row <= 0x28); return c3l_heads[row]; }
static void c3l_item(void *c, const void *n, AoeViewDrawItemRor *o)
{
    const C3lNode *p = n; (void)c;
    memset(o, 0, sizeof *o);
    o->next = p->next; o->object_id = p->id; o->key = p->key; o->flags = p->flags;
    o->left = p->l; o->top = p->t; o->right = p->r; o->bottom = p->b;
    o->field_14 = p->f14; o->field_18 = p->f18;
    o->frame = &c3l_frame; o->shape = c3l_shape; o->shape_size = sizeof c3l_shape;
}
static int c3l_object(void *c, int32_t id, int16_t *pl, uint8_t *st, uint8_t *k)
{ (void)c; *pl = (int16_t)(id % 3); *st = id == 7 ? 1 : 2; *k = (uint8_t)id; return 1; }

static void test_c3l_pixel_pick(void)
{
    AoeViewDrawRowsOpsRor ro = {NULL, c3l_head, c3l_item, c3l_object};
    AoeViewPickedRor out[AOE_VIEW_PICKED_MAX_ROR];
    AoeMainViewRor v;
    int32_t id;

    /* 0x005149d0 */
    c3l_build_shape();
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 0, 0) == 0);  /* left of outline */
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 2, 0) == 1);  /* skip */
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 3, 0) == 2);  /* colour */
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 5, 0) == 2);
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 6, 0) == 1);
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 8, 0) == 2);  /* shadow */
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 9, 0) == 0);  /* past width-right */
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 3, 1) == 0);  /* empty row */
    assert(aoe_shape_hit_ror(c3l_shape, sizeof c3l_shape, &c3l_frame, 3, 2) == 0);
    /* The 12-bit form keeps the low nibble: 0x02 << 4 | 1 = 0x21 pixels. */
    {
        static uint8_t big[0x40]; AoeShapeHitFrameRor fr = {0, 4, 0x40, 1};
        big[0] = 8; big[8] = 0x02; big[9] = 0x01;
        assert(aoe_shape_hit_ror(big, sizeof big, &fr, 0x20, 0) == 2);
        assert(aoe_shape_hit_ror(big, 9, &fr, 0x20, 0) == 0);
    }

    memset(&v, 0, sizeof v);
    v.left = 100; v.top = 50; v.right = 300; v.bottom = 200;
    memset(c3l_heads, 0, sizeof c3l_heads);
    memset(c3l_nodes, 0, sizeof c3l_nodes);
    /* Row 5: node 0 (id 4) at left 100, node 1 (id 5) mirrored, right 109. */
    c3l_nodes[0] = (C3lNode){&c3l_nodes[1], 4, 0, 0, 100, 50, 111, 52, 1, 2};
    c3l_nodes[1] = (C3lNode){NULL, 5, 0, 2, 98, 50, 109, 52, 3, 4};
    c3l_heads[5] = &c3l_nodes[0];
    /* Row 9: node 2 (id 6) keyed (ignored by the pixel pick). */
    c3l_nodes[2] = (C3lNode){NULL, 6, 1, 0, 100, 50, 111, 52, 5, 6};
    c3l_heads[9] = &c3l_nodes[2];

    /* 0x00514d70: x 103 is pixel 3 of node 0 and pixel 6 (skip) of node 1. */
    assert(aoe_main_view_pixel_object_ror(&v, &ro, 103, 50, 0, 40, &id) == 1 && id == 4);
    /* x 104 hits both (4 and mirrored 5): the later node wins. */
    assert(aoe_main_view_pixel_object_ror(&v, &ro, 104, 50, -3, 99, &id) == 1 && id == 5);
    assert(aoe_main_view_pixel_object_ror(&v, &ro, 100, 50, 0, 40, &id) == 0 && id == 0);
    assert(aoe_main_view_pixel_object_ror(&v, &ro, 103, 50, 6, 40, &id) == 0);
    assert(aoe_main_view_pixel_object_ror(&v, &ro, 99, 50, 0, 40, &id) == 0);
    assert(aoe_main_view_pixel_object_ror(&v, &ro, 103, 50, 7, 6, &id) == 0);

    /* 0x00514b60: panel-relative boxes; IDs 4 and 5 (player 1 and 2). */
    c3l_nodes[2].key = 0; c3l_nodes[2].id = 4;   /* a repeat of 4 in a later-scanned row */
    c3l_heads[3] = &c3l_nodes[2];
    c3l_heads[9] = NULL;
    assert(aoe_main_view_box_objects_ror(&v, &ro, 100, 50, 300, 200, 0, 40, -1, 0, out) == 2);
    assert(out[0].id == 4 && out[0].count == 1 && out[0].field_6 == 1 && out[0].field_14 == 1 &&
           out[0].field_18 == 2 && out[1].id == 5 && out[1].field_14 == 3);
    assert(aoe_main_view_box_objects_ror(&v, &ro, 100, 50, 300, 200, 0, 40, 2, 0, out) == 1 &&
           out[0].id == 5);
    assert(aoe_main_view_box_objects_ror(&v, &ro, 100, 50, 300, 200, 0, 40, -1, 5, out) == 1 &&
           out[0].id == 5);
    /* The box must overlap the node (x0 - 100 > 111 misses). */
    assert(aoe_main_view_box_objects_ror(&v, &ro, 212, 50, 300, 200, 0, 40, -1, 0, out) == 0);
    assert(aoe_main_view_box_objects_ror(&v, &ro, 301, 50, 400, 200, 0, 40, -1, 0, out) == 0);
    /* State 1 (id 7) excluded. */
    c3l_nodes[2].id = 7;
    assert(aoe_main_view_box_objects_ror(&v, &ro, 100, 50, 300, 200, 3, 3, -1, 0, out) == 0);
}

/* ---- C3L: line nodes ---- */
static int32_t c3l_calls[32][6]; static int c3l_ncalls;
static void c3l_span(void *c, int32_t a, int32_t b, int32_t y)
{ (void)c; int32_t *r = c3l_calls[c3l_ncalls++]; r[0] = 'S'; r[1] = a; r[2] = b; r[3] = y; }
static void c3l_line(void *c, int32_t a, int32_t b, int32_t d, int32_t e)
{ (void)c; int32_t *r = c3l_calls[c3l_ncalls++]; r[0] = 'L'; r[1] = a; r[2] = b; r[3] = d; r[4] = e; }
static void c3l_draw(void *c, int32_t a, int32_t b, int32_t d, int32_t e, uint8_t col)
{ (void)c; int32_t *r = c3l_calls[c3l_ncalls++]; r[0] = 'D'; r[1] = a; r[2] = b; r[3] = d; r[4] = e; r[5] = col; }

static void test_c3l_line_nodes(void)
{
    AoeViewLineOpsRor o = {NULL, c3l_span, c3l_line, c3l_draw};
    AoeViewLineNodeRor n = {1, 9, {1, 2, 3, 4, 5, 6, 7, 8}};

    /* 0x00513a40 */
    c3l_ncalls = 0;
    aoe_view_line_node_dirty_ror(&n, &o);
    assert(c3l_ncalls == 4 && c3l_calls[3][0] == 'L' && c3l_calls[3][1] == 7 &&
           c3l_calls[3][2] == 8 && c3l_calls[3][3] == 1 && c3l_calls[3][4] == 2);
    n.kind = 3; n.pt[0] = 5; n.pt[1] = 12; n.pt[2] = 9; n.pt[3] = 10;
    c3l_ncalls = 0;
    aoe_view_line_node_dirty_ror(&n, &o);
    assert(c3l_ncalls == 3 && c3l_calls[0][1] == 4 && c3l_calls[0][2] == 11 &&
           c3l_calls[0][3] == 10 && c3l_calls[2][3] == 12);
    n.kind = 4; n.pt[0] = 13; n.pt[2] = 2; n.pt[1] = 3; n.pt[3] = 0;
    c3l_ncalls = 0;
    aoe_view_line_node_dirty_ror(&n, &o);
    /* top, bottom full rows; rows 1, 2 the two edge groups. */
    assert(c3l_ncalls == 6);
    assert(c3l_calls[0][1] == 0 && c3l_calls[0][2] == 15 && c3l_calls[0][3] == 0);
    assert(c3l_calls[1][3] == 3);
    assert(c3l_calls[2][1] == 0 && c3l_calls[2][2] == 3 && c3l_calls[2][3] == 1);
    assert(c3l_calls[3][1] == 12 && c3l_calls[3][2] == 15 && c3l_calls[3][3] == 1);

    /* 0x00513bc0 */
    n.kind = 1; c3l_ncalls = 0;
    aoe_view_line_node_draw_ror(&n, &o);
    assert(c3l_ncalls == 4 && c3l_calls[0][5] == 9);
    n.kind = 3; c3l_ncalls = 0;
    aoe_view_line_node_draw_ror(&n, &o);
    assert(c3l_ncalls == 0);
    n.kind = 2; n.pt[0] = 1; n.pt[1] = 4; n.pt[2] = 20; n.pt[3] = 5;
    n.pt[4] = 6; n.pt[5] = 1; n.pt[6] = 15; n.pt[7] = 0;
    aoe_view_line_node_draw_ror(&n, &o);
    assert(c3l_ncalls == 2 && c3l_calls[0][1] == 1 && c3l_calls[0][3] == 6 &&
           c3l_calls[0][2] == 4 && c3l_calls[0][5] == 0x4a && c3l_calls[1][2] == 5);
    n.pt[7] = 1; c3l_ncalls = 0;
    aoe_view_line_node_draw_ror(&n, &o);
    assert(c3l_ncalls == 4 && c3l_calls[1][1] == 15 && c3l_calls[1][3] == 20 && c3l_calls[1][5] == 0x97);
    n.kind = 4; c3l_ncalls = 0;
    aoe_view_line_node_draw_ror(&n, &o);
    assert(c3l_ncalls == 4 && c3l_calls[3][1] == 20 && c3l_calls[3][2] == 4 &&
           c3l_calls[3][4] == 5 && c3l_calls[3][5] == 0xff);
}

static char wl_log[1024];
static int wl_cmd_ret;
static int wl_blocked(void *c, float x, float y) { (void)c; return x == 2.5f && y == 2.5f; }
static void wl_place(void *c, float x, float y) {
    char t[24];
    (void)c;
    snprintf(t, sizeof t, "%d,%d;", (int)x, (int)y);
    strcat(wl_log, t);
}
static int wl_cmd(void *c, int32_t a, int32_t b, int32_t d, int32_t e) {
    (void)c; assert(a == 1 && b == 2 && d == 3 && e == 4); strcat(wl_log, "C;"); return wl_cmd_ret;
}
static void wl_g10(void *c) { (void)c; strcat(wl_log, "G;"); }
static void wl_clear(void *c) { (void)c; strcat(wl_log, "X;"); }
static void wl_redraw(void *c, int32_t l) { (void)c; assert(l == 1); strcat(wl_log, "R;"); }

static void test_wall_drag(void) {
    AoeMainViewWallOpsRor o = {NULL, 1, 7, 1, wl_blocked, wl_place, wl_cmd, wl_g10, wl_clear, wl_redraw};
    wl_log[0] = 0;
    aoe_main_view_wall_run_ror(&o, 0, 0, 3, 3);
    assert(!strcmp(wl_log, "0,0;1,1;3,3;"));
    wl_log[0] = 0;
    assert(aoe_main_view_wall_drag_ror(&o, 5, 5, 8, 4, 0) == 1);
    assert(!strcmp(wl_log, "5,5;6,5;7,5;8,5;8,4;X;R;"));
    wl_log[0] = 0;
    assert(aoe_main_view_wall_drag_ror(&o, 5, 5, 4, 7, 0) == 1);
    assert(!strcmp(wl_log, "5,5;5,6;5,7;4,7;X;R;"));
    wl_log[0] = 0;
    assert(aoe_main_view_wall_drag_ror(&o, 5, 5, 5, 5, 0) == 1 && !strcmp(wl_log, "5,5;X;R;"));
    wl_log[0] = 0;
    o.has_template = 0;
    assert(aoe_main_view_wall_drag_ror(&o, 5, 5, 6, 6, 1) == 0 && !strcmp(wl_log, "X;R;"));
    wl_log[0] = 0;
    o.game_mode = 4;
    o.has_template = 1;
    wl_cmd_ret = 1;
    assert(aoe_main_view_wall_drag_ror(&o, 1, 2, 3, 4, 1) == 1 && !strcmp(wl_log, "C;G;X;R;"));
    wl_log[0] = 0;
    o.editing = 0;
    assert(aoe_main_view_wall_drag_ror(&o, 1, 2, 3, 4, 1) == 0 && wl_log[0] == 0);
}

/* ---- C3L: tile object pick ---- */
typedef struct { uint8_t state, sel, kind; int owner, hit, box; int16_t ox, oy; } C3lObj;
static int c3l_sel_room;
static uint8_t c3l_ost(void *c, void *o) { (void)c; return ((C3lObj *)o)->state; }
static int c3l_osel(void *c, void *o) { (void)c; return ((C3lObj *)o)->sel; }
static uint8_t c3l_okind(void *c, void *o) { (void)c; return ((C3lObj *)o)->kind; }
static int c3l_me;
static void *c3l_oown(void *c, void *o) { (void)c; return ((C3lObj *)o)->owner ? (void *)&c3l_me : NULL; }
static void c3l_ooff(void *c, void *o, int16_t *x, int16_t *y) { (void)c; *x = ((C3lObj *)o)->ox; *y = ((C3lObj *)o)->oy; }
static int c3l_obox(void *c, void *o, int32_t x, int32_t y, const int16_t a[3], const int16_t b[3])
{ (void)c; (void)x; (void)y; (void)a; (void)b; return ((C3lObj *)o)->box; }
static int c3l_ohit(void *c, void *o, int32_t x, int32_t y, const int16_t b[3], int32_t s)
{ (void)c; (void)x; (void)y; (void)b; assert(s == 5); return ((C3lObj *)o)->hit; }
static int c3l_oadd(void *c, void *o) { (void)c; (void)o; return c3l_sel_room-- > 0; }
static int c3l_ospr(void *c, void *o, int32_t *dx, int32_t *dy) { (void)c; (void)o; *dx = 2; *dy = 3; return 1; }

static void test_c3l_tile_object(void)
{
    /* 0x005152f0 */
    AoeTilePickOpsRor o = {NULL, c3l_ost, c3l_osel, c3l_okind, c3l_oown, c3l_ooff,
                           c3l_obox, c3l_ohit, c3l_oadd, c3l_ospr};
    C3lObj objs[3] = {{2, 1, 4, 1, 1, 1, 10, 20}, {2, 0, 6, 0, 1, 1, 30, 40}, {2, 1, 3, 1, 1, 0, 50, 60}};
    void *list[3] = {&objs[0], &objs[1], &objs[2]};
    int16_t a[3] = {0}, b[3] = {0}, ox = 0, oy = 0;

    assert(aoe_main_view_tile_object_ror(&o, 0, list, 3, &c3l_me, 0, 0, 1, 1, a, b, &ox, &oy, NULL, 0) == NULL);
    /* The other player's object 1 does not beat the player's object 0;
       object 2 (lower kind) neither. */
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, 0, 0, 100, 200, a, b, &ox, &oy, NULL, 0) == &objs[0]);
    assert(ox == 112 && oy == 223);
    /* Kind filter and selectable filter. */
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, 0, 5, 100, 200, a, b, &ox, &oy, NULL, 0) == &objs[1]);
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, 0, 5, 100, 200, a, b, &ox, &oy, NULL, 1) == NULL);
    /* Not the player's first: the higher kind wins. */
    objs[0].owner = 0;
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, 0, 0, 0, 0, a, b, &ox, &oy, NULL, 0) == &objs[1]);
    /* The preferred object returns the earlier pick at once. */
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, 0, 0, 0, 0, a, b, &ox, &oy, &objs[1], 0) == &objs[0]);
    objs[0].owner = 1;
    /* Mode '+': only the player's objects. */
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, '+', 0, 0, 0, a, b, &ox, &oy, NULL, 0) == &objs[0]);
    /* Mode ',': box hits added; a full selection stops the scan. */
    objs[2].box = 1; c3l_sel_room = 5;
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, ',', 0, 0, 0, a, b, &ox, &oy, NULL, 0) == &objs[2]);
    c3l_sel_room = 1;
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, ',', 0, 0, 0, a, b, &ox, &oy, NULL, 0) == &objs[0]);
    /* State 7 skipped: the player's object 2 is left. */
    objs[0].state = 7;
    assert(aoe_main_view_tile_object_ror(&o, 1, list, 3, &c3l_me, '+', 0, 0, 0, a, b, &ox, &oy, NULL, 0) == &objs[2]);
}

static char g_log[64];
static AoeGameViewUnitRor g_units[3]; static int g_hit;
static int32_t g_state(void *c) { (void)c; return m_state; }
static void *g_pick(void *c, int32_t x, int32_t y, AoeGameViewUnitRor *i)
{ (void)c; (void)x; (void)y; if (!g_hit) return NULL; *i = g_units[0]; return &g_units[0]; }
static int32_t g_count(void *c) { (void)c; return 3; }
static void *g_at(void *c, int32_t n, AoeGameViewUnitRor *i) { (void)c; *i = g_units[n]; return &g_units[n]; }
static int g_screen(void *c, int32_t id) { (void)c; return id != 13; }
static void g_clear(void *c) { (void)c; strcat(g_log, "X"); }
static void g_select(void *c, void *u) { (void)c; char b[2] = {(char)('0' + (int)((AoeGameViewUnitRor *)u - g_units)), 0}; strcat(g_log, b); }
static void *c_target; static uint8_t c_res, c_pick_code;
static void *c_find(void *c, int32_t x, int32_t y, int32_t a) { (void)c; assert(x == 4 && y == 5 && a == 6); strcat(g_log, "T"); return c_target; }
static uint8_t c_ot(void *c, void *t) { (void)c; (void)t; strcat(g_log, "O"); return c_res; }
static void c_flash(void *c, void *t) { (void)c; (void)t; strcat(g_log, "L"); }
static void c_clear(void *c) { (void)c; strcat(g_log, "C"); }
static void c_red(void *c, int32_t l) { (void)c; assert(l == 1); strcat(g_log, "D"); }
static uint8_t c_pick(void *c, int32_t x, int32_t y, float p[2]) { (void)c; p[0] = (float)x; p[1] = (float)y; strcat(g_log, "P"); return c_pick_code; }
static void c_clamp(void *c, float p[2]) { (void)c; p[0] += 0.5f; strcat(g_log, "K"); }
static uint8_t c_move(void *c, float x, float y) { (void)c; assert(x == 4.5f && y == 5.0f); strcat(g_log, "M"); return c_res; }
static void c_rewind(void *c, int32_t k) { (void)c; assert(k == 2); strcat(g_log, "W"); }
static int32_t c_mx, c_my;
static void *c_find_alt(void *c, int32_t x, int32_t y, int32_t a) { (void)c; assert(x == 4 && y == 5 && a == -1); strcat(g_log, "T"); return c_target; }
static void c_mode(void *c, int32_t m, int32_t a) { (void)c; assert(m == 0 && a == 0); strcat(g_log, "G"); }
static void c_mark(void *c, int32_t x, int32_t y) { (void)c; c_mx = x; c_my = y; strcat(g_log, "A"); }

static void test_batch9(void)
{
    /* 0x00509880 */
    assert(aoe_game_view_unit_cursor_ror(1, 199, 0, 0, 1) == 3);
    assert(aoe_game_view_unit_cursor_ror(3, 0, 1, 0, 3) == 2);
    assert(aoe_game_view_unit_cursor_ror(1, 0x116, 2, 0, 3) == 0);
    assert(aoe_game_view_unit_cursor_ror(3, 0, 0, 1, 3) == 5);
    assert(aoe_game_view_unit_cursor_ror(4, 0, 0, 0, 3) == 3);
    assert(aoe_game_view_unit_cursor_ror(2, 0, 0, 0, 3) == 2);
    /* 0x005099b0 */
    {
        AoeGameViewSelectOpsRor o = {NULL, g_state, g_pick, g_count, g_at, g_screen, g_clear, g_select};
        g_units[0] = (AoeGameViewUnitRor){5, 4, 3, 11, 1};
        g_units[1] = (AoeGameViewUnitRor){6, 4, 3, 12, 1};
        g_units[2] = (AoeGameViewUnitRor){5, 4, 0, 13, 1};
        m_state = 7; g_hit = 1; g_log[0] = 0;
        assert(aoe_game_view_select_same_ror(&o, 0, 0, 0, 0) == 0 && g_log[0] == 0);
        m_state = 4; g_hit = 0;
        assert(aoe_game_view_select_same_ror(&o, 0, 0, 0, 0) == 0);
        g_hit = 1; g_units[0].owned = 0;
        assert(aoe_game_view_select_same_ror(&o, 0, 0, 0, 0) == 0 && g_log[0] == 0);
        g_units[0].owned = 1;
        assert(aoe_game_view_select_same_ror(&o, 0, 0, 0, 0) == 1 && strcmp(g_log, "X0010") == 0);
        g_log[0] = 0; g_units[0].unit_class = 2;
        assert(aoe_game_view_select_same_ror(&o, 0, 0, 1, 0) == 1 && strcmp(g_log, "000") == 0);
    }
    /* 0x0051a4d0 */
    {
        AoeGameViewCommandOpsRor o = {NULL, o_on, o_sel, c_find, c_ot, c_flash, c_clear, c_red,
                                      c_pick, c_clamp, c_move, c_rewind, c_mark};
        int tgt;
        g_log[0] = 0; o_sel_n = 0;
        assert(aoe_game_view_command_ror(&o, 100, 200, 4, 5, 1, 6) == 0 && g_log[0] == 0);
        o_sel_n = 1; c_target = &tgt; c_res = 1;
        assert(aoe_game_view_command_ror(&o, 100, 200, 4, 5, 1, 6) == 1 && strcmp(g_log, "TOLCD") == 0);
        g_log[0] = 0; c_res = 0;
        assert(aoe_game_view_command_ror(&o, 100, 200, 4, 5, 1, 6) == 0 && strcmp(g_log, "TOCD") == 0);
        g_log[0] = 0; c_target = NULL;
        assert(aoe_game_view_command_ror(&o, 100, 200, 4, 5, 0, 6) == 0 && strcmp(g_log, "TCD") == 0);
        g_log[0] = 0; c_pick_code = '2';
        assert(aoe_game_view_command_ror(&o, 100, 200, 4, 5, 1, 6) == 0 && strcmp(g_log, "TPCD") == 0);
        g_log[0] = 0; c_pick_code = '3';
        assert(aoe_game_view_command_ror(&o, 100, 200, 4, 5, 1, 6) == 0 && strcmp(g_log, "TPKM") == 0);
        g_log[0] = 0; c_res = 1;
        assert(aoe_game_view_command_ror(&o, 100, 200, 4, 5, 1, 6) == 1 && strcmp(g_log, "TPKMWAD") == 0);
        assert(c_mx == 104 && c_my == 205);
    }
    /* 0x0051ae10 */
    {
        AoeGameViewCommandOpsRor o = {NULL, o_on, o_sel, c_find_alt, c_ot, c_flash, c_clear, c_red,
                                      c_pick, c_clamp, c_move, c_rewind, c_mark, c_mode};
        int tgt;
        g_log[0] = 0; o_sel_n = 0;
        assert(aoe_game_view_alt_command_ror(&o, 100, 200, 4, 5) == 0 && g_log[0] == 0);
        o_sel_n = 1; c_target = &tgt; c_res = 1;
        assert(aoe_game_view_alt_command_ror(&o, 100, 200, 4, 5) == 1 && strcmp(g_log, "TOLCDG") == 0);
        g_log[0] = 0; c_target = NULL; c_pick_code = '2';
        assert(aoe_game_view_alt_command_ror(&o, 100, 200, 4, 5) == 0 && strcmp(g_log, "TPCDG") == 0);
        g_log[0] = 0; c_pick_code = '3'; c_res = 0;
        assert(aoe_game_view_alt_command_ror(&o, 100, 200, 4, 5) == 0 && strcmp(g_log, "TPKMDG") == 0);
        g_log[0] = 0; c_res = 1;
        assert(aoe_game_view_alt_command_ror(&o, 1, 2, 4, 5) == 1 && strcmp(g_log, "TPKMWADG") == 0);
        assert(c_mx == 5 && c_my == 7);
    }
}

static int16_t th_w(void *c, int32_t s) { (void)c; return (int16_t)(10 + s); }
static int16_t th_h(void *c, int32_t s) { (void)c; (void)s; return 5; }
static int th_loaded_v = 1; static void *th_art_v; static int16_t th_frame_v;
static int th_loaded(void *c, int32_t t) { (void)c; (void)t; return th_loaded_v; }
static void *th_art(void *c, int32_t t) { (void)c; assert(t == 3); return th_art_v; }
static int16_t th_frame(void *c, int32_t t, int32_t s, int32_t x, int32_t y)
{ (void)c; assert(t == 3 && s == 2 && x == 7 && y == 8); return th_frame_v; }
static int32_t th_px[3];
static int th_pixel(void *c, void *a, int32_t x, int32_t y, int32_t f)
{ (void)c; (void)a; th_px[0] = x; th_px[1] = y; th_px[2] = f; return x == 3; }

static void test_batch10(void)
{
    /* 0x005151d0 */
    AoeMainViewTerrainHitOpsRor o = {NULL, th_w, th_h, 4, th_loaded, th_art, th_frame, th_pixel};
    uint8_t tile[6] = {0, 0, 0, 0, 2, 0xe3};
    int32_t p[2] = {103, 51};
    int art;
    assert(aoe_main_view_terrain_hit_ror(&o, tile, 100, 50, 7, 8, p) == 1); /* no art */
    th_art_v = &art; th_frame_v = -1;
    assert(aoe_main_view_terrain_hit_ror(&o, tile, 100, 50, 7, 8, p) == 1);
    th_frame_v = 9;
    assert(aoe_main_view_terrain_hit_ror(&o, tile, 100, 50, 7, 8, p) == 1);
    assert(th_px[0] == 3 && th_px[1] == 1 && th_px[2] == 9);
    p[0] = 104;
    assert(aoe_main_view_terrain_hit_ror(&o, tile, 100, 50, 7, 8, p) == 0);
    p[0] = 112; /* width 12 */
    assert(aoe_main_view_terrain_hit_ror(&o, tile, 100, 50, 7, 8, p) == 0);
    p[0] = 111; p[1] = 55;
    assert(aoe_main_view_terrain_hit_ror(&o, tile, 100, 50, 7, 8, p) == 0);
    p[1] = 54; th_loaded_v = 0;
    assert(aoe_main_view_terrain_hit_ror(&o, tile, 100, 50, 7, 8, p) == 1);
    th_loaded_v = 1; tile[5] = 4; /* terrain 4 out of range */
    assert(aoe_main_view_terrain_hit_ror(&o, tile, 100, 50, 7, 8, p) == 1);
    p[0] = 110; p[1] = 50;
    assert(aoe_main_view_terrain_hit_ror(&o, NULL, 100, 50, 7, 8, p) == 0);
}

static char pc_log[256];
static uint8_t pc_code = '3';
static int pc_block, pc_build_ok = 1;
static int pc_base(void *c, int32_t x, int32_t y, int32_t f) { (void)c; assert(f == 1); return x + y; }
static uint8_t pc_pick(void *c, int32_t x, int32_t y, float p[2]) { (void)c; p[0] = (float)x; p[1] = (float)y; strcat(pc_log, "P;"); return pc_code; }
static void pc_snap(void *c, float p[2]) { (void)c; p[0] += 0.5f; strcat(pc_log, "S;"); }
static void pc_adjust(void *c, float p[2]) { (void)c; p[1] += 0.25f; strcat(pc_log, "A;"); }
static int pc_blocked(void *c, float x, float y) { (void)c; assert(x == 3.5f && y == 4.25f); strcat(pc_log, "B;"); return pc_block; }
static void pc_g10(void *c) { (void)c; strcat(pc_log, "G;"); }
static int pc_build(void *c, int16_t id, float x, float y) { (void)c; (void)id; assert(x == 3.5f && y == 4.25f); strcat(pc_log, "U;"); return pc_build_ok; }

static void test_place_click(void) {
    AoeMainViewPlaceOpsRor o = {NULL, 1, 7, 12, 1, pc_base, pc_pick, pc_snap, pc_adjust, pc_blocked, pc_g10, pc_build};
    pc_log[0] = 0;
    assert(aoe_main_view_place_click_ror(&o, 3, 4, 0) == 1 && !strcmp(pc_log, "P;S;A;B;U;"));
    pc_log[0] = 0;
    assert(aoe_main_view_place_click_ror(&o, 3, 4, 1) == 1 && !strcmp(pc_log, "P;S;A;B;G;U;"));
    pc_log[0] = 0;
    o.template_id = 0x6d;
    pc_build_ok = 0;
    assert(aoe_main_view_place_click_ror(&o, 3, 4, 0) == 0 && !strcmp(pc_log, "P;S;A;B;G;U;"));
    pc_log[0] = 0;
    pc_block = 1;
    assert(aoe_main_view_place_click_ror(&o, 3, 4, 1) == 0 && !strcmp(pc_log, "P;S;A;B;"));
    pc_log[0] = 0;
    o.template_id = -1;
    assert(aoe_main_view_place_click_ror(&o, 3, 4, 1) == 0 && !strcmp(pc_log, "P;S;"));
    pc_log[0] = 0;
    pc_code = '1';
    assert(aoe_main_view_place_click_ror(&o, 3, 4, 1) == 0 && !strcmp(pc_log, "P;"));
    o.mode_1b4 = 4;
    assert(aoe_main_view_place_click_ror(&o, 3, 4, 1) == 7);
    o.editing = 0;
    assert(aoe_main_view_place_click_ror(&o, 3, 4, 1) == 0);
}


static AoeViewClassNodeRor cn_nodes[4] = {{10, 0, 1, 5, 5}, {11, 0, 2, 7, 5}, {12, 0, 1, 20, 20}, {13, 0, 9, 5, 6}};
static AoeViewClassListRor cn_list = {cn_nodes, 4};
static const AoeViewClassListRor *cn_get(void *c, int32_t player, int32_t cls) { (void)c; return player == 1 && cls == 3 ? &cn_list : NULL; }
static uint8_t cn_dist[16][16];

static void test_collect_nearby(void)
{
    AoeViewClassListsRor l = {NULL, 2, cn_dist, cn_get};
    static AoeViewNearbyRor out;
    uint8_t kinds[256];
    int32_t classes[2] = {3, 4}, dx, dy;
    for (dy = 0; dy < 16; ++dy) for (dx = 0; dx < 16; ++dx) cn_dist[dy][dx] = (uint8_t)(dx > dy ? dx : dy);
    memset(kinds, 0, sizeof kinds); kinds[1] = 0; kinds[2] = 4; kinds[9] = 7; /* 7: no bucket */
    assert(aoe_view_collect_nearby_ror(&l, 5, 5, 3, 1, classes, 2, kinds, &out) == 2);
    assert(out.count[0] == 1 && out.entry[0][0].object == 10 && out.entry[0][0].byte_4 == 0 &&
           out.count[4] == 1 && out.entry[4][0].object == 11 && out.entry[4][0].byte_4 == 2);
    assert(aoe_view_collect_nearby_ror(&l, 5, 5, 1, 1, classes, 2, kinds, &out) == 1 && out.count[4] == 0);
    assert(aoe_view_collect_nearby_ror(&l, 5, 5, 3, 2, classes, 2, kinds, &out) == 0);
    assert(aoe_view_collect_nearby_ror(&l, 5, 5, 3, 1, classes, 0, kinds, &out) == 0);
}

static int sv_surfaces, sv_inits, sv_clears, sv_rows_new, sv_rows_free, sv_dirty, sv_regions, sv_region_free, sv_game_region, sv_switch, sv_redraw, sv_base, sv_layout;
static int32_t sv_rows_args[6], sv_dirty_rect[4], sv_gr[4];
static void *sv_surface_create(void *c, const char *n) { (void)c; assert(strcmp(n, "Terrain Buffer") == 0); ++sv_surfaces; return (void *)0x51; }
static int sv_surface_init(void *c, void *s, void *p, int32_t w, int32_t h) { (void)c; (void)s; (void)p; (void)w; (void)h; ++sv_inits; return 1; }
static void sv_surface_clear(void *c, void *s, int32_t w, int32_t h) { (void)c; (void)s; (void)w; (void)h; ++sv_clears; }
static void sv_origin(void *c, void *s, int32_t x, int32_t y) { (void)c; (void)s; assert(x == 0 && y == 0); }
static void sv_rows_free_f(void *c, void *r) { (void)c; (void)r; ++sv_rows_free; }
static void *sv_rows_new_f(void *c, int32_t a, int32_t b, int32_t d, int32_t e, int32_t f, int32_t g) {
    (void)c; sv_rows_args[0] = a; sv_rows_args[1] = b; sv_rows_args[2] = d; sv_rows_args[3] = e; sv_rows_args[4] = f; sv_rows_args[5] = g; return (void *)(intptr_t)(0x60 + ++sv_rows_new);
}
static void sv_dirty_f(void *c, void *r, int32_t x0, int32_t y0, int32_t x1, int32_t y1) { (void)c; (void)r; ++sv_dirty; sv_dirty_rect[0] = x0; sv_dirty_rect[1] = y0; sv_dirty_rect[2] = x1; sv_dirty_rect[3] = y1; }
static void sv_base_setup(void *c, int32_t x, int32_t y, int32_t w, int32_t h) { (void)c; (void)x; (void)y; (void)w; (void)h; ++sv_base; }
static void sv_layout_f(void *c) { (void)c; ++sv_layout; }
static void sv_region_free_f(void *c, void *r) { (void)c; (void)r; ++sv_region_free; }
static void *sv_region_new_f(void *c, int32_t w, int32_t h) { (void)c; (void)w; (void)h; return (void *)(intptr_t)(0x70 + ++sv_regions); }
static int sv_gr_enabled(void *c) { (void)c; return 1; }
static void sv_game_region_f(void *c, int32_t a, int32_t b, int32_t d, int32_t e) { (void)c; ++sv_game_region; sv_gr[0] = a; sv_gr[1] = b; sv_gr[2] = d; sv_gr[3] = e; }
static void sv_switch_f(void *c, int32_t on) { (void)c; sv_switch = on; }
static void sv_redraw_f(void *c, int32_t l) { (void)c; sv_redraw = l; }
static const AoeMainViewSetupOpsRor sv_ops = {NULL, sv_surface_create, sv_surface_init, sv_surface_clear, sv_origin,
    sv_rows_free_f, sv_rows_new_f, sv_dirty_f, sv_base_setup, sv_layout_f, sv_region_free_f, sv_region_new_f,
    sv_gr_enabled, sv_game_region_f, sv_switch_f, sv_redraw_f};
static int pv_hit = 1, pv_place = 1, pv_graphic = 1, pv_drawn_x, pv_drawn_y; static uint32_t pv_flags;
static int pv_tile(void *c, int32_t x, int32_t y, int32_t *tx, int32_t *ty) { (void)c; (void)x; (void)y; *tx = 4; *ty = 6; return pv_hit; }
static void pv_corners(void *c, float fx, float fy, int16_t o[8]) { (void)c; assert(fx == 4.5f && fy == 6.5f); o[0] = 100; o[1] = 50; o[2] = 0; o[3] = 0; o[4] = 140; o[5] = 70; o[6] = 0; o[7] = 0; }
static int pv_can(void *c, float fx, float fy) { (void)c; (void)fx; (void)fy; return pv_place; }
static uint32_t pv_tflags(void *c) { (void)c; return pv_flags; }
static int pv_has(void *c) { (void)c; return pv_graphic; }
static void pv_draw(void *c, int32_t x, int32_t y) { (void)c; pv_drawn_x = x; pv_drawn_y = y; }
static const AoeMainViewPreviewOpsRor pv_ops = {NULL, pv_tile, pv_corners, pv_can, pv_tflags, pv_has, pv_draw};

static void test_setup_and_preview(void)
{
    AoeMainViewRor v; AoeMainViewBuffersRor b; int32_t code = -1, py = -1;
    memset(&v, 0, sizeof v); memset(&b, 0, sizeof b);
    b.draw_rows[0] = (void *)1;
    aoe_main_view_setup_ror(&v, &b, &sv_ops, 10, 20, 300, 200);
    assert(sv_base == 1 && sv_layout == 1 && b.width == 300 && b.height == 200);
    assert(sv_surfaces == 1 && sv_inits == 1 && sv_clears == 1 && sv_rows_free == 1 && sv_rows_new == 2 &&
           sv_rows_args[0] == (200 + 0x1c2) * 2 && sv_rows_args[1] == 200 + 0x1c2 && sv_rows_args[2] == 0x40 &&
           sv_rows_args[3] == -200 && sv_rows_args[4] == 200 + 0xf9 && sv_rows_args[5] == 0x28 &&
           sv_dirty == 2 && sv_dirty_rect[2] == 299 && sv_dirty_rect[3] == 199);
    assert(v.point_x == AOE_MAIN_VIEW_NO_POINT_ROR && sv_regions == 3 && sv_region_free == 0 &&
           sv_game_region == 1 && sv_gr[2] == 310 && sv_gr[3] == 220 && sv_switch == 1 && sv_redraw == 2);
    /* A zero-size view keeps no overlay but reports success. */
    memset(&v, 0, sizeof v); memset(&b, 0, sizeof b);
    assert(aoe_main_view_rebuild_overlay_ror(&v, &b, &sv_ops) == 1 && v.overlay == NULL);
    /* The placement preview: code 0x269 for v = 2 (0x26 << 4 | 9) during
       the draw, 5 when refused, cleared after; the midpoint drawn. */
    pv_flags = 2u << 7;
    assert(aoe_main_view_place_preview_ror(&pv_ops, 1, 2, &code, &py) == 1 && code == 0 &&
           pv_drawn_x == 121 && pv_drawn_y == 61 && py == 61);
    pv_graphic = 0; pv_drawn_x = 0;
    aoe_main_view_place_preview_ror(&pv_ops, 1, 2, &code, &py);
    assert(pv_drawn_x == 0);
    pv_hit = 0;
    assert(aoe_main_view_place_preview_ror(&pv_ops, 1, 2, &code, &py) == 1 && code == 0);
}
static int ep_ok, ep_reloc, ep_x, ep_y, ep_rel; static uint32_t ep_shape, ep_face;
static int ep_true(void *c) { (void)c; return 1; }
static void ep_mouse(void *c, int32_t *x, int32_t *y) { (void)c; *x = 50; *y = 60; }
static int ep_pick(void *c, int32_t x, int32_t y) { (void)c; return x == 40 && y == 40; }
static int ep_tile(void *c, int32_t x, int32_t y, int32_t *tx, int32_t *ty) { (void)c; (void)x; (void)y; *tx = 3; *ty = 4; return 1; }
static void ep_corners(void *c, float fx, float fy, int16_t o[8]) { (void)c; (void)fx; (void)fy; o[0] = 10; o[1] = 20; o[4] = 30; o[5] = 40; }
static int ep_can(void *c, float fx, float fy) { (void)c; assert(fx == 3.5f && fy == 4.5f); return ep_ok; }
static uint32_t ep_flags(void *c) { (void)c; return 5u << 7; }
static uint8_t ep_facing(void *c) { (void)c; return 3; }
static uint8_t ep_shape_cb(void *c) { (void)c; return 0x42; }
static int ep_relocating(void *c) { (void)c; return ep_reloc; }
static void ep_draw(void *c, int32_t x, int32_t y, int r, uint32_t f, uint32_t s) { (void)c; ep_x = x; ep_y = y; ep_rel = r; ep_face = f; ep_shape = s; }
static void test_editor_preview(void)
{
    AoeMainViewEditorPreviewOps o = {0, ep_true, ep_mouse, ep_pick, ep_tile, ep_corners, ep_can,
                                     ep_flags, ep_true, ep_facing, ep_shape_cb, ep_relocating, ep_draw};
    int32_t code = 7, py = 0;
    ep_ok = 1; ep_reloc = 1;
    assert(aoe_main_view_editor_preview(&o, 10, 20, 100, 100, &code, &py) == 1);
    assert(code == 0 && py == 31 && ep_x == 21 && ep_y == 31);
    assert(ep_rel == 1 && ep_face == 1 && ep_shape == 0x97);
    ep_ok = 0;
    assert(aoe_main_view_editor_preview(&o, 10, 20, 100, 100, &code, &py) == 1);
    assert(ep_shape == 0x42);
    assert(aoe_main_view_editor_preview(&o, 60, 20, 100, 100, &code, &py) == 0);
}

static int rc_log;
static void rc_cancel(void *c) { (void)c; rc_log |= 1; }
static void rc_prep(void *c) { (void)c; rc_log |= 2; }
static int rc_yes(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return 1; }
static void rc_note(void *c) { (void)c; rc_log |= 4; }
static int rc_busy(void *c) { (void)c; return 0; }
static void rc_order(void *c, int32_t x, int32_t y) { (void)c; assert(x == 5 && y == 6); rc_log |= 8; }
static int rc_find(void *c, int32_t u) { (void)c; return u == 9; }
static void rc_rally(void *c) { (void)c; rc_log |= 16; }
static void rc_box(void *c) { (void)c; rc_log |= 32; }
static void rc_redraw(void *c) { (void)c; rc_log |= 64; }
static void rc_reset(void *c) { (void)c; rc_log |= 128; }
static void rc_unsel(void *c) { (void)c; rc_log |= 256; }
static void test_right_click(void)
{
    AoeMainViewRightClickOps o = {0, 3, 0, 0, 1, 1, 1, rc_cancel, rc_prep, rc_yes, rc_note, rc_busy,
                                  rc_order, rc_find, rc_yes, rc_rally, rc_box, rc_redraw, rc_reset, rc_unsel};
    assert(aoe_main_view_right_click(&o, 5, 6, 0) == 0);
    o.mode = 4; o.drag_pending = 1; rc_log = 0;
    assert(aoe_main_view_right_click(&o, 5, 6, 0) == 1 && rc_log == 1);
    o.drag_pending = 0; rc_log = 0;
    assert(aoe_main_view_right_click(&o, 5, 6, 0) == 1 && rc_log == (2 | 8));
    rc_log = 0;
    assert(aoe_main_view_right_click(&o, 5, 6, 9) == 1 && rc_log == (2 | 16 | 64));
    o.sub_mode = 0x15; rc_log = 0;
    assert(aoe_main_view_right_click(&o, 5, 6, 0) == 1 && rc_log == (2 | 32 | 64 | 128));
    o.sub_mode = 0x14; rc_log = 0;
    assert(aoe_main_view_right_click(&o, 5, 6, 0) == 1 && rc_log == (2 | 4));
    o.sub_mode = 0; o.full_player = 0; rc_log = 0;
    assert(aoe_main_view_right_click(&o, 5, 6, 0) == 1 && rc_log == (2 | 256 | 64));
    o.selection_kind = 3;
    assert(aoe_main_view_right_click(&o, 5, 6, 0) == 0);
}


static void test_queue_scroll(void)
{
    AoeMainViewRor v; AoeMainViewBlitQueueRor q = {NULL, 0};
    int32_t src[4] = {0, 0, 99, 49}, clip[4] = {0, 0, 199, 99}, far[4] = {500, 500, 600, 600};
    memset(&v, 0, sizeof v); v.left = 7; v.top = 9;
    assert(aoe_main_view_queue_scroll_ror(&v, &q, src, clip, 0, 0) == 0);
    /* dy = 10: 5 strips (49 + 10) / 10, each moved up by 10. */
    assert(aoe_main_view_queue_scroll_ror(&v, &q, src, clip, 0, 10) == 5 && q.capacity == 5);
    assert(v.blits_active == 1 && v.blit_count == 5 && v.blit_index == -1 && v.blit_off_x == 7 && v.blit_off_y == 9);
    assert(q.list[0].top == 0 && q.list[0].dest_y == -10 && q.list[0].bottom == 49 && q.list[1].top == 10 && q.list[4].top == 40);
    /* dx = -20: 5 strips starting at the shifted first column 80. */
    assert(aoe_main_view_queue_scroll_ror(&v, &q, src, clip, -20, 0) == 5 && q.list[0].left == 80 && q.list[0].dest_x == 100 && q.list[1].left == 60);
    /* Disjoint rectangles: one blit of the whole view onto the clip. */
    assert(aoe_main_view_queue_scroll_ror(&v, &q, src, far, 3, 0) == 1 && q.list[0].dest_x == 500 && q.list[0].dest_bottom == 600);
    free(q.list);
}

static int gd_base, gd_hl[4]; static uint32_t gd_count, gd_size;
static void gd_basef(void *c) { (void)c; ++gd_base; }
static void gd_hlf(void *c, int32_t a, int32_t b, int32_t d, int32_t e) { (void)c; gd_hl[0] = a; gd_hl[1] = b; gd_hl[2] = d; gd_hl[3] = e; }
static void *gd_alloc(void *c, uint32_t n, uint32_t s) { (void)c; gd_count = n; gd_size = s; return calloc(n, s); }
static void game_view_default_tests(void) {
    AoeGameViewDefaultsRor v; int32_t g0 = 7, g1 = 9;
    AoeGameViewDefaultOpsRor o = {NULL, gd_basef, gd_hlf, gd_alloc, {&g0, &g1}};
    memset(&v, 0x5a, sizeof v);
    aoe_game_view_construct_default_ror(&v, &o);
    assert(gd_base == 1 && gd_hl[0] == -1 && gd_hl[3] == -1 && gd_count == 8 && gd_size == 0x14);
    assert(v.link == NULL && v.shorts_108[19] == 0 && v.point_x == AOE_MAIN_VIEW_NO_POINT_ROR && v.point_y == AOE_MAIN_VIEW_NO_POINT_ROR);
    assert(v.field_154 == -1 && v.field_16c == -1 && v.field_170 == -1 && v.field_340 == INT32_MIN && v.red_pen == NULL);
    assert(v.timers && v.timer_count == 0 && v.timer_capacity == 8 && v.list_count[4] == 0 && v.overlay == NULL);
    assert(g0 == 0 && g1 == 0);
    free(v.timers);
}

static char ec_log[256]; static int ec_n, ec_point = 1, ec_cmd = 1, ec_objcmd = 1, ec_repair = 1; static int32_t ec_obj = 9;
static int32_t ec_base(void *c, int32_t x, int32_t y, int32_t a, int32_t b) { (void)c; (void)x; (void)y; (void)a; (void)b; ec_n += snprintf(ec_log + ec_n, sizeof ec_log - (size_t)ec_n, "base "); return 7; }
static int ec_pointf(void *c, int32_t x, int32_t y, int32_t *px, int32_t *py) { (void)c; *px = x / 2; *py = y / 2; return ec_point; }
static void ec_clamp(void *c, int32_t *px, int32_t *py) { (void)c; if (*px > 50) *px = 50; (void)py; }
static int ec_c6d(void *c, int32_t px, int32_t py) { (void)c; ec_n += snprintf(ec_log + ec_n, sizeof ec_log - (size_t)ec_n, "6d(%d,%d) ", (int)px, (int)py); return ec_cmd; }
static int ec_c6f(void *c, int32_t px, int32_t py) { (void)c; ec_n += snprintf(ec_log + ec_n, sizeof ec_log - (size_t)ec_n, "6f(%d,%d) ", (int)px, (int)py); return ec_cmd; }
static int ec_objc(void *c, int32_t x, int32_t y, int32_t code) { (void)c; (void)x; (void)y; ec_n += snprintf(ec_log + ec_n, sizeof ec_log - (size_t)ec_n, "oc%x ", (unsigned)code); return ec_objcmd; }
static int32_t ec_objat(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return ec_obj; }
static int ec_repairf(void *c, int32_t id) { (void)c; (void)id; return ec_repair; }
static void ec_step(void *c, int32_t id, int32_t a, int32_t b) { (void)c; ec_n += snprintf(ec_log + ec_n, sizeof ec_log - (size_t)ec_n, "%d(%d,%d) ", (int)id, (int)a, (int)b); }
static void edit_click_tests(void) {
    AoeViewEditClickOpsRor o = {NULL, 4, 100, 0, 1, 10, 20, ec_base, ec_pointf, ec_clamp, ec_c6d, ec_c6f, ec_objc, ec_objat, ec_repairf, ec_step};
    assert(aoe_view_edit_click_ror(&o, 200, 30, 0, 0) == 1 && strcmp(ec_log, "1(0,0) 6d(50,15) 2(0,0) 3(210,50) 4(0,0) 8(0,0) ") == 0);
    ec_n = 0; ec_cmd = 0; o.edit_mode = 0x69;
    assert(aoe_view_edit_click_ror(&o, 20, 30, 0, 0) == 1 && strcmp(ec_log, "1(0,0) 6f(10,15) 5(0,0) 8(0,0) ") == 0);
    ec_n = 0; o.edit_mode = 0x66;
    assert(aoe_view_edit_click_ror(&o, 20, 30, 0, 0) == 1 && strcmp(ec_log, "1(0,0) oc69 8(0,0) ") == 0);
    ec_n = 0; o.edit_mode = 0x68;
    assert(aoe_view_edit_click_ror(&o, 20, 30, 0, 0) == 1 && strcmp(ec_log, "1(0,0) 6(9,0) 7(0,0) 4(0,0) 8(0,0) ") == 0);
    ec_n = 0; ec_repair = 0;
    assert(aoe_view_edit_click_ror(&o, 20, 30, 0, 0) == 1 && strcmp(ec_log, "1(0,0) 5(0,0) 8(0,0) ") == 0);
    ec_n = 0; o.mode_ec = 3;
    assert(aoe_view_edit_click_ror(&o, 20, 30, 0, 0) == 1 && strcmp(ec_log, "1(0,0) ") == 0);
    ec_n = 0; o.edit_mode = 5;
    assert(aoe_view_edit_click_ror(&o, 20, 30, 0, 0) == 7 && strcmp(ec_log, "base ") == 0);
    ec_n = 0; o.edit_mode = 100; o.comm_value = 1;
    assert(aoe_view_edit_click_ror(&o, 20, 30, 0, 0) == 7);
    o.game_state = 2;
    assert(aoe_view_edit_click_ror(&o, 20, 30, 0, 0) == 0);
}

static char mv_log[256]; static int mv_n, mv_over = 1, mv_point = 1; static float mv_vx = 10.0f, mv_vy = 20.0f; static int32_t mv_unit_ok = 1;
static int mv_overf(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return mv_over; }
static int mv_pointf(void *c, int32_t x, int32_t y, float *px, float *py) { (void)c; *px = (float)x; *py = (float)y; return mv_point; }
static void mv_setview(void *c, float x, float y) { (void)c; mv_vx = x; mv_vy = y; mv_n += snprintf(mv_log + mv_n, sizeof mv_log - (size_t)mv_n, "v(%g,%g) ", (double)x, (double)y); }
static int32_t mv_step(void *c, int32_t id, int32_t a, int32_t b, int32_t d, int32_t e) { (void)c; mv_n += snprintf(mv_log + mv_n, sizeof mv_log - (size_t)mv_n, "%d(%d,%d,%d,%d) ", (int)id, (int)a, (int)b, (int)d, (int)e); return id == AOE_VIEW_MOVE_UNIT ? mv_unit_ok : 0; }
static void mouse_move_tests(void) {
    int32_t lx = 100, ly = 100, px = 100, py = 100; int16_t p120 = 0, p122 = 0;
    AoeViewMoveOpsRor o = {NULL, 0, &lx, &ly, &px, &py, 0, 0, 640, 400, &mv_vx, &mv_vy, &p120, &p122, 1, 1, 0, mv_overf, mv_pointf, mv_setview, mv_step};
    assert(aoe_view_mouse_move_ror(&o, 100, 100) == 0 && mv_n == 0);
    /* Kind 0 always counts as near: a 16-pixel step scrolls by 1 tile and warps the cursor back. */
    assert(aoe_view_mouse_move_ror(&o, 116, 100) == 1 && strcmp(mv_log, "v(11,21) 5(0,0,0,0) 6(1,0,0,0) ") == 0 && lx == 100 && px == 100);
    /* Kind 3 away from the edges: no scroll, just the highlight; the view is unchanged -> 0. */
    mv_n = 0; o.kind = 3; mv_vx = 11.0f;
    assert(aoe_view_mouse_move_ror(&o, 120, 110) == 0 && strcmp(mv_log, "2(120,110,0,0) ") == 0 && px == 120 && py == 110);
    /* Kind 3 at the left edge moving outward: scrolls, and a far press point warps the cursor. */
    mv_n = 0; lx = 30; ly = 110; px = 300; py = 300;
    assert(aoe_view_mouse_move_ror(&o, 14, 110) == 1 && strcmp(mv_log, "v(10,20) 5(0,0,0,0) 2(14,110,0,0) 6(1,0,0,0) ") == 0 && lx == 300 && ly == 300);
    /* Kind 7: shorts; kind 8: view by half the step. */
    mv_n = 0; o.kind = 7; lx = 100; ly = 100; px = 100; py = 100;
    assert(aoe_view_mouse_move_ror(&o, 104, 96) == 1 && p122 == 0 && p120 == 4 && strcmp(mv_log, "6(1,0,0,0) ") == 0 && lx == 104);
    mv_n = 0; o.kind = 8; mv_vx = 10.0f; mv_vy = 20.0f;
    assert(aoe_view_mouse_move_ror(&o, 110, 106) == 1 && strcmp(mv_log, "v(5,25) v(8,28) 5(0,0,0,0) 6(1,0,0,0) ") == 0 && lx == 100);
    mv_n = 0;
    assert(aoe_view_mouse_move_ror(&o, 106, 104) == 1 && strcmp(mv_log, "v(6,30) v(9,33) 6(1,0,0,0) ") == 0 && lx == 106);
    /* Kind 2 pulls the point; kind 6 jumps the view; kind 4 moves the unit; kind 9 reports the rectangle. */
    mv_n = 0; o.kind = 2;
    assert(aoe_view_mouse_move_ror(&o, 5, 5) == 0 && strcmp(mv_log, "1(0,0,0,0) 6(1,0,0,0) ") == 0 && lx == 5);
    mv_n = 0; o.kind = 6;
    assert(aoe_view_mouse_move_ror(&o, 7, 9) == 1 && strcmp(mv_log, "v(7.5,9.5) 6(1,0,0,0) ") == 0);
    mv_n = 0; o.kind = 4; lx = 200; ly = 200; px = 200; py = 200; mv_unit_ok = 0;
    assert(aoe_view_mouse_move_ror(&o, 204, 200) == 0 && strcmp(mv_log, "3(204,200,0,0) 6(2,0,0,0) ") == 0);
    mv_n = 0; o.kind = 9; lx = 200;
    assert(aoe_view_mouse_move_ror(&o, 210, 205) == 0 && strcmp(mv_log, "4(200,200,210,205) 6(1,0,0,0) ") == 0);
}

/* 0x005145a0: four nodes in one row: A id 1 box 0..10 (a skip run at the point: class 2, depth 5),
   B id 2 box 0..10 (a colour run: class 1, depth 3), C id 1 again box 14..20 (class 4 by the
   slack, depth 9, dropped for A's better class), D id 3 box 2..8 (class 4 by the slack, depth 7). */
typedef struct { int32_t id; int16_t l, t, r, b, depth; uint8_t px; const void *next; } po_node;
static po_node po_nodes[4];
static uint8_t po_data[2][162]; /* 20 row offsets (all 160), 20 outlines (0), then one command */
static const void *po_head(void *c, int32_t row) { (void)c; return row == 7 ? &po_nodes[0] : NULL; }
static void po_item(void *c, const void *n, AoeViewDrawItemRor *o) {
    const po_node *p = (const po_node *)n; static AoeShapeHitFrameRor fr; int k;
    (void)c;
    for (k = 0; k < 20; ++k) { po_data[0][k * 4] = po_data[1][k * 4] = 160; }
    po_data[0][160] = 0x51; po_data[1][160] = 0x50; /* skip 20 / colour run 20 */
    memset(o, 0, sizeof *o); o->next = p->next; o->object_id = p->id; o->left = p->l; o->top = p->t; o->right = p->r; o->bottom = p->b; o->depth = p->depth;
    fr.cmd_table_offset = 0; fr.outline_offset = 80; fr.width = 20; fr.height = 20; o->frame = &fr;
    o->shape = po_data[p->px]; o->shape_size = sizeof po_data[0]; o->key = 0;
}
static void point_objects_tests(void) {
    AoeMainViewRor v; AoeViewDrawRowsOpsRor rows = {NULL, po_head, po_item, NULL};
    static AoeViewHitEntryRor store[5][64]; AoeViewHitListsRor lists; AoeViewPickedRor out[64];
    int c, n;
    memset(&v, 0, sizeof v); v.left = 100; v.top = 50; v.right = 300; v.bottom = 250;
    for (c = 0; c < 5; ++c) lists.lists[c] = store[c];
    po_nodes[0] = (po_node){1, 0, 0, 10, 10, 5, 0, &po_nodes[1]};
    po_nodes[1] = (po_node){2, 0, 0, 10, 10, 3, 1, &po_nodes[2]};
    po_nodes[2] = (po_node){1, 14, 14, 20, 20, 9, 0, &po_nodes[3]};
    po_nodes[3] = (po_node){3, 2, 2, 8, 8, 7, 0, NULL};
    assert(aoe_main_view_point_objects_ror(&v, &rows, &lists, 130, 90, 0, 7, 0, 4, 0, out) == 0);
    n = (int)aoe_main_view_point_objects_ror(&v, &rows, &lists, 110, 60, 0, 7, 5, 4, 0, out);
    assert(n == 3 && out[0].id == 2 && out[0].count == 1 && out[0].field_6 == 3 && out[1].id == 1 && out[1].count == 2 && out[1].field_6 == 5 && out[2].id == 3 && out[2].count == 4 && out[2].field_6 == 7);
    assert(lists.counts[1] == 1 && lists.counts[2] == 1 && lists.counts[3] == 0 && lists.counts[4] == 1);
    /* Class limit 2 drops D; without the slack A, B and D are class 3 (D's box misses the point); a
       reversed row range or a point outside gives 0. */
    assert(aoe_main_view_point_objects_ror(&v, &rows, &lists, 110, 60, 0, 7, 5, 2, 0, out) == 2);
    n = (int)aoe_main_view_point_objects_ror(&v, &rows, &lists, 110, 60, 0, 7, 0, 4, 0, out);
    assert(n == 2 && out[0].id == 2 && out[1].id == 1);
    assert(aoe_main_view_point_objects_ror(&v, &rows, &lists, 110, 60, 8, 7, 5, 4, 0, out) == 0);
    assert(aoe_main_view_point_objects_ror(&v, &rows, &lists, 99, 60, 0, 7, 5, 4, 0, out) == 0);
    /* Two nodes of one ID in the same class keep the deeper one: a copy of B at depth 8 replaces B. */
    po_nodes[3] = (po_node){2, 0, 0, 10, 10, 8, 1, NULL};
    n = (int)aoe_main_view_point_objects_ror(&v, &rows, &lists, 110, 60, 0, 7, 0, 4, 0, out);
    assert(n == 2 && out[0].id == 2 && out[0].field_6 == 8 && lists.counts[1] == 1); /* the freed slot is reused */
}

static void grid_lookup_tests(void) {
    static const int32_t t0[] = {0, 0, 0, 999}, t1[] = {0, -3, 2, 1, 1, 1, 999}, t2[] = {1, 0, 40, 17, 0, 0, 999}, tn[] = {999};
    const int32_t *tables[17] = {t0, t1, t2, tn, tn, tn, tn, tn, tn, tn, tn, tn, tn, tn, tn, tn, tn};
    AoeViewGridRor g; memset(&g, 0, sizeof g);
    aoe_view_grid_build_lookup_ror(&g, tables);
    assert(g.lookup && g.lookup[0] == 0 && g.lookup[1] == 1 && g.lookup[2] == 1 && g.lookup[3] == -1);
    assert(g.lookup[16] == 2 && g.lookup[17] == 1 && g.lookup[18] == 2 && g.lookup[31] == 2 && g.lookup[32] == -1);
    free(g.lookup);
}

/* 0x00518720 / 0x00518a30 recorder. */
static int32_t gs_state = 4, gs_edit; static int gs_comm, gs_fore = 1, gs_modal, gs_focus, gs_buttons, gs_keys[4]; static int32_t gs_cur[2] = {5, 300}; static uint32_t gs_now = 1000;
static float gs_vx = 100.0f, gs_vy = 100.0f; static char gs_log[256];
static int32_t gs_game_state(void *c) { (void)c; return gs_state; }
static int32_t gs_edit_mode(void *c) { (void)c; return gs_edit; }
static int gs_comm_waiting(void *c) { (void)c; return gs_comm; }
static int gs_foreground(void *c) { (void)c; return gs_fore; }
static int gs_modal_q(void *c) { (void)c; return gs_modal; }
static int gs_focus_q(void *c) { (void)c; return gs_focus; }
static void gs_rect(void *c, int32_t r[4]) { (void)c; r[0] = 0; r[1] = 0; r[2] = 640; r[3] = 480; }
static void gs_cursor(void *c, int32_t *x, int32_t *y) { (void)c; *x = gs_cur[0]; *y = gs_cur[1]; }
static int gs_buttons_q(void *c) { (void)c; return gs_buttons; }
static void gs_keys_q(void *c, int *l, int *u, int *r, int *d) { (void)c; *l = gs_keys[0]; *u = gs_keys[1]; *r = gs_keys[2]; *d = gs_keys[3]; }
static uint32_t gs_time(void *c) { (void)c; return gs_now; }
static void gs_view_point(void *c, float *x, float *y) { (void)c; *x = gs_vx; *y = gs_vy; }
static void gs_set_view_point(void *c, float x, float y) { (void)c; gs_vx = x; gs_vy = y; }
static void gs_held_move(void *c, int sx, int sy) { (void)c; size_t n = strlen(gs_log); snprintf(gs_log + n, sizeof gs_log - n, "held(%d,%d) ", sx, sy); }
static void gs_redraw(void *c) { (void)c; strcat(gs_log, "redraw "); }
static void scroll_tests(void) {
    AoeGameViewScrollOpsRor o = {NULL, gs_game_state, gs_edit_mode, gs_comm_waiting, 1, 20, 100, 3.0f, 2.0f, 0.5f, gs_foreground, gs_modal_q, gs_focus_q, gs_rect, gs_cursor, gs_buttons_q, gs_keys_q, gs_time, 1, 1, 0, gs_view_point, gs_set_view_point, gs_held_move, gs_redraw};
    AoeGameViewScrollRor st; memset(&st, 0, sizeof st);
    int32_t a = 1;
    /* The speed: first tick 1.0; then 250 ms / 100 = 2.5 -> 2.5; 2.4 -> 2.0; capped at 3. */
    assert(aoe_game_view_scroll_speed_ror(&a, 1250, 1000, 100, 3.0f) == 2.5f && aoe_game_view_scroll_speed_ror(&a, 1240, 1000, 100, 3.0f) == 2.0f && aoe_game_view_scroll_speed_ror(&a, 1900, 1000, 100, 3.0f) == 3.0f);
    a = 0; assert(aoe_game_view_scroll_speed_ror(&a, 9, 0, 100, 3.0f) == 1.0f && a == 1);
    /* The cursor at the left edge: the first tick moves by (-1, -1) and stamps the time. */
    gs_log[0] = '\0';
    assert(aoe_game_view_edge_scroll_ror(&st, &o) == 1 && gs_vx == 99.0f && gs_vy == 99.0f && st.edge_time == 1000 && strcmp(gs_log, "redraw ") == 0);
    /* 250 ms later, bottom-left corner: speed 2.5: x step (-2.5, -2.5), then y +1 step (-5, +5). */
    gs_now = 1250; gs_cur[1] = 470;
    assert(aoe_game_view_edge_scroll_ror(&st, &o) == 1 && gs_vx == 91.5f && gs_vy == 101.5f);
    /* A mouse button, a modal screen, a key scroll in progress or the cursor inside clear the flag. */
    gs_buttons = 1; assert(aoe_game_view_edge_scroll_ror(&st, &o) == 0 && st.edge_active == 0); gs_buttons = 0;
    gs_cur[0] = 300; gs_cur[1] = 200; assert(aoe_game_view_edge_scroll_ror(&st, &o) == 0);
    gs_cur[0] = 5; st.key_active = 1; assert(aoe_game_view_edge_scroll_ror(&st, &o) == 0); st.key_active = 0;
    gs_modal = 1; assert(aoe_game_view_edge_scroll_ror(&st, &o) == 0); gs_modal = 0;
    /* Keys: right + up: first tick speed 1: (+1, +1) then (+2, -2). */
    gs_keys[2] = 1; gs_keys[1] = 1; gs_vx = gs_vy = 50.0f; gs_log[0] = '\0';
    assert(aoe_game_view_key_scroll_ror(&st, &o) == 1 && gs_vx == 53.0f && gs_vy == 49.0f && st.key_active == 1 && st.key_time == 1250 && strcmp(gs_log, "redraw ") == 0);
    /* Edit mode 0x12 with a held object moves it instead and clears the flag. */
    gs_edit = 0x12; o.has_held = 1; gs_log[0] = '\0';
    assert(aoe_game_view_key_scroll_ror(&st, &o) == 1 && strcmp(gs_log, "held(1,-1) held(1,1) redraw ") == 0 && st.key_active == 0);
    gs_keys[2] = gs_keys[1] = 0; gs_edit = 0; o.has_held = 0;
    assert(aoe_game_view_key_scroll_ror(&st, &o) == 0);
    gs_comm = 1; st.key_active = 1; assert(aoe_game_view_key_scroll_ror(&st, &o) == 0 && st.key_active == 0); gs_comm = 0;
}

/* 0x0051a650 / 0x0051aa10 recorder: objects 1..4 with kinds 1, 2, 3, 3 at tiles (1, 1)..; records in order. */
typedef struct { uint8_t state, kind; float x, y; int32_t pri; int visible; } gp_obj;
static gp_obj gp_objs[5] = {{0}, {2, 1, 1.5f, 1.5f, 5, 1}, {2, 2, 2.5f, 2.5f, 9, 0}, {2, 3, 3.5f, 3.5f, 7, 1}, {8, 3, 4.5f, 4.5f, 7, 1}};
static int32_t gp_ids[4] = {1, 2, 3, 4}; static int gp_n = 4; static uint8_t gp_fog = 0x0f;
static int32_t gp_point_objects(void *c, int32_t x, int32_t y, int32_t lo, int32_t hi, AoeViewPickedRor out[64]) { int i; (void)c; (void)x; (void)y; (void)lo; (void)hi; for (i = 0; i < gp_n; ++i) { out[i].id = gp_ids[i]; out[i].count = 1; out[i].field_14 = (int16_t)(10 * i); out[i].field_18 = (int16_t)(20 * i); } return gp_n; }
static void *gp_object(void *c, int32_t id, uint8_t *state, uint8_t *kind, float *x, float *y) { (void)c; if (id < 1 || id > 4) return NULL; *state = gp_objs[id].state; *kind = gp_objs[id].kind; *x = gp_objs[id].x; *y = gp_objs[id].y; return &gp_objs[id]; }
static int gp_visible(void *c, void *obj) { (void)c; return ((gp_obj *)obj)->visible; }
static uint8_t gp_fog_q(void *c, int32_t x, int32_t y) { (void)c; (void)x; (void)y; return gp_fog; }
static int32_t gp_priority(void *c, void *obj, int32_t cls) { (void)c; (void)cls; return ((gp_obj *)obj)->pri; }
static uint8_t gp_base(void *c, uint8_t kind, uint8_t mk, int32_t x, int32_t y, AoeMainViewPickRor *out, void *cur) { (void)c; (void)mk; (void)x; (void)y; (void)cur; out->other_value = kind; return '3'; }
static void pick_tests(void) {
    AoeGameViewPickOpsRor o = {NULL, 100, 50, 0, gp_base, gp_point_objects, gp_object, gp_visible, gp_fog_q, gp_priority};
    AoeMainViewPickRor out; float f;
    /* Object 2 has the highest priority but sits in fog and is not seen: object 3 wins (7 > 5). */
    gp_fog = 0;
    assert(aoe_game_view_pick_ror(&o, ')', 0, 7, 7, &out, NULL, 0) == '4' && out.object_value == (int32_t)(intptr_t)&gp_objs[3] && out.half0 == 120 && out.half1 == 90);
    memcpy(&f, &out.word0, 4); assert(f == 3.5f);
    /* Ignoring the fog (or a revealed map) lets object 2 win; the min kind 3 leaves object 3 only (4 is in state 8). */
    assert(aoe_game_view_pick_ror(&o, ')', 0, 7, 7, &out, NULL, 1) == '4' && out.object_value == (int32_t)(intptr_t)&gp_objs[2]);
    o.reveal_all = 1; assert(aoe_game_view_pick_ror(&o, ')', 3, 7, 7, &out, NULL, 0) == '4' && out.object_value == (int32_t)(intptr_t)&gp_objs[3]); o.reveal_all = 0;
    /* Cycling: after object 1 comes 3 (2 is fogged); after 3 wraps to 1; a current that is alone is itself. */
    gp_fog = 0;
    assert(aoe_game_view_pick_ror(&o, ')', 0, 7, 7, &out, &gp_objs[1], 0) == '4' && out.object_value == (int32_t)(intptr_t)&gp_objs[3]);
    assert(aoe_game_view_pick_ror(&o, ')', 0, 7, 7, &out, &gp_objs[3], 0) == '4' && out.object_value == (int32_t)(intptr_t)&gp_objs[1] && out.half0 == 100);
    gp_n = 1; assert(aoe_game_view_pick_ror(&o, ')', 0, 7, 7, &out, &gp_objs[1], 0) == '4' && out.object_value == (int32_t)(intptr_t)&gp_objs[1]); gp_n = 4;
    /* '*' falls back to the base '(' pick when no object is hit; other kinds go straight to it. */
    gp_n = 0; assert(aoe_game_view_pick_ror(&o, '*', 0, 7, 7, &out, NULL, 0) == '3' && out.other_value == '(' && aoe_game_view_pick_ror(&o, ')', 0, 7, 7, &out, NULL, 0) == '2'); gp_n = 4;
    assert(aoe_game_view_pick_ror(&o, '*', 0, 7, 7, &out, NULL, 0) == '4' && out.object_value == (int32_t)(intptr_t)&gp_objs[2]);
    assert(aoe_game_view_pick_ror(&o, '<', 0, 7, 7, &out, NULL, 0) == '3' && out.other_value == '(');
}
static int gt_units[2] = {1, 2}; static int gt_acts[3][5]; /* [unit][object] action kind + 1 (0 none) */
static int32_t gt_selection(void *c) { (void)c; return 2; }
static void *gt_selected(void *c, int32_t i) { (void)c; return &gt_units[i]; }
static void *gt_object(void *c, int32_t id, uint8_t *state) { (void)c; if (id < 1 || id > 4) return NULL; *state = gp_objs[id].state; return &gp_objs[id]; }
static void *gt_ability(void *c, void *unit, void *obj, int16_t *kind) { int u = *(int *)unit, ob = (int)((gp_obj *)obj - gp_objs); (void)c; if (!gt_acts[u][ob]) return NULL; *kind = (int16_t)(gt_acts[u][ob] - 1); return &gt_acts[u][ob]; }
static int gt_flag(void *c, void *act) { (void)c; return *(int *)act == 3; }
static int gt_ec(void *c, void *unit) { (void)c; return *(int *)unit == 2; }
static int32_t gt_tcount(void *c) { (void)c; return 3; }
static int gt_tmatch(void *c, int32_t i, void *unit) { (void)c; (void)unit; return i == 1; }
static void *gt_tability(void *c, int32_t i, void *unit, void *obj, int16_t *kind) { (void)c; (void)i; (void)unit; if ((gp_obj *)obj != &gp_objs[3]) return NULL; *kind = 4; return &gt_units[0]; }
static void target_tests(void) {
    AoeGameViewTargetOpsRor o = {NULL, gp_point_objects, gt_selection, gt_selected, gt_object, gt_ability, gt_flag, gt_ec, gt_tcount, gt_tmatch, gt_tability, gp_priority};
    int32_t flag = 5;
    /* Unit 1 can act on objects 1 (kind 0) and 2 (kind 2, flagged); unit 2 only through its matching template on 3 (kind 4). */
    gt_acts[1][1] = 1; gt_acts[1][2] = 3;
    assert(aoe_game_view_selection_target_ror(&o, 7, 7, &flag, -1) == &gp_objs[2] && flag == 1);
    assert(aoe_game_view_selection_target_ror(&o, 7, 7, &flag, 0) == &gp_objs[1] && flag == 0);
    assert(aoe_game_view_selection_target_ror(&o, 7, 7, &flag, 4) == &gp_objs[3] && flag == 0);
    assert(aoe_game_view_selection_target_ror(&o, 7, 7, &flag, 9) == NULL && flag == 0);
    gp_n = 0; flag = 5; assert(aoe_game_view_selection_target_ror(&o, 7, 7, &flag, -1) == NULL && flag == 0); gp_n = 4;
}

/* 0x00510580 recorder. */
static char vr_log[512]; static int32_t vr_dst[4], vr_src[4]; static int vr_mouse, vr_edit; static int16_t vr_new_start[4] = {1, 2, 3, 4}; static int32_t vr_new_origin[2] = {16, -8};
static void vr_step(void *c, int id, int32_t a, int32_t b) { (void)c; size_t n = strlen(vr_log); snprintf(vr_log + n, sizeof vr_log - n, "s%d(%d,%d) ", id, (int)a, (int)b); }
static void vr_layout(void *c, AoeViewRenderRor *v) { (void)c; memcpy(v->start, vr_new_start, sizeof v->start); v->origin_x = vr_new_origin[0]; v->origin_y = vr_new_origin[1]; strcat(vr_log, "layout "); }
static void vr_rects(void *c, const int32_t d[4], const int32_t s[4]) { (void)c; memcpy(vr_dst, d, sizeof vr_dst); memcpy(vr_src, s, sizeof vr_src); }
static int vr_mouse_q(void *c) { (void)c; return vr_mouse; }
static int32_t vr_edit_q(void *c) { (void)c; return vr_edit; }
static void render_tests(void) {
    AoeViewRenderOpsRor o = {NULL, vr_step, vr_layout, vr_rects, 0, vr_mouse_q, vr_edit_q};
    AoeViewRenderRor v; memset(&v, 0, sizeof v);
    /* No map: begin, clear, end. */
    vr_log[0] = '\0'; aoe_main_view_render_ror(&v, &o);
    assert(strcmp(vr_log, "s0(0,0) s1(0,0) s2(0,0) ") == 0);
    /* The first (non-incremental) draw: the origin reset, the whole buffer dirtied, the draw. */
    v.has_map = 1; v.width = 64; v.height = 32; v.buf_w = 64; v.buf_h = 32; vr_log[0] = '\0';
    aoe_main_view_render_ror(&v, &o);
    assert(strcmp(vr_log, "s3(0,0) layout s0(0,0) s11(0,0) s12(0,0) s14(10,0) s2(0,0) s15(0,0) ") == 0 && v.incremental == 1 && v.origin_x == 16);
    /* An incremental scroll by (16, -8): the overlap copy is queued, the rows scrolled, the buffer origin
       wraps to (16, 24), the new columns (48..63) are rect 1 and the new rows (0..7 up to x 47) rect 2. */
    vr_new_start[0] = 9; vr_new_origin[0] = 32; vr_new_origin[1] = -16; vr_mouse = 1; vr_edit = 9; vr_log[0] = '\0';
    aoe_main_view_render_ror(&v, &o);
    assert(strcmp(vr_log, "s3(0,0) layout s0(0,0) s5(16,-8) s6(0,0) s8(16,-8) s11(16,24) s12(1,0) s12(2,0) s6(0,0) s13(0,0) s14(10,0) s2(0,0) s15(0,0) ") == 0);
    assert(vr_dst[0] == 16 && vr_dst[1] == 0 && vr_dst[2] == 63 && vr_dst[3] == 23 && vr_src[0] == 0 && vr_src[1] == 8 && vr_src[2] == 47 && vr_src[3] == 31);
    assert(v.highlight_on && v.highlight2_on && v.rect1[0] == 48 && v.rect1[2] == 63 && v.rect1[3] == 31 && v.rect2[1] == 0 && v.rect2[2] == 47 && v.rect2[3] == 7 && v.buf_x == 16 && v.buf_y == 24);
    /* A move of 5 rounds to 0 (the origin pulled back by 5): nothing exposed. A move past the view drops to a full draw. */
    vr_new_start[0] = 10; vr_new_origin[0] = 37; vr_mouse = 0; vr_edit = 0; vr_log[0] = '\0'; o.direct_move = 1; v.blits_active = 1;
    aoe_main_view_render_ror(&v, &o);
    assert(v.origin_x == 32 && !v.highlight_on && v.blits_active == 0 && strcmp(vr_log, "s3(0,0) layout s0(0,0) s4(0,0) s8(0,0) s11(16,24) s14(10,0) s2(0,0) s15(0,0) ") == 0);
    vr_new_start[0] = 11; vr_new_origin[0] = 200; vr_log[0] = '\0';
    aoe_main_view_render_ror(&v, &o);
    assert(strstr(vr_log, "s0(0,0) s11(0,0) s12(0,0) s14(10,0) ") && v.incremental == 1 && v.buf_x == 0);
}
/* 0x0050cef0 recorder. */
static int32_t mp_sent_flag; static uint8_t mp_settings[0x6c]; static uint8_t mp_record[AOE_MP_END_REPORT_SIZE]; static int mp_send_calls;
static int mp_launched(void *c) { (void)c; return 1; }
static int mp_active(void *c, int32_t p) { (void)c; return p != 2; }
static const char *mp_title(void *c) { (void)c; return "Hill Fort"; }
static void mp_leader(void *c, int32_t cat, AoeLeaderRor *out) { (void)c; out->score = cat * 10; out->colour = (uint8_t)cat; strcpy(out->name, "L"); out->allies = 1; }
static int32_t mp_slot_player(void *c, int32_t slot) { (void)c; return slot == 1 ? 1 : slot == 3 ? 2 : 0; }
static const char *mp_slot_name(void *c, int32_t slot) { (void)c; return slot == 1 ? "Ann" : "Bob"; }
static int32_t mp_points(void *c, int32_t p) { (void)c; return 1000 + p; }
static uint8_t mp_p80(void *c, int32_t p) { (void)c; return p == 1 ? 1 : 2; }
static uint8_t mp_p88(void *c, int32_t p) { (void)c; return (uint8_t)(7 + p); }
static uint8_t mp_b(void *c, int32_t i) { (void)c; return (uint8_t)(i + 1); }
static int mp_lobby(void *c, int32_t slot) { (void)c; return slot == 3; }
static uint8_t mp_a15(void *c, int32_t slot) { (void)c; (void)slot; return 0; }
static int32_t mp_value(void *c, int32_t p, int kind) { (void)c; return kind == 1 ? -1 : p * 100 + kind; }
static void mp_send(void *c, const uint8_t *rec, int32_t size) { (void)c; assert(size == AOE_MP_END_REPORT_SIZE); memcpy(mp_record, rec, (size_t)size); ++mp_send_calls; }
static void mp_end_report_tests(void) {
    AoeMpEndReportOpsRor o = {NULL, &mp_sent_flag, mp_launched, 4, mp_active, mp_title, 125999, mp_leader, mp_slot_player, mp_slot_name, mp_points, mp_p80, mp_p88, mp_b, mp_b, mp_b, mp_b, mp_lobby, mp_a15, mp_value, 1, 2, 0x66, mp_settings, mp_send};
    uint8_t out[AOE_MP_END_REPORT_SIZE];
    mp_settings[0] = 3; mp_settings[4] = 5; mp_settings[0x6a] = 75; mp_settings[0x10] = 7; mp_settings[0x64] = 2; mp_settings[0x60] = 1; mp_settings[0x5d] = 9; mp_settings[0x5c] = 8; mp_settings[0x69] = 1;
    assert(aoe_world_mp_end_report_ror(&o, out) == 1 && mp_send_calls == 1 && mp_sent_flag == 1);
    assert(out[0] == 2 && out[1] == 2 && strcmp((const char *)out + 2, "Hill Fort") == 0 && aoe_le_u32(out + 0x24) == 125);
    assert(aoe_le_u32(out + 0x28) == 2550 && out[0x2c] == 0xff && out[0x2d] == 'L' && out[0x3d] == 1 && aoe_le_u32(out + 0x40) == 20 && aoe_le_u32(out + 0x88) == 30);
    /* Slot 1 (player 1, Ann): points 1001, +0x80 == 1, status 0; slot 3 (Bob, player 2): status 3 (lobby), value kind 1 is -1. */
    assert(strcmp((const char *)out + 0xa0, "Ann") == 0 && aoe_le_u32(out + 0xb0) == 1001 && out[0xb4] == 1 && out[0xb5] == 1 && out[0xb6] == 2 && out[0xb7] == 1 && out[0xb8] == 1 && out[0xb9] == 0 && aoe_le_u32(out + 0xbc) == 100 && aoe_le_u32(out + 0xc0) == 0xffffffffu && aoe_le_u32(out + 0xc4) == 102 && out[0xc8] == 8);
    assert(strcmp((const char *)out + 0xa0 + 0x58, "Bob") == 0 && out[0xa0 + 0x58 + 0x19] == 3 && out[0xa0 + 0x58 + 0x14] == 0 && out[0xa0 + 0x2c] == 0);
    assert(out[0x200] == 3 && out[0x201] == 5 && aoe_le_u16(out + 0x202) == 75 && out[0x204] == 7 && out[0x205] == 2 && out[0x206] == 1 && out[0x207] == 9 && out[0x208] == 8 && out[0x209] == 2 && out[0x20a] == 1 && out[0x20c] == 1 && out[0x20d] == 4 && aoe_le_u32(out + 0x218) == 0);
    /* Sent once only. */
    assert(aoe_world_mp_end_report_ror(&o, out) == 0 && mp_send_calls == 1);
}
int main(void)
{
    AoeMainViewRor v;
    int map, player, area, overlay, aux;

    memset(&v, 0, sizeof(v));
    v.ops = &ops;
    v.area = &area;

    /* 0x0050fee0 */
    aoe_main_view_set_map_ror(&v, &map);
    assert(v.map == &map && v.world == &f.world_marker && f.attached == 1);
    assert(v.geometry[0] == 64 && v.geometry[4] == 60 && f.layouts == 1);
    aoe_main_view_set_map_ror(&v, NULL);
    assert(v.world == NULL && v.geometry[0] == 0 && v.geometry[4] == 0);
    assert(f.attached == 1 && f.layouts == 2);

    /* 0x0050ffa0 */
    aoe_main_view_set_player_ror(&v, &player);
    assert(v.player == &player && v.point_x == -10000.0f && v.point_y == -10000.0f);
    assert(f.last_level == 2 && f.last_panel == &v);

    /* 0x0050fec0 */
    v.drag_flag = 1;
    aoe_main_view_set_mode_ror(&v, 1);
    assert(f.base_mode == 1 && v.drag_flag == 1);
    aoe_main_view_set_mode_ror(&v, 2);
    assert(f.base_mode == 2 && v.drag_flag == 0);

    /* 0x0050f590 */
    aoe_main_view_handle_ror(&v, 7);
    assert(f.base_value == 7 && f.notified == 0);
    f.flag = 1;
    aoe_main_view_handle_ror(&v, 8);
    assert(f.notified == 1 && f.last_area == &area);

    /* 0x0050fba0 */
    aoe_main_view_free_overlay_ror(&v);
    assert(f.destroyed == 0);
    v.overlay = &overlay;
    aoe_main_view_free_overlay_ror(&v);
    assert(f.destroyed == 1 && v.overlay == NULL);

    /* 0x00518e50 */
    f.redraws = 0;
    f.state = 3;
    assert(aoe_game_view_check_redraw_ror(&v) == 0);
    f.state = 4; f.edit_mode = 2;
    assert(aoe_game_view_check_redraw_ror(&v) == 1 && f.redraws == 0);
    for (int m = 0; m < 10; m++) {
        f.redraws = 0; f.edit_mode = m; f.state = 7;
        assert(aoe_game_view_check_redraw_ror(&v) == 1);
        assert(f.redraws == (m == 1 || m == 7 || m == 8));
    }

    /* 0x0051a180 / 0x0051a1d0 */
    f.redraws = 0;
    assert(aoe_game_view_after_base_ror(&v, 0) == 0 && f.redraws == 0);
    assert(aoe_game_view_after_base_ror(&v, 1) == 1 && f.redraws == 0);
    v.aux_panel = &aux;
    assert(aoe_game_view_after_base_ror(&v, 1) == 1 && f.redraws == 1);
    assert(f.last_panel == &aux && f.last_level == 1);

    /* 0x0051afb0 */
    assert(aoe_game_view_clear_highlight_ror(&v) == 0);
    assert(f.highlight[0] == -1 && f.highlight[3] == -1 && f.last_panel == &v);

    /* 0x005104a0 */
    {
        static const AoeMainViewBlitRor list[2] = {
            {0, 0, 9, 9, 50, 60}, {10, 20, 19, 29, 70, 80},
        };
        v.blits = list; v.blit_count = 2; v.blit_index = -1;
        v.blit_off_x = 5; v.blit_off_y = 7; v.blits_active = 1;
        busy = 1;
        aoe_main_view_step_blits_ror(&v, 0);
        assert(blits == 0 && v.blit_index == -1);
        aoe_main_view_step_blits_ror(&v, 1);
        assert(blits == 1 && v.blits_active == 1 && last_blit[0] == 55 && last_blit[1] == 67);
        assert(last_blit[2] == 5 && last_blit[3] == 7 && last_blit[4] == 15 && last_blit[5] == 17);
        busy = 0;
        aoe_main_view_step_blits_ror(&v, 0);
        assert(blits == 2 && v.blits_active == 0 && last_blit[2] == 15 && last_blit[5] == 37);
        aoe_main_view_step_blits_ror(&v, 1);
        assert(blits == 2);
        v.blits_active = 1; v.blits = NULL;
        aoe_main_view_step_blits_ror(&v, 1);
        assert(blits == 2 && v.blits_active == 0);
    }

    /* 0x00510ef0 */
    {
        int surface, buffer;
        drawing = &v;
        v.surface = &surface; v.overlay = &buffer;
        v.left = 10; v.top = 20; v.right = 110; v.bottom = 220;
        v.scroll_x = 15; v.scroll_y = 25; v.origin_x = 1; v.origin_y = 2;
        lock_ok = 1;
        aoe_main_view_draw_ror(&v, 10, &buffer);
        assert(regions == 1 && region[0] == 10 && region[1] == 20 && region[2] == 110 && region[3] == 220);
        assert(seen_left == 0 && seen_scroll == 5 && unlocks == 1);
        assert(v.left == 10 && v.top == 20 && v.right == 110 && v.bottom == 220);
        assert(v.scroll_x == 15 && v.scroll_y == 25 && v.origin_x == 1 && v.origin_y == 2);
        assert(v.draw_target == NULL);
        aoe_main_view_draw_ror(&v, 3, NULL);
        assert(codes == 3 && regions == 1 && v.left == 10);
        lock_ok = 0;
        aoe_main_view_draw_ror(&v, 4, NULL);
        assert(codes == 3 && locks == 3 && unlocks == 2);
    }

    /* 0x00511430 */
    {
        AoeMainViewPickRor out;
        pick_result = '4';
        assert(aoe_main_view_pick_ror(&v, 1, 9, 11, 12, 5, &out) == '4');
        assert(out.object_value == 105 && out.other_value == 0 && out.word0 == 1 && out.half1 == 4);
        pick_result = '3';
        assert(aoe_main_view_pick_ror(&v, 0x28, 9, 11, 12, 5, &out) == '3');
        assert(out.other_value == 100 && out.object_value == 0);
        pick_result = '1';
        aoe_main_view_pick_ror(&v, 1, 9, 11, 12, 5, &out);
        assert(out.other_value == 0 && out.object_value == 0);
    }

    /* 0x00514500 */
    {
        int32_t x, y;
        v.left = 0; v.top = 0; v.right = 100; v.bottom = 100;
        cur_x = 50; cur_y = 60; win = 0;
        assert(aoe_main_view_cursor_in_view_ror(&v, 2, 3, &x, &y) == 1 && x == 52 && y == 63);
        assert(aoe_main_view_cursor_in_view_ror(&v, 51, 0, &x, &y) == 0);
        cur_x = 0x10000 + 0x20; /* client 0x10 after conversion */
        win = 1;
        assert(aoe_main_view_cursor_in_view_ror(&v, 0, 0, &x, &y) == 1 && x == 0x10 && y == 55);
        cur_x = 5; /* client negative: 5 - 0x10010 wraps unchanged (< 0x7fff) */
        assert(aoe_main_view_cursor_in_view_ror(&v, 0, 0, &x, &y) == 0 && x == 5 - 0x10010);
        cur_x = 0x18000 + 0x10010; /* client 0x18000 wraps to 0x8000 */
        assert(aoe_main_view_cursor_in_view_ror(&v, 0, 0, &x, &y) == 0 && x == 0x8000);
    }

    /* 0x00513980 */
    {
        static uint8_t pixels[8][16];
        uint8_t *rows[8];
        int surf;
        for (int i = 0; i < 8; i++) rows[i] = pixels[i];
        v.left = 3; v.top = 2; v.right = 10; v.bottom = 5;
        assert(aoe_main_view_update_row_starts_ror(&v, NULL, rows) == 1 && v.row_starts == NULL);
        assert(aoe_main_view_update_row_starts_ror(&v, &surf, rows) == 1);
        assert(v.row_count == 4 && v.row_starts[0] == &pixels[2][3] && v.row_starts[3] == &pixels[5][3]);
        /* The cached surface is never recorded, so a same-height call rebuilds. */
        v.left = 4;
        assert(aoe_main_view_update_row_starts_ror(&v, &surf, rows) == 1 && v.row_starts[0] == &pixels[2][4]);
        v.rows_surface = &surf; v.left = 5;
        assert(aoe_main_view_update_row_starts_ror(&v, &surf, rows) == 1 && v.row_starts[0] == &pixels[2][4]);
        v.bottom = 1;
        assert(aoe_main_view_update_row_starts_ror(&v, rows, rows) == 1 && v.row_count == 4);
        aoe_main_view_free_row_starts_ror(&v);
    }

    /* 0x00518e00 */
    assert(aoe_game_view_timed_draw_ror(&v, 1, 2, 3, 4, 5) == 15);
    assert(recorded_slot == 13 && recorded_ms == 40);

    test_batch_lifecycle();
    test_batch2();
    test_batch3();
    test_batch4();
    test_batch5();
    test_batch6();
    test_batch7();
    test_batch8();
    test_batch9();
    test_batch10();
    test_c3l_begin_drag();
    test_c3l_pixel_pick();
    test_c3l_line_nodes();
    test_wall_drag();
    test_place_click();
    test_c3l_tile_object();
    puts("main_view_test: ok");
    test_collect_nearby();
    test_setup_and_preview();
    test_editor_preview();
    test_right_click();
    test_queue_scroll();
    game_view_default_tests();
    edit_click_tests();
    mouse_move_tests();
    point_objects_tests();
    grid_lookup_tests();
    scroll_tests();
    pick_tests();
    target_tests();
    render_tests();
    mp_end_report_tests();
    return 0;
}
