#ifndef VITA_AOE_ACTION_TRADE_H
#define VITA_AOE_ACTION_TRADE_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a trade action (vtable 0x005489f0, action type
   0x6f; routed from task action 0x6f to 0x004b6520 (target) / 0x004b65b0
   (point)). A trade boat loads goods at a foreign dock (the target) and is
   paid in gold at its own dock (the drop site, template drop site 0).
   States: 1 done, 2 cleared, 3 idle (notice 0x202), 4 sails to the
   target, 6 faces it, 7 loads, 8 sails home and is paid, 11 sails to a
   point, 13 failed (notice 0x1f9, result 3). Notices carry 0x267. */
typedef struct {
    AoeMoveActionRor base;
    AoeLiveUnit *home;          /* +0x14, ID in base.secondary_id */
    float timer;                /* +0x2c: retry wait */
    const DatUnitTask *task;    /* +0x30 */
    float origin_x, origin_y;   /* +0x40/+0x44: where the goods came from */
} AoeTradeActionRor;

typedef struct {
    AoeMoveActionOps move; /* resolve, target hooks, unit_notice,
                              select_graphic */
    const void *(*graphic)(void *context, int16_t id);
    float (*tick_seconds)(void *context);                    /* world +0x84 */
    int (*face)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    /* Owner resource (+0x50). */
    float (*resource)(void *context, AoeLiveUnit *unit, int16_t type);
    /* Direct store into the owner's resource (+0x50[type] = value). */
    void (*resource_store)(void *context, AoeLiveUnit *unit, int16_t type,
                           float value);
    /* Owner +0x78(type, amount, 1). */
    void (*resource_add)(void *context, AoeLiveUnit *unit, int16_t type,
                         float amount);
    uint8_t (*civ)(void *context, AoeLiveUnit *unit);  /* owner +0x109 */
    /* 0x004514d0 on the owner's list (state filter 2): nearest unit of
       either template ID to (x, y), zone-matched to `zone_unit` if set. */
    AoeLiveUnit *(*find_home)(void *context, AoeLiveUnit *unit, int16_t id1,
                              int16_t id2, float x, float y,
                              AoeLiveUnit *zone_unit);
    void (*play_sound)(void *context, int16_t sound_id);
    int (*start_move)(void *context, AoeMoveActionRor *action,
                      AoeLiveUnit *target, float range, const void *graphic,
                      int quiet_first);
    uint8_t (*sub_update)(void *context, AoeMoveActionRor *action);
    void (*sub_clear)(void *context, AoeMoveActionRor *action);
} AoeTradeActionOps;

int aoe_trade_action_init_target_ror(AoeTradeActionRor *action,
                                     AoeLiveUnit *unit,
                                     const DatUnitTask *task,
                                     AoeLiveUnit *target,
                                     const AoeMoveActionOps *ops);
int aoe_trade_action_init_point_ror(AoeTradeActionRor *action,
                                    AoeLiveUnit *unit,
                                    const DatUnitTask *task, float x,
                                    float y, float z);
/* +0x5c = 0x004b6680. */
int aoe_trade_action_set_state_ror(AoeTradeActionRor *action, uint8_t state,
                                   const AoeTradeActionOps *ops);
/* +0x1c = 0x004b6630: the carried type becomes 1 unless it is 0, 1 or 2;
   with `arg` a target sails (4), none clears (2); without, idle (3). */
int aoe_trade_action_activate_ror(AoeTradeActionRor *action, int arg,
                                  const AoeTradeActionOps *ops);
/* +0x34 = 0x004b7120 (the same target in 4/6/7 is kept; an own home dock
   sails home (8); another target is sailed to (4); none sails to the
   point (11)), +0x30 = 0x004b7090 (an own home dock sails home, else the
   point) and +0x2c = 0x00403b70 (state 2). */
int aoe_trade_action_retarget_ror(AoeTradeActionRor *action,
                                  AoeLiveUnit *target, float x, float y,
                                  float z, const AoeTradeActionOps *ops);
int aoe_trade_action_deliver_ror(AoeTradeActionRor *action,
                                 AoeLiveUnit *target, float x, float y,
                                 float z, const AoeTradeActionOps *ops);
int aoe_trade_action_stop_ror(AoeTradeActionRor *action,
                              const AoeTradeActionOps *ops);
/* +0x28 = 0x004b6af0. *result: 0 working, 1 done, 3 failed, 4 the target
   now belongs to the unit's owner (state 1, notice 0x1fb unless quiet).
   Loading takes up to the template capacity, limited by the owner's
   carried resource and the target owner's resource 9 (paid with owner
   +0x78(type, -amount), resource 9 stored directly). At home the load
   pays gold: rate = squared distance from the loading point x 0.000251256
   x 0.7 (0.35 without a loading point), doubled for civilization 15, at
   least 0.35; gold = rate x load, at most 75. Returns 0 for missing
   dependencies. */
int aoe_trade_action_update_ror(AoeTradeActionRor *action,
                                const AoeTradeActionOps *ops,
                                uint8_t *result);

#endif
