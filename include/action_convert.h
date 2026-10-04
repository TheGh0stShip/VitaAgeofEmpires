#ifndef VITA_AOE_ACTION_CONVERT_H
#define VITA_AOE_ACTION_CONVERT_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a conversion action (vtable 0x0054871c, action
   type 0x68; routed from task action 0x68 to 0x004b1cb0 (target) /
   0x004b1d40 (point)). States: 1 done, 2 cleared, 3 idle (notice 0x202),
   4 walks to the target (range +0x44), 6 faces it, 7 converts, 11 walks
   to a point, 13 failed (notice 0x1f9, result 3). Notices carry value
   0x25c. The priest's faith is its carried amount (+0x44, resource 0x22),
   which must reach 100 to start. */
typedef struct {
    AoeMoveActionRor base;
    float timer;             /* +0x2c: time to the next attempt */
    const DatUnitTask *task; /* +0x30 */
    uint8_t same_owner;      /* +0x40: target had the unit's owner */
    float range;             /* +0x44: 1.0 for buildings, else max range */
} AoeConvertActionRor;

typedef struct {
    AoeMoveActionOps move;
    const void *(*graphic)(void *context, int16_t id);
    float (*tick_seconds)(void *context);                    /* world +0x84 */
    int (*face)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    /* Owner +0x1c(target owner index): 1 when allied or the same player. */
    int (*allied)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target);
    int32_t (*owner_kind)(void *context, AoeLiveUnit *unit); /* 0x00454150 */
    /* Owner resource (+0x50 floats). */
    float (*resource)(void *context, AoeLiveUnit *unit, int16_t type);
    /* Game +0x40(event, a, b, c, d). */
    void (*event)(void *context, uint32_t event, int32_t a, int32_t b,
                  int32_t c, int32_t d);
    uint16_t (*rand)(void *context);                          /* 0x0052605d */
    uint8_t (*civ)(void *context, AoeLiveUnit *unit);  /* owner +0x109 */
    /* Target +0x44(new owner) (aoe_live_unit_change_owner_ror). */
    int (*change_owner)(void *context, AoeLiveUnit *target, int16_t owner);
    /* Owner +0x78(type, amount, 0). */
    void (*resource_add)(void *context, AoeLiveUnit *unit, int16_t type,
                         float amount);
    /* Unit +0x80(type, amount): carried resource. */
    void (*set_carried)(void *context, AoeLiveUnit *unit, int16_t type,
                        float amount);
    /* Target owner +0x94(0x70, x, y, z, 0): a flare (template 112) at the
       converting unit for the target's owner. */
    void (*flare)(void *context, AoeLiveUnit *target, float x, float y,
                  float z);
    /* Target +0x148(unit ID, target ID, 0x1f4, unit ID, ftol hp, max). */
    void (*target_notice)(void *context, AoeLiveUnit *target,
                          AoeLiveUnit *unit);
    void (*play_sound)(void *context, int16_t sound_id);
    int (*start_move)(void *context, AoeMoveActionRor *action,
                      AoeLiveUnit *target, float range, const void *graphic,
                      int quiet_first);
    uint8_t (*sub_update)(void *context, AoeMoveActionRor *action);
    void (*sub_clear)(void *context, AoeMoveActionRor *action);
} AoeConvertActionOps;

/* 0x004b1cb0 (a target's position is copied when present) / 0x004b1d40.
   Range starts at 1.0. */
int aoe_convert_action_init_target_ror(AoeConvertActionRor *action,
                                       AoeLiveUnit *unit,
                                       const DatUnitTask *task,
                                       AoeLiveUnit *target,
                                       const AoeMoveActionOps *ops);
int aoe_convert_action_init_point_ror(AoeConvertActionRor *action,
                                      AoeLiveUnit *unit,
                                      const DatUnitTask *task, float x,
                                      float y, float z);
/* +0x5c = 0x004b2020. State 7 alerts the target's owner (a flare, for
   players above Gaia) and sets the timer to the task's work value 2. */
int aoe_convert_action_set_state_ror(AoeConvertActionRor *action,
                                     uint8_t state,
                                     const AoeConvertActionOps *ops);
/* +0x1c = 0x004b1dc0. With a target:
   - town centres (109), walls (class 27) and templates 158, 159, 163 and
     276 cannot be converted: owner kind 1 walks to the point (11), others
     fail (13); event 0x79 (town centre) or 0x7a (276) follows;
   - faith below 100: 13 and event 0x76;
   - buildings (class 3) need resource 28 above 0 (range 1.0; else 11/13
     and event 0x78); class 30 always fails (13); priests (class 18) need
     resource 27 above 0 (else 11/13 and event 0x77); other units use the
     template's max range.
   Then `active` with a target walks (4); otherwise idle (3). */
int aoe_convert_action_activate_ror(AoeConvertActionRor *action, int active,
                                    const AoeConvertActionOps *ops);
/* +0x34 = 0x004b26a0 (shared with healing). */
int aoe_convert_action_retarget_ror(AoeConvertActionRor *action,
                                    AoeLiveUnit *target,
                                    const AoeConvertActionOps *ops,
                                    int *accepted);
/* +0x28 = 0x004b2280. *result: 0 working, 1 done, 3 failed, 4 the target
   became allied (faith spent, state 1, notice 0x1fb). In state 7 the
   target hears 0x1f4 each update; when the timer runs out a flare is
   sent and the attempt is rand() / 32767 (x4 against civilization 16, x2
   for boats and priests, x8 for classes 23 and 35) against work rate x
   task work value: success converts the target (event 0x7b, resource 41
   +1, faith 0, states 3 then 4); failure walks on (4). Returns 0 for
   missing dependencies. */
int aoe_convert_action_update_ror(AoeConvertActionRor *action,
                                  const AoeConvertActionOps *ops,
                                  uint8_t *result);

#endif
