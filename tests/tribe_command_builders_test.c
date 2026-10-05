#include "tribe_command_builders.h"

#include <stdio.h>
#include <string.h>

#include "le_bytes.h"

static int failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static int16_t i16(const uint8_t *p) { return (int16_t)(p[0] | p[1] << 8); }

int main(void) {
    AoeTribeCommandUnitRor a = {11, 3}, b = {22, 4};
    const AoeTribeCommandUnitRor *two[2] = {&a, &b};
    const AoeTribeCommandUnitRor *gap[2] = {NULL, &b};
    int32_t ids[2] = {5, 6};
    uint8_t out[64];

    CHECK(aoe_tribe_build_66_ror(out, sizeof out, two, 2, 7, 1.5f, 2.5f) ==
          0x1c);
    CHECK(out[0] == 0x66 && out[1] == 2 && out[2] == 3 && i16(out + 0xc) == 7);
    CHECK(aoe_le_f32(out + 4) == 1.5f && aoe_le_f32(out + 8) == 2.5f);
    CHECK(aoe_le_i32(out + 0x10) == -1 && aoe_le_i32(out + 0x18) == 22);
    CHECK(aoe_tribe_build_66_ror(out, sizeof out, gap, 2, 7, 0, 0) == 0x1c);
    CHECK(out[2] == 0xff && aoe_le_i32(out + 0x14) == -1);
    CHECK(aoe_tribe_build_66_ror(out, sizeof out, two, 0, 7, 0, 0) == 0);
    CHECK(aoe_tribe_build_66_ror(out, 0x1b, two, 2, 7, 0, 0) == 0);
    CHECK(aoe_tribe_build_66_ids_ror(out, sizeof out, 9, ids, 2, 8, 1, 2, 44) ==
          0x1c);
    CHECK(out[2] == 9 && aoe_le_i32(out + 0x10) == 44 &&
          aoe_le_i32(out + 0x14) == 5);

    CHECK(aoe_tribe_build_69_ror(out, sizeof out, two, 2, 4, -3, 300, 7, 255) ==
          0x18);
    CHECK(out[0] == 0x69 && out[3] == 0 && out[4] == 0xff && out[5] == 7 &&
          out[6] == 0xff && i16(out + 8) == 4 && aoe_le_i32(out + 0xc) == -1);
    CHECK(aoe_le_i32(out + 0x10) == 11);

    CHECK(aoe_tribe_build_67_value_ror(out, sizeof out, 1, 2, 9) == 0x10);
    CHECK(out[1] == 0 && aoe_le_f32(out + 8) == 9.0f && i16(out + 0xc) == 9);
    CHECK(aoe_tribe_build_67_word_ror(out, sizeof out, 0xabcdu) == 0x10);
    CHECK(out[1] == 1 && aoe_le_u32(out + 8) == 0xabcdu && i16(out + 2) == 0);
    CHECK(aoe_tribe_build_67_byte_ror(out, sizeof out, -2, 200) == 0x10);
    CHECK(out[1] == 5 && i16(out + 2) == -2 && i16(out + 4) == 200);
    CHECK(aoe_tribe_build_67_pair_ror(out, sizeof out, 3, 4) == 0x10);
    CHECK(out[0] == 0x67 && out[1] == 6 && i16(out + 4) == 4);

    CHECK(aoe_tribe_build_6a_ror(out, sizeof out, &b) == 8);
    CHECK(out[0] == 0x6a && aoe_le_i32(out + 4) == 22);
    CHECK(aoe_tribe_build_6b_ror(out, sizeof out, two, 2, 1, 2) == 0x14);
    CHECK(out[0] == 0x6b && aoe_le_u32(out + 8) == 2 &&
          aoe_le_i32(out + 0x10) == 22);
    CHECK(aoe_tribe_build_6d_ror(out, sizeof out, gap, 2, 3) == 0x10);
    CHECK(out[0] == 0x6d && aoe_le_i32(out + 8) == -1 && aoe_le_u32(out + 4) == 3);
    CHECK(aoe_tribe_build_6d_one_ror(out, sizeof out, 77, 2) == 0xc);
    CHECK(out[1] == 1 && aoe_le_i32(out + 8) == 77);
    CHECK(aoe_tribe_build_repair_ror(out, sizeof out, two, 1, &b) == 0xc);
    CHECK(out[0] == 0x6e && aoe_le_i32(out + 4) == 22 && aoe_le_i32(out + 8) == 11);
    CHECK(aoe_tribe_build_repair_ror(out, sizeof out, two, 1, NULL) == 0);
    CHECK(aoe_tribe_build_6f_ror(out, sizeof out, two, 2, 3.0f, 4.0f) == 0x14);
    CHECK(out[0] == 0x6f && aoe_le_f32(out + 8) == 4.0f &&
          aoe_le_i32(out + 0xc) == 11);

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("tribe_command_builders_test: ok");
    return 0;
}
