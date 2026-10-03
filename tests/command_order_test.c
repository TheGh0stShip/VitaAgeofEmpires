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
    assert(order_count < 8);
    orders[order_count++] = (Order){u, t, x, y};
}
static int32_t map_size(void *c, const AoeLiveUnit *u, int axis) {
    (void)c; (void)u;
    return axis ? 22 : 64;
}

static int stops, alternates, cancels_accepted, group_stops, refunds;
static AoeLiveUnit *alternate_target;

static int stop_unit(void *c, AoeLiveUnit *u) {
    (void)c; (void)u;
    ++stops;
    return 1;
}
static void alternate(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x,
                      float y) {
    (void)c; (void)u;
    assert(x == 7.5f && y == 8.5f);
    alternate_target = t;
    ++alternates;
}
static int cancel_head(void *c, void *action) {
    (void)c; (void)action;
    return cancels_accepted;
}
static int group_object(void *c, const AoeLiveUnit *u) {
    (void)c; (void)u;
    return 1;
}
static void stop_group(void *c, AoeLiveUnit *u) {
    (void)c; (void)u;
    ++group_stops;
}
static void refund(void *c, int16_t id) { (void)c; (void)id; ++refunds; }
static void queue_notify(void *c, uint32_t e, int16_t o, uint32_t r) {
    (void)c; (void)e; (void)o; (void)r;
}
static int destroyed;
static void destroy_action(void *c, void *a) { (void)c; (void)a; ++destroyed; }
static void action_state(void *c, void *a, uint8_t s) {
    (void)c; (void)a; (void)s;
}

static void stop_tests(void) {
    AoeLiveUnit unit;
    AoeActionList list;
    AoeBuildingUpdate building;
    AoeBuildingQueueEntry entries[2] = {{83, 2}, {0, 0}};
    AoeActionList queue_list;
    uint32_t status_four = 0;
    int action_a, action_b;
    AoeLiveUnitStopOps ops = {0, cancel_head, group_object, stop_group,
                              {0, refund, NULL, queue_notify}, &status_four};
    memset(&unit, 0, sizeof(unit));
    memset(&building, 0, sizeof(building));
    unit.constructed = 1;
    unit.core.state = 2;
    list = (AoeActionList){&unit.core.state, NULL,
                           {0, destroy_action, action_state}};
    unit.derived_action_list = &list;
    assert(aoe_action_list_push(&list, &action_a) &&
           aoe_action_list_append(&list, &action_b));
    /* Root stop is a bare ret. */
    assert(aoe_live_unit_stop_ror(&unit, AOE_LIVE_UNIT_STOP_ROOT_ROR, &ops) &&
           list.head && !destroyed);
    /* A head that accepts cancellation keeps the list. */
    cancels_accepted = 1;
    assert(aoe_live_unit_stop_ror(&unit, AOE_LIVE_UNIT_STOP_ACTION_ROR,
                                  &ops) && list.head && !destroyed);
    /* Otherwise the whole list is cleared; group classes also stop +0x74. */
    cancels_accepted = 0;
    assert(aoe_live_unit_stop_ror(&unit, AOE_LIVE_UNIT_STOP_GROUP_ROR, &ops) &&
           !list.head && destroyed == 2 && group_stops == 1);
    /* Buildings also drain their queue one refund at a time. */
    queue_list = (AoeActionList){&building.state, NULL,
                                 {0, destroy_action, action_state}};
    building.state = 2;
    building.queue_actions = &queue_list;
    building.queue_entries = entries;
    building.queue_capacity = 2;
    building.queue_aux_count = 2;
    building.queue_count = 1;
    building.queue_status = -1;
    unit.building_update = &building;
    assert(aoe_live_unit_stop_ror(&unit, AOE_LIVE_UNIT_STOP_BUILDING_ROR,
                                  &ops) &&
           building.queue_count == 0 && refunds == 2 && group_stops == 2);
}

static int formation_orders, group_reports;
static AoeLiveUnit *formation_target;
static int32_t reported_id, reported_class;
static float reported_x;

static float placed_x[8], placed_y[8];
static AoeLiveUnit *placed_unit[8];
static uint32_t last_arg;

static void formation(void *c, AoeLiveUnit *u, AoeLiveUnit *t, float x,
                      float y, uint32_t last) {
    (void)c;
    formation_target = t;
    last_arg = last;
    if (formation_orders < 8) {
        placed_unit[formation_orders] = u;
        placed_x[formation_orders] = x;
        placed_y[formation_orders] = y;
    }
    ++formation_orders;
}
static int has_group(void *c, const AoeLiveUnit *u) {
    (void)c;
    return u->core.unit_id != 3; /* unit 3 has no +0x74 object */
}
static void group_order(void *c, AoeLiveUnit *u, int32_t id, int32_t cls,
                        float x, float y) {
    (void)c; (void)u; (void)y;
    reported_id = id;
    reported_class = cls;
    reported_x = x;
    ++group_reports;
}

static void place(int i, int32_t id, const DatUnit *t, float x, float y) {
    units[i].constructed = 1;
    units[i].core.unit_id = id;
    units[i].core.unit_template = t;
    units[i].core.position.x38 = x;
    units[i].core.position.y3c = y;
}

int main(void) {
    stop_tests();
    AoeRng rng;
    int32_t rand_result = -5;
    AoeCommandOrderOps ops = {
        {0, resolve}, 0, session_check, owner_kind, 1, player_104, map_cell,
        &rng, &rand_result, set_flag, order, map_size, stop_unit, alternate,
        formation, has_group, group_order};
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
    order_count = 0;
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
        order_count = 0;
        assert(aoe_command_execute_order_ror(record, size, &ops) == 1 &&
               rng.state == expected.state);
        ops.local_player = 1;
        /* So does a non-session game, or a computer-owned first unit. */
        session = 0;
        order_count = 0;
        assert(aoe_command_execute_order_ror(record, size, &ops) == 1);
        session = 1;
        kinds[1] = 2;
        order_count = 0;
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
    /* Opcode 1: unaligned IDs from +0x02; template 50 is skipped. */
    {
        int32_t stop_ids[3] = {1, 2, 99};
        uint8_t stop_record[2 + 12];
        size_t n = aoe_command_encode_stop_ror(stop_record, sizeof(stop_record),
                                               stop_ids, 3);
        assert(n == 14 && stop_record[2] == 1 && stop_record[6] == 2);
        villager.id = 50;
        units[2].core.unit_template = &house;
        stops = 0;
        assert(aoe_command_dispatch_ror(&dispatch, stop_record, n) ==
                   AOE_COMMAND_ROUTE_HANDLED && stops == 1);
        villager.id = 0;
        units[2].core.unit_template = &villager;
    }
    /* Opcode 2: raw point and target for every unit, no gate. */
    size = aoe_command_encode_order_ror(record, sizeof(record), 2, 9, 7.5f,
                                        8.5f, 1, 0, 0, ids, 3);
    record[0] = AOE_COMMAND_ORDER_ALTERNATE_ROR;
    cell_bits = 0;
    assert(aoe_command_dispatch_ror(&dispatch, record, size) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           alternates == 3 && alternate_target == &units[0]);

    record[0] = AOE_COMMAND_ORDER_ROR; /* short-record check below */
    /* Opcode 3, simple path (units spread > 8): the order gets the raw
       target even with zero size, but the group report filters it to
       -1/-1. Unit 3 has no group object and reports nothing. */
    house.size[0] = 0.0f;
    house.size[1] = 0.0f;
    units[1].core.position.x38 = 50.0f;
    units[2].core.position.x38 = 10.0f;
    units[3].core.position.x38 = 30.0f;
    size = aoe_command_encode_order_ror(record, sizeof(record), 2, 9, 7.5f,
                                        8.5f, 0, 0, 0, ids, 3);
    record[0] = AOE_COMMAND_ORDER_FORMATION_ROR;
    assert(aoe_command_dispatch_ror(&dispatch, record, size) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           formation_orders == 3 && formation_target == &units[0] &&
           group_reports == 2 && reported_id == -1 && reported_class == -1 &&
           reported_x == 7.5f);
    /* With a sized target the report carries its ID and class. */
    house.size[0] = 1.0f;
    house.unit_class = 3;
    assert(aoe_command_execute_order_formation_ror(record, size, &ops) == 1 &&
           reported_id == 9 && reported_class == 3);
    /* Opcode 6: leader is the unit nearest the origin (unit 2 at
       (10, 9)); others step from it. Pattern 4 zig-zags. */
    {
        int32_t arrange_ids[3] = {1, 2, 3};
        uint8_t arrange_record[0x0c + 12];
        size_t n;
        units[1].core.position.x38 = 50.0f; units[1].core.position.y3c = 5.0f;
        units[2].core.position.x38 = 10.0f; units[2].core.position.y3c = 9.0f;
        units[3].core.position.x38 = 30.0f; units[3].core.position.y3c = 4.0f;
        n = aoe_command_encode_arrange_ror(arrange_record,
                                           sizeof(arrange_record), 4,
                                           arrange_ids, 3);
        formation_orders = 0;
        assert(n == 0x18 &&
               aoe_command_dispatch_ror(&dispatch, arrange_record, n) ==
                   AOE_COMMAND_ROUTE_HANDLED);
        assert(formation_orders == 2 && last_arg == 0x3f800000 &&
               formation_target == NULL);
        assert(placed_unit[0] == &units[1] && placed_x[0] == 11.5f &&
               placed_y[0] == 7.5f);
        assert(placed_unit[1] == &units[3] && placed_x[1] == 11.5f &&
               placed_y[1] == 10.5f);
        /* Pattern 1 (anti-diagonal) and pattern 3 (column). */
        aoe_le_put_u32(arrange_record + 8, 1);
        formation_orders = 0;
        assert(aoe_command_execute_arrange_ror(arrange_record, n, &ops) == 1 &&
               placed_x[0] == 11.5f && placed_y[0] == 7.5f &&
               placed_x[1] == 13.0f && placed_y[1] == 6.0f);
        aoe_le_put_u32(arrange_record + 8, 3);
        formation_orders = 0;
        assert(aoe_command_execute_arrange_ror(arrange_record, n, &ops) == 1 &&
               placed_x[0] == 10.0f && placed_y[0] == 11.0f &&
               placed_y[1] == 13.0f);
        /* A single unit does nothing, not even group preparation. */
        aoe_le_put_u32(arrange_record + 4, 1);
        formation_orders = 0;
        flag_calls = 0;
        assert(aoe_command_execute_arrange_ror(arrange_record, n, &ops) == 1 &&
               !formation_orders && !flag_calls);
        /* Count larger than the record is malformed. */
        aoe_le_put_u32(arrange_record + 4, 4);
        assert(aoe_command_execute_arrange_ror(arrange_record, n, &ops) == -1);
    }

    record[0] = AOE_COMMAND_ORDER_ROR;
    /* Short record. */
    assert(aoe_command_execute_order_ror(record, size - 1, &ops) == -1);
    for (int i = 0; i < 4; ++i) free(units[i].group_unit_ids);
    return 0;
}
