#include "combat.h"

#include <math.h>
#include <stddef.h>

#include "aoe_ftol.h"

int aoe_combat_ready_ror(float reload_timer) {
    return !(reload_timer > 0.0f);
}

float aoe_combat_reload_ror(const DatUnit *t) {
    return t ? t->reload_time : 0.0f;
}

double aoe_combat_damage_ror(const DatUnit *t,
                             const DatUnitAttackArmor *attacks,
                             int32_t count, float terrain_multiplier,
                             float multiplier) {
    double total = 0.0;
    if (!t || !attacks || count <= 0) return 0.0;
    for (int32_t i = 0; i < count; ++i) {
        int32_t armor = t->base_armor;
        double dmg;
        for (uint16_t j = 0; j < t->armor_count && t->armors; ++j)
            if (t->armors[j].class_id == attacks[i].class_id)
                armor = t->armors[j].amount;
        dmg = ((double)attacks[i].amount - armor) * terrain_multiplier *
              multiplier;
        if (dmg > 0.0) total += dmg;
    }
    return total;
}

float aoe_combat_receive_ror(AoeLiveUnit *target, float terrain_multiplier,
                             const DatUnitAttackArmor *attacks,
                             int32_t count, float multiplier,
                             AoeLiveUnit *attacker,
                             const AoeCombatReceiveOps *ops) {
    const DatUnit *t;
    double hp;
    if (!target || !target->core.unit_template) return 0.0f;
    t = target->core.unit_template;
    hp = (double)target->core.hit_points -
         aoe_combat_damage_ror(t, attacks, count, terrain_multiplier,
                               multiplier);
    target->core.hit_points = (float)hp;
    if (attacker && ops && ops->damaged_notice)
        ops->damaged_notice(ops->context, target, attacker, aoe_ftol32(hp),
                            t->hit_points);
    /* fcomp 1.0; test ah,1: below 1.0 or NaN. */
    if (!(target->core.hit_points >= 1.0f)) target->core.hit_points = 0.0f;
    return target->core.hit_points;
}

static int light_attacker(const AoeLiveUnit *attacker) {
    const DatUnit *t = attacker ? attacker->core.unit_template : NULL;
    int16_t c, id;
    if (!t) return 0;
    c = t->unit_class;
    id = t->id;
    return c == 0 || c == 0x17 || c == 0x24 || c == 0x1a || id == 0x4f ||
           id == 0x45 || id == 0xc7 || id == 0x17f || id == 0x17c;
}

float aoe_combat_receive_tribe_ror(AoeLiveUnit *target,
                                   float terrain_multiplier,
                                   const DatUnitAttackArmor *attacks,
                                   int32_t count, float multiplier,
                                   void *attacker_player,
                                   AoeLiveUnit *attacker,
                                   const AoeCombatReceiveOps *ops) {
    float old;
    if (!target) return 0.0f;
    old = target->core.hit_points;
    if (!(old > 0.0f)) return old;
    aoe_combat_receive_ror(target, terrain_multiplier, attacks, count,
                           multiplier, attacker, ops);
    if (light_attacker(attacker)) {
        if (!((double)old - target->core.hit_points >= 0.1))
            target->core.hit_points = (float)((double)old - (double)0.1f);
    } else if (!((double)old - target->core.hit_points >= 1.0)) {
        target->core.hit_points = (float)((double)old - 1.0);
    }
    if (ops && ops->owner_id && ops->owner_id(ops->context, target) > 0 &&
        !(target->core.hit_points >= 1.0f) && ops->credit_kill)
        ops->credit_kill(ops->context, attacker_player);
    return target->core.hit_points;
}

float aoe_combat_multiplier_tribe_ror(const AoeLiveUnit *attacker,
                                      const AoeLiveUnit *target,
                                      int ranged_bonus) {
    if (!attacker || !attacker->core.unit_template || !target) return 1.0f;
    if (attacker->core.unit_template->projectile_unit_id >= 0)
        return ranged_bonus ? 1.5f : 1.0f;
    if (!(attacker->core.position.z40 >= target->core.position.z40))
        return 0.6666f;
    return 1.0f;
}

void aoe_combat_projectile_origin_ror(const AoeLiveUnit *a, float facing,
                                      float *x, float *y, float *z,
                                      float *radius) {
    const DatUnit *t;
    double c, s;
    if (!a || !a->core.unit_template || !x || !y || !z || !radius) return;
    t = a->core.unit_template;
    c = cos((double)facing);
    s = sin((double)facing);
    *x = (float)(-s * t->projectile_offset[0] +
                 c * t->projectile_offset[1] + a->core.position.x38);
    *y = (float)(s * t->projectile_offset[1] +
                 c * t->projectile_offset[0] + a->core.position.y3c);
    *z = (float)((double)t->projectile_offset[2] + a->core.position.z40);
    *radius = (float)(sqrt((double)t->size[0] * t->size[0] +
                           (double)t->size[1] * t->size[1]) +
                      t->max_range + 6.0);
}

int aoe_combat_melee_hit_ror(AoeLiveUnit *attacker, AoeLiveUnit *target,
                             float x, float y, float z,
                             const AoeCombatHitOps *ops) {
    const DatUnit *t;
    int32_t roll;
    if (!attacker || !attacker->core.unit_template || !ops || !ops->rand ||
        !ops->attack_notice || !ops->blast || !ops->multiplier ||
        !ops->receive)
        return -1;
    if (!target) return 0;
    t = attacker->core.unit_template;
    if (t->blast_width > 0.0f) {
        ops->blast(ops->context, attacker, x, y, z);
        return 0;
    }
    ops->attack_notice(ops->context, attacker, target->core.unit_id);
    roll = ops->rand(ops->context) * 100 / 32767;
    if (roll > t->accuracy_percent) return 0;
    ops->receive(ops->context, target, t->attacks, t->attack_count,
                 ops->multiplier(ops->context, attacker, target), attacker);
    return 1;
}
