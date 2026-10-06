#include "combat.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

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

void aoe_combat_melee_armor_readout_ror(const DatUnit *t, int16_t *armor, int16_t *displayed) {
    uint16_t i;
    *armor = t->base_armor;
    for (i = 0; i < t->armor_count; ++i)
        if (t->armors[i].class_id == 4 && *armor < t->armors[i].amount) *armor = t->armors[i].amount;
    *displayed = t->displayed_melee_armor;
}

void aoe_combat_pierce_armor_readout_ror(const DatUnit *t, int16_t *armor, int16_t *displayed) {
    uint16_t i;
    *armor = 0;
    for (i = 0; i < t->armor_count; ++i)
        if (t->armors[i].class_id == 3 && *armor < t->armors[i].amount) *armor = t->armors[i].amount;
    *displayed = t->displayed_pierce_armor;
}

int16_t aoe_combat_panel_pierce_armor_ror(const DatUnit *t) {
    int16_t amount = 0;
    uint16_t i;
    if (!t || t->type != 70) return 0;
    for (i = 0; i < t->armor_count; ++i)
        if (t->armors[i].class_id == 3) amount = t->armors[i].amount;
    return amount;
}

void aoe_combat_attacked_by_ror(AoeLiveUnit *target, AoeLiveUnit *attacker,
                                const AoeCombatSlotOpsRor *ops) {
    const DatUnit *t = attacker->core.unit_template;
    float m = ops->multiplier(ops->context, attacker, target);
    ops->take_damage(ops->context, target, (int16_t)t->attack_count, t->attacks, m, attacker);
}

void aoe_combat_scaled_ror(AoeLiveUnit *unit, int32_t a, int32_t b, const AoeCombatSlotOpsRor *ops) {
    ops->scaled(ops->context, unit, a, 1.0f, b);
}

int aoe_combat_stand_ror(AoeLiveUnit *unit, int clear, const AoeCombatSlotOpsRor *ops) {
    if (clear) ops->clear_actions(ops->context, unit);
    ops->set_graphic(ops->context, unit, unit->core.unit_template->standing_graphic);
    return 1;
}

void aoe_combat_notice_ror(AoeLiveUnit *unit, int32_t a, int32_t b, int32_t code, int32_t c,
                           int32_t d, int32_t e, const AoeCombatSlotOpsRor *ops) {
    if (unit->core.state != 2) return;
    if (ops->has_ai(ops->context, unit) &&
        ops->ai_notice(ops->context, unit, a, b, code, c, d, e))
        return;
    if (code == 699) return;
    ops->owner_notice(ops->context, unit, a, b, code, c, d, e);
}

int aoe_combat_order_ror(AoeLiveUnit *unit, AoeCombatOrderKindRor kind, int32_t id, float x,
                         float y, float scale, int force, const AoeCombatOrderOpsRor *ops) {
    AoeLiveUnit *target = NULL;
    const DatUnitTask *task = NULL;
    const DatUnit *t;
    void *head, *action;
    float z;
    int16_t graphic = -1;
    if (!unit || !ops || !ops->head || !ops->action_type || !ops->build || !ops->adopt) return 0;
    z = unit->core.position.z40;
    if (kind != AOE_COMBAT_ORDER_EXPLORE_ROR) {
        if (!ops->resolve || !(target = ops->resolve(ops->context, id))) return 0;
        x = target->core.position.x38;
        y = target->core.position.y3c;
        z = target->core.position.z40;
    }
    head = ops->head(ops->context, unit);
    if (kind == AOE_COMBAT_ORDER_ATTACK_ROR) {
        int same = head && ops->action_target && ops->action_target(ops->context, head) == target;
        int16_t ht = head ? ops->action_type(ops->context, head) : -1;
        if (!force && same && (ht == 9 || ht == 7)) return 1;
        if (same && ht == 9) {
            if (ops->retarget) ops->retarget(ops->context, head, target, x, y, z);
            return 1;
        }
        if ((t = unit->core.unit_template) != NULL && t->unit_class == 4 && ops->owner_template &&
            ops->change_template) {
            const DatUnit *base = ops->owner_template(ops->context, unit, 83);
            if (base) ops->change_template(ops->context, unit, base);
        }
    }
    if (!force && head && ops->action_type(ops->context, head) == (int16_t)kind &&
        (kind == AOE_COMBAT_ORDER_EXPLORE_ROR ||
         (ops->action_target && ops->action_target(ops->context, head) == target)))
        return 1;
    if ((kind == AOE_COMBAT_ORDER_GATHER_ROR || kind == AOE_COMBAT_ORDER_EXPLORE_ROR) && head &&
        ops->action_type(ops->context, head) == (int16_t)kind) {
        if (ops->retarget) ops->retarget(ops->context, head, target, x, y, z);
        return 1;
    }
    if (kind == AOE_COMBAT_ORDER_GATHER_ROR || kind == AOE_COMBAT_ORDER_ENTER_ROR) {
        if (!ops->task || !(task = ops->task(ops->context, unit, target, x, y))) return 0;
    }
    if (kind == AOE_COMBAT_ORDER_MOVE_ROR && (t = unit->core.unit_template) != NULL)
        graphic = t->walking_graphic2 >= 0 ? t->walking_graphic2 : t->walking_graphic;
    action = ops->build(ops->context, unit, kind, task, target, x, y, z, scale, graphic);
    if (!action) return 0;
    if (ops->clear_actions) ops->clear_actions(ops->context, unit);
    ops->adopt(ops->context, unit, action);
    return 1;
}

int aoe_combat_order_point_ror(AoeLiveUnit *unit, float x, float y, float z, int force,
                               const AoeCombatOrderOpsRor *ops) {
    void *head, *action;
    if (!unit || !ops || !ops->head || !ops->action_type || !ops->build || !ops->adopt) return 0;
    head = ops->head(ops->context, unit);
    if (!force && head && ops->action_point) {
        float hx, hy, hz;
        ops->action_point(ops->context, head, &hx, &hy, &hz);
        if (hx == x && hy == y && hz == z) {
            int16_t ht = ops->action_type(ops->context, head);
            if (ht == 9 || ht == 7) return 1;
        }
    }
    action = ops->build(ops->context, unit, AOE_COMBAT_ORDER_ATTACK_POINT_ROR, NULL, NULL, x, y, z,
                        0.0f, -1);
    if (!action) return 0;
    if (ops->clear_actions) ops->clear_actions(ops->context, unit);
    ops->adopt(ops->context, unit, action);
    return 1;
}

void aoe_combat_memory_moved_ror(uint8_t *entries[9], int32_t id, float old_x, float old_y,
                                 float new_x, float new_y) {
    int32_t ox = (int32_t)old_x, oy = (int32_t)old_y, nx, ny;
    int i;
    if (id < 0) return;
    nx = (int32_t)new_x;
    ny = (int32_t)new_y;
    if (nx == ox && ny == oy) return;
    for (i = 0; i < 9; ++i)
        if (entries[i]) {
            entries[i][6] = (uint8_t)nx;
            entries[i][7] = (uint8_t)ny;
        }
}

int32_t aoe_combat_attack_total_ror(const DatUnit *attacker) {
    double sum = 0.0;
    int i;
    if (attacker)
        for (i = 0; i < attacker->attack_count; ++i)
            if ((double)attacker->attacks[i].amount > 0.0) sum += attacker->attacks[i].amount;
    if (sum < 1.0) sum = 1.0;
    return aoe_ftol32(sum);
}

int32_t aoe_combat_template_damage_ror(const DatUnit *target, const DatUnit *attacker) {
    double sum = 0.0;
    int i, j;
    if (target && attacker) {
        for (i = 0; i < attacker->attack_count; ++i) {
            int32_t armor = target->base_armor;
            double d;
            for (j = 0; j < target->armor_count; ++j)
                if (target->armors[j].class_id == attacker->attacks[i].class_id)
                    armor = target->armors[j].amount;
            d = (double)attacker->attacks[i].amount - armor;
            if (d > 0.0) sum += d;
        }
    }
    if (sum < 1.0) sum = 1.0;
    return aoe_ftol32(sum);
}

uint8_t aoe_combat_type50_init_ror(AoeCombatType50Ror *unit,
                                   const DatUnit *unit_template, int reinit) {
    if (!unit) return 0;
    memset(unit->memory, 0, sizeof(unit->memory));
    unit->memory_count = 0;
    unit->reload_timer = unit_template ? unit_template->reload_time : 0.0f;
    if (reinit) return 0;
    unit->field_1b8 = 0;
    return 0x32;
}

void aoe_combat_type50_destroy_ror(const AoeCombatType50Ror *unit,
                                   int32_t unit_id, int16_t owner, float x,
                                   float y, int16_t template_word_14,
                                   AoeCombatType50RecordFn record,
                                   void *context) {
    if (!unit || unit_id < 0 || !record) return;
    record(context, unit_id, owner, (int32_t)x, (int32_t)y, template_word_14,
           unit->memory_count, unit->memory);
}

int aoe_combat_high_ground_ror(float from_x, float from_y, float from_z,
                               float to_x, float to_y, float to_z,
                               AoeCombatHighGroundProbeFn probe,
                               void *context) {
    float dx, dy, step_x, step_y, x, y, best = -1.0f;
    int32_t steps, ox = 0, oy = 0, prev_x = -999, prev_y = -999, count = 0;
    if (!(from_z <= to_z)) return 1;
    dx = to_x - from_x;
    dy = to_y - from_y;
    if (dx == 0.0f && dy == 0.0f) return 0;
    steps = aoe_ftol32(fabsf(dx) < fabsf(dy) ? dy : dx);
    steps = (int32_t)(2u * (uint32_t)(steps < 0 ? -(uint32_t)steps : (uint32_t)steps));
    step_x = (float)((double)dx / (double)steps);
    step_y = (float)((double)dy / (double)steps);
    if (fabsf(step_x) <= fabsf(step_y)) ox = 1; else oy = 1;
    if (!probe) return 0;
    x = from_x;
    y = from_y;
    for (int32_t i = steps; i > 0; --i) {
        int32_t tx, ty;
        x += step_x;
        y += step_y;
        tx = aoe_ftol32(x);
        ty = aoe_ftol32(y);
        if (tx == prev_x && ty == prev_y) continue;
        prev_x = tx;
        prev_y = ty;
        if (probe(context, tx, ty, from_x, from_y, to_x, to_y, &best, &count))
            continue;
        if (probe(context, tx - ox, ty - oy, from_x, from_y, to_x, to_y,
                  &best, &count))
            continue;
        probe(context, tx + ox, ty + oy, from_x, from_y, to_x, to_y, &best,
              &count);
    }
    return count > 0;
}
