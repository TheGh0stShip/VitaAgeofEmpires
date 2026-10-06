#include "player_object_memory.h"

#include <assert.h>

/* Player +0x114 removal 0x00517220: first matching ID, swap-with-last. */
#include <string.h>

typedef struct { const uint8_t *p; size_t n; } MemStream;
static int mem_read(void *c, void *buf, size_t size) {
    MemStream *s = c;
    if (size > s->n) return 0;
    memcpy(buf, s->p, size);
    s->p += size;
    s->n -= size;
    return 1;
}

static void test_load(void) {
    /* 0x00516e50 */
    static const uint8_t v711[] = {
        2, 0, 0, 0,  4, 0, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
        0x34, 0x12, 0, 0, 9, 8, 7, 6};
    uint8_t old[5 * 8 + 8];
    AoePlayerObjectMemory m;
    MemStream s = {v711, sizeof v711};
    assert(aoe_player_object_memory_load(&m, mem_read, &s, 7.11f));
    assert(m.category_count == 2 && m.categories[0].capacity == 4 && m.categories[0].count == 1);
    assert(m.categories[1].count == 0 && s.n == 0);
    assert(m.categories[0].entries[0].unit_id == 0x1234 && m.categories[0].entries[0].x == 7 &&
           m.categories[0].entries[0].y == 6 && m.categories[0].entries[0].field_05 == 8);
    aoe_player_object_memory_free(&m);
    /* Before 7.11: five categories, no count dword. */
    memset(old, 0, sizeof old);
    old[32] = 2; old[36] = 1;
    old[40] = 5; old[47] = 3;
    s.p = old; s.n = sizeof old;
    assert(aoe_player_object_memory_load(&m, mem_read, &s, 7.1f));
    assert(m.category_count == 5 && m.categories[4].count == 1 && m.categories[4].entries[0].unit_id == 5 &&
           m.categories[4].entries[0].y == 3 && s.n == 0);
    aoe_player_object_memory_free(&m);
    s.p = v711; s.n = 20;
    assert(!aoe_player_object_memory_load(&m, mem_read, &s, 8.0f) && !m.categories);
}

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
    test_load();
    return 0;
}
