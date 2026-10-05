#ifndef VITA_AOE_UNIT_GARRISON_H
#define VITA_AOE_UNIT_GARRISON_H

#include <stdint.h>

#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a units inside other units: a container's
   contents list (+0x24, aoe_live_unit.contents) and each passenger's
   container (+0x20, placement.attached_unit). */
typedef struct {
    void *context;
    /* Unit +0xdc(owner, 0, -1): the unit's vision for its owner off. */
    void (*vision_off)(void *context, AoeLiveUnit *unit);
    /* Remove from the map tile list (0x00450fe0 on tile +0x0c; tile and
       node cleared). */
    void (*leave_tile)(void *context, AoeLiveUnit *unit);
    /* 0x004ab7f0(2): lift the unit's obstruction footprint. */
    void (*lift)(void *context, AoeLiveUnit *unit);
    /* Type 50+ (0x00426b80): global 0x7d2058 0x00516a50 record of the
       entering unit (its class, position and +0x18c/+0x1b0 state). */
    void (*record)(void *context, AoeLiveUnit *unit);
    /* Unloading. */
    uint16_t (*rand)(void *context);                         /* 0x0052605d */
    int32_t (*map_width)(void *context);
    int32_t (*map_height)(void *context);
    /* 0x004ab9e0 of the transport. */
    int (*zone_at)(void *context, AoeLiveUnit *unit, int32_t x, int32_t y);
    /* Passenger template +0x20(owner, x, y, 0, 1, 1, 1, 0, 1, 1): 0 when
       the point is allowed. */
    int (*placement)(void *context, AoeLiveUnit *unit, float x, float y);
    /* Passenger +0x114(x, y, 0): 1 when it fits. */
    int (*fits)(void *context, AoeLiveUnit *unit, float x, float y);
    /* Passenger +0x34(x, y, z): place it on the map. */
    void (*place)(void *context, AoeLiveUnit *unit, float x, float y,
                  float z);
} AoeGarrisonOps;

/* 0x004aa920: fewer units inside than template +0x2c. */
int aoe_unit_has_room_ror(const AoeLiveUnit *container);

/* Unit +0xd0, root 0x004aa940 (types 50+ 0x00426b80 add `record`): with
   room, a unit already inside something calls that container's +0xd4; a
   unit on the map loses its vision and tile; the footprint is lifted; it
   joins the contents and takes the container's x/y. Returns 1 when it
   entered, 0 when there was no room, -1 for bad input. */
int aoe_unit_enter_ror(AoeLiveUnit *unit, AoeLiveUnit *container, int type50,
                       const AoeGarrisonOps *ops);
/* Unit +0xd4 = 0x004aa9c0: leave the container's contents. */
void aoe_unit_leave_ror(AoeLiveUnit *unit);

/* 0x004ab3f0: a free point for `passenger` around the rectangle
   (cx, cy) +/- (w + size): 50 tries of a random side and a random point
   along it (the original's ring loop runs once).
   The point must be on the map, in `zone` (unless -1) of the transport,
   allowed by the template and fit the passenger. */
int aoe_unit_unload_spot_ror(AoeLiveUnit *transport, int zone,
                             AoeLiveUnit *passenger, float cx, float cy,
                             float w, float h, float *x, float *y,
                             const AoeGarrisonOps *ops);
/* 0x004ab280: unload every passenger around the transport. Search
   centres start with the transport (twice its size); each placed
   passenger (moved to the transport, +0xd4, +0x34 at the point) adds its
   point as a centre, up to 9. A failed centre is replaced by the next
   newer one. *count: passengers unloaded. Returns 0 for bad input. */
int aoe_unit_unload_ror(AoeLiveUnit *transport, int zone,
                        const AoeGarrisonOps *ops, uint8_t *count);

#endif
