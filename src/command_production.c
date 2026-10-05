#include "command_production.h"

#include <string.h>

#include "le_bytes.h"

size_t aoe_command_encode_production_ror(uint8_t *out, size_t out_size,
                                         uint8_t opcode, int32_t building_id,
                                         uint8_t player, int16_t item,
                                         int32_t request) {
    if (!out || out_size < AOE_COMMAND_PRODUCTION_SIZE_ROR ||
        (opcode != AOE_COMMAND_TRAIN_ROR && opcode != AOE_COMMAND_RESEARCH_ROR))
        return 0;
    memset(out, 0, AOE_COMMAND_PRODUCTION_SIZE_ROR);
    out[0] = opcode;
    aoe_le_put_u32(out + 4, (uint32_t)building_id);
    out[8] = player;
    out[10] = (uint8_t)(uint16_t)item;
    out[11] = (uint8_t)((uint16_t)item >> 8);
    aoe_le_put_u32(out + 12, (uint32_t)request);
    return AOE_COMMAND_PRODUCTION_SIZE_ROR;
}

int aoe_command_execute_production_ror(const uint8_t *command, size_t size,
                                       const AoeCommandProductionOps *ops) {
    int train;
    AoeLiveUnit *building;
    int16_t item;
    int32_t building_id, request;
    if (!command || size < AOE_COMMAND_PRODUCTION_SIZE_ROR || !ops ||
        !ops->world.resolve || !ops->active_action_type ||
        !ops->has_group_object || !ops->group_production)
        return -1;
    train = command[0] == AOE_COMMAND_TRAIN_ROR;
    if (!train && command[0] != AOE_COMMAND_RESEARCH_ROR) return -1;
    if (train ? !ops->owner_unit_template || !ops->start_train ||
                    !ops->player_train
              : !ops->research_location || !ops->start_research ||
                    !ops->player_research)
        return -1;
    building_id = aoe_le_i32(command + 4);
    item = (int16_t)(uint16_t)(command[10] | (uint16_t)command[11] << 8);
    request = aoe_le_i32(command + 12);
    building = ops->world.resolve(ops->world.context, building_id);
    if (building) {
        int16_t busy = ops->active_action_type(ops->context, building);
        if (busy != 0x66 && busy != 0x67) {
            int16_t own_id;
            if (!building->core.unit_template) return -1;
            own_id = building->core.unit_template->id;
            if (train) {
                const DatUnit *t =
                    ops->owner_unit_template(ops->context, building, item);
                if (!t) return -1;
                if (t->train_location_id != own_id &&
                    t->train_location_id != -1)
                    return 0;
                ops->start_train(ops->context, building, item, request);
            } else {
                if (ops->research_location(ops->context, building, item) !=
                    own_id)
                    return 0;
                ops->start_research(ops->context, building, item, request);
            }
            if (ops->has_group_object(ops->context, building))
                ops->group_production(ops->context, building, request);
            return 1;
        }
    }
    if (train)
        ops->player_train(ops->context, command[8], item, building_id,
                          request);
    else
        ops->player_research(ops->context, command[8], item, building_id,
                             request);
    return 1;
}
