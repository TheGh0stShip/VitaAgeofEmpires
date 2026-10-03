#include "command_dispatch.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

typedef struct {
    AoeLiveUnit *unit;
    AoeLiveUnit *target;
    float x, y;
} Order;

static AoeLiveUnit units[4]; /* ids 1..3, target id 9 = units[0] */
static DatUnit house, villager;
static Order orders[8];
static int order_count, flag_calls;
static int32_t kinds[4];
static int32_t flag_104 = 1;
static uint32_t cell_bits;
static int session = 1;

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    for (int i = 0; i < 4; ++i)
        if (units[i].constructed && units[i].core.unit_id == id)
            return &units[i];
    return NULL;
}
static int session_check(void *c) { (void)c; return session; }
static int32_t owner_kind(void *c, const AoeLiveUnit *u) {
    (void)c;
    return kinds[u - units];
}
static int32_t player_104(void *c, uint8_t p) {
    (void)c;
    assert(p == 2);
    return flag_104;
}
static int map_cell(void *c, int32_t row, int32_t col, uint32_t *cell) {
    (void)c;
    assert(row == 20 && col == 10);
    *cell = cell_bits;
    return 1;
}
static void set_flag(void *c, AoeLiveUnit *u, int32_t v) {
    (void)c; (void)u;
    assert(v == 1);
    ++flag_calls;
}
static void order(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x, float y) {
    (void)c;
    orders[order_count++] = (Order){u, t, x, y};
}
static int32_t map_size(void *c, const AoeLiveUnit *u, int axis) {
    (void)c; (void)u;
    return axis ? 22 : 64;
}

static void place(int i, int32_t id, const DatUnit *t, float x, float y) {
    units[i].constructed = 1;
    units[i].core.unit_id = id;
    units[i].core.unit_template = t;
    units[i].core.position.x38 = x;
    units[i].core.position.y3c = y;
}

int main(void) {
    AoeRng rng;
    int32_t rand_result = -5;
    AoeCommandOrderOps ops = {
        {0, resolve}, 0, session_check, owner_kind, 1, player_104, map_cell,
        &rng, &rand_result, set_flag, order, map_size};
    AoeCommandDispatchRor dispatch;
    uint8_t record[0x1c + 16];
    int32_t ids[3] = {1, 2, 3};
    size_t size;

    house.type = 80;
    house.size[0] = 1.5f;
    villager.type = 70;
    place(0, 9, &house, 10.5f, 20.5f);
    place(1, 1, &villager, 4.0f, 5.0f);
    place(2, 2, &villager, 6.0f, 9.0f);
    place(3, 3, &villager, 5.0f, 4.0f);
    aoe_rng_seed(&rng, 1);
    memset(&dispatch, 0, sizeof(dispatch));
    aoe_command_bind_order_ror(&dispatch, &ops);

    /* Formation: centre (5, 6); target (10.5, 20.5). Unit 2's y offset
       (20.5 + 3 = 23.5) clamps to map height 22 - 1 = 21. */
    size = aoe_command_encode_order_ror(record, sizeof(record), 2, 9, 10.5f,
                                        20.5f, 0, 0x10000, 0x10000, ids, 3);
    assert(size == 0x1c + 12);
    cell_bits = 0x10000;
    assert(aoe_command_dispatch_ror(&dispatch, record, size) ==
           AOE_COMMAND_ROUTE_HANDLED);
    assert(order_count == 3 && flag_calls == 3);
    assert(orders[0].unit == &units[1] && orders[0].target == &units[0] &&
           orders[0].x == 9.5f && orders[0].y == 19.5f);
    assert(orders[1].x == 11.5f && orders[1].y == 21.0f);
    assert(orders[2].x == 10.5f && orders[2].y == 18.5f);
    /* Each unit records every other listed ID. */
    assert(units[1].group_unit_count == 2 && units[1].group_unit_ids[0] == 2 &&
           units[1].group_unit_ids[1] == 3);
    assert(units[2].group_unit_ids[0] == 1 && units[2].group_unit_ids[1] == 3);

    /* A zero-size target is dropped in formation mode only. */
    house.size[0] = 0.0f;
    order_count = 0;
    assert(aoe_command_dispatch_ror(&dispatch, record, size) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           orders[0].target == NULL);
    /* Spread above 8 tiles: everyone gets the raw point and target. */
    units[3].core.position.x38 = 14.5f;
    order_count = 0;
    assert(aoe_command_dispatch_ror(&dispatch, record, size) ==
           AOE_COMMAND_ROUTE_HANDLED);
    assert(order_count == 3 && orders[2].target == &units[0] &&
           orders[2].x == 10.5f && orders[2].y == 20.5f);
    /* Owner kind 1 empties the group before re-adding (no duplicates). */
    kinds[1] = 1;
    assert(aoe_command_dispatch_ror(&dispatch, record, size) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           units[1].group_unit_count == 2);
    kinds[1] = 0;

    /* Validity gate (target is a type-80 building, foreign player 2):
       unexplored cell drops the command and consumes rand(). */
    cell_bits = 0;
    order_count = 0;
    {
        AoeRng expected;
        aoe_rng_seed(&expected, 1);
        assert(aoe_command_dispatch_ror(&dispatch, record, size) ==
                   AOE_COMMAND_ROUTE_HANDLED && !order_count &&
               rand_result == aoe_rng_next(&expected));
        /* Disjoint masks drop too. */
        cell_bits = 0x10000;
        aoe_le_put_u32(record + 0x14, 0);
        assert(aoe_command_execute_order_ror(record, size, &ops) == 0 &&
               rand_result == aoe_rng_next(&expected));
        aoe_le_put_u32(record + 0x14, 0x10000);
        /* Player +0x104 == 0 with the flag byte set drops. */
        flag_104 = 0;
        record[0x12] = 1;
        assert(aoe_command_execute_order_ror(record, size, &ops) == 0 &&
               rand_result == aoe_rng_next(&expected));
        /* The local player's own commands skip the gate. */
        ops.local_player = 2;
        assert(aoe_command_execute_order_ror(record, size, &ops) == 1 &&
               rng.state == expected.state);
        ops.local_player = 1;
        /* So does a non-session game, or a computer-owned first unit. */
        session = 0;
        assert(aoe_command_execute_order_ror(record, size, &ops) == 1);
        session = 1;
        kinds[1] = 2;
        assert(aoe_command_execute_order_ror(record, size, &ops) == 1);
        kinds[1] = 0;
        flag_104 = 1;
        record[0x12] = 0;
    }

    /* Original quirk: the maximum is only updated in the `else` branch, so
       strictly decreasing x (50, 30, 10) keeps max_x = 0 and a 40-tile
       group still moves in formation. */
    house.size[0] = 1.5f;
    units[1].core.position.x38 = 50.0f; units[1].core.position.y3c = 5.0f;
    units[2].core.position.x38 = 30.0f; units[2].core.position.y3c = 5.0f;
    units[3].core.position.x38 = 10.0f; units[3].core.position.y3c = 5.0f;
    size = aoe_command_encode_order_ror(record, sizeof(record), 2, 9, 10.5f,
                                        20.5f, 0, 0x10000, 0x10000, ids, 3);
    order_count = 0;
    assert(aoe_command_execute_order_ror(record, size, &ops) == 1 &&
           order_count == 3 && orders[0].x == 50.0f - 30.0f + 10.5f &&
           orders[2].x == 1.0f /* 10 - 30 + 10.5 < 0 */);

    /* Single unit with no target: raw coordinates. */
    size = aoe_command_encode_order_ror(record, sizeof(record), 2, -1, -3.0f,
                                        99.0f, 0, 0, 0, ids, 1);
    order_count = 0;
    assert(aoe_command_execute_order_ror(record, size, &ops) == 1 &&
           order_count == 1 && orders[0].target == NULL &&
           orders[0].x == -3.0f && orders[0].y == 99.0f);
    /* Short record. */
    assert(aoe_command_execute_order_ror(record, size - 1, &ops) == -1);
    for (int i = 0; i < 4; ++i) free(units[i].group_unit_ids);
    return 0;
}
