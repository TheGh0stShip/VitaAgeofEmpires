#ifndef VITA_AOE_AI_TACTICS_H
#define VITA_AOE_AI_TACTICS_H

#include <stdint.h>

#include "ai_group.h"
#include "ai_module.h"
#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a tactical AI pieces (0x0040d150..0x0040ede0).
   Semantic storage; original offsets are noted. */

/* 0x0040d150: the index of the first action of group rule `rule` (0..9)
   with this field_0 (0..4) and code, else -1. */
int32_t aoe_ai_group_find_action_ror(AoeAiGroupRor *group, uint32_t field_0,
                                     uint32_t code, int32_t rule);
/* 0x0040d1d0: Attack 1, Annoy 2, Flank 3, Recon 4, else 0. */
uint8_t aoe_ai_attack_kind_ror(const char *name);
/* 0x0040e9e0: Stationary 1, Moving 2, Slow 3, Fast 4, Any 5, else 0;
   0x0040e980: Any 5, else 0. */
int32_t aoe_ai_speed_kind_ror(const char *name);
int32_t aoe_ai_any_kind_ror(const char *name);

#define AOE_AI_MEMBERS 50

typedef struct {
    int32_t id;        /* -1 free */
    uint8_t category;  /* +4 */
    uint8_t role;      /* +5 */
    uint8_t pad[2];    /* saved as they are */
} AoeAiMemberRor;

typedef struct {
    AoeAiMemberRor slots[AOE_AI_MEMBERS]; /* +0, 400 bytes */
    int32_t field_190, field_194, field_198, field_19c, field_1a0;
    int32_t totals[5];  /* +0x1a4 */
    uint8_t field_1b8;
    int32_t field_1bc, field_1c0;
} AoeAiMembersRor;

/* 0x0040d370 / 0x0040d390: per-category totals (0..4; -1 outside). */
int32_t aoe_ai_members_total_ror(const AoeAiMembersRor *m, int32_t category);
void aoe_ai_members_set_total_ror(AoeAiMembersRor *m, int32_t category,
                                  int32_t value);
/* 0x0040d3b0 used slots; 0x0040d3d0 with this category; 0x0040d400 with
   this category and role. */
uint8_t aoe_ai_members_count_ror(const AoeAiMembersRor *m);
uint8_t aoe_ai_members_count_category_ror(const AoeAiMembersRor *m,
                                          uint32_t category);
uint8_t aoe_ai_members_count_role_ror(const AoeAiMembersRor *m,
                                      uint32_t category, uint32_t role);
/* 0x0040d440: the category of an ID (0xff when absent or negative). */
uint8_t aoe_ai_members_category_ror(const AoeAiMembersRor *m, int32_t id);
/* 0x0040d470: into the first free slot (0 when full). */
int aoe_ai_members_add_ror(AoeAiMembersRor *m, int32_t id, uint8_t category,
                           uint8_t role);
/* 0x0040d4b0: the used IDs in slot order. */
int32_t aoe_ai_members_ids_ror(const AoeAiMembersRor *m, int32_t *out);
/* Load 0x0040d4f0 / save 0x0040d5b0: five i32, the totals (20 bytes),
   one byte, the 400 slot bytes, two i32. */
int aoe_ai_members_load_ror(AoeAiMembersRor *m, AoeOutcomeStream *in);
int aoe_ai_members_save_ror(const AoeAiMembersRor *m, AoeOutcomeStream *out);

/* The world object for an ID (0x005206d0): its state (+0x48) and hit
   points (+0x30). Nonzero when it exists. */
typedef int (*AoeAiUnitFn)(void *context, int32_t id, uint8_t *state,
                           float *hp);
/* 0x0040d720: free the slots whose object is gone or past state 2 (ID
   -1, category and role 0xff). 0x0040d770: total = sum of ftol(hit
   points) of the category's present members. */
void aoe_ai_members_purge_ror(AoeAiMembersRor *m, AoeAiUnitFn unit,
                              void *context);
void aoe_ai_members_sum_hp_ror(AoeAiMembersRor *m, uint32_t category,
                               AoeAiUnitFn unit, void *context);

/* Play list (vtable 0x00542c40): count +4, plays +0x4c of 0x608 bytes.
   Init 0x0040d7d0, release 0x0040d810 (deleting 0x0040d7f0), play(i)
   0x0040e950 (NULL outside or without plays). */
typedef struct { uint8_t bytes[0x608]; } AoeAiPlayRor;
typedef struct {
    int32_t count;
    AoeAiPlayRor *plays;
} AoeAiPlayListRor;
void aoe_ai_play_list_init_ror(AoeAiPlayListRor *list);
void aoe_ai_play_list_release_ror(AoeAiPlayListRor *list);
AoeAiPlayRor *aoe_ai_play_list_get_ror(AoeAiPlayListRor *list, int32_t i);

/* Counters (vtable 0x00542c54): up to eight values (+0xc) with two
   companion arrays (+0x2c, +0x4c), count +0x6c. Init 0x0040eb80: used
   values 0, the rest -1, companions -1; destructor 0x0040ebf0 (deleting
   0x0040eb60). get 0x0040ec00 (-1 outside), set 0x0040ec20, add
   0x0040ec40, fill all eight 0x0040ec60, count 0x0040ec80. */
typedef struct {
    int32_t field_4, field_8;
    int32_t values[8], second[8], third[8];
    int32_t count;
} AoeAiCountersRor;
void aoe_ai_counters_init_ror(AoeAiCountersRor *c, int32_t count);
void aoe_ai_counters_release_ror(AoeAiCountersRor *c);
int32_t aoe_ai_counters_get_ror(const AoeAiCountersRor *c, int32_t i);
void aoe_ai_counters_set_ror(AoeAiCountersRor *c, int32_t i, int32_t v);
void aoe_ai_counters_add_ror(AoeAiCountersRor *c, int32_t i, int32_t v);
void aoe_ai_counters_fill_ror(AoeAiCountersRor *c, int32_t v);
int32_t aoe_ai_counters_count_ror(const AoeAiCountersRor *c);

/* Strategy AI (0x3f0): 0x0040ec90, destructor 0x0040ece0 (deleting
   0x0040ecc0). TacAI (0x3f1): 0x0040ecf0, 0x0040ed40 (0x0040ed20).
   Trade AI (0x3f2, +0xf0 = 0): new 0x0040ed50 and load 0x0040edb0 (no
   saved state), 0x0040ede0 (0x0040ed90). */
typedef struct {
    AoeAiModuleRor module;
    int32_t field_f0;
} AoeAiTradeRor;
void aoe_ai_strategy_init_ror(AoeAiModuleRor *ai, int32_t player,
                              AoeAiLogRor *log);
void aoe_ai_strategy_release_ror(AoeAiModuleRor *ai);
void aoe_ai_tac_init_ror(AoeAiModuleRor *ai, int32_t player,
                         AoeAiLogRor *log);
void aoe_ai_tac_release_ror(AoeAiModuleRor *ai);
void aoe_ai_trade_init_ror(AoeAiTradeRor *ai, int32_t player,
                           AoeAiLogRor *log);
void aoe_ai_trade_load_ror(AoeAiTradeRor *ai, int32_t player,
                           AoeAiLogRor *log);
void aoe_ai_trade_release_ror(AoeAiTradeRor *ai);

/* Unit AI vtable +0xc0 default 0x0040eb50: -1. */
int32_t aoe_ai_unit_default_c0_ror(void);

#endif
