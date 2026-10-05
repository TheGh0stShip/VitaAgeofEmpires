#include "combat_stats.h"

#include <math.h>

void aoe_combat_attack_ror(const AoeTemplateRor *t, int16_t *value, int16_t *base) {
    int16_t i;
    *value = (int16_t)t->field_100;
    for (i = 0; i < t->attack_count; ++i)
        if (*value < t->attacks[i].amount) *value = t->attacks[i].amount;
    *base = t->max_attack;
}

void aoe_combat_armor_ror(const AoeTemplateRor *t, int16_t *value, int16_t *base) {
    int16_t i;
    *value = 0;
    for (i = 0; i < t->armor_count; ++i)
        if (*value < t->armors[i].amount) *value = t->armors[i].amount;
    *base = t->max_armor;
}

void aoe_combat_range_pair_ror(const AoeTemplateRor *t, float *value, float *base) {
    *value = t->field_114;
    *base = t->field_140;
}

void aoe_combat_reload_pair_ror(const AoeTemplateRor *t, float *value, float *base) {
    *value = t->field_120;
    *base = t->field_144;
}

float aoe_combat_reload_ror(const AoeTemplateRor *t) { return t->field_120; }

float aoe_combat_armor_sum_ror(const AoeTemplateRor *t) {
    float sum = 0.0f;
    int16_t i;
    for (i = 0; i < t->armor_count; ++i)
        if (0.0f < (float)t->armors[i].amount) sum += (float)t->armors[i].amount;
    return sum;
}

float aoe_combat_range_ror(const AoeTemplateRor *t) { return t->field_114; }
float aoe_combat_field_138_ror(const AoeTemplateRor *t) { return t->field_138; }

int aoe_combat_in_range_ror(const AoeTemplateRor *t, float unit_range, float distance) {
    float sx = t->size[1], sy = t->size[0];
    return distance <= (float)(unit_range + sqrtf(sx * sx + sy * sy) + 0.3f);
}
