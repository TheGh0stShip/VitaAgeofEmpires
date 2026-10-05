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

/* Action unit vtable 0x00542748 (and every subclass): +0x100 / +0x108 /
   +0x10c / +0x110 (base unit +0xf8 / +0xfc too) 0x004a7080 answer 0.0
   (aoe_unit_default_zero_float_ror); +0x1e0 0x004a7090 a new empty
   object list (NULL when out of memory). */
AoeObjectUpdateList *aoe_unit_new_list_ror(void);

/* Action unit +0x98 0x00405d70: a notice from another unit goes to
   +0x1f8(other, a, b, c, 0) and counts in the +0x188 byte; one from the
   unit itself is ignored. +0x1c8 0x00405a30: the +0x16c value. */
void aoe_unit_notice_from_ror(AoeLiveUnit *unit, AoeLiveUnit *other, int32_t a, int32_t b,
                              int32_t c, uint8_t *count_188,
                              void (*notice)(void *context, AoeLiveUnit *unit, AoeLiveUnit *other,
                                             int32_t a, int32_t b, int32_t c, int32_t d),
                              void *context);
int32_t aoe_unit_field_16c_ror(const int32_t *field_16c);

/* Base unit vtable 0x00542e14 +0x78 0x004aac90: turn the facing byte
   (+0x35) by `delta`, wrapped into the graphic's angle count (+0x10's
   +0x60). Vtable 0x0054a264 +0x11c 0x004b0010: true
   (aoe_unit_default_true_ror). */
void aoe_unit_turn_ror(AoeLiveUnit *unit, int32_t delta, int16_t angle_count);

/* State setters (+0x5c): 0x004b0db0 (vtable 0x00548474) is the root
   setter 0x004aa550; 0x004ae920 (type 70, 0x0054820c) calls 0x004aeed0
   instead when the world's +0x1d is clear, the new state is above 2,
   the unit is below 3 and its template class is 1. 0x004ae960 (+0x244):
   the unit's +0x6c removal. +0x90 0x004b0830 (types 80 / 70): whether
   the owner can pay (0x004f15f0) for the missing fraction of hit points
   of the template. */
void aoe_unit_set_state_70_ror(AoeLiveUnit *unit, uint8_t state, int world_1d,
                               void (*root_set)(void *context, AoeLiveUnit *unit, uint8_t state),
                               void (*special)(void *context, AoeLiveUnit *unit), void *context);
int aoe_unit_can_pay_repair_ror(const AoeLiveUnit *unit,
                                int (*check)(void *context, int16_t template_id, float fraction),
                                void *context);

/* Action unit vtable 0x00542748 (and every subclass) +0xe8 0x004ab6c0
   reads and +0xec 0x004ab6d0 writes the +0x86 byte. */
uint8_t aoe_unit_byte_86_ror(const AoeLiveUnit *unit);
void aoe_unit_set_byte_86_ror(AoeLiveUnit *unit, uint8_t value);

#endif
