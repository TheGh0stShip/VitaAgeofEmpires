#ifndef VITA_AOE_COMMAND_PRODUCTION_H
#define VITA_AOE_COMMAND_PRODUCTION_H

#include <stddef.h>
#include <stdint.h>

#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a tribe commands 0x64 (train, 0x004e85e0) and
   0x65 (research, 0x004e8780). Builders 0x004e9490/0x004e94e0 (train) and
   0x004e9530/0x004e9580 (research) calloc 0x10 bytes: building ID +0x04,
   player byte +0x08, int16 unit/tech ID +0x0a, request ID +0x0c (-1 from
   the interface builders). */
enum {
    AOE_COMMAND_TRAIN_ROR = 0x64,
    AOE_COMMAND_RESEARCH_ROR = 0x65,
    AOE_COMMAND_PRODUCTION_SIZE_ROR = 0x10
};

typedef struct {
    AoeLiveUnitDependencyOps world; /* 0x005206d0 */
    void *context;
    /* 0x004ad870: type of the object's current head action (0 when idle);
       0x66 (train) and 0x67 (research) mean busy. */
    int16_t (*active_action_type)(void *context, const AoeLiveUnit *unit);
    /* Owner +0x24[unit] template (its train_location_id, +0x15c). */
    const DatUnit *(*owner_unit_template)(void *context,
                                          const AoeLiveUnit *building,
                                          int16_t unit_index);
    /* Owner tech tree +0x224 entry +0x28 for the tech. */
    int16_t (*research_location)(void *context, const AoeLiveUnit *building,
                                 int16_t tech);
    /* Allocate and construct the 0x50-byte train action 0x004b4f10(
       building, unit, request, 0) or the 0x48-byte research action
       0x004b60b0(building, tech, request), then add it through building
       vtable +0x208. An allocation failure adds nothing. */
    void (*start_train)(void *context, AoeLiveUnit *building, int16_t unit,
                        int32_t request);
    void (*start_research)(void *context, AoeLiveUnit *building,
                           int16_t tech, int32_t request);
    /* Building +0x74 group object (0x00407a60), then 0x0040f6d0(0x26c) and
       0x00412f50(request, -1, -1.0f, -1.0f, -1.0f) on it. */
    int (*has_group_object)(void *context, const AoeLiveUnit *unit);
    void (*group_production)(void *context, AoeLiveUnit *building,
                             int32_t request);
    /* Fallback on players[+0x08]: train +0x10c(0, unit, building_id,
       request); research +0x118(tech, building_id, -1, request). */
    void (*player_train)(void *context, uint8_t player, int16_t unit,
                         int32_t building_id, int32_t request);
    void (*player_research)(void *context, uint8_t player, int16_t tech,
                            int32_t building_id, int32_t request);
} AoeCommandProductionOps;

size_t aoe_command_encode_production_ror(uint8_t *out, size_t out_size,
                                         uint8_t opcode, int32_t building_id,
                                         uint8_t player, int16_t item,
                                         int32_t request);

/* When the building resolves and is not already training or researching:
   train requires the unit template's train location to equal the
   building's template ID or be -1, research requires an exact location
   match; a mismatch does nothing. Otherwise the action starts and a
   building with a group object reports it. When the building is missing
   or busy, the player fallback runs instead. Returns -1 for a malformed
   record, missing dependencies, or a missing template; 0 for a location
   mismatch; 1 otherwise. */
int aoe_command_execute_production_ror(const uint8_t *command, size_t size,
                                       const AoeCommandProductionOps *ops);

#endif
