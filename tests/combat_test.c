#include "combat.h"

#include <assert.h>
#include <math.h>
#include <string.h>

static int notices, kills, hp_seen, max_seen, blasts, receives, attack_notices;
static int32_t rand_value;
static float mult_seen;

static void damaged(void *c, AoeLiveUnit *t, AoeLiveUnit *a, int32_t hp,
                    int32_t max) {
    (void)c; (void)t; (void)a;
    ++notices;
    hp_seen = hp;
    max_seen = max;
}
static int16_t owner(void *c, AoeLiveUnit *u) { (void)c; (void)u; return 2; }
static void credit(void *c, void *p) { (void)c; (void)p; ++kills; }
static int dies, spared, kill_attr, killed_notices;
static int spares(void *c, AoeLiveUnit *v, int16_t cls) {
    (void)c; (void)v;
    return cls == spared;
}
static void clear_carried(void *c, AoeLiveUnit *v) { (void)c; (void)v; ++dies; }
static void killed_notice(void *c, AoeLiveUnit *v, AoeLiveUnit *a) {
    (void)c; (void)v; (void)a;
    ++killed_notices;
}
static int credit_attr(void *c, void *p, int16_t attribute) {
    (void)c; (void)p;
    kill_attr = attribute;
    return 1;
}
static int32_t rnd(void *c) { (void)c; return rand_value; }
static void attack_notice(void *c, AoeLiveUnit *a, int32_t id) {
    (void)c; (void)a; (void)id;
    ++attack_notices;
}
static AoeLiveUnit *blast_excluded;
static void blast(void *c, AoeLiveUnit *a, float x, float y, float z,
                  AoeLiveUnit *src, AoeLiveUnit *excluded) {
    (void)c; (void)x; (void)y; (void)z;
    assert(src == a);
    ++blasts;
    blast_excluded = excluded;
}
static int destroyed_count;
static void destroy(void *c, AoeLiveUnit *t) { (void)c; (void)t; ++destroyed_count; }
static float multiplier(void *c, AoeLiveUnit *a, AoeLiveUnit *t) {
    (void)c; (void)a; (void)t;
    return 1.5f;
}
static void receive(void *c, AoeLiveUnit *t, const DatUnitAttackArmor *at,
                    int32_t n, float m, AoeLiveUnit *a) {
    (void)c; (void)t; (void)at; (void)n; (void)a;
    ++receives;
    mult_seen = m;
}

static AoeLiveUnit *tile_a[3];
static size_t tile_a_count;
static int32_t tiles_seen[64][2], tiles_seen_count;
static AoeLiveUnit *blast_hits[8];
static int blast_hit_count, killed_count;
static AoeLiveUnit *owner_a, *owner_b;

static size_t tile_units(void *c, int32_t x, int32_t y, AoeLiveUnit **out,
                         size_t cap) {
    (void)c;
    if (tiles_seen_count < 64) {
        tiles_seen[tiles_seen_count][0] = x;
        tiles_seen[tiles_seen_count][1] = y;
        ++tiles_seen_count;
    }
    if (x != 5 || y != 5) return 0;
    for (size_t i = 0; i < tile_a_count && i < cap; ++i) out[i] = tile_a[i];
    return tile_a_count;
}
static void blast_receive(void *c, AoeLiveUnit *t,
                          const DatUnitAttackArmor *at, int32_t n, float m,
                          AoeLiveUnit *src) {
    (void)c; (void)at; (void)n; (void)m; (void)src;
    blast_hits[blast_hit_count++] = t;
    t->core.hit_points = 0.5f;
}
static int same(void *c, AoeLiveUnit *a, AoeLiveUnit *b) {
    (void)c;
    return (a == owner_a || a == owner_b) && (b == owner_a || b == owner_b);
}
static void killed(void *c, AoeLiveUnit *v) { (void)c; (void)v; ++killed_count; }

static int s_damage_count, s_cleared, s_graphic, s_owner_notices, s_ai, s_ai_handles;
static float s_mult, s_scale;
static void s_take(void *c, AoeLiveUnit *t, int16_t n, const DatUnitAttackArmor *a, float m, AoeLiveUnit *at) {
    (void)c; (void)t; (void)a; (void)at; s_damage_count = n; s_mult = m;
}
static float s_multiplier(void *c, AoeLiveUnit *a, AoeLiveUnit *t) { (void)c; (void)a; (void)t; return 1.5f; }
static void s_scaled(void *c, AoeLiveUnit *u, int32_t a, float sc, int32_t b) { (void)c; (void)u; (void)a; (void)b; s_scale = sc; }
static void s_clear(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++s_cleared; }
static void s_setg(void *c, AoeLiveUnit *u, int16_t g) { (void)c; (void)u; s_graphic = g; }
static int s_hasai(void *c, AoeLiveUnit *u) { (void)c; (void)u; return s_ai; }
static int s_ainotice(void *c, AoeLiveUnit *u, int32_t a, int32_t b, int32_t code, int32_t cc, int32_t d, int32_t e) {
    (void)c; (void)u; (void)a; (void)b; (void)code; (void)cc; (void)d; (void)e; return s_ai_handles;
}
static void s_owner(void *c, AoeLiveUnit *u, int32_t a, int32_t b, int32_t code, int32_t cc, int32_t d, int32_t e) {
    (void)c; (void)u; (void)a; (void)b; (void)code; (void)cc; (void)d; (void)e; ++s_owner_notices;
}

static void count_death(void *c, AoeLiveUnit *u) {
    (void)u;
    ++*(int *)c;
}

int main(void) {
    DatUnit at, tt;
    AoeLiveUnit attacker, target;
    DatUnitAttackArmor attacks[2] = {{4, 7}, {3, 2}};
    DatUnitAttackArmor armors[2] = {{4, 1}, {4, 2}};
    AoeCombatReceiveOps rops = {NULL, damaged, owner, credit};
    AoeCombatHitOps hops = {NULL, rnd, attack_notice, blast, multiplier,
                            receive, 0, destroy};
    float x, y, z, r;
    memset(&at, 0, sizeof(at));
    memset(&tt, 0, sizeof(tt));
    memset(&attacker, 0, sizeof(attacker));
    memset(&target, 0, sizeof(target));
    attacker.core.unit_template = &at;
    target.core.unit_template = &tt;

    /* Reload: ready at <= 0 (and NaN); reset to template reload time. */
    assert(aoe_combat_ready_ror(0.0f) && aoe_combat_ready_ror(-1.0f) &&
           aoe_combat_ready_ror(NAN) && !aoe_combat_ready_ror(0.1f));
    at.reload_time = 2.0f;
    assert(aoe_combat_reload_ror(&at) == 2.0f);

    /* Damage: class 4 uses the last matching armor (2): 7 - 2 = 5; class 3
       falls back to base armor 3: 2 - 3 < 0 is dropped. */
    tt.base_armor = 3;
    tt.armors = armors;
    tt.armor_count = 2;
    assert(aoe_combat_damage_ror(&tt, attacks, 2, 1.0f, 1.0f) == 5.0);
    assert(aoe_combat_damage_ror(&tt, attacks, 2, 0.5f, 1.5f) == 3.75);
    assert(aoe_combat_damage_ror(&tt, attacks, 0, 1.0f, 1.0f) == 0.0);

    /* Base receive: hp 40 - 5 = 35, notice with the truncated hp and the
       template maximum; below 1.0 clamps to 0. */
    tt.hit_points = 40;
    target.core.hit_points = 40.0f;
    assert(aoe_combat_receive_ror(&target, 1.0f, attacks, 2, 1.0f,
                                  &attacker, &rops) == 35.0f &&
           notices == 1 && hp_seen == 35 && max_seen == 40);
    target.core.hit_points = 5.5f;
    assert(aoe_combat_receive_ror(&target, 1.0f, attacks, 2, 1.0f, NULL,
                                  &rops) == 0.0f && notices == 1);

    /* Tribe receive: dead targets are skipped; a weak hit still removes 1
       (0.1 for light attackers); dropping below 1 credits a kill. */
    target.core.hit_points = 0.0f;
    assert(aoe_combat_receive_building_ror(&target, 1.0f, attacks, 2, 1.0f,
                                        NULL, &attacker, &rops) == 0.0f &&
           notices == 1);
    attacks[0].amount = 2; /* 2 - 2 = 0 */
    target.core.hit_points = 10.0f;
    at.unit_class = 6;
    assert(aoe_combat_receive_building_ror(&target, 1.0f, attacks, 2, 1.0f,
                                        NULL, &attacker, &rops) == 9.0f);
    at.unit_class = 0x17;
    assert(aoe_combat_receive_building_ror(&target, 1.0f, attacks, 2, 1.0f,
                                        NULL, &attacker, &rops) ==
           (float)(9.0 - (double)0.1f));
    at.unit_class = 6;
    target.core.hit_points = 1.5f;
    assert(aoe_combat_receive_building_ror(&target, 1.0f, attacks, 2, 1.0f,
                                        NULL, &attacker, &rops) == 0.5f &&
           kills == 1);

    /* Type-70 receive 0x004ae9d0. */
    {
        AoeCombatReceiveOps uops = {NULL, damaged, owner, credit, spares,
                                    clear_carried,
                                    killed_notice, credit_attr};
        DatUnitAttackArmor hit5[1] = {{4, 5}};
        tt.armors = NULL;
        tt.armor_count = 0;
        tt.base_armor = 0;
        at.unit_class = 6;
        spared = -1;
        /* Already below 1.0: nothing happens. */
        target.core.hit_points = 0.5f;
        assert(aoe_combat_receive_unit_ror(&target, 1.0f, hit5, 1, 1.0f, NULL,
                                           &attacker, &uops) == 0.5f);
        /* Ordinary hit. */
        target.core.hit_points = 20.0f;
        assert(aoe_combat_receive_unit_ror(&target, 1.0f, hit5, 1, 1.0f, NULL,
                                           &attacker, &uops) == 15.0f &&
               !dies);
        /* A zero-damage hit still removes 1.0. */
        hit5[0].amount = 0;
        assert(aoe_combat_receive_unit_ror(&target, 1.0f, hit5, 1, 1.0f, NULL,
                                           &attacker, &uops) == 14.0f);
        /* A killing hit dies, credits attribute 0x14 and notices 0x201. */
        hit5[0].amount = 30;
        assert(aoe_combat_receive_unit_ror(&target, 1.0f, hit5, 1, 1.0f, NULL,
                                           &attacker, &uops) == 0.0f &&
               dies == 1 && kill_attr == 0x14 && killed_notices == 1);
        /* A spared attacker class skips only the death call. */
        spared = 6;
        target.core.hit_points = 20.0f;
        aoe_combat_receive_unit_ror(&target, 1.0f, hit5, 1, 1.0f, NULL,
                                    &attacker, &uops);
        assert(dies == 1 && killed_notices == 2);
    }

    /* Tribe multipliers. */
    at.projectile_unit_id = 3;
    assert(aoe_combat_multiplier_tribe_ror(&attacker, &target, 1) == 1.5f &&
           aoe_combat_multiplier_tribe_ror(&attacker, &target, 0) == 1.0f);
    at.projectile_unit_id = -1;
    target.core.position.z40 = 1.0f;
    assert(aoe_combat_multiplier_tribe_ror(&attacker, &target, 1) == 0.6666f);
    attacker.core.position.z40 = 1.0f;
    assert(aoe_combat_multiplier_tribe_ror(&attacker, &target, 1) == 1.0f);

    /* Projectile origin: facing 0 puts the forward offset on y and the
       side offset on x (as the original's rotation does). */
    at.projectile_offset[0] = 0.5f;
    at.projectile_offset[1] = 0.25f;
    at.projectile_offset[2] = 1.0f;
    at.size[0] = 0.3f;
    at.size[1] = 0.4f;
    at.max_range = 5.0f;
    attacker.core.position.x38 = 10.0f;
    attacker.core.position.y3c = 20.0f;
    aoe_combat_projectile_origin_ror(&attacker, 0.0f, &x, &y, &z, &r);
    assert(x == 10.25f && y == 20.5f && z == 2.0f &&
           r == (float)(sqrt((double)0.3f * 0.3f + (double)0.4f * 0.4f) +
                        5.0 + 6.0));

    /* Melee hit: roll rand * 100 / 32767 against accuracy. */
    at.accuracy_percent = 50;
    at.attacks = attacks;
    at.attack_count = 2;
    rand_value = 16384; /* 50 */
    assert(aoe_combat_melee_hit_ror(&attacker, &target, 0, 0, 0, &hops) == 1 &&
           receives == 1 && mult_seen == 1.5f && attack_notices == 1);
    rand_value = 16712; /* 51: miss */
    assert(aoe_combat_melee_hit_ror(&attacker, &target, 0, 0, 0, &hops) == 0 &&
           receives == 1 && attack_notices == 2);
    at.blast_width = 1.0f; /* blast templates hand over without a notice */
    assert(aoe_combat_melee_hit_ror(&attacker, &target, 0, 0, 0, &hops) == 0 &&
           blasts == 1 && attack_notices == 2);
    assert(blast_excluded == NULL);
    /* Tribe: a short-range blast unit splashes around the target and
       still rolls a direct hit; long-range blast units use the base path. */
    at.max_range = 0.5f;
    at.accuracy_percent = 100;
    rand_value = 0;
    assert(aoe_combat_melee_hit_tribe_ror(&attacker, &target, 0, 0, 0,
                                          &hops) == 1 &&
           blasts == 2 && blast_excluded == &target && receives == 2 &&
           attack_notices == 3);
    at.max_range = 0.99f;
    assert(aoe_combat_melee_hit_tribe_ror(&attacker, &target, 0, 0, 0,
                                          &hops) == 0 &&
           blasts == 3 && receives == 2);
    /* The instant-kill option for template IDs 122/123. */
    hops.instant_kill_option = 1;
    at.id = 122;
    assert(aoe_combat_melee_hit_tribe_ror(&attacker, &target, 0, 0, 0,
                                          &hops) == 1 &&
           destroyed_count == 1);
    at.id = 4;
    hops.instant_kill_option = 0;
    at.blast_width = 0.0f;
    assert(aoe_combat_melee_hit_ror(&attacker, NULL, 0, 0, 0, &hops) == 0);
    assert(aoe_combat_melee_hit_ror(&attacker, &target, 0, 0, 0, NULL) == -1);

    /* Edge distances: centres 3 apart on x, sizes 0.3 + 0.25. */
    {
        AoeLiveUnit a2, b2;
        DatUnit ta, tb;
        memset(&a2, 0, sizeof(a2));
        memset(&b2, 0, sizeof(b2));
        memset(&ta, 0, sizeof(ta));
        memset(&tb, 0, sizeof(tb));
        ta.size[0] = ta.size[1] = 0.25f;
        tb.size[0] = tb.size[1] = 0.25f;
        a2.core.unit_template = &ta;
        b2.core.unit_template = &tb;
        b2.core.position.x38 = 3.0f;
        assert(aoe_combat_distance_ror(&a2, &b2) == 2.5);
        assert(aoe_combat_in_range_ror(&a2, &b2, 2.6f) &&
               !aoe_combat_in_range_ror(&a2, &b2, 2.5f));
        b2.core.position.y3c = 4.5f; /* ey = 4.0 */
        assert(aoe_combat_distance_ror(&a2, &b2) == sqrt(2.5 * 2.5 + 16.0));
        assert(aoe_combat_point_distance_ror(&a2, 3.0f, 0.0f) == 2.75);
        assert(aoe_combat_point_distance_ror(&a2, 0.1f, 0.1f) == 0.0);
    }

    /* Blast at (5.5, 5.5) with radius 1: tiles 2..8 (exclusive) on both
       axes in y-major order, clamped to a 7-wide map. */
    {
        AoeCombatBlastOps bops = {NULL, rnd, 7, 20, tile_units, multiplier,
                                  blast_receive, same, killed};
        /* `killed` stands in for the carried-resource clear. */
        AoeLiveUnit near_u, far_u, ally;
        DatUnit nt;
        memset(&nt, 0, sizeof(nt));
        memset(&near_u, 0, sizeof(near_u));
        memset(&far_u, 0, sizeof(far_u));
        memset(&ally, 0, sizeof(ally));
        nt.size[0] = nt.size[1] = 0.25f;
        near_u.core.unit_template = far_u.core.unit_template =
            ally.core.unit_template = &nt;
        near_u.core.position.x38 = 6.0f; /* edge 0.25 away */
        near_u.core.position.y3c = 5.5f;
        far_u.core.position.x38 = 7.0f;  /* edge 1.25 away */
        far_u.core.position.y3c = 5.5f;
        ally.core.position.x38 = 5.5f;
        ally.core.position.y3c = 5.5f;
        tile_a[0] = &near_u;
        tile_a[1] = &far_u;
        tile_a[2] = &attacker;
        tile_a_count = 3;
        at.blast_width = 1.0f;
        at.accuracy_percent = 100;
        at.unit_class = 6;
        rand_value = 0;
        assert(aoe_combat_blast_ror(&attacker, 5.5f, 5.5f, 0, NULL, NULL, 0,
                                    &bops) == 1);
        assert(blast_hit_count == 1 && blast_hits[0] == &near_u &&
               !killed_count);
        assert(tiles_seen_count == 6 * 5 && tiles_seen[0][0] == 2 &&
               tiles_seen[0][1] == 2 && tiles_seen[1][0] == 3 &&
               tiles_seen[29][0] == 6 && tiles_seen[29][1] == 7);
        /* Blast level above the victims' defense: nobody. */
        at.blast_level = 1;
        blast_hit_count = 0;
        aoe_combat_blast_ror(&attacker, 5.5f, 5.5f, 0, NULL, NULL, 0, &bops);
        assert(!blast_hit_count);
        at.blast_level = 0;
        /* Excluded units and failed accuracy rolls are skipped. */
        aoe_combat_blast_ror(&attacker, 5.5f, 5.5f, 0, NULL, &near_u, 0,
                             &bops);
        assert(!blast_hit_count);
        at.accuracy_percent = 0;
        rand_value = 400; /* roll 1 */
        aoe_combat_blast_ror(&attacker, 5.5f, 5.5f, 0, NULL, NULL, 0, &bops);
        assert(!blast_hit_count);
        at.accuracy_percent = 100;
        /* Tribe: class 35 spares its own side; kills are reported. */
        tile_a[0] = &ally;
        tile_a[1] = &near_u;
        owner_a = &attacker;
        owner_b = &ally;
        at.unit_class = 0x23;
        aoe_combat_blast_ror(&attacker, 5.5f, 5.5f, 0, NULL, NULL, 1, &bops);
        assert(blast_hit_count == 1 && blast_hits[0] == &near_u &&
               killed_count == 1);
        at.unit_class = 6;
        blast_hit_count = 0;
        aoe_combat_blast_ror(&attacker, 5.5f, 5.5f, 0, NULL, NULL, 1, &bops);
        assert(blast_hit_count == 2);
        at.blast_width = 0.0f;
        assert(aoe_combat_blast_ror(&attacker, 5.5f, 5.5f, 0, NULL, NULL, 0,
                                    &bops) == 0);
    }
    {
        DatUnitAttackArmor at[3] = {{4, 6}, {3, 2}, {4, 9}};
        DatUnit t;
        AoeLiveUnit u;
        int16_t a, b;
        int deaths = 0;
        memset(&t, 0, sizeof t);
        memset(&u, 0, sizeof u);
        assert(aoe_combat_plain_damage_ror(at, 3, 0.5f) == 8.5);
        u.core.hit_points = 10.0f;
        assert(aoe_combat_plain_hit_ror(&u, 8.5, count_death, &deaths) == 1.5f && deaths == 0);
        assert(aoe_combat_plain_hit_ror(&u, 0.75, count_death, &deaths) == 0.0f && deaths == 1);
        t.armors = at;
        t.armor_count = 3;
        t.base_armor = 7;
        t.displayed_melee_armor = 2;
        t.displayed_pierce_armor = 5;
        aoe_combat_melee_armor_readout_ror(&t, &a, &b);
        assert(a == 9 && b == 2);
        t.base_armor = 12;
        aoe_combat_melee_armor_readout_ror(&t, &a, &b);
        assert(a == 12);
        aoe_combat_pierce_armor_readout_ror(&t, &a, &b);
        assert(a == 2 && b == 5);
        assert(aoe_combat_panel_pierce_armor_ror(&t) == 0);
        t.type = 70;
        assert(aoe_combat_panel_pierce_armor_ror(&t) == 2 && aoe_combat_panel_pierce_armor_ror(NULL) == 0);
    }
    {
        AoeCombatSlotOpsRor so = {NULL, s_take, s_multiplier, s_scaled, s_clear, s_setg, s_hasai,
                                  s_ainotice, s_owner};
        static DatUnit tt;
        static AoeLiveUnit a, t;
        DatUnitAttackArmor at2[2] = {{4, 1}, {3, 2}};
        uint8_t e1[8] = {0}, e2[8] = {0};
        uint8_t *entries[9] = {e1, NULL, e2};
        tt.attacks = at2;
        tt.attack_count = 2;
        tt.standing_graphic = 77;
        a.core.unit_template = &tt;
        t.core.unit_template = &tt;
        aoe_combat_attacked_by_ror(&t, &a, &so);
        assert(s_damage_count == 2 && s_mult == 1.5f);
        aoe_combat_scaled_ror(&t, 1, 2, &so);
        assert(s_scale == 1.0f);
        assert(aoe_combat_stand_ror(&t, 1, &so) && s_cleared == 1 && s_graphic == 77);
        assert(aoe_combat_stand_ror(&t, 0, &so) && s_cleared == 1);
        t.core.state = 2;
        aoe_combat_notice_ror(&t, 0, 0, 5, 0, 0, 0, &so);
        assert(s_owner_notices == 1);
        aoe_combat_notice_ror(&t, 0, 0, 699, 0, 0, 0, &so);
        assert(s_owner_notices == 1);
        s_ai = 1;
        s_ai_handles = 1;
        aoe_combat_notice_ror(&t, 0, 0, 5, 0, 0, 0, &so);
        assert(s_owner_notices == 1);
        s_ai_handles = 0;
        aoe_combat_notice_ror(&t, 0, 0, 5, 0, 0, 0, &so);
        assert(s_owner_notices == 2);
        t.core.state = 3;
        aoe_combat_notice_ror(&t, 0, 0, 5, 0, 0, 0, &so);
        assert(s_owner_notices == 2);
        aoe_combat_memory_moved_ror(entries, 4, 1.5f, 2.5f, 1.9f, 2.1f);
        assert(e1[6] == 0);
        aoe_combat_memory_moved_ror(entries, -1, 1.5f, 2.5f, 3.0f, 2.1f);
        assert(e1[6] == 0);
        aoe_combat_memory_moved_ror(entries, 4, 1.5f, 2.5f, 3.0f, 4.2f);
        assert(e1[6] == 3 && e1[7] == 4 && e2[6] == 3);
    }
    return 0;
}
