#include "live_unit_id.h"

#include <assert.h>

int main(void) {
    DatUnit ordinary = {.present = 1, .negative_id_mode = 0};
    DatUnit negative = {.present = 1, .negative_id_mode = 1};
    AoeOwnedVisibilityRegistry registry;
    int32_t id = 123;
    assert(aoe_owned_visibility_registry_init(&registry));
    assert(aoe_live_unit_allocate_id(&ordinary, &registry, &id));
    assert(id == 0 && registry.next_positive_id == 1);
    assert(aoe_live_unit_allocate_id(&negative, &registry, &id));
    assert(id == -1 && registry.next_negative_id == -2);
    registry.special_id_mode = 1;
    registry.reserved_id = 23;
    assert(aoe_live_unit_allocate_id(&ordinary, &registry, &id));
    assert(id == 23 && registry.next_positive_id == 1);
    registry.reserved_id = -9;
    assert(aoe_live_unit_allocate_id(&negative, &registry, &id));
    assert(id == -9 && registry.next_negative_id == -10);
    assert(!aoe_live_unit_allocate_id(NULL, &registry, &id));
    ordinary.present = 0;
    assert(!aoe_live_unit_allocate_id(&ordinary, &registry, &id));
    assert(!aoe_live_unit_allocate_id(&negative, NULL, &id));
    assert(!aoe_live_unit_allocate_id(&negative, &registry, NULL));
    aoe_owned_visibility_registry_destroy(&registry);
    return 0;
}
