#include "comm_queue.h"

#include <stdlib.h>
#include <string.h>

static uint8_t *copy_data(const void *data, uint32_t size) {
    uint8_t *p = malloc((size_t)size + 1);
    if (p && size) memcpy(p, data, size);
    return p;
}

int aoe_comm_queue_init_ror(AoeCommQueueRor *q, int32_t capacity) {
    if (capacity < 0x14) capacity = 0x14;
    q->current = 0;
    q->high = 0;
    q->seq_turn = 0;
    q->entries = NULL;
    q->capacity = (uint32_t)capacity;
    q->seq = 1;
    return aoe_comm_queue_grow_ror(q, (uint32_t)capacity);
}

void aoe_comm_queue_entry_clear_ror(AoeCommQueueEntryRor *e) {
    e->key = 0;
    e->turn = 0;
    e->seq = 0;
    e->player = 0;
    e->size = 0;
    e->extra = 0;
    free(e->data);
    e->data = NULL;
}

void aoe_comm_queue_release_ror(AoeCommQueueRor *q) {
    uint32_t i;
    if (!q->entries) return;
    for (i = 0; i <= q->capacity; ++i) aoe_comm_queue_entry_clear_ror(&q->entries[i]);
    free(q->entries);
    q->entries = NULL;
}

int aoe_comm_queue_grow_ror(AoeCommQueueRor *q, uint32_t n) {
    AoeCommQueueEntryRor *fresh;
    uint32_t i;
    if (n < q->capacity) return 0;
    fresh = calloc((size_t)n + 1, sizeof *fresh);
    if (!fresh) return 0;
    if (q->entries) {
        for (i = 0; i <= q->capacity; ++i) {
            AoeCommQueueEntryRor *src = &q->entries[i];
            if (!src->size) continue;
            fresh[i] = *src;
            fresh[i].data = copy_data(src->data, src->size);
        }
        aoe_comm_queue_release_ror(q);
    }
    q->capacity = n;
    q->entries = fresh;
    return 1;
}

int32_t aoe_comm_queue_flush_ror(AoeCommQueueRor *q, uint32_t lo, uint32_t hi) {
    int32_t n = 0;
    uint32_t i;
    for (i = 0; i <= q->high; ++i) {
        AoeCommQueueEntryRor *e = &q->entries[i];
        if ((lo <= e->turn && e->turn <= hi) || (lo == 0 && hi == 0)) {
            ++n;
            if (e->size) aoe_comm_queue_entry_clear_ror(e);
        }
    }
    return n;
}

void aoe_comm_queue_flush_all_ror(AoeCommQueueRor *q) { aoe_comm_queue_flush_ror(q, 0, 0); }

void aoe_comm_queue_flush_player_ror(AoeCommQueueRor *q, int32_t player) {
    uint32_t i;
    for (i = 0; i <= q->high; ++i) {
        AoeCommQueueEntryRor *e = &q->entries[i];
        if (e->player == player && e->size) aoe_comm_queue_entry_clear_ror(e);
    }
}

int aoe_comm_queue_add_ror(AoeCommQueueRor *q, uint32_t turn, const void *data, uint32_t size,
                           int32_t player, uint8_t seq, int32_t extra) {
    uint32_t i = 0;
    AoeCommQueueEntryRor *e;
    while (q->entries[i].size != 0 && i < q->capacity) ++i;
    if (q->capacity <= i) {
        if (!aoe_comm_queue_grow_ror(q, q->capacity + 0x14)) return 0;
        ++i;
    }
    if (q->high < i) q->high = i;
    e = &q->entries[i];
    e->data = copy_data(data, size);
    e->size = size;
    e->seq = seq;
    e->turn = turn;
    e->player = player;
    e->key = (uint32_t)seq + (uint32_t)player * 1000u;
    e->extra = extra;
    q->current = i;
    return 1;
}

uint8_t *aoe_comm_queue_take_first_ror(AoeCommQueueRor *q) {
    uint32_t i;
    for (i = 0; i <= q->high; ++i) {
        AoeCommQueueEntryRor *e = &q->entries[i];
        if (e->size) {
            uint8_t *data = e->data;
            e->data = NULL;
            aoe_comm_queue_entry_clear_ror(e);
            return data;
        }
    }
    return NULL;
}

uint8_t *aoe_comm_queue_take_turn_ror(AoeCommQueueRor *q, uint32_t turn, uint32_t *late) {
    uint32_t i, best_key = 0xffffffffu;
    int32_t best = -1;
    uint8_t *data;
    if (late) *late = 0;
    for (i = 0; i <= q->high; ++i) {
        AoeCommQueueEntryRor *e = &q->entries[i];
        if (!e->key || !e->size) continue;
        if (e->turn < turn && late) ++*late;
        if (e->key < best_key && e->turn == turn) {
            best_key = e->key;
            best = (int32_t)i;
        }
    }
    if (best < 0) return NULL;
    data = q->entries[best].data;
    q->entries[best].data = NULL;
    aoe_comm_queue_entry_clear_ror(&q->entries[best]);
    return data;
}

uint8_t aoe_comm_queue_next_seq_ror(AoeCommQueueRor *q, uint32_t turn) {
    if (turn != q->seq_turn) {
        q->seq_turn = turn;
        q->seq = 1;
        return 1;
    }
    ++q->seq;
    return q->seq != 0xff ? q->seq : 0;
}
