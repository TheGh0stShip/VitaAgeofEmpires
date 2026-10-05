#ifndef VITA_AOE_UNIT_VTABLE_DEFAULTS_H
#define VITA_AOE_UNIT_VTABLE_DEFAULTS_H

#include <stdint.h>

#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a: the default slots of the action unit vtable
   0x00542748 that its subclasses (0x00542e14, 0x0054324c, 0x00544838,
   0x005445e8, 0x00547fa0, 0x0054820c, ...) inherit unchanged. Each is a
   constant or a field read. */

/* +0x7c 0x00405930, +0x138 0x004059a0: false. +0x13c 0x004059b0: true. */
uint8_t aoe_unit_default_false_ror(void);
uint8_t aoe_unit_default_true_ror(void);
/* +0x90 0x00405940, +0xe4 0x00405950, +0x158 / +0x17c 0x004059f0: 0. */
int32_t aoe_unit_default_zero_ror(void);
/* +0x11c 0x00405980: -1. */
int32_t aoe_unit_default_minus_one_ror(void);
/* +0xf0 0x00405960: 1.0; +0x104 0x00405970: 0.0; +0x120 / +0x124 / +0x128
   0x00405990: -1.0. */
float aoe_unit_default_one_ror(void);
float aoe_unit_default_zero_float_ror(void);
float aoe_unit_default_minus_one_float_ror(void);
/* +0x94 0x00405d60, +0x148 0x004059c0: nothing. */
void aoe_unit_default_nothing_ror(void);
/* +0xf4 0x00405a10: the animation rate (+0x88); +0xf8 0x00405a20: the
   +0x9c rotation. */
float aoe_unit_animation_rate_ror(const AoeLiveUnit *unit);
float aoe_unit_rotation_ror(const AoeLiveUnit *unit);

/* The slots the base unit vtable 0x00542e14 adds (inherited unchanged by
   0x005434b4, 0x005468cc and 0x00548474): +0x98 / +0x9c / +0xa0
   0x00415300 nothing (aoe_unit_default_nothing_ror); +0xbc 0x00415310
   the template (+0x08); +0x118 0x00415320 0xff; +0x198 0x00415330,
   +0x19c / +0x1a0 0x00415340, +0x1a4 0x00415350 and +0x194 / +0x1ac /
   +0x1b4 0x00415360 0 (aoe_unit_default_zero_ror). */
const DatUnit *aoe_unit_template_ror(const AoeLiveUnit *unit);
uint8_t aoe_unit_default_ff_ror(void);

#endif
