#include "player_resources.h"

static int valid(const AoePlayerResources *resources, int16_t index) {
    return resources && resources->values && index >= 0 &&
           index < resources->count;
}

void aoe_player_resource_set(AoePlayerResources *resources, int16_t index,
                             float amount) {
    if (valid(resources, index)) resources->values[index] = amount;
}

void aoe_player_resource_add(AoePlayerResources *resources, int16_t index,
                             float amount, AoePlayerEdition edition) {
    if (!valid(resources, index)) return;
    if (edition == AOE_PLAYER_RISE_OF_ROME)
        resources->values[index] = amount + resources->values[index];
    else if (edition == AOE_PLAYER_BASE)
        resources->values[index] = resources->values[index] + amount;
}

int aoe_player_resource_add_virtual_ror(
    const AoePlayerResourceTargetRor *player, int16_t index, float amount,
    uint8_t flag) {
    if (!player || !player->resources) return 0;
    if (player->derived) {
        if (index == 3 && flag) {
            if (!player->resources->values || player->resources->count <= 49)
                return 0;
            player->resources->values[49] =
                amount + player->resources->values[49];
        } else if (index == 0x32) {
            if (!player->field_104 || !player->refresh_world) return 0;
            *player->field_104 = 1;
            player->refresh_world(player->context);
        }
    }
    aoe_player_resource_add(player->resources, index, amount,
                            AOE_PLAYER_RISE_OF_ROME);
    return 1;
}
