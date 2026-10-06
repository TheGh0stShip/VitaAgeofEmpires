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
       task action_range, template min_range +0x138, projectile_unit_id
       +0x124, fire_frame +0x12a). Before construction a unit with a +0x74
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

/* The six tribe-unit "order by ID" slots (vtable 0x00547fa0 +0x164.. and
   its twin at 0x0054820c): 0x004af200 (gather 0x6e, maker 0x004b32c0),
   0x004af390 (convert 0x68, 0x004b1cb0), 0x004af520 (heal 0x69,
   0x004b2c00), 0x004af6b0 (repair 0x6a, 0x004b55c0), 0x004af850 (build
   0x65, 0x004b13c0) and 0x004af9e0 (trade 0x6f, 0x004b6520). The caller
   resolves the ID (0x005206d0); a NULL target gives 0.
   1. Without `force`, a non-empty list (0x00404520) whose head (0x004045b0)
      is already of `action_type` and aimed at the target returns 1.
   2. A head of `action_type` is retargeted (+0x34) to the target and its
      position; 1 whatever the retarget answers.
   3. Repair only: a non-NULL owner template 156 (owner +0x24 +0x270)
      becomes the unit's template (+0x54).
   4. The task comes from +0x214 = 0x00406670 (with its swap); none gives
      0. The class's target maker (route of `action_type` through the tribe
      factory, allocation 0x44) builds the action; failure gives 0.
   5. The list is cleared (0x00403e50) and the unit adopts (+0x208); 1. */
/* 0x004b0870 (tribe unit repair order on a target): nothing for NULL or
   the unit itself. A unit whose template is not 156 takes owner template
   156 (+0x54, unchecked like the original). The (new) template's first
   repair task (action 0x6a) whose target class is the target's class, or
   -1/0 for a type-80 target, is the task. A head action (0x004045c0)
   built from that task and retargeted (0x00404580) gives its type;
   otherwise the list factory builds the task's action, which the unit
   adopts (+0x208), giving its type; else the result is 0. A unit with a
   group object acknowledges 0x2ce/0x26a for result 0x6a (else -1/-1),
   with the target's ID, class and position. Returns 0 for missing
   dependencies; *code gets the result. */
int aoe_unit_order_repair_ror(AoeLiveUnit *unit, AoeLiveUnit *target,
                              const AoeUnitOrderOps *ops, int16_t *code);
int aoe_unit_order_class_action_ror(AoeLiveUnit *unit, AoeLiveUnit *target,
                                    int force, int16_t action_type,
                                    const AoeUnitOrderOps *ops);


/* 0x004b14c0 (the tribe action unit's order-by-code slot): the action
   list cleared (0x00403e50) and the code stored (+0x0c). Codes: 1 and
   0xd set the template's standing graphic (+0x18); 2 does that, slots
   +0x54(0) and +0x58(0) and clears the saved point (-1.0 x3); 3 notifies
   the owner (+0x148) with (owner, owner, 0x1fa) then +0x5c(1) when the
   game mode (0x00454150) is 1, else (owner, owner, 0x202, 0x25a, 0, 0),
   and sets the standing graphic; 4 saves the target's position and
   makes a move action to it (0x004052d0 with the +0x30 record's +0x24
   range and +0x34 graphic), failing with +0x5c(0xd), else pushing it
   (0x00404400) and 0x004076a0(1); 6 sets the record's +0x34 graphic; 7
   needs a target, else +0x5c(0xd): when the target's size (+0x30 /
   +0x34) is whole, the unit and target tiles pick two tiles beside the
   diagonal between them and a type-0x1b unit on both (0x00520590 with
   the owner) answers +0x5c(0xd); otherwise the record's +0x38 graphic is
   set and its +0x44 sound played (0x004a2370(1)); 0xb makes a move to
   the saved point (0x00405250 with range 0 and the +0x34 graphic),
   failing with +0x5c(0xd), else 0x004076a0(1) and the push. */
typedef struct {
    void *context;
    void (*clear_actions)(void *context);
    void (*set_graphic)(void *context, int32_t graphic);
    int32_t standing_graphic;                 /* template +0x18 */
    int32_t record_24, record_34, record_38;  /* the +0x30 record's fields */
    int record_has_sound;                     /* its +0x44 */
    void (*slot_54)(void *context, int32_t);
    void (*slot_58)(void *context, int32_t);
    void (*slot_5c)(void *context, int32_t);
    int32_t (*game_mode)(void *context);      /* 0x00454150 */
    int16_t owner;
    void (*notify)(void *context, int16_t a, int16_t b, int32_t code, int32_t d, int32_t e, int32_t f);
    int has_target;
    float target_x, target_y, target_z;
    float target_size_x, target_size_y;       /* target template +0x30 / +0x34 */
    float unit_x, unit_y;
    int (*make_move_to_target)(void *context, int32_t range, int32_t graphic); /* 0x004052d0 */
    int (*make_move_to_point)(void *context, float x, float y, float z, int32_t graphic); /* 0x00405250 */
    void (*push_action)(void *context);       /* 0x00404400 */
    void (*action_hook)(void *context, int32_t); /* 0x004076a0 */
    int32_t (*find_unit)(void *context, int16_t owner, int32_t type, int32_t x, int32_t y); /* 0x00520590 */
    void (*play_sound)(void *context);        /* 0x004a2370(1) */
} AoeUnitOrderCodeOpsRor;
typedef struct { uint8_t code; float saved_x, saved_y, saved_z; } AoeUnitOrderCodeStateRor;
void aoe_unit_order_by_code_ror(AoeUnitOrderCodeStateRor *st, const AoeUnitOrderCodeOpsRor *o,
                                uint8_t code);

#endif
