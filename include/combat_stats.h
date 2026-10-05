#ifndef VITA_AOE_COMBAT_STATS_H
#define VITA_AOE_COMBAT_STATS_H

#include <stdint.h>

#include "unit_template.h"

/* Patched Rise of Rome 1.0a combat unit readings of its template (the
   type 50 class, vtables 0x0054324c / 0x005445e8 and the RoR 0x00547fa0 /
   0x0054820c; 0x00426a20..0x00427b70). */

/* +0x230 0x00426a20: the displayed attack (+0x100, raised to the largest
   attack amount) and its shown base (+0x13c); 0x00426a80: the armor (the
   largest armor amount, from 0) and +0x13e; 0x00426ae0 the range +0x114
   and +0x140; 0x00426b10 the reload +0x120 and +0x144. */
void aoe_combat_attack_ror(const AoeTemplateRor *t, int16_t *value, int16_t *base);
void aoe_combat_armor_ror(const AoeTemplateRor *t, int16_t *value, int16_t *base);
void aoe_combat_range_pair_ror(const AoeTemplateRor *t, float *value, float *base);
void aoe_combat_reload_pair_ror(const AoeTemplateRor *t, float *value, float *base);
/* +0x100 0x00427a60 the reload (+0x120); 0x00427a70 the sum of the
   positive armor amounts; 0x00427b60 the range (+0x114); 0x00427b70
   +0x138. */
float aoe_combat_reload_ror(const AoeTemplateRor *t);
float aoe_combat_armor_sum_ror(const AoeTemplateRor *t);
float aoe_combat_range_ror(const AoeTemplateRor *t);
float aoe_combat_field_138_ror(const AoeTemplateRor *t);
/* +0xe4 0x00427b00: a target in range: its distance (0x004aaeb0) within
   the unit's range (+0x10c) plus the template's footprint radius
   (sqrt(x^2 + y^2) of +0x30 / +0x34) plus 0.3. */
int aoe_combat_in_range_ror(const AoeTemplateRor *t, float unit_range, float distance);

#endif
