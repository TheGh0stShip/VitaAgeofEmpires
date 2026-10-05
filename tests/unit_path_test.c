/* Patched RoR 1.0a unit path waypoints 0x00457fe0..0x00458470 and the
   pathfinder accessors 0x004587e0 / 0x0045a4e0. */
#include "pathfinder_search.h"

#include <assert.h>
#include <math.h>
#include <string.h>

int main(void) {
    AoeUnitPathRor p, q;
    AoePathfinderRor pf;
    uint8_t penalty[AOE_PATH_GRID_CELLS_ROR];

    aoe_unit_path_init_ror(&p, 2);
    assert(p.capacity == 2 && p.count == 0 && p.points);
    assert(aoe_unit_path_append_ror(&p, 0, 0, 0, 1));
    assert(aoe_unit_path_append_ror(&p, 3, 4, 0, 2));
    assert(p.capacity == 2);
    /* Full: the capacity doubles. */
    assert(aoe_unit_path_append_ror(&p, 3, 4, 12, 3) && p.capacity == 4);
    assert(aoe_unit_path_count_ror(&p) == 3);
    assert(aoe_unit_path_at_ror(&p, 1)->x == 3 && !aoe_unit_path_at_ror(&p, 3) &&
           !aoe_unit_path_at_ror(&p, -1));
    assert(aoe_unit_path_last_ror(&p)->direction == 3);

    /* Lengths: segment 0 is 5; segment 1 is 12 as a true distance, but
       the total's z term is dz * (z1 - x0) = 12 * (12 - 3) = 108. */
    aoe_unit_path_set_cursor_ror(&p, 0);
    assert(aoe_unit_path_segment_ror(&p) == 5.0);
    aoe_unit_path_set_cursor_ror(&p, 1);
    assert(aoe_unit_path_cursor_ror(&p) == 1 && aoe_unit_path_segment_ror(&p) == 12.0);
    aoe_unit_path_set_cursor_ror(&p, 2);
    assert(aoe_unit_path_segment_ror(&p) == 0.0);
    assert(aoe_unit_path_length_ror(&p) == 5.0 + sqrt(108.0));

    /* Copy: capacity grows to 0 + 4. */
    aoe_unit_path_init_ror(&q, 0);
    assert(!q.points && q.capacity == 0);
    assert(aoe_unit_path_assign_ror(&q, &p));
    assert(q.capacity == 4 && q.count == 3 && q.cursor == 2 &&
           q.points[2].z == 12);

    /* Prepend leaves the capacity; the port's append still fits. */
    {
        AoePathWaypointRor w = {9, 9, 9, 9};
        assert(aoe_unit_path_prepend_ror(&q, &w) && q.count == 4 &&
               q.capacity == 4);
        assert(aoe_unit_path_prepend_ror(&q, &w) && q.count == 5);
        assert(aoe_unit_path_append_ror(&q, 1, 1, 1, 1) && q.capacity >= 6 &&
               q.points[5].x == 1 && q.points[0].x == 9);
    }
    aoe_unit_path_init_ror(&q, -1);
    assert(!q.points && q.capacity == 0);
    aoe_unit_path_free_ror(&p);

    memset(&pf, 0, sizeof(pf));
    memset(penalty, 0, sizeof(penalty));
    penalty[3 * 255 + 7] = 42;
    pf.penalty = penalty;
    assert(aoe_path_penalty_ror(&pf, 3, 7) == 42 && aoe_path_penalty_ror(&pf, 7, 3) == 0);
    aoe_path_count_search_ror(&pf);
    aoe_path_count_search_ror(&pf);
    assert(pf.searches == 2);
    return 0;
}
