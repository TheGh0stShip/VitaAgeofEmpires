#ifndef VITA_AOE_PLAYER_WATCH_H
#define VITA_AOE_PLAYER_WATCH_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a player +0x40 object (0x00436850..0x00436b30):
   the player's own objects whose visibility masks it watches. Four
   parallel arrays of `capacity` entries: the object, the address of its
   mask, the mask last seen and the object's ID. Semantic storage; the
   object is an opaque token compared against the world's lookup. */
typedef struct {
    int16_t player;            /* +0x00: the owner's index (+0x4a) */
    int32_t capacity;          /* +0x04 */
    int32_t count;             /* +0x08 */
    const void **objects;      /* +0x0c */
    const uint32_t **masks;    /* +0x10 */
    uint32_t *last;            /* +0x14 */
    int32_t *ids;              /* +0x18 */
} AoePlayerWatchRor;

/* 0x00436850(player, capacity): four zeroed arrays of `capacity`; count
   0. 0x004368a0 frees them. 0 when an allocation fails (host-only: the
   original stores NULL and carries on). */
int aoe_player_watch_init_ror(AoePlayerWatchRor *w, int16_t player, int32_t capacity);
void aoe_player_watch_release_ror(AoePlayerWatchRor *w);
/* 0x00436b30: `extra` more entries (new arrays, the entries copied, the
   old ones freed). */
int aoe_player_watch_grow_ror(AoePlayerWatchRor *w, int32_t extra);
/* 0x004368f0(object, mask): a full list first grows by 0x20; an object of
   this player not yet listed is appended with its mask, that mask's
   value and its ID (1); else 0. */
int aoe_player_watch_add_ror(AoePlayerWatchRor *w, const void *object, int16_t owner,
                             int32_t id, const uint32_t *mask);
/* 0x00436980(object): the entry is replaced by the last one and the
   freed last slot zeroed; 1 when found. */
int aoe_player_watch_remove_ror(AoePlayerWatchRor *w, const void *object);
/* 0x00436a20: each entry whose mask changed: an object still in the world
   (the lookup of its ID gives it back) reports, for each other player
   whose bit (1..15, from bit 1) was set and is now clear, that it lost
   sight of the object (`lost`: that player's map +0x3cc vtable +0x38 with
   the object's position and the object); the mask is then remembered.
   A missing or replaced object leaves the list, its slot looked at
   again. */
typedef struct {
    void *context;
    const void *(*lookup)(void *context, int32_t id);   /* 0x005206d0 */
    void (*lost)(void *context, int32_t player, const void *object);
} AoePlayerWatchOpsRor;
void aoe_player_watch_update_ror(AoePlayerWatchRor *w, const AoePlayerWatchOpsRor *ops);

#endif
