#ifndef VITA_AOE_SHARED_VISION_H
#define VITA_AOE_SHARED_VISION_H

#include <stdint.h>

enum { AOE_SHARED_VISION_PLAYERS = 9 };

/* Per-player masks written by the patched RoR 1.0a world method
   0x00520c60 (players +0xb0 .. +0x100):
   - explored  +0xd4: explored-mask bits (bit 16 + player) seen as own;
   - visible   +0xd8: visible-mask bits (bit player) seen as own;
   - indirect  +0xdc: players linked through an intermediary;
   - via[k]    +0xe0 + 4k: the intermediaries linking player k;
   - shares[j] +0xb0 + 4j: 1 for self and for each mutual ally whose
     vision this player receives. */
typedef struct {
    uint32_t explored;
    uint32_t visible;
    uint32_t indirect;
    uint32_t via[AOE_SHARED_VISION_PLAYERS];
    uint32_t shares[AOE_SHARED_VISION_PLAYERS];
} AoePlayerSharedVision;

/* 0x00520c60 over `count` players (Gaia first):
   1. every player: masks cleared, then own bits and shares[self] = 1;
   2. for players 1.. with the shared-exploration flag (+0x104 == 1, set
      by resource 50): each other player j >= 1 with stance 0 both ways
      (+0x84 byte arrays) is shared: shares[j] = 1, visible |= 1 << j,
      explored |= 1 << (j + 16);
   3. for every player i and k >= 1, k != i, with shares[k] == 0: each
      m >= 1 with i's shares[m] == 0, m's shares[k] == 1 and m's
      shares[i] == 1 sets indirect |= 1 << k and via[k] |= 1 << m.
   `stance[a][b]` is player a's +0x84 byte for player b. */
void aoe_world_shared_vision_ror(int count, const int32_t shares_flag[],
                                 const uint8_t stance[][AOE_SHARED_VISION_PLAYERS],
                                 AoePlayerSharedVision out[]);

#endif
