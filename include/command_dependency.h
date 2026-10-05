#ifndef VITA_AOE_COMMAND_DEPENDENCY_H
#define VITA_AOE_COMMAND_DEPENDENCY_H

#include <stddef.h>
#include <stdint.h>

#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a dependency-group commands dispatched by
   0x0042a180. Commands are little-endian byte records; the sizes are the
   zero-filled allocations made by the original builders before submission
   through 0x0042b6e0. Every record starts with opcode byte +0x00, player
   byte +0x01, and the acting unit ID at +0x04. */
enum {
    AOE_COMMAND_DEPENDENCY_SET_ROR = 0x08,       /* 0042bbe0 -> 0042b460 */
    AOE_COMMAND_DEPENDENCY_CLEAR_ROR = 0x09,     /* 0042bcc0 -> 0042b520 */
    AOE_COMMAND_DEPENDENCY_PROPAGATE_ROR = 0x0e, /* 0042bc40 -> 0042b4a0 */
    AOE_COMMAND_DEPENDENCY_REMOVE_ROR = 0x0f     /* 0042bc80 -> 0042b4e0 */
};

enum {
    AOE_COMMAND_DEPENDENCY_SET_SIZE_ROR = 0x74,
    AOE_COMMAND_DEPENDENCY_CLEAR_SIZE_ROR = 0x08,
    AOE_COMMAND_DEPENDENCY_PROPAGATE_SIZE_ROR = 0x10,
    AOE_COMMAND_DEPENDENCY_REMOVE_SIZE_ROR = 0x0c,
    /* Set records hold IDs at +0x0c..+0x6f and the count byte at +0x70. */
    AOE_COMMAND_DEPENDENCY_SET_MAX_IDS_ROR = 25
};

/* Builders return the record size, or 0 when `out_size` is too small. The
   set builder also rejects counts outside 0..25: the original copies the
   caller's count into the fixed 25-slot record and truncates it to a byte. */
size_t aoe_command_encode_dependency_set_ror(
    uint8_t *out, size_t out_size, uint8_t player_id, int32_t unit_id,
    const int32_t *unit_ids, int32_t count, uint32_t context_bits);
size_t aoe_command_encode_dependency_clear_ror(
    uint8_t *out, size_t out_size, uint8_t player_id, int32_t unit_id);
size_t aoe_command_encode_dependency_propagate_ror(
    uint8_t *out, size_t out_size, uint8_t player_id, int32_t unit_id,
    int32_t target_unit_id, uint32_t context_bits);
size_t aoe_command_encode_dependency_remove_ror(
    uint8_t *out, size_t out_size, uint8_t player_id, int32_t unit_id,
    int32_t removed_unit_id);

typedef struct {
    /* World lookup 0x005206d0 through command-processor +0x04. */
    AoeLiveUnitDependencyOps world;
    void *context;
    /* Word at the unit owner's +0x4a, compared with the zero-extended
       command player byte. */
    uint16_t (*owner_player_id)(void *context, const AoeLiveUnit *unit);
} AoeCommandDependencyOps;

int aoe_command_is_dependency_ror(uint8_t opcode);

/* Executes one dependency command. Returns -1 for missing dependencies, a
   non-dependency opcode, a short record, or a set count above 25 (the
   original would read past its 0x74-byte record; this is a host bounds
   guard). Returns 0 when the unit does not resolve or its owner differs,
   which the original ignores silently, and 1 after invoking the unit
   operation. The operation's own result is ignored, as in the executable. */
int aoe_command_execute_dependency_ror(
    const uint8_t *command, size_t size, const AoeCommandDependencyOps *ops);

#endif
