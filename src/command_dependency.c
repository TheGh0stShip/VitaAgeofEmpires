#include "command_dependency.h"

#include <string.h>

#include "le_bytes.h"

static size_t begin(uint8_t *out, size_t out_size, size_t size,
                    uint8_t opcode, uint8_t player_id, int32_t unit_id) {
    if (!out || out_size < size) return 0;
    memset(out, 0, size); /* 0x0052607b is calloc(1, size). */
    out[0] = opcode;
    out[1] = player_id;
    aoe_le_put_u32(out + 4, (uint32_t)unit_id);
    return size;
}

size_t aoe_command_encode_dependency_set_ror(
    uint8_t *out, size_t out_size, uint8_t player_id, int32_t unit_id,
    const int32_t *unit_ids, int32_t count, uint32_t context_bits) {
    if (count < 0 || count > AOE_COMMAND_DEPENDENCY_SET_MAX_IDS_ROR ||
        (count && !unit_ids) ||
        !begin(out, out_size, AOE_COMMAND_DEPENDENCY_SET_SIZE_ROR,
               AOE_COMMAND_DEPENDENCY_SET_ROR, player_id, unit_id))
        return 0;
    for (int32_t i = 0; i < count; ++i)
        aoe_le_put_u32(out + 0x0c + 4 * i, (uint32_t)unit_ids[i]);
    aoe_le_put_u32(out + 8, context_bits);
    out[0x70] = (uint8_t)count;
    return AOE_COMMAND_DEPENDENCY_SET_SIZE_ROR;
}

size_t aoe_command_encode_dependency_clear_ror(
    uint8_t *out, size_t out_size, uint8_t player_id, int32_t unit_id) {
    return begin(out, out_size, AOE_COMMAND_DEPENDENCY_CLEAR_SIZE_ROR,
                 AOE_COMMAND_DEPENDENCY_CLEAR_ROR, player_id, unit_id);
}

size_t aoe_command_encode_dependency_propagate_ror(
    uint8_t *out, size_t out_size, uint8_t player_id, int32_t unit_id,
    int32_t target_unit_id, uint32_t context_bits) {
    if (!begin(out, out_size, AOE_COMMAND_DEPENDENCY_PROPAGATE_SIZE_ROR,
               AOE_COMMAND_DEPENDENCY_PROPAGATE_ROR, player_id, unit_id))
        return 0;
    aoe_le_put_u32(out + 0x0c, (uint32_t)target_unit_id);
    aoe_le_put_u32(out + 8, context_bits);
    return AOE_COMMAND_DEPENDENCY_PROPAGATE_SIZE_ROR;
}

size_t aoe_command_encode_dependency_remove_ror(
    uint8_t *out, size_t out_size, uint8_t player_id, int32_t unit_id,
    int32_t removed_unit_id) {
    if (!begin(out, out_size, AOE_COMMAND_DEPENDENCY_REMOVE_SIZE_ROR,
               AOE_COMMAND_DEPENDENCY_REMOVE_ROR, player_id, unit_id))
        return 0;
    aoe_le_put_u32(out + 8, (uint32_t)removed_unit_id);
    return AOE_COMMAND_DEPENDENCY_REMOVE_SIZE_ROR;
}

static size_t record_size(uint8_t opcode) {
    switch (opcode) {
    case AOE_COMMAND_DEPENDENCY_SET_ROR:
        return AOE_COMMAND_DEPENDENCY_SET_SIZE_ROR;
    case AOE_COMMAND_DEPENDENCY_CLEAR_ROR:
        return AOE_COMMAND_DEPENDENCY_CLEAR_SIZE_ROR;
    case AOE_COMMAND_DEPENDENCY_PROPAGATE_ROR:
        return AOE_COMMAND_DEPENDENCY_PROPAGATE_SIZE_ROR;
    case AOE_COMMAND_DEPENDENCY_REMOVE_ROR:
        return AOE_COMMAND_DEPENDENCY_REMOVE_SIZE_ROR;
    default:
        return 0;
    }
}

int aoe_command_is_dependency_ror(uint8_t opcode) {
    return record_size(opcode) != 0;
}

int aoe_command_execute_dependency_ror(
    const uint8_t *command, size_t size, const AoeCommandDependencyOps *ops) {
    size_t required;
    AoeLiveUnit *unit;
    if (!command || !size || !ops || !ops->world.resolve ||
        !ops->owner_player_id)
        return -1;
    required = record_size(command[0]);
    if (!required || size < required) return -1;
    if (command[0] == AOE_COMMAND_DEPENDENCY_SET_ROR &&
        command[0x70] > AOE_COMMAND_DEPENDENCY_SET_MAX_IDS_ROR)
        return -1;
    /* 0042b460/0042b4a0/0042b4e0/0042b520 share this owner gate. */
    unit = ops->world.resolve(ops->world.context, aoe_le_i32(command + 4));
    if (!unit || ops->owner_player_id(ops->context, unit) != command[1])
        return 0;
    switch (command[0]) {
    case AOE_COMMAND_DEPENDENCY_SET_ROR: {
        int32_t ids[AOE_COMMAND_DEPENDENCY_SET_MAX_IDS_ROR];
        for (uint8_t i = 0; i < command[0x70]; ++i)
            ids[i] = aoe_le_i32(command + 0x0c + 4 * i);
        (void)aoe_live_unit_set_dependency_group_ror(
            unit, ids, command[0x70], aoe_le_i32(command + 4),
            aoe_le_u32(command + 8), &ops->world);
        break;
    }
    case AOE_COMMAND_DEPENDENCY_CLEAR_ROR:
        (void)aoe_live_unit_clear_dependency_group_ror(unit, &ops->world);
        break;
    case AOE_COMMAND_DEPENDENCY_PROPAGATE_ROR:
        (void)aoe_live_unit_propagate_dependency_ror(
            unit, aoe_le_i32(command + 0x0c), aoe_le_u32(command + 8),
            &ops->world);
        break;
    default:
        (void)aoe_live_unit_remove_dependency_group_ror(
            unit, aoe_le_i32(command + 8), &ops->world);
        break;
    }
    return 1;
}
