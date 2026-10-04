#ifndef VITA_AOE_ACTION_REPAIR_H
#define VITA_AOE_ACTION_REPAIR_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a repair action (vtable 0x00548924, action type
   0x6a; routed from task action 0x6a to 0x004b55c0 (target) / 0x004b5650
   (point)). States:
   - 1 done; 2 cleared; 3 idle (notice 0x202);
   - 4 walks to the target (template max range);
   - 6 faces it; 7 repairs;
   - 10 waits for a boat to come ashore;
   - 11 walks to a point; 13 failed (notice 0x1f9, result 3).
   Notices go through unit +0x148 with value 0x26a. */
typedef struct {
    AoeMoveActionRor base;
    float timer;             /* +0x2c: state-10 recheck countdown */
    const DatUnitTask *task; /* +0x30 */
    uint8_t boat_order;      /* +0x40: the boat's order counter (+0x188)
                                when this action called it ashore */
} AoeRepairActionRor;

typedef struct AoeRepairActionOps AoeRepairActionOps;
struct AoeRepairActionOps {
    AoeMoveActionOps move; /* resolve, target hooks, unit_notice,
                              select_graphic, clear_secondary */
    const void *(*graphic)(void *context, int16_t id);
    float (*tick_seconds)(void *context);                    /* world +0x84 */
    double (*distance)(void *context, AoeLiveUnit *unit,
                       AoeLiveUnit *target);                 /* 0x004aaeb0 */
    /* Unit +0x1f0(target, 0, 0): nonzero once facing it. */
    int (*face)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    /* Owner +0x1c(target's owner index): nonzero keeps the target. */
    int (*may_repair)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    /* Target +0x8c(amount) (aoe_live_unit_repair_ror): 0 working, 1
       repaired, 2 the owner cannot pay. */
    uint8_t (*repair)(void *context, AoeLiveUnit *target, float amount);
    /* Game +0x40(event, owner index, target ID, 0, 0). */
    void (*event)(void *context, uint32_t event, int16_t owner,
                  int32_t target_id);
    void (*play_sound)(void *context, int16_t sound_id);      /* 0x004a2370 */
    /* The sub-action list +0x34 (as for building). */
    int (*start_move)(void *context, AoeRepairActionRor *action,
                      AoeLiveUnit *target, float range, const void *graphic,
                      int quiet_first);
    uint8_t (*sub_update)(void *context, AoeRepairActionRor *action);
    void (*sub_clear)(void *context, AoeRepairActionRor *action);
    /* Boat meeting point (0x004b5e80). */
    uint8_t (*order_serial)(void *context, AoeLiveUnit *unit); /* +0x188 */
    /* Owner list +0x28 in order. */
    AoeLiveUnit *(*owner_unit_at)(void *context, AoeLiveUnit *unit,
                                  uint32_t index);
    /* Head action (+0x184) queries: +0xcc target (+0x4c), +0x134 state
       (0 without a head), and the head's destination 0x00407670 /
       0x00407680 (x, y). */
    AoeLiveUnit *(*head_target)(void *context, AoeLiveUnit *unit);
    uint8_t (*head_state)(void *context, AoeLiveUnit *unit);
    void (*head_destination)(void *context, AoeLiveUnit *unit, float *x,
                             float *y);
    /* Unit +0x1b4 inputs: terrain byte at a tile (0x00447de0) and unit
       +0x114(x, y, 0), whether the unit fits at a point. */
    int (*terrain_at)(void *context, int32_t x, int32_t y);
    int (*fits)(void *context, AoeLiveUnit *unit, float x, float y);
    /* Unit +0x154(x, y, z, range, force) (0x004272b0). */
    int (*move_to)(void *context, AoeLiveUnit *unit, float x, float y,
                   float z, float range, int force);
};

/* Unit +0x1b4 for types 30+ (0x0044e920): steps from tile `from` towards
   `to` in 2 x max(|dx|, |dy|) equal float steps (ftol of the larger
   axis), skipping repeated tiles; the first tile whose terrain is
   `terrain_a` or `terrain_b`, when `require_fit` is 1 and the unit fits
   at the stepped point, is written to `out`. Equal tiles, or no match,
   return 0. */
int aoe_unit_line_terrain_point_ror(AoeLiveUnit *unit, const int32_t from[2],
                                    const int32_t to[2], int32_t out[2],
                                    int terrain_a, int terrain_b,
                                    int require_fit,
                                    const AoeRepairActionOps *ops);

/* 0x004b55c0 (target: its position, then the boat meeting point) and
   0x004b5650 (point). */
int aoe_repair_action_init_target_ror(AoeRepairActionRor *action,
                                      AoeLiveUnit *unit,
                                      const DatUnitTask *task,
                                      AoeLiveUnit *target,
                                      const AoeRepairActionOps *ops);
int aoe_repair_action_init_point_ror(AoeRepairActionRor *action,
                                     AoeLiveUnit *unit,
                                     const DatUnitTask *task, float x,
                                     float y, float z);
/* 0x004b5e80: for a boat target (class 2, 20, 21 or 22) without another
   working repairer (template 156) whose head target it is, a beach
   (terrain 2) tile on the line from the unit to the boat where the unit
   fits becomes the action point (tile + 0.5) and the boat is ordered
   there (+0x154, forced); its order counter is remembered. When another
   repairer is busy with it, the action point becomes the boat's walk
   destination (head state 11) or position. */
int aoe_repair_action_meet_boat_ror(AoeRepairActionRor *action,
                                    const AoeRepairActionOps *ops);
/* +0x5c = 0x004b5750. */
int aoe_repair_action_set_state_ror(AoeRepairActionRor *action,
                                    uint8_t state,
                                    const AoeRepairActionOps *ops);
/* +0x1c = 0x004b5720: with `arg` and a target, walk to it (4); else 3. */
int aoe_repair_action_activate_ror(AoeRepairActionRor *action, int arg,
                                   const AoeRepairActionOps *ops);
/* +0x34 = 0x004b5df0 (a new target: meeting point, then walk; the same
   target is kept in states 4/6; none: walk to the point) and +0x30 =
   0x004b5db0 (walk to the point). */
int aoe_repair_action_retarget_ror(AoeRepairActionRor *action,
                                   AoeLiveUnit *target, float x, float y,
                                   float z, const AoeRepairActionOps *ops);
int aoe_repair_action_walk_ror(AoeRepairActionRor *action, float x, float y,
                               float z, const AoeRepairActionOps *ops);
/* +0x28 = 0x004b5a50. *result: 0 working, 1 done, 3 failed. A lost,
   inactive (state != 2) or refused target ends the action (state 1).
   Returns 0 for missing dependencies. */
int aoe_repair_action_update_ror(AoeRepairActionRor *action,
                                 const AoeRepairActionOps *ops,
                                 uint8_t *result);

/* Pay-for-repair callback of the +0x8c methods: owner 0x004f1750(template
   ID, multiplier, population flag). Nonzero when paid. */
typedef int (*AoeRepairPayFn)(void *context, AoeLiveUnit *unit,
                              float multiplier, int pop_flag);

/* Unit +0x8c, hit points from repair:
   - root 0x004aad30: hp += amount (a negative amount stops at 1.0); at
     or above the template maximum hp is clamped and 1 is returned;
   - type 70 0x004b0780: boats (class 2, 20, 21, 22) pay first, other
     units repair for free;
   - type 80 0x004acb70: buildings always pay.
   Paying units gain g = maximum x amount / train time (rounded to float,
   capped at the missing hit points) for multiplier g / (2 x maximum)
   (population flag 1 for boats, 0 for buildings); refusal returns 2.
   Returns 0 still damaged, 1 repaired, 2 unpaid; -1 for bad input, a
   paying unit without `pay`, or a zero train time (the original divides
   by it). */
int aoe_live_unit_repair_ror(AoeLiveUnit *unit, float amount,
                             AoeRepairPayFn pay, void *context);

#endif
