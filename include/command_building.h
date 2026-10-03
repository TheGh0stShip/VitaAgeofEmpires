#ifndef VITA_AOE_COMMAND_BUILDING_H
#define VITA_AOE_COMMAND_BUILDING_H

#include <stddef.h>
#include <stdint.h>

#include "building_update.h"
#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a tribe command 0x77 (handler 0x004e9450).
   Builder 0x004e9c20 callocs 0x0c bytes: building ID +0x04 (copied from
   the building's +0x04), int16 unit ID +0x08, int16 count +0x0a. No player
   byte is written and the handler performs no owner check. A positive
   count calls queue add 0x004adab0; otherwise queue cancel 0x004add50
   receives the 16-bit negation of the count. */
enum {
    AOE_COMMAND_BUILDING_QUEUE_ROR = 0x77,
    AOE_COMMAND_BUILDING_QUEUE_SIZE_ROR = 0x0c
};

size_t aoe_command_encode_building_queue_ror(
    uint8_t *out, size_t out_size, int32_t building_id, int16_t unit_id,
    int16_t count);

typedef struct {
    AoeLiveUnitDependencyOps world; /* 0x005206d0 lookup of +0x04 */
    AoeBuildingQueueAddOps queue;   /* add, and cancel through queue.cancel */
} AoeCommandBuildingOps;

/* Returns 0 when the ID does not resolve (ignored by the original), 1
   after the queue operation succeeds, and -1 for a malformed record,
   missing dependencies, or a failed operation. The original would call the
   queue methods on any resolved object; a resolved object without a
   building binding is rejected here as a host-only guard. The cancel path
   rejects the one count whose negation stays negative (-32768), as the
   translated cancel only supports nonnegative amounts. */
int aoe_command_execute_building_queue_ror(
    const uint8_t *command, size_t size, const AoeCommandBuildingOps *ops);

#endif
