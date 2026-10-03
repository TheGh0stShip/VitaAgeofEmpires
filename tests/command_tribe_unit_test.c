#include "command_dispatch.h"

#include <assert.h>
#include <string.h>

#include "le_bytes.h"

static AoeLiveUnit units[3];
static int calls_244, calls_14c, repairs, tasks;
static uint32_t last_a, last_b;
static AoeLiveUnit *repair_source;
static AoePlayerUnitTaskRor last_task;

static AoeLiveUnit *resolve(void *c, int32_t id) {
    (void)c;
    for (int i = 0; i < 3; ++i)
        if (units[i].core.unit_id == id) return &units[i];
    return NULL;
}
static int16_t owner_id(void *c, const AoeLiveUnit *u) {
    (void)c;
    return (int16_t)(u - units + 1);
}
static int32_t owner_kind(void *c, const AoeLiveUnit *u) {
    (void)c; (void)u;
    return 0;
}
static int has_group(void *c, const AoeLiveUnit *u) { (void)c; (void)u; return 1; }
static void group_task(void *c, AoeLiveUnit *u, const AoePlayerUnitTaskRor *t) {
    (void)c; (void)u;
    last_task = *t;
    ++tasks;
}
static void unit_244(void *c, AoeLiveUnit *u) { (void)c; (void)u; ++calls_244; }
static void unit_14c(void *c, AoeLiveUnit *u, uint32_t a, uint32_t b) {
    (void)c; (void)u;
    last_a = a; last_b = b;
    ++calls_14c;
}
static void repair(void *c, AoeLiveUnit *u, AoeLiveUnit *s) {
    (void)c; (void)u;
    repair_source = s;
    ++repairs;
}

int main(void) {
    AoeCommandTribeUnitOps ops;
    AoeCommandTribeDispatchRor tribe;
    AoeRng rng;
    uint8_t record[32];
    int32_t ids[2] = {10, 11};
    size_t n;
    memset(&ops, 0, sizeof(ops));
    ops.player.world.resolve = resolve;
    ops.player.owner_id = owner_id;
    ops.player.owner_kind = owner_kind;
    ops.player.has_group_object = has_group;
    ops.player.group_task = group_task;
    ops.unit_244 = unit_244;
    ops.unit_14c = unit_14c;
    ops.repair = repair;
    for (int i = 0; i < 3; ++i) {
        units[i].constructed = 1;
        units[i].core.unit_id = 10 + i;
    }
    memset(&tribe, 0, sizeof(tribe));
    aoe_rng_seed(&rng, 1);
    tribe.rng = &rng;
    aoe_command_bind_tribe_unit_ror(&tribe, &ops);

    memset(record, 0, sizeof(record));
    record[0] = 0x6a;
    aoe_le_put_u32(record + 4, 12);
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, 8) ==
               AOE_COMMAND_ROUTE_HANDLED && calls_244 == 1);

    n = aoe_command_encode_unit_list_ror(record, sizeof(record), 0x6b, 0x0c,
                                         ids, 2);
    aoe_le_put_u32(record + 4, 7);
    aoe_le_put_u32(record + 8, 8);
    assert(n == 0x14 && aoe_command_dispatch_tribe_ror(&tribe, record, n) ==
                            AOE_COMMAND_ROUTE_HANDLED &&
           calls_14c == 2 && last_a == 7 && last_b == 8);

    n = aoe_command_encode_unit_list_ror(record, sizeof(record), 0x6d, 0x08,
                                         ids, 2);
    record[4] = 0xfe; record[5] = 0xff; /* -2 */
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, n) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           units[0].core.storage.carried_resource_type == -2 &&
           units[1].core.storage.carried_resource_type == -2 &&
           units[2].core.storage.carried_resource_type == 0);

    n = aoe_command_encode_unit_list_ror(record, sizeof(record), 0x6e, 0x08,
                                         ids, 2);
    aoe_le_put_u32(record + 4, 12);
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, n) ==
               AOE_COMMAND_ROUTE_HANDLED &&
           repairs == 2 && repair_source == &units[2]);

    n = aoe_command_encode_unit_list_ror(record, sizeof(record), 0x6f, 0x0c,
                                         ids, 1);
    aoe_le_put_u32(record + 4, 0x40a00000);
    aoe_le_put_u32(record + 8, 0x40c00000);
    assert(aoe_command_dispatch_tribe_ror(&tribe, record, n) ==
               AOE_COMMAND_ROUTE_HANDLED && tasks == 1);
    assert(last_task.mode == 1 && last_task.unit_id == 10 &&
           last_task.value_16 == 0x2d1 && last_task.value_32 == 10 &&
           last_task.flag == 1 && last_task.x_bits == 0x40a00000 &&
           last_task.y_bits == 0x40c00000 && !last_task.z_bits &&
           last_task.extra_32 == 0x3f800000 && last_task.extra_a == 1 &&
           !last_task.extra_b && last_task.extra_c == 100);
    assert(aoe_command_execute_tribe_unit_ror(record, n - 1, &ops) == -1);
    return 0;
}
