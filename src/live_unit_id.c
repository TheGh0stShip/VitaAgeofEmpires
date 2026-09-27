#include "live_unit_id.h"

int aoe_live_unit_allocate_id(const DatUnit *unit_template,
                              AoeOwnedVisibilityRegistry *registry,
                              int32_t *unit_id) {
    if (!unit_template || !unit_template->present || !registry || !unit_id)
        return 0;
    if (unit_template->negative_id_mode)
        return aoe_owned_visibility_registry_next_negative_id(registry,
                                                               unit_id);
    return aoe_owned_visibility_registry_next_positive_id(registry, unit_id);
}
