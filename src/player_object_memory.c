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

static int memory_read_i32(AoePlayerMemoryReadFn read, void *context, int32_t *out) {
    uint8_t b[4];
    if (!read(context, b, 4)) return 0;
    *out = (int32_t)((uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 |
                     (uint32_t)b[3] << 24);
    return 1;
}

/* 0x00516e50 */
int aoe_player_object_memory_load(AoePlayerObjectMemory *memory,
                                  AoePlayerMemoryReadFn read, void *context,
                                  float version) {
    int32_t count = 5;
    if (!memory) return 0;
    memset(memory, 0, sizeof(*memory));
    if (version >= 7.11f && !memory_read_i32(read, context, &count)) return 0;
    if (count < 1) return 1;
    memory->categories = calloc((size_t)count, sizeof(*memory->categories));
    if (!memory->categories) return 0;
    memory->category_count = (size_t)count;
    for (int32_t i = 0; i < count; ++i) {
        int32_t capacity, used;
        if (!memory_read_i32(read, context, &capacity) ||
            !memory_read_i32(read, context, &used) || capacity < 0 || used < 0 ||
            used > capacity)
            goto fail;
        memory->categories[i].capacity = (size_t)capacity;
        memory->categories[i].count = (size_t)used;
        memory->categories[i].entries =
            calloc(capacity ? (size_t)capacity : 1, sizeof(AoePlayerMemoryEntry));
        if (!memory->categories[i].entries) goto fail;
    }
    for (int32_t i = 0; i < count; ++i) {
        for (size_t j = 0; j < memory->categories[i].count; ++j) {
            AoePlayerMemoryEntry *e = &memory->categories[i].entries[j];
            uint8_t b[4];
            if (!memory_read_i32(read, context, &e->unit_id) || !read(context, b, 4))
                goto fail;
            e->field_04 = b[0];
            e->field_05 = b[1];
            e->x = b[2];
            e->y = b[3];
        }
    }
    return 1;
fail:
    aoe_player_object_memory_free(memory);
    return 0;
}
