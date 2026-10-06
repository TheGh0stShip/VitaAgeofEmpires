#ifndef VITA_AOE_PLAYER_OBJECT_MEMORY_H
#define VITA_AOE_PLAYER_OBJECT_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* Patched RoR 1.0a player +0x114 object memory: per category (a template's
   notify value, +0xa7) a list of 8-byte entries with the unit ID at +0 and
   tile x/y bytes at +6/+7 (category arrays +0x00, counts +0x08, category
   count +0x10, owner +0x0c). Read by the AI nearest-object query
   0x00517290. The tribe constructor 0x004efb00 builds it with
   0x00516da0(player, 5): five categories of 32 entries each. Entries are
   added by the player map update 0x00517060 / 0x00517130. */
enum {
    AOE_PLAYER_MEMORY_CATEGORIES = 5,     /* 0x004efb6e push 5 */
    AOE_PLAYER_MEMORY_INITIAL = 32        /* 0x00516df6, 0x100 bytes */
};
typedef struct {
    int32_t unit_id;
    uint8_t field_04, field_05; /* not yet identified */
    uint8_t x, y;
} AoePlayerMemoryEntry;

typedef struct {
    AoePlayerMemoryEntry *entries;
    size_t count, capacity;
} AoePlayerMemoryCategory;

typedef struct {
    AoePlayerMemoryCategory *categories;
    size_t category_count;
} AoePlayerObjectMemory;

int aoe_player_object_memory_init(AoePlayerObjectMemory *memory,
                                  size_t category_count);
void aoe_player_object_memory_free(AoePlayerObjectMemory *memory);

/* 0x00516da0: `category_count` categories of AOE_PLAYER_MEMORY_INITIAL
   entries each. */

/* 0x00517130 append: doubles a full category (allocate, copy, free), then
   stores the entry. No duplicate check. Returns 0 for an out-of-range
   category (the original indexes without a bound) or allocation failure. */
int aoe_player_object_memory_add(AoePlayerObjectMemory *memory,
                                 size_t category,
                                 const AoePlayerMemoryEntry *entry);

/* 0x00517220(unit ID, category): removes the first entry with the ID by
   moving the category's last entry into its place. Returns 1 if removed,
   0 if absent, for an empty table or an out-of-range category (the
   original indexes without a bound). */
int aoe_player_object_memory_remove(AoePlayerObjectMemory *memory,
                                    int32_t unit_id, size_t category);

/* 0x00516e50: the saved form. Before game version 7.11 (0x0054a720) the
   category count is 5, else a dword; per category the capacity and
   count dwords, then per category `count` 8-byte entries (little
   endian: ID, two bytes, x, y). The owner (+0x0c) is the caller's. The
   original trusts the stream; a negative size, count above capacity or
   short read fails here (0, the memory left empty). */
typedef int (*AoePlayerMemoryReadFn)(void *context, void *buffer, size_t size);
int aoe_player_object_memory_load(AoePlayerObjectMemory *memory,
                                  AoePlayerMemoryReadFn read, void *context,
                                  float version);

#endif
