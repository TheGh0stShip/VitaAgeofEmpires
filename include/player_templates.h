#ifndef VITA_AOE_PLAYER_TEMPLATES_H
#define VITA_AOE_PLAYER_TEMPLATES_H

#include "dat.h"

#include <stdint.h>

/* Portable parsed-field portion of the original player template array.
   RoR player constructor 0x0045c650 copies civ +0x18/+0x1c to player
   +0x22/+0x24 by calling every present template's vtable +0x1c clone.
   Each slot here owns an independent copy of all currently parsed fields;
   original unparsed template fields and methods are not represented. */
typedef struct {
    uint16_t count;
    DatUnit **slots;
} AoePlayerTemplates;

/* `out` must be zero-initialized or freed first. On failure it is unchanged. */
int aoe_player_templates_clone_civ(const DatCiv *civ,
                                   AoePlayerTemplates *out);
DatUnit *aoe_player_templates_get(const AoePlayerTemplates *templates,
                                  uint16_t unit_id);
void aoe_player_templates_free(AoePlayerTemplates *templates);

/* Template vtable +0x1c: an independent copy of every parsed field (name,
   tasks, damage graphics, attacks, armors). NULL on allocation failure or
   a template without a name. Free with aoe_player_template_free_ror. */
DatUnit *aoe_player_template_clone_ror(const DatUnit *source);
void aoe_player_template_free_ror(DatUnit *unit);

#endif
