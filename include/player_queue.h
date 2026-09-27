#ifndef VITA_AOE_PLAYER_QUEUE_H
#define VITA_AOE_PLAYER_QUEUE_H

#include <stdint.h>

typedef enum {
    AOE_PLAYER_QUEUE_BASE = 0,
    AOE_PLAYER_QUEUE_RISE_OF_ROME = 1
} AoePlayerQueueEdition;

typedef struct {
    uint32_t first;
    uint32_t second;
} AoePlayerQueueEntry;

/* Semantic form of the five 32-bit fields at player +0x58. The storage
   holds capacity eight-byte entries and belongs to this queue. */
typedef struct {
    uint32_t capacity_00;
    uint32_t field_04;
    uint32_t field_08;
    uint32_t dirty_0c;
    AoePlayerQueueEntry *storage_10;
} AoePlayerQueue;

/* RoR 0x0045fb20 / base 0x005196b0 initialize the queue. The base
   constructor requires at least 100 entries; RoR has no such minimum. The
   portable routine accepts a zeroed queue and returns zero on invalid input
   or allocation failure. */
int aoe_player_queue_init(AoePlayerQueue *queue, uint32_t capacity,
                          AoePlayerQueueEdition edition);

/* RoR 0x0045fa50 / base 0x005195d0 append two 32-bit values. Both double
   capacity when full and copy the entire old allocation. Base rejects
   growth when old capacity exceeds 65000. The portable version also rejects
   malformed state and arithmetic overflow. */
int aoe_player_queue_append(AoePlayerQueue *queue, uint32_t first,
                            uint32_t second, AoePlayerQueueEdition edition);

/* RoR 0x0045fad0 / base 0x00519670: clear the two counters; when dirty,
   replace storage with eight eight-byte entries and clear the dirty flag.
   Returns zero on invalid state or allocation failure. */
int aoe_player_queue_reset(AoePlayerQueue *queue);

/* RoR 0x0045fb70 / base 0x00519720 expose storage and the first count;
   RoR 0x0045fb90 / base 0x00519740 return then clear the second count. */
int aoe_player_queue_view(const AoePlayerQueue *queue,
                          const AoePlayerQueueEntry **entries,
                          uint32_t *used);
int aoe_player_queue_take_pending(AoePlayerQueue *queue, uint32_t *pending);

/* RoR 0x0045fb50 / base 0x00519700 free the owned storage. Unlike the
   original destructor, this also clears the portable queue fields. */
void aoe_player_queue_dispose(AoePlayerQueue *queue);

#endif
