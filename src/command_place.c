#include "command_place.h"

#include <string.h>

#include "le_bytes.h"

size_t aoe_command_encode_place_ror(uint8_t *out, size_t out_size,
                                    uint8_t player, float x, float y,
                                    int16_t template_index, uint32_t value_10,
                                    const int32_t *builder_ids, uint8_t count) {
    size_t size = AOE_COMMAND_PLACE_HEADER_ROR + 4u * count;
    if (!out || out_size < size || (count && !builder_ids)) return 0;
    memset(out, 0, size);
    out[0] = AOE_COMMAND_PLACE_ROR;
    out[1] = count;
    out[2] = player;
    aoe_le_put_f32(out + 4, x);
    aoe_le_put_f32(out + 8, y);
    out[0x0c] = (uint8_t)(uint16_t)template_index;
    out[0x0d] = (uint8_t)((uint16_t)template_index >> 8);
    aoe_le_put_u32(out + 0x10, value_10);
    for (uint8_t i = 0; i < count; ++i)
        aoe_le_put_u32(out + 0x14 + 4u * i, (uint32_t)builder_ids[i]);
    return size;
}

static AoeLiveUnit *lookup(const AoeCommandPlaceOps *ops, int32_t id) {
    return id == -1 ? NULL : ops->world.resolve(ops->world.context, id);
}

static void report(const uint8_t *command, const AoeCommandPlaceOps *ops,
                   uint8_t code, uint32_t out, int16_t index) {
    AoeLiveUnit *builder;
    if (!command[1] || !(builder = lookup(ops, aoe_le_i32(command + 0x14))))
        return;
    ops->placement_failed(ops->context, command[2], code, out, index,
                          aoe_le_u32(command + 4), aoe_le_u32(command + 8),
                          aoe_le_u32(command + 0x10), builder->core.unit_id);
}

int aoe_command_execute_place_ror(const uint8_t *command, size_t size,
                                  const AoeCommandPlaceOps *ops) {
    const DatUnit *unit_template;
    AoeLiveUnit *foundation;
    int16_t index;
    uint32_t out;
    uint8_t code;
    float x, y;
    int paid;
    if (!command || size < AOE_COMMAND_PLACE_HEADER_ROR ||
        command[0] != AOE_COMMAND_PLACE_ROR || !ops || !ops->world.resolve ||
        !ops->player_present || !ops->player_template || !ops->resources ||
        !ops->placement_check || !ops->find_object ||
        !ops->placement_failed || !ops->create || !ops->owner_kind ||
        !ops->order || size < AOE_COMMAND_PLACE_HEADER_ROR + 4u * command[1])
        return -1;
    index = (int16_t)(uint16_t)(command[0x0c] | (uint16_t)command[0x0d] << 8);
    if (!ops->player_present(ops->context, command[2]) || index == -1 ||
        !(unit_template =
              ops->player_template(ops->context, command[2], index)))
        return 0;
    x = aoe_le_f32(command + 4);
    y = aoe_le_f32(command + 8);
    out = 0; /* see the header: never a truncated host pointer */
    code = ops->placement_check(ops->context, command[2], unit_template, x, y,
                                &out);
    if (code) {
        report(command, ops, code, out, index);
        return 0;
    }
    if (index == AOE_TEMPLATE_TOWN_CENTER_ROR) {
        const DatUnit *center = ops->player_template(
            ops->context, command[2], AOE_TEMPLATE_TOWN_CENTER_ROR);
        AoeLiveUnit *existing;
        if (center && !center->enabled &&
            (existing = ops->find_object(ops->context, command[2],
                                         AOE_TEMPLATE_TOWN_CENTER_ROR)) &&
            existing->core.state <= 2)
            return 0;
    }
    if (!aoe_player_cost_pay_ror(ops->resources(ops->context, command[2]),
                                 unit_template, 1.0f, 0, &paid))
        return -1;
    if (!paid) {
        report(command, ops, 0, out, index);
        return 0;
    }
    foundation = ops->create(ops->context, command[2], unit_template, x, y,
                             aoe_le_u32(command + 0x10));
    for (unsigned i = 0; i < command[1]; ++i) {
        AoeLiveUnit *builder = lookup(ops, aoe_le_i32(command + 0x14 + 4u * i));
        if (!builder) continue;
        if (!aoe_live_unit_reset_group_ror(builder,
                                           ops->owner_kind(ops->context,
                                                           builder)))
            return -1;
        ops->order(ops->context, builder, foundation, x, y);
    }
    return 1;
}
