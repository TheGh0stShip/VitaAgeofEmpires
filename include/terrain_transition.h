#ifndef VITA_AOE_TERRAIN_TRANSITION_H
#define VITA_AOE_TERRAIN_TRANSITION_H

#include "aoe_rng.h"
#include "runtime_map.h"

typedef struct {
    void *context;
    int16_t terrain_owner_index;
    int16_t (*object_owner_index)(void *context, const void *object);
    int16_t (*object_template_id)(void *context, const void *object);
    /* Mutation/factory callbacks return zero when their dynamic method could
       not be represented; the transition then stops and reports failure. */
    int (*remove_object)(void *context, void *object);
    int (*blocked)(void *context, int16_t unit_id, float x, float y);
    int (*create)(void *context, int16_t unit_id, float x, float y);
} AoeTerrainTransitionOps;

/* RoR 004481b0, the per-cell transition called by 00449110 after writing the
   new terrain ID. Existing terrain-owner objects matching filter_terrain's
   unit list are removed in tile-list order. The new terrain's unit entries
   then use their density and placement mode with the world RNG before the
   template placement/factory callbacks. min/max_template are exclusion
   bounds: IDs outside the inclusive interval are candidates, matching the
   original `(id < min || max < id)` test. */
int aoe_terrain_transition_cell_ror(
    AoeRuntimeMap *map, const DatTerrains *terrains, AoeRng *rng,
    uint32_t x, uint32_t y, int16_t filter_terrain,
    int16_t min_template, int16_t max_template,
    const AoeTerrainTransitionOps *ops);

#endif
