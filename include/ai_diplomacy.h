#ifndef VITA_AOE_AI_DIPLOMACY_H
#define VITA_AOE_AI_DIPLOMACY_H

#include <stdint.h>

#include "ai_module.h"
#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a computer-player Diplomacy AI (module 0x3eb,
   vtable 0x00542af4), Emotional AI (0x3ec, vtable 0x00542b34) and InfAI
   (0x3ed, vtable 0x00542b74). Semantic storage, not the x86 layout. */

#define AOE_AI_DIPLOMACY_SLOTS 10

typedef struct {
    AoeAiModuleRor module;
    void *owner;                                  /* +0xf0 */
    int32_t dislike[AOE_AI_DIPLOMACY_SLOTS];      /* +0xf4, 49 */
    int32_t like[AOE_AI_DIPLOMACY_SLOTS];         /* +0x11c, 49 */
    uint8_t changeable[AOE_AI_DIPLOMACY_SLOTS];   /* +0x144, 1 */
} AoeAiDiplomacyRor;

/* The owning player (owner +0xf0): its index (+0x4a), the world player
   count (+0x3c +0x3c) and stance bytes (+0x84); the other players'
   state (+0x80, 2 = out of the game), their opinion of us (vtable +0x1c
   with our index), their outcome-manager points (+0x34, then +0x1c via
   0x00454170), and the stance request (our vtable +0x28). */
typedef struct {
    void *context;
    int16_t self;
    int16_t player_count;
    const uint8_t *stances;
    uint8_t (*state)(void *context, int32_t player);
    int (*considers_ally)(void *context, int32_t player);
    int32_t (*points)(void *context, int32_t player);
    void (*request)(void *context, int32_t player, int32_t stance);
} AoeAiDiplomacyOps;

/* 0x0040a780 new (dislike and like 49, all changeable) and 0x0040a800
   load (per slot i32 dislike, i32 like, u8 changeable); save 0x0040a950;
   destructor 0x0040a8b0 (deleting 0x0040a7e0). */
void aoe_ai_diplomacy_init_ror(AoeAiDiplomacyRor *ai, int32_t player,
                               AoeAiLogRor *log);
int aoe_ai_diplomacy_load_ror(AoeAiDiplomacyRor *ai, int32_t player,
                              AoeAiLogRor *log, AoeOutcomeStream *in);
int aoe_ai_diplomacy_save_ror(const AoeAiDiplomacyRor *ai,
                              AoeOutcomeStream *out);
void aoe_ai_diplomacy_release_ror(AoeAiDiplomacyRor *ai);

/* Values: 0 dislike, 1 the derived "neutrality" 100 - |49 - like| -
   |49 - dislike| (not below 0), 2 like. Players 1..count-1. */
enum { AOE_AI_DISLIKE = 0, AOE_AI_NEUTRALITY = 1, AOE_AI_LIKE = 2 };
/* 0x0040a9a0: -1 for another player or kind. */
int32_t aoe_ai_diplomacy_get_ror(const AoeAiDiplomacyRor *ai,
                                 const AoeAiDiplomacyOps *ops,
                                 int32_t player, int kind);
/* 0x0040aa20 (add) and 0x0040ab40 (set) for dislike or like. A player
   that cannot change keeps dislike in 50..100 and like in 0..48; others
   0..100. */
void aoe_ai_diplomacy_add_ror(AoeAiDiplomacyRor *ai,
                              const AoeAiDiplomacyOps *ops, int32_t player,
                              int kind, int32_t delta);
void aoe_ai_diplomacy_set_ror(AoeAiDiplomacyRor *ai,
                              const AoeAiDiplomacyOps *ops, int32_t player,
                              int kind, int32_t value);
/* 0x0040ac40 / 0x0040ac80. */
int aoe_ai_diplomacy_changeable_ror(const AoeAiDiplomacyRor *ai,
                                    const AoeAiDiplomacyOps *ops,
                                    int32_t player);
void aoe_ai_diplomacy_set_changeable_ror(AoeAiDiplomacyRor *ai,
                                         const AoeAiDiplomacyOps *ops,
                                         int32_t player, uint8_t value);
/* Stance tests: 0x0040ae60 enemy (3), 0x0040aea0 neutral (1), both for
   another player 1..count (inclusive, as in the original); 0x0040aee0
   ally (ourselves, or stance 0); 0x0040af30 the other player names us
   an ally (1..count-1). */
int aoe_ai_diplomacy_is_enemy_ror(const AoeAiDiplomacyOps *ops,
                                  int32_t player);
int aoe_ai_diplomacy_is_neutral_ror(const AoeAiDiplomacyOps *ops,
                                    int32_t player);
int aoe_ai_diplomacy_is_ally_ror(const AoeAiDiplomacyOps *ops,
                                 int32_t player);
int aoe_ai_diplomacy_allied_by_ror(const AoeAiDiplomacyOps *ops,
                                   int32_t player);
/* 0x0040a8c0: for every other player, an ally that does not return the
   favour gains 1 dislike, and a non-ally that names us an ally gains 1
   like. Returns 1. */
int aoe_ai_diplomacy_update_ror(AoeAiDiplomacyRor *ai,
                                const AoeAiDiplomacyOps *ops);
/* 0x0040acb0: the most disliked other player still in the game (state
   not 2), skipping `spare` when `use_spare` > 0 and it is an ally; with
   `divisor` > 1 its points / divisor are added (mode 1) or subtracted.
   0x0040ada0: the most liked, skipping `spare` when it is an enemy. -1
   when none. */
int32_t aoe_ai_diplomacy_most_disliked_ror(const AoeAiDiplomacyRor *ai,
                                           const AoeAiDiplomacyOps *ops,
                                           int32_t use_spare, int32_t spare,
                                           int32_t mode, int32_t divisor);
int32_t aoe_ai_diplomacy_most_liked_ror(const AoeAiDiplomacyRor *ai,
                                        const AoeAiDiplomacyOps *ops,
                                        int32_t use_spare, int32_t spare);
/* 0x0040af80: ask for a stance toward a changeable player. */
void aoe_ai_diplomacy_request_ror(const AoeAiDiplomacyRor *ai,
                                  const AoeAiDiplomacyOps *ops,
                                  int32_t player, int32_t stance);

/* Emotional AI traits, in their storage order (+0xf4..+0x108). */
enum {
    AOE_AI_AGGRESSIVE, AOE_AI_COMPASSIONATE, AOE_AI_DEFENSIVE,
    AOE_AI_FRIENDLY, AOE_AI_PASSIVE, AOE_AI_VENGEFUL, AOE_AI_TRAITS
};

typedef struct {
    AoeAiModuleRor module;
    void *owner;                       /* +0xf0 */
    int32_t traits[AOE_AI_TRAITS];     /* +0xf4, 49 */
    char names[AOE_AI_TRAITS][30];     /* +0x10c, 0x1e apart */
} AoeAiEmotionalRor;

/* 0x0040afc0 new (traits 49) and 0x0040b120 load (six i32); save
   0x0040b2a0; destructor 0x0040b290 (deleting 0x0040b100). */
void aoe_ai_emotional_init_ror(AoeAiEmotionalRor *ai, int32_t player,
                               AoeAiLogRor *log);
int aoe_ai_emotional_load_ror(AoeAiEmotionalRor *ai, int32_t player,
                              AoeAiLogRor *log, AoeOutcomeStream *in);
int aoe_ai_emotional_save_ror(const AoeAiEmotionalRor *ai,
                              AoeOutcomeStream *out);
void aoe_ai_emotional_release_ror(AoeAiEmotionalRor *ai);
/* 0x0040b2d0: personality presets 0..5 (others leave the traits). */
void aoe_ai_emotional_preset_ror(AoeAiEmotionalRor *ai, int32_t preset);

/* InfAI: 0x0040b470 constructor, 0x0040b4c0 destructor (deleting
   0x0040b4a0); only the module base. */
void aoe_ai_inf_init_ror(AoeAiModuleRor *ai, int32_t player,
                         AoeAiLogRor *log);
void aoe_ai_inf_release_ror(AoeAiModuleRor *ai);

#endif
