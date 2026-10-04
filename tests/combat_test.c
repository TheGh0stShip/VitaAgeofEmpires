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
static int32_t rnd(void *c) { (void)c; return rand_value; }
static void attack_notice(void *c, AoeLiveUnit *a, int32_t id) {
    (void)c; (void)a; (void)id;
    ++attack_notices;
}
static void blast(void *c, AoeLiveUnit *a, float x, float y, float z) {
    (void)c; (void)a; (void)x; (void)y; (void)z;
    ++blasts;
}
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

int main(void) {
    DatUnit at, tt;
    AoeLiveUnit attacker, target;
    DatUnitAttackArmor attacks[2] = {{4, 7}, {3, 2}};
    DatUnitAttackArmor armors[2] = {{4, 1}, {4, 2}};
    AoeCombatReceiveOps rops = {NULL, damaged, owner, credit};
    AoeCombatHitOps hops = {NULL, rnd, attack_notice, blast, multiplier,
                            receive};
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
    assert(aoe_combat_receive_tribe_ror(&target, 1.0f, attacks, 2, 1.0f,
                                        NULL, &attacker, &rops) == 0.0f &&
           notices == 1);
    attacks[0].amount = 2; /* 2 - 2 = 0 */
    target.core.hit_points = 10.0f;
    at.unit_class = 6;
    assert(aoe_combat_receive_tribe_ror(&target, 1.0f, attacks, 2, 1.0f,
                                        NULL, &attacker, &rops) == 9.0f);
    at.unit_class = 0x17;
    assert(aoe_combat_receive_tribe_ror(&target, 1.0f, attacks, 2, 1.0f,
                                        NULL, &attacker, &rops) ==
           (float)(9.0 - (double)0.1f));
    at.unit_class = 6;
    target.core.hit_points = 1.5f;
    assert(aoe_combat_receive_tribe_ror(&target, 1.0f, attacks, 2, 1.0f,
                                        NULL, &attacker, &rops) == 0.5f &&
           kills == 1);

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
    assert(aoe_combat_melee_hit_ror(&attacker, NULL, 0, 0, 0, &hops) == 0);
    assert(aoe_combat_melee_hit_ror(&attacker, &target, 0, 0, 0, NULL) == -1);

    /* Blast at (5.5, 5.5) with radius 1: tiles 2..8 (exclusive) on both
       axes in y-major order, clamped to a 7-wide map. */
    {
        AoeCombatBlastOps bops = {NULL, rnd, 7, 20, tile_units, multiplier,
                                  blast_receive, same, killed};
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
    return 0;
}
