/* Patched RoR 1.0a type-25 doppelganger record: load 0x004361f0 /
   0x00436520, save 0x004366d0, link pass 0x00436580. */
#include "doppelganger_record.h"

#include <assert.h>
#include <string.h>

static uint8_t buf[512];
static AoeLiveUnit source_unit;
static DatUnit templ;

static AoeLiveUnit *find(void *c, int32_t id) {
    (void)c; return id == 77 ? &source_unit : NULL;
}
static const DatUnit *tmpl(void *c, int32_t owner, int32_t id) {
    (void)c; return owner == 2 && id == 5 ? &templ : NULL;
}

int main(void) {
    AoeDoppelRecordRor w, r;
    AoeOutcomeStream out = {0}, in = {0};
    AoeDoppelResolveOpsRor ops = {NULL, find, tmpl};
    AoeLiveUnit unit;
    memset(&w, 0, sizeof(w));
    aoe_unit_record_defaults_ror(&w.base, 20);
    w.base.class_code = 0x19;
    w.base.sprite_id = -1; w.base.attached_id = -1; w.base.template_id = 243;
    w.base.animation_rate = 1.5f;
    w.source_ref = 77; w.display_kind = 3; w.display_raw = 0x11223344u;
    w.template_id = 5; w.source_owner = 2; w.hidden_players = 0x0au;
    out.data = buf; out.size = sizeof(buf);
    assert(aoe_doppel_record_save_ror(&w, 7.23f, NULL, &out));
    assert(buf[0] == 0x19);
    /* tail: ref, kind, 4 raw, template, owner, hidden */
    assert(buf[out.pos - 21] == 77 && buf[out.pos - 17] == 3);
    assert(buf[out.pos - 16] == 0x44 && buf[out.pos - 4] == 0x0a);

    in.data = buf + 1; in.size = out.pos - 1;
    memset(&r, 0, sizeof(r));
    assert(aoe_doppel_record_load_ror(&r, 7.23f, NULL, &in) && in.pos == in.size);
    assert(r.base.class_code == 0x19 && r.base.animation_rate == 1.5f);
    assert(r.source_ref == 77 && r.display_raw == 0x11223344u &&
           r.template_id == 5 && r.source_owner == 2 && r.hidden_players == 0x0a);
    aoe_unit_record_free_ror(&r.base);

    /* before 7.06 the hidden-player bits are not written */
    {
        size_t full = out.pos;
        out.pos = 0;
        assert(aoe_doppel_record_save_ror(&w, 7.0f, NULL, &out));
        assert(out.pos == full - 4);
    }

    memset(&unit, 0, sizeof(unit));
    source_unit.core.unit_id = 77;
    assert(aoe_doppel_record_resolve_ror(&unit, &w, &ops));
    assert(unit.doppel.source == &source_unit && unit.doppel.source_id == 77 &&
           unit.doppel.source_template == &templ);
    /* a gone source (-1 - owner) keeps no link; out-of-range template fails */
    w.source_ref = -1 - 2; w.template_id = 9;
    assert(!aoe_doppel_record_resolve_ror(&unit, &w, &ops));
    assert(!unit.doppel.source && unit.doppel.source_id == -1);
    return 0;
}
