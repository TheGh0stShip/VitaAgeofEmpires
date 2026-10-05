#include "command_dispatch.h"

#include <assert.h>
#include <string.h>

static float values[0x40];
static AoePlayerResources resources = {0x40, values};
static AoePlayerResourceTargetRor player = {&resources, 0, 0, 0, 0};

static const AoePlayerResourceTargetRor *lookup(void *context,
                                                uint8_t index) {
    (void)context;
    return index == 1 ? &player : 0;
}

static AoeLiveUnit unit;

static AoeLiveUnit *resolve(void *context, int32_t unit_id) {
    (void)context;
    return unit_id == unit.core.unit_id ? &unit : 0;
}

static uint16_t owner(void *context, const AoeLiveUnit *live) {
    (void)context;
    (void)live;
    return 1;
}

static int calls;

static int tribute(void *context, const uint8_t *command, size_t size) {
    assert(context == &calls && command[0] == 7 && size == 8);
    ++calls;
    return 0;
}

int main(void) {
    AoeCommandDispatchRor dispatch;
    AoeCommandPlayerOps player_ops = {0, lookup};
    AoeCommandDependencyOps dependency_ops = {{0, resolve}, 0, owner};
    uint8_t record[0x74];
    memset(&dispatch, 0, sizeof(dispatch));

    /* Jump table 0x0042a230: 0x0d and > 0x11 are no-ops. */
    for (unsigned op = 0; op < 0x100; ++op)
        assert(aoe_command_opcode_routed_ror((uint8_t)op) ==
               (op <= 0x11 && op != 0x0d));
    record[0] = 0x0d;
    assert(aoe_command_dispatch_ror(&dispatch, record, 1) ==
           AOE_COMMAND_ROUTE_IGNORED);
    record[0] = 0x12;
    assert(aoe_command_dispatch_ror(&dispatch, record, 1) ==
           AOE_COMMAND_ROUTE_IGNORED);

    /* Routed but untranslated opcodes are surfaced, not dropped. */
    record[0] = 0;
    assert(aoe_command_dispatch_ror(&dispatch, record, 1) ==
           AOE_COMMAND_ROUTE_UNBOUND);

    aoe_command_bind_player_resource_ror(&dispatch, &player_ops);
    aoe_command_bind_dependency_ror(&dispatch, &dependency_ops);
    assert(aoe_command_encode_player_resource_add_ror(
               record, sizeof(record), 1, 2, 3.0f) == 8 &&
           aoe_command_dispatch_ror(&dispatch, record, 8) ==
               AOE_COMMAND_ROUTE_HANDLED && values[2] == 3.0f);
    record[1] = 2; /* unresolved player is a host-side INVALID */
    assert(aoe_command_dispatch_ror(&dispatch, record, 8) ==
           AOE_COMMAND_ROUTE_INVALID);

    unit.constructed = 1;
    unit.core.unit_id = 40;
    unit.dependent_source_id = -1;
    assert(aoe_command_encode_dependency_clear_ror(
               record, sizeof(record), 1, 40) == 8 &&
           aoe_command_dispatch_ror(&dispatch, record, 8) ==
               AOE_COMMAND_ROUTE_HANDLED);
    for (uint8_t op = 8; op <= 0x0f; ++op)
        assert((dispatch.opcodes[op].handler != 0) ==
               (op == 8 || op == 9 || op == 0x0e || op == 0x0f));

    /* Opcode 7 is the processor's overridable vtable slot +0x00. */
    dispatch.opcodes[7].handler = tribute;
    dispatch.opcodes[7].context = &calls;
    record[0] = 7;
    assert(aoe_command_dispatch_ror(&dispatch, record, 8) ==
               AOE_COMMAND_ROUTE_HANDLED && calls == 1);
    assert(aoe_command_dispatch_ror(0, record, 8) ==
           AOE_COMMAND_ROUTE_INVALID);

    /* Derived processor 0x004e83e0. */
    {
        AoeCommandTribeDispatchRor tribe;
        AoeRng rng, expected;
        memset(&tribe, 0, sizeof(tribe));
        aoe_rng_seed(&rng, 1);
        aoe_rng_seed(&expected, 1);
        tribe.base = dispatch;
        tribe.rng = &rng;
        for (unsigned op = 0; op < 0x100; ++op)
            assert(aoe_command_opcode_tribe_routed_ror((uint8_t)op) ==
                   (op == 4 || (op >= 0x64 && op <= 0x6f) || op == 0x77));
        /* rand() advances once for every command, even ignored ones. */
        record[0] = 0x0d;
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 1) ==
                   AOE_COMMAND_ROUTE_IGNORED &&
               rng.state == (aoe_rng_next(&expected), expected.state));
        record[0] = 0x70; /* inside the byte table but default-routed */
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 1) ==
                   AOE_COMMAND_ROUTE_IGNORED &&
               rng.state == (aoe_rng_next(&expected), expected.state));
        /* Opcode 4 uses the derived override, not the base slot. */
        tribe.base.opcodes[4].handler = tribute;
        tribe.base.opcodes[4].context = &calls;
        record[0] = 4;
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 8) ==
                   AOE_COMMAND_ROUTE_UNBOUND && calls == 1);
        record[0] = 0x6c;
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 8) ==
               AOE_COMMAND_ROUTE_UNBOUND);
        /* Fallback reaches base handlers (opcode 7 here). */
        record[0] = 7;
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 8) ==
                   AOE_COMMAND_ROUTE_HANDLED && calls == 2);
        aoe_rng_next(&expected);
        aoe_rng_next(&expected);
        aoe_rng_next(&expected);
        assert(rng.state == expected.state);
        /* Missing RNG is rejected without consuming anything. */
        tribe.rng = 0;
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 8) ==
               AOE_COMMAND_ROUTE_INVALID && calls == 2);
    }
    return 0;
}
