#ifndef VITA_AOE_UNIT_TEMPLATE_FACTORY_H
#define VITA_AOE_UNIT_TEMPLATE_FACTORY_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t record_type;
    uint8_t parent_record_type; /* 0 for the root template */
    size_t original_object_size;
    size_t original_live_object_size_ror;
    size_t original_live_object_size_base;
} AoeUnitTemplateFactorySpec;

/* DAT/serialized template dispatch. The common seven-case switch is RoR
   0x004502e0 / base 0x004acf20. Civilization type 1 uses the derived
   override RoR 0x004f1890 / base 0x0052a0e0, adding F/P/Z (70/80/90)
   before falling back to those seven cases. The player stream reader also
   uses the same type family. Original byte sizes do not define portable
   C struct layouts. The two live sizes are allocations at template vtable
   +0x18, not portable runtime struct layouts. parent_record_type follows
   original RoR constructor calls, not a synthetic gameplay classification.
   Returns 0 for a type the original switch leaves alone. */
int aoe_unit_template_factory_spec(uint8_t civilization_type,
                                    uint8_t record_type,
                                    AoeUnitTemplateFactorySpec *spec);

#endif
