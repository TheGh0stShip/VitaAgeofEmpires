#ifndef VITA_AOE_COMMAND_GAME_H
#define VITA_AOE_COMMAND_GAME_H

#include <stddef.h>
#include <stdint.h>

#include "player_diplomacy.h"
#include "player_resources.h"

/* Patched Rise of Rome 1.0a tribe command 0x67 (0x004e8eb0, jump table
   0x004e9104): sub-code +0x01, int16 player +0x02, int16/byte +0x04,
   dword/float +0x08, int16 +0x0c. */
enum {
    AOE_COMMAND_GAME_ROR = 0x67,
    AOE_COMMAND_GAME_SIZE_ROR = 0x10
};

typedef struct {
    void *context;
    AoePlayerDiplomacyRor *(*diplomacy)(void *context, int16_t player);
    AoePlayerDiplomacyOps diplomacy_ops;
    const AoePlayerResourceTargetRor *(*resources)(void *context,
                                                   int16_t player);
    void (*refresh_world)(void *context);               /* 0x00520c60 */
    /* Global +0x40(event, a, b, c, 0). */
    void (*game_event)(void *context, uint32_t event, int32_t a, int32_t b,
                       int32_t c);
    int32_t *world_18;                                  /* 0x004ccc80 */
    int32_t *game_9a8;
    void (*cheat)(void *context, int16_t player, uint8_t code); /* 0x0050c570 */
    void (*world_4c_notify)(void *context, int16_t player); /* +8(0x71, p) */
    void (*sync_error)(void *context); /* "syncerr%d.gam" save + screen */
} AoeCommandGameOps;

size_t aoe_command_encode_game_ror(uint8_t *out, size_t out_size,
                                   uint8_t code, int16_t player,
                                   int16_t value_4, uint32_t value_8,
                                   int16_t value_c);

/* Sub-codes:
   0 diplomacy: player +0x88 = 1; 0x0045da70(int16 +0x04,
     __ftol(float +0x08)); event 8(+0x02, +0x04, +0x0c); world refresh.
   1 world +0x18 = dword +0x08.   2 player +0x78(word +0x04, float +0x08).
   4 game +0x9a8 = +0x02.         5 player +0x108 = byte +0x04.
   6 cheat(+0x02, byte +0x04).    7 world +0x4c object +8(0x71, player).
   8 sync error.                  3 and above 8: nothing.
   Returns -1 for a malformed record, missing dependencies or an
   unresolved player; otherwise 1. */
int aoe_command_execute_game_ror(const uint8_t *command, size_t size,
                                 const AoeCommandGameOps *ops);

#endif
