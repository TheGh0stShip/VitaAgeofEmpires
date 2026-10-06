#include "view_lists.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static uint8_t obj_flags[8];
static uint8_t *flags_of(void *c, int32_t i) { (void)c; return i == 5 ? &obj_flags[5] : (i == 6 ? &obj_flags[6] : NULL); }

int main(void) {
    AoeViewMarkRor *marks = NULL;
    AoeDrawRowsRor d;
    AoeDrawRowNodeRor *a, *b;

    aoe_view_mark_add_ror(&marks, 7, 0, 10, 20, 0, 0xf, 0, 2, 0x7d);
    aoe_view_mark_add_ror(&marks, 8, 0, 1, 2, 0, 1, 0, 1, 5);
    aoe_view_mark_add_ror(&marks, 9, 0, 3, 4, 0, 1, 0, 1, 5);
    assert(marks->shape == 9 && marks->next->shape == 8 && marks->next->prev == marks);
    assert(marks->next->next->interval == 0x7d && marks->next->next->frame == 0);
    aoe_view_marks_drop_once_ror(&marks);
    assert(marks && marks->shape == 7 && !marks->next && !marks->prev);
    aoe_view_marks_free_ror(&marks);
    assert(marks == NULL);

    memset(&d, 0, sizeof d);
    assert(aoe_pool_init_ror(&d.pool, 4, 4, sizeof(AoeDrawRowNodeRor)));
    d.rows = 10;
    d.table_count = 3;
    d.table = calloc(3, sizeof *d.table);
    d.heads = calloc(11, sizeof *d.heads);
    d.tails = calloc(11, sizeof *d.tails);

    aoe_draw_rows_set_clip_ror(&d, 5, 6, 1, 9);
    assert(d.clip_right == 0);
    aoe_draw_rows_set_clip_ror(&d, 1, 2, 3, 4);
    assert(d.clip_left == 1 && d.clip_right == 3 && d.clip_top == 2 && d.clip_bottom == 4);
    aoe_draw_rows_set_window_ror(&d, 20, -3);
    assert(d.row_lo == 0 && d.row_hi == 10);

    a = aoe_draw_rows_take_ror(&d);
    b = aoe_draw_rows_take_ror(&d);
    assert(a && b && a != b);
    a->id = 5; a->key = 1; a->next = NULL;
    b->id = 5; b->key = 2; b->next = a;
    d.heads[3] = b;
    assert(aoe_draw_rows_find_ror(&d, 5, 9, 0, 1) == a);
    assert(aoe_draw_rows_find_ror(&d, 5, 0, 2, 1) == NULL);
    assert(aoe_draw_rows_find_ror(&d, 5, 20, 11, 2) == NULL);
    d.table[2] = &d;
    aoe_draw_rows_reset_ror(&d);
    assert(!d.heads[3] && !d.table[2] && d.pool.available == d.pool.total && d.pool.resets == 1);
    {
        /* 0x00516510 / 0x00516660 */
        const int16_t c[8] = {10, 20, 30, 40, 50, 60, 70, 80};
        AoeDrawRowNodeRor *p, *q, *r;
        d.table_base = 1; d.row_lo = 2; d.row_hi = 5;
        assert(aoe_draw_rows_add_ror(&d, 3, 0, c, 1, 0, 9) == NULL);
        assert(aoe_draw_rows_add_ror(&d, 3, 2, c, 3, 0, 9) == NULL);
        assert(aoe_draw_rows_add_ror(&d, 3, -2, c, 3, 0, 9) == NULL);
        p = aoe_draw_rows_add_ror(&d, 0, 0, c, 3, 44, 9);
        q = aoe_draw_rows_add_ror(&d, 1, 0, c, 5, 0, 8);
        r = aoe_draw_rows_add_ror(&d, 2, -1, c, 2, 0, 7);
        assert(p && q && r && d.table[1] == q && q->free_link == p && !p->free_link && d.table[0] == r);
        assert(p->id == 9 && p->key == 0 && aoe_draw_row_node_dword_ror(p, 0x10) == 3);
        assert(aoe_draw_row_node_dword_ror(p, 0x1c) == 44 && aoe_draw_row_node_dword_ror(p, 0x14) == 0);
        assert(aoe_draw_row_node_coord_ror(q, 7) == 80);
        aoe_draw_rows_scroll_ror(&d, 2, 1);
        assert(d.table[0] == q && !d.table[1] && !d.table[2]);
        assert(aoe_draw_row_node_dword_ror(p, 0x14) == -2 && aoe_draw_row_node_dword_ror(p, 0x18) == -1);
        assert(aoe_draw_row_node_coord_ror(q, 0) == 8 && aoe_draw_row_node_coord_ror(q, 1) == 19);
        assert(aoe_draw_row_node_coord_ror(q, 4) == 48 && aoe_draw_row_node_coord_ror(q, 5) == 59);
        assert(aoe_draw_row_node_coord_ror(q, 6) == 68 && aoe_draw_row_node_coord_ror(q, 7) == 79);
        assert(aoe_draw_row_node_coord_ror(r, 0) == 10);
        aoe_draw_rows_scroll_ror(&d, 0, -2);
        assert(!d.table[0] && !d.table[1] && d.table[2] == q);
        aoe_draw_rows_scroll_ror(&d, 0, 7);
        assert(!d.table[0] && !d.table[1] && !d.table[2]);
    }
    aoe_draw_rows_free_ror(&d);
    assert(!d.heads && !d.pool.blocks);
    {
        float pt[2] = {-0.5f, 7.0f};
        AoeViewTimerRor t[2] = {{1, 0, 100, 30, 2}, {0, 0, 100, 30, 2}};
        aoe_view_clamp_point_ror(pt, 10, 7);
        assert(pt[0] == 0.0f && pt[1] == 6.0f);
        pt[0] = 9.5f;
        pt[1] = 3.25f;
        aoe_view_clamp_point_ror(pt, 10, 7);
        assert(pt[0] == 9.5f && pt[1] == 3.25f);
        aoe_view_timers_rewind_ror(t, 2, 2);
        assert(t[0].time == 70 && t[1].time == 100);
        aoe_view_timers_rewind_ror(t, 2, 3);
        assert(t[0].time == 70);
    }
    {
        AoeViewTimerRor t[3];
        int32_t active = 0;
        AoeViewGeometryRor g;
        memset(t, 0, sizeof t);
        aoe_view_timer_add_ror(t, 3, &active, 5, 1000, 4, 0, 100);
        assert(active == 1 && t[2].active && t[2].field_4 == 5 && t[2].time == 100);
        aoe_view_timer_add_ror(t, 3, &active, 6, 500, 2, 4, 200);
        assert(active == 2 && t[1].field_4 == 6 && t[2].time == 100 - 1000);
        t[2].time = 100;
        obj_flags[5] = 0;
        aoe_view_timers_tick_ror(t, 3, &active, flags_of, NULL, 250);
        assert(obj_flags[5] == 4 && obj_flags[6] == 2 && active == 2);
        aoe_view_timers_tick_ror(t, 3, &active, flags_of, NULL, 100 + 0x100);
        assert(obj_flags[5] == 0);
        aoe_view_timers_tick_ror(t, 3, &active, flags_of, NULL, 100 + 1001);
        assert(!t[2].active && !t[1].active && active == 0 && obj_flags[6] == 0);
        memset(&g, 0, sizeof g);
        g.has_map = 1;
        g.width = 640;
        g.height = 480;
        g.tile_w = 64;
        g.tile_h = 32;
        g.half_tile_w = 32;
        g.half_tile_h = 16;
        g.origin_x = 3;
        aoe_view_geometry_ror(&g);
        assert(g.across == 10 && g.down == 15 && g.centre_x == 320 - 32 + 3 && g.centre_y == 224);
        g.tile_h = 0;
        aoe_view_geometry_ror(&g);
        assert(g.across == 0 && g.centre_x == 0);
    }
    return 0;
}
