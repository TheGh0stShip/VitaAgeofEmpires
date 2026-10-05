#ifndef VITA_AOE_ACTION_HEAL_H
#define VITA_AOE_ACTION_HEAL_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a heal action (vtable 0x005487e8, action type
   0x69; routed from task action 0x69 to 0x004b2c00 (target) / 0x004b2c80
   (point)). States: 1 done, 2 cleared, 3 idle (notice 0x202), 4 walks to
   the target (range 0.4), 6 faces it, 7 heals, 11 walks to a point, 13
   failed (notice 0x1f9, result 3). Notices carry value 0x25b. Healing uses
   the target's +0x8c repair method (aoe_live_unit_repair_ror). */
typedef struct {
    AoeMoveActionRor base;
    const DatUnitTask *task; /* +0x30 */
} AoeHealActionRor;

typedef struct {
    AoeMoveActionOps move;
    const void *(*graphic)(void *context, int16_t id);
    float (*tick_seconds)(void *context);                    /* world +0x84 */
    double (*distance)(void *context, AoeLiveUnit *unit,
                       AoeLiveUnit *target);                 /* 0x004aaeb0 */
    int (*face)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    /* Owner +0x1c(target owner index): nonzero allows healing. */
    int (*allowed)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    /* Target +0x8c(amount): 0 working, 1 full, 2 unpaid. */
    uint8_t (*heal)(void *context, AoeLiveUnit *target, float amount);
    /* Owner resource 56 (+0x50 +0xe0): the heal-rate multiplier. */
    float (*heal_rate)(void *context, AoeLiveUnit *unit);
    void (*play_sound)(void *context, int16_t sound_id);
    int (*start_move)(void *context, AoeMoveActionRor *action,
                      AoeLiveUnit *target, float range, const void *graphic,
                      int quiet_first);
    uint8_t (*sub_update)(void *context, AoeMoveActionRor *action);
    void (*sub_clear)(void *context, AoeMoveActionRor *action);
} AoeHealActionOps;

int aoe_heal_action_init_target_ror(AoeHealActionRor *action,
                                    AoeLiveUnit *unit,
                                    const DatUnitTask *task,
                                    AoeLiveUnit *target,
                                    const AoeMoveActionOps *ops);
int aoe_heal_action_init_point_ror(AoeHealActionRor *action,
                                   AoeLiveUnit *unit, const DatUnitTask *task,
                                   float x, float y, float z);
/* +0x5c = 0x004b2d00. */
int aoe_heal_action_set_state_ror(AoeHealActionRor *action, uint8_t state,
                                  const AoeHealActionOps *ops);
/* +0x1c = 0x004b2cc0: a boat target (class 2, 20, 21, 22) leaves the
   state unchanged; another target walks (4); none idles (3). The argument
   is not read. */
int aoe_heal_action_activate_ror(AoeHealActionRor *action,
                                 const AoeHealActionOps *ops);
/* +0x34 = 0x004b26a0 (shared with conversion): the same target in states
   4/6/7 is kept; a new target is taken and activated (+0x1c); no target
   refuses (0). */
int aoe_heal_action_retarget_ror(AoeHealActionRor *action,
                                 AoeLiveUnit *target,
                                 const AoeHealActionOps *ops, int *accepted);
/* +0x28 = 0x004b2f40. *result: 0 working, 1 done, 3 failed, 4 the target
   became unfriendly (state 1, notice 0x1fb). A lost or inactive target
   idles (state 3). Healing amount: tick x work rate x task work value,
   times resource 56 when that exceeds 0.1. A moved target is followed
   while within the task work range or 0.5, else idle. Returns 0 for
   missing dependencies. */
int aoe_heal_action_update_ror(AoeHealActionRor *action,
                               const AoeHealActionOps *ops, uint8_t *result);

#endif
