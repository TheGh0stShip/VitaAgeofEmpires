#ifndef VITA_AOE_LIVE_UNIT_ID_H
#define VITA_AOE_LIVE_UNIT_ID_H

#include "dat.h"
#include "visibility_watch.h"

#include <stdint.h>

/* RoR common live initializer 0x004a8200 / base 0x00412c20 chooses world
   +0x68 or +0x6c ID cursor from template byte +0xa4. The DAT parser exposes
   that byte as negative_id_mode; the world cursor operations are translated
   separately. This does not construct or register a live unit. */
int aoe_live_unit_allocate_id(const DatUnit *unit_template,
                              AoeOwnedVisibilityRegistry *registry,
                              int32_t *unit_id);

#endif
