#ifndef VITA_AOE_SCENARIO_SPAWN_H
#define VITA_AOE_SCENARIO_SPAWN_H

#include "scn.h"

#include <stdint.h>

/* RoR world +0x70/+0x74 and base equivalent: the active scenario spawn ID
   remains set after creation, while the loading flag is cleared. */
typedef struct {
    uint32_t active_spawn_id;
    uint8_t loading_flag;
} AoeScenarioSpawnWorld;

/* The actual DAT unit factory and unit state methods are separate original
   routines and remain to be ported. These callbacks are their call boundary,
   not substitute gameplay. A factory returns NULL for an absent definition. */
typedef struct {
    void *(*construct)(void *context, uint16_t object_id,
                       float x, float y, float z);
    void (*state_0_to_2)(void *context, void *unit, float rotation);
    void (*state_3_to_6)(void *context, void *unit);
    void (*state_7_plus)(void *context, void *unit);
} AoeScenarioSpawnMethods;

/* Scenario-load placement dispatch: RoR 0x00522be0 -> player +0x60
   (0x0045d1c0), base 0x00505da0 -> player +0x60 (0x00516af0).
   Sets world loading state around each player call, constructs from the
   original object ID/X/Y/Z, then invokes exactly one state category.
   Returns 0 only for invalid arguments, 1 for a valid dispatch. A valid
   dispatch can still return a NULL unit when the DAT definition is absent. */
int aoe_scenario_spawn_one(const ScnUnit *placement,
                            AoeScenarioSpawnWorld *world,
                            const AoeScenarioSpawnMethods *methods,
                            void *context, void **out_unit);

#endif
