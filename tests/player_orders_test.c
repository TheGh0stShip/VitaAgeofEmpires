#include <assert.h>
#include <math.h>
#include <string.h>

#include "le_bytes.h"
#include "player_orders.h"

typedef struct {
    uint8_t rec[16][160];
    size_t size[16];
    int n;
    int acks, move_acks, target_acks;
    const AoeSelectableUnitRor *acked;
    int entry_ok;
} Capture;

static void send(void *c, const uint8_t *r, size_t s) {
    Capture *cap = c;
    assert(cap->n < 16 && s <= 160);
    memcpy(cap->rec[cap->n], r, s);
    cap->size[cap->n++] = s;
}
static void ack(void *c, AoeSelectableUnitRor *u) {
    ((Capture *)c)->acks++;
    ((Capture *)c)->acked = u;
}
static void ack_move(void *c, AoeSelectableUnitRor *u) {
    ((Capture *)c)->move_acks++;
    ((Capture *)c)->acked = u;
}
static void ack_target(void *c, AoeSelectableUnitRor *u,
                       const AoePlayerOrderTargetRor *t, float x, float y) {
    (void)t;
    (void)x;
    (void)y;
    ((Capture *)c)->target_acks++;
    ((Capture *)c)->acked = u;
}
static int has_template(void *c, int16_t index) {
    (void)c;
    return index == 7;
}
static int has_entry(void *c, int32_t index) {
    (void)index;
    return ((Capture *)c)->entry_ok;
}

static Capture cap;
static AoePlayerOrdersOps ops = {&cap,         send,   ack,       ack_move,
                                 ack_target,   has_template, has_entry,
                                 NULL};

static AoePlayerSelectionRor sel;
static AoeSelectableUnitRor units[3];
static AoePlayerOrdersRor player;
static float stock[4] = {100.0f, 50.0f, 0.0f, 0.0f};

static void setup(void) {
    int i;
    memset(&cap, 0, sizeof(cap));
    memset(&sel, 0, sizeof(sel));
    aoe_player_selection_init_ror(&sel);
    sel.player_id = 3;
    for (i = 0; i < 3; ++i) {
        memset(&units[i], 0, sizeof(units[i]));
        units[i].owner = &sel;
        units[i].owner_id = 3;
        units[i].id = 500 + i;
        units[i].state = 2;
        units[i].type = 70;
        units[i].field_8d = 4;
    }
    sel.slots[1] = &units[0];
    sel.slots[4] = &units[1];
    sel.slots[6] = &units[2];
    sel.count = 3;
    units[1].field_8d = 3; /* only ordered by stop (level 3) */
    player.selection = &sel;
    player.game_state = 0;
    player.resource_count = 4;
    player.resources = stock;
}

static void test_selection_orders(void) {
    AoePlayerOrderTargetRor target = {{77, 9}, 2, 10.5f, 20.25f, 1.5f};
    const uint8_t *r;

    setup();
    assert(aoe_player_order_selection_ror(&player, &target, 1.0f, 2.0f,
                                          &ops) == 1);
    assert(cap.n == 1 && cap.move_acks == 1 && cap.acked == &units[0]);
    r = cap.rec[0];
    assert(r[0] == 3 && r[1] == 2 && aoe_le_i32(r + 4) == 77 &&
           aoe_le_f32(r + 8) == 1.0f && aoe_le_i32(r + 0x1c) == 500 &&
           aoe_le_i32(r + 0x20) == 502 && cap.size[0] == 0x24);

    setup();
    assert(aoe_player_order_selection_follow_ror(&player, &target, 3.0f,
                                                 4.0f, &ops) == 1);
    assert(cap.n == 3 && cap.target_acks == 1);
    assert(cap.rec[0][0] == 2);
    r = cap.rec[2];
    assert(r[0] == 0x0a && r[1] == 3 && r[2] == 3 &&
           aoe_le_i32(r + 4) == 502 && aoe_le_u16(r + 8) == 0x2bd &&
           r[0xa] == 100 && aoe_le_i32(r + 0xc) == 77 && r[0x10] == 2 &&
           aoe_le_f32(r + 0x14) == 10.5f && aoe_le_f32(r + 0x18) == 20.25f &&
           aoe_le_f32(r + 0x1c) == 1.0f && aoe_le_f32(r + 0x20) == 0.75f &&
           r[0x24] == 0 && r[0x25] == 0);
    /* Without a target only the order is queued. */
    setup();
    assert(aoe_player_order_selection_follow_ror(&player, NULL, 3.0f, 4.0f,
                                                 &ops) == 1);
    assert(cap.n == 1 && aoe_le_i32(cap.rec[0] + 4) == -1);

    setup();
    assert(aoe_player_order_selection_primary_ror(&player, &target, 3.0f,
                                                  4.0f, &ops) == 1);
    assert(cap.n == 1 && cap.rec[0][0] == 0 && cap.rec[0][0x11] == 9);

    setup();
    assert(aoe_player_stop_selection_ror(&player, &ops) == 1);
    assert(cap.acks == 1 && cap.rec[0][0] == 1 && cap.rec[0][1] == 3 &&
           cap.size[0] == 14);

    setup();
    assert(aoe_player_arrange_selection_ror(&player, 6, &ops) == 0);
    assert(aoe_player_arrange_selection_ror(&player, -1, &ops) == 0);
    assert(aoe_player_arrange_selection_ror(&player, 5, &ops) == 1);
    assert(cap.rec[0][0] == 6 && aoe_le_i32(cap.rec[0] + 8) == 5 &&
           aoe_le_u32(cap.rec[0] + 4) == 2);

    setup();
    assert(aoe_player_task_selection_ror(&player, &ops) == 1);
    r = cap.rec[0];
    assert(r[0] == 0x11 && r[1] == 3 && r[2] == 3 && r[3] == 2 &&
           aoe_le_u16(r + 4) == 0x2d5 && r[6] == 100 &&
           aoe_le_i32(r + 8) == -1 && r[0xc] == 0xff &&
           aoe_le_f32(r + 0x10) == -1.0f && aoe_le_f32(r + 0x1c) == 1.0f &&
           r[0x20] == 1 && r[0x21] == 0);

    setup();
    assert(aoe_player_record_selection_ror(&player, 2.9f, -1.5f, &ops) == 1);
    r = cap.rec[0];
    assert(r[0] == 0x10 && r[1] == 2 && r[2] == 2 && r[3] == 0xff);

    /* Nothing eligible: no acknowledgement, no record. */
    setup();
    sel.count = 0;
    assert(aoe_player_order_selection_ror(&player, NULL, 0, 0, &ops) == 0);
    assert(aoe_player_stop_selection_ror(&player, &ops) == 0);
    assert(cap.n == 0 && cap.acks == 0 && cap.move_acks == 0);
}

static void test_direct_records(void) {
    AoePlayerUnitTaskRor task;
    AoeCommandObjectRor unit = {41, 0}, target = {42, 5};
    int32_t ids[2] = {8, 9};
    uint8_t recs[6] = {1, 2, 3, 4, 5, 6};
    const uint8_t *r;

    setup();
    memset(&task, 0, sizeof(task));
    task.mode = 77; /* replaced by the player */
    task.unit_id = 12;
    task.value_16 = 0x2d7;
    task.extra_c = 100;
    assert(aoe_player_send_unit_task_ror(5, &task, &ops) == 1);
    assert(cap.rec[0][1] == 5 && cap.rec[0][2] == 5 &&
           aoe_le_i32(cap.rec[0] + 4) == 12 && cap.rec[0][0xa] == 100);

    assert(aoe_player_order_unit_ror(&unit, &target, 1, 2, &ops) == 1);
    r = cap.rec[1];
    assert(r[0] == 0 && r[1] == 1 && aoe_le_i32(r + 0x1c) == 41 &&
           aoe_le_i32(r + 4) == 42);

    assert(aoe_player_send_unit_records_ror(&player, 70, recs, 2, &ops) ==
           1);
    assert(cap.rec[2][0] == 0x0c && cap.rec[2][1] == 3 && cap.rec[2][8] == 2);

    assert(aoe_player_create_object_ror(&player, -1, 0, 0, 0, &ops) == 0);
    assert(aoe_player_create_object_ror(&player, 6, 0, 0, 0, &ops) == 0);
    assert(aoe_player_create_object_ror(&player, 7, 1, 2, 3, &ops) == 1);
    assert(cap.rec[3][0] == 4 && aoe_le_u16(cap.rec[3] + 2) == 7 &&
           cap.rec[3][4] == 3);

    assert(aoe_player_dependency_set_ror(&player, 1, ids, 2, 0x40000000u,
                                         &ops) == 1);
    assert(cap.rec[4][0] == 8 && cap.rec[4][0x70] == 2);
    assert(aoe_player_dependency_propagate_ror(&player, 1, 2, 3, &ops) == 1);
    assert(cap.rec[5][0] == 0x0e && aoe_le_i32(cap.rec[5] + 0xc) == 2);
    assert(aoe_player_dependency_remove_ror(&player, 1, 2, &ops) == 1);
    assert(cap.rec[6][0] == 0x0f && aoe_le_i32(cap.rec[6] + 8) == 2);
    assert(aoe_player_dependency_clear_ror(&player, 1, &ops) == 1);
    assert(cap.rec[7][0] == 9 && cap.rec[7][1] == 3);

    assert(aoe_player_send_resign_ror(&player, 4, 1, &ops) == 1);
    assert(cap.rec[8][0] == 0x0b && cap.rec[8][2] == 4 && cap.rec[8][3] == 1);
    player.game_state = 2;
    assert(aoe_player_send_resign_ror(&player, 4, 1, &ops) == 0);
    assert(cap.n == 9);

    {
        int32_t a = 5, b = 6;
        assert(aoe_player_query_none_ror(&a, &b) == 0 && a == 0 && b == 0);
    }
}

static void test_tribute(void) {
    setup();
    assert(aoe_player_tribute_ror(&player, 2, -1, 5.0f, &ops) == 0);
    assert(aoe_player_tribute_ror(&player, 2, 1, 0.0f, &ops) == 0);
    assert(aoe_player_tribute_ror(&player, 2, 1, NAN, &ops) == 0);
    assert(aoe_player_tribute_ror(&player, 2, 1, 50.5f, &ops) == 0);
    assert(aoe_player_tribute_ror(&player, 2, 9, 1.0f, &ops) == 0);
    assert(cap.n == 0);
    assert(aoe_player_tribute_ror(&player, 2, 1, 50.0f, &ops) == 1);
    assert(cap.rec[0][0] == 7 && cap.rec[0][1] == 3 && cap.rec[0][2] == 2 &&
           cap.rec[0][3] == 1 && aoe_le_f32(cap.rec[0] + 4) == 50.0f);
    /* Negative amounts pass the stock check. */
    assert(aoe_player_tribute_ror(&player, 2, 2, -3.0f, &ops) == 1);
    stock[3] = NAN;
    assert(aoe_player_tribute_ror(&player, 2, 3, 1.0f, &ops) == 1);
}

static void test_assign(void) {
    int32_t ids[1] = {44};
    const uint8_t *r;

    setup();
    cap.entry_ok = 0;
    assert(aoe_player_assign_selection_ror(&player, 30, 5, 6, &ops) == 1);
    assert(cap.n == 2 && cap.rec[0][0] == 9);
    r = cap.rec[1];
    assert(r[0] == 0x0a && aoe_le_i32(r + 4) == 30 &&
           aoe_le_u16(r + 8) == 0x2d7 && aoe_le_i32(r + 0xc) == -1 &&
           r[0x10] == 0xff && aoe_le_f32(r + 0x20) == -1.0f &&
           r[0x24] == 1 && r[0x25] == 0 && r[0xa] == 100);

    setup();
    cap.entry_ok = 1;
    sel.count = 0; /* the slots are scanned regardless of the count */
    assert(aoe_player_assign_selection_ror(&player, 30, 5, 6, &ops) == 1);
    assert(cap.n == 3 && cap.rec[1][0] == 8 && cap.rec[1][0x70] == 3 &&
           aoe_le_i32(cap.rec[1] + 0xc) == 500 &&
           aoe_le_f32(cap.rec[1] + 8) == 2.0f);
    r = cap.rec[2];
    assert(aoe_le_u16(r + 8) == 0x2d6 && aoe_le_i32(r + 0xc) == 5 &&
           r[0x10] == 6);

    setup();
    cap.entry_ok = 1;
    memset(sel.slots, 0, sizeof(sel.slots));
    assert(aoe_player_assign_selection_ror(&player, 30, 5, 6, &ops) == 1);
    assert(cap.n == 1);

    setup();
    cap.entry_ok = 1;
    assert(aoe_player_assign_units_ror(&player, 31, ids, 0, 1, 2, &ops) == 1);
    assert(cap.n == 1);
    assert(aoe_player_assign_units_ror(&player, 31, ids, 1, 1, 2, &ops) == 1);
    assert(cap.n == 4 && cap.rec[2][0x70] == 1 &&
           aoe_le_i32(cap.rec[3] + 0xc) == 1 && cap.rec[3][0x10] == 2);
}

static int chats;
static char last_chat[64];
static int32_t last_from, last_slot;
static int32_t chat_count(void *c) {
    (void)c;
    return 4;
}
static int32_t chat_owner(void *c, int32_t slot) {
    (void)c;
    return slot == 2 ? 5 : 3; /* slots 1, 3, 4: player 3; slot 2: 5 */
}
static void chat_send(void *c, int32_t from, int32_t slot, const char *t) {
    (void)c;
    chats++;
    last_from = from;
    last_slot = slot;
    strncpy(last_chat, t, sizeof(last_chat) - 1);
}

static void test_chat(void) {
    AoePlayerChatOps chat = {NULL, chat_count, chat_owner, chat_send};
    setup();
    aoe_player_send_chat_ror(&player, 5, 0, &chat, "%s", "");
    assert(chats == 0);
    aoe_player_send_chat_ror(&player, 5, 0, &chat, "hi %d", 7);
    assert(chats == 1 && last_slot == 2 && last_from == 3 &&
           strcmp(last_chat, "hi 7") == 0);
    aoe_player_send_chat_ror(&player, 5, 1, &chat, "x");
    assert(chats == 5 && last_slot == 4);
    aoe_player_send_chat_ror(&player, 3, 1, &chat, "x"); /* no echo */
    assert(chats == 8);
}

int main(void) {
    test_chat();
    test_selection_orders();
    test_direct_records();
    test_tribute();
    test_assign();
    return 0;
}
