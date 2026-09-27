#include "unit_storage.h"

int aoe_unit_storage_initialize(const DatUnit *unit,
                                AoePlayerResources *owner_resources,
                                AoePlayerEdition edition,
                                AoeUnitStorageState *state) {
    AoeUnitStorageState next = {-1, 0.0f};
    if (!unit || !unit->present || !owner_resources || !state ||
        (edition != AOE_PLAYER_BASE && edition != AOE_PLAYER_RISE_OF_ROME))
        return 0;
    for (int i = 0; i < 3; ++i) {
        const DatUnitStorage *entry = &unit->storage[i];
        if (entry->paid == 0 && entry->resource_type != -1) {
            next.carried_resource_type = entry->resource_type;
            next.carried_amount = entry->amount;
        }
    }
    *state = next;
    for (int i = 0; i < 3; ++i) {
        const DatUnitStorage *entry = &unit->storage[i];
        if ((entry->paid == 1 || entry->paid == 2) &&
            entry->resource_type >= 0)
            aoe_player_resource_add(owner_resources, entry->resource_type,
                                    entry->amount, edition);
    }
    return 1;
}

int aoe_unit_storage_exit_active(const DatUnit *unit,
                                  AoePlayerResources *owner_resources,
                                  AoePlayerEdition edition) {
    if (!unit || !unit->present || !owner_resources ||
        (edition != AOE_PLAYER_BASE && edition != AOE_PLAYER_RISE_OF_ROME))
        return 0;
    for (int i = 0; i < 3; ++i) {
        const DatUnitStorage *entry = &unit->storage[i];
        if (entry->paid == 2 && entry->resource_type >= 0)
            aoe_player_resource_add(owner_resources, entry->resource_type,
                                    -entry->amount, edition);
    }
    return 1;
}
