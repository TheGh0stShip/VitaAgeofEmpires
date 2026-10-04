#ifndef VITA_AOE_WORLD_COMBAT_H
#define VITA_AOE_WORLD_COMBAT_H

#include <stdint.h>

#include "action_attack.h"
#include "action_list.h"
#include "combat.h"
#include "world_motion.h"

/* Binds the translated RoR 1.0a attack action, its move sub-action and the
   combat core (melee hits, damage, receive) to a world_motion world.

   Per attacking unit this owns the reload timer (+0x1b4), the last-turn
   time (+0xa0), the attack action and its sub-action list (+0x34). Each
   tick advances the world clock (world +0x04 ms, +0x84 seconds), counts the
   reload down (0x00426bf0) and updates the attack action (0x004017c0).

   Inferences and placeholders (see docs/reversing.md):
   - the attack parameters follow the type-7 route of 0x00405f20: the
     unit's first action-type-7 task gives the move (+0x34) and attack
     (+0x38) graphics and range (work range), template min range,
     projectile and frame delay the rest;
   - firing at a target uses the tribe fire 0x004aec50 / melee hit
     0x004aeca0 for type >= 70 and the base ones otherwise; projectile
     launches are counted but not simulated yet;
   - visibility is always "visible", owners never refuse, groups never
     hold, unit notices and target attach/release hooks are counted only,
     and the instant-kill option is off. */
typedef struct {
    AoeLiveUnit *unit;
    float reload_timer;
    float last_turn;
    AoeAttackActionRor attack;
    uint8_t attacking;
    AoeActionList sub_actions;
} AoeWorldCombatUnitRor;

typedef struct {
    AoeWorldMotionRor *motion;
    AoeWorldCombatUnitRor **units; /* by unit ID + 1000 */
    uint32_t world_ms;
    float tick;
    uint32_t notices, hits, misses, projectiles, kills;
} AoeWorldCombatRor;

int aoe_world_combat_init_ror(AoeWorldCombatRor *combat,
                              AoeWorldMotionRor *motion);
void aoe_world_combat_destroy_ror(AoeWorldCombatRor *combat);

AoeWorldCombatUnitRor *aoe_world_combat_unit_ror(AoeWorldCombatRor *combat,
                                                 AoeLiveUnit *unit);

/* Constructs and activates the attack action of `attacker` against
   `target` (0x00401150 + +0x1c). Returns 0 when the unit has no attack
   (type < 50) or on failure. */
int aoe_world_combat_attack_ror(AoeWorldCombatRor *combat,
                                AoeLiveUnit *attacker, AoeLiveUnit *target);

/* Advances the world clock by `tick` seconds and updates one attacker:
   reload countdown, then its attack action. Writes the action result
   (attack update codes) to *result. Returns 0 on missing state. */
int aoe_world_combat_tick_ror(AoeWorldCombatRor *combat,
                              AoeLiveUnit *attacker, float tick,
                              uint8_t *result);

#endif
