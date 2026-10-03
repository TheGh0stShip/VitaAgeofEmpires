#include "command_building.h"

#include <string.h>

#include "le_bytes.h"

static int16_t le_i16(const uint8_t *in) {
    return (int16_t)(uint16_t)(in[0] | (uint16_t)in[1] << 8);
}

static void put_i16(uint8_t *out, int16_t value) {
    out[0] = (uint8_t)(uint16_t)value;
    out[1] = (uint8_t)((uint16_t)value >> 8);
}

size_t aoe_command_encode_building_queue_ror(
    uint8_t *out, size_t out_size, int32_t building_id, int16_t unit_id,
    int16_t count) {
    if (!out || out_size < AOE_COMMAND_BUILDING_QUEUE_SIZE_ROR) return 0;
    memset(out, 0, AOE_COMMAND_BUILDING_QUEUE_SIZE_ROR);
    out[0] = AOE_COMMAND_BUILDING_QUEUE_ROR;
    aoe_le_put_u32(out + 4, (uint32_t)building_id);
    put_i16(out + 8, unit_id);
    put_i16(out + 10, count);
    return AOE_COMMAND_BUILDING_QUEUE_SIZE_ROR;
}

int aoe_command_execute_building_queue_ror(
    const uint8_t *command, size_t size, const AoeCommandBuildingOps *ops) {
    AoeLiveUnit *unit;
    int16_t unit_id, count, removed;
    if (!command || size < AOE_COMMAND_BUILDING_QUEUE_SIZE_ROR ||
        command[0] != AOE_COMMAND_BUILDING_QUEUE_ROR || !ops ||
        !ops->world.resolve)
        return -1;
    unit = ops->world.resolve(ops->world.context, aoe_le_i32(command + 4));
    if (!unit) return 0;
    if (!unit->building_update) return -1;
    unit_id = le_i16(command + 8);
    count = le_i16(command + 10);
    if (count > 0)
        return aoe_building_queue_add_ror(unit->building_update, unit_id,
                                          count, &ops->queue) ? 1 : -1;
    /* `neg edx`; only the low word reaches the int16 parameter. */
    removed = (int16_t)(uint16_t)(0u - (uint16_t)count);
    if (removed < 0 || !ops->queue.owner_status_four_count) return -1;
    return aoe_building_queue_cancel_ror(unit->building_update, unit_id,
                                         removed,
                                         ops->queue.owner_status_four_count,
                                         &ops->queue.cancel) ? 1 : -1;
}
