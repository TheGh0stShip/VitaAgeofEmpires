#include "scenario_spawn.h"

#include <assert.h>
#include <stdint.h>

typedef struct {
    AoeScenarioSpawnWorld *world;
    uint16_t object_id;
    float x, y, z, rotation;
    int factory_calls, low_calls, middle_calls, high_calls;
    int missing;
} Trace;

static void *construct(void *context, uint16_t object_id,
                       float x, float y, float z) {
    Trace *trace = context;
    assert(trace->world->loading_flag == 1);
    assert(trace->world->active_spawn_id == 1234);
    trace->object_id = object_id;
    trace->x = x;
    trace->y = y;
    trace->z = z;
    ++trace->factory_calls;
    return trace->missing ? NULL : trace;
}

static void low(void *context, void *unit, float rotation) {
    Trace *trace = context;
    assert(unit == trace && trace->world->loading_flag == 1);
    trace->rotation = rotation;
    ++trace->low_calls;
}

static void middle(void *context, void *unit) {
    Trace *trace = context;
    assert(unit == trace && trace->world->loading_flag == 1);
    ++trace->middle_calls;
}

static void high(void *context, void *unit) {
    Trace *trace = context;
    assert(unit == trace && trace->world->loading_flag == 1);
    ++trace->high_calls;
}

int main(void) {
    AoeScenarioSpawnWorld world = {42, 0};
    AoeScenarioSpawnMethods methods = {construct, low, middle, high};
    Trace trace = {.world = &world};
    ScnUnit unit = {1.5f, 2.5f, 3.5f, 1234, 79, 0, 6.25f};
    void *result = NULL;
    for (unsigned state = 0; state < 256; ++state) {
        unit.state = (uint8_t)state;
        assert(aoe_scenario_spawn_one(&unit, &world, &methods,
                                      &trace, &result));
        assert(result == &trace && world.loading_flag == 0 &&
               world.active_spawn_id == 1234);
    }
    assert(trace.factory_calls == 256);
    assert(trace.low_calls == 3 && trace.middle_calls == 4 &&
           trace.high_calls == 249);
    assert(trace.object_id == 79 && trace.x == 1.5f &&
           trace.y == 2.5f && trace.z == 3.5f &&
           trace.rotation == 6.25f);

    trace.missing = 1;
    unit.state = 1;
    result = &trace;
    assert(aoe_scenario_spawn_one(&unit, &world, &methods,
                                  &trace, &result));
    assert(result == NULL && trace.factory_calls == 257 &&
           trace.low_calls == 3 && world.loading_flag == 0);

    world.active_spawn_id = 99;
    assert(!aoe_scenario_spawn_one(NULL, &world, &methods, &trace, &result));
    assert(world.active_spawn_id == 99 && world.loading_flag == 0);
    assert(!aoe_scenario_spawn_one(&unit, NULL, &methods, &trace, &result));
    methods.state_7_plus = NULL;
    assert(!aoe_scenario_spawn_one(&unit, &world, &methods, &trace, &result));
    assert(world.active_spawn_id == 99 && world.loading_flag == 0);
    return 0;
}
