#include "player_object_memory.h"

#include <stdlib.h>
#include <string.h>

int aoe_player_object_memory_init(AoePlayerObjectMemory *memory,
                                  size_t category_count) {
    if (!memory) return 0;
    memset(memory, 0, sizeof(*memory));
    if (!category_count) return 1;
    memory->categories = calloc(category_count, sizeof(*memory->categories));
    if (!memory->categories) return 0;
    memory->category_count = category_count;
    for (size_t i = 0; i < category_count; ++i) {
        memory->categories[i].entries =
            calloc(AOE_PLAYER_MEMORY_INITIAL, sizeof(AoePlayerMemoryEntry));
        if (!memory->categories[i].entries) {
            aoe_player_object_memory_free(memory);
            return 0;
        }
        memory->categories[i].capacity = AOE_PLAYER_MEMORY_INITIAL;
    }
    return 1;
}

void aoe_player_object_memory_free(AoePlayerObjectMemory *memory) {
    if (!memory) return;
    for (size_t i = 0; i < memory->category_count; ++i)
        free(memory->categories[i].entries);
    free(memory->categories);
    memset(memory, 0, sizeof(*memory));
}

int aoe_player_object_memory_add(AoePlayerObjectMemory *memory,
                                 size_t category,
                                 const AoePlayerMemoryEntry *entry) {
    AoePlayerMemoryCategory *c;
    if (!memory || !entry || category >= memory->category_count) return 0;
    c = &memory->categories[category];
    if (c->count == c->capacity) {
        size_t next = c->capacity ? c->capacity * 2 : AOE_PLAYER_MEMORY_INITIAL;
        AoePlayerMemoryEntry *grown =
            realloc(c->entries, next * sizeof(*grown));
        if (!grown) return 0;
        c->entries = grown;
        c->capacity = next;
    }
    c->entries[c->count++] = *entry;
    return 1;
}

int aoe_player_object_memory_remove(AoePlayerObjectMemory *memory,
                                    int32_t unit_id, size_t category) {
    AoePlayerMemoryCategory *c;
    if (!memory || !memory->category_count ||
        category >= memory->category_count)
        return 0;
    c = &memory->categories[category];
    for (size_t i = 0; i < c->count; ++i) {
        if (c->entries[i].unit_id != unit_id) continue;
        if (i + 1 < c->count) c->entries[i] = c->entries[c->count - 1];
        --c->count;
        return 1;
    }
    return 0;
}
