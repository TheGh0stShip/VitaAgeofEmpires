#ifndef VITA_AOE_AI_TACTICAL_H
#define VITA_AOE_AI_TACTICAL_H

#include <stdint.h>

#include "ai_info.h" /* AoeAiWorldObjectRor */
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

enum { AOE_AI_TAC_LISTS = 10, AOE_AI_TAC_GATHERERS = 50 };

typedef struct {
    AoeAiModuleRor module;
    int32_t field_f0;                       /* +0xf0, the computer player */
    /* +0xf4 + 0x10 * i: ten ID lists. 0 holds +0xf8 / 0x004d76c0, 1 its
       count +0x108 (0x004d76d0) and spare +0x10c (0x004d76f0), 2 +0x118
       (0x004d7710), 4 +0x138 (0x004d7730). */
    AoeAiIntListRor lists[AOE_AI_TAC_LISTS];
    int32_t sn[AOE_AI_SN_COUNT];            /* +0x194, -1 */
    /* +0x51c: 50 gatherers of 24 bytes: unit ID, resource kind, +8, the
       amount gathered, +0x10, +0x14 (the first five -1 when free). */
    int32_t gatherers[AOE_AI_TAC_GATHERERS][6];
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
   +0x9d0, the 0x4b0-byte gatherer block, lists 3..9, i32 226 and the
   numbers, lists +0xd28..+0xd58, the 16-byte arrays +0xd68, +0xd78,
   +0xd88, +0xd98, +0xe24, +0xe34, the four counter values, +0xd04, the
   group count (+0xd08) and every listed group (0x004cca20; the two
   differ only if unnumbered groups were made), +0xd0c..+0xd24, +0xe18,
   +0xe1c, +0xe44, +0xe48, the 0x130-byte block, +0xf7c..+0xf9c, +0xfa4,
   +0xfa0, +0xfa8, the 0x2c- and 0x24-byte blocks.
   0x004cef10 load constructor: the same, with the save-version gates of
   the original: list 3 from 2.3, list 9 from 1.4; the gatherer block is 50
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
/* 0x004e0520: delete every group with this task (+0x14; -1 for all); 1. */
int aoe_ai_tactical_delete_groups_ror(AoeAiTacticalRor *ai, int32_t task);
/* 0x004e05a0: the first group whose ID, task (+0x14), +0x180 and +0x184 match
   (-1 matches anything), else NULL. */
AoeAiUnitGroupRor *aoe_ai_tactical_find_group_ror(const AoeAiTacticalRor *ai,
                                                  int32_t id, int32_t task,
                                                  int32_t field_180,
                                                  int32_t field_184);
/* 0x004e09c0: how many groups match the task and +0x180 (-1 any). */
int32_t aoe_ai_tactical_count_groups_ror(const AoeAiTacticalRor *ai,
                                         int32_t task, int32_t field_180);
/* 0x004e0a20: the members of the groups with this task (-1 any);
   0x004d7720 is the count for task 0x66. */
int32_t aoe_ai_tactical_grouped_units_ror(const AoeAiTacticalRor *ai,
                                          int32_t task);
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


/* ---- Members working through the computer player ---- */

/* A unit order through the player's vtable +0x30: the player's index,
   the unit, the action, a target and second target, four floats, then
   the fixed 1, 0 and a value. */
typedef struct {
    int32_t player, unit, action, target, target2;
    float a, b, c, d;
    int32_t flag, zero, value;
} AoeAiTaskOrderRor;

typedef struct {
    void *context;
    int16_t self; /* the player's index (+0x4a) */
    /* 0x0040bab0 on the player (a listed object) and 0x005206d0 (any). */
    int (*object)(void *context, int32_t id, AoeAiWorldObjectRor *out);
    int (*world_object)(void *context, int32_t id, AoeAiWorldObjectRor *out);
    /* The object's unit AI (+0x74) and its task (+0x30); 0 without one. */
    int (*unit_task)(void *context, int32_t id, int32_t *task);
    /* Player vtable +0x30 (orders) and +0x44 (three bytes, count 1). */
    void (*order)(void *context, const AoeAiTaskOrderRor *order);
    void (*player_44)(void *context, int32_t id, const uint8_t bytes[3]);
    /* Player +0x50 resource `i`; the explored share (player +0x38 map,
       +0x1c / +0x20). */
    float (*resource)(void *context, int32_t i);
    double (*explored_ratio)(void *context);
    /* Build AI list counts (0x00408ff0 all, 0x004090a0 not built) for a
       type and category. */
    int32_t (*build_items)(void *context, int32_t type_id, int32_t category);
    int32_t (*build_unbuilt)(void *context, int32_t type_id,
                             int32_t category);
    /* The information AI's far-target tests (0x004c5fc0, 0x004c60b0). */
    int (*target_far)(void *context, int32_t k);
    int (*food_far)(void *context);
    /* The Diplomacy AI's dislike setter (0x0040ab40 with kind 0). */
    void (*set_dislike)(void *context, int32_t player, int32_t value);
    /* World time in milliseconds (world +4). */
    uint32_t (*game_time)(void *context);
    /* Unit vtable +0x10c (range). */
    double (*range)(void *context, int32_t id);
    /* World players (+0x3c +0x3c), a player's +4 (0x00454150), our
       vtable +0x1c (ally) toward it, its vtable +0x1c toward us, the
       world's statistics player byte (+0x11d, 0xff unset), and that
       player's vtable +0x54 with our index, 2 and three values. */
    int16_t player_count;
    int32_t (*player_field_4)(void *context, int32_t player);
    int (*is_ally)(void *context, int32_t player);
    int (*names_us_ally)(void *context, int32_t player);
    uint8_t (*stats_player)(void *context);
    void (*set_stats_player)(void *context, uint8_t player);
    void (*player_54)(void *context, int32_t player, int32_t from,
                      int32_t kind, int32_t a, int32_t b, int32_t c);
    int32_t (*rand)(void *context); /* 0x0052605d */
    /* A unit AI's current action (+0x28); 0 without a unit AI. */
    int (*unit_action)(void *context, int32_t id, int32_t *action);
    /* An object's distance to a point (0x004aaf50). */
    double (*distance)(void *context, int32_t id, float x, float y, float z);
    /* Our first town centre (0x0040bad0 with type 0x6d), -1 for none. */
    int32_t (*first_center)(void *context);
} AoeAiTacticalOpsRor;

/* 0x004dc650: ftol(number 34 / f), at most 99. */
int32_t aoe_ai_tactical_effort_ror(const AoeAiTacticalRor *ai, float f);
/* 0x004e2f90: +0xd68[i] (0..3, else -1). 0x004e2f30 / 0x004e2f60 /
   0x004e2fb0: +0xd98 / +0xd68 / +0xd78 at index +0xd88[i]. The original
   indexes with whatever +0xd88[i] holds; the port returns -1 when that
   is outside 0..3. */
int32_t aoe_ai_tactical_d68_ror(const AoeAiTacticalRor *ai, int32_t i);
int32_t aoe_ai_tactical_mapped_ror(const AoeAiTacticalRor *ai, int which,
                                   int32_t i);
enum { AOE_AI_TAC_MAP_D98, AOE_AI_TAC_MAP_D68, AOE_AI_TAC_MAP_D78 };
/* 0x004e1a40: number 148 when +0xd78[i] is 0, else counter i /
   +0xd78[i]. */
int32_t aoe_ai_tactical_counter_share_ror(const AoeAiTacticalRor *ai,
                                          int32_t i);
/* 0x004e15f0: +0xe18 becomes 1 when it is not already and number 134 is
   0 or equal to `value`. */
void aoe_ai_tactical_arm_e18_ror(AoeAiTacticalRor *ai, int32_t value);
/* 0x004e2190: ftol(resource 6) equals `age`. */
int aoe_ai_tactical_age_is_ror(const AoeAiTacticalOpsRor *ops, int32_t age);
/* Gatherers: 0x004e1870 the entry with this unit (NULL), 0x004e1a10
   whether there is one, 0x004e33f0 how many of the first +0x9cc have
   +0x10 equal to `value`. 0x004e18a0 adds a unit (the existing entry, or
   the first free one with +0x10 -1 and +0x14 0; the count +0x9cc rises;
   NULL when full or for -1, which logs "ERROR adding gatherer id -1").
   0x004e1940 removes one (its first five fields -1, the count drops, and
   following entries move down while used, without their +0x14; -1 logs
   "ERROR removing gatherer id -1"). */
int32_t *aoe_ai_tactical_gatherer_ror(AoeAiTacticalRor *ai, int32_t unit);
int aoe_ai_tactical_has_gatherer_ror(const AoeAiTacticalRor *ai,
                                     int32_t unit);
int32_t aoe_ai_tactical_count_gatherers_ror(const AoeAiTacticalRor *ai,
                                            int32_t value);
int32_t *aoe_ai_tactical_add_gatherer_ror(AoeAiTacticalRor *ai, int32_t unit);
int aoe_ai_tactical_remove_gatherer_ror(AoeAiTacticalRor *ai, int32_t unit);
/* 0x004dc570: drop a unit: out of list 1 (searched to its capacity) and
   the gatherers; a unit whose AI task is 0x261 or 0x265 still listed as a
   gatherer lowers +0xd68 for its resource kind. 0x004d7790 then orders
   it to stop (0x004da110 with 100); -1 logs "ERROR DETASKING Unit #%d!". */
void aoe_ai_tactical_drop_unit_ror(AoeAiTacticalRor *ai, int32_t unit,
                                   const AoeAiTacticalOpsRor *ops);
void aoe_ai_tactical_detask_ror(AoeAiTacticalRor *ai, int32_t unit,
                                const AoeAiTacticalOpsRor *ops);
/* Orders: 0x004da110 stop (0x2c2, no targets, -1.0 four times, value);
   0x004da160 move to (x, y) (0x2c6, 1.0, 1.0); 0x004da1b0 action 0x2d4
   on `target` with four floats. Each returns 1. */
int aoe_ai_tactical_order_stop_ror(const AoeAiTacticalOpsRor *ops,
                                   int32_t unit, int32_t value);
int aoe_ai_tactical_order_move_ror(const AoeAiTacticalOpsRor *ops,
                                   int32_t unit, float x, float y,
                                   int32_t value);
int aoe_ai_tactical_order_2d4_ror(const AoeAiTacticalOpsRor *ops,
                                  int32_t unit, int32_t target, float a,
                                  float b, float c, float d, int32_t value);
/* 0x004e1570: the player's vtable +0x44 with the bytes (a, b, 1). */
void aoe_ai_tactical_player_44_ror(const AoeAiTacticalOpsRor *ops,
                                   int32_t id, uint8_t a, uint8_t b);
/* 0x004e15b0: a listed object of class 0x12 or 0xd. 0x004d99a0: a world
   object whose unit AI has task `task`. 0x004e2030: a world object (the
   caller's) of type 0x103 whose AI task is 0x261 or 0x262; 0x004e2090
   counts those in list 0. */
int aoe_ai_tactical_priest_or_siege_ror(const AoeAiTacticalOpsRor *ops,
                                        int32_t id);
int aoe_ai_tactical_unit_has_task_ror(const AoeAiTacticalOpsRor *ops,
                                      int32_t id, int32_t task);
int aoe_ai_tactical_busy_103_ror(const AoeAiTacticalOpsRor *ops, int32_t id,
                                 int listed);
int32_t aoe_ai_tactical_count_busy_103_ror(const AoeAiTacticalRor *ai,
                                           const AoeAiTacticalOpsRor *ops);
/* 0x004e34e0: groups with task 0x67..0x6c and +0x180 equal to 8. */
int32_t aoe_ai_tactical_count_groups_8_ror(const AoeAiTacticalRor *ai);
/* 0x004e2ea0: when +0xe18 is set, dislike `player` 100 and cap +0x160 of
   the groups with task 100 or 0x67 at their member count (1); else 0.
   0x004e3420: when +0xe18 is set, a task-100 group of another type than
   1 with +0x160 above its count lowers it to the count once (the excess,
   at most 5) minutes have passed since its last member joined. */
int aoe_ai_tactical_cap_groups_ror(AoeAiTacticalRor *ai, int32_t player,
                                   const AoeAiTacticalOpsRor *ops);
void aoe_ai_tactical_settle_groups_ror(AoeAiTacticalRor *ai,
                                       const AoeAiTacticalOpsRor *ops);
/* 0x004e1130: the group member to lead: number 75 picks by range (2) or
   hit points (otherwise), the smallest for 1 and 3, else the largest;
   type 0x7d is skipped. -1 for none. */
int32_t aoe_ai_tactical_pick_leader_ror(const AoeAiTacticalRor *ai,
                                        const AoeAiUnitGroupRor *group,
                                        const AoeAiTacticalOpsRor *ops);
/* 0x004e1200: the same choice over an ID list. */
int32_t aoe_ai_tactical_pick_leader_ids_ror(const AoeAiTacticalRor *ai,
                                            const AoeAiIntListRor *ids,
                                            const AoeAiTacticalOpsRor *ops);
/* 0x004db9f0: the unit joins the tasked list (+0xd58, once) and is
   ordered to action 0x2bd on `target` (-1 x3 then `d`, value).
   0x004dc310: the same with action 0x2cd, our index as second target,
   -1 x3 then 1.0 and value 100; -1 refused (0). Both return 1. */
int aoe_ai_tactical_order_2bd_ror(AoeAiTacticalRor *ai, int32_t unit,
                                  int32_t target, float d, int32_t value,
                                  const AoeAiTacticalOpsRor *ops);
int aoe_ai_tactical_order_2cd_ror(AoeAiTacticalRor *ai, int32_t unit,
                                  int32_t target,
                                  const AoeAiTacticalOpsRor *ops);
/* 0x004e2cd0: with number 196 at 1 and number 197 seconds since +0xfa0
   (then reset to now): an unset statistics player becomes us when we are
   the only player with +4 equal to 1, else the (rand() % that count +
   1)-th such player; then every other player with +4 set, that we and
   that name each other allies, receives its vtable +0x54 call (ours, 2,
   a, b, c) when its index is above ours and not the statistics player,
   or when we are the statistics player. */
void aoe_ai_tactical_share_with_allies_ror(AoeAiTacticalRor *ai, int32_t a,
                                           int32_t b, int32_t c,
                                           const AoeAiTacticalOpsRor *ops);
/* 0x004e1cb0: number 167 is not positive, or the explored share * 100
   (truncated) reaches it. */
int aoe_ai_tactical_explored_enough_ror(const AoeAiTacticalRor *ai,
                                        const AoeAiTacticalOpsRor *ops);
/* 0x004e1b20: granaries to add for resource `k`: none while one is
   unbuilt (`unbuilt`) or the stock reaches number 190 + k; with number
   150 set, 150 * the age (at least 1) less the granaries (`total`), not
   below 0; otherwise the far-target test. 0x004e1c00: the same for
   storage pits with food, number 151 as a cap, and the food test. */
int32_t aoe_ai_tactical_need_granary_ror(const AoeAiTacticalRor *ai,
                                         int32_t *total, int32_t *unbuilt,
                                         int32_t k,
                                         const AoeAiTacticalOpsRor *ops);
int32_t aoe_ai_tactical_need_pit_ror(const AoeAiTacticalRor *ai,
                                     int32_t *total, int32_t *unbuilt,
                                     const AoeAiTacticalOpsRor *ops);

/* Collecting candidates into +0xd48 (emptied first); each returns how
   many it found. The ungrouped units of a list: 0x004ddd30 list 2,
   0x004df9f0 list 4, 0x004dfe00 list 6, 0x004e0000 list 7, 0x004e0200 list
   8, 0x004de120 list 9. The surplus members of groups whose +0x180 is 0
   or 1 and whose +0x160 is below their count, taken in member order
   (skipping the commander) until the count would fall to +0x160:
   0x004ddf30 for tasks 100, 0x65, 0x66 and 0x004dfbf0 for 0x67..0x6c. */
int32_t aoe_ai_tactical_collect_ungrouped_ror(AoeAiTacticalRor *ai,
                                              int32_t list);
int32_t aoe_ai_tactical_collect_surplus_ror(AoeAiTacticalRor *ai,
                                            int boats);

/* 0x004d9760: among list 0 units not tasked this round (+0xd58), world
   objects with a unit AI whose action is none of `skip0..2` (type 0x103
   only with `allow_103`), the nearest (`nearest` 1) or farthest (0) from
   tile (x, y) by the unit's distance (kept as a float); -1 for none. */
int32_t aoe_ai_tactical_pick_unit_ror(const AoeAiTacticalRor *ai,
                                      const int32_t tile[2], int32_t nearest,
                                      int32_t skip0, int32_t skip1,
                                      int32_t skip2, int32_t allow_103,
                                      const AoeAiTacticalOpsRor *ops);
/* 0x004e1620: an exploring spot for `unit`: of the eight tiles 10 away
   from our town centre's truncated tile (clockwise from x-10, y-10), the
   one inside the information AI's map, unexplored, in the unit's zone and
   nearest to it; it is cleared in the explored grid and written to
   `out` (1). Without a centre or such a tile, the information AI's own
   choice (0x004c3d00). */
int aoe_ai_tactical_explore_spot_ror(AoeAiTacticalRor *ai, int32_t unit,
                                     int32_t out[2],
                                     const AoeAiTacticalOpsRor *ops,
                                     AoeAiInfoRor *info,
                                     const AoeAiInfoOpsRor *info_ops);

#endif
