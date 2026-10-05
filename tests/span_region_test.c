/* Patched RoR 1.0a node pool 0x004a34b0..0x004a3700 / 0x00516280 and span
   region 0x004a3710..0x004a4200. */
#include "span_region.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The row's spans as "x0-x1 x0-x1". */
static const char *row(const AoeSpanRegionRor *r, int y) {
    static char buf[256];
    const AoeSpanRor *s;
    int n = 0;
    buf[0] = '\0';
    for (s = r->first[y]; s; s = s->next) {
        n += snprintf(buf + n, sizeof(buf) - (size_t)n, "%s%d-%d", n ? " " : "",
                      (int)s->x0, (int)s->x1);
        if (s->next) assert(s->next->prev == s);
    }
    if (r->first[y]) {
        assert(r->min_x[y] == r->first[y]->x0 && r->max_x[y] == r->last[y]->x1);
    }
    return buf;
}

int main(void) {
    AoeNodePoolRor p;
    AoeSpanRegionRor r, a, b;
    void *n1, *n2, *n3;
    static const uint8_t list[] = {2, 1, 3, 2, 10, 12, 0xff};
    uint8_t outline[8];

    /* Pool: 2 nodes, then blocks of 3; returned nodes come back first. */
    assert(aoe_pool_init_ror(&p, 2, 3, 16));
    assert(p.capacity == 8 && p.block_count == 1 && p.available == 2 &&
           p.cur_block == 0);
    n1 = aoe_pool_take_ror(&p);
    n2 = aoe_pool_take_ror(&p);
    assert(n1 && n2 && p.available == 0 && p.cur_block == -1);
    n3 = aoe_pool_take_ror(&p);
    assert(n3 && p.block_count == 2 && p.total == 5 && p.available == 2);
    aoe_pool_give_ror(&p, n1);
    assert(aoe_pool_take_ror(&p) == n1);
    aoe_pool_give_ror(&p, n2);
    aoe_pool_give_ror(&p, n1);
    assert(aoe_pool_give_chain_ror(&p, NULL, NULL) == 0);
    aoe_pool_reset_ror(&p);
    assert(p.available == 5 && !p.free_list && p.resets == 1);
    aoe_pool_free_ror(&p);

    assert(aoe_span_region_init_ror(&r, 64, 8));
    assert(r.rows == 16 && r.cols == 64 && r.right == 63 && r.bottom == 15);
    /* Adds merge; touching spans join; gaps stay. */
    aoe_span_region_add_ror(&r, 10, 20, 0);
    aoe_span_region_add_ror(&r, 30, 40, 0);
    assert(strcmp(row(&r, 0), "10-20 30-40") == 0 && r.count[0] == 2);
    aoe_span_region_add_ror(&r, 25, 26, 0);
    assert(strcmp(row(&r, 0), "10-20 25-26 30-40") == 0 && r.count[0] == 3);
    aoe_span_region_add_ror(&r, 21, 29, 0);
    assert(strcmp(row(&r, 0), "10-40") == 0 && r.count[0] == 1);
    aoe_span_region_add_ror(&r, 50, 45, 0);
    aoe_span_region_add_ror(&r, 2, 4, 0);
    assert(strcmp(row(&r, 0), "2-4 10-40 45-50") == 0);
    aoe_span_region_add_ror(&r, 0, 60, 0);
    assert(strcmp(row(&r, 0), "0-60") == 0 && r.count[0] == 1);
    /* Clipping to the bounds and the area. */
    aoe_span_region_add_ror(&r, -5, 100, 1);
    assert(strcmp(row(&r, 1), "0-63") == 0);
    aoe_span_region_add_ror(&r, 0, 5, 99);
    /* Removal splits and trims. */
    aoe_span_region_remove_ror(&r, 10, 20, 0);
    assert(strcmp(row(&r, 0), "0-9 21-60") == 0 && r.count[0] == 2);
    aoe_span_region_remove_ror(&r, 5, 30, 0);
    assert(strcmp(row(&r, 0), "0-4 31-60") == 0);
    aoe_span_region_remove_ror(&r, 0, 63, 0);
    assert(!r.first[0] && r.count[0] == 0);
    /* Byte lists are offset. */
    aoe_span_region_add_list_ror(&r, list, 1, 3);
    assert(strcmp(row(&r, 5), "2-4 11-13") == 0);
    aoe_span_region_remove_list_ror(&r, list, 1, 3);
    assert(!r.first[5]);
    /* Lines are widened to groups of four. */
    aoe_span_region_add_line_ror(&r, 5, 6, 5, 8);
    assert(strcmp(row(&r, 6), "4-7") == 0 && strcmp(row(&r, 8), "4-7") == 0);
    aoe_span_region_add_line_ror(&r, 0, 10, 15, 11);
    assert(strcmp(row(&r, 10), "0-7") == 0 && strcmp(row(&r, 11), "8-15") == 0);
    /* An outline row: left 2, right 1 of an 8-wide frame at x 9. */
    outline[0] = 2; outline[1] = 0; outline[2] = 1; outline[3] = 0;
    outline[4] = 0; outline[5] = 0x80; outline[6] = 0; outline[7] = 0;
    aoe_span_region_add_outline_ror(&r, outline, 8, 2, 9, 12, 0);
    assert(strcmp(row(&r, 12), "8-15") == 0 && !r.first[13]);
    /* Clearing empties every row. */
    aoe_span_region_clear_ror(&r);
    assert(!r.first[12] && r.pool.available == r.pool.total);

    /* Union of two regions. */
    assert(aoe_span_region_init_ror(&a, 64, 8) && aoe_span_region_init_ror(&b, 64, 8));
    aoe_span_region_add_ror(&a, 1, 2, 3);
    aoe_span_region_add_ror(&b, 9, 10, 3);
    aoe_span_region_union_ror(&r, &a, &b);
    assert(strcmp(row(&r, 3), "0-3 8-11") == 0);
    /* Bounds: set only when they overlap. */
    aoe_span_region_set_bounds_ror(&r, 50, 20, 40, 2);
    assert(r.left == 40 && r.right == 50 && r.top == 2 && r.bottom == 15);
    aoe_span_region_set_bounds_ror(&r, 100, 0, 200, 5);
    assert(r.left == 40);
    aoe_span_region_free_ror(&a);
    aoe_span_region_free_ror(&b);
    aoe_span_region_free_ror(&r);
    return 0;
}
