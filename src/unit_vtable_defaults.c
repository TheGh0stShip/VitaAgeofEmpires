#include "unit_vtable_defaults.h"

uint8_t aoe_unit_default_false_ror(void) { return 0; }
uint8_t aoe_unit_default_true_ror(void) { return 1; }
int32_t aoe_unit_default_zero_ror(void) { return 0; }
int32_t aoe_unit_default_minus_one_ror(void) { return -1; }
float aoe_unit_default_one_ror(void) { return 1.0f; }
float aoe_unit_default_zero_float_ror(void) { return 0.0f; }
float aoe_unit_default_minus_one_float_ror(void) { return -1.0f; }
void aoe_unit_default_nothing_ror(void) {}
float aoe_unit_animation_rate_ror(const AoeLiveUnit *unit) { return unit->animation_rate; }
float aoe_unit_rotation_ror(const AoeLiveUnit *unit) { return unit->core.scenario_rotation; }
