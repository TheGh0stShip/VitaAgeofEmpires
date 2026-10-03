#ifndef VITA_AOE_COMMAND_TRIBE_UNIT_H
#define VITA_AOE_COMMAND_TRIBE_UNIT_H

#include <stddef.h>
#include <stdint.h>

#include "command_player_task.h"

/* Patched Rise of Rome 1.0a tribe-processor unit commands. List records
   carry a count byte at +0x01 and 32-bit unit IDs from the listed offset;
   IDs go through 0x0042a120 (-1 never resolves). */
enum {
    AOE_COMMAND_TRIBE_UNIT_244_ROR = 0x6a,   /* 0x004e9130, ID +0x04 */
    AOE_COMMAND_TRIBE_UNIT_14C_ROR = 0x6b,   /* 0x004e9160, IDs +0x0c */
    AOE_COMMAND_TRIBE_SET_4C_ROR = 0x6d,     /* 0x004e9340, IDs +0x08 */
    AOE_COMMAND_TRIBE_REPAIR_ROR = 0x6e,     /* 0x004e9380, IDs +0x08 */
    AOE_COMMAND_TRIBE_GROUP_POINT_ROR = 0x6f /* 0x004e93d0, IDs +0x0c */
};

typedef struct {
    AoeCommandPlayerTaskOps player; /* world lookup, owners, +0x34 path */
    /* Class virtuals that are not translated yet. */
    void (*unit_244)(void *context, AoeLiveUnit *unit);
    void (*unit_14c)(void *context, AoeLiveUnit *unit, uint32_t a,
                     uint32_t b);                  /* +0x14c(a, b, 0, 1) */
    /* 0x004b0870: pick the actor's action-0x6a task for `source` and
       build the action. */
    void (*repair)(void *context, AoeLiveUnit *unit, AoeLiveUnit *source);
} AoeCommandTribeUnitOps;

/* Writes the opcode, count byte and IDs at `ids_offset`, zero-filling the
   header; callers set the remaining header fields. */
size_t aoe_command_encode_unit_list_ror(uint8_t *out, size_t out_size,
                                        uint8_t opcode, size_t ids_offset,
                                        const int32_t *unit_ids,
                                        uint8_t count);

/* One entry point for 0x6a/0x6b/0x6d/0x6e/0x6f:
   0x6a: the unit at +0x04 (direct lookup) runs vtable +0x244.
   0x6b: each unit runs +0x14c(+0x04, +0x08, 0, 1).
   0x6d: each unit's word +0x4c (carried resource type) becomes the int16
         at +0x04.
   0x6e: the source at +0x04 is resolved once; each unit runs 0x004b0870
         with it.
   0x6f: each unit runs 0x004ac320, then its own owner's +0x34 receives
         (owner, unit, 0x2d1, unit, owner, x = +0x04, y = +0x08, 0,
         1.0f, 1, 0, 100).
   Returns -1 for a malformed record or missing dependencies, else 1. */
int aoe_command_execute_tribe_unit_ror(const uint8_t *command, size_t size,
                                       const AoeCommandTribeUnitOps *ops);

#endif
