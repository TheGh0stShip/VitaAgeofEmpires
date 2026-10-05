#include "player_object_memory.h"

#include <assert.h>

/* Player +0x114 removal 0x00517220: first matching ID, swap-with-last. */
int main(void) {
    AoePlayerObjectMemory m;
    AoePlayerMemoryEntry e = {0};
    assert(aoe_player_object_memory_init(&m, 4));
    for (int32_t id = 10; id < 14; ++id) {
        e.unit_id = id;
        e.x = (uint8_t)id;
        assert(aoe_player_object_memory_add(&m, 2, &e));
    }
    assert(aoe_player_object_memory_remove(&m, 11, 2));
    assert(m.categories[2].count == 3);
    assert(m.categories[2].entries[0].unit_id == 10 &&
           m.categories[2].entries[1].unit_id == 13 &&
           m.categories[2].entries[1].x == 13 &&
           m.categories[2].entries[2].unit_id == 12);
    assert(aoe_player_object_memory_remove(&m, 12, 2)); /* last: no move */
    assert(m.categories[2].count == 2);
    assert(!aoe_player_object_memory_remove(&m, 99, 2)); /* absent */
    assert(!aoe_player_object_memory_remove(&m, 10, 1)); /* other category */
    assert(!aoe_player_object_memory_remove(&m, 10, 4)); /* out of range */
    aoe_player_object_memory_free(&m);
    assert(aoe_player_object_memory_init(&m, 0));
    assert(!aoe_player_object_memory_remove(&m, 1, 0)); /* empty table */
    aoe_player_object_memory_free(&m);
    return 0;
}
