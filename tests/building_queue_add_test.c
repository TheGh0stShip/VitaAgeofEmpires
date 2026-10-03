#include "command_dispatch.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "le_bytes.h"

typedef struct {
    AoeBuildingUpdate b;
    AoeActionList list;
    AoeLiveUnit unit;
    int affordable;      /* how many more checks succeed */
    int16_t failing;     /* resource reported on failure */
    int fail_alloc;
    uint32_t status_four;
    char trace[256];
} Fixture;

static Fixture f;

static void mark(const char *text) {
    strncat(f.trace, text, sizeof(f.trace) - strlen(f.trace) - 1);
}

static int can_afford(void *p, int16_t unit_id, int16_t *failing) {
    (void)p;
    assert(*failing == unit_id); /* slot initially holds the argument */
    *failing = -1;
    if (f.affordable <= 0) {
        *failing = f.failing;
        mark("C-");
        return 0;
    }
    --f.affordable;
    mark("C");
    return 1;
}

static void pay(void *p, int16_t unit_id) {
    char text[16];
    (void)p;
    snprintf(text, sizeof(text), "P%d", unit_id);
    mark(text);
}

static void notify(void *p, uint32_t event, int16_t owner, uint32_t reason) {
    char text[32];
    (void)p;
    assert(owner == 3);
    snprintf(text, sizeof(text), "E%x:%d", event, (int32_t)reason);
    mark(text);
}

static void refund(void *p, int16_t unit_id) {
    char text[16];
    (void)p;
    snprintf(text, sizeof(text), "R%d", unit_id);
    mark(text);
}

static void *alloc(void *p, size_t count, size_t size) {
    (void)p;
    mark("A");
    return f.fail_alloc ? NULL : calloc(count, size);
}

static void dealloc(void *p, void *block) {
    (void)p;
    mark("F");
    free(block);
}

static void destroy_action(void *p, void *action) { (void)p; (void)action; }
static void action_state(void *p, void *action, uint8_t state) {
    (void)p; (void)action; (void)state;
}

static AoeLiveUnit *resolve(void *p, int32_t id) {
    (void)p;
    return id == 500 ? &f.unit : NULL;
}

static AoeBuildingQueueAddOps queue_ops(void) {
    AoeBuildingQueueAddOps ops = {
        &f, can_afford, pay, notify, alloc, dealloc,
        {&f, refund, NULL, notify}, &f.status_four};
    return ops;
}

static void reset(void) {
    free(f.b.queue_entries);
    memset(&f, 0, sizeof(f));
    f.list = (AoeActionList){&f.b.state, NULL,
                            {&f, destroy_action, action_state}};
    f.b.queue_actions = &f.list;
    f.b.owner_id = 3;
    f.b.queue_status = -1;
    f.b.state = 2;
    f.unit.building_update = &f.b;
}

static void expect(const char *trace) {
    if (strcmp(f.trace, trace)) {
        fprintf(stderr, "trace %s != %s\n", f.trace, trace);
        assert(0);
    }
    f.trace[0] = 0;
}

int main(void) {
    AoeBuildingQueueAddOps ops;
    reset();
    ops = queue_ops();

    /* Empty queue: up-front check, grow by 10, then a check before each
       payment; the entry is appended and 0x7c sent. */
    f.affordable = 3;
    assert(aoe_building_queue_add_ror(&f.b, 83, 2, &ops));
    expect("CACP83CP83E7c:0");
    assert(f.b.queue_count == 1 && f.b.queue_aux_count == 10 &&
           f.b.queue_capacity == 10 && f.b.queue_entries[0].unit_id == 83 &&
           f.b.queue_entries[0].quantity == 2 && f.b.queue_revision == 1);

    /* Same tail type: add until the third check fails with resource 2. */
    f.affordable = 1;
    f.failing = 2;
    assert(aoe_building_queue_add_ror(&f.b, 83, 3, &ops));
    expect("CP83C-E7d:2E7c:0");
    assert(f.b.queue_entries[0].quantity == 3 && f.b.queue_revision == 2);

    /* Same tail type, first check fails: no revision bump, no 0x7c. */
    f.affordable = 0;
    assert(aoe_building_queue_add_ror(&f.b, 83, 1, &ops));
    expect("C-E7d:2");
    assert(f.b.queue_revision == 2);

    /* A different type drains the old queue one refund at a time (each
       cancel sends 0x7c), then queues the new type. */
    f.affordable = 2;
    assert(aoe_building_queue_add_ror(&f.b, 75, 1, &ops));
    expect("R83E7c:0R83E7c:0R83E7c:0CCP75E7c:0");
    assert(f.b.queue_count == 1 && f.b.queue_entries[0].unit_id == 75 &&
           f.b.queue_entries[0].quantity == 1);

    /* Different type, up-front check fails: queue drained, nothing added. */
    f.affordable = 0;
    f.failing = -1;
    assert(aoe_building_queue_add_ror(&f.b, 83, 1, &ops));
    expect("R75E7c:0C-E7d:-1");
    assert(f.b.queue_count == 0);

    /* Passing the up-front check but failing the per-item check still
       appends an empty entry and sends 0x7c. */
    f.affordable = 1;
    f.failing = 1;
    assert(aoe_building_queue_add_ror(&f.b, 40, 1, &ops));
    expect("CC-E7d:1E7c:0");
    assert(f.b.queue_count == 1 && f.b.queue_entries[0].quantity == 0);

    /* Growth failure returns silently after the drain and check. */
    reset();
    ops = queue_ops();
    f.fail_alloc = 1;
    f.affordable = 5;
    assert(aoe_building_queue_add_ror(&f.b, 83, 1, &ops));
    expect("CA");
    assert(!f.b.queue_count && !f.b.queue_entries);

    /* Command 0x77 through the tribe router (record from 0x004e9c20). */
    {
        static const uint8_t expected[12] = {
            0x77, 0, 0, 0, 0xf4, 0x01, 0, 0, 83, 0, 0xfe, 0xff};
        AoeCommandBuildingOps command_ops = {{&f, resolve}, queue_ops()};
        AoeCommandTribeDispatchRor tribe;
        AoeRng rng;
        uint8_t record[12];
        memset(&tribe, 0, sizeof(tribe));
        aoe_rng_seed(&rng, 1);
        tribe.rng = &rng;
        aoe_command_bind_building_queue_ror(&tribe, &command_ops);
        f.fail_alloc = 0;
        f.affordable = 3;
        assert(aoe_command_encode_building_queue_ror(
                   record, sizeof(record), 500, 83, 3) == 12);
        /* The up-front check consumes one of three affordable checks, so
           the third per-item check fails (resource 0). */
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 12) ==
               AOE_COMMAND_ROUTE_HANDLED);
        expect("CACP83CP83C-E7d:0E7c:0");
        assert(f.b.queue_entries[0].quantity == 2);
        /* Count -2 cancels two through 0x004add50. */
        assert(aoe_command_encode_building_queue_ror(
                   record, sizeof(record), 500, 83, -2) == 12 &&
               !memcmp(record, expected, 12));
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 12) ==
               AOE_COMMAND_ROUTE_HANDLED);
        expect("R83R83E7c:0");
        assert(f.b.queue_count == 0);
        /* Unknown building: silently ignored. -32768: host guard. */
        aoe_le_put_u32(record + 4, 501);
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 12) ==
               AOE_COMMAND_ROUTE_HANDLED);
        assert(aoe_command_encode_building_queue_ror(
                   record, sizeof(record), 500, 83, INT16_MIN) == 12);
        assert(aoe_command_dispatch_tribe_ror(&tribe, record, 12) ==
               AOE_COMMAND_ROUTE_INVALID);
    }
    free(f.b.queue_entries);
    return 0;
}
