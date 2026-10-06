/* Patched RoR 1.0a type-25 doppelganger record (see doppelganger_record.h). */
#include "doppelganger_record.h"

#include <string.h>

#define DOPPEL_HIDDEN_VERSION 7.06f /* 0x005443b4 */

int aoe_doppel_record_load_ror(AoeDoppelRecordRor *r, float version,
                               const AoeUnitRecordOpsRor *ops,
                               AoeOutcomeStream *in) {
    const uint8_t *raw;
    if (!r || !in) return 0;
    if (!aoe_unit_record_load_ror(&r->base, 20, version, ops, in)) return 0;
    r->base.class_code = 0x19;
    r->hidden_players = 0;
    r->source_ref = aoe_stream_i32(in);
    r->display_kind = aoe_stream_u8(in);
    raw = aoe_stream_take(in, 4);
    r->display_raw = raw ? aoe_le_u32(raw) : 0;
    r->template_id = aoe_stream_i32(in);
    r->source_owner = aoe_stream_i32(in);
    if (version >= DOPPEL_HIDDEN_VERSION)
        r->hidden_players = (uint32_t)aoe_stream_i32(in);
    return !in->failed;
}

int aoe_doppel_record_save_ror(const AoeDoppelRecordRor *r, float version,
                               const AoeUnitRecordOpsRor *ops,
                               AoeOutcomeStream *out) {
    AoeUnitRecordRor base;
    size_t start;
    uint8_t raw[4];
    if (!r || !out) return 0;
    /* The shared type-20 save keys on class 20; its leading class byte is
       then the doppelganger's 0x19. */
    base = r->base;
    base.class_code = 20;
    start = out->pos;
    if (!aoe_unit_record_save_ror(&base, ops, out)) return 0;
    if (start < out->pos) out->data[start] = 0x19;
    aoe_stream_put_i32(out, r->source_ref);
    aoe_stream_put_u8(out, r->display_kind);
    aoe_le_put_u32(raw, r->display_raw);
    aoe_stream_put(out, raw, 4);
    aoe_stream_put_i32(out, r->template_id);
    aoe_stream_put_i32(out, r->source_owner);
    if (version >= DOPPEL_HIDDEN_VERSION)
        aoe_stream_put_i32(out, (int32_t)r->hidden_players);
    return !out->failed;
}

int aoe_doppel_record_resolve_ror(AoeLiveUnit *unit,
                                  const AoeDoppelRecordRor *r,
                                  const AoeDoppelResolveOpsRor *ops) {
    if (!unit || !r || !ops) return 0;
    unit->doppel.source = NULL;
    unit->doppel.source_id = -1;
    unit->doppel.source_owner = (int16_t)r->source_owner;
    unit->doppel.hidden_players = r->hidden_players;
    unit->doppel.last_mask = 0;
    if (r->source_ref >= 0 && ops->find_unit) {
        AoeLiveUnit *source = ops->find_unit(ops->context, r->source_ref);
        if (source) {
            unit->doppel.source = source;
            unit->doppel.source_id = source->core.unit_id;
        }
    }
    unit->doppel.source_template = NULL;
    if (r->template_id >= 0 && ops->template_at)
        unit->doppel.source_template =
            ops->template_at(ops->context, r->source_owner, r->template_id);
    return unit->doppel.source_template != NULL;
}
