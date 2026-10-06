#ifndef VITA_AOE_UNIT_GRAPHIC_H
#define VITA_AOE_UNIT_GRAPHIC_H

#include <stdint.h>

#include "player_resources.h"

/* Graphics are opaque runtime objects. Pointer identity matches the original
   comparison; the DAT loader/renderer owns their allocation and lifetime. */
typedef struct {
    const void *standing_graphic; /* template +0x18 */
    const void *walking_graphic;  /* template +0x1c */
} AoeUnitGraphicTemplate;

typedef struct {
    const void *current_graphic;  /* unit +0x10 */
    const void *previous_graphic; /* unit +0x14 */
    uint8_t animation_state;      /* unit +0x48, used by Rise of Rome */
    uint8_t graphic_dirty;        /* unit +0x35 */
} AoeUnitGraphicState;

typedef struct {
    void *context; /* original sprite manager at unit +0x18 */
    void (*remove)(void *context, const void *graphic);
    void (*add)(void *context, const void *graphic,
                unsigned duration, unsigned arg1, unsigned arg2);
} AoeUnitGraphicOps;

/* Base 0x004161a0; Rise of Rome 0x004ab6b0 (patched 1.0a 0x004a9e10,
   the live unit's new-graphic slot). A null request selects the
   template's standing graphic. The callbacks correspond to the original
   sprite-manager calls; no graphics are synthesized here. Returns zero on
   invalid arguments, otherwise one (including when no switch is needed). */
int aoe_unit_graphic_select(AoeUnitGraphicState *state,
                            const AoeUnitGraphicTemplate *unit_template,
                            const AoeUnitGraphicOps *ops,
                            const void *requested_graphic,
                            AoePlayerEdition edition);

/* Patched RoR 1.0a 0x004a9e70: add a further graphic to the unit's sprite
   manager with the given priority (extra arguments 0, 0); 0x004a9e90:
   remove one. The current/previous graphic fields are not touched. */
int aoe_unit_graphic_add_overlay(const AoeUnitGraphicOps *ops,
                                 const void *graphic, unsigned priority);
int aoe_unit_graphic_remove_overlay(const AoeUnitGraphicOps *ops,
                                    const void *graphic);

#endif
