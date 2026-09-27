#include "player_queue.h"

#include <assert.h>
#include <stdlib.h>

static void test_reset(void) {
    AoePlayerQueue queue = {0};
    assert(!aoe_player_queue_reset(0));
    queue.capacity_00 = 4;
    queue.field_04 = 3;
    queue.field_08 = 5;
    assert(aoe_player_queue_reset(&queue));
    assert(queue.capacity_00 == 4 && queue.field_04 == 0 &&
           queue.field_08 == 0 && queue.storage_10 == 0);
    queue.storage_10 = malloc(4u * 8u);
    assert(queue.storage_10);
    queue.dirty_0c = 1;
    queue.field_04 = 9;
    queue.field_08 = 7;
    assert(aoe_player_queue_reset(&queue));
    assert(queue.capacity_00 == 8 && queue.storage_10 &&
           queue.field_04 == 0 && queue.field_08 == 0 && queue.dirty_0c == 0);
    aoe_player_queue_dispose(&queue);
}

static void test_ror_append_and_observe(void) {
    AoePlayerQueue queue = {0};
    const AoePlayerQueueEntry *entries = 0;
    uint32_t used = 0;
    uint32_t pending = 0;
    assert(!aoe_player_queue_init(&queue, 0, AOE_PLAYER_QUEUE_RISE_OF_ROME));
    assert(aoe_player_queue_init(&queue, 2, AOE_PLAYER_QUEUE_RISE_OF_ROME));
    assert(!aoe_player_queue_init(&queue, 2, AOE_PLAYER_QUEUE_RISE_OF_ROME));
    assert(aoe_player_queue_append(&queue, 11, 12,
                                   AOE_PLAYER_QUEUE_RISE_OF_ROME));
    assert(aoe_player_queue_append(&queue, 21, 22,
                                   AOE_PLAYER_QUEUE_RISE_OF_ROME));
    assert(aoe_player_queue_append(&queue, 31, 32,
                                   AOE_PLAYER_QUEUE_RISE_OF_ROME));
    assert(queue.capacity_00 == 4);
    assert(aoe_player_queue_view(&queue, &entries, &used));
    assert(entries == queue.storage_10 && used == 3);
    assert(entries[0].first == 11 && entries[0].second == 12);
    assert(entries[1].first == 21 && entries[1].second == 22);
    assert(entries[2].first == 31 && entries[2].second == 32);
    assert(aoe_player_queue_take_pending(&queue, &pending) && pending == 3);
    assert(queue.field_08 == 0 && queue.field_04 == 3);
    assert(aoe_player_queue_take_pending(&queue, &pending) && pending == 0);
    assert(aoe_player_queue_reset(&queue));
    assert(queue.field_04 == 0 && queue.field_08 == 0);
    aoe_player_queue_dispose(&queue);
    assert(queue.storage_10 == 0 && queue.capacity_00 == 0);
}

static void test_base_growth_rules(void) {
    AoePlayerQueue queue = {0};
    assert(!aoe_player_queue_init(&queue, 99, AOE_PLAYER_QUEUE_BASE));
    assert(aoe_player_queue_init(&queue, 100, AOE_PLAYER_QUEUE_BASE));
    for (uint32_t i = 0; i < 101; ++i)
        assert(aoe_player_queue_append(&queue, i, i + 1,
                                       AOE_PLAYER_QUEUE_BASE));
    assert(queue.capacity_00 == 200 && queue.field_04 == 101 &&
           queue.field_08 == 101);
    for (uint32_t i = 0; i < 101; ++i)
        assert(queue.storage_10[i].first == i &&
               queue.storage_10[i].second == i + 1);
    aoe_player_queue_dispose(&queue);

    assert(aoe_player_queue_init(&queue, 65001, AOE_PLAYER_QUEUE_BASE));
    queue.field_04 = 65001;
    assert(!aoe_player_queue_append(&queue, 1, 2, AOE_PLAYER_QUEUE_BASE));
    assert(queue.capacity_00 == 65001 && queue.field_04 == 65001 &&
           queue.field_08 == 0);
    aoe_player_queue_dispose(&queue);
}

int main(void) {
    test_reset();
    test_ror_append_and_observe();
    test_base_growth_rules();
    return 0;
}
