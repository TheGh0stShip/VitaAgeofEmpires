#ifndef VITA_AOE_PLAYER_RESOURCES_H
#define VITA_AOE_PLAYER_RESOURCES_H

#include <stdint.h>

/* Player resource storage is caller-owned (player +0x4c count, +0x50
   floats). These operations translate player vtable +0x74/+0x78 targets:
   patched RoR 1.0a 0x0045cfc0/0x0045cfe0 and patched AoE 1.0c
   0x0045e7a0/0x0045e7c0 (unpatched-release notes used disc RoR
   0x0045def0/0x0045df10 and base 0x00517880/0x005178a0). The original uses a
   signed 16-bit count and index. Its single x87 add is rounded straight back
   to a 32-bit float, which equals an IEEE single-precision add. */
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

/* Patched RoR 1.0a player vtable +0x78 dispatch. Base vtable 0x00544d18
   uses 0x0045cfe0 directly; derived vtables 0x00549a44 and 0x00549b80 both
   override it with 0x004f6c60, which first:
   - for resource 3 with a nonzero flag byte, adds the amount to raw value
     slot 49 (+0xc4) without the +0x4c range check;
   - for resource 0x32, stores 1 at player +0x104 and calls world
     0x00520c60 through player +0x3c;
   and then always calls 0x0045cfe0. */
typedef struct {
    AoePlayerResources *resources;
    int derived;               /* nonzero for vtables 0x549a44/0x549b80 */
    int32_t *field_104;        /* derived player +0x104 */
    void *context;
    void (*refresh_world)(void *context); /* world 0x00520c60 */
} AoePlayerResourceTargetRor;

/* Returns 0 without changes for missing dependencies, or when the unchecked
   slot-49 write would fall outside the host allocation (a host-only guard).
   Otherwise returns 1, including for an out-of-range index ignored by
   0x0045cfe0 after the derived side effects. */
int aoe_player_resource_add_virtual_ror(
    const AoePlayerResourceTargetRor *player, int16_t index, float amount,
    uint8_t flag);

#endif
