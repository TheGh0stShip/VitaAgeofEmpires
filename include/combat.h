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

/* Template vtable +0x28: the damage `attacker`'s attacks would do.
   0x00443680 (type 20): its positive attack amounts summed; 0x0043ff10
   (type 50, against this template): each attack less this template's
   armor for its class (the last matching entry, else the base armor
   +0x100), positive terms summed. Both at least 1.0, truncated. */
int32_t aoe_combat_attack_total_ror(const DatUnit *attacker);
int32_t aoe_combat_template_damage_ror(const DatUnit *target, const DatUnit *attacker);

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
    /* Kill credit: the attacker's player vtable +0x78(attribute, 1.0, 0),
       attribute 0x2b for buildings and 0x14 for type-70 units. */
    void (*credit_kill)(void *context, void *attacker_player);
    /* Type 70 only: victim vtable +0x13c(attacker class) (nonzero keeps
       the carried resource, e.g. a hunter's carcass), +0x84(0, 0, 1) =
       0x004aace0 setting the carried amount (+0x44) to 0 within template
       +0x7c, and the victim player's +0xe8(victim, -1, 0x201, attacker,
       -1, -1) notice. */
    int (*spares_class)(void *context, AoeLiveUnit *victim, int16_t cls);
    void (*clear_carried)(void *context, AoeLiveUnit *victim);
    void (*killed_notice)(void *context, AoeLiveUnit *victim,
                          AoeLiveUnit *attacker);
    int (*credit_kill_attribute)(void *context, void *attacker_player,
                                 int16_t attribute);
} AoeCombatReceiveOps;

float aoe_combat_receive_ror(AoeLiveUnit *target, float terrain_multiplier,
                             const DatUnitAttackArmor *attacks,
                             int32_t attack_count, float multiplier,
                             AoeLiveUnit *attacker,
                             const AoeCombatReceiveOps *ops);

/* 0x004ad9b0 (type-80 building vtable 0x00547fa0 +0x70; earlier notes
   called it the tribe receive): only a living target (hp > 0) takes
   damage. After the base receive, a hit that removed less than 1.0 hit
   points removes exactly 1.0 instead, or (attackers of class 0, 23, 26 or
   36, or IDs 79, 69, 199, 383, 380) less than 0.1 removes 0.1. A non-Gaia
   target left below 1.0 credits the attacker's player with a kill. */
float aoe_combat_receive_building_ror(AoeLiveUnit *target,
                                   float terrain_multiplier,
                                   const DatUnitAttackArmor *attacks,
                                   int32_t attack_count, float multiplier,
                                   void *attacker_player,
                                   AoeLiveUnit *attacker,
                                   const AoeCombatReceiveOps *ops);

/* 0x004ae9d0 (type-70 vtable 0x0054820c +0x70): a target already below
   1.0 takes nothing. After the base receive, a hit that leaves it below
   1.0 empties its carried resource (+0x84(0, 0, 1)) unless +0x13c keeps
   it for the attacker's class (dying itself follows from the state
   switch of 0x004a8f40); any hit removes at least 1.0; a non-Gaia victim left below 1.0
   credits the attacker's player (+0x78(0x14, 1.0, 0)) and notices 0x201.
   The attacker must be non-NULL. */
float aoe_combat_receive_unit_ror(AoeLiveUnit *target,
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

/* Per-tile probe of the high-ground walk: vtable +0x24c(target, tile x,
   tile y, from x, from y, to x, to y, map (owner +0x3c +0x28), 0, &best,
   &count); non-zero ends that step's probes. `best` starts at -1.0 and
   `count` at 0; both belong to the probe. */
typedef int (*AoeCombatHighGroundProbeFn)(void *context, int32_t tile_x,
                                          int32_t tile_y, float from_x,
                                          float from_y, float to_x,
                                          float to_y, float *best,
                                          int32_t *count);
/* 0x004b01c0 (vtables 0x00547fa0 / 0x0054820c +0x250, the `ranged_bonus`
   above): 1 when the attacker's z (+0x40) is above the target's. Else 0
   at the same (x, y); otherwise a walk of n = 2 * |ftol(larger-magnitude
   delta)| float steps (delta / n, from the attacker's x/y) probes each
   newly entered tile (ftol of both), then, while the probes return 0, its
   neighbour at -1 and +1 across the walk (x for a mostly-y walk, which
   includes |dx| == |dy|; else y). True when the count ends positive. */
int aoe_combat_high_ground_ror(float from_x, float from_y, float from_z,
                               float to_x, float to_y, float to_z,
                               AoeCombatHighGroundProbeFn probe,
                               void *context);

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
    /* Vtable +0x220(x, y, z, source, excluded): blast damage. */
    void (*blast)(void *context, AoeLiveUnit *attacker, float x, float y,
                  float z, AoeLiveUnit *source, AoeLiveUnit *excluded);
    float (*multiplier)(void *context, AoeLiveUnit *attacker,
                        AoeLiveUnit *target);                 /* +0xf0 */
    /* Target vtable +0x70(attacks, count, multiplier, attacker player,
       attacker). */
    void (*receive)(void *context, AoeLiveUnit *target,
                    const DatUnitAttackArmor *attacks, int32_t count,
                    float multiplier, AoeLiveUnit *attacker);
    /* Tribe only: game option global 0x00580e38 +0x9a8, and target vtable
       +0x6c (immediate removal). */
    int instant_kill_option;
    void (*destroy)(void *context, AoeLiveUnit *target);
} AoeCombatHitOps;

/* 0x004267b0 (vtable +0x22c, reached from the fire method with the
   attacker's own position): a blast template (+0x118 > 0) hands over to
   the blast; otherwise notice 0x200 and roll rand() * 100 / 32767 against
   accuracy (+0x126), delivering the attack list on success. Returns 1 when
   it hit, 0 for a miss or blast, -1 for missing dependencies. */
int aoe_combat_melee_hit_ror(AoeLiveUnit *attacker, AoeLiveUnit *target,
                             float x, float y, float z,
                             const AoeCombatHitOps *ops);

/* Tribe vtable +0x22c = 0x004aeca0. With the option set, template IDs 122
   and 123 notice 0x200 and remove the target outright (returns 1). Blast
   units with max range below 0.99 splash (+0x220 at the point, excluding
   the target) and then roll for a direct hit on the target. Everything
   else takes the base hit 0x004267b0. */
int aoe_combat_melee_hit_tribe_ror(AoeLiveUnit *attacker, AoeLiveUnit *target,
                                   float x, float y, float z,
                                   const AoeCombatHitOps *ops);

/* Edge distances between footprints (template +0x30/+0x34), per axis
   |delta| minus the sizes and clamped at 0 (the x term is rounded to
   float):
   - 0x004aaeb0: unit to target, sqrt(ey^2 + ex^2);
   - 0x004aadf0: that edge vector is strictly within `range`
     (range * range > ey^2 + ex^2);
   - 0x004aaf50(x, y, z): unit to a point, subtracting only the unit's
     size (z is unused). */
double aoe_combat_distance_ror(const AoeLiveUnit *unit,
                               const AoeLiveUnit *target);
int aoe_combat_in_range_ror(const AoeLiveUnit *unit,
                            const AoeLiveUnit *target, float range);
double aoe_combat_point_distance_ror(const AoeLiveUnit *unit, float x,
                                     float y);

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
    /* Tribe only: a victim left below 1.0 hit points has its carried
       resource emptied (+0x84(0, 0, 1) = 0x004aace0). */
    void (*clear_carried)(void *context, AoeLiveUnit *victim);
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
   and 24 and empties a killed victim's carried resource. Returns 1, 0 when the radius is not positive,
   -1 for missing dependencies. */
int aoe_combat_blast_ror(AoeLiveUnit *attacker, float x, float y, float z,
                         AoeLiveUnit *source, AoeLiveUnit *excluded,
                         int tribe, const AoeCombatBlastOps *ops);

/* The root action unit's own (vtable 0x00542748 and the classes that
   keep it): +0x74 0x004aac60 the damage is each attack amount times the
   multiplier, summed, no armor; +0x70 0x004aac10 hit points drop by
   +0x74's damage and below 1.0 become 0 with the death call
   (0x004ac380(0)). Returns the new hit points. */
double aoe_combat_plain_damage_ror(const DatUnitAttackArmor *attacks, int32_t attack_count,
                                   float multiplier);
float aoe_combat_plain_hit_ror(AoeLiveUnit *target, double damage,
                               void (*die)(void *context, AoeLiveUnit *unit), void *context);

/* The tribe armor readouts (types 80 / 70, vtables 0x00547fa0 /
   0x0054820c), from the armor list (+0x102 / +0x104): +0x230 0x004aee00
   the melee armor (the base armor byte raised to the largest class 4
   armor) and the displayed melee armor (+0x13c); +0x248 0x004aee70 the
   largest class 3 armor (0 base) and the displayed pierce armor (+0x160).
   0x004a4b90 (the object panel): a type 70 template's last class 3
   armor, else 0. */
void aoe_combat_melee_armor_readout_ror(const DatUnit *t, int16_t *armor, int16_t *displayed);
void aoe_combat_pierce_armor_readout_ror(const DatUnit *t, int16_t *armor, int16_t *displayed);
int16_t aoe_combat_panel_pierce_armor_ror(const DatUnit *t);

/* More type-50 slots (vtable 0x0054324c, kept by 0x005445e8, 0x00547fa0
   and 0x0054820c). */
typedef struct {
    void *context;
    /* The target's +0x74 damage (attack count and list, multiplier,
       attacker's owner, attacker). */
    void (*take_damage)(void *context, AoeLiveUnit *target, int16_t attack_count,
                        const DatUnitAttackArmor *attacks, float multiplier, AoeLiveUnit *attacker);
    /* The attacker's +0xf0 multiplier against a target. */
    float (*multiplier)(void *context, AoeLiveUnit *attacker, AoeLiveUnit *target);
    /* The unit's +0x158(a, scale, b). */
    void (*scaled)(void *context, AoeLiveUnit *unit, int32_t a, float scale, int32_t b);
    /* 0x00403e50 clear the action list; +0x38 set the graphic. */
    void (*clear_actions)(void *context, AoeLiveUnit *unit);
    void (*set_graphic)(void *context, AoeLiveUnit *unit, int16_t graphic);
    /* The unit AI (+0x74) notice handler +0x14 (nonzero: handled) and
       the owner's +0xe8 notice. */
    int (*has_ai)(void *context, AoeLiveUnit *unit);
    int (*ai_notice)(void *context, AoeLiveUnit *unit, int32_t a, int32_t b, int32_t code,
                     int32_t c, int32_t d, int32_t e);
    void (*owner_notice)(void *context, AoeLiveUnit *unit, int32_t a, int32_t b, int32_t code,
                         int32_t c, int32_t d, int32_t e);
} AoeCombatSlotOpsRor;

/* +0x104 0x00427ac0: the target takes the attacker's attacks (template
   +0x108 / +0x10c) at the attacker's multiplier. +0x15c 0x00427170: +0x158
   with scale 1.0. +0x18c 0x00427a10: the action list cleared, then the
   standing graphic (template +0x18); +0x190 0x00427a40 the graphic alone;
   both 1. +0x148 0x00426db0: in state 2 a notice the unit AI (if any)
   does not handle, other than code 699, goes to the owner. */
void aoe_combat_attacked_by_ror(AoeLiveUnit *target, AoeLiveUnit *attacker,
                                const AoeCombatSlotOpsRor *ops);
void aoe_combat_scaled_ror(AoeLiveUnit *unit, int32_t a, int32_t b, const AoeCombatSlotOpsRor *ops);
int aoe_combat_stand_ror(AoeLiveUnit *unit, int clear, const AoeCombatSlotOpsRor *ops);
void aoe_combat_notice_ror(AoeLiveUnit *unit, int32_t a, int32_t b, int32_t code, int32_t c,
                           int32_t d, int32_t e, const AoeCombatSlotOpsRor *ops);

/* The type-50 "order" slots that build one base action (vtable 0x0054324c,
   kept by 0x005445e8, 0x00547fa0 and 0x0054820c):
   +0x158 0x00427190 move to an object (type 1, 0x004052d0 with the scale
   and the walking graphic +0xc0, else +0xbc; no task),
   +0x168 0x00427420 gather an object (type 5, 0x00402ef0 with the task),
   +0x17c 0x004275b0 explore a point (type 4, 0x00402b80 at (x, y, unit z);
   no task),
   +0x180 0x004276e0 enter an object (type 3, 0x00402440 with the task).
   An object slot resolves the ID (0x005206d0; none gives 0). Without
   `force`, a head action (0x00404520 / 0x004045b0) of the type already
   aimed at the object (+0x4c; explore: any) gives 1. Gather and explore
   then retarget (+0x34) a head of their type (object and position, or the
   point and unit z) and give 1. Gather and enter need a task (+0x214 with
   the object and its x, y; none gives 0). A built action replaces the list
   (0x00403e50) and is adopted (+0x208): 1; a failed build gives 0.
   The attacks (type 9; a head of type 9 or 7 counts as the same order):
   +0x154 0x00426e20 attack an object: without `force` a head aimed at it
   gives 1; a type-9 head aimed at it is retargeted (1); a class-4 unit
   first takes its owner's template 83 (owner +0x24 +0x14c, when present,
   through +0x54); then 0x00401150. +0x14c 0x00426ff0 attack a point
   (x, y, z): without `force` a head at exactly that point (0x00407670)
   gives 1; then 0x00401290. Both builds take the template's attack fields
   (+0x114, +0x138, +0x124, +0x12a). */
typedef enum {
    AOE_COMBAT_ORDER_ATTACK_POINT_ROR = 2,
    AOE_COMBAT_ORDER_MOVE_ROR = 1,
    AOE_COMBAT_ORDER_ENTER_ROR = 3,
    AOE_COMBAT_ORDER_EXPLORE_ROR = 4,
    AOE_COMBAT_ORDER_GATHER_ROR = 5,
    AOE_COMBAT_ORDER_ATTACK_ROR = 9
} AoeCombatOrderKindRor;

typedef struct {
    void *context;
    AoeLiveUnit *(*resolve)(void *context, int32_t id);
    void *(*head)(void *context, AoeLiveUnit *unit); /* NULL: empty list */
    int16_t (*action_type)(void *context, void *action);
    AoeLiveUnit *(*action_target)(void *context, void *action);
    void (*retarget)(void *context, void *action, AoeLiveUnit *target, float x, float y,
                     float z);
    const DatUnitTask *(*task)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target, float x,
                               float y);
    void *(*build)(void *context, AoeLiveUnit *unit, AoeCombatOrderKindRor kind,
                   const DatUnitTask *task, AoeLiveUnit *target, float x, float y, float z,
                   float scale, int16_t graphic);
    void (*clear_actions)(void *context, AoeLiveUnit *unit);
    void (*adopt)(void *context, AoeLiveUnit *unit, void *action);
    /* The head action's point (0x00407670), the owner's template `i` and
       the unit's +0x54 (template change). */
    void (*action_point)(void *context, void *action, float *x, float *y, float *z);
    const DatUnit *(*owner_template)(void *context, AoeLiveUnit *unit, int16_t i);
    void (*change_template)(void *context, AoeLiveUnit *unit, const DatUnit *t);
} AoeCombatOrderOpsRor;

/* `id` serves the object slots, x / y the explore point and `scale` the
   move. ATTACK_POINT goes through aoe_combat_order_point_ror. */
int aoe_combat_order_ror(AoeLiveUnit *unit, AoeCombatOrderKindRor kind, int32_t id, float x,
                         float y, float scale, int force, const AoeCombatOrderOpsRor *ops);
int aoe_combat_order_point_ror(AoeLiveUnit *unit, float x, float y, float z, int force,
                               const AoeCombatOrderOpsRor *ops);

/* +0x34 0x00427c60: after the move (0x0044bc40), a unit with an ID that
   changed tiles writes the new tile into the nine object memory entries
   it holds (+0x18c..+0x1ac; entry bytes +6 / +7). */
void aoe_combat_memory_moved_ror(uint8_t *entries[9], int32_t id, float old_x, float old_y,
                                 float new_x, float new_y);

/* Type-50 own fields (live +0x18c..+0x1b8). */
typedef struct {
    uint8_t *memory[9];   /* +0x18c..+0x1ac object memory entries */
    int32_t memory_count; /* +0x1b0 */
    float reload_timer;   /* +0x1b4 */
    uint8_t field_1b8;    /* +0x1b8 (saved from 2.01) */
} AoeCombatType50Ror;

/* Init 0x00426140 (run by the constructor 0x00425f30 when its init flag
   is set, after the type-40 constructor; the load constructor 0x00425fe0
   runs the type-40 load constructor and, flagged, the record load
   0x004261a0):
   the memory and its count cleared, the reload timer at the template's
   reload time (+0x120), +0x1b8 0; the type-40 init is the caller's. Returns the class byte, 0x32. The re-initialiser 0x004260f0
   (`reinit`) clears the memory and reloads the timer but keeps +0x1b8 and
   the class (it runs the type-40 re-initialiser instead). */
uint8_t aoe_combat_type50_init_ror(AoeCombatType50Ror *unit,
                                   const DatUnit *unit_template, int reinit);

/* The release record at destruction: global 0x7d2058's
   0x00516a50 (unit ID, owner +0x4a, truncated x and y, template +0x14,
   memory count, 0, memory). */
typedef void (*AoeCombatType50RecordFn)(void *context, int32_t unit_id,
                                        int16_t owner, int32_t x, int32_t y,
                                        int16_t template_word_14,
                                        int32_t memory_count,
                                        uint8_t *const memory[9]);

/* Destructor 0x00426050 (deleting form 0x00425fc0): a unit with an ID
   (>= 0) is recorded through `record` with its position truncated toward
   zero; the type-40 destructor is the caller's. */
void aoe_combat_type50_destroy_ror(const AoeCombatType50Ror *unit,
                                   int32_t unit_id, int16_t owner, float x,
                                   float y, int16_t template_word_14,
                                   AoeCombatType50RecordFn record,
                                   void *context);

#endif
