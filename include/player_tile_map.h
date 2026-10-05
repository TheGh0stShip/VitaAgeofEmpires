#ifndef VITA_AOE_PLAYER_TILE_MAP_H
#define VITA_AOE_PLAYER_TILE_MAP_H

#include <stdint.h>

#include "player_object_memory.h"
#include "player_outcome.h" /* AoeOutcomeStream */
#include "player_vision_state.h"

/* Patched Rise of Rome 1.0a player tile map (the player's +0x38: one
   byte per tile, 0xff unexplored, with the explored count), its save,
   the reveal-all and explored fraction; the save of the player's object
   memory (+0x114) and of the world's shared visibility masks.
   Semantic storage; offsets noted. */

typedef struct {
    uint8_t **rows;        /* +0x00 */
    void *player;          /* +0x08 */
    int32_t field_c;       /* +0x0c */
    uint8_t *data;         /* +0x10 */
    int32_t width, height; /* +0x14, +0x18 */
    int32_t explored;      /* +0x1c */
    int32_t total;         /* +0x20 */
    uint32_t mask;         /* +0x2c: this player's visibility bit */
} AoePlayerTileMapRor;

/* 0x005175b0: frees the data and rows, forgets the player. */
void aoe_player_tile_map_free_ror(AoePlayerTileMapRor *m);
/* 0x005175f0: i32 width, height, the explored count (from save version
   6.7), the player's i16 ID (+0x4a), then the tiles (when both sizes are
   positive). */
int aoe_player_tile_map_save_ror(const AoePlayerTileMapRor *m, int16_t player_id, float version,
                                 AoeOutcomeStream *s);
/* 0x00517810: explored / total. */
float aoe_player_tile_map_explored_ror(const AoePlayerTileMapRor *m);
/* 0x00517780: every unexplored tile becomes explored (0, counted), gets
   the player's bit in the shared masks and is reported (0x0045eac0(x,
   y)); then the player's +0x64 is set. */
void aoe_player_tile_map_reveal_ror(AoePlayerTileMapRor *m, AoeVisibilityMaskMap *masks,
                                    void (*explored)(void *context, int32_t x, int32_t y),
                                    void (*done)(void *context), void *context);

/* 0x00516fd0: i32 category count, each category's capacity and count,
   then each category's entries (8 bytes: i32 ID, two bytes, x, y).
   0x00516f70 is aoe_player_object_memory_free. */
int aoe_player_object_memory_save_ror(const AoePlayerObjectMemory *m, AoeOutcomeStream *s);

/* 0x00517f60: i32 width, height, then the masks (when both positive).
   0x00517fb0: the global row table 0x007d205c (256 entries) pointed at
   the rows, the rest NULL. */
int aoe_visibility_masks_save_ror(const AoeVisibilityMaskMap *m, AoeOutcomeStream *s);
void aoe_visibility_masks_bind_rows_ror(const AoeVisibilityMaskMap *m, uint32_t *rows[256]);

#endif
