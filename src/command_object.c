#include "command_object.h"

#include <string.h>

#include "le_bytes.h"

size_t aoe_command_encode_object_create_ror(
    uint8_t *out, size_t out_size, int16_t template_index,
    uint8_t player_index, float x, float y, float z) {
    if (!out || out_size < AOE_COMMAND_OBJECT_CREATE_SIZE_ROR) return 0;
    memset(out, 0, AOE_COMMAND_OBJECT_CREATE_SIZE_ROR);
    out[0] = AOE_COMMAND_OBJECT_CREATE_ROR;
    out[2] = (uint8_t)(uint16_t)template_index;
    out[3] = (uint8_t)((uint16_t)template_index >> 8);
    out[4] = player_index;
    aoe_le_put_f32(out + 0x08, x);
    aoe_le_put_f32(out + 0x0c, y);
    aoe_le_put_f32(out + 0x10, z);
    return AOE_COMMAND_OBJECT_CREATE_SIZE_ROR;
}

static const DatUnit *decode(const uint8_t *command, size_t size,
                             const AoeCommandObjectOps *ops) {
    if (!command || size < AOE_COMMAND_OBJECT_CREATE_SIZE_ROR ||
        command[0] != AOE_COMMAND_OBJECT_CREATE_ROR || !ops ||
        !ops->player_template || !ops->create)
        return NULL;
    /* movsx word +0x02 */
    return ops->player_template(
        ops->context, command[4],
        (int16_t)(uint16_t)(command[2] | (uint16_t)command[3] << 8));
}

int aoe_command_execute_object_create_ror(
    const uint8_t *command, size_t size, const AoeCommandObjectOps *ops) {
    const DatUnit *unit_template = decode(command, size, ops);
    if (!unit_template) return -1;
    (void)ops->create(ops->context, command[4], unit_template,
                      aoe_le_f32(command + 0x08), aoe_le_f32(command + 0x0c),
                      aoe_le_f32(command + 0x10));
    return 1;
}

int aoe_command_execute_object_create_tribe_ror(
    const uint8_t *command, size_t size, const AoeCommandObjectOps *ops) {
    const DatUnit *unit_template = decode(command, size, ops);
    AoeLiveUnit *unit;
    float z;
    int completed;
    if (!unit_template) return -1;
    z = unit_template->elevation_mode == 1 ? 4.0f
                                           : aoe_le_f32(command + 0x10);
    unit = ops->create(ops->context, command[4], unit_template,
                       aoe_le_f32(command + 0x08), aoe_le_f32(command + 0x0c),
                       z);
    if (!unit || !unit->core.unit_template) return -1;
    if (unit->core.unit_template->type == 80 &&
        !aoe_live_unit_add_build_work_ror(unit, 10000.0f, &ops->state,
                                          &completed))
        return -1;
    return 1;
}
