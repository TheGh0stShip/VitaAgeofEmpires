#include "command_dispatch.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include "le_bytes.h"

static DatUnit soldier, bird, house;
static AoeLiveUnit created;
static AoeBuildingUpdate building;
static float last_z;
static int creations, state_calls;
static uint8_t last_state;
static int fail_create;

static const DatUnit *templates(void *context, uint8_t player,
                                int16_t index) {
    (void)context;
    if (player != 2) return NULL;
    switch (index) {
    case 83: return &soldier;
    case -2: return &bird;
    case 70: return &house;
    default: return NULL;
    }
}

static AoeLiveUnit *create(void *context, uint8_t player,
                           const DatUnit *unit_template, float x, float y,
                           float z) {
    (void)context;
    assert(player == 2 && x == 10.5f && y == 20.25f);
    ++creations;
    last_z = z;
    if (fail_create) return NULL;
    memset(&created, 0, sizeof(created));
    created.constructed = 1;
    created.core.unit_template = unit_template;
    if (unit_template == &house) {
        memset(&building, 0, sizeof(building));
        building.unit_template = &house;
        created.building_update = &building;
        created.core.hit_points = 1.0f;
        created.core.state = 0;
    } else {
        created.core.state = 2;
    }
    return &created;
}

static int set_state(void *context, AoeLiveUnit *unit, uint8_t state) {
    (void)context;
    unit->core.state = state;
    last_state = state;
    ++state_calls;
    return 1;
}

static void build_work_tests(void) {
    AoeLiveUnitSetStateOps ops = {0, set_state};
    int completed = -1;
    memset(&created, 0, sizeof(created));
    memset(&building, 0, sizeof(building));
    building.unit_template = &house;
    created.constructed = 1;
    created.core.unit_template = &house;
    created.building_update = &building;
    created.core.hit_points = 1.0f;
    state_calls = 0;
    /* 15 of 30: hp gains 600*15/30 = 300; not finished. */
    assert(aoe_live_unit_add_build_work_ror(&created, 15.0f, &ops,
                                            &completed) &&
           !completed && created.core.hit_points == 301.0f &&
           building.elapsed_build_time == 15.0f && !state_calls);
    /* Overshoot clamps work to the remaining 15; hp clamps to 600. */
    assert(aoe_live_unit_add_build_work_ror(&created, 100.0f, &ops,
                                            &completed) &&
           completed && created.core.hit_points == 600.0f &&
           building.elapsed_build_time == 30.0f && state_calls == 1 &&
           last_state == 2);
    /* Nonzero state returns completed without changes. */
    assert(aoe_live_unit_add_build_work_ror(&created, 5.0f, &ops,
                                            &completed) &&
           completed && building.elapsed_build_time == 30.0f &&
           state_calls == 1);
    /* NaN work: no clamp, and the unordered final compare is "not done". */
    created.core.state = 0;
    building.elapsed_build_time = 1.0f;
    assert(aoe_live_unit_add_build_work_ror(&created, NAN, &ops,
                                            &completed) &&
           !completed && isnan(building.elapsed_build_time) &&
           state_calls == 1);
    /* Fractional work: hp is H*amount/T + hp evaluated in double and
       rounded once on store. */
    created.core.hit_points = 599.0f;
    building.elapsed_build_time = 0.0f;
    assert(aoe_live_unit_add_build_work_ror(&created, 0.05f, &ops,
                                            &completed) &&
           !completed && created.core.hit_points == (float)(
               600.0 * (double)0.05f / 30.0 + 599.0));
    ops.set_state = NULL;
    building.elapsed_build_time = 29.0f;
    assert(!aoe_live_unit_add_build_work_ror(&created, 1.0f, &ops,
                                             &completed));
}

int main(void) {
    AoeCommandObjectOps ops = {0, templates, create, {0, set_state}};
    AoeCommandDispatchRor base;
    AoeCommandTribeDispatchRor tribe;
    AoeRng rng;
    uint8_t record[0x14];
    static const uint8_t expected[0x14] = {
        4, 0, 0xfe, 0xff, 2, 0, 0, 0, 0x00, 0x00, 0x28, 0x41,
        0x00, 0x00, 0xa2, 0x41, 0x00, 0x00, 0x00, 0x40};

    soldier.type = 70;
    bird.type = 30;
    bird.elevation_mode = 1;
    house.type = 80;
    house.hit_points = 600;
    house.train_time = 30;
    build_work_tests();

    /* 0x0042ba90 layout with a negative template index. */
    assert(aoe_command_encode_object_create_ror(
               record, sizeof(record), -2, 2, 10.5f, 20.25f, 2.0f) == 0x14 &&
           !memcmp(record, expected, sizeof(expected)));

    memset(&base, 0, sizeof(base));
    memset(&tribe, 0, sizeof(tribe));
    aoe_rng_seed(&rng, 7);
    tribe.rng = &rng;
    aoe_command_bind_object_create_ror(&base, &ops);
    aoe_command_bind_object_create_tribe_ror(&tribe, &ops);
    tribe.base = base;

    /* Base handler keeps z; tribe forces 4.0f for elevation mode 1. */
    assert(aoe_command_dispatch_ror(&base, record, 0x14) ==
               AOE_COMMAND_ROUTE_HANDLED && last_z == 2.0f);
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, 0x14) ==
               AOE_COMMAND_ROUTE_HANDLED && last_z == 4.0f);
    assert(aoe_le_f32(record + 0x10) == 2.0f); /* local copy only */

    /* Tribe completes a created building with 10000 work. */
    assert(aoe_command_encode_object_create_ror(
               record, sizeof(record), 70, 2, 10.5f, 20.25f, 0.0f) == 0x14);
    state_calls = 0;
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, 0x14) ==
               AOE_COMMAND_ROUTE_HANDLED && state_calls == 1 &&
           created.core.state == 2 && created.core.hit_points == 600.0f &&
           building.elapsed_build_time == 30.0f);
    /* Base handler leaves the new building unfinished. */
    state_calls = 0;
    assert(aoe_command_dispatch_ror(&base, record, 0x14) ==
               AOE_COMMAND_ROUTE_HANDLED && !state_calls &&
           created.core.state == 0);

    /* A non-building is not given work. */
    assert(aoe_command_encode_object_create_ror(
               record, sizeof(record), 83, 2, 10.5f, 20.25f, 0.0f) == 0x14);
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, 0x14) ==
               AOE_COMMAND_ROUTE_HANDLED && !state_calls);

    /* Failed creation: ignored by base, host-guarded in tribe. */
    fail_create = 1;
    assert(aoe_command_dispatch_ror(&base, record, 0x14) ==
           AOE_COMMAND_ROUTE_HANDLED);
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, 0x14) ==
           AOE_COMMAND_ROUTE_INVALID);
    fail_create = 0;
    /* Unresolved template or short record. */
    record[4] = 3;
    creations = 0;
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, 0x14) ==
               AOE_COMMAND_ROUTE_INVALID && !creations);
    record[4] = 2;
    assert(aoe_command_dispatch_ror(&base, record, 0x13) ==
           AOE_COMMAND_ROUTE_INVALID);
    return 0;
}
