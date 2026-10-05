#ifndef VITA_AOE_UNIT_STORAGE_H
#define VITA_AOE_UNIT_STORAGE_H

#include "dat.h"
#include "player_resources.h"

/* Unit +0x44/+0x4c after construction. This is semantic portable state,
   not the original packed 32-bit object layout. */
typedef struct {
    int16_t carried_resource_type;
    float carried_amount;
} AoeUnitStorageState;

/* RoR 0x004a8950/0x004a89a0 and base 0x004133a0/0x004133f0.
   Mode 0 selects the last resource entry whose type is not -1. Modes 1/2
   add their amounts to the owner's indexed resources when the type is
   nonnegative. This is the storage portion of the original unit constructor,
   not full unit construction. Returns zero without mutation on invalid
   arguments or edition. */
int aoe_unit_storage_initialize(const DatUnit *unit,
                                AoePlayerResources *owner_resources,
                                AoePlayerEdition edition,
                                AoeUnitStorageState *state);

/* RoR 0x004a8a00 / base 0x00413440: when a live unit leaves states 0–2,
   subtract each mode-2 template storage amount from its owner's resource
   slot. This is the exact three-entry resource effect, not the rest of the
   state transition. */
int aoe_unit_storage_exit_active(const DatUnit *unit,
                                  AoePlayerResources *owner_resources,
                                  AoePlayerEdition edition);

/* 0x004a7150: mode 0 alone (the load path): the type -1 and amount 0,
   then the last entry with mode 0 and a type other than -1. */
void aoe_unit_storage_select_ror(const DatUnit *unit, AoeUnitStorageState *state);

#endif
