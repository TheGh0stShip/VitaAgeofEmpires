#ifndef VITA_AOE_COMBAT_H
#define VITA_AOE_COMBAT_H

#include <stddef.h>
#include <stdint.h>

#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a combat core for type-50 units (vtable
   0x0054324c) and the tribe overrides (0x00547fa0 / 0x0054820c). */

/* 0x004269f0 (vtable +0x7c): ready when the reload timer (live +0x1b4) is
   <= 0 or NaN. 0x00426a10 resets it to template reload_time (+0x120). */
int aoe_combat_ready_ror(float reload_timer);
float aoe_combat_reload_ror(const DatUnit *attacker_template);

/* 0x00426910 (vtable +0x74): the damage `target_template` takes from the
   attack list. Each attack's armor is the last matching armor class entry
   (template +0x102/+0x104), else base_armor (+0x100); the term
   (amount - armor) * terrain_multiplier * multiplier counts only when
   positive. `terrain_multiplier` is world +0x54[terrain_defense_bonus]
   [terrain under the target] when that table index is >= 0 and the unit
   has a tile, else 1.0. */
double aoe_combat_damage_ror(const DatUnit *target_template,
                             const DatUnitAttackArmor *attacks,
                             int32_t attack_count, float terrain_multiplier,
                             float multiplier);

/* 0x00426890 (vtable +0x70): hit points drop by the damage (stored as a
   float); with an attacker the target hears +0x148(attacker, target,
   0x1f4, attacker, ftol(hp), max hit points); hit points below 1.0 become
   0. Returns the new hit points. */
typedef struct {
    void *context;
    void (*damaged_notice)(void *context, AoeLiveUnit *target,
                           AoeLiveUnit *attacker, int32_t hit_points,
                           int32_t max_hit_points);
    /* Owner player +0x4a of a unit (0 for Gaia). */
    int16_t (*owner_id)(void *context, AoeLiveUnit *unit);
    /* Tribe kill credit: the attacker's player vtable +0x78(0x2b, 1.0, 0). */
    void (*credit_kill)(void *context, void *attacker_player);
} AoeCombatReceiveOps;

float aoe_combat_receive_ror(AoeLiveUnit *target, float terrain_multiplier,
                             const DatUnitAttackArmor *attacks,
                             int32_t attack_count, float multiplier,
                             AoeLiveUnit *attacker,
                             const AoeCombatReceiveOps *ops);

/* 0x004ad9b0 (tribe vtable +0x70): only a living target (hp > 0) takes
   damage. After the base receive, a hit that removed less than 1.0 hit
   points removes exactly 1.0 instead, or (attackers of class 0, 23, 26 or
   36, or IDs 79, 69, 199, 383, 380) less than 0.1 removes 0.1. A non-Gaia
   target left below 1.0 credits the attacker's player with a kill. */
float aoe_combat_receive_tribe_ror(AoeLiveUnit *target,
                                   float terrain_multiplier,
                                   const DatUnitAttackArmor *attacks,
                                   int32_t attack_count, float multiplier,
                                   void *attacker_player,
                                   AoeLiveUnit *attacker,
                                   const AoeCombatReceiveOps *ops);

/* Attack multipliers (vtable +0xf0): base 0x00426270 is 1.0. Tribe
   0x004aec00: ranged units get 1.5 when vtable +0x250(target) (the
   caller's `ranged_bonus`) holds; melee units below their target's z get
   0.6666. */
float aoe_combat_multiplier_tribe_ror(const AoeLiveUnit *attacker,
                                      const AoeLiveUnit *target,
                                      int ranged_bonus);

/* Projectile launch point shared by 0x00426520 and 0x004266a0: the
   template offsets (+0x12c forward, +0x130 sideways) rotated by the
   facing, z offset +0x134; and the launch radius footprint + max range +
   6.0 passed to the projectile. Host libm cos/sin may differ from x87 in
   the last bit. */
void aoe_combat_projectile_origin_ror(const AoeLiveUnit *attacker,
                                      float facing, float *x, float *y,
                                      float *z, float *radius);

typedef struct {
    void *context;
    int32_t (*rand)(void *context);                           /* 0x0052605d */
    /* Attacker +0x148(self, self, 0x200, 0x258, target ID, 0). */
    void (*attack_notice)(void *context, AoeLiveUnit *attacker,
                          int32_t target_id);
    /* Vtable +0x220(x, y, z, attacker, 0): blast damage. */
    void (*blast)(void *context, AoeLiveUnit *attacker, float x, float y,
                  float z);
    float (*multiplier)(void *context, AoeLiveUnit *attacker,
                        AoeLiveUnit *target);                 /* +0xf0 */
    /* Target vtable +0x70(attacks, count, multiplier, attacker player,
       attacker). */
    void (*receive)(void *context, AoeLiveUnit *target,
                    const DatUnitAttackArmor *attacks, int32_t count,
                    float multiplier, AoeLiveUnit *attacker);
} AoeCombatHitOps;

/* 0x004267b0 (vtable +0x22c, reached from the fire method with the
   attacker's own position): a blast template (+0x118 > 0) hands over to
   the blast; otherwise notice 0x200 and roll rand() * 100 / 32767 against
   accuracy (+0x126), delivering the attack list on success. Returns 1 when
   it hit, 0 for a miss or blast, -1 for missing dependencies. */
int aoe_combat_melee_hit_ror(AoeLiveUnit *attacker, AoeLiveUnit *target,
                             float x, float y, float z,
                             const AoeCombatHitOps *ops);

typedef struct {
    void *context;
    int32_t (*rand)(void *context);                           /* 0x0052605d */
    /* Map width/height (map +0x08/+0x0c) and the units on tile (x, y) in
       its list order (map +0x8d8c rows, tile +0x10); returns the count
       written. */
    int32_t map_width, map_height;
    size_t (*tile_units)(void *context, int32_t x, int32_t y,
                         AoeLiveUnit **out, size_t capacity);
    float (*multiplier)(void *context, AoeLiveUnit *attacker,
                        AoeLiveUnit *target);                 /* +0xf0 */
    /* Target vtable +0x70(attacks, count, multiplier, attacker player,
       `source`). */
    void (*receive)(void *context, AoeLiveUnit *target,
                    const DatUnitAttackArmor *attacks, int32_t count,
                    float multiplier, AoeLiveUnit *source);
    int (*same_owner)(void *context, AoeLiveUnit *a, AoeLiveUnit *b);
    /* Tribe only: a victim left below 1.0 hit points hears +0x84(0, 0, 1). */
    void (*killed)(void *context, AoeLiveUnit *victim);
} AoeCombatBlastOps;

/* Blast damage, vtable +0x220: base 0x00426280 and tribe 0x004b04a0, at
   (x, y, z) for `attacker` with `source` passed on to the receivers and
   `excluded` skipped. Radius r = template +0x118 (nothing for r <= 0).
   Tiles ftol(x - r) - 2 .. ftol(x + r) + 2 (end exclusive, clamped to the
   map; rows by y) are scanned; a unit takes the attack list when it is not
   the attacker or `excluded`, its blast defense (+0x8b) is at least the
   attacker's blast level (+0x11c), the squared distance from the point to
   its footprint edge is below r^2, and rand() * 100 / 32767 <= accuracy.
   The tribe version also spares same-owner units for attacker classes 35
   and 24 and reports kills. Returns 1, 0 when the radius is not positive,
   -1 for missing dependencies. */
int aoe_combat_blast_ror(AoeLiveUnit *attacker, float x, float y, float z,
                         AoeLiveUnit *source, AoeLiveUnit *excluded,
                         int tribe, const AoeCombatBlastOps *ops);

#endif
