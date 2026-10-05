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
    AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR = 0x08,
    /* Opcode 7: processor vtable slot +0x00 = 0x0042ad40 in both the base
       (0x00543498) and derived (0x00549904) processor vtables. Builder
       0x0042bb20 (processor slot +0x0c) callocs 8 bytes: sender byte +0x01,
       receiver byte +0x02, resource byte +0x03, float amount +0x04. */
    AOE_COMMAND_PLAYER_TRIBUTE_ROR = 0x07,
    AOE_COMMAND_PLAYER_TRIBUTE_SIZE_ROR = 0x08,
    /* Constant pushed to receiver vtable +0xe8 by 0x0042ad40. */
    AOE_COMMAND_PLAYER_TRIBUTE_NOTICE_ROR = 0x20a,
    /* Tribe opcode 0x6c: derived processor vtable +0x10 = 0x004e91b0,
       built by +0x14 = 0x004e9a10 (12 bytes: sender +0x01, receiver +0x02,
       resource +0x03, float amount +0x04, float fee rate +0x08). */
    AOE_COMMAND_PLAYER_TRIBUTE_FEE_ROR = 0x6c,
    AOE_COMMAND_PLAYER_TRIBUTE_FEE_SIZE_ROR = 0x0c,
    /* Resource receiving the tribute balance in 0x004e91b0. */
    AOE_PLAYER_TRIBUTE_BALANCE_ROR = 0x35
};

/* Stores the float's exact 32-bit pattern little-endian. Returns the record
   size, or 0 when `out_size` is too small. */
size_t aoe_command_encode_player_resource_add_ror(
    uint8_t *out, size_t out_size, uint8_t player_index, uint8_t resource,
    float amount);
size_t aoe_command_encode_player_tribute_ror(
    uint8_t *out, size_t out_size, uint8_t sender, uint8_t receiver,
    uint8_t resource, float amount);

typedef struct {
    void *context;
    /* World +0x40 player table entry. The original does not bound the byte
       against world +0x3c, so a NULL result is treated as malformed. */
    const AoePlayerResourceTargetRor *(*player)(void *context,
                                                uint8_t player_index);
    /* Tribute only. Global game *0x00580e38 vtable +0x40, called as
       (7, sender, receiver, resource, __ftol(amount)). */
    void (*game_event)(void *context, uint32_t kind, uint32_t sender,
                       uint32_t receiver, uint32_t resource, int32_t amount);
    /* Tribute only. Receiver player vtable +0xe8 (base 0x0045e930, derived
       0x004f3350), called as (sender, receiver, 0x20a, resource,
       __ftol(amount), 0); the trailing 0 is visible only in disassembly. */
    void (*receiver_notice)(void *context, uint8_t receiver_index,
                            uint32_t sender, uint32_t receiver,
                            uint32_t notice, uint32_t resource,
                            int32_t amount, uint32_t zero);
} AoeCommandPlayerOps;

/* Calls player vtable +0x78 with the zero-extended resource word, the
   amount, and flag 0. Returns -1 for a wrong opcode, short record, missing
   dependency or unresolved player; otherwise the virtual add's result. */
int aoe_command_execute_player_resource_add_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerOps *ops);

/* 0x0042ad40. Proceeds only when the sender's raw value for the resource is
   >= amount (an unordered NaN compare fails). Then, in order: sender
   +0x78(resource, -amount, 0); receiver +0x78(resource, amount, 0); the
   game event; the receiver notice. There is no tribute fee. Returns 0 when
   the gate fails, 1 when performed, and -1 for a malformed record, missing
   dependency, unresolved player, or a resource byte at or beyond the
   sender's +0x4c count. The original reads +0x50[resource] unchecked; that
   rejection, and resolving both players before any mutation, are host-only
   guards. */
int aoe_command_execute_player_tribute_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerOps *ops);

size_t aoe_command_encode_player_tribute_fee_ror(
    uint8_t *out, size_t out_size, uint8_t sender, uint8_t receiver,
    uint8_t resource, float amount, float fee_rate);

/* 0x004e91b0, the tribute used by tribe sessions. A = amount, or 0 when
   negative or NaN; fee = (float)(rate * A) when A > 0. If A + fee
   (unrounded) exceeds the sender's raw stock S, fee = (float)(S * rate)
   and A = (float)(S - fee); there is no rejection. Then, in order:
   receiver +0x78(resource, A); sender +0x78(resource, -total) with total =
   (float)(A + fee); sender +0x78(0x35, total); receiver +0x78(0x35, -A);
   game event (7, sender, receiver, resource, __ftol(A)); receiver notice
   (sender, receiver, 0x20a, resource, __ftol(requested amount), 0).
   Returns 1, or -1 under the same host guards as opcode 7. */
int aoe_command_execute_player_tribute_fee_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerOps *ops);

#endif
