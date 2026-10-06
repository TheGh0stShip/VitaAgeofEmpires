#ifndef VITA_AOE_PLAYER_COSTS_H
#define VITA_AOE_PLAYER_COSTS_H

#include <stdint.h>

#include "dat.h"
#include "player_resources.h"

/* Patched Rise of Rome 1.0a tribe-player cost methods, `this` = player:
   0x004f15f0 check, 0x004f1750 pay, 0x004f17f0 refund. The template comes
   from player +0x24[index]; its three costs are at +0x148 (type, amount,
   deduct byte at +0x14c), and only the low byte of DatUnitCost.paid is
   read. Values are player +0x50 floats. x87 stack temporaries are modelled
   as double, assuming the MSVC default 53-bit precision control.

   Check (0x004f15f0): sets *failing = -1. A gold budget G = values[3] is
   available only when values[29] >= 1.0; otherwise G = 0. A missing
   template, or template type < 70, fails with -1. For each cost with a
   nonnegative type, cost = (float)(amount * mult):
   1. type 4 with pop_flag 0 fails with 0x20 when values[11] >= values[32];
   2. when G is enabled, type 3 also does G -= cost and fails with 3 if
      G < 0;
   3. if values[type] < cost (or NaN): type 4 with pop_flag set is
      skipped; otherwise a cost without the deduct byte fails with its
      type, as does G + values[type] < cost; otherwise
      G -= cost - values[type].

   The result goes to *affordable. All functions return 0 only for missing
   dependencies or for a referenced resource index outside the host
   allocation (a host-only guard: the original reads unchecked). */
int aoe_player_cost_check_ror(const AoePlayerResources *resources,
                              const DatUnit *unit_template, float mult,
                              int pop_flag, int16_t *failing,
                              int *affordable);

/* 0x004f1750: re-runs the check with a NULL failure pointer. If
   affordable, each nonnegative-type cost whose deduct byte is set (type 4
   skipped when pop_flag is nonzero) is charged with an unrounded
   amount * mult: if that exceeds the value, gold (values[3]) -= cost -
   value and the value becomes 0; otherwise the value -= cost. */
int aoe_player_cost_pay_ror(AoePlayerResources *resources,
                            const DatUnit *unit_template, float mult,
                            int pop_flag, int *paid);

/* 0x004f17f0: for template type >= 70, every nonnegative-type cost with
   the deduct byte set gets (float)amount added back. The multiplier is not
   used and population is refunded too. */
int aoe_player_cost_refund_ror(AoePlayerResources *resources,
                               const DatUnit *unit_template);

/* Ready-made callbacks for AoeBuildingQueueAddOps.can_afford/pay and
   AoeBuildingQueueCancelOps.refund_one, with `context` pointing at this
   binding. The queue passes 1.0f and pop flag 1 (0x004adab0). A host-guard
   failure in the check is reported as unaffordable with resource -1. */
typedef struct {
    AoePlayerResources *resources;          /* owner +0x50 */
    void *context;
    const DatUnit *(*unit_template)(void *context, int16_t index); /* +0x24 */
} AoePlayerCostBindingRor;

int aoe_player_cost_queue_afford_ror(void *binding, int16_t unit_id,
                                     int16_t *failing);
void aoe_player_cost_queue_pay_ror(void *binding, int16_t unit_id);
void aoe_player_cost_queue_refund_ror(void *binding, int16_t unit_id);


/* 0x004f0830: the costs to show for a template (the player's +0x24
   entry): all three pairs start as (-1, 0); nothing more for no template
   or a template type below 70. Each cost with the deduct byte and a
   nonnegative type shows (int16)trunc(amount * mult); when the check
   (0x004f15f0 with no failure pointer, multiplier 1 and pop flag 0)
   passes and the value is below that amount, the amount shown becomes
   the truncated value and the difference is summed. A positive sum is
   added to the first pair that is gold (3) or unused (made gold), else
   the second; else it replaces the third when that is gold or unused.
   Pairs whose amount is below 1 get type -1. (The template's +0x08 that
   the original can also return is not modelled.) Returns 0 only for the
   host guards of the check. */
int aoe_player_cost_shown_ror(const AoePlayerResources *resources,
                              const DatUnit *unit_template, float mult,
                              int16_t types[3], int16_t amounts[3]);
/* 0x004f1ff0: the sum of the three shown amounts at multiplier 1. */
int32_t aoe_player_cost_shown_total_ror(const AoePlayerResources *resources,
                                        const DatUnit *unit_template);

#endif
