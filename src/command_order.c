#include "command_order.h"

#include <string.h>

#include "aoe_ftol.h"
#include "le_bytes.h"

size_t aoe_command_encode_order_ror(
    uint8_t *out, size_t out_size, uint8_t player, int32_t target_id,
    float x, float y, uint8_t flag, uint32_t mask_14, uint32_t mask_18,
    const int32_t *unit_ids, uint8_t count) {
    size_t size = AOE_COMMAND_ORDER_HEADER_SIZE_ROR + 4u * count;
    if (!out || out_size < size || (count && !unit_ids)) return 0;
    memset(out, 0, size);
    out[0] = AOE_COMMAND_ORDER_ROR;
    out[1] = count;
    aoe_le_put_u32(out + 0x04, (uint32_t)target_id);
    aoe_le_put_f32(out + 0x08, x);
    aoe_le_put_f32(out + 0x0c, y);
    out[0x10] = player;
    out[0x12] = flag;
    aoe_le_put_u32(out + 0x14, mask_14);
    aoe_le_put_u32(out + 0x18, mask_18);
    for (uint8_t i = 0; i < count; ++i)
        aoe_le_put_u32(out + 0x1c + 4u * i, (uint32_t)unit_ids[i]);
    return size;
}

static AoeLiveUnit *lookup(const AoeCommandOrderOps *ops, int32_t id) {
    return id == -1 ? NULL : ops->world.resolve(ops->world.context, id);
}

static int32_t unit_id_at(const uint8_t *command, unsigned index) {
    return aoe_le_i32(command + 0x1c + 4u * index);
}

static int drop(const AoeCommandOrderOps *ops) {
    *ops->rand_result = aoe_rng_next(ops->rng);
    return 0;
}

/* 1 = continue, 0 = dropped, -1 = host error. */
static int gate(const uint8_t *command, unsigned count, AoeLiveUnit *target,
                const AoeCommandOrderOps *ops) {
    AoeLiveUnit *first;
    uint8_t type;
    uint32_t mask_18, cell;
    if (!target || !ops->session_check(ops->context) ||
        aoe_le_i32(command + 4) == -1 || !count ||
        !(first = lookup(ops, unit_id_at(command, 0))) ||
        ops->owner_kind(ops->context, first) != 0 ||
        ops->local_player == command[0x10])
        return 1;
    if (!target->core.unit_template) return -1;
    type = target->core.unit_template->type;
    if (type != 80 && type != 90 && type != 10 && type != 20) return 1;
    if (ops->player_104(ops->context, command[0x10]) == 0 && command[0x12])
        return drop(ops);
    mask_18 = aoe_le_u32(command + 0x18);
    if (!(mask_18 & aoe_le_u32(command + 0x14))) return drop(ops);
    if (!ops->map_cell(ops->context, aoe_ftol32(aoe_le_f32(command + 0x0c)),
                       aoe_ftol32(aoe_le_f32(command + 0x08)), &cell))
        return -1;
    if (~cell & mask_18 & UINT32_C(0xffff0000)) return drop(ops);
    return 1;
}

static int prepare(AoeLiveUnit *unit, const uint8_t *command, unsigned count,
                   unsigned self, const AoeCommandOrderOps *ops) {
    if (!aoe_live_unit_reset_group_ror(unit, ops->owner_kind(ops->context,
                                                              unit)))
        return 0;
    ops->set_order_flag(ops->context, unit, 1);
    for (unsigned j = 0; j < count; ++j)
        if (j != self &&
            !aoe_live_unit_add_group_member_ror(unit, unit_id_at(command, j)))
            return 0;
    return 1;
}

static float clamp_axis(const AoeCommandOrderOps *ops, const AoeLiveUnit *unit,
                        float value, int axis) {
    if (!(value >= 0.0)) return 1.0f;
    {
        int32_t size = ops->map_size(ops->context, unit, axis);
        if (value >= (double)size) return (float)((double)size - 1.0f);
    }
    return value;
}

static AoeLiveUnit *sized_target(AoeLiveUnit *target) {
    return target && target->core.unit_template &&
                   (target->core.unit_template->size[0] > 0.0f ||
                    target->core.unit_template->size[1] > 0.0f)
               ? target
               : NULL;
}

typedef enum { ORDER_PRIMARY, ORDER_FORMATION } OrderKind;

/* Opcode 3 only: the +0x74 group object hears about the order. */
static void notify_group(const AoeCommandOrderOps *ops, AoeLiveUnit *unit,
                         AoeLiveUnit *target, float x, float y) {
    AoeLiveUnit *sized;
    if (!ops->has_group_object(ops->context, unit)) return;
    sized = sized_target(target);
    ops->group_order(ops->context, unit,
                     sized ? sized->core.unit_id : -1,
                     sized ? sized->core.unit_template->unit_class : -1, x, y);
}

static void issue(const AoeCommandOrderOps *ops, OrderKind kind,
                  AoeLiveUnit *unit, AoeLiveUnit *order_target,
                  AoeLiveUnit *target, float x, float y) {
    if (kind == ORDER_PRIMARY) {
        ops->order(ops->context, unit, order_target, x, y);
    } else {
        ops->order_formation(ops->context, unit, order_target, x, y);
        notify_group(ops, unit, target, x, y);
    }
}

/* Shared body of 0x0042a280 (opcode 0) and 0x0042a850 (opcode 3). */
static int run_orders(const uint8_t *command, unsigned count,
                      AoeLiveUnit *target, OrderKind kind,
                      const AoeCommandOrderOps *ops) {
    float min_x = 10000.0f, max_x = 0.0f, min_y = 10000.0f, max_y = 0.0f;
    float sum_x = 0.0f, sum_y = 0.0f, center_x, center_y;
    if (count > 1) {
        for (unsigned i = 0; i < count; ++i) {
            AoeLiveUnit *unit = lookup(ops, unit_id_at(command, i));
            float x, y;
            if (!unit) continue;
            x = unit->core.position.x38;
            y = unit->core.position.y3c;
            /* fcomp; test ah,1: less-than or unordered sets the minimum,
               otherwise only a strictly greater value sets the maximum. */
            if (!(x >= min_x)) min_x = x;
            else if (x > max_x) max_x = x;
            if (!(y >= min_y)) min_y = y;
            else if (y > max_y) max_y = y;
            sum_x += x;
            sum_y += y;
        }
        if (!((double)max_x - min_x > 8.0) &&
            !((double)max_y - min_y > 8.0)) {
            center_x = (float)((double)sum_x / (double)count);
            center_y = (float)((double)sum_y / (double)count);
            for (unsigned i = 0; i < count; ++i) {
                AoeLiveUnit *unit = lookup(ops, unit_id_at(command, i));
                float x, y;
                if (!unit) continue;
                if (!prepare(unit, command, count, i, ops)) return -1;
                x = (float)((double)unit->core.position.x38 - center_x +
                            aoe_le_f32(command + 0x08));
                y = (float)((double)unit->core.position.y3c - center_y +
                            aoe_le_f32(command + 0x0c));
                x = clamp_axis(ops, unit, x, 0);
                y = clamp_axis(ops, unit, y, 1);
                issue(ops, kind, unit, sized_target(target), target, x, y);
            }
            return 1;
        }
    }
    for (unsigned i = 0; i < count; ++i) {
        AoeLiveUnit *unit = lookup(ops, unit_id_at(command, i));
        if (!unit) continue;
        if (!prepare(unit, command, count, i, ops)) return -1;
        issue(ops, kind, unit, target, target, aoe_le_f32(command + 0x08),
              aoe_le_f32(command + 0x0c));
    }
    return 1;
}

static int order_ops_valid(const AoeCommandOrderOps *ops) {
    return ops && ops->world.resolve && ops->owner_kind &&
           ops->set_order_flag && ops->map_size;
}

int aoe_command_execute_order_ror(const uint8_t *command, size_t size,
                                  const AoeCommandOrderOps *ops) {
    unsigned count;
    AoeLiveUnit *target;
    int result;
    if (!command || size < AOE_COMMAND_ORDER_HEADER_SIZE_ROR ||
        command[0] != AOE_COMMAND_ORDER_ROR || !order_ops_valid(ops) ||
        !ops->session_check || !ops->player_104 || !ops->map_cell ||
        !ops->rng || !ops->rand_result || !ops->order)
        return -1;
    count = command[1];
    if (size < AOE_COMMAND_ORDER_HEADER_SIZE_ROR + 4u * count) return -1;
    target = lookup(ops, aoe_le_i32(command + 4));
    if ((result = gate(command, count, target, ops)) != 1) return result;
    return run_orders(command, count, target, ORDER_PRIMARY, ops);
}

int aoe_command_execute_order_formation_ror(const uint8_t *command,
                                            size_t size,
                                            const AoeCommandOrderOps *ops) {
    unsigned count;
    if (!command || size < AOE_COMMAND_ORDER_HEADER_SIZE_ROR ||
        command[0] != AOE_COMMAND_ORDER_FORMATION_ROR ||
        !order_ops_valid(ops) || !ops->order_formation ||
        !ops->has_group_object || !ops->group_order)
        return -1;
    count = command[1];
    if (size < AOE_COMMAND_ORDER_HEADER_SIZE_ROR + 4u * count) return -1;
    return run_orders(command, count, lookup(ops, aoe_le_i32(command + 4)),
                      ORDER_FORMATION, ops);
}

size_t aoe_command_encode_stop_ror(uint8_t *out, size_t out_size,
                                   const int32_t *unit_ids, uint8_t count) {
    size_t size = 2u + 4u * count;
    if (!out || out_size < size || (count && !unit_ids)) return 0;
    memset(out, 0, size);
    out[0] = AOE_COMMAND_STOP_ROR;
    out[1] = count;
    for (uint8_t i = 0; i < count; ++i)
        aoe_le_put_u32(out + 2 + 4u * i, (uint32_t)unit_ids[i]);
    return size;
}

int aoe_command_execute_stop_ror(const uint8_t *command, size_t size,
                                 const AoeCommandOrderOps *ops) {
    if (!command || size < 2 || command[0] != AOE_COMMAND_STOP_ROR || !ops ||
        !ops->world.resolve || !ops->owner_kind || !ops->stop ||
        size < 2u + 4u * command[1])
        return -1;
    for (unsigned i = 0; i < command[1]; ++i) {
        AoeLiveUnit *unit = lookup(ops, aoe_le_i32(command + 2 + 4u * i));
        if (!unit) continue;
        if (!unit->core.unit_template) return -1;
        if (unit->core.unit_template->id == 50) continue;
        if (!aoe_live_unit_reset_group_ror(unit,
                                           ops->owner_kind(ops->context, unit)) ||
            !ops->stop(ops->context, unit))
            return -1;
    }
    return 1;
}

int aoe_command_execute_order_alternate_ror(const uint8_t *command,
                                            size_t size,
                                            const AoeCommandOrderOps *ops) {
    unsigned count;
    AoeLiveUnit *target;
    if (!command || size < AOE_COMMAND_ORDER_HEADER_SIZE_ROR ||
        command[0] != AOE_COMMAND_ORDER_ALTERNATE_ROR || !ops ||
        !ops->world.resolve || !ops->owner_kind || !ops->set_order_flag ||
        !ops->order_alternate)
        return -1;
    count = command[1];
    if (size < AOE_COMMAND_ORDER_HEADER_SIZE_ROR + 4u * count) return -1;
    target = lookup(ops, aoe_le_i32(command + 4));
    for (unsigned i = 0; i < count; ++i) {
        AoeLiveUnit *unit = lookup(ops, unit_id_at(command, i));
        if (!unit) continue;
        if (!prepare(unit, command, count, i, ops)) return -1;
        ops->order_alternate(ops->context, unit, target,
                             aoe_le_f32(command + 0x08),
                             aoe_le_f32(command + 0x0c));
    }
    return 1;
}
