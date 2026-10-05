#include "command_player_task.h"

#include <string.h>

#include "le_bytes.h"

static int16_t le_i16(const uint8_t *in) {
    return (int16_t)(uint16_t)(in[0] | (uint16_t)in[1] << 8);
}

static AoeLiveUnit *resolve(const AoeCommandPlayerTaskOps *ops, int32_t id) {
    return ops->world.resolve(ops->world.context, id);
}

/* 0x0042a120 additionally treats -1 as "no unit". */
static AoeLiveUnit *lookup(const AoeCommandPlayerTaskOps *ops, int32_t id) {
    return id == -1 ? NULL : resolve(ops, id);
}

int aoe_player_unit_task_ror(int16_t player_id,
                             const AoePlayerUnitTaskRor *task,
                             const AoeCommandPlayerTaskOps *ops) {
    AoeLiveUnit *unit;
    if (!task || !ops || !ops->world.resolve || !ops->has_group_object ||
        !ops->owner_id || !ops->group_task)
        return 0;
    unit = resolve(ops, task->unit_id);
    if (unit && ops->has_group_object(ops->context, unit) &&
        ops->owner_id(ops->context, unit) == player_id)
        ops->group_task(ops->context, unit, task);
    return 1;
}

int aoe_player_unit_records_ror(int32_t unit_id, const uint8_t *records,
                                int32_t count,
                                const AoeCommandPlayerTaskOps *ops) {
    AoeLiveUnit *unit;
    if (!ops || !ops->world.resolve || !ops->unit_record ||
        (count > 0 && !records))
        return 0;
    unit = resolve(ops, unit_id);
    if (!unit) return 1;
    for (int32_t i = 0; i < count; ++i)
        ops->unit_record(ops->context, unit, records + 3 * i);
    return 1;
}

int aoe_player_resign_ror(uint8_t player_index, uint32_t value,
                          const AoeCommandPlayerTaskOps *ops) {
    uint8_t *resigned, was;
    if (!ops || !ops->player_resigned || !ops->player_state ||
        !ops->game_event || !ops->player_id ||
        !(resigned = ops->player_resigned(ops->context, player_index)))
        return 0;
    was = *resigned;
    *resigned = 1;
    ops->player_state(ops->context, player_index, 2);
    if (!was)
        ops->game_event(ops->context, 5,
                        ops->player_id(ops->context, player_index), value);
    return 1;
}

size_t aoe_command_encode_unit_task_ror(uint8_t *out, size_t out_size,
                                        uint8_t player,
                                        const AoePlayerUnitTaskRor *task) {
    if (!out || !task || out_size < AOE_COMMAND_UNIT_TASK_SIZE_ROR) return 0;
    memset(out, 0, AOE_COMMAND_UNIT_TASK_SIZE_ROR);
    out[0] = AOE_COMMAND_UNIT_TASK_ROR;
    out[1] = player;
    out[2] = task->mode;
    aoe_le_put_u32(out + 0x04, (uint32_t)task->unit_id);
    out[0x08] = (uint8_t)(uint16_t)task->value_16;
    out[0x09] = (uint8_t)((uint16_t)task->value_16 >> 8);
    out[0x0a] = task->extra_c;
    aoe_le_put_u32(out + 0x0c, task->value_32);
    out[0x10] = task->flag;
    aoe_le_put_u32(out + 0x14, task->x_bits);
    aoe_le_put_u32(out + 0x18, task->y_bits);
    aoe_le_put_u32(out + 0x1c, task->z_bits);
    aoe_le_put_u32(out + 0x20, task->extra_32);
    out[0x24] = task->extra_a;
    out[0x25] = task->extra_b;
    return AOE_COMMAND_UNIT_TASK_SIZE_ROR;
}

int aoe_command_execute_unit_task_ror(const uint8_t *command, size_t size,
                                      const AoeCommandPlayerTaskOps *ops) {
    AoeLiveUnit *unit;
    AoePlayerUnitTaskRor task;
    if (!command || size < AOE_COMMAND_UNIT_TASK_SIZE_ROR ||
        command[0] != AOE_COMMAND_UNIT_TASK_ROR || !ops ||
        !ops->world.resolve || !ops->owner_kind || !ops->player_id)
        return -1;
    unit = resolve(ops, aoe_le_i32(command + 4));
    if (!unit) return 0;
    if (!aoe_live_unit_reset_group_ror(unit, ops->owner_kind(ops->context,
                                                              unit)))
        return -1;
    task.mode = command[2];
    task.unit_id = aoe_le_i32(command + 0x04);
    task.value_16 = le_i16(command + 0x08);
    task.value_32 = aoe_le_u32(command + 0x0c);
    task.flag = command[0x10];
    task.x_bits = aoe_le_u32(command + 0x14);
    task.y_bits = aoe_le_u32(command + 0x18);
    task.z_bits = aoe_le_u32(command + 0x1c);
    task.extra_32 = aoe_le_u32(command + 0x20);
    task.extra_a = command[0x24];
    task.extra_b = command[0x25];
    task.extra_c = command[0x0a];
    return aoe_player_unit_task_ror(ops->player_id(ops->context, command[1]),
                                    &task, ops) ? 1 : -1;
}

static uint32_t float_bits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

int aoe_command_execute_unit_task_list_ror(const uint8_t *command, size_t size,
                                           const AoeCommandPlayerTaskOps *ops) {
    unsigned count;
    if (!command || size < AOE_COMMAND_UNIT_TASK_LIST_HEADER_ROR ||
        command[0] != AOE_COMMAND_UNIT_TASK_LIST_ROR || !ops ||
        !ops->world.resolve || !ops->owner_kind || !ops->player_id)
        return -1;
    count = command[3];
    if (size < AOE_COMMAND_UNIT_TASK_LIST_HEADER_ROR + 4u * count) return -1;
    for (unsigned i = 0; i < count; ++i) {
        AoeLiveUnit *unit =
            lookup(ops, aoe_le_i32(command + 0x24 + 4u * i));
        AoePlayerUnitTaskRor task;
        const uint32_t unset = UINT32_C(0xbf800000); /* -1.0f bits */
        if (!unit) continue;
        task.x_bits = aoe_le_u32(command + 0x10);
        task.y_bits = aoe_le_u32(command + 0x14);
        task.z_bits = aoe_le_u32(command + 0x18);
        if (task.x_bits == unset) task.x_bits = float_bits(unit->core.position.x38);
        if (task.y_bits == unset) task.y_bits = float_bits(unit->core.position.y3c);
        if (task.z_bits == unset) task.z_bits = float_bits(unit->core.position.z40);
        if (!aoe_live_unit_reset_group_ror(unit,
                                           ops->owner_kind(ops->context, unit)))
            return -1;
        task.mode = command[2];
        task.unit_id = unit->core.unit_id;
        task.value_16 = le_i16(command + 0x04);
        task.value_32 = aoe_le_u32(command + 0x08);
        task.flag = command[0x0c];
        task.extra_32 = aoe_le_u32(command + 0x1c);
        task.extra_a = command[0x20];
        task.extra_b = command[0x21];
        task.extra_c = command[0x06];
        if (!aoe_player_unit_task_ror(ops->player_id(ops->context, command[1]),
                                      &task, ops))
            return -1;
    }
    return 1;
}

int aoe_command_execute_resign_ror(const uint8_t *command, size_t size,
                                   const AoeCommandPlayerTaskOps *ops) {
    int controlled = 0, left = 0;
    if (!command || size < 4 || command[0] != AOE_COMMAND_RESIGN_ROR ||
        !ops || !ops->session_check)
        return -1;
    if (!ops->session_check(ops->context))
        return aoe_player_resign_ror(command[1], command[3], ops) ? 1 : -1;
    if (!ops->slot_left || !ops->local_slot || !ops->local_left ||
        !ops->slot_player)
        return -1;
    ops->slot_left[command[2]] = 1;
    if (command[2] == ops->local_slot(ops->context))
        ops->local_left(ops->context);
    for (int slot = 1; slot < 9; ++slot)
        if (ops->slot_player(ops->context, slot) == command[1]) {
            ++controlled;
            if (ops->slot_left[slot]) ++left;
        }
    if (controlled != left) return 0;
    return aoe_player_resign_ror(command[1], command[3], ops) ? 1 : -1;
}

int aoe_command_execute_unit_records_ror(const uint8_t *command, size_t size,
                                         const AoeCommandPlayerTaskOps *ops) {
    if (!command || size < 9 || command[0] != AOE_COMMAND_UNIT_RECORDS_ROR ||
        !ops || !ops->world.resolve || size < 9u + 3u * command[8])
        return -1;
    if (!resolve(ops, aoe_le_i32(command + 4))) return 0;
    /* The issuing player's +0x48; every player class shares 0x0045bf90. */
    return aoe_player_unit_records_ror(aoe_le_i32(command + 4), command + 9,
                                       command[8], ops) ? 1 : -1;
}

int aoe_command_execute_unit_record_list_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerTaskOps *ops) {
    uint8_t record[3];
    if (!command || size < 4 || command[0] != AOE_COMMAND_UNIT_RECORD_LIST_ROR ||
        !ops || !ops->world.resolve || size < 4u + 4u * command[1])
        return -1;
    record[0] = command[2];
    record[1] = command[3];
    record[2] = 0;
    for (unsigned i = 0; i < command[1]; ++i) {
        AoeLiveUnit *unit = lookup(ops, aoe_le_i32(command + 4 + 4u * i));
        if (unit &&
            !aoe_player_unit_records_ror(unit->core.unit_id, record, 1, ops))
            return -1;
    }
    return 1;
}
