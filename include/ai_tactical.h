#ifndef VITA_AOE_AI_TACTICAL_H
#define VITA_AOE_AI_TACTICAL_H

#include <stdint.h>

#include "ai_module.h"
#include "ai_strategy.h"   /* AoeAiIntListRor, AOE_AI_SN_COUNT */
#include "ai_tactics.h"    /* AoeAiCountersRor */
#include "ai_unit_group.h"
#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a tactical AI ("TacAI", module 0x3f1, vtable
   0x00548cf4): its state, strategic numbers, unit groups and the small
   members around them (0x004cebe0..0x004d0f10, 0x004d76c0..0x004d77f0,
   0x004e0400..0x004e0c00). Semantic storage; comments give the original
   offsets. The computer player holds it at +0x1146c, its Strategy AI at
   +0x10e74 and its information AI at +0xcec. */

enum { AOE_AI_TAC_LISTS = 10, AOE_AI_TAC_TARGETS = 50 };

typedef struct {
    AoeAiModuleRor module;
    int32_t field_f0;                       /* +0xf0, the computer player */
    /* +0xf4 + 0x10 * i: ten ID lists. 0 holds +0xf8 / 0x004d76c0, 1 its
       count +0x108 (0x004d76d0) and spare +0x10c (0x004d76f0), 2 +0x118
       (0x004d7710), 4 +0x138 (0x004d7730). */
    AoeAiIntListRor lists[AOE_AI_TAC_LISTS];
    int32_t sn[AOE_AI_SN_COUNT];            /* +0x194, -1 */
    int32_t targets[AOE_AI_TAC_TARGETS][6]; /* +0x51c, first five -1 */
    int32_t field_9cc, field_9d0;           /* +0x9cc, +0x9d0 */
    /* The unit-group list (+0x9d4 sentinel; groups in list order). */
    AoeAiUnitGroupRor **groups;
    int32_t groups_len, groups_cap;
    int32_t next_group_id;                  /* +0xd04 */
    int32_t group_count;                    /* +0xd08 */
    int32_t field_d0c, field_d10, field_d14, field_d18;
    int32_t last_update;                    /* +0xd1c */
    int32_t field_d20, field_d24;
    AoeAiIntListRor list_d28, list_d38, list_d48, list_d58;
    int32_t field_d68[4], field_d78[4];     /* 0 */
    int32_t field_d88[4];                   /* -1 (0x004d7740) */
    int32_t field_d98[4];                   /* 0 */
    AoeAiCountersRor counters;              /* +0xda8, four */
    int32_t field_e18, state, field_e20;    /* +0xe18, +0xe1c, +0xe20 */
    int32_t field_e24[4], field_e34[4];     /* 0 */
    int32_t field_e44, field_e48;
    int32_t block_e4c[76];                  /* +0xe4c; [69] is +0xf60 */
    int32_t field_f7c, field_f80, field_f84; /* -1 */
    int32_t field_f88, field_f8c, field_f90, field_f94, field_f98;
    int32_t field_f9c, field_fa0, field_fa4, field_fa8;
    int32_t block_fac[11];                  /* +0xfac; [10] is +0xfd4 */
    int32_t block_fd8[9];                   /* +0xfd8 */
    int32_t field_ffc, field_1000;          /* -1 */
    int32_t field_1004, field_1008;
} AoeAiTacticalRor;

/* 0x004cebe0: the module and the defaults above (no groups). */
void aoe_ai_tactical_init_ror(AoeAiTacticalRor *ai, int32_t player,
                              AoeAiLogRor *log);
/* 0x004d0590 (deleting 0x004ceef0): remove every group, free the lists. */
void aoe_ai_tactical_release_ror(AoeAiTacticalRor *ai);

/* 0x004d0f10 save (vtable +0x3c): lists 0..2 (count and values), +0x9cc,
   +0x9d0, the 0x4b0-byte target block, lists 3..9, i32 226 and the
   numbers, lists +0xd28..+0xd58, the 16-byte arrays +0xd68, +0xd78,
   +0xd88, +0xd98, +0xe24, +0xe34, the four counter values, +0xd04, the
   group count (+0xd08) and every listed group (0x004cca20; the two
   differ only if unnumbered groups were made), +0xd0c..+0xd24, +0xe18,
   +0xe1c, +0xe44, +0xe48, the 0x130-byte block, +0xf7c..+0xf9c, +0xfa4,
   +0xfa0, +0xfa8, the 0x2c- and 0x24-byte blocks.
   0x004cef10 load constructor: the same, with the save-version gates of
   the original: list 3 from 2.3, list 9 from 1.4; the target block is 50
   entries of 16 bytes before 7.17 (then -1, 0), of 20 bytes before 7.19
   (then 0); +0xd10 from 2.4, +0xe18 from 2.2, +0xe44/+0xe48 from 6.1, the
   0x130 block from 6.2 (zero before), +0xf7c from 6.3, +0xf80/+0xf84
   from 6.4, +0xf88/+0xf8c from 6.9, +0xf90..+0xf98 from 6.95, +0xf9c from
   7.18, +0xfa4 from 6.96, +0xfa0 from 6.98, +0xfa8 from 6.993, the 0x2c
   block from 7.04, the 0x24 block from 7.15. +0xf60 and +0xfd4 are reset
   to 0. Lists drop repeated values. Each group is loaded into a new list
   entry (0x004e0400 without an ID). The original reads as many numbers
   as the count says; the port refuses a count above 226. */
int aoe_ai_tactical_save_ror(const AoeAiTacticalRor *ai,
                             AoeOutcomeStream *out);
int aoe_ai_tactical_load_ror(AoeAiTacticalRor *ai, int32_t player,
                             float version, AoeOutcomeStream *in,
                             AoeAiLogRor *log);

/* 0x004d0880: the state (+0xe1c) name: "Setup SolGroups", "Eval
   CivDist", "Task Civs", "Setup BoatGroups", "Fill BoatGroups", "Task
   Boats", "Fill SolGroups", "Task IdleSol", "Task ActSol",
   "Playtasking", "Task UngrpSol", "Research", "Train", "BldLst Change",
   "Help Build", "Repair Bldg", "Repair Wall", "Build", "Open Builds",
   "Open Tasks", "Food dropsite", "BldLst Insertions" (0..21), else
   "Invalid". */
const char *aoe_ai_tactical_state_name_ror(const AoeAiTacticalRor *ai);

/* Plain getters: 0x004d76c0 list 0 count, 0x004d76d0 list 1 count,
   0x004d76e0 +0x9cc, 0x004d76f0 list 1 spare (+0x10c), 0x004d7700 +0x9d0,
   0x004d7710 list 2 count, 0x004d7730 list 4 count. */
enum {
    AOE_AI_TAC_LIST0_COUNT, AOE_AI_TAC_LIST1_COUNT, AOE_AI_TAC_FIELD_9CC,
    AOE_AI_TAC_LIST1_SPARE, AOE_AI_TAC_FIELD_9D0, AOE_AI_TAC_LIST2_COUNT,
    AOE_AI_TAC_LIST4_COUNT
};
int32_t aoe_ai_tactical_get_ror(const AoeAiTacticalRor *ai, int which);
/* 0x004d7740: +0xd88[i] for i 0..3, else -1; 0x004d7760: the counter
   value at that index (the counters' -1 outside). */
int32_t aoe_ai_tactical_d88_ror(const AoeAiTacticalRor *ai, int32_t i);
int32_t aoe_ai_tactical_d88_counter_ror(const AoeAiTacticalRor *ai,
                                        int32_t i);

/* 0x004d77d0: strategic number `i` (-1 outside 0..225). */
int32_t aoe_ai_tactical_sn_ror(const AoeAiTacticalRor *ai, int32_t i);
/* 0x004d77f0: set number `i` (ignored outside 0..225). Numbers 117..120
   and 156..159 are clamped to 0..100. Setting 105 also passes the value
   to the information AI (its +0x204, 0x004c3cf0) through `info_204`. */
typedef void (*AoeAiInfoSetFn)(void *context, int32_t value);
void aoe_ai_tactical_set_sn_ror(AoeAiTacticalRor *ai, int32_t i,
                                int32_t value, AoeAiInfoSetFn info_204,
                                void *context);

/* 0x004e0400: a new group appended to the list; with `numbered` 1 it
   takes the next ID (+0xd04, then raised) and the group count rises.
   NULL when out of memory. */
AoeAiUnitGroupRor *aoe_ai_tactical_new_group_ror(AoeAiTacticalRor *ai,
                                                 int32_t numbered);
/* 0x004e04b0: delete the first group with this ID (1), lowering the
   count; 0 when none. */
int aoe_ai_tactical_delete_group_ror(AoeAiTacticalRor *ai, int32_t id);
/* 0x004e0520: delete every group of this type (-1 for all); 1. */
int aoe_ai_tactical_delete_groups_ror(AoeAiTacticalRor *ai, int32_t type);
/* 0x004e05a0: the first group whose ID, type, +0x180 and +0x184 match
   (-1 matches anything), else NULL. */
AoeAiUnitGroupRor *aoe_ai_tactical_find_group_ror(const AoeAiTacticalRor *ai,
                                                  int32_t id, int32_t type,
                                                  int32_t field_180,
                                                  int32_t field_184);
/* 0x004e09c0: how many groups match the type and +0x180 (-1 any). */
int32_t aoe_ai_tactical_count_groups_ror(const AoeAiTacticalRor *ai,
                                         int32_t type, int32_t field_180);
/* 0x004e0a20: the members of the groups of this type (-1 any);
   0x004d7720 is the count for type 0x66. */
int32_t aoe_ai_tactical_grouped_units_ror(const AoeAiTacticalRor *ai,
                                          int32_t type);
int32_t aoe_ai_tactical_grouped_66_ror(const AoeAiTacticalRor *ai);
/* 0x004e0980: the first group holding the unit, else NULL. */
AoeAiUnitGroupRor *aoe_ai_tactical_group_of_ror(const AoeAiTacticalRor *ai,
                                                int32_t unit);
/* 0x004e0930: take the unit out of its group (0x004ccd10 with the
   group ops); a group left empty is deleted. 0 when it is in none. */
int aoe_ai_tactical_ungroup_ror(AoeAiTacticalRor *ai, int32_t unit,
                                const AoeAiUnitGroupOpsRor *ops);

/* 0x004e0b00: with victory kind 0, the strategy's target unit when no
   group has it as +0x184; 0x004e0b50: the same for kind 4 and the second
   unit. Otherwise -1. */
int32_t aoe_ai_tactical_free_target_ror(const AoeAiTacticalRor *ai,
                                        const AoeAiStrategyRor *strategy);
int32_t aoe_ai_tactical_free_second_ror(const AoeAiTacticalRor *ai,
                                        const AoeAiStrategyRor *strategy);

/* 0x004e0bc0: with number 71 at 1, the smaller of +0xd24 and number 48;
   otherwise +0xd24 for `kind` 0x2e, else number 48. */
int32_t aoe_ai_tactical_d24_limit_ror(const AoeAiTacticalRor *ai,
                                      int32_t kind);
/* 0x004e0c00: +0xd24 = number 46 moved by rand() % n - n / 2 when
   number 102 (n) is positive; not below 0. */
typedef int32_t (*AoeAiRandFn)(void *context); /* 0x0052605d */
void aoe_ai_tactical_roll_d24_ror(AoeAiTacticalRor *ai, AoeAiRandFn rand_fn,
                                  void *context);

#endif
