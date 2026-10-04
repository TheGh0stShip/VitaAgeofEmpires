#ifndef VITA_AOE_ACTION_GATHER_H
#define VITA_AOE_ACTION_GATHER_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a gather action (vtable 0x00542598, action type
   5; routed from task action type 5 through 0x00402ef0 (target) and
   0x00402f80 (point); activation is the tribe override 0x004b33a0).

   States: 3 waits for a new target (a 10 s cycle that notices 0x202 to the
   unit), 4 walks to the resource, 6 turns to face it, 7 works it, 8 brings
   the load to a drop site (deposits there), 11/15 walk to a point, 13 failed
   (notice 0x1f9, result 3), 1 done, 2 idle. `base` holds the shared base
   fields; its move-only `graphic`/`range` are unused. */
typedef struct {
    AoeMoveActionRor base;
    AoeLiveUnit *drop_site;    /* +0x14, ID in base.secondary_id (+0x1c) */
    float timer;               /* +0x2c */
    const DatUnitTask *task;   /* +0x30 */
    int32_t target_template;   /* +0x40: target template ID or -1 */
} AoeGatherActionRor;

typedef struct {
    AoeMoveActionOps move; /* resolve, target hooks (also for the drop-site
                              setter 0x004073f0), unit_notice, select_graphic */
    /* DAT graphic ID to runtime graphic (NULL for -1/absent). */
    const void *(*graphic)(void *context, int16_t id);
    float (*tick_seconds)(void *context);                 /* world +0x84 */
    int32_t (*owner_kind)(void *context, AoeLiveUnit *unit); /* 0x00454150 */
    /* Unit +0x1f0(target, 0, 0): turned to face the target. */
    int (*face)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    float (*distance)(void *context, AoeLiveUnit *unit,
                      AoeLiveUnit *target);                /* 0x004aaeb0 */
    /* Owner player attribute table +0x50[index]. */
    float (*player_attribute)(void *context, AoeLiveUnit *unit,
                              int16_t index);
    /* Owner player +0x78(attribute, amount, flag) and +0x5c(unit ID, type,
       ftol(amount)) when depositing (flag 1 gathering, 0 hunting). */
    void (*deposit)(void *context, AoeLiveUnit *unit, int16_t type,
                    float amount, int flag);
    /* 0x004514d0 on the owner's object list with state filter 2: the
       nearest unit of either template ID to (x, y) (zone-matched when
       `unit` is passed), or NULL. */
    AoeLiveUnit *(*find_drop_site)(void *context, AoeLiveUnit *owner_unit,
                                   int16_t id1, int16_t id2, float x,
                                   float y, AoeLiveUnit *zone_unit);
    /* Sound object play 0x004a2370(1). */
    void (*play_sound)(void *context, int16_t sound_id);
    /* Game option global 0x00580e38 +0x9a8: gather a target in one go. */
    int instant_gather_option;
    /* Sub-action list +0x34: construct a move action (0x004052d0 at the
       target or 0x00405250 at the point) with the given range and graphic,
       push it (0x00404400, which activates it) and mark it quiet
       (0x004076a0(1)) after the push, or before it when `quiet_first`
       (states 11/15); 0 on allocation failure. */
    int (*start_move)(void *context, AoeGatherActionRor *action,
                      AoeLiveUnit *target, float range, const void *graphic,
                      int quiet_first);
    uint8_t (*sub_update)(void *context, AoeGatherActionRor *action);
    void (*sub_clear)(void *context, AoeGatherActionRor *action);
    /* Hunting state 12: construct the attack action 0x00401150(unit,
       target, move graphic, attack graphic, idle 0, range, template min
       range, projectile, fire frame), push it on the sub-action list and
       mark it quiet (0x004076a0(1)); 0 on allocation failure. */
    int (*start_attack)(void *context, AoeGatherActionRor *action,
                        const void *move_graphic, const void *attack_graphic,
                        float range);
} AoeGatherActionOps;

/* Unit methods used by gathering (type-70 vtable 0x0054820c):
   - +0x80 = 0x004aacc0: carried type and amount;
   - +0x84 = 0x004aace0: set or add the amount, optionally clamped to
     template +0x7c;
   - +0x138 = 0x004afb70: targets of template 45 or 50 stay gatherable
     when empty;
   - +0x13c = 0x004afb90: a victim keeps its resource unless it is class 9
     or 10 and the attacker class is neither 4 nor 11;
   - +0x140 = 0x004afbc0: 0 for classes 2 and 21 (no zone-matched drop
     site search). */
void aoe_unit_set_carried_ror(AoeLiveUnit *unit, int16_t type, float amount);
void aoe_unit_add_carried_ror(AoeLiveUnit *unit, float amount, int add,
                              int clamp);
int aoe_unit_keeps_empty_target_ror(int16_t template_id);
int aoe_unit_keeps_resource_ror(const AoeLiveUnit *victim,
                                int16_t attacker_class);
int aoe_unit_zone_drop_search_ror(const AoeLiveUnit *unit);

/* Constructors 0x00402ef0 (target: its position and template ID are
   recorded) and 0x00402f80 (point; template ID -1). */
int aoe_gather_action_init_target_ror(AoeGatherActionRor *action,
                                      AoeLiveUnit *unit,
                                      const DatUnitTask *task,
                                      AoeLiveUnit *target,
                                      const AoeGatherActionOps *ops);
int aoe_gather_action_init_point_ror(AoeGatherActionRor *action,
                                     AoeLiveUnit *unit,
                                     const DatUnitTask *task, float x,
                                     float y, float z);

/* +0x5c = 0x00402fd0. */
int aoe_gather_action_set_state_ror(AoeGatherActionRor *action,
                                    uint8_t state,
                                    const AoeGatherActionOps *ops);
/* +0x1c = 0x004b33a0. */
int aoe_gather_action_activate_ror(AoeGatherActionRor *action, int arg,
                                   const AoeGatherActionOps *ops);
/* +0x2c = 0x00403b70: idle (state 2), returning 1. */
int aoe_gather_action_stop_ror(AoeGatherActionRor *action,
                               const AoeGatherActionOps *ops);
/* +0x30 = 0x00403b80 (deliver to a drop site or walk to a point) and +0x34
   = 0x00403c40 (retarget). */
int aoe_gather_action_deliver_ror(AoeGatherActionRor *action,
                                  AoeLiveUnit *target, float x, float y,
                                  float z, const AoeGatherActionOps *ops);
int aoe_gather_action_retarget_ror(AoeGatherActionRor *action,
                                   AoeLiveUnit *target, float x, float y,
                                   float z, const AoeGatherActionOps *ops);
/* +0x28 = 0x00403480. *result: 0 working, 1 done, 3 failed, 4 target
   owned by another player. Returns 0 for missing dependencies. */
int aoe_gather_action_update_ror(AoeGatherActionRor *action,
                                 const AoeGatherActionOps *ops,
                                 uint8_t *result);

/* Hunting action (vtable 0x00548848, action type 0x6e; routed from task
   action 0x6e to 0x004b32c0 (target: position and template ID only for a
   target below state 7) / 0x004b3350 (point)). It shares the gather
   layout, activation (0x004b33a0) and delivery (+0x30 0x00403b80); the
   gather entry points dispatch to these by type, as the vtable does.
   Differences from gathering:
   - a live target (state <= 2) is attacked first (state 12, attack action
     with the task's move/proceeding graphics and work range);
   - the working graphic (task +0x3c) while gathering; notices carry
     0x265;
   - no foreign-owner check and no template 45/50 exception;
   - an unreachable drop site retries with a fresh search after the 3 s
     wait; none at all idles (owner kind 0: state 2, else 1);
   - deposits use flag 0. */
int aoe_hunt_action_init_target_ror(AoeGatherActionRor *action,
                                    AoeLiveUnit *unit,
                                    const DatUnitTask *task,
                                    AoeLiveUnit *target,
                                    const AoeGatherActionOps *ops);
int aoe_hunt_action_init_point_ror(AoeGatherActionRor *action,
                                   AoeLiveUnit *unit,
                                   const DatUnitTask *task, float x, float y,
                                   float z);
/* +0x5c = 0x004b3460. */
int aoe_hunt_action_set_state_ror(AoeGatherActionRor *action, uint8_t state,
                                  const AoeGatherActionOps *ops);
/* +0x2c = 0x004b4060: owner kind 0 idles (2), others finish (1). */
int aoe_hunt_action_stop_ror(AoeGatherActionRor *action,
                             const AoeGatherActionOps *ops);
/* +0x34 = 0x004b40a0: an own drop site with a load delivers (8); another
   target is hunted (4); none walks to the point (15). */
int aoe_hunt_action_retarget_ror(AoeGatherActionRor *action,
                                 AoeLiveUnit *target, float x, float y,
                                 float z, const AoeGatherActionOps *ops);
/* +0x28 = 0x004b39c0. *result: 0 working, 1 done, 3 failed. */
int aoe_hunt_action_update_ror(AoeGatherActionRor *action,
                               const AoeGatherActionOps *ops,
                               uint8_t *result);

#endif
