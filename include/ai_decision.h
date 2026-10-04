#ifndef VITA_AOE_AI_DECISION_H
#define VITA_AOE_AI_DECISION_H

#include <stdint.h>

#include "ai_module.h"
#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a Main Decision AI (module 1000, vtable
   0x00542bb0): the list of object IDs the computer player commands.
   Semantic storage, not the x86 layout. */
typedef struct {
    AoeAiModuleRor module;
    void *owner;       /* +0xf0, the player (world at +0x3c) */
    int32_t *ids;      /* +0xf4 */
    int32_t count;     /* +0xf8 */
    int32_t field_fc;  /* +0xfc */
    int32_t capacity;  /* +0x100 */
} AoeAiDecisionRor;

/* What the queries read from a world object (0x005206d0 lookup): ID +4,
   master type +8 then +0x10 (type), +0x14 (class) and +0x26 (maximum
   hit points), hit points +0x30, +0x44, state +0x48, vtable +0x90() and
   the unit AI +0x74 with its +0x30 (order), +0x34 and +0x38. */
typedef struct {
    int32_t id;
    int16_t type, unit_class, max_hp;
    float hp;
    float field_44;
    uint8_t state;
    int32_t method_90;
    int has_ai;
    int32_t ai_order, ai_34, ai_38;
} AoeAiObjectViewRor;

typedef struct {
    void *context;
    /* Nonzero and `out` filled when the ID names an object. */
    int (*object)(void *context, int32_t id, AoeAiObjectViewRor *out);
    /* Module vtable +0x4c(id, argument), a further filter. */
    int (*accept)(void *context, int32_t id, int32_t argument);
} AoeAiDecisionOps;

/* 0x0040b4d0 new and 0x0040b570 load (an i32 count, then IDs; duplicates
   are dropped); destructor 0x0040b710 (deleting 0x0040b550). Save
   0x0040b790 writes the count and the IDs. */
void aoe_ai_decision_init_ror(AoeAiDecisionRor *ai, int32_t player,
                              void *owner, AoeAiLogRor *log);
int aoe_ai_decision_load_ror(AoeAiDecisionRor *ai, int32_t player,
                             void *owner, AoeAiLogRor *log,
                             AoeOutcomeStream *in);
int aoe_ai_decision_save_ror(AoeAiDecisionRor *ai, AoeOutcomeStream *out);
void aoe_ai_decision_release_ror(AoeAiDecisionRor *ai);
/* 0x0040b880: add the ID (1 if already listed; 0 only for no object);
   0x0040b950: remove it, shifting the rest (0 when absent). The array
   grows one slot at a time; a failed growth keeps the old array, as in
   the original (here the add is then refused). */
int aoe_ai_decision_add_ror(AoeAiDecisionRor *ai, int has_object,
                            int32_t id);
int aoe_ai_decision_remove_ror(AoeAiDecisionRor *ai, int32_t id);
/* 0x0040bab0: the world object for an ID (0x005206d0). */
int aoe_ai_decision_object_ror(const AoeAiDecisionOps *ops, int32_t id,
                               AoeAiObjectViewRor *out);
/* 0x0040b9c0: listed objects with a unit AI whose order is `order`. */
int32_t aoe_ai_decision_count_order_ror(AoeAiDecisionRor *ai,
                                        const AoeAiDecisionOps *ops,
                                        int32_t order);
/* 0x0040bad0: the first listed object with a unit AI matching every
   filter that is not -1: ID, type, class, order, accept(argument), AI
   +0x34, AI +0x38, state, and hit points (+0x44) at most `max_44`. */
int aoe_ai_decision_find_ror(AoeAiDecisionRor *ai,
                             const AoeAiDecisionOps *ops, int32_t id,
                             int32_t type, int32_t unit_class, int32_t order,
                             int32_t accept, int32_t ai_34, int32_t ai_38,
                             int32_t state, int32_t max_44,
                             AoeAiObjectViewRor *out);
/* 0x0040bd10: the first matching ID, type and class whose order is
   neither `not_order` nor `not_order2`, with the state (-1 any). */
int aoe_ai_decision_find_other_ror(AoeAiDecisionRor *ai,
                                   const AoeAiDecisionOps *ops, int32_t id,
                                   int32_t type, int32_t unit_class,
                                   int32_t not_order, int32_t not_order2,
                                   int32_t state, AoeAiObjectViewRor *out);
/* 0x0040be70: of the damaged objects of this class (state nonzero,
   +0x90 = 1, type when not -1), the one with the highest health percent
   (truncated hit points x 100 / maximum). 0 when none. */
int aoe_ai_decision_healthiest_damaged_ror(AoeAiDecisionRor *ai,
                                           const AoeAiDecisionOps *ops,
                                           int32_t unit_class, int32_t type,
                                           AoeAiObjectViewRor *out);
/* 0x0040be50: logs "ERROR!  %d is an unrecognized action id." and returns
   -1. */
int32_t aoe_ai_unknown_action_ror(const AoeAiModuleRor *module,
                                  int32_t action);

#endif
