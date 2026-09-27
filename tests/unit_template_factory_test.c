#include "unit_template_factory.h"

#include <assert.h>

int main(void) {
    static const struct {
        uint8_t type, parent;
        size_t size, ror_live_size, base_live_size;
    } expected[] = {
        {10, 0, 0xb8, 0x88, 0x88}, {20, 10, 0xbc, 0x8c, 0x8c},
        {25, 20, 0xbc, 0x8c, 0x8c}, {30, 20, 0xd8, 0x180, 0x178},
        {40, 30, 0xfc, 0x18c, 0x184}, {50, 40, 0x148, 0x1bc, 0x1b4},
        {60, 50, 0x154, 0x1c0, 0x1b8}
    };
    for (unsigned civ_type = 0; civ_type < 3; ++civ_type) {
        for (unsigned type = 0; type < 256; ++type) {
            AoeUnitTemplateFactorySpec spec = {0xff, 0xfe, 0x1234,
                                                0x5678, 0x9abc};
            size_t size = 0;
            size_t ror_live_size = 0, base_live_size = 0;
            uint8_t parent = 0;
            for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
                if (expected[i].type == type) {
                    size = expected[i].size;
                    ror_live_size = expected[i].ror_live_size;
                    base_live_size = expected[i].base_live_size;
                    parent = expected[i].parent;
                }
            if (civ_type == 1) {
                if (type == 70) { size = 0x164; ror_live_size = 0x1c0; base_live_size = 0x1b8; parent = 50; }
                if (type == 80) { size = 0x17c; ror_live_size = 0x1fc; base_live_size = 0x1e0; parent = 70; }
                if (type == 90) { size = 0xb8; ror_live_size = 0x88; base_live_size = 0x88; parent = 10; }
            }
            assert(aoe_unit_template_factory_spec((uint8_t)civ_type,
                                                   (uint8_t)type, &spec) ==
                   (size != 0));
            if (size)
                assert(spec.record_type == type && spec.parent_record_type == parent &&
                       spec.original_object_size == size &&
                       spec.original_live_object_size_ror == ror_live_size &&
                       spec.original_live_object_size_base == base_live_size);
            else
                assert(spec.record_type == 0xff && spec.parent_record_type == 0xfe &&
                       spec.original_object_size == 0x1234 &&
                       spec.original_live_object_size_ror == 0x5678 &&
                       spec.original_live_object_size_base == 0x9abc);
        }
    }
    assert(!aoe_unit_template_factory_spec(1, 10, NULL));
    return 0;
}
