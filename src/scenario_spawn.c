#include "scenario_spawn.h"

int aoe_scenario_spawn_one(const ScnUnit *placement,
                            AoeScenarioSpawnWorld *world,
                            const AoeScenarioSpawnMethods *methods,
                            void *context, void **out_unit) {
    void *unit;
    if (!placement || !world || !methods || !methods->construct ||
        !methods->state_0_to_2 || !methods->state_3_to_6 ||
        !methods->state_7_plus) return 0;
    world->active_spawn_id = placement->spawn_id;
    world->loading_flag = 1;
    unit = methods->construct(context, placement->object_id,
                              placement->x, placement->y, placement->z);
    if (unit) {
        if (placement->state >= 7)
            methods->state_7_plus(context, unit);
        else if (placement->state >= 3)
            methods->state_3_to_6(context, unit);
        else
            methods->state_0_to_2(context, unit, placement->rotation);
    }
    world->loading_flag = 0;
    if (out_unit) *out_unit = unit;
    return 1;
}
