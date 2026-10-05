#include "command_dependency.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    AoeLiveUnit units[3];
    uint16_t owners[3];
    int lookups;
} World;

static AoeLiveUnit *resolve(void *context, int32_t unit_id) {
    World *world = context;
    ++world->lookups;
    for (int i = 0; i < 3; ++i)
        if (world->units[i].core.unit_id == unit_id) return &world->units[i];
    return NULL;
}

static uint16_t owner(void *context, const AoeLiveUnit *unit) {
    World *world = context;
    return world->owners[unit - world->units];
}

int main(void) {
    static const uint8_t expected_propagate[16] = {
        0x0e, 0x03, 0, 0, 0x0a, 0, 0, 0,
        0x00, 0x00, 0x80, 0x40, 0x0b, 0, 0, 0};
    World world;
    AoeCommandDependencyOps ops = {{&world, resolve}, &world, owner};
    uint8_t record[0x80];
    int32_t ids[26] = {10, 11, 12};
    memset(&world, 0, sizeof(world));
    for (int i = 0; i < 3; ++i) {
        world.units[i].constructed = 1;
        world.units[i].core.unit_id = 10 + i;
        world.units[i].dependent_source_id = -1;
        world.owners[i] = 3;
    }

    /* Builder layouts and sizes match 0042bbe0/0042bc40/0042bc80/0042bcc0. */
    memset(record, 0xcc, sizeof(record));
    assert(aoe_command_encode_dependency_propagate_ror(
               record, sizeof(record), 3, 10, 11, UINT32_C(0x40800000)) ==
           0x10 && !memcmp(record, expected_propagate, 16));
    assert(aoe_command_encode_dependency_clear_ror(record, 7, 3, 10) == 0);
    assert(aoe_command_encode_dependency_set_ror(
               record, sizeof(record), 3, 10, ids, 26, 0) == 0);
    assert(aoe_command_encode_dependency_set_ror(
               record, sizeof(record), 3, 10, ids, 3, UINT32_C(7)) == 0x74 &&
           record[0] == 8 && record[0x70] == 3 && record[0x0c] == 10 &&
           record[0x14] == 12 && record[0x18] == 0 && record[8] == 7);

    /* Op 8 rebuilds every resolved listed unit, caller included. */
    assert(aoe_command_execute_dependency_ror(record, 0x74, &ops) == 1);
    for (int i = 0; i < 3; ++i)
        assert(world.units[i].dependent_unit_count == 3 &&
               world.units[i].dependent_source_id == 10 &&
               world.units[i].dependent_context_bits == 7 &&
               world.units[i].dependent_unit_ids[2] == 12);

    /* Short records, oversized counts, and foreign opcodes are rejected. */
    assert(aoe_command_execute_dependency_ror(record, 0x73, &ops) == -1);
    record[0x70] = 26;
    assert(aoe_command_execute_dependency_ror(record, 0x74, &ops) == -1);
    record[0] = 7;
    assert(!aoe_command_is_dependency_ror(7) &&
           aoe_command_execute_dependency_ror(record, 0x74, &ops) == -1);

    /* Owner word mismatch is ignored before any unit operation. */
    assert(aoe_command_encode_dependency_remove_ror(
               record, sizeof(record), 4, 10, 12) == 0x0c);
    assert(aoe_command_execute_dependency_ror(record, 0x0c, &ops) == 0 &&
           world.units[0].dependent_unit_count == 3);
    world.owners[0] = 0x104; /* Word compare: high byte must also match. */
    record[1] = 4;
    assert(aoe_command_execute_dependency_ror(record, 0x0c, &ops) == 0);
    world.owners[0] = 3;

    /* Op 0xf removes 12 from non-self members (keeping their source),
       then from the caller through 004ac030. */
    record[1] = 3;
    assert(aoe_command_execute_dependency_ror(record, 0x0c, &ops) == 1 &&
           world.units[0].dependent_unit_count == 2 &&
           world.units[1].dependent_unit_count == 2 &&
           world.units[2].dependent_unit_count == 2 &&
           world.units[1].dependent_source_id == 10);

    /* An unresolved acting unit is ignored. */
    assert(aoe_command_encode_dependency_clear_ror(
               record, sizeof(record), 3, 99) == 8 &&
           aoe_command_execute_dependency_ror(record, 8, &ops) == 0);

    /* Op 9 clears the group: members first, then the caller. */
    assert(aoe_command_encode_dependency_clear_ror(
               record, sizeof(record), 3, 10) == 8 &&
           aoe_command_execute_dependency_ror(record, 8, &ops) == 1);
    for (int i = 0; i < 2; ++i)
        assert(!world.units[i].dependent_unit_count &&
               world.units[i].dependent_source_id == -1);
    /* Unit 12 was no longer a member of 10's group. */
    assert(world.units[2].dependent_unit_count == 2);

    /* Op 0xe adds the target to each non-self member, then rebuilds the
       caller from [10, 11, 11]; 004abc80 drops the duplicate. */
    assert(aoe_command_encode_dependency_propagate_ror(
               record, sizeof(record), 3, 12, 11, 5) == 0x10 &&
           aoe_command_execute_dependency_ror(record, 0x10, &ops) == 1 &&
           world.units[2].dependent_source_id == 12 &&
           world.units[2].dependent_context_bits == 5 &&
           world.units[2].dependent_unit_count == 2 &&
           world.units[0].dependent_unit_count == 1 &&
           world.units[0].dependent_unit_ids[0] == 11);

    assert(aoe_command_execute_dependency_ror(record, 0x10, NULL) == -1);
    for (int i = 0; i < 3; ++i) free(world.units[i].dependent_unit_ids);
    return 0;
}
