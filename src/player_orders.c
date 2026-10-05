#include "player_orders.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "aoe_ftol.h"
#include "command_dependency.h"
#include "command_object.h"
#include "command_player.h"

#define MINUS_ONE_BITS 0xbf800000u
#define ONE_BITS 0x3f800000u

typedef struct {
    AoeSelectableUnitRor *units[AOE_PLAYER_SELECTION_SLOTS];
    AoeCommandObjectRor objects[AOE_PLAYER_SELECTION_SLOTS];
    const AoeCommandObjectRor *list[AOE_PLAYER_SELECTION_SLOTS];
    int16_t count;
} Ordered;

static uint32_t float_bits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static uint8_t player_byte(const AoePlayerOrdersRor *player) {
    return (uint8_t)player->selection->player_id;
}

static int ready(const AoePlayerOrdersRor *player,
                 const AoePlayerOrdersOps *ops) {
    return player && player->selection && ops && ops->send;
}

static int send(const AoePlayerOrdersOps *ops, const uint8_t *record,
                size_t size) {
    if (!size) return 0;
    ops->send(ops->context, record, size);
    return 1;
}

/* The selection query with default filters except the level. */
static int collect(const AoePlayerOrdersRor *player, int16_t min_8d,
                   Ordered *ordered) {
    int16_t i;
    ordered->count = aoe_player_selection_collect_ror(
        player->selection, ordered->units, min_8d, -1, -1, -1);
    for (i = 0; i < ordered->count; ++i) {
        ordered->objects[i].id = ordered->units[i]->id;
        ordered->objects[i].template_65 = 0;
        ordered->list[i] = &ordered->objects[i];
    }
    return ordered->count != 0;
}

static void acknowledge(const AoePlayerOrdersOps *ops,
                        AoeSelectableUnitRor *unit) {
    if (ops->acknowledge) ops->acknowledge(ops->context, unit);
}

int aoe_player_send_unit_task_ror(uint8_t player,
                                  const AoePlayerUnitTaskRor *task,
                                  const AoePlayerOrdersOps *ops) {
    AoePlayerUnitTaskRor copy;
    uint8_t record[AOE_COMMAND_UNIT_TASK_SIZE_ROR];
    if (!task || !ops || !ops->send) return 0;
    copy = *task;
    copy.mode = player;
    return send(ops, record,
                aoe_command_encode_unit_task_ror(record, sizeof(record),
                                                 player, &copy));
}

int aoe_player_order_unit_ror(const AoeCommandObjectRor *unit,
                              const AoeCommandObjectRor *target, float x,
                              float y, const AoePlayerOrdersOps *ops) {
    const AoeCommandObjectRor *units[1];
    uint8_t record[0x20];
    if (!ops || !ops->send) return 0;
    units[0] = unit;
    return send(ops, record,
                aoe_command_build_order_ror(record, sizeof(record), units, 1,
                                            target, x, y, ops->session));
}

int aoe_player_send_unit_records_ror(const AoePlayerOrdersRor *player,
                                     int32_t unit_id, const uint8_t *records,
                                     int32_t count,
                                     const AoePlayerOrdersOps *ops) {
    uint8_t record[0x18];
    if (!ready(player, ops)) return 0;
    return send(ops, record,
                aoe_command_build_unit_records_ror(record, sizeof(record),
                                                   player_byte(player),
                                                   unit_id, records, count));
}

static AoePlayerUnitTaskRor assign_task(int32_t unit_id, int16_t task_id,
                                        int32_t value, uint8_t flag) {
    AoePlayerUnitTaskRor task;
    memset(&task, 0, sizeof(task));
    task.unit_id = unit_id;
    task.value_16 = task_id;
    task.value_32 = (uint32_t)value;
    task.flag = flag;
    task.x_bits = task.y_bits = task.z_bits = MINUS_ONE_BITS;
    task.extra_32 = MINUS_ONE_BITS;
    task.extra_a = 1;
    task.extra_b = 0;
    task.extra_c = 100;
    return task;
}

static int assign(const AoePlayerOrdersRor *player, int32_t unit_id,
                  const int32_t *ids, int32_t count, int check_count,
                  int32_t value, int32_t entry,
                  const AoePlayerOrdersOps *ops) {
    AoePlayerUnitTaskRor task;
    aoe_player_dependency_clear_ror(player, unit_id, ops);
    if (!ops->has_world_entry ||
        !ops->has_world_entry(ops->context, entry)) {
        task = assign_task(unit_id, 0x2d7, -1, 0xff);
        return aoe_player_send_unit_task_ror(player_byte(player), &task,
                                             ops);
    }
    if (check_count && count == 0) return 1;
    aoe_player_dependency_set_ror(player, unit_id, ids, count, 0x40000000u,
                                  ops);
    task = assign_task(unit_id, 0x2d6, value, (uint8_t)entry);
    return aoe_player_send_unit_task_ror(player_byte(player), &task, ops);
}

int aoe_player_assign_selection_ror(const AoePlayerOrdersRor *player,
                                    int32_t unit_id, int32_t value,
                                    int32_t entry,
                                    const AoePlayerOrdersOps *ops) {
    int32_t ids[AOE_PLAYER_SELECTION_SLOTS];
    int32_t count = 0;
    int slot;
    if (!ready(player, ops)) return 0;
    for (slot = 0; slot < AOE_PLAYER_SELECTION_SLOTS; ++slot)
        if (player->selection->slots[slot])
            ids[count++] = player->selection->slots[slot]->id;
    return assign(player, unit_id, ids, count, 1, value, entry, ops);
}

int aoe_player_assign_units_ror(const AoePlayerOrdersRor *player,
                                int32_t unit_id, const int32_t *ids,
                                int32_t count, int32_t value, int32_t entry,
                                const AoePlayerOrdersOps *ops) {
    if (!ready(player, ops)) return 0;
    return assign(player, unit_id, ids, count, 1, value, entry, ops);
}

int aoe_player_query_none_ror(int32_t *a, int32_t *b) {
    if (a) *a = 0;
    if (b) *b = 0;
    return 0;
}

int aoe_player_order_selection_ror(const AoePlayerOrdersRor *player,
                                   const AoePlayerOrderTargetRor *target,
                                   float x, float y,
                                   const AoePlayerOrdersOps *ops) {
    Ordered o;
    uint8_t record[0x1c + 4 * AOE_PLAYER_SELECTION_SLOTS];
    if (!ready(player, ops) || !collect(player, -1, &o)) return 0;
    if (ops->acknowledge_move) ops->acknowledge_move(ops->context, o.units[0]);
    send(ops, record,
         aoe_command_build_order_target_ror(record, sizeof(record), o.list,
                                            o.count,
                                            target ? &target->object : NULL,
                                            x, y));
    return 1;
}

int aoe_player_order_selection_follow_ror(
    const AoePlayerOrdersRor *player, const AoePlayerOrderTargetRor *target,
    float x, float y, const AoePlayerOrdersOps *ops) {
    Ordered o;
    uint8_t record[0x1c + 4 * AOE_PLAYER_SELECTION_SLOTS];
    int16_t i;
    if (!ready(player, ops) || !collect(player, -1, &o)) return 0;
    if (ops->acknowledge_target)
        ops->acknowledge_target(ops->context, o.units[0], target, x, y);
    send(ops, record,
         aoe_command_build_order_alternate_ror(
             record, sizeof(record), o.list, o.count,
             target ? &target->object : NULL, x, y));
    if (target) {
        for (i = 0; i < o.count; ++i) {
            AoePlayerUnitTaskRor task;
            memset(&task, 0, sizeof(task));
            task.unit_id = o.units[i]->id;
            task.value_16 = 0x2bd;
            task.value_32 = (uint32_t)target->object.id;
            task.flag = (uint8_t)target->owner_id;
            task.x_bits = float_bits(target->x);
            task.y_bits = float_bits(target->y);
            task.z_bits = ONE_BITS;
            task.extra_32 = float_bits((float)(target->template_28 * 0.5));
            task.extra_a = 0;
            task.extra_b = 0;
            task.extra_c = 100;
            aoe_player_send_unit_task_ror(player_byte(player), &task, ops);
        }
    }
    return 1;
}

int aoe_player_order_selection_primary_ror(
    const AoePlayerOrdersRor *player, const AoePlayerOrderTargetRor *target,
    float x, float y, const AoePlayerOrdersOps *ops) {
    Ordered o;
    uint8_t record[0x1c + 4 * AOE_PLAYER_SELECTION_SLOTS];
    if (!ready(player, ops) || !collect(player, -1, &o)) return 0;
    if (ops->acknowledge_target)
        ops->acknowledge_target(ops->context, o.units[0], target, x, y);
    send(ops, record,
         aoe_command_build_order_ror(record, sizeof(record), o.list, o.count,
                                     target ? &target->object : NULL, x, y,
                                     ops->session));
    return 1;
}

int aoe_player_stop_selection_ror(const AoePlayerOrdersRor *player,
                                  const AoePlayerOrdersOps *ops) {
    Ordered o;
    uint8_t record[2 + 4 * AOE_PLAYER_SELECTION_SLOTS];
    if (!ready(player, ops) || !collect(player, 3, &o)) return 0;
    acknowledge(ops, o.units[0]);
    send(ops, record,
         aoe_command_build_stop_ror(record, sizeof(record), o.list, o.count));
    return 1;
}

int aoe_player_create_object_ror(const AoePlayerOrdersRor *player,
                                 int16_t index, float x, float y, float z,
                                 const AoePlayerOrdersOps *ops) {
    uint8_t record[AOE_COMMAND_OBJECT_CREATE_SIZE_ROR];
    if (!ready(player, ops) || index == -1 || !ops->has_template ||
        !ops->has_template(ops->context, index))
        return 0;
    send(ops, record,
         aoe_command_encode_object_create_ror(record, sizeof(record), index,
                                              player_byte(player), x, y, z));
    return 1;
}

int aoe_player_tribute_ror(const AoePlayerOrdersRor *player,
                           uint8_t receiver, int32_t resource, float amount,
                           const AoePlayerOrdersOps *ops) {
    uint8_t record[AOE_COMMAND_PLAYER_TRIBUTE_SIZE_ROR];
    float stock;
    if (!ready(player, ops) || resource == -1) return 0;
    /* fcomp/test ah,0x40: equal to 0.0 or unordered fails. */
    if (amount == 0.0f || amount != amount) return 0;
    if (!player->resources || resource < 0 ||
        resource >= player->resource_count)
        return 0;
    stock = player->resources[resource];
    /* fcomp/test ah,0x41: below, equal or unordered passes. */
    if (amount > stock) return 0;
    send(ops, record,
         aoe_command_encode_player_tribute_ror(record, sizeof(record),
                                               player_byte(player), receiver,
                                               (uint8_t)resource, amount));
    return 1;
}

int aoe_player_arrange_selection_ror(const AoePlayerOrdersRor *player,
                                     int32_t pattern,
                                     const AoePlayerOrdersOps *ops) {
    Ordered o;
    uint8_t record[0xc + 4 * AOE_PLAYER_SELECTION_SLOTS];
    if (!ready(player, ops) || pattern < 0 || pattern > 5) return 0;
    if (!collect(player, -1, &o)) return 0;
    acknowledge(ops, o.units[0]);
    send(ops, record,
         aoe_command_build_arrange_ror(record, sizeof(record), o.list,
                                       (uint32_t)(int32_t)o.count, pattern));
    return 1;
}

int aoe_player_task_selection_ror(const AoePlayerOrdersRor *player,
                                  const AoePlayerOrdersOps *ops) {
    Ordered o;
    AoeCommandTaskFieldsRor f;
    uint8_t record[0x24 + 4 * AOE_PLAYER_SELECTION_SLOTS];
    if (!ready(player, ops) || !collect(player, -1, &o)) return 0;
    acknowledge(ops, o.units[0]);
    memset(&f, 0, sizeof(f));
    f.player = player_byte(player);
    f.mode = player_byte(player);
    f.value_16 = 0x2d5;
    f.value_32 = -1;
    f.flag = 0xff;
    f.x_bits = f.y_bits = f.z_bits = MINUS_ONE_BITS;
    f.extra_32 = ONE_BITS;
    f.extra_a = 1;
    f.extra_b = 0;
    f.extra_c = 100;
    send(ops, record,
         aoe_command_build_task_list_ror(record, sizeof(record), o.list,
                                         o.count, &f));
    return 1;
}

int aoe_player_dependency_set_ror(const AoePlayerOrdersRor *player,
                                  int32_t unit_id, const int32_t *ids,
                                  int32_t count, uint32_t context_bits,
                                  const AoePlayerOrdersOps *ops) {
    uint8_t record[AOE_COMMAND_DEPENDENCY_SET_SIZE_ROR];
    if (!ready(player, ops)) return 0;
    return send(ops, record,
                aoe_command_encode_dependency_set_ror(
                    record, sizeof(record), player_byte(player), unit_id, ids,
                    count, context_bits));
}

int aoe_player_dependency_propagate_ror(const AoePlayerOrdersRor *player,
                                        int32_t unit_id, int32_t target_id,
                                        uint32_t context_bits,
                                        const AoePlayerOrdersOps *ops) {
    uint8_t record[AOE_COMMAND_DEPENDENCY_PROPAGATE_SIZE_ROR];
    if (!ready(player, ops)) return 0;
    return send(ops, record,
                aoe_command_encode_dependency_propagate_ror(
                    record, sizeof(record), player_byte(player), unit_id,
                    target_id, context_bits));
}

int aoe_player_dependency_remove_ror(const AoePlayerOrdersRor *player,
                                     int32_t unit_id, int32_t removed_id,
                                     const AoePlayerOrdersOps *ops) {
    uint8_t record[AOE_COMMAND_DEPENDENCY_REMOVE_SIZE_ROR];
    if (!ready(player, ops)) return 0;
    return send(ops, record,
                aoe_command_encode_dependency_remove_ror(
                    record, sizeof(record), player_byte(player), unit_id,
                    removed_id));
}

int aoe_player_dependency_clear_ror(const AoePlayerOrdersRor *player,
                                    int32_t unit_id,
                                    const AoePlayerOrdersOps *ops) {
    uint8_t record[AOE_COMMAND_DEPENDENCY_CLEAR_SIZE_ROR];
    if (!ready(player, ops)) return 0;
    return send(ops, record,
                aoe_command_encode_dependency_clear_ror(
                    record, sizeof(record), player_byte(player), unit_id));
}

int aoe_player_send_resign_ror(const AoePlayerOrdersRor *player,
                               uint8_t slot, uint8_t value,
                               const AoePlayerOrdersOps *ops) {
    uint8_t record[4];
    if (!ready(player, ops) || player->game_state != 0) return 0;
    send(ops, record,
         aoe_command_build_resign_ror(record, sizeof(record),
                                      player_byte(player), slot, value));
    return 1;
}

int aoe_player_record_selection_ror(const AoePlayerOrdersRor *player,
                                    float a, float b,
                                    const AoePlayerOrdersOps *ops) {
    Ordered o;
    uint8_t record[4 + 4 * AOE_PLAYER_SELECTION_SLOTS];
    int32_t second, first;
    if (!ready(player, ops) || !collect(player, -1, &o)) return 0;
    acknowledge(ops, o.units[0]);
    second = aoe_ftol32(b);
    first = aoe_ftol32(a);
    send(ops, record,
         aoe_command_build_record_list_ror(record, sizeof(record), o.list,
                                           o.count, (uint8_t)first,
                                           (uint8_t)second));
    return 1;
}

static void chat_to(const AoePlayerChatOps *ops, int32_t from, int32_t owner,
                    const char *text) {
    int32_t slot;
    for (slot = 1; ops->slot_count && slot <= ops->slot_count(ops->context);
         ++slot)
        if (ops->slot_player && ops->slot_player(ops->context, slot) == owner &&
            ops->send)
            ops->send(ops->context, from, slot, text);
}

void aoe_player_send_chat_ror(const AoePlayerOrdersRor *player, int32_t to,
                              int32_t echo, const AoePlayerChatOps *ops,
                              const char *format, ...) {
    char text[1024];
    va_list args;
    int32_t self;

    if (!player || !player->selection || !ops || !format) return;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (text[0] == '\0') return;
    self = player->selection->player_id;
    chat_to(ops, self, to, text);
    if (echo == 1 && to != self) chat_to(ops, self, self, text);
}
