#ifndef VITA_AOE_ACTION_BUILD_H
#define VITA_AOE_ACTION_BUILD_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a build action (vtable 0x005486bc, action type
   0x65; routed from task action 0x65 to 0x004b13c0 (target) / 0x004b1450
   (point)). States: 3 idle (notice 0x202; computer players stop with
   0x1fa), 4 walks to the foundation (task work range), 6 faces it, 7 works
   it, 11 walks to a point, 13 failed (notice 0x1f9, result 3). */
typedef struct {
    AoeMoveActionRor base;
    float timer;             /* +0x2c: -1 after finishing a farm */
    const DatUnitTask *task; /* +0x30 */
} AoeBuildActionRor;

typedef struct {
    AoeMoveActionOps move; /* resolve, target hooks, unit_notice,
                              select_graphic, clear_secondary */
    const void *(*graphic)(void *context, int16_t id);
    float (*tick_seconds)(void *context);                 /* world +0x84 */
    int32_t (*owner_kind)(void *context, AoeLiveUnit *unit); /* 0x00454150 */
    int (*face)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    /* World 0x00520590(owner ID, class 27, x, y, &count, 1): a wall of the
       unit's owner on tile (x, y). */
    int (*wall_at)(void *context, AoeLiveUnit *unit, int32_t x, int32_t y);
    /* 0x004ad7b0 on the target: add work, report completion. */
    int (*add_build_work)(void *context, AoeLiveUnit *target, float amount,
                          int *completed);
    /* 0x004ac320 on the unit. */
    void (*reset_group)(void *context, AoeLiveUnit *unit);
    /* Owner player +0x34(owner, unit, 0x2ce, target, target owner, x, y, z,
       1.0, 1, 0, 0x64): queue a repair of a finished damaged building. */
    void (*request_repair)(void *context, AoeLiveUnit *unit,
                           AoeLiveUnit *target);
    /* Unit +0x12c(1) then +0x98(target, x, y, 0): order the unit onto a
       finished farm. */
    void (*order_onto)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    void (*play_sound)(void *context, int16_t sound_id);   /* 0x004a2370 */
    int instant_build_option; /* global 0x00580e38 +0x9a8: 1000 per tick */
    /* Sub-action list +0x34 as for gathering (quiet after the push for
       the target walk, before it for the point walk). */
    int (*start_move)(void *context, AoeBuildActionRor *action,
                      AoeLiveUnit *target, float range, const void *graphic,
                      int quiet_first);
    uint8_t (*sub_update)(void *context, AoeBuildActionRor *action);
    void (*sub_clear)(void *context, AoeBuildActionRor *action);
} AoeBuildActionOps;

/* 0x004b13c0: the target is taken only when it has the unit's owner (a
   NULL target would crash the original); 0x004b1450: a point. */
int aoe_build_action_init_target_ror(AoeBuildActionRor *action,
                                     AoeLiveUnit *unit,
                                     const DatUnitTask *task,
                                     AoeLiveUnit *target,
                                     const AoeBuildActionOps *ops);
int aoe_build_action_init_point_ror(AoeBuildActionRor *action,
                                    AoeLiveUnit *unit,
                                    const DatUnitTask *task, float x,
                                    float y, float z);
int aoe_build_action_set_state_ror(AoeBuildActionRor *action, uint8_t state,
                                   const AoeBuildActionOps *ops);
/* +0x1c = 0x004b1490: with `arg` and a target walk (4), else idle (3). */
int aoe_build_action_activate_ror(AoeBuildActionRor *action, int arg,
                                  const AoeBuildActionOps *ops);
/* +0x34 = 0x004b1bb0 (own target: walk to it, kept in states 4/6; else
   walk to the point) and +0x30 = 0x004b5db0 (walk to the point). */
int aoe_build_action_retarget_ror(AoeBuildActionRor *action,
                                  AoeLiveUnit *target, float x, float y,
                                  float z, const AoeBuildActionOps *ops);
int aoe_build_action_walk_ror(AoeBuildActionRor *action, float x, float y,
                              float z, const AoeBuildActionOps *ops);
/* +0x28 = 0x004b1880. *result: 0 working, 1 done, 3 failed. Returns 0
   for missing dependencies. */
int aoe_build_action_update_ror(AoeBuildActionRor *action,
                                const AoeBuildActionOps *ops,
                                uint8_t *result);

#endif
