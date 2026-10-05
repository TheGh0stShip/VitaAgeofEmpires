#include "unit_storage.h"

#include <assert.h>
#include <string.h>

static void run_edition(AoePlayerEdition edition) {
    DatUnit unit = {.present = 1};
    float values[3] = {10.0f, 20.0f, 30.0f};
    AoePlayerResources resources = {.count = 3, .values = values};
    AoeUnitStorageState state = {99, 99.0f};
    unit.storage[0] = (DatUnitStorage){.resource_type = 1,
                                       .amount = 2.5f, .paid = 0};
    unit.storage[1] = (DatUnitStorage){.resource_type = 0,
                                       .amount = 3.0f, .paid = 1};
    unit.storage[2] = (DatUnitStorage){.resource_type = 1,
                                       .amount = 4.0f, .paid = 2};
    assert(aoe_unit_storage_initialize(&unit, &resources, edition, &state));
    assert(state.carried_resource_type == 1 && state.carried_amount == 2.5f);
    assert(values[0] == 13.0f && values[1] == 24.0f && values[2] == 30.0f);
    assert(aoe_unit_storage_exit_active(&unit, &resources, edition));
    assert(values[0] == 13.0f && values[1] == 20.0f && values[2] == 30.0f);
    values[1] = 24.0f;

    unit.storage[0] = (DatUnitStorage){.resource_type = -1,
                                       .amount = 10.0f, .paid = 0};
    unit.storage[1] = (DatUnitStorage){.resource_type = 2,
                                       .amount = 8.0f, .paid = 0};
    unit.storage[2] = (DatUnitStorage){.resource_type = -2,
                                       .amount = 9.0f, .paid = 0};
    assert(aoe_unit_storage_initialize(&unit, &resources, edition, &state));
    assert(state.carried_resource_type == -2 && state.carried_amount == 9.0f);
    assert(values[0] == 13.0f && values[1] == 24.0f && values[2] == 30.0f);

    unit.storage[0].paid = 3;
    unit.storage[1].paid = 2;
    unit.storage[1].resource_type = -1;
    unit.storage[2].paid = 1;
    unit.storage[2].resource_type = 99; /* Original bound check ignores it. */
    assert(aoe_unit_storage_initialize(&unit, &resources, edition, &state));
    assert(state.carried_resource_type == -1 && state.carried_amount == 0.0f);
    assert(values[0] == 13.0f && values[1] == 24.0f && values[2] == 30.0f);

    /* memcpy, not assignment: assignment leaves padding unspecified. */
    AoeUnitStorageState before;
    memcpy(&before, &state, sizeof(before));
    float original[3];
    memcpy(original, values, sizeof(original));
    assert(!aoe_unit_storage_initialize(&unit, &resources,
                                         (AoePlayerEdition)9, &state));
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(memcmp(values, original, sizeof(original)) == 0);
    unit.present = 0;
    assert(!aoe_unit_storage_initialize(&unit, &resources, edition, &state));
    assert(!aoe_unit_storage_exit_active(&unit, &resources, edition));
}

int main(void) {
    run_edition(AOE_PLAYER_BASE);
    run_edition(AOE_PLAYER_RISE_OF_ROME);
    return 0;
}
