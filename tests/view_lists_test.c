#include "view_lists.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
    return 0;
}
