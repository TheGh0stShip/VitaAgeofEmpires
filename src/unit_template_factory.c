#include "unit_template_factory.h"

int aoe_unit_template_factory_spec(uint8_t civilization_type,
                                    uint8_t record_type,
                                    AoeUnitTemplateFactorySpec *spec) {
    size_t size;
    size_t ror_live_size, base_live_size;
    uint8_t parent;
    if (!spec) return 0;
    if (civilization_type == 1) {
        switch (record_type) {
        case 70: size = 0x164; ror_live_size = 0x1c0; base_live_size = 0x1b8; parent = 50; goto found;
        case 80: size = 0x17c; ror_live_size = 0x1fc; base_live_size = 0x1e0; parent = 70; goto found;
        case 90: size = 0xb8; ror_live_size = 0x88; base_live_size = 0x88; parent = 10; goto found;
        default: break;
        }
    }
    switch (record_type) {
    case 10: size = 0xb8; ror_live_size = 0x88; base_live_size = 0x88; parent = 0; break;
    case 20: size = 0xbc; ror_live_size = 0x8c; base_live_size = 0x8c; parent = 10; break;
    case 25: size = 0xbc; ror_live_size = 0x8c; base_live_size = 0x8c; parent = 20; break;
    case 30: size = 0xd8; ror_live_size = 0x180; base_live_size = 0x178; parent = 20; break;
    case 40: size = 0xfc; ror_live_size = 0x18c; base_live_size = 0x184; parent = 30; break;
    case 50: size = 0x148; ror_live_size = 0x1bc; base_live_size = 0x1b4; parent = 40; break;
    case 60: size = 0x154; ror_live_size = 0x1c0; base_live_size = 0x1b8; parent = 50; break;
    default: return 0;
    }
found:
    *spec = (AoeUnitTemplateFactorySpec){record_type, parent, size,
                                         ror_live_size, base_live_size};
    return 1;
}
