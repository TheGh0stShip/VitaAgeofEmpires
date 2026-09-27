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
