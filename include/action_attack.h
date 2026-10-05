#ifndef VITA_AOE_ACTION_ATTACK_H
#define VITA_AOE_ACTION_ATTACK_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a attack action (vtable 0x005423f8, action type
   9; routed from task action type 7 through 0x00401150 / 0x00401290).
   `base` carries the shared base action fields (+0x04..+0x3c); its
   move-only `graphic` and `range` are unused. */
typedef struct {
    AoeMoveActionRor base;
    float timer;                /* +0x2c: accumulates the world tick */
    const void *move_graphic;   /* +0x40 */
    const void *attack_graphic; /* +0x44 */
    const void *idle_graphic;   /* +0x48 */
    float range;                /* +0x4c */
    float min_range;            /* +0x50 */
    int16_t projectile_id;      /* +0x54, -1 for melee */
    int16_t fire_frame;         /* +0x56; 0 means half the sprite frames */
    uint8_t firing;             /* +0x58 */
    uint8_t friendly;           /* +0x59: target shares the unit's owner */
    /* +0x5a: 2 for point attacks, decremented by each approach (state 4)
       and cleared on retargeting; a point attack whose count reached 0
       finishes after its next shot. */
    uint8_t approach_left;
} AoeAttackActionRor;

typedef struct {
    AoeMoveActionOps move; /* resolve, target setter hooks, clear_secondary,
                              unit_notice, select_graphic, speed */
    /* Unit vtable +0x1f0(target, x, y): may this unit attack it at all. */
    int (*can_attack)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target,
                      float x, float y);
    float (*distance_to_point)(void *context, AoeLiveUnit *unit, float x,
                               float y, float z);          /* 0x004aaf50 */
    float (*distance_to_target)(void *context, AoeLiveUnit *unit,
                                AoeLiveUnit *target);      /* 0x004aaeb0 */
    /* Unit +0x74 group object present and its 0x0040f6b0() == 0x2d5. */
    int (*group_holds)(void *context, AoeLiveUnit *unit);
    /* Tower templates with a group: group 0x00412f00(-1, -1, -1, -1). */
    void (*group_clear_target)(void *context, AoeLiveUnit *unit);
    /* Owner player +0x4a of a unit (0 for Gaia). */
    int16_t (*owner_id)(void *context, AoeLiveUnit *unit);
    /* Unit owner's vtable +0x1c(other player ID), asked only when the
       unit's owner ID is nonzero: nonzero stops the attack. */
    int (*refuses_owner)(void *context, AoeLiveUnit *unit, int16_t other);
    /* Unit player +0x38 visibility map 0x00517670(x, y): 0 unseen, 0x80
       explored but fogged. */
    uint8_t (*visibility)(void *context, AoeLiveUnit *unit, int32_t x,
                          int32_t y);
    int (*attached)(void *context, AoeLiveUnit *unit);   /* unit +0x20 */
    uint8_t (*ready)(void *context, AoeLiveUnit *unit);  /* vtable +0x7c */
    void (*reset_reload)(void *context, AoeLiveUnit *unit); /* 0x00426a10 */
    /* Unit +0x18 sprite list on sprite +0x10: +0x24 (animation done == 1)
       and +0x20(frame) (frame reached). */
    uint8_t (*sprite_done)(void *context, AoeLiveUnit *unit);
    uint8_t (*sprite_reached)(void *context, AoeLiveUnit *unit,
                              int32_t frame);
    int16_t (*sprite_frames)(void *context, AoeLiveUnit *unit); /* +0x5e */
    uint8_t (*sprite_flags)(void *context, AoeLiveUnit *unit);  /* +0x70 */
    void (*fire_target)(void *context, AoeLiveUnit *unit,
                        AoeLiveUnit *target);                   /* +0x228 */
    void (*fire_point)(void *context, AoeLiveUnit *unit, float x, float y,
                       float z);                                /* +0x224 */
    /* Sub-action list +0x34: construct a quiet move action (0x004052d0 on
       the target, or 0x00405250 on the point, with range +0x4c and graphic
       +0x40) and push it (0x00404400); 0 when allocation failed. */
    int (*start_move)(void *context, AoeAttackActionRor *action);
    uint8_t (*sub_update)(void *context, AoeAttackActionRor *action);
    void (*sub_clear)(void *context, AoeAttackActionRor *action);
    float (*tick_seconds)(void *context);                 /* world +0x84 */
} AoeAttackActionOps;

/* Constructors 0x00401150 (target) and 0x00401290 (point). Graphic
   defaults: move = template +0xc0 or +0xbc, attack = +0xfc, idle = +0x18
   (supplied by the caller); a range of exactly 0 (or NaN) becomes template
   +0x114 (`default_range`), and 0.4 if that is 0 too. A target attack
   caches the position as (-1, -1, -1) and flags a same-owner target. */
typedef struct {
    const void *move_graphic, *attack_graphic, *idle_graphic;
    float range, min_range;
    int16_t projectile_id, fire_frame;
} AoeAttackParamsRor;

int aoe_attack_action_init_target_ror(AoeAttackActionRor *action,
                                      AoeLiveUnit *unit, AoeLiveUnit *target,
                                      const AoeAttackParamsRor *params,
                                      const AoeAttackParamsRor *defaults,
                                      float default_range,
                                      const AoeAttackActionOps *ops);
int aoe_attack_action_init_point_ror(AoeAttackActionRor *action,
                                     AoeLiveUnit *unit, float x, float y,
                                     float z,
                                     const AoeAttackParamsRor *params,
                                     const AoeAttackParamsRor *defaults,
                                     float default_range);

/* +0x5c = 0x00401520: clear the sub-actions, store the state, then
   - 1, 0xd, 0xe: towers (template IDs 79, 199, 69, 278) clear their
     group's target; show the idle graphic;
   - 2: approach_left = 0;
   - 4: spend one approach and push a move toward the target or point
     (a failed allocation notices 0x1f9/0x262 unless quiet and stops);
   - 6: in range (0x00401b60) resets the reload and enters 7, otherwise
     shows the attack graphic for melee or the idle one for ranged units;
   - 7: attack graphic, firing = 1. */
int aoe_attack_action_set_state_ror(AoeAttackActionRor *action, uint8_t state,
                                    const AoeAttackActionOps *ops);

/* 0x00401b60: the range check behind activation and states 6/7. Writes 0
   (too close), 1 (in range) or 2 (too far) to *where, may change the state
   (approach 4, or 0xd when the unit cannot move or its group holds), and
   returns 1 only when in range and the unit is ready (+0x7c). A target
   still within 0.5 of the cached position counts as in range without
   measuring; a measured hit refreshes that cache. Returns -1 for missing
   dependencies (host-only). */
int aoe_attack_action_check_range_ror(AoeAttackActionRor *action,
                                      int32_t *where,
                                      const AoeAttackActionOps *ops);

/* +0x1c = 0x00401480: a mobile unit without a holding group approaches
   (4); otherwise in range (or merely ready to aim) enters 6, else 0xd. */
int aoe_attack_action_activate_ror(AoeAttackActionRor *action,
                                   const AoeAttackActionOps *ops);

/* +0x34 = 0x00401b10: switch to a new target. The same target in states
   4/6/7 is kept; otherwise approach_left = 0, set the target, state 4.
   Returns 0 for a NULL target. */
int aoe_attack_action_retarget_ror(AoeAttackActionRor *action,
                                   AoeLiveUnit *target,
                                   const AoeAttackActionOps *ops);

/* +0x2c = 0x00401510: stop (state 1), returning 1. */
int aoe_attack_action_stop_ror(AoeAttackActionRor *action,
                               const AoeAttackActionOps *ops);

/* +0x28 = 0x004017c0. *result: 0 working, 1 done (state 1), 3 could not
   reach (0xd), 4 target lost or refused, 5 target gone or dead (quiet
   attacks also use 5 where loud ones return 2). Returns 0 for missing
   dependencies. */
int aoe_attack_action_update_ror(AoeAttackActionRor *action,
                                 const AoeAttackActionOps *ops,
                                 uint8_t *result);

/* +0x44 = 0x00401fe0: after a template or task change, swap graphics that
   still match the old task (+0x34 move, +0x38/+0x3c attack and idle) or
   the old template (+0xbc, +0xfc, +0x18) for the new ones. */
typedef struct {
    const void *move, *proceed, *work; /* task +0x34/+0x38/+0x3c */
} AoeAttackTaskGraphicsRor;
typedef struct {
    const void *walking, *attack, *standing; /* template +0xbc/+0xfc/+0x18 */
} AoeAttackTemplateGraphicsRor;
void aoe_attack_action_swap_graphics_ror(
    AoeAttackActionRor *action, const AoeAttackTemplateGraphicsRor *old_t,
    const AoeAttackTemplateGraphicsRor *new_t,
    const AoeAttackTaskGraphicsRor *old_task,
    const AoeAttackTaskGraphicsRor *new_task);

#endif
