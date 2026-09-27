#ifndef VITA_AOE_PLAYER_RESOURCES_H
#define VITA_AOE_PLAYER_RESOURCES_H

#include <stdint.h>

/* Player resource storage is caller-owned. These operations translate
   expansion 0x0045def0/0x0045df10 and base 0x00517880/0x005178a0.
   The original uses a signed 16-bit count and index. */
typedef struct {
    int16_t count;
    float *values;
} AoePlayerResources;

typedef enum {
    AOE_PLAYER_BASE = 0,
    AOE_PLAYER_RISE_OF_ROME = 1
} AoePlayerEdition;

void aoe_player_resource_set(AoePlayerResources *resources, int16_t index,
                             float amount);
void aoe_player_resource_add(AoePlayerResources *resources, int16_t index,
                             float amount, AoePlayerEdition edition);

#endif
