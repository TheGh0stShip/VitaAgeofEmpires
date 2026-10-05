#ifndef VITA_AOE_ACTION_ARTIFACT_H
#define VITA_AOE_ACTION_ARTIFACT_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a object actions that run on the object itself
   (default tasks): artifacts (0x6b), discoveries (0x6c) and wonders
   (0x78). */

/* Shared bindings. */
typedef struct {
    AoeMoveActionOps move; /* resolve, target hooks */
    float (*tick_seconds)(void *context);                    /* world +0x84 */
    int16_t (*player_count)(void *context);                  /* world +0x3c */
    /* Map size (world +0x28 +0x08/+0x0c) and the objects on one tile
       (+0x8d8c rows, 24-byte tiles, list at +0x10), in list order. */
    int32_t (*map_width)(void *context);
    int32_t (*map_height)(void *context);
    AoeLiveUnit *(*tile_object)(void *context, int32_t x, int32_t y,
                                uint32_t index);
    /* Sub-action list (point walks). */
    int (*start_move)(void *context, AoeMoveActionRor *action,
                      const void *graphic);
    uint8_t (*sub_update)(void *context, AoeMoveActionRor *action);
    void (*sub_clear)(void *context, AoeMoveActionRor *action);
    const void *(*graphic)(void *context, int16_t id);
    /* Unit +0x44 (aoe_live_unit_change_owner_ror). */
    int (*change_owner)(void *context, AoeLiveUnit *unit, int16_t owner);
    /* Owner +0x24[template] +0x18(owner, x, y, z): create a unit. */
    void (*create)(void *context, int16_t owner, int16_t template_id,
                   float x, float y, float z);
    /* Unit +0xdc / +0xd8(player, flag, -1): the unit's vision for another
       player. */
    void (*vision)(void *context, AoeLiveUnit *unit, int16_t player, int add,
                   int flag);
    /* Game +0x40(event, owner, x, y, 0). */
    void (*event)(void *context, uint32_t event, int16_t owner, int32_t x,
                  int32_t y);
    /* Game 0x0041c0d0(owner, years) / 0x0041c0b0(owner): the wonder
       countdown shown for a player. */
    void (*countdown_set)(void *context, int16_t owner, int32_t years);
    void (*countdown_clear)(void *context, int16_t owner);
    /* Player 0x0045d460: the owner wins (allies with team victory too). */
    void (*win)(void *context, int16_t owner);
    /* Game +0x3f4 +0x108 (countdown hidden) and +0x1d (tearing down). */
    int countdown_disabled, tearing_down;
    /* Game +0x3f4 +0x1e, set to 100 when a wonder wins. */
    uint8_t *game_over;
} AoeObjectActionOps;

/* Artifact action (vtable 0x0054865c): constructors 0x004b0df0 (the
   target pointer is stored without the base setter) / 0x004b0e20. */
typedef struct {
    AoeMoveActionRor base;
    float timer;             /* +0x2c */
    const DatUnitTask *task; /* +0x30 */
} AoeArtifactActionRor;

int aoe_artifact_action_init_ror(AoeArtifactActionRor *action,
                                 AoeLiveUnit *unit, const DatUnitTask *task,
                                 AoeLiveUnit *target);
int aoe_artifact_action_init_point_ror(AoeArtifactActionRor *action,
                                       AoeLiveUnit *unit,
                                       const DatUnitTask *task, float x,
                                       float y, float z);
/* +0x5c = 0x004b0ea0: 4 walks to the point, 5 walks to the target's
   position (dropping the target), both then wait (6); 10 waits 3 s. */
int aoe_artifact_action_set_state_ror(AoeArtifactActionRor *action,
                                      uint8_t state,
                                      const AoeObjectActionOps *ops);
/* +0x1c = 0x004b0e60: with `arg`, a target walks (5), a point (4), else
   6. +0x34 = 0x004b1350 / +0x30 = 0x004b1320 / +0x2c = 0x004b1310. */
int aoe_artifact_action_activate_ror(AoeArtifactActionRor *action, int arg,
                                     const AoeObjectActionOps *ops);
int aoe_artifact_action_retarget_ror(AoeArtifactActionRor *action,
                                     AoeLiveUnit *target, float x, float y,
                                     float z, const AoeObjectActionOps *ops);
/* 0x004b1120: the one player above Gaia, other than the artifact's owner,
   with state-2 objects (other than classes 1, 11 and 30) on the tiles
   within 1 + size of it; -1 when none or several. */
int16_t aoe_artifact_claimant_ror(const AoeArtifactActionRor *action,
                                  const AoeObjectActionOps *ops);
/* +0x28 = 0x004b0fe0: a finished walk (1..3) clears the walk; state 6
   changes owner to the claimant or waits (10); 10 counts down; 100 returns
   the artifact to Gaia. *result 1 in state 1. */
int aoe_artifact_action_update_ror(AoeArtifactActionRor *action,
                                   const AoeObjectActionOps *ops,
                                   uint8_t *result);

/* Discovery action (vtable 0x00548788): constructors 0x004b2880 /
   0x004b2900 allocate one "found" byte per player. */
typedef struct {
    AoeMoveActionRor base;
    float timer;             /* +0x2c */
    const DatUnitTask *task; /* +0x30 */
    uint8_t *found;          /* +0x40 */
    int16_t found_count;
} AoeDiscoveryActionRor;

int aoe_discovery_action_init_ror(AoeDiscoveryActionRor *action,
                                  AoeLiveUnit *unit, const DatUnitTask *task,
                                  AoeLiveUnit *target,
                                  const AoeObjectActionOps *ops);
void aoe_discovery_action_free_ror(AoeDiscoveryActionRor *action);
/* +0x1c = 0x004b2990 (`arg`: state 6), +0x5c = 0x004b29b0 (10 waits 3
   s), +0x28 = 0x004b29d0: state 6 scans (0x004b2a30: each player other
   than the owner that first has an object on the tiles within 1 + size
   gets template 162 created at that object), then waits (10). */
int aoe_discovery_action_activate_ror(AoeDiscoveryActionRor *action, int arg);
int aoe_discovery_action_update_ror(AoeDiscoveryActionRor *action,
                                    const AoeObjectActionOps *ops,
                                    uint8_t *result);

/* Wonder action (vtable 0x00548a68, constructor 0x004b7220). */
typedef struct {
    AoeMoveActionRor base;
    float timer; /* +0x2c: years elapsed (two per second) */
} AoeWonderActionRor;

int aoe_wonder_action_init_ror(AoeWonderActionRor *action,
                               AoeLiveUnit *unit);
/* +0x5c = 0x004b7410: 2 (foundation) shows it to every other player
   (event 0x6c); 6 (built) swaps that vision for the finished one, starts
   the 2000-year countdown (unless hidden) and sends event 0x6d. */
int aoe_wonder_action_set_state_ror(AoeWonderActionRor *action,
                                    uint8_t state,
                                    const AoeObjectActionOps *ops);
/* +0x1c = 0x004b73f0: `arg` enters 2. */
int aoe_wonder_action_activate_ror(AoeWonderActionRor *action, int arg,
                                   const AoeObjectActionOps *ops);
/* +0x28 = 0x004b7580: 2 waits for the building to finish (state 2 of the
   unit); 6 counts years at twice the tick, updates the countdown and at
   2000 makes the owner win (0x0045d460) and sets the game-over byte. */
int aoe_wonder_action_update_ror(AoeWonderActionRor *action,
                                 const AoeObjectActionOps *ops,
                                 uint8_t *result);
/* Destructor 0x004b7250 (deleting 0x004b7200; unless tearing down): removes the vision it gave
   other players; a built wonder clears the countdown (restarting it at
   2000 when the owner has another standing wonder, template 276) and
   sends event 0x6e. `owner_has_other` reports that other wonder. */
int aoe_wonder_action_destroy_ror(AoeWonderActionRor *action,
                                  const AoeObjectActionOps *ops,
                                  int owner_has_other);

#endif
