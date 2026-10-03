#ifndef VITA_AOE_COMMAND_PLAYER_H
#define VITA_AOE_COMMAND_PLAYER_H

#include <stddef.h>
#include <stdint.h>

#include "player_resources.h"

/* Patched Rise of Rome 1.0a command opcode 5 (dispatcher 0x0042a180,
   handler 0x0042ad10). Builder 0x0042bae0, called from player vtable
   +0xb4 = 0x0045e580, callocs 8 bytes: opcode +0x00, player byte +0x01,
   resource byte +0x02, and 32-bit float amount +0x04. */
enum {
    AOE_COMMAND_PLAYER_RESOURCE_ADD_ROR = 0x05,
    AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR = 0x08
};

/* Stores the float's exact 32-bit pattern little-endian. Returns the record
   size, or 0 when `out_size` is too small. */
size_t aoe_command_encode_player_resource_add_ror(
    uint8_t *out, size_t out_size, uint8_t player_index, uint8_t resource,
    float amount);

typedef struct {
    void *context;
    /* World +0x40 player table entry. The original does not bound the byte
       against world +0x3c, so a NULL result is treated as malformed. */
    const AoePlayerResourceTargetRor *(*player)(void *context,
                                                uint8_t player_index);
} AoeCommandPlayerOps;

/* Calls player vtable +0x78 with the zero-extended resource word, the
   amount, and flag 0. Returns -1 for a wrong opcode, short record, missing
   dependency or unresolved player; otherwise the virtual add's result. */
int aoe_command_execute_player_resource_add_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerOps *ops);

#endif
