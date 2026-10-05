/* Patched RoR 1.0a small functions translated in batches: constants,
   accessors and thin wrappers, each checked against its decompilation. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "unit_vtable_defaults.h"

static void unit_defaults(void) {
    static AoeLiveUnit u;
    assert(aoe_unit_default_false_ror() == 0 && aoe_unit_default_true_ror() == 1);
    assert(aoe_unit_default_zero_ror() == 0 && aoe_unit_default_minus_one_ror() == -1);
    assert(aoe_unit_default_one_ror() == 1.0f && aoe_unit_default_zero_float_ror() == 0.0f &&
           aoe_unit_default_minus_one_float_ror() == -1.0f);
    aoe_unit_default_nothing_ror();
    u.animation_rate = 1.25f;
    u.core.scenario_rotation = 3.5f;
    assert(aoe_unit_animation_rate_ror(&u) == 1.25f && aoe_unit_rotation_ror(&u) == 3.5f);
}

int main(void) {
    unit_defaults();
    puts("small functions ok");
    return 0;
}
