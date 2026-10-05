/* Patched RoR 1.0a tribe player command methods (0x004f10e0..0x004f2d60). */
#include "tribe_player_commands.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static AoeTribeCommandUnitRor u1 = {11, 2}, u2 = {12, 2}, bld = {30, 2};
static int16_t last_min = -9, last_cls = -9, last_8f = -9, units_n = 2;
static int16_t collect(void *c, int16_t m, int16_t cls, int16_t t, int16_t f, const AoeTribeCommandUnitRor **out) {
    (void)c; (void)t;
    last_min = m, last_cls = cls, last_8f = f;
    out[0] = &u1;
    out[1] = &u2;
    return units_n;
}
static int tstate = 2;
static int template_state(void *c, int16_t id) { (void)c; (void)id; return tstate; }
static int acks_b, acks_u;
static void ack_b(void *c) { (void)c; ++acks_b; }
static void ack_u(void *c, const AoeTribeCommandUnitRor *u) { (void)c; assert(u == &u1); ++acks_u; }
static const AoeTribeCommandUnitRor *sel = &bld;
static const AoeTribeCommandUnitRor *selected(void *c) { (void)c; return sel; }
static uint8_t otype = 'P';
static uint8_t object_type(void *c, const AoeTribeCommandUnitRor *o) { (void)c; (void)o; return otype; }
static int16_t ocls = 3;
static int16_t object_class(void *c, const AoeTribeCommandUnitRor *o) { (void)c; (void)o; return ocls; }
static uint8_t queued[0x200];
static size_t qsize;
static int queue(void *c, const uint8_t *cmd, size_t n) { (void)c; memcpy(queued, cmd, n); qsize = n; return 1; }

int main(void) {
    AoeTribePlayerOpsRor ops = {0, 2, collect, template_state, ack_b, ack_u, selected, object_type, object_class, queue};
    /* Build: villagers (min 4, class 4), template 118 acknowledges, 0x66. */
    assert(aoe_tribe_player_build_ror(&ops, 70, 5.0f, 6.0f) && last_min == 4 && last_cls == 4 && acks_b == 1);
    assert(queued[0] == 0x66 && queued[1] == 2);
    tstate = 1;
    assert(!aoe_tribe_player_build_ror(&ops, 70, 5, 6));
    assert(aoe_tribe_player_build_ror(&ops, 0x6d, 5, 6)); /* 0x6d needs only presence */
    tstate = 0;
    assert(!aoe_tribe_player_build_ror(&ops, 0x6d, 5, 6));
    tstate = 2;
    units_n = 0;
    assert(!aoe_tribe_player_build_ror(&ops, 70, 5, 6));
    units_n = 2;
    assert(aoe_tribe_player_build_69_ror(&ops, 70, 1, 2, 3, 300) && queued[0] == 0x69);
    /* Train / research at the selected building. */
    assert(aoe_tribe_player_train_ror(&ops, 83) && queued[0] == 0x64 && qsize == 0x10);
    assert(aoe_tribe_player_research_ror(&ops, 5) && queued[0] == 0x65);
    otype = 'F';
    assert(!aoe_tribe_player_train_ror(&ops, 83));
    /* Delete. */
    assert(aoe_tribe_player_delete_selected_ror(&ops) && queued[0] == 0x6a);
    ocls = 1;
    assert(!aoe_tribe_player_delete_selected_ror(&ops));
    otype = 'D';
    assert(!aoe_tribe_player_delete_ror(&ops, &bld, 1));
    otype = 'P';
    assert(!aoe_tribe_player_delete_ror(&ops, &bld, 0));
    /* Selection commands acknowledge with the first unit. */
    assert(aoe_tribe_player_6d_ror(&ops, 7) && last_8f == 5 && acks_u == 1 && queued[0] == 0x6d);
    assert(aoe_tribe_player_6b_ror(&ops, 1, 2) && last_8f == 4 && queued[0] == 0x6b);
    assert(aoe_tribe_player_6f_ror(&ops, 1, 2) && last_8f == 7 && queued[0] == 0x6f);
    assert(aoe_tribe_player_6d_one_ror(&ops, 44, 3) && queued[0] == 0x6d);
    assert(aoe_tribe_player_66_one_ror(&ops, 44, 70, 1, 2, -1) && queued[0] == 0x66);
    assert(aoe_tribe_player_67_ror(&ops, 5, 9) && queued[0] == 0x67);
    /* Template and tech checks. */
    tstate = 1;
    assert(!aoe_tribe_player_template_usable_ror(&ops, 70) && aoe_tribe_player_template_usable_ror(&ops, 0x6d));
    {
        uint8_t entries[24] = {0};
        entries[8 + 4] = 1;
        assert(aoe_tribe_tech_state_is_one_ror(entries, 1) && !aoe_tribe_tech_state_is_one_ror(entries, 0));
    }
    assert(aoe_tribe_research_open_ror(2) && !aoe_tribe_research_open_ror(3) && !aoe_tribe_research_open_ror(-1));
    puts("tribe player commands ok");
    return 0;
}
