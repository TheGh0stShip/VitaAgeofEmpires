#include "movement.h"

#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct {
    int flags, kind, finds, counts, fits_calls, fits, places;
    int32_t result, path_count;
    AoeMovementRor *movement;
    AoePathRequestRor last;
    float fit_x, fit_y, place_x, place_y, place_z;
} Env;

static void order_flag(void *c, AoeLiveUnit *u, int32_t v) {
    (void)u;
    assert(v == 0);
    ++((Env *)c)->flags;
}
static int32_t owner_kind(void *c, AoeLiveUnit *u) {
    (void)u;
    return ((Env *)c)->kind;
}
static int32_t find(void *c, const AoePathRequestRor *r) {
    Env *e = c;
    AoePathWaypointRor wp = {0};
    ++e->finds;
    e->last = *r;
    aoe_unit_path_clear_ror(&e->movement->path);
    for (int32_t i = 0; i < e->path_count; ++i) {
        wp.x = (float)i;
        assert(aoe_unit_path_prepend_ror(&e->movement->path, &wp));
    }
    return e->result;
}
static void count_search(void *c) { ++((Env *)c)->counts; }
static int fits(void *c, AoeLiveUnit *u, float x, float y) {
    Env *e = c;
    (void)u;
    ++e->fits_calls;
    e->fit_x = x;
    e->fit_y = y;
    return e->fits;
}
static void place(void *c, AoeLiveUnit *u, float x, float y, float z) {
    Env *e = c;
    (void)u;
    ++e->places;
    e->place_x = x;
    e->place_y = y;
    e->place_z = z;
}

static void count_clear(void *c) { ++*(int *)c; }

int main(void) {
    DatUnit tmpl;
    AoeLiveUnit unit;
    AoeMovementRor m;
    AoePathBudgetRor budget = {2, -1, 0};
    Env env;
    AoeMovementOps ops = {&env, order_flag, owner_kind, find, count_search,
                          fits, place};
    const AoePathWaypointRor *wp;
    uint8_t tile[2] = {7, 9};
    uint32_t bits;
    memset(&tmpl, 0, sizeof(tmpl));
    memset(&unit, 0, sizeof(unit));
    memset(&m, 0, sizeof(m));
    memset(&env, 0, sizeof(env));
    env.movement = &m;
    unit.core.unit_template = &tmpl;
    unit.core.position.x38 = 3.25f;
    unit.core.position.y3c = 4.5f;
    unit.core.position.z40 = 1.0f;
    unit.group_unit_count = 5;

    /* Budget 0x0045c180: override < retry, unlimited, or used < max. */
    assert(aoe_path_budget_allows_ror(&budget, 200));
    budget.used = 2;
    assert(!aoe_path_budget_allows_ror(&budget, 200));
    budget.retry_override = 3;
    assert(aoe_path_budget_allows_ror(&budget, 4) &&
           !aoe_path_budget_allows_ror(&budget, 3));
    budget.max_per_tick = -1;
    assert(aoe_path_budget_allows_ror(&budget, 0));
    budget = (AoePathBudgetRor){1, -1, 0};

    /* Height <= 0 (and NaN): direct two-waypoint path, cursor on the
       destination, no budget or search. */
    aoe_movement_set_destination_ror(&m, 10.0f, 11.0f, 1.0f);
    m.dest_direction = 6;
    tmpl.size[2] = NAN;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0);
    assert(m.path.count == 2 && m.path.cursor == 1 && !budget.used &&
           !env.finds && env.flags == 1 && unit.group_unit_count == 0 &&
           m.best == 100000.0f && m.start_x == 3.25f);
    wp = aoe_unit_path_current_ror(&m.path);
    assert(wp && wp->x == 10.0f && wp->y == 11.0f && wp->direction == 6 &&
           m.path.points[0].x == 3.25f);

    /* Out of budget: the retry byte counts and nothing else changes. */
    tmpl.size[2] = 0.5f;
    budget.used = 1;
    m.best = 1.0f;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 2 &&
           m.retry == 1 && env.flags == 1 && m.best == 1.0f && !env.finds);

    /* With budget: the search targets the destination tile with the
       original fixed arguments; kind 0 owners get baseline +25. */
    budget.used = 0;
    m.retry = 4;
    aoe_movement_set_range_ror(&m, 0.75f);
    aoe_movement_set_target_id_ror(&m, 42);
    env.result = 1;
    env.path_count = 3;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0);
    assert(budget.used == 1 && m.retry == 0 && env.counts == 1 &&
           m.retry_baseline == 25 && m.path.cursor == 1);
    assert(env.last.x1 == 3 && env.last.y1 == 4 && env.last.x2 == 10 &&
           env.last.y2 == 11 && env.last.unit == &unit &&
           env.last.range == 0.75f && env.last.target_id == 42 &&
           env.last.mode == 1 && env.last.budget_limited == 1 &&
           env.last.argument_30 == 1 && env.last.check_occupancy == 1 &&
           env.last.step == 1 && env.last.query_filter == -1);
    memcpy(&bits, &env.last.query_radius, sizeof(bits));
    assert(bits == UINT32_MAX);

    /* Intermediate waypoint tiles replace the destination; other owner
       kinds use +15. */
    env.kind = 2;
    m.waypoint_tile = tile;
    m.waypoint_count = 4;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0 &&
           env.last.x2 == 7 && env.last.y2 == 9 && m.retry_baseline == 19);
    m.waypoint_count = 0;

    /* A failed search leaving one waypoint snaps an off-centre unit to its
       tile centre when it fits there. */
    env.result = 0;
    env.path_count = 1;
    env.fits = 1;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 2 &&
           env.places == 1 && env.fit_x == 3.5f && env.fit_y == 4.5f &&
           env.place_x == 3.5f && env.place_y == 4.5f &&
           env.place_z == 1.0f);

    /* Already centred on both axes: no snap, normal return. */
    unit.core.position.x38 = 3.5f;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0 &&
           env.places == 1 && m.path.cursor == 0);

    /* Not fitting, or more than one waypoint, also skips the snap. */
    unit.core.position.x38 = 3.25f;
    env.fits = 0;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0 &&
           env.places == 1);
    env.fits = 1;
    env.path_count = 2;
    env.fits_calls = 0;
    budget.used = 0;
    assert(aoe_movement_start_ror(&unit, &m, &budget, &ops) == 0 &&
           !env.fits_calls && m.path.cursor == 1);

    /* Advance clamps at the last waypoint; empty paths have no current. */
    assert(!aoe_unit_path_advance_ror(&m.path) && m.path.cursor == 1);
    aoe_unit_path_clear_ror(&m.path);
    assert(!aoe_unit_path_current_ror(&m.path));
    assert(aoe_movement_start_ror(&unit, &m, &budget, NULL) == -1);
    aoe_unit_path_free_ror(&m.path);

    /* Intermediate waypoint list 0x0044eb40..0x0044ed50. */
    {
        AoeMovementRor w;
        int cleared = 0;
        memset(&w, 0, sizeof(w));
        for (uint8_t i = 0; i < 6; ++i) {
            uint8_t e[3] = {i, (uint8_t)(i + 10), (uint8_t)(i + 20)};
            assert(aoe_movement_waypoint_append_ror(&w, e, 1, NULL, NULL));
        }
        /* 5 entries, then doubled to 10 with zero fill. */
        assert(w.waypoint_count == 6 && w.waypoint_capacity == 10 &&
               w.waypoint_tile[3 * 6] == 0 && w.waypoint_tile[3 * 9 + 2] == 0);
        assert(aoe_movement_waypoint_get_ror(&w, 5)[1] == 15 &&
               !aoe_movement_waypoint_get_ror(&w, 6) &&
               !aoe_movement_waypoint_get_ror(&w, -1));
        /* Index 0 drops entry 0; index 2 drops entry 1 (off by one). */
        aoe_movement_waypoint_remove_ror(&w, 0);
        assert(w.waypoint_count == 5 && w.waypoint_tile[0] == 1);
        aoe_movement_waypoint_remove_ror(&w, 2);
        assert(w.waypoint_count == 4 && w.waypoint_tile[0] == 1 &&
               w.waypoint_tile[3] == 3 && w.waypoint_tile[6] == 4);
        aoe_movement_waypoint_remove_ror(&w, 4); /* out of range */
        assert(w.waypoint_count == 4);
        /* A stale list is cleared (hook only with argument 1) on append. */
        aoe_movement_mark_waypoints_stale_ror(&w);
        {
            uint8_t e[3] = {9, 9, 9};
            assert(aoe_movement_waypoint_append_ror(&w, e, 1, count_clear,
                                                    &cleared));
        }
        assert(w.waypoint_count == 1 && !w.waypoints_stale && cleared == 1 &&
               w.waypoint_tile[0] == 9 && w.waypoint_capacity == 10);
        aoe_movement_waypoint_clear_ror(&w, 0, count_clear, &cleared);
        assert(!w.waypoint_count && cleared == 1);
        /* The move action's record step appends once, then marks stale so
           the next append starts a fresh list. */
        {
            uint8_t t1[3] = {4, 5, 0}, t2[3] = {6, 7, 0};
            assert(aoe_movement_record_waypoint_ror(&w, t1) &&
                   w.waypoint_count == 1 && w.waypoints_stale);
            assert(aoe_movement_record_waypoint_ror(&w, t2) &&
                   w.waypoint_count == 1 && w.waypoint_tile[0] == 4);
            assert(aoe_movement_waypoint_append_ror(&w, t2, 0, NULL, NULL) &&
                   w.waypoint_count == 1 && w.waypoint_tile[0] == 6);
        }
        aoe_movement_waypoint_free_ror(&w);
    }
    return 0;
}
