#include "command_tribe_unit.h"

#include <string.h>

#include "aoe_ftol.h"
#include "le_bytes.h"

size_t aoe_command_encode_unit_list_ror(uint8_t *out, size_t out_size,
                                        uint8_t opcode, size_t ids_offset,
                                        const int32_t *unit_ids,
                                        uint8_t count) {
    size_t size = ids_offset + 4u * count;
    if (!out || ids_offset < 2 || out_size < size || (count && !unit_ids))
        return 0;
    memset(out, 0, size);
    out[0] = opcode;
    out[1] = count;
    for (uint8_t i = 0; i < count; ++i)
        aoe_le_put_u32(out + ids_offset + 4u * i, (uint32_t)unit_ids[i]);
    return size;
}

static AoeLiveUnit *lookup(const AoeCommandTribeUnitOps *ops, int32_t id) {
    return id == -1 ? NULL
                    : ops->player.world.resolve(ops->player.world.context, id);
}

int aoe_command_execute_tribe_unit_ror(const uint8_t *command, size_t size,
                                       const AoeCommandTribeUnitOps *ops) {
    size_t ids;
    AoeLiveUnit *source = NULL;
    if (!command || size < 2 || !ops || !ops->player.world.resolve) return -1;
    switch (command[0]) {
    case AOE_COMMAND_TRIBE_UNIT_244_ROR: {
        AoeLiveUnit *unit;
        if (size < 8 || !ops->unit_244) return -1;
        unit = ops->player.world.resolve(ops->player.world.context,
                                         aoe_le_i32(command + 4));
        if (unit) ops->unit_244(ops->player.context, unit);
        return 1;
    }
    case AOE_COMMAND_TRIBE_UNIT_14C_ROR:
        if (!ops->unit_14c) return -1;
        ids = 0x0c;
        break;
    case AOE_COMMAND_TRIBE_POINT_17C_ROR:
        if (!ops->player_present || !ops->unit_17c ||
            !ops->player.owner_kind)
            return -1;
        if (size < 0x0c + 4u * command[1]) return -1;
        if (!ops->player_present(ops->player.context, command[2])) return 1;
        ids = 0x0c;
        break;
    case AOE_COMMAND_TRIBE_SET_4C_ROR:
        ids = 0x08;
        break;
    case AOE_COMMAND_TRIBE_REPAIR_ROR:
        if (!ops->repair) return -1;
        ids = 0x08;
        break;
    case AOE_COMMAND_TRIBE_GROUP_POINT_ROR:
        if (!ops->player.owner_id || !ops->player.owner_kind) return -1;
        ids = 0x0c;
        break;
    default:
        return -1;
    }
    if (size < ids + 4u * command[1]) return -1;
    if (command[0] == AOE_COMMAND_TRIBE_REPAIR_ROR)
        source = lookup(ops, aoe_le_i32(command + 4));
    for (unsigned i = 0; i < command[1]; ++i) {
        AoeLiveUnit *unit = lookup(ops, aoe_le_i32(command + ids + 4u * i));
        if (!unit) continue;
        switch (command[0]) {
        case AOE_COMMAND_TRIBE_UNIT_14C_ROR:
            ops->unit_14c(ops->player.context, unit, aoe_le_u32(command + 4),
                          aoe_le_u32(command + 8));
            break;
        case AOE_COMMAND_TRIBE_POINT_17C_ROR:
            if (!aoe_live_unit_reset_group_ror(
                    unit, ops->player.owner_kind(ops->player.context, unit)))
                return -1;
            ops->unit_17c(ops->player.context, unit,
                          aoe_ftol32(aoe_le_f32(command + 4)),
                          aoe_ftol32(aoe_le_f32(command + 8)));
            break;
        case AOE_COMMAND_TRIBE_SET_4C_ROR:
            unit->core.storage.carried_resource_type = (int16_t)(uint16_t)(
                command[4] | (uint16_t)command[5] << 8);
            break;
        case AOE_COMMAND_TRIBE_REPAIR_ROR:
            ops->repair(ops->player.context, unit, source);
            break;
        default: {
            int16_t owner = ops->player.owner_id(ops->player.context, unit);
            AoePlayerUnitTaskRor task;
            if (!aoe_live_unit_reset_group_ror(
                    unit, ops->player.owner_kind(ops->player.context, unit)))
                return -1;
            task.mode = (uint8_t)owner;
            task.unit_id = unit->core.unit_id;
            task.value_16 = 0x2d1;
            task.value_32 = (uint32_t)unit->core.unit_id;
            task.flag = (uint8_t)owner;
            task.x_bits = aoe_le_u32(command + 4);
            task.y_bits = aoe_le_u32(command + 8);
            task.z_bits = 0;
            task.extra_32 = UINT32_C(0x3f800000);
            task.extra_a = 1;
            task.extra_b = 0;
            task.extra_c = 100;
            if (!aoe_player_unit_task_ror(owner, &task, &ops->player))
                return -1;
            break;
        }
        }
    }
    return 1;
}
