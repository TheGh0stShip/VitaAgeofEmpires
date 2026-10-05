#include "unit_class.h"

int aoe_class_is_resource_ror(int32_t c) {
    return c == 7 || c == 9 || c == 10 || c == 15 || c == 8 || c == 32;
}

int aoe_class_is_boat_ror(int32_t c) {
    return c == 0x15 || c == 2 || c == 0x14 || c == 0x16;
}

int aoe_class_is_structure_ror(int32_t c) { return c == 3 || c == 0x1b; }

int aoe_class_is_combatant_ror(int32_t c) {
    return !(c == 3 || c == 0x1b || c == 4 || c == 0x15 || c == 2 || c == 0x14);
}

int aoe_class_is_combatant_or_transport_ror(int32_t c) {
    return !(c == 3 || c == 0x1b || c == 4 || c == 0x15 || c == 2);
}

int aoe_unit_not_artifact_ror(const DatUnit *t) {
    return t && t->unit_class != 1;
}

int aoe_unit_target_ready_ror(const DatUnit *target, uint8_t own_state) {
    if (!target || target->unit_class == 1) return 0;
    return own_state >= 2;
}

int aoe_code_in_101_103_ror(int32_t c) { return c == 0x65 || c == 0x66 || c == 0x67; }
int aoe_code_1_or_30_ror(int32_t c) { return c == 1 || c == 0x1e; }
int aoe_code_in_705_709_713_ror(int32_t c) { return c == 0x2c1 || c == 0x2c5 || c == 0x2c9; }
int aoe_code_703_or_704_ror(int32_t c) { return c == 0x2c0 || c == 0x2bf; }
int aoe_code_31_or_5_ror(int32_t c) { return c == 0x1f || c == 5; }

int32_t aoe_code_31_5_33_rank_ror(int32_t c) {
    return c == 0x1f || c == 5 || c == 0x21 ? 2 : -1;
}

int32_t aoe_square_min2_ror(int32_t v) { return v < 2 ? 2 : v * v; }

const char *aoe_activation_text_ror(int32_t active) {
    return active ? " is now ACTIVE." : " has been deactivated.";
}

uint32_t aoe_dib_stride_ror(int32_t row_bytes) {
    return ((uint32_t)row_bytes + 3u) & 0xfffffffcu;
}
