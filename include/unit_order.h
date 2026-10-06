#ifndef VITA_AOE_UNIT_ORDER_H
#define VITA_AOE_UNIT_ORDER_H

#include <stdint.h>

#include "live_unit_factory.h"
#include "unit_task.h"

/* Patched Rise of Rome 1.0a: how an ordered unit turns a task into an
   action. Action lists (+0x184) come in two classes: base list vtable
   0x00542600 (task factory +0x04 = 0x004040b0) and tribe list vtable
   0x005488a8 (+0x04 = 0x004b4600, which defers unknown types to the base
   factory). */
typedef enum {
    AOE_ACTION_SHAPE_NONE_ROR,   /* no action for this type */
    AOE_ACTION_SHAPE_TARGET_ROR, /* ctor(unit, task, target) */
    AOE_ACTION_SHAPE_POINT_ROR,  /* ctor(unit, task, x, y, z) */
    AOE_ACTION_SHAPE_TASK_ROR,   /* ctor(unit, task) */
    AOE_ACTION_SHAPE_UNIT_ROR,   /* ctor(unit) */
    /* Type 7: ctor(unit, target or x/y/z, task +0x34/+0x38 graphics, 0,
       task work_range, template min_range +0x138, projectile_unit_id
       +0x124, frame_delay +0x12a). Before construction a unit with a +0x74
       group object hears 0x0040f6d0(600) and 0x00412f00(target ID and
       position, or -1 and x/y/z). */
    AOE_ACTION_SHAPE_ATTACK_TARGET_ROR,
    AOE_ACTION_SHAPE_ATTACK_POINT_ROR
} AoeActionShapeRor;

typedef struct {
    uint32_t constructor; /* original constructor address */
    uint16_t size;        /* original allocation size */
    AoeActionShapeRor shape;
} AoeActionRouteRor;

/* Route a task's action_type (task +0x06) through the base or tribe
   factory. Types with target/point variants choose by `has_target`; type
   3 (tribe) always uses its target constructor, even for a NULL target. */
void aoe_action_route_ror(int tribe_list, int16_t action_type, int has_target,
                          AoeActionRouteRor *route);

/* Acknowledgement IDs passed to the group object's 0x0040f6c0/0x0040f6d0
   for an order's result code (jump tables 0x00406264/0x00406230).
   Unknown codes give -1/-1. */
void aoe_order_acknowledgement_ror(int16_t code, int32_t *id_6c0,
                                   int32_t *id_6d0);

typedef struct {
    AoeUnitTaskOps task;
    void *context;
    /* Whether a template's task and action lists are the tribe classes
       (template vtable 0x00549930 and its list 0x005499ac / 0x005488a8). */
    int (*tribe_template)(void *context, const DatUnit *unit_template);
    /* Owner +0x24 template table (count +0x22). */
    int16_t (*owner_template_count)(void *context, const AoeLiveUnit *unit);
    const DatUnit *(*owner_template)(void *context, const AoeLiveUnit *unit,
                                     int16_t index);
    void (*change_template)(void *context, AoeLiveUnit *unit,
                            const DatUnit *unit_template); /* +0x54 */
    /* Head action of +0x184: its task (+0x30), +0x34 retarget, +0x18 type,
       +0x4c target. */
    const DatUnitTask *(*action_task)(void *context, void *action);
    int (*action_retarget)(void *context, void *action, AoeLiveUnit *target,
                           float x, float y, float z);
    int16_t (*action_type)(void *context, void *action);
    AoeLiveUnit *(*action_target)(void *context, void *action);
    /* Allocate and construct `route` (NULL on allocation failure). */
    void *(*construct)(void *context, AoeLiveUnit *unit,
                       const AoeActionRouteRor *route,
                       const DatUnitTask *task, AoeLiveUnit *target, float x,
                       float y, float z);
    void (*adopt)(void *context, AoeLiveUnit *unit, void *action); /* +0x208 */
    void (*move)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target,
                 float x, float y, float z); /* +0x9c */
    int (*has_group_object)(void *context, const AoeLiveUnit *unit);
    /* 0x0040f6c0(id_6c0), 0x0040f6d0(id_6d0), 0x00412f50(target ID,
       target class, x, y, z), then 0x00413040(). */
    void (*acknowledge)(void *context, AoeLiveUnit *unit, int32_t id_6c0,
                        int32_t id_6d0, int32_t target_id,
                        int32_t target_class, float x, float y, float z);
} AoeUnitOrderOps;

/* 0x00406670 (a unit slot): the unit's task list (+0xd8's +4) asked for
   the target; without a task and with a task swap group (+0xec), the
   first owner template of the same class, type and swap group whose list
   has one becomes the unit's template (+0x54). 0 for a host failure;
   `task` gets the task (NULL for none). */
int aoe_unit_select_task_swap_ror(AoeLiveUnit *unit, AoeLiveUnit *target, float x, float y,
                                  float z, const AoeUnitOrderOps *ops,
                                  const DatUnitTask **task);
/* 0x00406300 (a unit slot): the template that would take the order: the
   unit's own unless, for another target and a swap group, only a swap
   template's list has a task. */
const DatUnit *aoe_unit_task_template_ror(AoeLiveUnit *unit, AoeLiveUnit *target, float x,
                                          float y, float z, const AoeUnitOrderOps *ops);

/* 0x00406420(id): the template's task with ID `id` (0x0043e6a0; -1
   finds none) builds an action through the list factory with no target
   at (-1, -1, -1), which the unit adopts (+0x208). 1 when adopted. */
int aoe_unit_start_task_ror(AoeLiveUnit *unit, int16_t id, const AoeUnitOrderOps *ops);

/* Unit vtable +0x1f8 = 0x00405f20 (reached from +0x98 / +0xa0, whose last
   flag it ignores). Nothing happens when the target is the unit itself.
   1. Select a task with the unit's list. If none and the template's
      task_swap_group is set, the first owner template with the same class,
      type and task_swap_group whose list yields a task becomes the unit's
      template.
   2. A head action built from the same task is retargeted (+0x34); on
      success its type is the result. Independently, a head action of type 9
      already aimed at this target is retargeted too and, on success,
      skips creation.
   3. Otherwise a task builds an action through the list factory, which the
      unit adopts with its type as the result; with no task or no action
      the unit moves (+0x9c) and the result is 1.
   4. A unit with a group object acknowledges the result.
   Returns 0 for missing dependencies or a host failure, else 1; *code
   receives the result (-1 when nothing was handled). */
int aoe_unit_order_ror(AoeLiveUnit *unit, AoeLiveUnit *target, float x,
                       float y, float z, const AoeUnitOrderOps *ops,
                       int16_t *code);

#endif
