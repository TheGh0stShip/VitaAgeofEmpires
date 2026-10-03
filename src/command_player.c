#include "command_player.h"

#include <string.h>

size_t aoe_command_encode_player_resource_add_ror(
    uint8_t *out, size_t out_size, uint8_t player_index, uint8_t resource,
    float amount) {
    uint32_t bits;
    if (!out || out_size < AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR) return 0;
    memcpy(&bits, &amount, sizeof(bits));
    memset(out, 0, AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR);
    out[0] = AOE_COMMAND_PLAYER_RESOURCE_ADD_ROR;
    out[1] = player_index;
    out[2] = resource;
    out[4] = (uint8_t)bits;
    out[5] = (uint8_t)(bits >> 8);
    out[6] = (uint8_t)(bits >> 16);
    out[7] = (uint8_t)(bits >> 24);
    return AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR;
}

int aoe_command_execute_player_resource_add_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerOps *ops) {
    const AoePlayerResourceTargetRor *player;
    uint32_t bits;
    float amount;
    if (!command || size < AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR ||
        command[0] != AOE_COMMAND_PLAYER_RESOURCE_ADD_ROR || !ops ||
        !ops->player)
        return -1;
    player = ops->player(ops->context, command[1]);
    if (!player) return -1;
    bits = (uint32_t)command[4] | (uint32_t)command[5] << 8 |
           (uint32_t)command[6] << 16 | (uint32_t)command[7] << 24;
    memcpy(&amount, &bits, sizeof(amount));
    /* 0x0042ad10 pushes movzx(byte +0x02), raw dword +0x04, and 0. */
    return aoe_player_resource_add_virtual_ror(
        player, (int16_t)command[2], amount, 0);
}
