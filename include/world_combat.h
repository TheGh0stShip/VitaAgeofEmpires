#ifndef VITA_AOE_WORLD_COMBAT_H
#define VITA_AOE_WORLD_COMBAT_H

#include <stdint.h>

#include "action_artifact.h"
#include "action_attack.h"
#include "action_build.h"
#include "action_farm.h"
#include "action_convert.h"
#include "action_gather.h"
#include "action_heal.h"
#include "action_repair.h"
#include "action_trade.h"
#include "action_list.h"
#include "combat.h"
#include "live_unit_owner.h"
#include "command_production.h"
#include "research_action.h"
#include "projectile.h"
#include "train_action_adapter.h"
#include "unit_order.h"
#include "player_outcome.h"
#include "world_motion.h"

/* Binds the translated RoR 1.0a attack action, its move sub-action and the
   combat core (melee hits, damage, receive) to a world_motion world.

   Per ordered unit this owns the reload timer (+0x1b4), the last-turn
   time (+0xa0), the adopted action, its sub-action list (+0x34) and, when
   the unit has none, its +0x184 action list. Each tick advances the world
   clock (world +0x04 ms, +0x84 seconds) and runs units through their class
   update chain (see aoe_world_combat_update_world_ror).

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
     hold, unit notices are counted only, and the instant-kill option is
     off; target attach/release (+0xc0/+0xc4) keep the reference count and
     dormancy;
   - victims never keep their carried resource for an attacker class
     (+0x13c) and kill credits are counted only;
   - terminal cleanup (+0xb8) spawns the corpse; the world update removes
     units reaching state 8 (predicate 1/2, then +0x08). */
struct AoeWorldCombatRor;

/* First int16 of every +0x184 entry: actions begin with their type; the
   record carries this impossible type instead. */
enum { AOE_WORLD_COMBAT_RECORD_TAG = -2 };

typedef struct {
    int16_t action_tag; /* AOE_WORLD_COMBAT_RECORD_TAG */
    AoeLiveUnit *unit;
    struct AoeWorldCombatRor *world;
    float reload_timer;
    float last_turn;
    AoeAttackActionRor attack;
    uint8_t attacking;
    AoeGatherActionRor gather;
    uint8_t gathering;
    AoeBuildActionRor build;
    uint8_t constructing;
    uint8_t pending; /* action built by the order step, not yet adopted */
    /* Set once this world has taken over a building's runtime-owned live
       (+0x184) and queue (+0x1cc) list callbacks. */
    uint8_t adopted_building;
    /* The unit's +0x184 action list when it had none. Its entries are this
       record (while a gather, attack or build action is adopted), move
       actions (0x00405db0) or farm actions (default task 0x15); the list
       context is this record. Type-80 units use their runtime-owned live
       list instead, with the same entries. */
    AoeActionList unit_actions;
    uint8_t last_result; /* head update result of the latest unit update */
    uint8_t order_serial; /* unit byte +0x188: orders received */
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

typedef struct AoeWorldCombatRor {
    AoeWorldMotionRor *motion;
    AoeWorldCombatUnitRor **units; /* by unit ID + 1000 */
    size_t unit_slots;
    AoeWorldProjectileRor *flights;
    size_t flight_count, flight_capacity;
    uint32_t world_ms;
    float tick;
    uint32_t notices, hits, misses, projectiles, kills, blasts, alerts,
        corpses, deposits, repairs, completions;
    /* World update placeholders: default tasks (+0x1fc) routed to an
       action class that is not translated yet, and attached-unit list
       updates, are counted only; list objects that are not runtime units
       are counted and skipped. */
    uint32_t default_tasks, attached_updates, foreign_objects, removed,
        farms_depleted, research_removals, queue_refunds;
    /* Empty outcome managers for players without one (no victory
       conditions are loaded yet) and the type-80 status-4 counters
       (global +0xa34). */
    AoeOutcomeManager outcome[AOE_SCENARIO_RUNTIME_PLAYERS];
    AoeOutcomePlayerRef outcome_player[AOE_SCENARIO_RUNTIME_PLAYERS];
    uint32_t queue_status_four[AOE_SCENARIO_RUNTIME_PLAYERS];
    uint32_t memory_add_failures; /* out-of-range notify categories */
    /* Production: the training action adapter over every building's queue
       list (+0x1cc), its hooks, the world unit-table snapshot handed to
       training actions, and the world update count (global 0x7d2460,
       recorded into building history +0x1f0). */
    AoeTrainActionAdapter train;
    AoeTrainActivationOps train_activation;
    AoeTrainActionOps train_update;
    AoeTrainSpawnOps train_spawn;
    AoeTrainWaitingOps train_waiting;
    AoeTrainTargetOps train_targets;
    AoeActionTickOps train_tick;
    AoeLiveUnitRegistry registry_view;
    uint32_t update_serial;
    uint32_t trained, train_failures, production_events;
    /* Research actions (type 0x67) on building +0x184 lists: completions
       (owner +0x11c), refusals and stops (owner +0x118) and portable
       dependency failures. */
    uint32_t researched, research_stopped, research_failures;
    /* Production commands answered by the player fallback (busy or
       missing building: owner +0x10c / +0x118). */
    uint32_t production_fallbacks;
    /* Repair actions (0x6a): unpaid repairs (event 0x6b) and portable
       dependency failures; completed repairs count in `repairs`. */
    uint32_t repair_unpaid, repair_failures;
    /* Heal actions (0x69): fully healed targets and dependency failures. */
    uint32_t heals, heal_failures;
    /* Changes of owner (unit +0x44 notices 0x20b) and conversion action
       dependency failures. */
    uint32_t conversions, convert_failures;
    uint32_t hunt_failures; /* hunting action dependency failures */
    uint32_t trade_payouts, trade_failures; /* gold paid at home docks */
    /* Wonders: the countdown per player (game 0x0041c0d0, -1 cleared by
       0x0041c0b0, 0 when never set), whether it is hidden (game +0x3f4
       +0x108), wins (0x0045d460) and the game-over byte (+0x3f4 +0x1e,
       100 after a wonder win). Artifact/discovery/wonder dependency
       failures. */
    int32_t wonder_countdown[AOE_SCENARIO_RUNTIME_PLAYERS];
    int wonder_countdown_hidden;
    uint32_t wonder_victories, object_failures;
    uint8_t game_over;
    uint8_t tearing_down; /* game +0x3f4 +0x1d during destruction */
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

/* Gather (0x00402ef0/0x00402f80), attack (0x00401150/0x00401290) and build
   (0x004b13c0/0x004b1450) routes are constructed by the order step below.
   Building work binds the foundation's type-80 storage on demand and
   completes it through aoe_live_unit_scenario_building_complete_ror (class
   pre-completion, adjacency and disappear hooks are no-ops). */

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

/* One world update (0x0051da66 loop) bound to this world: advances the
   clock by `tick` and runs every player's +0x7c list update (0x0045d040
   traversal of list +0x28 through 0x00451270) with each unit's class
   chain:
   - type 70/80 class-18 faith regeneration (0x004ae7d0), type 50..80
     reload countdown (0x00426bf0);
   - the common predicate (prelude, attached unit, 0x004a8f40 state
     switch with the state-2 damage graphics +0x28 = 0x004a9240);
   - the 0x00415200 tail (dying states, sprites) and the 0x00406500 action
     list (projectiles update their flight there);
   - type 80 runs its own 0x004acf80 around that chain (construction
     frame, history, class-18 generation, then production or the terminal
     queue cleanup);
   - units reaching state 8 are cleaned up (+0x08) and removed.
   Not run yet: player +0x30 lists, the type-50 visibility spotting after
   0x00406500 and the player map update. Returns 0 on failure. */
int aoe_world_combat_update_world_ror(AoeWorldCombatRor *combat, float tick);

/* Production queue add 0x004adab0 on a type-80 unit: charges the owner
   (0x004f15f0 / 0x004f1750 with multiplier 1 and population flag 1) per
   unit and queues up to `count` of `unit_id`. Training then runs inside
   the building's update. Returns 0 on failure. */
int aoe_world_combat_train_ror(AoeWorldCombatRor *combat,
                               AoeLiveUnit *building, int16_t unit_id,
                               int16_t count);

/* Tribe commands 0x64 (train, 0x004e85e0) and 0x65 (research,
   0x004e8780) from a 16-byte record (aoe_command_encode_production_ror),
   bound to this world: the building resolves through the world (ID
   lookup 0x005206d0); a type-80 building whose +0x184 head is not a
   0x66/0x67 action gets the action through +0x208 (0x00406490: clear the
   list, push and activate). Train actions from a command are not prepaid
   (0x004b4f10(building, unit, request, 0)) and charge on activation;
   research actions pay through the owner's manager (0x004eb100). Both then
   run as the building's head action during its update. Buildings have no
   group object (+0x74). The player fallbacks (owner +0x10c / +0x118) are
   counted. Returns aoe_command_execute_production_ror's result, or -1
   when the world is unusable or the action could not be built. */
int aoe_world_combat_command_production_ror(AoeWorldCombatRor *combat,
                                            const uint8_t *command,
                                            size_t size);

/* Unit +0x44 (aoe_live_unit_change_owner_ror) bound to the runtime: the
   player lists, vision, storage, production queue, building research and
   stack unit move to `owner`. Returns 0 on failure. */
int aoe_world_combat_change_owner_ror(AoeWorldCombatRor *combat,
                                      AoeLiveUnit *unit, int16_t owner);

/* Advances the world clock by `tick` seconds and runs one unit (which must
   have a record, e.g. from an order) through the same class chain as the
   world update, outside its player list. *result receives the latest head
   action update result (attack/gather/build/move codes). Returns 0 on
   failure. */
int aoe_world_combat_tick_ror(AoeWorldCombatRor *combat,
                              AoeLiveUnit *attacker, float tick,
                              uint8_t *result);

/* The same chain for any unit without advancing the clock. A unit below
   1.0 hit points dies (state 3, dying graphic, rand/srand-bracketed dying
   sound), moves on to 4 (dying animation, until its frame wraps) or 5,
   then decays per its carried resource; entering 7 spawns its corpse. */
int aoe_world_combat_update_unit_ror(AoeWorldCombatRor *combat,
                                     AoeLiveUnit *unit, float tick);

/* Runs every active projectile unit through the chain with the current
   tick (call after the attackers' ticks); a flight ends when its update
   (0x00404db0) reports done or the projectile leaves the active states.
   Returns 0 on failure. */
int aoe_world_combat_update_projectiles_ror(AoeWorldCombatRor *combat);

#endif
