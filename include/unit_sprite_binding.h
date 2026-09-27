#ifndef VITA_AOE_UNIT_SPRITE_BINDING_H
#define VITA_AOE_UNIT_SPRITE_BINDING_H

#include "sprite_runtime.h"
#include "unit_graphic.h"

/* Connects the original unit graphic-selection rule to the DAT-backed
   sprite manager. The target is acquired before the old graphic is released
   so allocation failure leaves the unit unchanged; successful final state
   and reference counts match the original. This ordering difference is a
   portable failure-safety adaptation, not a claim of instruction identity. */
int aoe_unit_graphic_select_runtime(AoeUnitGraphicState *state,
                                    const AoeUnitGraphicTemplate *unit_template,
                                    AoeSpriteRuntime *runtime,
                                    const DatGraphic *requested_graphic,
                                    AoePlayerEdition edition);

#endif
