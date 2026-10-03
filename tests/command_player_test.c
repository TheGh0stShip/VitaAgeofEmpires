#include "command_player.h"

#include <assert.h>
#include <string.h>

#include "le_bytes.h"

static float values[0x40];
static AoePlayerResources resources = {0x40, values};
static int32_t field_104;
static int refreshes;

static void refresh(void *context) {
    (void)context;
    ++refreshes;
}

static AoePlayerResourceTargetRor players[2] = {
    {&resources, 0, 0, 0, 0},
    {&resources, 1, &field_104, 0, refresh}};

static const AoePlayerResourceTargetRor *lookup(void *context,
                                                uint8_t index) {
    (void)context;
    return index < 2 ? &players[index] : 0;
}

static char order[32];
static unsigned order_used;
static uint32_t event_args[5];
static uint32_t notice_args[7];

static void game_event(void *context, uint32_t kind, uint32_t sender,
                       uint32_t receiver, uint32_t resource, int32_t amount) {
    (void)context;
    order[order_used++] = 'G';
    event_args[0] = kind; event_args[1] = sender; event_args[2] = receiver;
    event_args[3] = resource; event_args[4] = (uint32_t)amount;
}

static void receiver_notice(void *context, uint8_t receiver_index,
                            uint32_t sender, uint32_t receiver,
                            uint32_t notice, uint32_t resource,
                            int32_t amount, uint32_t zero) {
    (void)context;
    order[order_used++] = 'N';
    notice_args[0] = receiver_index; notice_args[1] = sender;
    notice_args[2] = receiver; notice_args[3] = notice;
    notice_args[4] = resource; notice_args[5] = (uint32_t)amount;
    notice_args[6] = zero;
}

static float giver_values[0x40], taker_values[0x40];
static AoePlayerResources giver_stock = {0x40, giver_values};
static AoePlayerResources taker_stock = {0x40, taker_values};
static int32_t taker_104;
static AoePlayerResourceTargetRor tribute_players[2] = {
    {&giver_stock, 0, 0, 0, 0},
    {&taker_stock, 1, &taker_104, 0, refresh}};

static const AoePlayerResourceTargetRor *tribute_lookup(void *context,
                                                        uint8_t index) {
    (void)context;
    return index >= 1 && index <= 2 ? &tribute_players[index - 1] : 0;
}

static void tribute_tests(void) {
    static const uint8_t expected[8] = {7, 1, 2, 3, 0x00, 0x00, 0x48, 0x42};
    AoeCommandPlayerOps ops = {0, tribute_lookup, game_event,
                               receiver_notice};
    uint8_t record[8];
    /* 0x0042bb20 layout; 50.0f is 0x42480000. */
    assert(aoe_command_encode_player_tribute_ror(
               record, sizeof(record), 1, 2, 3, 50.0f) == 8 &&
           !memcmp(record, expected, 8));
    /* Gate: 49.5 < 50 fails without side effects. */
    giver_values[3] = 49.5f;
    assert(aoe_command_execute_player_tribute_ror(record, 8, &ops) == 0 &&
           !order_used && giver_values[3] == 49.5f);
    /* Exactly equal passes: sender debited, receiver credited, then the
       game event and receiver notice (0x20a, trailing 0) in that order. */
    giver_values[3] = 50.0f;
    taker_values[3] = 1.0f;
    assert(aoe_command_execute_player_tribute_ror(record, 8, &ops) == 1 &&
           giver_values[3] == 0.0f && taker_values[3] == 51.0f &&
           taker_values[49] == 0.0f && !strcmp(order, "GN"));
    assert(event_args[0] == 7 && event_args[1] == 1 && event_args[2] == 2 &&
           event_args[3] == 3 && event_args[4] == 50);
    assert(notice_args[0] == 2 && notice_args[1] == 1 &&
           notice_args[2] == 2 && notice_args[3] == 0x20a &&
           notice_args[4] == 3 && notice_args[5] == 50 && !notice_args[6]);
    /* __ftol truncates the reported amount toward zero. */
    giver_values[0] = 10.0f;
    assert(aoe_command_encode_player_tribute_ror(
               record, sizeof(record), 1, 2, 0, 2.75f) == 8 &&
           aoe_command_execute_player_tribute_ror(record, 8, &ops) == 1 &&
           giver_values[0] == 7.25f && event_args[4] == 2 &&
           notice_args[5] == 2);
    /* NaN amount fails the unordered compare. */
    aoe_le_put_u32(record + 4, UINT32_C(0x7fc00000));
    order_used = 0;
    assert(aoe_command_execute_player_tribute_ror(record, 8, &ops) == 0 &&
           !order_used);
    /* Resource 0x32 to a derived receiver refreshes the world. */
    giver_values[0x32] = 5.0f;
    refreshes = 0;
    assert(aoe_command_encode_player_tribute_ror(
               record, sizeof(record), 1, 2, 0x32, 5.0f) == 8 &&
           aoe_command_execute_player_tribute_ror(record, 8, &ops) == 1 &&
           refreshes == 1 && taker_104 == 1 && taker_values[0x32] == 5.0f);
    /* Host guards fire before mutation: missing receiver hooks, an
       out-of-count resource byte, and an unresolved receiver. */
    tribute_players[1].refresh_world = 0;
    giver_values[0x32] = 5.0f;
    assert(aoe_command_execute_player_tribute_ror(record, 8, &ops) == -1 &&
           giver_values[0x32] == 5.0f);
    record[3] = 0x40;
    assert(aoe_command_execute_player_tribute_ror(record, 8, &ops) == -1);
    record[3] = 0;
    record[2] = 9;
    assert(aoe_command_execute_player_tribute_ror(record, 8, &ops) == -1);
    ops.receiver_notice = 0;
    record[2] = 2;
    assert(aoe_command_execute_player_tribute_ror(record, 8, &ops) == -1);
}

int main(void) {
    static const uint8_t expected[8] = {5, 1, 0x32, 0, 0x00, 0x00, 0xc0, 0xbf};
    AoeCommandPlayerOps ops = {0, lookup};
    uint8_t record[16];
    memset(record, 0xcc, sizeof(record));
    /* 0x0042bae0 layout; -1.5f is 0xbfc00000. */
    assert(aoe_command_encode_player_resource_add_ror(
               record, sizeof(record), 1, 0x32, -1.5f) == 8 &&
           !memcmp(record, expected, 8));
    assert(!aoe_command_encode_player_resource_add_ror(record, 7, 1, 0, 0));
    /* Derived player: +0x104 and world refresh, then the add. */
    assert(aoe_command_execute_player_resource_add_ror(record, 8, &ops) == 1 &&
           field_104 == 1 && refreshes == 1 && values[0x32] == -1.5f);
    /* The command always passes flag 0, so resource 3 never feeds slot 49. */
    assert(aoe_command_encode_player_resource_add_ror(
               record, sizeof(record), 1, 3, 2.0f) == 8 &&
           aoe_command_execute_player_resource_add_ror(record, 8, &ops) == 1 &&
           values[3] == 2.0f && values[49] == 0.0f);
    /* Base player: plain add, no side effects. */
    assert(aoe_command_encode_player_resource_add_ror(
               record, sizeof(record), 0, 0x32, 1.0f) == 8 &&
           aoe_command_execute_player_resource_add_ror(record, 8, &ops) == 1 &&
           refreshes == 1 && values[0x32] == -0.5f);
    /* Resource byte is zero-extended: 0xff is an ignored index, not -1. */
    record[2] = 0xff;
    assert(aoe_command_execute_player_resource_add_ror(record, 8, &ops) == 1);
    /* Malformed records and unresolved players. */
    assert(aoe_command_execute_player_resource_add_ror(record, 7, &ops) == -1);
    record[1] = 9;
    assert(aoe_command_execute_player_resource_add_ror(record, 8, &ops) == -1);
    record[0] = 4;
    record[1] = 0;
    assert(aoe_command_execute_player_resource_add_ror(record, 8, &ops) == -1);
    tribute_tests();
    return 0;
}
