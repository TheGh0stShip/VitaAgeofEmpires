#ifndef VITA_AOE_ACTION_FARM_H
#define VITA_AOE_ACTION_FARM_H

#include <stdint.h>

#include "action_move.h"

/* Patched Rise of Rome 1.0a farm action (vtable 0x0054261c, action type
   0x15, 0x40 bytes; constructor 0x00404750(unit, task)). A farm building
   builds it as its default task (template +0xdc). It grows the farm's own
   carried food (+0x44) for gatherers until the owner's farm allowance is
   used up, then retires the farm. States: 6 producing, 1 done. */
typedef struct {
    AoeMoveActionRor base;   /* base action 0x00406ea0(unit, 1) */
    float remaining;         /* +0x2c: owner attribute 36 at set_state */
    const DatUnitTask *task; /* +0x30 */
} AoeFarmActionRor;

typedef struct {
    void *context;
    float (*tick_seconds)(void *context);                 /* world +0x84 */
    /* Owner +0x50 resources[index]. */
    float (*player_attribute)(void *context, AoeLiveUnit *unit,
                              int16_t index);
    /* Owner player +0x78(type, amount, 0). */
    void (*deposit)(void *context, AoeLiveUnit *unit, int16_t type,
                    float amount);
    /* Global +0x40(0x7e, owner ID, unit ID, tile x, tile y). */
    void (*notify_depleted)(void *context, AoeLiveUnit *unit);
    void (*destroy)(void *context, AoeLiveUnit *unit);    /* unit +0x6c */
} AoeFarmActionOps;

int aoe_farm_action_init_ror(AoeFarmActionRor *action, AoeLiveUnit *unit,
                             const DatUnitTask *task);
/* +0x5c = 0x004047b0: the state, and +0x2c from owner attribute 36. */
int aoe_farm_action_set_state_ror(AoeFarmActionRor *action, uint8_t state,
                                  const AoeFarmActionOps *ops);
/* +0x1c = 0x00404780: state 6; a unit not carrying the task's resource-in
   type (when >= 0) switches to it with amount 0 (+0x80). */
int aoe_farm_action_activate_ror(AoeFarmActionRor *action,
                                 const AoeFarmActionOps *ops);
/* +0x28 = 0x004047d0. State 1 reports 1. In state 6:
   - while +0x2c > 0, or the owner's attribute 36 is <= 0 (or NaN), it
     produces tick x template work rate (+0xe4) x task work value 1:
     straight to the owner (+0x78) when task +0x30 is set, otherwise into
     the unit's carried amount (+0x84 add, clamped to template +0x7c), and
     +0x2c drops by the carried amount actually gained;
   - otherwise, once the unit carries nothing (<= 0 or NaN), it notices
     0x7e and destroys the unit.
   *result is 0 except in state 1. Returns 0 for missing dependencies. */
int aoe_farm_action_update_ror(AoeFarmActionRor *action,
                               const AoeFarmActionOps *ops, uint8_t *result);

#endif
