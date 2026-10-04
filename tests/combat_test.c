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
    return 0;
}
