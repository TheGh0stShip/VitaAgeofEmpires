#ifndef VITA_AOE_PLAYER_SCENARIO_H
#define VITA_AOE_PLAYER_SCENARIO_H

#include <stdint.h>

#include "aoe_stream.h"
#include "player_object.h"

/* Patched Rise of Rome 1.0a scenario player data (base player vtable
   +0x68 / +0x6c / +0x70 / +0x8c): the name, view, allied-victory byte,
   stances, relation codes and the victory-condition object. */

typedef struct {
    void *context;
    /* Global game +0x1b0 (game type; 7 never locks diplomacy). */
    int32_t (*game_type)(void *context);
    /* The player of session slot 1..8, the slot's kind (2 and 4 lock) and
       the team value queried for slot - 1 (above 1 locks). */
    int32_t (*slot_player)(void *context, int32_t slot);
    int32_t (*slot_kind)(void *context, int32_t slot);
    int32_t (*slot_team)(void *context, int32_t slot_minus_one);
    /* Condition object save (its vtable +0x0c). */
    void (*conditions_save)(void *context, void *conditions,
                            AoeOutcomeStream *out);
    /* Player vtable +0xf0(stream, player map, flag); NULL uses the base
       0x0045e9e0 (aoe_player_conditions_create_ror). */
    void (*load_conditions)(void *context, AoePlayerObjectRor *player,
                            AoeOutcomeStream *in, const int32_t *player_map,
                            int32_t flag);
    const AoePlayerObjectOps *object; /* condition create / release */
} AoePlayerScenarioOps;

/* 0x0045c5d0 (+0x6c): name (int16 length including the NUL, then the
   bytes), view x/y, view tile x/y, +0x108, the world player count
   (int16), that many stance bytes, the nine relation codes, then the
   condition object. Port: a missing name is written with length 0.
   Returns 0 when the stream fills up. */
int aoe_player_scenario_save_ror(const AoePlayerObjectRor *player,
                                 AoeOutcomeStream *out,
                                 const AoePlayerScenarioOps *ops);

/* 0x0045d300 (+0x8c): only the condition object. */
void aoe_player_conditions_save_ror(const AoePlayerObjectRor *player,
                                    AoeOutcomeStream *out,
                                    const AoePlayerScenarioOps *ops);

/* 0x0045c6a0 (+0x70): read a block written by the save above (or older).
   A name is adopted only when the player has none (otherwise the read
   copy is dropped). Diplomacy is locked when the game type is not 7 and
   the first session slot of this player whose kind is 2 or 4 has a team
   value above 1; locked reads are consumed but ignored. +0x108 is read for
   versions above 1.0; +0x104 = 0; versions up to 1.06 (or NaN) carry 29
   resource floats (read only when the player has resources); each stance
   byte i goes to player_map[i] (or i without a map) when it is a valid
   player, with relation code 0 for player 0, 1 for the player itself, and
   2/3/4 for stance 0/1/other; versions 1.08 and later carry the nine
   relation codes, stored at player_map[i] (or i for a missing/negative
   entry); the old condition object is released and a new one loaded with
   flag 1 for versions 1.09 and later. Ports: resource floats beyond the
   player's count, relation slots beyond nine and codes for stance entries
   beyond nine are dropped (the original overruns). */
int aoe_player_scenario_load_ror(AoePlayerObjectRor *player,
                                 AoeOutcomeStream *in,
                                 const int32_t *player_map, float version,
                                 const AoePlayerScenarioOps *ops);

/* 0x0045c340 (+0x68): the older reader, which does nothing for versions
   above 1.05. As above, except: the resource floats are always present
   (when the player has resources), the stance entries are always mapped
   through player_map, relation code 1 is chosen when the entry's
   position (not its mapped player) is this player's ID, the relation
   codes follow only above 1.08 (so never), and the new condition object
   gets flag 0. Port: a NULL player_map maps entries to themselves. */
int aoe_player_scenario_load_old_ror(AoePlayerObjectRor *player,
                                     AoeOutcomeStream *in,
                                     const int32_t *player_map,
                                     float version,
                                     const AoePlayerScenarioOps *ops);

#endif
