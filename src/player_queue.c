#include "player_queue.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int valid_edition(AoePlayerQueueEdition edition) {
    return edition == AOE_PLAYER_QUEUE_BASE ||
           edition == AOE_PLAYER_QUEUE_RISE_OF_ROME;
}

static int fits_entries(uint32_t count) {
    return (uint64_t)count * sizeof(AoePlayerQueueEntry) <= (uint64_t)SIZE_MAX;
}

int aoe_player_queue_init(AoePlayerQueue *queue, uint32_t capacity,
                          AoePlayerQueueEdition edition) {
    AoePlayerQueueEntry *storage;
    if (!queue || !valid_edition(edition) || !capacity ||
        (edition == AOE_PLAYER_QUEUE_BASE && capacity < 100) ||
        capacity > INT32_MAX || !fits_entries(capacity) || queue->storage_10)
        return 0;
    storage = malloc((size_t)capacity * sizeof(*storage));
    if (!storage) return 0;
    queue->capacity_00 = capacity;
    queue->field_04 = 0;
    queue->field_08 = 0;
    queue->dirty_0c = 0;
    queue->storage_10 = storage;
    return 1;
}

int aoe_player_queue_append(AoePlayerQueue *queue, uint32_t first,
                            uint32_t second, AoePlayerQueueEdition edition) {
    uint32_t capacity;
    AoePlayerQueueEntry *replacement;
    if (!queue || !valid_edition(edition) || !queue->storage_10 ||
        queue->field_04 > queue->capacity_00 ||
        queue->capacity_00 > INT32_MAX || queue->field_08 == UINT32_MAX)
        return 0;
    capacity = queue->capacity_00;
    if (queue->field_04 == capacity) {
        if (!capacity || capacity > INT32_MAX / 2 ||
            (edition == AOE_PLAYER_QUEUE_BASE && capacity > 65000) ||
            !fits_entries(capacity * 2))
            return 0;
        replacement = malloc((size_t)capacity * 2 * sizeof(*replacement));
        if (!replacement) return 0;
        memcpy(replacement, queue->storage_10,
               (size_t)capacity * sizeof(*replacement));
        free(queue->storage_10);
        queue->storage_10 = replacement;
        queue->capacity_00 = capacity * 2;
    }
    queue->storage_10[queue->field_04].first = first;
    queue->storage_10[queue->field_04].second = second;
    queue->field_04++;
    queue->field_08++;
    return 1;
}

int aoe_player_queue_reset(AoePlayerQueue *queue) {
    void *replacement = 0;
    if (!queue) return 0;
    if (queue->dirty_0c) {
        replacement = malloc(8u * 8u);
        if (!replacement) return 0;
        free(queue->storage_10);
        queue->storage_10 = replacement;
        queue->capacity_00 = 8;
        queue->dirty_0c = 0;
    }
    queue->field_04 = 0;
    queue->field_08 = 0;
    return 1;
}

int aoe_player_queue_view(const AoePlayerQueue *queue,
                          const AoePlayerQueueEntry **entries,
                          uint32_t *used) {
    if (!queue || !entries || !used) return 0;
    *entries = queue->storage_10;
    *used = queue->field_04;
    return 1;
}

int aoe_player_queue_take_pending(AoePlayerQueue *queue, uint32_t *pending) {
    if (!queue || !pending) return 0;
    *pending = queue->field_08;
    queue->field_08 = 0;
    return 1;
}

void aoe_player_queue_dispose(AoePlayerQueue *queue) {
    if (!queue) return;
    free(queue->storage_10);
    memset(queue, 0, sizeof(*queue));
}
