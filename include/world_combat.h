#ifndef VITA_AOE_WORLD_COMBAT_H
#define VITA_AOE_WORLD_COMBAT_H

#include <stdint.h>

#include "action_attack.h"
#include "action_gather.h"
#include "action_list.h"
#include "combat.h"
#include "projectile.h"
#include "unit_order.h"
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
     0x004aeca0 for type >= 70 and the base ones otherwise; ranged units
     launch real projectile units (0x00426520 / 0x00449070) whose flights
     use src/projectile.c; the trail units of 0x0044bc40 are not spawned;
   - visibility is always "visible", owners never refuse, groups never
     hold, unit notices and target attach/release hooks are counted only,
     and the instant-kill option is off;
   - victims never keep their carried resource for an attacker class
     (+0x13c) and kill credits are counted only;
   - terminal cleanup (+0xb8) spawns the corpse, but removing the dead
     unit (state 8, predicate 1/2) is left to the player update list. */
typedef struct {
    AoeLiveUnit *unit;
    float reload_timer;
    float last_turn;
    AoeAttackActionRor attack;
    uint8_t attacking;
    AoeGatherActionRor gather;
    uint8_t gathering;
    uint8_t pending; /* action built by the order step, not yet adopted */
    /* The unit's +0x184 action list when it had none: its head entry is
       this record while a gather/attack action is adopted. */
    AoeActionList unit_actions;
    AoeActionList sub_actions;
} AoeWorldCombatUnitRor;

/* One launched projectile: the projectile unit, its type-8 flight action
   and, once its launcher died, its private template copy (0x004495b0). */
typedef struct {
    AoeLiveUnit *unit;
    AoeProjectileRor flight;
    DatUnit *captured;
    const DatUnit *original;
    DatUnitAttackArmor *captured_attacks;
    uint8_t active;
} AoeWorldProjectileRor;

typedef struct {
    AoeWorldMotionRor *motion;
    AoeWorldCombatUnitRor **units; /* by unit ID + 1000 */
    AoeWorldProjectileRor *flights;
    size_t flight_count, flight_capacity;
    uint32_t world_ms;
    float tick;
    uint32_t notices, hits, misses, projectiles, kills, blasts, alerts,
        corpses, deposits;
    int32_t player_field_104[AOE_SCENARIO_RUNTIME_PLAYERS];
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

/* Selects the actor's task for `target` with the translated tribe selector
   0x00508010 (owners: same player accepts, other non-Gaia players are
   hostile; the villager job switch of 0x00405f20 is not applied) and, for
   an action-type-5 task, constructs and activates the gather action
   (0x00402ef0 + 0x004b33a0). Deposits go through the derived player +0x78
   0x004f6c60. Returns 0 if no gather task applies. */
int aoe_world_combat_gather_ror(AoeWorldCombatRor *combat, AoeLiveUnit *unit,
                                AoeLiveUnit *target);

/* The order step 0x00405f20 (unit vtable +0x1f8) bound to this world: task
   selection with the tribe selector (and the villager job switch, which
   changes the unit's template through 0x004aa160), retargeting of the head
   action, and construction of the routed action. Gather (0x00402ef0 /
   0x00402f80) and attack (0x00401150 / 0x00401290) actions are built and
   adopted; other routes, or no task, fall back to a plain move. *code
   receives the order result. Returns 0 on failure. */
int aoe_world_combat_order_ror(AoeWorldCombatRor *combat, AoeLiveUnit *unit,
                               AoeLiveUnit *target, float x, float y,
                               float z, int16_t *code);

/* 0x004aa160 (root +0x54 via 0x004152b0 / 0x004ae8e0): for states 0..2,
   swap the unit's template keeping its hit-point ratio, moving vision and
   the footprint across the swap, selecting the new standing graphic and
   re-entering state 2. */
int aoe_world_combat_change_template_ror(AoeWorldCombatRor *combat,
                                         AoeLiveUnit *unit,
                                         const DatUnit *unit_template);

/* Advances the world clock by `tick` seconds and updates one attacker:
   reload countdown (0x00426bf0), sprites, then the state switch of
   0x004a8f40 whose active update is the attack action. Writes the action
   result (attack update codes) to *result. Returns 0 on missing state. */
int aoe_world_combat_tick_ror(AoeWorldCombatRor *combat,
                              AoeLiveUnit *attacker, float tick,
                              uint8_t *result);

/* The same state switch for any unit (without advancing the clock): a
   unit below 1.0 hit points dies (state 3 through the runtime state
   setter, dying graphic, rand/srand-bracketed dying sound), then decays
   per its carried resource (states 3/5 -> 6 or 7 -> 8); entering 7 spawns
   its corpse. */
int aoe_world_combat_update_unit_ror(AoeWorldCombatRor *combat,
                                     AoeLiveUnit *unit, float tick);

/* Updates every active projectile with the current tick (call after the
   attackers' ticks): the 0x004a8f40 state switch whose active update is
   the flight action 0x00404db0. Finished projectiles (state >= 7) are
   retired. Returns 0 on failure. */
int aoe_world_combat_update_projectiles_ror(AoeWorldCombatRor *combat);

#endif
