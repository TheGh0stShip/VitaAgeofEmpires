/* Patched RoR 1.0a class tests and small rule helpers. */
#include "unit_class.h"

#include <assert.h>
#include <string.h>

int main(void) {
    static const int32_t resources[] = {7, 8, 9, 10, 15, 32};
    DatUnit t;
    int32_t c, i, n = 0;
    for (i = 0; i < 6; ++i) assert(aoe_class_is_resource_ror(resources[i]));
    for (c = -1; c < 64; ++c) n += aoe_class_is_resource_ror(c);
    assert(n == 6);
    assert(aoe_class_is_boat_ror(2) && aoe_class_is_boat_ror(20) &&
           aoe_class_is_boat_ror(21) && aoe_class_is_boat_ror(22) &&
           !aoe_class_is_boat_ror(23));
    assert(aoe_class_is_structure_ror(3) && aoe_class_is_structure_ror(27) &&
           !aoe_class_is_structure_ror(4));
    /* Transport boats (20) differ between the two combatant tests. */
    assert(!aoe_class_is_combatant_ror(20) &&
           aoe_class_is_combatant_or_transport_ror(20));
    assert(!aoe_class_is_combatant_ror(4) && aoe_class_is_combatant_ror(6) &&
           !aoe_class_is_combatant_or_transport_ror(21));
    memset(&t, 0, sizeof(t));
    assert(!aoe_unit_not_artifact_ror(NULL));
    t.unit_class = 1;
    assert(!aoe_unit_not_artifact_ror(&t) && !aoe_unit_target_ready_ror(&t, 2));
    t.unit_class = 6;
    assert(aoe_unit_not_artifact_ror(&t) && aoe_unit_target_ready_ror(&t, 2) &&
           !aoe_unit_target_ready_ror(&t, 1) && !aoe_unit_target_ready_ror(NULL, 5));
    assert(aoe_code_in_101_103_ror(102) && !aoe_code_in_101_103_ror(104));
    assert(aoe_code_1_or_30_ror(30) && !aoe_code_1_or_30_ror(2));
    assert(aoe_code_in_705_709_713_ror(709) && !aoe_code_in_705_709_713_ror(707));
    assert(aoe_code_703_or_704_ror(703) && !aoe_code_703_or_704_ror(705));
    assert(aoe_code_31_or_5_ror(5) && !aoe_code_31_or_5_ror(33));
    assert(aoe_code_31_5_33_rank_ror(33) == 2 && aoe_code_31_5_33_rank_ror(6) == -1);
    assert(aoe_square_min2_ror(1) == 2 && aoe_square_min2_ror(-5) == 2 &&
           aoe_square_min2_ror(2) == 4 && aoe_square_min2_ror(7) == 49);
    assert(strcmp(aoe_activation_text_ror(3), " is now ACTIVE.") == 0 &&
           strcmp(aoe_activation_text_ror(0), " has been deactivated.") == 0);
    assert(aoe_dib_stride_ror(5) == 8 && aoe_dib_stride_ror(8) == 8 &&
           aoe_dib_stride_ror(0) == 0);
    return 0;
}
