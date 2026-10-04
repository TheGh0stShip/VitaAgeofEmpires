#ifndef VITA_AOE_LIVE_UNIT_OWNER_H
#define VITA_AOE_LIVE_UNIT_OWNER_H

#include <stdint.h>

#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a unit vtable +0x44, change of owner (used by
   conversion, 0x004b2280):
   - root (types 10..60, 90; 0x004a9ea0);
   - type 70 (0x004aeac0), whose class-1 units (artifacts) take the
     type-40 path 0x00406590: stop (+0xa4), root, adopt nothing (+0x208);
   - type 80 (0x004ad190).
   Player-side effects are callbacks; their original methods are named on
   each. */
typedef struct {
    void *context;
    /* New owner +0x24[id]; NULL when the owner has no such template. */
    const DatUnit *(*owner_template)(void *context, int16_t owner,
                                     int16_t template_id);
    /* Old owner +0xe0(unit, +0x49, +0x4a, node) then new owner +0xdc(unit,
       +0x49, +0x4a), which returns the new list node (+0x50). Nonzero on
       success. */
    int (*move_lists)(void *context, AoeLiveUnit *unit, int16_t from,
                      int16_t to);
    /* Unit +0xdc(owner, foundation, -1) (remove) / +0xd8(owner,
       foundation, -1) (add): the unit's vision for that owner. */
    void (*vision)(void *context, AoeLiveUnit *unit, int16_t owner, int add,
                   int foundation);
    /* Owner +0x78(type, amount, 0). */
    void (*resource_add)(void *context, int16_t owner, int16_t type,
                         float amount);
    /* Graphic of an ID (NULL when none) and unit +0x38. */
    const void *(*graphic)(void *context, int16_t graphic_id);
    void (*select_graphic)(void *context, AoeLiveUnit *unit,
                           const void *graphic);
    /* Unit +0x148(id, id, 0x20b, new owner index, 0, 0). */
    void (*notice)(void *context, AoeLiveUnit *unit, int16_t new_owner);
    /* Type 80: queue drain 0x004add50(item, count), the +0x184 list
       clear, new owner +0x24[id] +0x18 create at the unit's position and
       the new owner's research completion 0x004eaa3d. */
    void (*queue_cancel)(void *context, AoeLiveUnit *unit, int16_t item,
                         int16_t count);
    void (*clear_actions)(void *context, AoeLiveUnit *unit);
    void (*create)(void *context, int16_t owner, int16_t template_id,
                   float x, float y, float z);
    void (*complete_research)(void *context, int16_t owner, int16_t tech);
    /* Global +0xa34 + 4 x owner (status-4 queue counter). */
    uint32_t *(*status_four_count)(void *context, int16_t owner);
    /* Class-1 path: unit +0xa4 (stop) and +0x208(NULL). */
    void (*stop)(void *context, AoeLiveUnit *unit);
    /* Type 70 +0x74 group object: 0x00414eb0(world +0x04). */
    void (*group_changed)(void *context, AoeLiveUnit *unit);
} AoeLiveUnitOwnerOps;

/* Unit +0x44. The unit's selection highlight (+0x36 bit 0, 0x0045dd00)
   and the type-70 per-unit owner bytes (+0x18c[9] +0x05) are interface
   state that is not modelled. Returns 0 for missing bindings or a failed
   list move or template copy; 1 otherwise, including when the new owner
   lacks the template (nothing changes, as in the original). */
int aoe_live_unit_change_owner_ror(AoeLiveUnit *unit, int16_t new_owner,
                                   const AoeLiveUnitOwnerOps *ops);

#endif
