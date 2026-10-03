#ifndef VITA_AOE_UNIT_TASK_H
#define VITA_AOE_UNIT_TASK_H

#include <stdint.h>

#include "dat.h"
#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a unit task selection: which DAT task an order
   on `target` (or the map point x/y) means for `actor`. The actor template's
   task-list object (+0xd8) holds the template's DatUnitTask records in
   order. Base templates (factory +0x38 = 0x0043e4d0, list vtable
   0x005498e4) select with 0x004e7dc0. Tribe templates (vtable 0x00549930,
   factory 0x004ece50, list vtable 0x005499ac) select with 0x00508010, which
   falls back to 0x004e7dc0. Owners are compared by player ID (+0x4a); the
   original compares owner pointers, and each player has one ID. */
typedef struct {
    void *context;
    int16_t (*owner_id)(void *context, const AoeLiveUnit *unit);
    /* Player vtable +0x1c(other_id), called on `player_id`. */
    int (*owner_accepts)(void *context, int16_t player_id, int16_t other_id);
    /* 0x0045da60(other_id), called on `player_id`. */
    int (*owner_hostile)(void *context, int16_t player_id, int16_t other_id);
    /* Actor owner +0x3c -> world +0x28 -> map: +0x08/+0x0c sizes, and
       terrain byte from 0x00447de0(x, y). Return 0 if the host map cannot
       answer (the original reads unchecked). */
    int32_t (*map_size)(void *context, const AoeLiveUnit *actor, int axis);
    int (*terrain_at)(void *context, const AoeLiveUnit *actor, int16_t x,
                      int16_t y, uint8_t *terrain);
} AoeUnitTaskOps;

/* List vtable +0x0c = 0x004e7f90. With requires_carried_resource == 1 the
   target must carry a resource type other than -1 with amount > 0. Then
   owner_mode: 1 same owner; 2 different, non-gaia owner the actor's owner
   finds hostile; 3 gaia target; 4 same owner (accepted immediately, even
   for the actor itself) or mutual acceptance through +0x1c; 5 different
   owner, either hostile or gaia with actor template field_8c set. Any
   other mode adds no constraint. Finally the target must not be the actor. */
int aoe_unit_task_allowed_ror(const DatUnitTask *task,
                              const AoeLiveUnit *actor,
                              const AoeLiveUnit *target,
                              const AoeUnitTaskOps *ops, int *allowed);

/* 0x004e7dc0. With a target, four passes each return the first allowed
   task matching: (state-0 target) unbuilt_target == 1; target_unit ==
   target template id; target_class == target unit_class; (target state
   < 3 and target template field_8c) enable_targeting. Then, unless x or y
   is -1.0 or NaN, the point (__ftol truncated to 16 bits) inside the map
   returns the first task whose target_terrain equals the terrain byte (no
   predicate). Otherwise *task = NULL. Returns 0 for missing dependencies
   or a failed host lookup. */
int aoe_unit_task_select_base_ror(const AoeLiveUnit *actor,
                                  const AoeLiveUnit *target, float x,
                                  float y, const AoeUnitTaskOps *ops,
                                  const DatUnitTask **task);

/* 0x00508010, the tribe "smart order":
   1. Drop-off: an actor carrying a resource (type != -1, amount > 0) of
      class 4 or 21, ordered onto its own owner's live (state 2) storage
      building, gets the first task whose resource_in equals the carried
      type and whose action is 5, 0x6e or 0x6f. Class 4 accepts building
      IDs 109/103 for resources 1, 2, 3, 15, 17 and 109/68 for resource 16;
      class 21 accepts 45 for resources 9 and 17.
   2. With a target (each rule returns NULL if no task matches):
      own state-0 building -> action 0x65; allied (same or mutual)
      target with state < 3 and template field_8f == 7 -> action 3;
      own or accepted damaged building (hp < max) with state < 3 and
      template id != 50 -> action 0x6a whose target_class is -1 or 0;
      allied damaged class 2/20/21/22 target with state < 3 -> action
      0x6a whose target_class equals that class.
   3. Class-18 actors: a live non-gaia target of another, unaccepted owner
      whose class is not 1 -> action 0x68; else an own or accepted damaged
      live target outside classes 1, 2, 3, 20, 21, 22, 27 -> action 0x69;
      otherwise NULL. They never use the base selector.
   4. Everything else: aoe_unit_task_select_base_ror. */
int aoe_unit_task_select_tribe_ror(const AoeLiveUnit *actor,
                                   const AoeLiveUnit *target, float x,
                                   float y, const AoeUnitTaskOps *ops,
                                   const DatUnitTask **task);

/* Variants that read the task list of `list_template` while every other
   check still uses the actor's current template (0x00405f20 asks other
   templates' lists before switching the unit's template). */
int aoe_unit_task_select_base_with_ror(const DatUnit *list_template,
                                       const AoeLiveUnit *actor,
                                       const AoeLiveUnit *target, float x,
                                       float y, const AoeUnitTaskOps *ops,
                                       const DatUnitTask **task);
int aoe_unit_task_select_tribe_with_ror(const DatUnit *list_template,
                                        const AoeLiveUnit *actor,
                                        const AoeLiveUnit *target, float x,
                                        float y, const AoeUnitTaskOps *ops,
                                        const DatUnitTask **task);

#endif
