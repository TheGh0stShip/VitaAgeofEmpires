#ifndef VITA_AOE_DOPPELGANGER_RECORD_H
#define VITA_AOE_DOPPELGANGER_RECORD_H

#include <stdint.h>

#include "live_unit_factory.h"
#include "unit_record.h"

/* Patched RoR 1.0a type-25 doppelganger (vtable 0x005441bc) saved
   record: the type-20 record (class byte 0x19), then its own fields. The
   live tile-mask pointer (+0x9c) and last mask (+0xa0) are not saved. */
typedef struct {
    AoeUnitRecordRor base;   /* class 20 fields, class_code 0x19 */
    int32_t source_ref;      /* +0x8c source unit ID, or -1 - owner (+0x98)
                                once the source is gone */
    uint8_t display_kind;    /* +0xa8 */
    uint32_t display_raw;    /* +0xa9: 4 raw bytes */
    int32_t template_id;     /* +0x94 template's ID (+0x10), -1 none */
    int32_t source_owner;    /* +0x98 */
    uint32_t hidden_players; /* +0xa4: saves from 7.06 (0x005443b4), else 0 */
} AoeDoppelRecordRor;

/* Load constructor 0x004361f0 with its stream part 0x00436520: the
   type-20 record, class 0x19, +0xa0 / +0xa4 0 (the +0x9c tile-mask
   pointer at the loaded position is the caller's), then source_ref,
   display bytes, template ID, owner and (from 7.06) hidden_players. */
int aoe_doppel_record_load_ror(AoeDoppelRecordRor *r, float version,
                               const AoeUnitRecordOpsRor *ops,
                               AoeOutcomeStream *in);

/* Save 0x004366d0 (vtable +0x30) at `version`: the type-20 record with
   class byte 0x19, then source_ref (the live source's ID, else -1 -
   owner), the display bytes, the template ID (-1 none), the owner and,
   from 7.06, hidden_players. */
int aoe_doppel_record_save_ror(const AoeDoppelRecordRor *r, float version,
                               const AoeUnitRecordOpsRor *ops,
                               AoeOutcomeStream *out);

typedef struct {
    void *context;
    /* The world's unit lookup: the live unit with this ID, NULL when gone. */
    AoeLiveUnit *(*find_unit)(void *context, int32_t unit_id);
    /* Player `owner`'s template `id` when id < its template count
       (player +0x22 / +0x24), else NULL. */
    const DatUnit *(*template_at)(void *context, int32_t owner, int32_t id);
} AoeDoppelResolveOpsRor;

/* Post-load link pass 0x00436580 (vtable +0x2c, run once every unit
   has loaded): a negative source_ref leaves no source and source ID -1
   (the colour comes from player -1 - ref); otherwise the source is
   looked up and, when found, its ID kept. The template is the owner's
   template `template_id` when in range. Returns 0 when it is not (the
   original then removes the unit through vtable +0x68). */
int aoe_doppel_record_resolve_ror(AoeLiveUnit *unit,
                                  const AoeDoppelRecordRor *r,
                                  const AoeDoppelResolveOpsRor *ops);

#endif
