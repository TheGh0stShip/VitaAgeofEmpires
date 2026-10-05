#include <assert.h>
#include "value_grid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static void test_cells(void) {
    AoeValueGridRor g, h;
    AoeOutcomeStream s;
    uint8_t buf[128];

    CHECK(aoe_value_grid_init_ror(&g, 4, 3, 7));
    CHECK(aoe_value_grid_get_ror(&g, 3, 2) == 7);
    g.origin_x = 10;
    g.origin_y = 20;
    CHECK(aoe_value_grid_set_ror(&g, 11, 21, 5) && g.rows[1][1] == 5);
    CHECK(!aoe_value_grid_set_ror(&g, 14, 21, 5));
    CHECK(aoe_value_grid_get_ror(&g, 11, 21) == 5);
    CHECK(!aoe_value_grid_set_box_ror(&g, 10, 20, 14, 22, 1));
    CHECK(aoe_value_grid_set_box_ror(&g, 12, 20, 13, 21, 1));
    CHECK(g.rows[0][2] == 1 && g.rows[1][3] == 1 && g.rows[2][3] == 7);

    aoe_value_grid_set_cap_ror(&g, 9);
    aoe_value_grid_set_cap_ror(&g, 0); /* ignored */
    CHECK(g.cap == 9);
    aoe_value_grid_raise_ror(&g, 10, 22, 5); /* 7 + 5 saturates at 8 */
    CHECK(g.rows[2][0] == 8);
    aoe_value_grid_lower_ror(&g, 10, 22, 20);
    CHECK(g.rows[2][0] == 0);
    g.rows[0][0] = 9; /* at the cap: left alone */
    aoe_value_grid_raise_ror(&g, 10, 20, 1);
    aoe_value_grid_lower_ror(&g, 10, 20, 1);
    CHECK(g.rows[0][0] == 9);

    /* Boxes clamp into the grid. */
    aoe_value_grid_fill_ror(&g, 2);
    aoe_value_grid_raise_box_ror(&g, 0, 0, 100, 100, 3);
    CHECK(g.rows[0][0] == 5 && g.rows[2][3] == 5);
    aoe_value_grid_lower_box_ror(&g, 12, 21, 12, 21, 4);
    CHECK(g.rows[1][2] == 1 && g.rows[1][1] == 5);

    s.data = buf;
    s.size = sizeof buf;
    s.pos = 0;
    s.failed = 0;
    aoe_value_grid_save_ror(&g, &s);
    CHECK(!s.failed && s.pos == 17 + 12);
    CHECK(buf[16] == 9 && buf[17 + 6] == 1);
    s.size = s.pos;
    s.pos = 0;
    aoe_value_grid_init_ror(&h, 1, 1, 0);
    CHECK(aoe_value_grid_load_ror(&h, &s));
    CHECK(h.width == 4 && h.height == 3 && h.origin_x == 10 && h.cap == 9);
    CHECK(h.rows[1][2] == 1 && h.rows[2][3] == 5);
    aoe_value_grid_free_ror(&h);
    aoe_value_grid_free_ror(&g);
}

static void test_neighbours(void) {
    AoeValueGridRor g;
    int32_t from[2] = {0, 0}, to[2] = {2, 2}, bad[2] = {3, 0};
    aoe_value_grid_init_ror(&g, 3, 3, 0);
    g.rows[1][1] = 5;
    g.rows[0][1] = 5;  /* above the centre (a side) */
    g.rows[0][0] = 5;  /* a diagonal */

    CHECK(aoe_value_grid_count_ror(&g, from, bad, 5, 0, 0, 1) == 0);
    /* Sides only: centre sees (1,0); (1,0) sees the centre and (0,0);
       (0,0) sees (1,0). */
    CHECK(aoe_value_grid_count_ror(&g, from, to, 5, 0, 0, 0) == 4);
    CHECK(g.count == 4);
    /* With diagonals the centre and (0,0) also see each other. */
    CHECK(aoe_value_grid_count_ror(&g, from, to, 5, 0, 0, 1) == 6);
    /* Off-grid sides: (0,0) has two, (1,0) one. */
    CHECK(aoe_value_grid_count_ror(&g, from, to, 5, 1, 0, 0) == 7);
    /* Off-grid diagonals: (0,0) has three, (1,0) two. */
    CHECK(aoe_value_grid_count_ror(&g, from, to, 5, 0, 1, 1) == 11);

    /* Scoring from count 0: only the centre is looked at; its side
       neighbours (1,0) at the threshold give 2 each. */
    from[0] = from[1] = to[0] = to[1] = 1;
    g.count = 0;
    CHECK(aoe_value_grid_score_ror(&g, from, to, 5, 0, 0, 0, 0) == 2);
    CHECK(aoe_value_grid_score_ror(&g, from, to, 5, 0, 0, 0, 1) == 4);
    /* A cell short of the threshold even with the bonus scores nothing. */
    CHECK(aoe_value_grid_score_ror(&g, from, to, 7, 1, 0, 0, 1) == 0);
    /* With the bonus it gets the second pass only (it is below 7). */
    g.rows[1][2] = 7;
    CHECK(aoe_value_grid_score_ror(&g, from, to, 7, 2, 0, 0, 0) == 2);
    /* Off-grid neighbours cost one in each pass at a corner cell. */
    from[0] = from[1] = to[0] = to[1] = 0;
    g.count = 10;
    CHECK(aoe_value_grid_score_ror(&g, from, to, 5, 0, 1, 0, 0) ==
          10 - 2 - 2 + 2);
    aoe_value_grid_destroy_ror(&g, 0);
    CHECK(aoe_value_grid_destroy_ror(
              (aoe_value_grid_init_ror(&g, 1, 1, 0), &g), 0) == &g);
}

int main(void) {
    AoeValueGridRor *heap = malloc(sizeof *heap);
    test_cells();
    test_neighbours();
    aoe_value_grid_init_ror(heap, 2, 2, 0);
    if (aoe_value_grid_destroy_ror(heap, 1) != NULL) ++failures;
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("value_grid_test: ok");
    {
        AoeValueGridRor big, a, b;
        assert(aoe_value_grids_static_init_ror(&big, &a, &b));
        assert(big.width == 254 && big.height == 254 && a.width == 50 && b.height == 50);
        assert(aoe_value_grid_get_ror(&big, 253, 253) == 0 && aoe_value_grid_get_ror(&b, 49, 0) == 0);
        aoe_value_grid_set_ror(&a, 3, 4, 9);
        assert(aoe_value_grid_max_ror(&a) == 9 && aoe_value_grid_max_ror(&b) == 0);
        aoe_value_grids_static_free_ror(&big, &a, &b);
    }
    return 0;
}
