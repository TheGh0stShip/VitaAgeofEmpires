#ifndef VITA_AOE_AI_PLAYER_H
#define VITA_AOE_AI_PLAYER_H

#include <stdint.h>

#include "ai_decision.h"
#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a computer player (vtable 0x00548bf0): the Main
   Decision AI with nine modules inside it and its own diplomacy and
   tribute state (0x004c6960..0x004c8630). The modules are reached
   through the ops below; this file keeps the composite's own fields and
   the order of its calls. Semantic storage; original offsets noted. */

/* The modules in the composite's order: Build AI +0x104, ConAI +0x6c4,
   Diplomacy AI +0x9dc, Emotional AI +0xb2c, information AI +0xcec,
   Resource AI +0x10d7c, Strategy AI +0x10e74, tactical AI +0x1146c and
   Trade AI +0x12478. */
enum {
    AOE_AI_MOD_BUILD, AOE_AI_MOD_CON, AOE_AI_MOD_DIPLOMACY,
    AOE_AI_MOD_EMOTIONAL, AOE_AI_MOD_INFO, AOE_AI_MOD_RESOURCE,
    AOE_AI_MOD_STRATEGY, AOE_AI_MOD_TACTICAL, AOE_AI_MOD_TRADE,
    AOE_AI_MODULES
};

typedef struct {
    AoeAiDecisionRor decision;  /* the Main Decision AI base */
    int32_t setup_pending;      /* +0x12570: 1 new, 0 loaded */
    int32_t field_12574;        /* +0x12574: 1 new, 0 loaded */
    int32_t update_count;       /* +0x12578 */
    int32_t diplomacy_time;     /* +0x1257c */
    int32_t field_12580;
    int32_t field_12584;
    int32_t tribute_delay;      /* +0x12588, rolled by 0x004c8630 */
    int32_t field_1258c, field_12590, field_12594, field_12598;
    int32_t field_1259c, field_125a0;
    int32_t field_125a4[9];
} AoeAiPlayerAiRor;

typedef struct {
    void *context;
    /* A module's constructor (the new form, or the load form reading
       `in`), its owner hookup (the Build AI, ConAI and information AI
       have their own setters, the others set +0xf0), its player name
       (0x0040c150), vtable +8 with a value, its save (vtable +0x3c; the
       information AI's own save) and its destructor. */
    void (*module_init)(void *context, int module, AoeOutcomeStream *in);
    void (*module_owner)(void *context, int module);
    void (*module_name)(void *context, int module, int32_t player,
                        const char *name);
    void (*module_set)(void *context, int module, int32_t value);
    int (*module_save)(void *context, int module, AoeOutcomeStream *out);
    void (*module_release)(void *context, int module);
    /* The information AI's per-kind arrays (0x004c4930). */
    void (*info_kind)(void *context, int32_t kind, int32_t value);
    /* The Diplomacy AI personality preset (0x0040b2d0). */
    void (*personality)(void *context, int32_t preset);
    /* The base's vtable +8 step after the modules'. */
    void (*base_set)(void *context, int32_t value);
    /* Object bookkeeping: the tactical AI's add / remove, the Build AI's
       remove and the information AI's own-object add / remove. */
    void (*tactical_added)(void *context, int32_t id);
    void (*tactical_removed)(void *context, int32_t id);
    void (*build_removed)(void *context, int32_t id);
    void (*info_added)(void *context, int32_t id);
    void (*info_removed)(void *context, int32_t id);
    /* Stopping a unit through the tactical AI. */
    void (*detask)(void *context, int32_t id);
    /* The listed world object's class (type +0x14); 0 for none. */
    int (*object_class)(void *context, int32_t id, int16_t *type_class);
    /* Kick actions: 1 runs a Build AI action, 2 inserts build item 0x53
       (with 1, -1), 3 prints the build list (Build
       AI vtable +0x40). */
    void (*kick)(void *context, int32_t kind);
    int32_t (*sn)(void *context, int32_t index);
    int32_t (*rand)(void *context); /* 0x0052605d */
    int logging_on;                 /* global 0x00580dd0 */
    /* Our object (0x0040bab0): its type ID and position; 0 for none. */
    int (*object_at)(void *context, int32_t id, int16_t *type_id,
                     float pos[3]);
    /* The Build AI's object event (0x004b8a70 with 0) and the ConAI's
       placement at a point with the map's size (0x0040a290). */
    void (*build_object)(void *context, int32_t id);
    void (*con_place)(void *context, float x, float y, float z);
    /* The Diplomacy AI (+0x9dc): add (0x0040aa20) or set (0x0040ab40)
       dislike (kind 0) or like (kind 2), its changeable flag (0x0040ac80)
       and a stance request (0x0040af80). */
    void (*dip_add)(void *context, int32_t player, int kind, int32_t delta);
    void (*dip_set)(void *context, int32_t player, int kind, int32_t value);
    void (*dip_changeable)(void *context, int32_t player, int flag);
    void (*dip_request)(void *context, int32_t player, int32_t stance);
    /* Player `from`'s vtable +`slot` (0x18 enemy, 0x1c ally, 0x20 the
       third test) toward `to`. */
    int (*stance_test)(void *context, int32_t from, int slot, int32_t to);
    /* Our vtable +0x54 (our index, 6, target, stance, 0): the diplomacy
       command. */
    void (*diplomacy_command)(void *context, int32_t target, int32_t stance);
    /* String `id` (the game's string table, 256 bytes) sent as chat to a
       player (0x0045c1d0 with 0). */
    void (*chat)(void *context, int32_t to, int32_t string_id);
    /* The tribe player's (+0x1256c) vtable +0x28 (player, value) and
       +0xf8 (tribute: to, resource, amount, the fee in resource 46); its
       object list's nearest type 0x54 (0x00451370: -1, -1, 0, 2, 0). */
    void (*tribe_28)(void *context, int32_t player, int32_t value);
    void (*tribute)(void *context, int32_t to, int32_t resource, float amount);
    int (*has_market)(void *context);
    /* The information AI's map scan (0x004c6170), the tactical AI's
       tribute handler (0x004e2ea0) and number setter (0x004d77f0). */
    void (*info_scan)(void *context);
    void (*tactical_tribute)(void *context, int32_t player, int32_t a,
                             int32_t b, int32_t c);
    void (*set_sn)(void *context, int32_t index, int32_t value);
    /* The settings' game kind (0x0041b990), a player's +4 (0x00454150)
       and the world command sub 5 (0x004e9840) with our index. */
    int32_t (*game_kind)(void *context);
    int32_t (*player_field_4)(void *context, int32_t player);
    void (*command_sub5)(void *context, int32_t flag);
    int16_t self, player_count;
} AoeAiPlayerOpsRor;

/* 0x004c6960: the Main Decision AI, the nine modules (each constructed,
   given this owner and the player name), setup flags 1, the update count
   starting at `stagger`, the other fields 0, logging per the global, the
   information AI's kind lists (kind 0: 0x30, 0x7e, 0x3b, 0x41, 0x34,
   0x35, 0x104, 0x107, 0x32; kind 1: 0x6f..0x72, 0x86..0x99, 0xc0..0xc5;
   kind 2: 0x66; kind 3: 0x42, 0x150..0x153) and personality 0. */
void aoe_ai_player_init_ror(AoeAiPlayerAiRor *ai, int32_t player,
                            int32_t stagger, const char *name,
                            AoeAiLogRor *log, const AoeAiPlayerOpsRor *ops);
/* 0x004c6ce0 load constructor: the modules load in order, setup flags
   0; then i32 +0x12578, +0x1257c, +0x12580, from save version 1.2
   +0x12584..+0x12598, and from 7.08 +0x1259c, +0x125a0 and the nine
   +0x125a4 values (zero before). */
int aoe_ai_player_load_ror(AoeAiPlayerAiRor *ai, int32_t player,
                           const char *name, float version,
                           AoeOutcomeStream *in, AoeAiLogRor *log,
                           const AoeAiPlayerOpsRor *ops);
/* 0x004c7e90 save (vtable +0x3c): the base's IDs, each module, then the
   own fields +0x12578..+0x125a0 and the nine values. */
int aoe_ai_player_save_ror(AoeAiPlayerAiRor *ai, AoeOutcomeStream *out,
                           const AoeAiPlayerOpsRor *ops);
/* 0x004c7010 (deleting 0x004c6cc0): the modules released from the Trade
   AI back to the Build AI, then the base. */
void aoe_ai_player_release_ror(AoeAiPlayerAiRor *ai,
                               const AoeAiPlayerOpsRor *ops);
/* 0x004c70f0 (vtable +8): every module's vtable +8, then the base's. */
void aoe_ai_player_set_ror(AoeAiPlayerAiRor *ai, int32_t value,
                           const AoeAiPlayerOpsRor *ops);
/* 0x004c8060 (vtable +0x40): a new object for the tactical and
   information AIs, then the base list (0x0040b880); 0 for no object.
   0x004c80a0 (vtable +0x44): a lost object leaves the tactical, Build and
   information AIs and the base list (0x0040b950). */
int aoe_ai_player_object_added_ror(AoeAiPlayerAiRor *ai, int32_t id,
                                   int has_object,
                                   const AoeAiPlayerOpsRor *ops);
int aoe_ai_player_object_removed_ror(AoeAiPlayerAiRor *ai, int32_t id,
                                     const AoeAiPlayerOpsRor *ops);
/* 0x004c80e0 (vtable +0x48): the class an action is for: 0x25a, 0x25d,
   0x261, 0x265 give 4, 0x26b and 0x26c give 3, others the base's
   unrecognized-action error (-1). 0x004c8140 (vtable +0x4c): the listed
   object's class matches that for the action. */
int32_t aoe_ai_player_action_class_ror(const AoeAiPlayerAiRor *ai,
                                       int32_t action);
int aoe_ai_player_object_fits_ror(const AoeAiPlayerAiRor *ai, int32_t id,
                                  int32_t action,
                                  const AoeAiPlayerOpsRor *ops);
/* 0x004c8180: stop a unit (the tactical AI's detask). */
void aoe_ai_player_detask_ror(const AoeAiPlayerOpsRor *ops, int32_t id);
/* 0x004c81a0: a listed object that is not a building, wall, resource or
   other passive class (3, 5, 7, 8, 0xe, 0xf, 0x10, 0x1b, 0x1e, 0x1f,
   0x20, 0x21). */
int aoe_ai_player_active_unit_ror(const AoeAiPlayerOpsRor *ops, int32_t id);
/* 0x004c8630: the tribute delay: number 125 moved by rand() % n - n / 2
   for a positive number 126 (n), else number 125. */
void aoe_ai_player_roll_delay_ror(AoeAiPlayerAiRor *ai,
                                  const AoeAiPlayerOpsRor *ops);
/* 0x004c7ff0: a debugging kick: logs "Got a kick=%d.", then kind 1 and
   2 run their Build AI action and kind 3 logs "Current Build list is:"
   and prints it. */
void aoe_ai_player_kick_ror(AoeAiPlayerAiRor *ai, int32_t kind,
                            const AoeAiPlayerOpsRor *ops);
/* 0x004c8210: each listed object (the list grown as 0x004cc5c0 would when
   its capacity is short; the port zero-fills the new slot, the original
   leaves it unset) that exists goes to the Build AI; a town centre
   (type 0x6d) also places the ConAI plan at its position. */
void aoe_ai_player_refresh_objects_ror(AoeAiPlayerAiRor *ai,
                                       const AoeAiPlayerOpsRor *ops);
/* 0x004c8300: tribute of `amount` in resource `kind` from `player`.
   While +0x1258c is set, gold (3) from number 128's player adds to
   +0x12598. From number 128's player nothing else happens. Otherwise
   like rises and dislike falls by amount / 100 and +0x125a4[player] adds
   the amount. When our vtable +0x20 test passes, number 217 is 1 and
   that sum reaches +0x125a0, we ally with the player (command 6, stance
   2), send it string 2311, ally with each other player it names ally
   (or turn neutral, stance 0, toward its enemies) and clear number 217.
   Else an enemy of ours gets string 2310. */
void aoe_ai_player_tribute_received_ror(AoeAiPlayerAiRor *ai, int32_t player,
                                        int32_t kind, int32_t amount,
                                        const AoeAiPlayerOpsRor *ops);
/* 0x004c8540: unless we are the enemy of number 128's player: dislike 0
   and like 100 toward it, the tribe player's +0x28 with 3, string 2301
   sent to it, +0x1258c 0 and +0x12594 1. */
void aoe_ai_player_befriend_ror(AoeAiPlayerAiRor *ai,
                                const AoeAiPlayerOpsRor *ops);
/* 0x004c86a0: a message from `player`: 0 rescans the map; 1 (not from
   us, with a market) pays tribute `b` of resource `a` to it; 2 and 3
   (not from us) go to the tactical AI; 6 asks for stance `b` toward
   player `a` (0 enemy: dislike 100, request 3; 1: dislike and like 49,
   request 1; 2: like 100, request 0, then command sub 5 with 0 only in a
   game of kind 4 when that player's +4 is nonzero, else 1), with `a`
   changeable meanwhile; 7 stores `a` in +0x125a0 and sets number 217.
   0 for other kinds, else 1. */
int aoe_ai_player_message_ror(AoeAiPlayerAiRor *ai, int32_t player,
                              int32_t kind, int32_t a, int32_t b, int32_t c,
                              const AoeAiPlayerOpsRor *ops);

#endif
