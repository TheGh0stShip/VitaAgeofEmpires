#ifndef VITA_AOE_PLAYER_OBJECT_MEMORY_H
#define VITA_AOE_PLAYER_OBJECT_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* Patched RoR 1.0a player +0x114 object memory: per category (a template's
   notify value, +0xa7) a list of 8-byte entries with the unit ID at +0 and
   tile x/y bytes at +6/+7 (category arrays +0x00, counts +0x08, category
   count +0x10). Read by the AI nearest-object query 0x00517290. The code
   that adds entries (reached from the player map update 0x00517060) is
   not translated yet, so a runtime table starts and stays empty. */
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

/* Portable append (the original add path is not traced yet). */
int aoe_player_object_memory_add(AoePlayerObjectMemory *memory,
                                 size_t category,
                                 const AoePlayerMemoryEntry *entry);

/* 0x00517220(unit ID, category): removes the first entry with the ID by
   moving the category's last entry into its place. Returns 1 if removed,
   0 if absent, for an empty table or an out-of-range category (the
   original indexes without a bound). */
int aoe_player_object_memory_remove(AoePlayerObjectMemory *memory,
                                    int32_t unit_id, size_t category);

#endif
