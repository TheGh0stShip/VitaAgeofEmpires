#include "command_game.h"

#include <string.h>

#include "aoe_ftol.h"
#include "le_bytes.h"

static int16_t le_i16(const uint8_t *in) {
    return (int16_t)(uint16_t)(in[0] | (uint16_t)in[1] << 8);
}

size_t aoe_command_encode_game_ror(uint8_t *out, size_t out_size,
                                   uint8_t code, int16_t player,
                                   int16_t value_4, uint32_t value_8,
                                   int16_t value_c) {
    if (!out || out_size < AOE_COMMAND_GAME_SIZE_ROR) return 0;
    memset(out, 0, AOE_COMMAND_GAME_SIZE_ROR);
    out[0] = AOE_COMMAND_GAME_ROR;
    out[1] = code;
    out[2] = (uint8_t)(uint16_t)player;
    out[3] = (uint8_t)((uint16_t)player >> 8);
    out[4] = (uint8_t)(uint16_t)value_4;
    out[5] = (uint8_t)((uint16_t)value_4 >> 8);
    aoe_le_put_u32(out + 8, value_8);
    out[12] = (uint8_t)(uint16_t)value_c;
    out[13] = (uint8_t)((uint16_t)value_c >> 8);
    return AOE_COMMAND_GAME_SIZE_ROR;
}

int aoe_command_execute_game_ror(const uint8_t *command, size_t size,
                                 const AoeCommandGameOps *ops) {
    int16_t player;
    if (!command || size < AOE_COMMAND_GAME_SIZE_ROR ||
        command[0] != AOE_COMMAND_GAME_ROR || !ops)
        return -1;
    player = le_i16(command + 2);
    switch (command[1]) {
    case 0: {
        AoePlayerDiplomacyRor *d;
        if (!ops->diplomacy || !ops->game_event || !ops->refresh_world ||
            !(d = ops->diplomacy(ops->context, player)))
            return -1;
        d->changed = 1;
        if (!aoe_player_set_stance_ror(
                d, le_i16(command + 4),
                (uint8_t)aoe_ftol32(aoe_le_f32(command + 8)),
                &ops->diplomacy_ops))
            return -1;
        ops->game_event(ops->context, 8, player, le_i16(command + 4),
                        le_i16(command + 12));
        ops->refresh_world(ops->context);
        return 1;
    }
    case 1:
        if (!ops->world_18) return -1;
        *ops->world_18 = aoe_le_i32(command + 8);
        return 1;
    case 2: {
        const AoePlayerResourceTargetRor *r;
        if (!ops->resources || !(r = ops->resources(ops->context, player)))
            return -1;
        return aoe_player_resource_add_virtual_ror(
                   r, le_i16(command + 4), aoe_le_f32(command + 8), 0)
                   ? 1
                   : -1;
    }
    case 4:
        if (!ops->game_9a8) return -1;
        *ops->game_9a8 = player;
        return 1;
    case 5: {
        AoePlayerDiplomacyRor *d;
        if (!ops->diplomacy || !(d = ops->diplomacy(ops->context, player)))
            return -1;
        d->flag_108 = command[4];
        return 1;
    }
    case 6:
        if (!ops->cheat) return -1;
        ops->cheat(ops->context, player, command[4]);
        return 1;
    case 7:
        if (!ops->world_4c_notify) return -1;
        ops->world_4c_notify(ops->context, player);
        return 1;
    case 8:
        if (!ops->sync_error) return -1;
        ops->sync_error(ops->context);
        return 1;
    default:
        return 1;
    }
}
