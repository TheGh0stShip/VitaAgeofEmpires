#include "command_player.h"

#include <assert.h>
#include <string.h>

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
    return 0;
}
