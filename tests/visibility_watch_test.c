#include "visibility_watch.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    AoeVisibilityUnitRef units[3];
    AoeVisibilityUnitRef *resolved[3];
    int recipients[8];
    unsigned notices;
} Trace;

static void notify_lost(void *user, int player_index,
                        const AoeVisibilityUnitRef *unit) {
    Trace *trace = user;
    assert(unit >= &trace->units[0] && unit <= &trace->units[2]);
    assert(trace->notices < 8);
    trace->recipients[trace->notices++] = player_index;
}

static AoeVisibilityWatchOps ops_for(Trace *trace) {
    AoeVisibilityWatchOps ops = {0};
    ops.user = trace;
    ops.notify_lost = notify_lost;
    return ops;
}

static AoeVisibilityRegistry registry_for(Trace *trace) {
    AoeVisibilityRegistry registry = {0};
    registry.positive = trace->resolved;
    registry.positive_count = 3;
    registry.positive_capacity = 3;
    return registry;
}

static void test_invalid_removal(AoeVisibilityEdition edition) {
    Trace trace = {0};
    AoeVisibilityWatchOps ops = ops_for(&trace);
    AoeVisibilityRegistry registry = registry_for(&trace);
    uint32_t live[3] = {0, 0, 0};
    AoeVisibilityWatchEntry entries[3] = {{0}};
    AoeVisibilityWatch watch = {.entries = entries, .count = 3,
                                .capacity = 3};
    trace.resolved[2] = &trace.units[1];
    for (int i = 0; i < 3; ++i) {
        entries[i].unit = &trace.units[i];
        entries[i].live_flags = &live[i];
        entries[i].previous_flags = 2;
        entries[i].unit_id = i + 1;
    }
    assert(aoe_visibility_watch_step(&watch, edition, 0,
                                     &registry, &ops));
    assert(trace.notices == 1 && trace.recipients[0] == 1);
    if (edition == AOE_VISIBILITY_RISE_OF_ROME) {
        assert(watch.count == 1 && entries[0].unit == &trace.units[1]);
        assert(entries[0].previous_flags == 0);
        assert(entries[1].unit == 0 && entries[2].unit == 0);
    } else {
        assert(watch.count == 2);
        assert(entries[0].unit == &trace.units[0]);
        assert(entries[1].unit == &trace.units[1]);
        assert(entries[0].previous_flags == 0);
        assert(entries[1].previous_flags == 0);
        assert(entries[2].unit == 0);
    }
}

static void test_bit_recipients(AoeVisibilityEdition edition) {
    Trace trace = {0};
    AoeVisibilityWatchOps ops = ops_for(&trace);
    AoeVisibilityRegistry registry = registry_for(&trace);
    uint32_t live = 0;
    AoeVisibilityWatchEntry entry = {.unit = &trace.units[0],
                                     .live_flags = &live,
                                     .previous_flags = 0x800au,
                                     .unit_id = 1};
    AoeVisibilityWatch watch = {.entries = &entry, .count = 1,
                                .capacity = 1};
    trace.resolved[1] = &trace.units[0];
    assert(aoe_visibility_watch_step(&watch, edition, 3,
                                     &registry, &ops));
    assert(trace.notices == 2);
    assert(trace.recipients[0] == 1 && trace.recipients[1] == 15);
    assert(entry.previous_flags == 0 && watch.count == 1);
    assert(aoe_visibility_watch_step(&watch, edition, 3,
                                     &registry, &ops));
    assert(trace.notices == 2); /* No change, no second notification. */
}

static void test_remove_and_invalid_state(void) {
    Trace trace = {0};
    uint32_t flags[2] = {0, 0};
    AoeVisibilityWatchEntry entries[2] = {
        {.unit = &trace.units[0], .live_flags = &flags[0]},
        {.unit = &trace.units[1], .live_flags = &flags[1]}
    };
    AoeVisibilityWatch watch = {.entries = entries, .count = 2,
                                .capacity = 2};
    AoeVisibilityWatchOps ops = ops_for(&trace);
    AoeVisibilityRegistry registry = registry_for(&trace);
    assert(aoe_visibility_watch_remove(&watch, &trace.units[0]));
    assert(watch.count == 1 && entries[0].unit == &trace.units[1]);
    assert(entries[1].unit == 0);
    assert(!aoe_visibility_watch_remove(&watch, &trace.units[0]));
    entries[0].live_flags = 0;
    assert(!aoe_visibility_watch_step(&watch, AOE_VISIBILITY_BASE, 0,
                                      &registry, &ops));
    assert(watch.count == 1 && trace.notices == 0);
}

static void test_add(void) {
    Trace trace = {0};
    uint32_t flags = 0x12345678;
    AoeVisibilityWatch watch = {0};
    assert(!aoe_visibility_watch_add(&watch, 1, 2, &trace.units[0],
                                     &flags, 9));
    assert(aoe_visibility_watch_add(&watch, 1, 1, &trace.units[0],
                                    &flags, 9));
    assert(watch.count == 1 && watch.capacity == 32 &&
           watch.entries[0].unit == &trace.units[0] &&
           watch.entries[0].live_flags == &flags &&
           watch.entries[0].previous_flags == flags &&
           watch.entries[0].unit_id == 9);
    assert(!aoe_visibility_watch_add(&watch, 1, 1, &trace.units[0],
                                     &flags, 9));
    assert(aoe_visibility_watch_remove(&watch, &trace.units[0]));
    aoe_visibility_watch_destroy(&watch);
    assert(!watch.entries && watch.count == 0 && watch.capacity == 0);
}

static void test_world_registry_id_rules(void) {
    Trace trace = {0};
    AoeVisibilityUnitRef *positive[3] = {&trace.units[0], 0,
                                          &trace.units[1]};
    AoeVisibilityUnitRef *negative[3] = {0, &trace.units[2],
                                          &trace.units[0]};
    AoeVisibilityRegistry registry = {
        .positive = positive, .positive_count = 3, .positive_capacity = 3,
        .negative = negative, .negative_count = 3, .negative_capacity = 3
    };
    assert(aoe_visibility_registry_valid(&registry));
    assert(aoe_visibility_registry_resolve(&registry, 0) == &trace.units[0]);
    assert(aoe_visibility_registry_resolve(&registry, 1) == 0);
    assert(aoe_visibility_registry_resolve(&registry, 2) == &trace.units[1]);
    assert(aoe_visibility_registry_resolve(&registry, 3) == 0);
    assert(aoe_visibility_registry_resolve(&registry, -1) == &trace.units[2]);
    assert(aoe_visibility_registry_resolve(&registry, -2) == &trace.units[0]);
    assert(aoe_visibility_registry_resolve(&registry, -3) == 0);
    assert(aoe_visibility_registry_resolve(&registry, INT32_MIN) == 0);
    registry.negative_capacity = 2;
    assert(!aoe_visibility_registry_valid(&registry));
    assert(aoe_visibility_registry_resolve(&registry, -1) == 0);
}

static void test_owned_world_registry(void) {
    Trace trace = {0};
    AoeOwnedVisibilityRegistry owned;
    AoeVisibilityUnitRef *replaced = 0;
    int needs_reassign = 0;
    assert(aoe_owned_visibility_registry_init(&owned));
    assert(owned.view.positive_count == 10000);
    assert(owned.view.negative_count == 1000);
    assert(owned.next_positive_id == 0 && owned.next_negative_id == -1);
    assert(aoe_owned_visibility_registry_insert(&owned, 0, &trace.units[0],
                                                 &replaced, &needs_reassign));
    assert(replaced == 0 && owned.positive_insertions == 1);
    assert(aoe_visibility_registry_resolve(&owned.view, 0) == &trace.units[0]);
    assert(aoe_owned_visibility_registry_insert(&owned, 7, &trace.units[1],
                                                 &replaced, &needs_reassign));
    assert(replaced == 0 && owned.view.positive_count >= 8);
    assert(aoe_visibility_registry_resolve(&owned.view, 0) == &trace.units[0]);
    assert(aoe_visibility_registry_resolve(&owned.view, 6) == 0);
    assert(aoe_owned_visibility_registry_insert(&owned, 12000,
                                                 &trace.units[1], &replaced,
                                                 &needs_reassign));
    assert(replaced == 0 && owned.view.positive_count > 12000);
    assert(aoe_visibility_registry_resolve(&owned.view, 0) == &trace.units[0]);
    assert(aoe_owned_visibility_registry_insert(&owned, 7, &trace.units[2],
                                                 &replaced, &needs_reassign));
    assert(replaced == &trace.units[1] && !needs_reassign &&
           owned.positive_insertions == 4);
    assert(aoe_visibility_registry_resolve(&owned.view, 7) == &trace.units[2]);
    assert(aoe_owned_visibility_registry_insert(&owned, -1, &trace.units[1],
                                                 &replaced, &needs_reassign));
    assert(replaced == 0 && owned.negative_insertions == 1);
    assert(owned.view.negative_count >= 2 && owned.view.negative[0] == 0);
    assert(aoe_visibility_registry_resolve(&owned.view, -1) == &trace.units[1]);
    assert(aoe_owned_visibility_registry_insert(&owned, -9, &trace.units[0],
                                                 &replaced, &needs_reassign));
    assert(replaced == 0 && owned.view.negative_count >= 10);
    assert(aoe_visibility_registry_resolve(&owned.view, -1) == &trace.units[1]);
    assert(aoe_owned_visibility_registry_insert(&owned, -1200,
                                                 &trace.units[2], &replaced,
                                                 &needs_reassign));
    assert(replaced == 0 && owned.view.negative_count > 1200);
    assert(aoe_visibility_registry_resolve(&owned.view, -1) == &trace.units[1]);
    assert(aoe_owned_visibility_registry_clear(&owned, 7));
    assert(aoe_owned_visibility_registry_clear(&owned, -1));
    assert(aoe_owned_visibility_registry_clear(&owned, 7));
    assert(aoe_visibility_registry_resolve(&owned.view, 7) == 0);
    assert(aoe_visibility_registry_resolve(&owned.view, -1) == 0);
    assert(owned.positive_insertions == 4 && owned.negative_insertions == 3);
    assert(!aoe_owned_visibility_registry_clear(&owned, 30000));
    assert(!aoe_owned_visibility_registry_clear(&owned, -30000));
    assert(!aoe_owned_visibility_registry_clear(&owned, INT32_MIN));
    assert(!aoe_owned_visibility_registry_insert(&owned, INT32_MIN,
                                                  &trace.units[0], &replaced,
                                                  &needs_reassign));
    assert(aoe_owned_visibility_registry_insert(&owned, 7, &trace.units[0],
                                                 &replaced, &needs_reassign));
    assert(!needs_reassign && replaced == 0);
    owned.special_id_mode = 1;
    assert(aoe_owned_visibility_registry_insert(&owned, 7, &trace.units[1],
                                                 &replaced, &needs_reassign));
    assert(needs_reassign && replaced == &trace.units[0]);
    assert(!owned.special_id_mode);
    assert(aoe_visibility_registry_resolve(&owned.view, 7) == &trace.units[1]);
    owned.next_positive_id = 23;
    owned.next_negative_id = -23;
    owned.special_id_mode = 1;
    owned.reserved_id = 41;
    assert(aoe_owned_visibility_registry_clear_id_tables(&owned));
    assert(aoe_visibility_registry_resolve(&owned.view, 7) == 0);
    assert(aoe_visibility_registry_resolve(&owned.view, 12000) == 0);
    assert(aoe_visibility_registry_resolve(&owned.view, -9) == 0);
    assert(aoe_visibility_registry_resolve(&owned.view, -1200) == 0);
    assert(owned.next_positive_id == 0 && owned.next_negative_id == -1);
    assert(owned.positive_insertions == 6 && owned.negative_insertions == 3);
    assert(owned.special_id_mode == 1 && owned.reserved_id == 41);
    aoe_owned_visibility_registry_destroy(&owned);
    assert(aoe_visibility_registry_valid(&owned.view));
    assert(owned.positive_insertions == 0 && owned.negative_insertions == 0);
}

static void test_world_id_allocator(void) {
    Trace trace = {0};
    AoeOwnedVisibilityRegistry owned;
    AoeVisibilityUnitRef *replaced = 0;
    int needs_reassign = 0;
    int32_t id = 123;
    assert(aoe_owned_visibility_registry_init(&owned));
    assert(aoe_owned_visibility_registry_next_positive_id(&owned, &id));
    assert(id == 0 && owned.next_positive_id == 1);
    assert(aoe_owned_visibility_registry_insert(&owned, 1, &trace.units[0],
                                                 &replaced, &needs_reassign));
    assert(aoe_owned_visibility_registry_next_positive_id(&owned, &id));
    assert(id == 2 && owned.next_positive_id == 3);
    owned.special_id_mode = 1;
    owned.reserved_id = 7;
    assert(aoe_owned_visibility_registry_next_positive_id(&owned, &id));
    assert(id == 7 && owned.next_positive_id == 3);
    owned.special_id_mode = 0;
    assert(aoe_owned_visibility_registry_next_negative_id(&owned, &id));
    assert(id == -1 && owned.next_negative_id == -2);
    owned.special_id_mode = 1;
    owned.reserved_id = -10;
    assert(aoe_owned_visibility_registry_next_negative_id(&owned, &id));
    assert(id == -10 && owned.next_negative_id == -11);
    owned.reserved_id = -5;
    assert(aoe_owned_visibility_registry_next_negative_id(&owned, &id));
    assert(id == -5 && owned.next_negative_id == -11);
    owned.special_id_mode = 0;
    assert(aoe_owned_visibility_registry_next_negative_id(&owned, &id));
    assert(id == -11 && owned.next_negative_id == -12);
    owned.next_positive_id = INT32_MAX;
    assert(!aoe_owned_visibility_registry_next_positive_id(&owned, &id));
    owned.next_negative_id = INT32_MIN;
    assert(!aoe_owned_visibility_registry_next_negative_id(&owned, &id));
    aoe_owned_visibility_registry_destroy(&owned);
}

static void init_test(void) {
    AoeVisibilityWatch w;
    assert(aoe_visibility_watch_init(&w, 100) && w.capacity == 100 && w.count == 0 && w.entries);
    aoe_visibility_watch_destroy(&w);
    assert(w.entries == NULL && w.capacity == 0);
}

int main(void) {
    init_test();
    test_invalid_removal(AOE_VISIBILITY_BASE);
    test_invalid_removal(AOE_VISIBILITY_RISE_OF_ROME);
    test_bit_recipients(AOE_VISIBILITY_BASE);
    test_bit_recipients(AOE_VISIBILITY_RISE_OF_ROME);
    test_remove_and_invalid_state();
    test_add();
    test_world_registry_id_rules();
    test_owned_world_registry();
    test_world_id_allocator();
    return 0;
}
