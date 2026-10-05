#ifndef VITA_AOE_PLAYER_VICTORY_H
#define VITA_AOE_PLAYER_VICTORY_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a player outcome state and the per-player
   victory test. Player state +0x80 is 0 while playing, 1 after a win and 2
   after a loss. Semantic caller-owned storage; original offsets noted. */
typedef struct {
    int16_t player_id;       /* +0x4a */
    uint8_t state;           /* +0x80 */
    uint8_t resigned;        /* +0x81 */
    uint8_t allied_victory;  /* +0x108 */
    const uint8_t *stances;  /* +0x84, one byte per player (0 = ally) */
    /* The player's victory-condition object (+0x34) byte +0x10; 2 means
       the conditions are met. */
    uint8_t condition_state;
} AoePlayerVictoryRor;

/* The world's player table (+0x40) and count (+0x3c), and the game-over
   byte +0x1e. Every listed player's stance table holds `player_count`
   bytes. */
typedef struct {
    AoePlayerVictoryRor *const *players;
    int16_t player_count;
    uint8_t game_over;
} AoePlayerVictoryWorldRor;

typedef struct {
    void *context;
    /* Global game vtable +0x40(event, player, 0, 0, 0). */
    void (*game_event)(void *context, int32_t event, int32_t player);
    /* The player of a session slot 0..8. */
    int32_t (*slot_player)(void *context, int32_t slot);
    uint8_t *slot_left;      /* game +0xa15, nine bytes */
    int (*session)(void *context);
    int32_t (*local_slot)(void *context);
    /* The local slot's player lost: the session's two "lost" calls, then
       the game's +0x2c and +0x30 screens (when present) receive +0x20(2). */
    void (*local_lost)(void *context);
} AoePlayerStateOps;

/* 0x0045bbd0 (player vtable +0x04 in all three player vtables): set the
   state to `state` when it differs, unless the player resigned and the new
   state is not 2. A non-resigned player entering state 2 raises game event
   6 and marks each session slot mapped to it as left; when in a session
   that slot is the local one, `local_lost` runs. */
void aoe_player_set_state_ror(AoePlayerVictoryRor *player, uint8_t state,
                              const AoePlayerStateOps *ops);

/* 0x0045d4f0: whether the player lost or its conditions are met. */
int aoe_player_lost_or_conditions_met_ror(const AoePlayerVictoryRor *player);

/* 0x0045d510: for a player that has not lost, whether every ally (stance
   0) among players 1..n-1 regards it as an ally too and has allied victory
   set. *count receives how many players were passed before the first
   failure (n-1 when none fails); a lost player leaves it untouched. When
   the incoming *count was not negative, a different outgoing count makes
   the result 0. Port: a missing player or stance table counts as a
   failure (the original dereferences it). */
int aoe_player_allies_agree_ror(const AoePlayerVictoryRor *player,
                                const AoePlayerVictoryWorldRor *world,
                                int32_t *count);

/* 0x0045d5b0: a player that has not lost and whose conditions are met
   wins (state 1 through 0x0045bbd0, world game-over byte 2) unless, with
   allied victory, the player and its allies do not all agree (each with
   the same count) or an ally has neither lost nor met its conditions.
   Returns the resulting state byte. */
uint8_t aoe_player_check_victory_ror(AoePlayerVictoryRor *player,
                                     AoePlayerVictoryWorldRor *world,
                                     const AoePlayerStateOps *ops);

#endif
