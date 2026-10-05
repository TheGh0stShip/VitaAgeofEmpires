#include "player_pair_list.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int aoe_pair_list_init_ror(AoePlayerPairListRor *list, int32_t capacity) {
    if (!list || capacity < 0) return 0;
    list->pairs = calloc(capacity ? (size_t)capacity : 1, 2 * sizeof(int32_t));
    list->capacity = capacity;
    list->count = 0;
    list->added = 0;
    list->shrink = 0;
    return list->pairs != NULL;
}

void aoe_pair_list_destroy_ror(AoePlayerPairListRor *list) {
    if (!list) return;
    free(list->pairs);
    list->pairs = NULL;
}

int aoe_pair_list_append_ror(AoePlayerPairListRor *list, int32_t first,
                             int32_t second) {
    if (!list) return 0;
    if (list->capacity <= list->count) {
        int32_t grown_capacity = list->capacity > 0 ? list->capacity * 2 : 1;
        int32_t *grown =
            calloc((size_t)grown_capacity, 2 * sizeof(int32_t));
        if (!grown) return 0;
        if (list->pairs && list->capacity > 0)
            memcpy(grown, list->pairs,
                   (size_t)list->capacity * 2 * sizeof(int32_t));
        free(list->pairs);
        list->pairs = grown;
        list->capacity = grown_capacity;
    }
    /* Port: a count beyond the capacity (never produced here) is refused. */
    if (list->count < 0 || list->count >= list->capacity) return 0;
    list->pairs[list->count * 2] = first;
    list->pairs[list->count * 2 + 1] = second;
    list->count++;
    list->added++;
    return 1;
}

int aoe_pair_list_clear_ror(AoePlayerPairListRor *list) {
    if (!list) return 0;
    list->count = 0;
    list->added = 0;
    if (list->shrink) {
        free(list->pairs);
        list->capacity = 8;
        list->pairs = calloc(8, 2 * sizeof(int32_t));
        list->shrink = 0;
        if (!list->pairs) {
            list->capacity = 0;
            return 0;
        }
    }
    return 1;
}

void aoe_pair_list_get_ror(const AoePlayerPairListRor *list,
                           const int32_t **pairs, int32_t *count) {
    if (pairs) *pairs = list ? list->pairs : NULL;
    if (count) *count = list ? list->count : 0;
}

int32_t aoe_pair_list_take_added_ror(AoePlayerPairListRor *list) {
    int32_t added;
    if (!list) return 0;
    added = list->added;
    list->added = 0;
    return added;
}
