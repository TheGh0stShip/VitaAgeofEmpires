#include "live_unit_registry.h"
#include <limits.h>
#include <stddef.h>

void *aoe_live_unit_registry_lookup(const AoeLiveUnitRegistry *r,
                                     int32_t id) {
    if (!r || r->positive_count < 0 || r->negative_count < 0) return NULL;
    if (id < 0) {
        int32_t index;
        if (id==INT32_MIN) return NULL;
        index=-id;
        if (!r->negative_units || index>=r->negative_count) return NULL;
        return r->negative_units[index];
    }
    if (!r->positive_units || id>=r->positive_count) return NULL;
    return r->positive_units[id];
}
