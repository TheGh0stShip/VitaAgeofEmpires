#include "player_resources.h"

#include <assert.h>

int main(void) {
    float values[4] = {10.0f, 20.0f, 30.0f, 40.0f};
    AoePlayerResources resources = {4, values};
    aoe_player_resource_set(&resources, 0, 50.0f);
    aoe_player_resource_add(&resources, 0, -7.5f, AOE_PLAYER_RISE_OF_ROME);
    aoe_player_resource_add(&resources, 3, 2.0f, AOE_PLAYER_BASE);
    assert(values[0] == 42.5f && values[3] == 42.0f);
    aoe_player_resource_set(&resources, -1, 99.0f);
    aoe_player_resource_add(&resources, 4, 99.0f, AOE_PLAYER_RISE_OF_ROME);
    assert(values[0] == 42.5f && values[1] == 20.0f);
    assert(values[2] == 30.0f && values[3] == 42.0f);
    resources.count = 0;
    aoe_player_resource_set(&resources, 0, 99.0f);
    assert(values[0] == 42.5f);
    resources.count = 4;
    resources.values = 0;
    aoe_player_resource_add(&resources, 0, 99.0f, AOE_PLAYER_BASE);
    aoe_player_resource_set(0, 0, 99.0f);
    assert(values[0] == 42.5f);
    return 0;
}
