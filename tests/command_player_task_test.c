#include "command_dispatch.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "le_bytes.h"

static AoeLiveUnit units[3];
static int16_t owners[3] = {2, 2, 3};
static int group[3] = {1, 1, 1};
static AoePlayerUnitTaskRor tasks[4];
static AoeLiveUnit *task_units[4];
static int task_count, record_count;
static uint8_t records[8][3];
static uint8_t left[9], resigned[9];
static uint32_t slots[9] = {0, 1, 2, 2, 3, 0, 0, 0, 0};
static int local_left_calls, states, events;
static uint32_t event_value;
static int session = 1;

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    for (int i = 0; i < 3; ++i)
        if (units[i].core.unit_id == id) return &units[i];
    return NULL;
}
static int16_t player_id(void *c, uint8_t p) { (void)c; return (int16_t)p; }
static int16_t owner_id(void *c, const AoeLiveUnit *u) {
    (void)c;
    return owners[u - units];
}
static int32_t owner_kind(void *c, const AoeLiveUnit *u) {
    (void)c; (void)u;
    return 1;
}
static int has_group(void *c, const AoeLiveUnit *u) {
    (void)c;
    return group[u - units];
}
static void group_task(void *c, AoeLiveUnit *u, const AoePlayerUnitTaskRor *t) {
    (void)c;
    task_units[task_count] = u;
    tasks[task_count++] = *t;
}
static void unit_record(void *c, AoeLiveUnit *u, const uint8_t r[3]) {
    (void)c; (void)u;
    memcpy(records[record_count++], r, 3);
}
static int session_check(void *c) { (void)c; return session; }
static uint32_t local_slot(void *c) { (void)c; return 1; }
static void local_left(void *c) { (void)c; ++local_left_calls; }
static uint32_t slot_player(void *c, int s) { (void)c; return slots[s]; }
static uint8_t *player_resigned(void *c, uint8_t p) {
    (void)c;
    return p < 9 ? &resigned[p] : NULL;
}
static void player_state(void *c, uint8_t p, int32_t s) {
    (void)c; (void)p;
    assert(s == 2);
    ++states;
}
static int32_t event_player;
static void game_event(void *c, uint32_t e, int32_t p, uint32_t v) {
    (void)c;
    assert(e == 5);
    event_player = p;
    event_value = v;
    ++events;
}

int main(void) {
    AoeCommandPlayerTaskOps ops = {
        {0, resolve}, 0, player_id, owner_id, owner_kind, has_group,
        group_task, unit_record, session_check, left, local_slot,
        local_left, slot_player, player_resigned, player_state, game_event};
    AoeCommandDispatchRor dispatch;
    AoePlayerUnitTaskRor task = {7, 11, -3, 0x12345678, 9, 0x40000000,
                                 0x40400000, 0, 0xabcdef01, 1, 2, 3};
    uint8_t record[0x40];
    size_t n;
    for (int i = 0; i < 3; ++i) {
        units[i].constructed = 1;
        units[i].core.unit_id = 11 + i;
        units[i].core.position.x38 = 1.0f + i;
        units[i].core.position.y3c = 2.0f;
        units[i].core.position.z40 = 3.0f;
    }
    memset(&dispatch, 0, sizeof(dispatch));
    aoe_command_bind_player_task_ror(&dispatch, &ops);

    /* 0x0a: builder 0x0042bcf0 layout round-trips into player +0x34. */
    n = aoe_command_encode_unit_task_ror(record, sizeof(record), 2, &task);
    assert(n == 0x28 && record[0x0a] == 3 && record[0x24] == 1);
    assert(aoe_command_dispatch_ror(&dispatch, record, n) ==
           AOE_COMMAND_ROUTE_HANDLED);
    assert(task_count == 1 && task_units[0] == &units[0] &&
           !memcmp(&tasks[0], &task, sizeof(task)));
    /* Wrong issuing player: +0x34 ignores units it does not own. */
    record[1] = 3;
    assert(aoe_command_execute_unit_task_ror(record, n, &ops) == 1 &&
           task_count == 1);
    /* Unresolved unit: nothing. */
    aoe_le_put_u32(record + 4, 99);
    assert(aoe_command_execute_unit_task_ror(record, n, &ops) == 0);

    /* 0x11: -1.0f coordinates fall back to the unit's position. */
    memset(record, 0, sizeof(record));
    record[0] = 0x11; record[1] = 2; record[2] = 5; record[3] = 2;
    aoe_le_put_u32(record + 0x10, 0xbf800000);
    aoe_le_put_u32(record + 0x14, 0x41200000); /* 10.0f */
    aoe_le_put_u32(record + 0x18, 0xbf800000);
    record[0x06] = 0x66;
    aoe_le_put_u32(record + 0x24, 12);
    aoe_le_put_u32(record + 0x28, 13); /* owned by player 3: ignored */
    task_count = 0;
    assert(aoe_command_dispatch_ror(&dispatch, record, 0x2c) ==
           AOE_COMMAND_ROUTE_HANDLED);
    assert(task_count == 1 && tasks[0].unit_id == 12 &&
           tasks[0].x_bits == 0x40000000 /* 2.0f */ &&
           tasks[0].y_bits == 0x41200000 &&
           tasks[0].z_bits == 0x40400000 /* 3.0f */ &&
           tasks[0].mode == 5 && tasks[0].extra_c == 0x66);

    /* 0x0c: records from +0x09 with count +0x08. */
    memset(record, 0, sizeof(record));
    record[0] = 0x0c; record[1] = 2;
    aoe_le_put_u32(record + 4, 11);
    record[8] = 2;
    memcpy(record + 9, "\x01\x02\x03\x04\x05\x06", 6);
    assert(aoe_command_dispatch_ror(&dispatch, record, 15) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           record_count == 2 && !memcmp(records[1], "\x04\x05\x06", 3));
    /* 0x10: one {+2, +3, 0} record per listed unit. */
    memset(record, 0, sizeof(record));
    record[0] = 0x10; record[1] = 2; record[2] = 8; record[3] = 9;
    aoe_le_put_u32(record + 4, 11);
    aoe_le_put_u32(record + 8, -1);
    record_count = 0;
    assert(aoe_command_dispatch_ror(&dispatch, record, 12) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           record_count == 1 && !memcmp(records[0], "\x08\x09\x00", 3));

    /* 0x0b in a session: player 2 is controlled by slots 2 and 3, so the
       first departure only marks the slot. */
    memset(record, 0, sizeof(record));
    record[0] = 0x0b; record[1] = 2; record[2] = 2; record[3] = 4;
    assert(aoe_command_dispatch_ror(&dispatch, record, 4) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           left[2] && !states && !local_left_calls);
    record[2] = 3;
    assert(aoe_command_execute_resign_ror(record, 4, &ops) == 1 &&
           states == 1 && events == 1 && event_value == 4 && resigned[2] &&
           event_player == 2);
    /* Repeat resignation: state again, no second event. */
    session = 0;
    assert(aoe_command_execute_resign_ror(record, 4, &ops) == 1 &&
           states == 2 && events == 1);
    /* The local slot runs its teardown. */
    session = 1;
    record[2] = 1;
    record[1] = 1;
    slots[1] = 1;
    assert(aoe_command_execute_resign_ror(record, 4, &ops) == 1 &&
           local_left_calls == 1 && events == 2 && event_player == 1);
    assert(aoe_command_execute_resign_ror(record, 3, &ops) == -1);
    return 0;
}
