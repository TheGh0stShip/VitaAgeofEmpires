#include "command_player.h"

#include <string.h>

#include "aoe_ftol.h"
#include "le_bytes.h"

size_t aoe_command_encode_player_resource_add_ror(
    uint8_t *out, size_t out_size, uint8_t player_index, uint8_t resource,
    float amount) {
    if (!out || out_size < AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR) return 0;
    memset(out, 0, AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR);
    out[0] = AOE_COMMAND_PLAYER_RESOURCE_ADD_ROR;
    out[1] = player_index;
    out[2] = resource;
    aoe_le_put_f32(out + 4, amount);
    return AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR;
}

int aoe_command_execute_player_resource_add_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerOps *ops) {
    const AoePlayerResourceTargetRor *player;
    if (!command || size < AOE_COMMAND_PLAYER_RESOURCE_ADD_SIZE_ROR ||
        command[0] != AOE_COMMAND_PLAYER_RESOURCE_ADD_ROR || !ops ||
        !ops->player)
        return -1;
    player = ops->player(ops->context, command[1]);
    if (!player) return -1;
    /* 0x0042ad10 pushes movzx(byte +0x02), raw dword +0x04, and 0. */
    return aoe_player_resource_add_virtual_ror(
        player, (int16_t)command[2], aoe_le_f32(command + 4), 0);
}

size_t aoe_command_encode_player_tribute_ror(
    uint8_t *out, size_t out_size, uint8_t sender, uint8_t receiver,
    uint8_t resource, float amount) {
    if (!out || out_size < AOE_COMMAND_PLAYER_TRIBUTE_SIZE_ROR) return 0;
    memset(out, 0, AOE_COMMAND_PLAYER_TRIBUTE_SIZE_ROR);
    out[0] = AOE_COMMAND_PLAYER_TRIBUTE_ROR;
    out[1] = sender;
    out[2] = receiver;
    out[3] = resource;
    aoe_le_put_f32(out + 4, amount);
    return AOE_COMMAND_PLAYER_TRIBUTE_SIZE_ROR;
}

/* Host guard: the only add path that can fail with flag 0 is a derived
   player's resource-0x32 refresh without its hooks. Check both players
   before mutating either. */
static int add_ready(const AoePlayerResourceTargetRor *player,
                     uint8_t resource) {
    return !player->derived || resource != 0x32 ||
           (player->field_104 && player->refresh_world);
}

int aoe_command_execute_player_tribute_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerOps *ops) {
    const AoePlayerResourceTargetRor *sender, *receiver;
    const AoePlayerResources *stock;
    uint8_t resource;
    float amount;
    int32_t whole;
    if (!command || size < AOE_COMMAND_PLAYER_TRIBUTE_SIZE_ROR ||
        command[0] != AOE_COMMAND_PLAYER_TRIBUTE_ROR || !ops ||
        !ops->player || !ops->game_event || !ops->receiver_notice)
        return -1;
    sender = ops->player(ops->context, command[1]);
    receiver = ops->player(ops->context, command[2]);
    resource = command[3];
    if (!sender || !receiver || !(stock = sender->resources) ||
        !stock->values || stock->count <= 0 || resource >= stock->count ||
        !receiver->resources || !add_ready(sender, resource) ||
        !add_ready(receiver, resource))
        return -1;
    amount = aoe_le_f32(command + 4);
    /* fcomp then `test ah, 1`: C0 is set for less-than or unordered. */
    if (!(stock->values[resource] >= amount)) return 0;
    if (!aoe_player_resource_add_virtual_ror(sender, resource, -amount, 0) ||
        !aoe_player_resource_add_virtual_ror(receiver, resource, amount, 0))
        return -1;
    whole = aoe_ftol32(amount);
    ops->game_event(ops->context, 7, command[1], command[2], resource, whole);
    ops->receiver_notice(ops->context, command[2], command[1], command[2],
                         AOE_COMMAND_PLAYER_TRIBUTE_NOTICE_ROR, resource,
                         whole, 0);
    return 1;
}
