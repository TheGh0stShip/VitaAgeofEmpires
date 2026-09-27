#ifndef VITA_AOE_LIVE_UNIT_REGISTRY_H
#define VITA_AOE_LIVE_UNIT_REGISTRY_H
#include <stdint.h>

/* Semantic view of original world registry +0x88/+0x90 and +0x94/+0x9c.
   Arrays and pointed-to unit objects remain caller-owned. */
typedef struct {
    void *const *positive_units;
    int32_t positive_count;
    void *const *negative_units;
    int32_t negative_count;
} AoeLiveUnitRegistry;

/* RoR 005230d0. Negative IDs index negative_units[-id], so -1 uses slot 1;
   nonnegative IDs index positive_units[id]. Invalid IDs/counts return NULL.
   INT32_MIN is rejected rather than reproducing x86 NEG overflow. */
void *aoe_live_unit_registry_lookup(const AoeLiveUnitRegistry *registry,
                                     int32_t unit_id);
#endif
