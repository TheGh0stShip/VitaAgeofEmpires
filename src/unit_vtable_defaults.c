#include "unit_vtable_defaults.h"

#include <stdlib.h>

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
const DatUnit *aoe_unit_template_ror(const AoeLiveUnit *unit) { return unit->core.unit_template; }
uint8_t aoe_unit_default_ff_ror(void) { return 0xff; }
uint8_t aoe_unit_byte_86_ror(const AoeLiveUnit *unit) { return unit->byte_86; }
void aoe_unit_set_byte_86_ror(AoeLiveUnit *unit, uint8_t value) { unit->byte_86 = value; }
AoeObjectUpdateList *aoe_unit_new_list_ror(void) { return calloc(1, sizeof(AoeObjectUpdateList)); }

void aoe_unit_turn_ror(AoeLiveUnit *unit, int32_t delta, int16_t angle_count) {
    int32_t f = (int32_t)unit->core.scenario_frame + delta;
    if (angle_count <= 0) return; /* host guard: the original loops forever */
    while (f < 0) f += angle_count;
    while (f >= angle_count) f -= angle_count;
    unit->core.scenario_frame = (uint8_t)f;
}

void aoe_unit_set_state_70_ror(AoeLiveUnit *unit, uint8_t state, int world_1d,
                               void (*root_set)(void *context, AoeLiveUnit *unit, uint8_t state),
                               void (*special)(void *context, AoeLiveUnit *unit), void *context) {
    if (!world_1d && state > 2 && unit->core.state < 3 && unit->core.unit_template->unit_class == 1)
        special(context, unit);
    else
        root_set(context, unit, state);
}

int aoe_unit_can_pay_repair_ror(const AoeLiveUnit *unit,
                                int (*check)(void *context, int16_t template_id, float fraction),
                                void *context) {
    const DatUnit *t = unit->core.unit_template;
    float max = (float)t->hit_points;
    return check(context, t->id, (max - unit->core.hit_points) / max) != 0;
}

void aoe_unit_notice_from_ror(AoeLiveUnit *unit, AoeLiveUnit *other, int32_t a, int32_t b,
                              int32_t c, uint8_t *count_188,
                              void (*notice)(void *context, AoeLiveUnit *unit, AoeLiveUnit *other,
                                             int32_t a, int32_t b, int32_t c, int32_t d),
                              void *context) {
    if (other == unit) return;
    notice(context, unit, other, a, b, c, 0);
    ++*count_188;
}

int32_t aoe_unit_field_16c_ror(const int32_t *field_16c) { return *field_16c; }
