#ifndef VITA_AOE_COMM_QUEUE_H
#define VITA_AOE_COMM_QUEUE_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a command queue (0x00427ce0..0x00428350; the
   comm handler's +0x14c4 / +0x14c8): commands waiting for their turn,
   each with its sequence byte, sender and data. Slots 0..capacity; a
   full queue grows by 20. Semantic storage; offsets noted. */

typedef struct {
    uint32_t key;      /* +0x00: sequence + player x 1000 */
    uint32_t turn;     /* +0x04: the turn it runs */
    uint8_t seq;       /* +0x08 */
    uint8_t *data;     /* +0x0c */
    int32_t player;    /* +0x10 */
    uint32_t size;     /* +0x14: 0 for a free slot */
    int32_t extra;     /* +0x18 */
} AoeCommQueueEntryRor;

typedef struct {
    uint32_t current;               /* +0x00: the slot last filled */
    uint32_t high;                  /* +0x04: the highest slot used */
    uint32_t capacity;              /* +0x08 */
    uint32_t seq_turn;              /* +0x10c */
    uint8_t seq;                    /* +0x110 */
    AoeCommQueueEntryRor *entries;  /* +0x114: capacity + 1 slots */
} AoeCommQueueRor;

/* 0x00427ce0: empty with room for `capacity` (at least 20); 0x00427d20
   frees it. 0x00427d50: grow to `n` (only when not below the capacity;
   the commands are copied); 0 when it does not. */
int aoe_comm_queue_init_ror(AoeCommQueueRor *q, int32_t capacity);
void aoe_comm_queue_release_ror(AoeCommQueueRor *q);
int aoe_comm_queue_grow_ror(AoeCommQueueRor *q, uint32_t n);
/* 0x00428350: one slot emptied (its data freed). 0x00427f40: the
   commands for turns lo..hi (both 0: all) dropped; returns how many
   slots matched (empty ones too); 0x00427f30 all of them. 0x00427fd0:
   a player's commands dropped. */
void aoe_comm_queue_entry_clear_ror(AoeCommQueueEntryRor *e);
int32_t aoe_comm_queue_flush_ror(AoeCommQueueRor *q, uint32_t lo, uint32_t hi);
void aoe_comm_queue_flush_all_ror(AoeCommQueueRor *q);
void aoe_comm_queue_flush_player_ror(AoeCommQueueRor *q, int32_t player);
/* 0x00428090: queue a copy of `data` for `turn` in the first free slot
   (growing by 20 when full). */
int aoe_comm_queue_add_ror(AoeCommQueueRor *q, uint32_t turn, const void *data, uint32_t size,
                           int32_t player, uint8_t seq, int32_t extra);
/* 0x004281b0: the first queued command's data (the caller owns it), its
   slot emptied; NULL when none. 0x00428220: the lowest-keyed command for
   `turn` (one due earlier is late; the original logs it). */
uint8_t *aoe_comm_queue_take_first_ror(AoeCommQueueRor *q);
uint8_t *aoe_comm_queue_take_turn_ror(AoeCommQueueRor *q, uint32_t turn, uint32_t *late);
/* 0x004282f0: the next sequence byte for `turn` (1 for a new turn; 0
   once 255 would be reached). */
uint8_t aoe_comm_queue_next_seq_ror(AoeCommQueueRor *q, uint32_t turn);

#endif
