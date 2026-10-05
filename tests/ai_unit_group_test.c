/* Patched RoR 1.0a tactical-AI unit group 0x004cc630..0x004cd2c0. */
#include "ai_unit_group.h"

#include <assert.h>
#include <string.h>

static int32_t led[41], led_n = -1, lead_by = -2, released = -2, follows, unfollows;
static int32_t dismissed = -1;

static void lead(void *c, int32_t cmd, const int32_t *ids, int32_t n, float r) {
    (void)c; assert(r == 4.0f);
    lead_by = cmd; led_n = n; memcpy(led, ids, (size_t)n * 4);
}
static void follow(void *c, int32_t cmd, int32_t id, float r) { (void)c; (void)cmd; (void)id; assert(r == 4.0f); ++follows; }
static void unfollow(void *c, int32_t cmd, int32_t id) { (void)c; (void)cmd; (void)id; ++unfollows; }
static void release(void *c, int32_t cmd) { (void)c; released = cmd; }
static int32_t now(void *c) { (void)c; return 777; }
static int present(void *c, int32_t id) { (void)c; return id != 12; }
static int hp(void *c, int32_t id, float *out) { (void)c; if (id == 12) return 0; *out = (float)id + 0.9f; return 1; }
static void dismiss(void *c, int32_t id, int32_t v) { (void)c; assert(v == 100); dismissed = id; }

int main(void) {
    static AoeAiUnitGroupRor g, h;
    AoeAiUnitGroupOpsRor ops = {NULL, lead, follow, unfollow, release, now,
                                present, hp, dismiss};
    static uint8_t buf[2048];
    AoeOutcomeStream out = {buf, sizeof buf, 0, 0}, in;

    aoe_ai_unit_group_init_ror(&g);
    assert(g.id == -1 && g.commander == -1 && g.members[39] == -1 &&
           g.loc_1ac[2] == -1.0f && g.list_2d0[19] == -1 && g.count == 0);

    assert(aoe_ai_unit_group_add_ror(&g, 10, &ops) && g.added_time == 777);
    assert(aoe_ai_unit_group_add_ror(&g, 11, &ops));
    assert(aoe_ai_unit_group_add_ror(&g, 12, &ops) && follows == 0);
    aoe_ai_unit_group_set_commander_ror(&g, 10, &ops);
    assert(lead_by == 10 && led_n == 3 && led[2] == 12);
    assert(aoe_ai_unit_group_add_ror(&g, 13, &ops) && follows == 1 && g.count == 4);

    /* Slots keep holes; the n-th lookups skip them. */
    assert(aoe_ai_unit_group_remove_ror(&g, 11, &ops) && unfollows == 1);
    assert(g.members[1] == -1 && aoe_ai_unit_group_member_ror(&g, 1) == 12 &&
           aoe_ai_unit_group_member_ror(&g, 2) == 13 &&
           aoe_ai_unit_group_member_ror(&g, 3) == -1 &&
           aoe_ai_unit_group_value_ror(&g, 0) == -1);
    assert(aoe_ai_unit_group_contains_ror(&g, 13) && !aoe_ai_unit_group_contains_ror(&g, 11));
    assert(!aoe_ai_unit_group_remove_ror(&g, 99, &ops));
    /* A re-add fills the first hole. */
    assert(aoe_ai_unit_group_add_ror(&g, 14, &ops) && g.members[1] == 14);

    /* Hit points truncate per member; a missing object adds nothing. */
    assert(aoe_ai_unit_group_hit_points_ror(&g, &ops) == 10 + 14 + 13);

    /* Pruning drops unit 12 and dismisses it. */
    assert(aoe_ai_unit_group_prune_ror(&g, &ops) && dismissed == 12 &&
           g.count == 3 && unfollows == 2);
    /* Removing the commander releases it. */
    assert(aoe_ai_unit_group_remove_slot_ror(&g, 0, &ops) && released == 10 &&
           g.commander == -1 && g.count == 2);
    assert(!aoe_ai_unit_group_remove_slot_ror(&g, 40, &ops));

    aoe_ai_unit_group_set_all_locations_ror(&g, 1, 2, 3);
    assert(g.location[0] == 1 && g.loc_19c[1] == 2 && g.loc_1ac[2] == 3 &&
           g.loc_18c[0] == -1.0f);

    /* Save writes the newest layout: 8 + 320 + 20 + 16 + 12 + 48 + 16 + 12
       + 2 + 240 + 4 + 1 + 4 + 80 + 8 + 4 + 4 bytes. */
    g.flag_2c8 = 1;
    g.field_328 = 5;
    g.block_1d8[0xef] = 0xab;
    assert(aoe_ai_unit_group_save_ror(&g, &out));
    assert(out.pos == 799);
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    assert(aoe_ai_unit_group_load_ror(&h, 7.2f, &in) && in.pos == out.pos);
    assert(h.members[1] == 14 && h.count == 2 && h.block_1d8[0xef] == 0xab &&
           h.flag_2c8 == 1 && h.field_328 == 5 && h.added_time == 777 &&
           h.loc_19c[1] == 2.0f);
    /* An older save stops before the version-gated tail. */
    in = (AoeOutcomeStream){buf, out.pos, 0, 0};
    aoe_ai_unit_group_init_ror(&h);
    assert(aoe_ai_unit_group_load_ror(&h, 6.9f, &in) && in.pos == 698 &&
           h.flag_2c8 == 0 && h.field_328 == 0);

    aoe_ai_unit_group_clear_ror(&g, &ops);
    assert(g.count == 0 && g.members[1] == -1);
    return 0;
}
