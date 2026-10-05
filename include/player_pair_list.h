#ifndef VITA_AOE_PLAYER_PAIR_LIST_H
#define VITA_AOE_PLAYER_PAIR_LIST_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a growable list of 32-bit value pairs (0x14
   bytes). The player constructors embed two, at +0x58 and +0x6c, each
   created with room for 400 pairs. Semantic storage; offsets noted. */
typedef struct {
    int32_t capacity;   /* +0x00, in pairs */
    int32_t count;      /* +0x04 */
    int32_t added;      /* +0x08, appended since the last take */
    int32_t shrink;     /* +0x0c, reset the storage on the next clear */
    int32_t *pairs;     /* +0x10, capacity * 2 values */
} AoePlayerPairListRor;

/* 0x0045eb80: zeroed storage for `capacity` pairs; everything else 0.
   Returns 0 on allocation failure. */
int aoe_pair_list_init_ror(AoePlayerPairListRor *list, int32_t capacity);

/* 0x0045ebb0: release the storage. */
void aoe_pair_list_destroy_ror(AoePlayerPairListRor *list);

/* 0x0045eac0: append (first, second), doubling the storage when full.
   Port: an empty capacity grows to one pair (the original doubles 0 and
   writes past it). Returns 0 on allocation failure. */
int aoe_pair_list_append_ror(AoePlayerPairListRor *list, int32_t first,
                             int32_t second);

/* 0x0045eb40: empty the list; with `shrink` set, the storage is replaced
   by a zeroed one of 8 pairs and the flag cleared. */
int aoe_pair_list_clear_ror(AoePlayerPairListRor *list);

/* 0x0045ebc0: the storage and the count. */
void aoe_pair_list_get_ror(const AoePlayerPairListRor *list,
                           const int32_t **pairs, int32_t *count);

/* 0x0045ebe0: the number appended since the last call (then 0). */
int32_t aoe_pair_list_take_added_ror(AoePlayerPairListRor *list);

#endif
