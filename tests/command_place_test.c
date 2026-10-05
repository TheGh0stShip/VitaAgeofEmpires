#include "command_dispatch.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static DatUnit house, center;
static AoeLiveUnit builders[2], foundation, old_center;
static float values[40];
static AoePlayerResources stock = {40, values};
static uint8_t check_code;
static int center_alive;
static char trace[160];

static void mark(const char *s) {
    strncat(trace, s, sizeof(trace) - strlen(trace) - 1);
}
static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    for (int i = 0; i < 2; ++i)
        if (builders[i].core.unit_id == id) return &builders[i];
    return NULL;
}
static int present(void *c, uint8_t p) { (void)c; return p == 2; }
static const DatUnit *tmpl(void *c, uint8_t p, int16_t i) {
    (void)c; (void)p;
    return i == 70 ? &house : i == 109 ? &center : NULL;
}
static AoePlayerResources *res(void *c, uint8_t p) { (void)c; (void)p; return &stock; }
static int wall_mode;
static float wall_x[16], wall_y[16];
static int wall_checks;
static uint8_t check(void *c, uint8_t p, const DatUnit *t, float x, float y,
                     uint32_t *out, const int32_t flags[6]) {
    (void)c; (void)p;
    if (wall_mode) {
        assert(!out && flags[0] == 1 && flags[1] == t->placement_class &&
               flags[2] == 0 && flags[5] == 1);
        wall_x[wall_checks] = x;
        wall_y[wall_checks] = y;
        ++wall_checks;
        /* Cell (3,5) is blocked. */
        return x == 3.5f && y == 5.5f;
    }
    assert(flags[0] == 1 && flags[5] == 1);
    assert(x == 12.5f && y == 7.5f);
    *out = 0x55;
    return check_code;
}
static AoeLiveUnit *find(void *c, uint8_t p, int16_t id) {
    (void)c; (void)p;
    assert(id == 109);
    return center_alive ? &old_center : NULL;
}
static void failed(void *c, uint8_t p, uint8_t code, uint32_t out,
                   int16_t index, uint32_t x, uint32_t y, uint32_t v,
                   int32_t builder) {
    char s[64];
    (void)c; (void)p; (void)x; (void)y;
    snprintf(s, sizeof(s), "F%u/%x/%d/%x/%d ", code, out, index, v, builder);
    mark(s);
}
static AoeLiveUnit pieces[16];
static int piece_count;
static AoeLiveUnit *create(void *c, uint8_t p, const DatUnit *t, float x,
                           float y, uint32_t v) {
    (void)c; (void)p;
    if (wall_mode) {
        assert(v == 9);
        pieces[piece_count].core.position.x38 = x;
        pieces[piece_count].core.position.y3c = y;
        return &pieces[piece_count++];
    }
    assert(v == 3);
    mark(t == &center ? "C109 " : "C70 ");
    return &foundation;
}
static int32_t kind(void *c, const AoeLiveUnit *u) { (void)c; (void)u; return 0; }
static void order(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x, float y) {
    char s[16];
    (void)c;
    if (wall_mode) {
        assert(t == &pieces[0] && x == t->core.position.x38 &&
               y == t->core.position.y3c);
    } else {
        assert(t == &foundation);
        (void)x; (void)y;
    }
    snprintf(s, sizeof(s), "O%d ", u->core.unit_id);
    mark(s);
}
static void expect(const char *s) {
    if (strcmp(trace, s)) {
        fprintf(stderr, "%s != %s\n", trace, s);
        assert(0);
    }
    trace[0] = 0;
}

int main(void) {
    AoeCommandPlaceOps ops = {{0, resolve}, 0, present, tmpl, res, check,
                              find, failed, create, kind, order};
    AoeCommandTribeDispatchRor tribe;
    AoeRng rng;
    uint8_t r[0x20];
    int32_t ids[2] = {1, 2};
    size_t n;
    house.type = 80;
    house.costs[0] = (DatUnitCost){1, 30, 1};
    house.costs[1] = (DatUnitCost){-1, 0, 0};
    house.costs[2] = (DatUnitCost){-1, 0, 0};
    center = house;
    center.costs[0] = (DatUnitCost){1, 200, 1};
    for (int i = 0; i < 2; ++i) {
        builders[i].constructed = 1;
        builders[i].core.unit_id = 1 + i;
    }
    old_center.core.state = 2;
    memset(&tribe, 0, sizeof(tribe));
    aoe_rng_seed(&rng, 1);
    tribe.rng = &rng;
    aoe_command_bind_place_ror(&tribe, &ops);

    values[1] = 100.0f;
    n = aoe_command_encode_place_ror(r, sizeof(r), 2, 12.5f, 7.5f, 70, 3,
                                     ids, 2);
    assert(n == 0x1c);
    /* Success: pay 30 wood, create, order both builders. */
    assert(aoe_command_dispatch_tribe_ror(&tribe, r, n) ==
           AOE_COMMAND_ROUTE_HANDLED);
    expect("C70 O1 O2 ");
    assert(values[1] == 70.0f);
    /* Placement error: report with the check's out-value. */
    check_code = 4;
    assert(aoe_command_execute_place_ror(r, n, &ops) == 0);
    expect("F4/55/70/3/1 ");
    check_code = 0;
    /* Cannot afford: code 0 report, nothing created. */
    values[1] = 10.0f;
    assert(aoe_command_execute_place_ror(r, n, &ops) == 0);
    expect("F0/55/70/3/1 ");
    /* Town center limit: disabled template + a live center -> silent. */
    values[1] = 500.0f;
    n = aoe_command_encode_place_ror(r, sizeof(r), 2, 12.5f, 7.5f, 109, 3,
                                     ids, 2);
    center_alive = 1;
    assert(aoe_command_execute_place_ror(r, n, &ops) == 0);
    expect("");
    center.enabled = 1;
    assert(aoe_command_execute_place_ror(r, n, &ops) == 1);
    expect("C109 O1 O2 ");
    /* Unknown player or -1 template: nothing. */
    r[2] = 3;
    assert(aoe_command_execute_place_ror(r, n, &ops) == 0);
    expect("");
    /* Walls: (2,3) -> (3,7) is an L since |dy| = 4 > |dx| = 1: the run
       x = 2 for y 3..7, then (3,7). Wall pieces cost 2 wood. */
    {
        DatUnit wall = house;
        wall.costs[0] = (DatUnitCost){1, 2, 1};
        wall.placement_class = 6;
        house = wall; /* template 70 now behaves as a wall */
        wall_mode = 1;
        r[2] = 2;
        values[1] = 100.0f;
        n = aoe_command_encode_wall_ror(r, sizeof(r), 2, 2, 3, 3, 7, 70, 9,
                                        ids, 2);
        assert(n == 0x18);
        assert(aoe_command_dispatch_tribe_ror(&tribe, r, n) ==
               AOE_COMMAND_ROUTE_HANDLED);
        assert(wall_checks == 6 && wall_x[0] == 2.5f && wall_y[0] == 3.5f &&
               wall_x[4] == 2.5f && wall_y[4] == 7.5f &&
               wall_x[5] == 3.5f && wall_y[5] == 7.5f);
        assert(piece_count == 6 && values[1] == 88.0f);
        expect("O1 O2 ");
        /* Horizontal-first L with a blocked cell and a payment cutoff:
           (1,5) -> (4,6): run y = 5 for x 1..4 (cell (3,5) blocked), then
           (4,6). Only 4 wood: two pieces, then payment stops each run. */
        wall_checks = 0;
        piece_count = 0;
        values[1] = 4.0f;
        n = aoe_command_encode_wall_ror(r, sizeof(r), 2, 1, 5, 4, 6, 70, 9,
                                        ids, 0);
        assert(aoe_command_execute_wall_ror(r, n, &ops) == 1);
        /* Checks: (1,5) (2,5) (3,5 blocked) (4,5 payment fails, run ends),
           then (4,6) whose payment also fails. */
        assert(piece_count == 2 && values[1] == 0.0f && wall_checks == 5 &&
               wall_x[0] == 1.5f && wall_x[1] == 2.5f && wall_x[2] == 3.5f &&
               wall_x[3] == 4.5f && wall_y[3] == 5.5f &&
               wall_x[4] == 4.5f && wall_y[4] == 6.5f);
        expect("");
    }
    return 0;
}
