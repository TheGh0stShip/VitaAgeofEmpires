/* Patched RoR 1.0a small functions translated in batches: constants,
   accessors and thin wrappers, each checked against its decompilation. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "unit_vtable_defaults.h"
#include "effects_io.h"
#include <stdlib.h>

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

static void text_helpers(void) {
    char *f = NULL;
    aoe_text_set_ror(&f, "abc");
    assert(strcmp(f, "abc") == 0);
    aoe_text_set_joined_ror(&f, "dir/", "file");
    assert(strcmp(f, "dir/file") == 0);
    free(f);
}

int main(void) {
    unit_defaults();
    text_helpers();
    puts("small functions ok");
    return 0;
}
