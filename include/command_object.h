#ifndef VITA_AOE_COMMAND_OBJECT_H
#define VITA_AOE_COMMAND_OBJECT_H

#include <stddef.h>
#include <stdint.h>

#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a command opcode 4, object creation. Builder
   0x0042ba90 callocs 0x14 bytes: template index int16 +0x02, player byte
   +0x04, and floats x/y/z at +0x08/+0x0c/+0x10. The base processor handler
   0x0042acd0 calls the template's vtable +0x18 factory with (player, x, y,
   z) and ignores the result. The tribe processor that real sessions run
   overrides it with 0x004e8570, which:
   - writes z = 4.0f into the record first when template +0x68
     (elevation_mode) is 1;
   - after creation, if the new unit's template type (+0x04) is 80, calls
     0x004ad7b0 with 10000.0f so the building completes immediately. */
enum {
    AOE_COMMAND_OBJECT_CREATE_ROR = 0x04,
    AOE_COMMAND_OBJECT_CREATE_SIZE_ROR = 0x14
};

size_t aoe_command_encode_object_create_ror(
    uint8_t *out, size_t out_size, int16_t template_index,
    uint8_t player_index, float x, float y, float z);

typedef struct {
    void *context;
    /* Player +0x24 template table entry. The original indexes it without a
       bound, so NULL is treated as malformed. */
    const DatUnit *(*player_template)(void *context, uint8_t player_index,
                                      int16_t template_index);
    /* Template vtable +0x18 (player, x, y, z). */
    AoeLiveUnit *(*create)(void *context, uint8_t player_index,
                           const DatUnit *unit_template, float x, float y,
                           float z);
    AoeLiveUnitSetStateOps state; /* tribe build completion only */
} AoeCommandObjectOps;

/* Return -1 for a wrong opcode, short record, missing dependency or
   unresolved template; otherwise 1. A failed creation is ignored by the
   base handler. The tribe handler would dereference a NULL result, so here
   it returns -1 (host-only guard). The tribe handler's z override is
   applied to a local copy; the original mutates the record, which is
   released right after dispatch. */
int aoe_command_execute_object_create_ror(
    const uint8_t *command, size_t size, const AoeCommandObjectOps *ops);
int aoe_command_execute_object_create_tribe_ror(
    const uint8_t *command, size_t size, const AoeCommandObjectOps *ops);

#endif
