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

float aoe_combat_receive_building_ror(AoeLiveUnit *target,
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

float aoe_combat_receive_unit_ror(AoeLiveUnit *target,
                                  float terrain_multiplier,
                                  const DatUnitAttackArmor *attacks,
                                  int32_t count, float multiplier,
                                  void *attacker_player,
                                  AoeLiveUnit *attacker,
                                  const AoeCombatReceiveOps *ops) {
    float old;
    if (!target || !attacker || !attacker->core.unit_template || !ops ||
        !ops->spares_class || !ops->clear_carried || !ops->owner_id ||
        !ops->credit_kill_attribute || !ops->killed_notice)
        return target ? target->core.hit_points : 0.0f;
    old = target->core.hit_points;
    if (!(old >= 1.0f)) return old;
    aoe_combat_receive_ror(target, terrain_multiplier, attacks, count,
                           multiplier, attacker, ops);
    if (!(target->core.hit_points >= 1.0f) &&
        !ops->spares_class(ops->context, target,
                           attacker->core.unit_template->unit_class))
        ops->clear_carried(ops->context, target);
    if (!((double)old - target->core.hit_points >= 1.0))
        target->core.hit_points = (float)((double)old - 1.0);
    if (ops->owner_id(ops->context, target) > 0 &&
        !(target->core.hit_points >= 1.0f)) {
        ops->credit_kill_attribute(ops->context, attacker_player, 0x14);
        ops->killed_notice(ops->context, target, attacker);
    }
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
        ops->blast(ops->context, attacker, x, y, z, attacker, NULL);
        return 0;
    }
    ops->attack_notice(ops->context, attacker, target->core.unit_id);
    roll = ops->rand(ops->context) * 100 / 32767;
    if (roll > t->accuracy_percent) return 0;
    ops->receive(ops->context, target, t->attacks, t->attack_count,
                 ops->multiplier(ops->context, attacker, target), attacker);
    return 1;
}

enum { BLAST_TILE_CAPACITY = 256 };

int aoe_combat_blast_ror(AoeLiveUnit *attacker, float x, float y, float z,
                         AoeLiveUnit *source, AoeLiveUnit *excluded,
                         int tribe, const AoeCombatBlastOps *ops) {
    const DatUnit *t;
    double r;
    float r2;
    int32_t x_lo, x_hi, y_lo, y_hi;
    (void)z;
    if (!attacker || !attacker->core.unit_template || !ops || !ops->rand ||
        !ops->tile_units || !ops->multiplier || !ops->receive ||
        (tribe && (!ops->same_owner || !ops->clear_carried)))
        return -1;
    t = attacker->core.unit_template;
    r = t->blast_width;
    r2 = (float)(r * r);
    if (!(r > 0.0)) return 0;
    /* 16-bit tile indexes in the original. */
    x_lo = (int16_t)(aoe_ftol32((double)x - r) - 2);
    x_hi = (int16_t)(aoe_ftol32(r + x) + 2);
    y_lo = (int16_t)(aoe_ftol32((double)y - r) - 2);
    y_hi = (int16_t)(aoe_ftol32(r + y) + 2);
    if (x_lo < 0) x_lo = 0;
    if (x_hi > ops->map_width) x_hi = ops->map_width;
    if (y_lo < 0) y_lo = 0;
    if (y_hi > ops->map_height) y_hi = ops->map_height;
    for (int32_t ty = y_lo; ty < y_hi; ++ty)
        for (int32_t tx = x_lo; tx < x_hi; ++tx) {
            AoeLiveUnit *units[BLAST_TILE_CAPACITY];
            size_t n = ops->tile_units(ops->context, tx, ty, units,
                                       BLAST_TILE_CAPACITY);
            for (size_t i = 0; i < n && i < BLAST_TILE_CAPACITY; ++i) {
                AoeLiveUnit *v = units[i];
                const DatUnit *vt = v ? v->core.unit_template : NULL;
                double dx, ex, ey;
                float dyf;
                if (!vt) continue;
                if (tribe) {
                    if (v == excluded) continue;
                    if ((t->unit_class == 0x23 || t->unit_class == 0x18) &&
                        ops->same_owner(ops->context, v, attacker))
                        continue;
                    if (v == attacker) continue;
                    if (vt->blast_defense_level < t->blast_level) continue;
                } else {
                    if (v == attacker) continue;
                    if (vt->blast_defense_level < t->blast_level) continue;
                    if (v == excluded) continue;
                }
                dx = (double)v->core.position.x38 - x;
                dyf = (float)((double)v->core.position.y3c - y);
                if (dx < 0.0 || isnan(dx)) dx = -dx;
                if (dyf < 0.0f || isnan(dyf)) dyf = -dyf;
                ex = dx > vt->size[0] ? dx - vt->size[0] : 0.0;
                ey = dyf > vt->size[1] ? (double)dyf - vt->size[1] : 0.0;
                if (!(r2 > ey * ey + ex * ex)) continue;
                if ((int16_t)(ops->rand(ops->context) * 100 / 32767) >
                    t->accuracy_percent)
                    continue;
                ops->receive(ops->context, v, t->attacks, t->attack_count,
                             ops->multiplier(ops->context, attacker, v),
                             source);
                if (tribe && !(v->core.hit_points >= 1.0f))
                    ops->clear_carried(ops->context, v);
            }
        }
    return 1;
}

/* Shared prologue of 0x004aadf0 / 0x004aaeb0: ex (float) and ey. */
static int edges(const AoeLiveUnit *u, const AoeLiveUnit *t, float *ex,
                 double *ey) {
    const DatUnit *ut, *tt;
    float dxf;
    double dy;
    if (!u || !t || !(ut = u->core.unit_template) ||
        !(tt = t->core.unit_template))
        return 0;
    dxf = (float)((double)t->core.position.x38 - u->core.position.x38);
    dy = (double)t->core.position.y3c - u->core.position.y3c;
    if (dxf < 0.0f || isnan(dxf)) dxf = -dxf;
    if (dy < 0.0 || isnan(dy)) dy = -dy;
    *ex = (float)(((double)dxf - ut->size[0]) - tt->size[0]);
    *ey = (dy - ut->size[1]) - tt->size[1];
    if (*ex < 0.0f || isnan(*ex)) *ex = 0.0f;
    if (*ey < 0.0 || isnan(*ey)) *ey = 0.0;
    return 1;
}

double aoe_combat_distance_ror(const AoeLiveUnit *u, const AoeLiveUnit *t) {
    float ex;
    double ey;
    if (!edges(u, t, &ex, &ey)) return 0.0;
    return sqrt(ey * ey + (double)ex * ex);
}

int aoe_combat_in_range_ror(const AoeLiveUnit *u, const AoeLiveUnit *t,
                            float range) {
    float ex;
    double ey;
    if (!edges(u, t, &ex, &ey)) return 0;
    return (double)range * range > ey * ey + (double)ex * ex;
}

double aoe_combat_point_distance_ror(const AoeLiveUnit *u, float x,
                                     float y) {
    const DatUnit *ut;
    float dxf, ex;
    double dy, ey;
    if (!u || !(ut = u->core.unit_template)) return 0.0;
    dxf = (float)((double)u->core.position.x38 - x);
    dy = (double)u->core.position.y3c - y;
    if (dxf < 0.0f || isnan(dxf)) dxf = -dxf;
    if (dy < 0.0 || isnan(dy)) dy = -dy;
    ex = (float)((double)dxf - ut->size[0]);
    ey = dy - ut->size[1];
    if (ex < 0.0f || isnan(ex)) ex = 0.0f;
    if (ey < 0.0 || isnan(ey)) ey = 0.0;
    return sqrt(ey * ey + (double)ex * ex);
}

int aoe_combat_melee_hit_tribe_ror(AoeLiveUnit *attacker, AoeLiveUnit *target,
                                   float x, float y, float z,
                                   const AoeCombatHitOps *ops) {
    const DatUnit *t;
    if (!attacker || !attacker->core.unit_template || !ops || !ops->rand ||
        !ops->attack_notice || !ops->blast || !ops->multiplier ||
        !ops->receive || (ops->instant_kill_option && !ops->destroy))
        return -1;
    if (!target) return 0;
    t = attacker->core.unit_template;
    if (ops->instant_kill_option && (t->id == 0x7a || t->id == 0x7b)) {
        ops->attack_notice(ops->context, attacker, target->core.unit_id);
        ops->destroy(ops->context, target);
        return 1;
    }
    if (t->blast_width > 0.0f && !(t->max_range >= 0.99f)) {
        ops->blast(ops->context, attacker, x, y, z, attacker, target);
        ops->attack_notice(ops->context, attacker, target->core.unit_id);
        if (ops->rand(ops->context) * 100 / 32767 > t->accuracy_percent)
            return 0;
        ops->receive(ops->context, target, t->attacks, t->attack_count,
                     ops->multiplier(ops->context, attacker, target),
                     attacker);
        return 1;
    }
    return aoe_combat_melee_hit_ror(attacker, target, x, y, z, ops);
}

double aoe_combat_plain_damage_ror(const DatUnitAttackArmor *attacks, int32_t attack_count,
                                   float multiplier) {
    double total = 0.0;
    int32_t i;
    for (i = 0; i < attack_count; ++i) total = (double)attacks[i].amount * multiplier + total;
    return total;
}

float aoe_combat_plain_hit_ror(AoeLiveUnit *target, double damage,
                               void (*die)(void *context, AoeLiveUnit *unit), void *context) {
    double hp = (double)target->core.hit_points - damage;
    target->core.hit_points = (float)hp;
    if ((double)target->core.hit_points < 1.0) {
        target->core.hit_points = 0.0f;
        die(context, target);
    }
    return target->core.hit_points;
}

void aoe_combat_melee_readout_ror(const DatUnit *t, int16_t *attack, int16_t *armor) {
    uint16_t i;
    *attack = t->base_armor;
    for (i = 0; i < t->attack_count; ++i)
        if (t->attacks[i].class_id == 4 && *attack < t->attacks[i].amount) *attack = t->attacks[i].amount;
    *armor = t->displayed_melee_armor;
}

void aoe_combat_pierce_readout_ror(const DatUnit *t, int16_t *attack, int16_t *armor) {
    uint16_t i;
    *attack = 0;
    for (i = 0; i < t->attack_count; ++i)
        if (t->attacks[i].class_id == 3 && *attack < t->attacks[i].amount) *attack = t->attacks[i].amount;
    *armor = t->displayed_pierce_armor;
}

int16_t aoe_combat_panel_pierce_ror(const DatUnit *t) {
    int16_t amount = 0;
    uint16_t i;
    if (!t || t->type != 70) return 0;
    for (i = 0; i < t->attack_count; ++i)
        if (t->attacks[i].class_id == 3) amount = t->attacks[i].amount;
    return amount;
}
