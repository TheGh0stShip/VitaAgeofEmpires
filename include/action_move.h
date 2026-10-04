#ifndef VITA_AOE_ACTION_MOVE_H
#define VITA_AOE_ACTION_MOVE_H

#include <stdint.h>

#include "action_list.h"
#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a base action (vtable 0x005429d8, initialiser
   0x00407050) fields used by the move action, as semantic storage. */
typedef struct {
    int16_t type;              /* +0x04 */
    AoeLiveUnit *unit;         /* +0x08 */
    uint8_t state;             /* +0x0c */
    AoeLiveUnit *target;       /* +0x10 */
    int32_t target_id;         /* +0x18, -1 when none */
    int32_t secondary_id;      /* +0x1c, -1 when none */
    float x, y, z;             /* +0x20/+0x24/+0x28 */
    const void *graphic;       /* +0x38 */
    uint8_t quiet;             /* +0x3c: suppress unit notices */
    float range;               /* move action +0x40 */
    /* +0x34 sub-action list when the action owns one (an action nested in
       another's list); NULL uses the unit's shared list. Owned by the
       action: cleared and freed with it. */
    AoeActionList *sub_list;
} AoeMoveActionRor;

typedef struct {
    void *context;
    /* World lookup through unit +0x0c -> owner +0x3c (0x005206d0). */
    AoeLiveUnit *(*resolve)(void *context, int32_t unit_id);
    /* Base target setter 0x00407380: released targets hear +0xc4(unit),
       attached targets +0xc0(unit, action type, 0). */
    void (*target_released)(void *context, AoeLiveUnit *target,
                            AoeLiveUnit *unit);
    void (*target_attached)(void *context, AoeLiveUnit *target,
                            AoeLiveUnit *unit, int16_t action_type);
    void (*clear_secondary)(void *context, AoeMoveActionRor *action); /* +0x58 */
    /* Unit +0x148(id, id, code, value, argument, 0). */
    void (*unit_notice)(void *context, AoeLiveUnit *unit, int32_t code,
                        int32_t value, int32_t argument);
    void (*select_graphic)(void *context, AoeLiveUnit *unit,
                           const void *graphic);              /* +0x38 */
    const void *(*standing_graphic)(void *context, AoeLiveUnit *unit);
    int (*in_range)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target,
                    float range);                             /* 0x004aadf0 */
    float (*speed)(void *context, AoeLiveUnit *unit);         /* +0xfc */
    void (*order_flag)(void *context, AoeLiveUnit *unit, int32_t value);
    /* Movement system of the unit. */
    void (*set_destination)(void *context, AoeLiveUnit *unit, float x,
                            float y);          /* 0x0044be90(x, y, 1.0f) */
    /* Byte +0x174 (aoe_movement_record_waypoint_ror composes the pair). */
    int (*waypoints_stale)(void *context, AoeLiveUnit *unit);
    /* +0x1bc(tile, 0) then 0x0044edd0, when +0x174 is clear. */
    void (*record_tile)(void *context, AoeLiveUnit *unit,
                        const uint8_t tile[3]);
    void (*set_range)(void *context, AoeLiveUnit *unit, float range);
    void (*set_target_id)(void *context, AoeLiveUnit *unit, int32_t id);
    void (*set_target_size)(void *context, AoeLiveUnit *unit, float sx,
                            float sy);
    int (*start)(void *context, AoeLiveUnit *unit);      /* 0x0044bf00 */
    int (*status)(void *context, AoeLiveUnit *unit);     /* 0x0044c2d0 */
} AoeMoveActionOps;

/* 0x00407380. Returns 0 for missing dependencies. */
int aoe_action_set_target_ror(AoeMoveActionRor *action, AoeLiveUnit *target,
                              const AoeMoveActionOps *ops);

/* Constructors 0x004052d0 (target) and 0x00405250 (point): type 1, range
   <= 0 becomes 0.4f, and a NULL graphic becomes the template's moving
   graphic (template +0xbc, or +0x18 when that is NULL), supplied by the
   caller as `default_graphic`. A target's position is copied. */
int aoe_move_action_init_target_ror(AoeMoveActionRor *action,
                                    AoeLiveUnit *unit, AoeLiveUnit *target,
                                    float range, const void *graphic,
                                    const void *default_graphic,
                                    const AoeMoveActionOps *ops);
int aoe_move_action_init_point_ror(AoeMoveActionRor *action,
                                   AoeLiveUnit *unit, float x, float y,
                                   float z, float range, const void *graphic,
                                   const void *default_graphic);

/* +0x5c = 0x00405490: store the state; states 1/0xd/0xe show the standing
   graphic unless quiet; state 4 shows the action graphic. */
int aoe_move_action_set_state_ror(AoeMoveActionRor *action, uint8_t state,
                                  const AoeMoveActionOps *ops);

/* +0x1c = 0x00405430: a target already in range finishes (state 1); a unit
   whose speed is 0 or NaN gets state 0xd; otherwise state 3. */
int aoe_move_action_activate_ror(AoeMoveActionRor *action,
                                 const AoeMoveActionOps *ops);

/* +0x28 = 0x00405500. Writes the original byte result to *result: 1 done,
   2 arrived near the target, 3 arrived at the point, 4 target lost (with
   notice 0x1fb carrying the already-reset target ID, normally -1), 5
   target lost while quiet, 0 still working. Returns 0 for
   missing dependencies. */
int aoe_move_action_update_ror(AoeMoveActionRor *action,
                               const AoeMoveActionOps *ops, uint8_t *result);

#endif
