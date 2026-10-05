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
    /* Our object's own zone (0x0040bab0, then its zone getter); 0 without
       one. */
    int (*object_zone)(void *context, int32_t id, uint8_t *zone);
    /* Unit vtable +0x114 (it could stand at (x, y)) and the world's +0x100
       (0x00520660). */
    int (*free_tile)(void *context, int32_t id, float x, float y);
    int32_t (*world_100)(void *context);
    /* A unit AI's +0x2c (0x00458340), read next to its task (+0x30,
       0x00458400); 0 without a unit AI. */
    int (*unit_2c)(void *context, int32_t id, int32_t *value);
    /* The Build AI's pending costs (0x004b8960 into `costs`, at most
       `limit` items) and the Resource AI's amount `i` (0x004c8c60). */
    void (*build_costs)(void *context, AoeAiCountersRor *costs, int32_t limit);
    int32_t (*resource_amount)(void *context, int32_t i);
    /* The Build AI's age test (0x004b94b0 with mode 1) and the information
       AI's best distance `k` (+0x540, the computer player's +0x122c for
       0). */
    int (*age_open)(void *context, int32_t age);
    int32_t (*info_best_distance)(void *context, int32_t k);
    /* A unit AI's target (+0x34, 0x0040f6e0); 0 without a unit AI. */
    int (*unit_ai_target)(void *context, int32_t id, int32_t *target);
    /* The tribe player's vtable +0x4c (unit and four values: a task
       order) and +0xfc (builder, type, x, y, unique ID: a build order). */
    void (*player_4c)(void *context, int32_t unit, int32_t a, int32_t b,
                      int32_t c, int32_t d);
    void (*build_order)(void *context, int32_t builder, int32_t type,
                        float x, float y, int32_t unique_id);
    /* The Build AI's next item (0x004b8110 with 0), the ConAI's site for
       it (0x004b98e0 with our +0xe4c search state; NULL for none) and
       deleting such a site (its vtable destructor with 1). */
    AoeAiBuildItemRor *(*build_next)(void *context);
    AoeAiConItemRor *(*con_choose)(void *context, AoeAiBuildItemRor *item,
                                   int32_t builder, int32_t *search,
                                   int32_t arg);
    void (*con_free)(void *context, AoeAiConItemRor *site);
    /* A unit's vtable +0x198 path test to tile (x, y) at height z within
       `radius` (then -1, 0, 1, -1, -1). */
    int (*can_reach)(void *context, int32_t id, int32_t x, int32_t y,
                     int32_t z, float radius);
} AoeAiTacticalOpsRor;

/* 0x004e0630: in list order, the group with task `task` and +0x180
   `field_180` (-1 matches any) nearest to tile `pt` by ftol(sqrt(dx^2 +
   dy^2)) from its truncated +0x19c location; for task 0x6a, groups some
   group names in +0x1c4 are skipped; with `zone` not -1 the commander
   must be in that zone. A group is taken when it is nearer than `max`
   with none taken yet, when `max` is -1 (so the last match wins) or when
   it is nearer than the one taken. NULL for none. */
AoeAiUnitGroupRor *aoe_ai_tactical_nearest_group_ror(
    const AoeAiTacticalRor *ai, int32_t task, int32_t field_180, int32_t max,
    const int32_t pt[2], int32_t zone, const AoeAiTacticalOpsRor *ops);
/* 0x004d2560: an idle unit (task -1, 0x004d99a0) steps around the box
   (x0, y0)..(x1, y1): of the eight points 2 outside its corners and edge
   middles (clockwise from the top left), the nearest it could stand at
   (distance from its height) gets a move order (0x004da160, 100). 1 when
   ordered. */
int aoe_ai_tactical_step_around_ror(const AoeAiTacticalOpsRor *ops, int32_t unit,
                                    float x0, float y0, float x1, float y1);
/* 0x004d2710: a new unit of ours goes on its list (each value once):
   class 4 list 0; classes 0, 0x18, 6, 0x19, 0x1a, 0xc, 0x23, 0x17, 0x24,
   0x1c, 0x12, 0xd, 0x27 list 2; boats (2, 0x14, 0x16, 0x15) list 4 and
   also class 2 list 7, 0x14 list 8, 0x16 list 5, 0x15 list 6; type 0x9f
   list 9 while the world's +0x100 is below 4. */
void aoe_ai_tactical_unit_added_ror(AoeAiTacticalRor *ai,
                                    const AoeAiWorldObjectRor *unit,
                                    const AoeAiTacticalOpsRor *ops);
/* 0x004d2a30: a lost unit is dropped (0x004dc570), then taken off list 0,
   else list 2, else list 4 and then the first of lists 5, 6, 7 and 8
   holding it, else list 9 (presence searched over the count, removal
   over the capacity), and out of its group (0x004e0930). */
void aoe_ai_tactical_unit_removed_ror(AoeAiTacticalRor *ai, int32_t unit,
                                      const AoeAiTacticalOpsRor *ops,
                                      const AoeAiUnitGroupOpsRor *group_ops);
/* 0x004d8960: list 2's units that exist with a unit AI and are not on
   +0xd58, by their AI's task and +0x2c against `n`: `mode` below 0 takes
   those without a task (-1) or with +0x2c below `n`, 0 those with a task
   and +0x2c equal to `n`, above 0 those with a task and +0x2c above. Taken units go on +0xd48 once (emptied first
   when `reset` is 1); returns how many were taken (repeats counted). */
int32_t aoe_ai_tactical_collect_by_order_ror(AoeAiTacticalRor *ai, int32_t n,
                                            int32_t mode, int32_t reset,
                                            const AoeAiTacticalOpsRor *ops);
/* 0x004d8be0: the same test (+0xd48 always emptied first) over list 5,
   then over list 2's priests and siege (class 0x12, 0xd). */
int32_t aoe_ai_tactical_collect_support_ror(AoeAiTacticalRor *ai, int32_t n,
                                            int32_t mode,
                                            const AoeAiTacticalOpsRor *ops);
/* 0x004d90b0: an idle villager (no AI task and no action): first one of
   list 0 not on +0xd58, with a unit AI, not a gatherer (0x004e1a10) and
   not on list 1; failing that, by `mode`: 0 any, 3 not on list 1, 1 not
   a gatherer, others neither. Its ID, else -1. */
int32_t aoe_ai_tactical_idle_villager_ror(AoeAiTacticalRor *ai, int32_t mode,
                                          const AoeAiTacticalOpsRor *ops);
/* 0x004d93a0: like 0x004d90b0's first pass; failing it, among the idle
   villagers allowed by `mode`, the nearest to `pt` (`nearest` 1) or the
   farthest (0) by distance from its height (the first on ties). -1 for
   none. */
int32_t aoe_ai_tactical_villager_near_ror(AoeAiTacticalRor *ai, int32_t mode,
                                          const int32_t pt[2], int32_t nearest,
                                          const AoeAiTacticalOpsRor *ops);
/* 0x004d99f0: the gathering order. The pending build costs (up to number
   147 items) fill the counters (+0xda8); each resource's surplus
   (+0xd98) is its amount less that cost. Four times the resource with
   the smallest surplus is picked among those not yet picked, with a cost
   and an amount below its number 190..193; failing that among all left.
   +0xd88 lists them in order. */
void aoe_ai_tactical_gather_order_ror(AoeAiTacticalRor *ai,
                                      const AoeAiTacticalOpsRor *ops);
/* 0x004d9be0: gatherer planning. +0xd68 counts, per resource kind 0..3,
   the gatherers (own objects with a unit AI) whose AI task is 0x261 or
   0x265, or that are on +0xd58; each entry's +0x10 takes its AI's target,
   and a farmer (type 0x103, task 0x261) of another kind not on +0xd58 is
   moved to kind 0 (counted under its old kind this time). The wanted
   split +0xd78 of +0x9d0 gatherers: in age 1 with number 181 set, all on
   food when age 2 is open, food has a known target and wood is above 120
   or +0xf8c is 1 (others then get +8 = 5; +0xfa4 1), else all on wood
   (+0xfa4 0); in age 2 with number 182, all on food when age 3 is open
   and wood is above 240 or above 120 with the food target within 20,
   else all on wood; in age 3 with number 183, without age 4 open a third
   each on food, wood and gold (+0xfa4 0), with it half on food and gold
   (others than stone get +8 = 5; +0xfa4 1). Otherwise (+0xfa4 0 unless no
   age matched) by numbers: food 156 + 117, wood 157 + 120, stone 158 +
   119, gold 159 + 118, percent of +0x9d0. Then, when some surplus
   (+0xd98) is negative, the first that many kinds of +0xd88 lose surplus
   x (+0x9d0 - the total) / the summed shortage; what is left of +0x9d0
   goes half (rounded up) to food and half to wood. */
void aoe_ai_tactical_plan_gatherers_ror(AoeAiTacticalRor *ai,
                                        const AoeAiTacticalOpsRor *ops);
/* 0x004da710: `count` waypoints (16 bytes apart, x then y) go to the
   player for the unit (0x004e1570 with the truncated x and y) when given;
   the unit is marked tasked (+0xd58, once) and the tribe player's vtable
   +0x4c gets it with `a`..`d`. 1. */
int aoe_ai_tactical_task_unit_ror(AoeAiTacticalRor *ai, int32_t unit, int32_t a,
                                  int32_t b, int32_t c, int32_t d,
                                  const float *waypoints, int32_t count,
                                  const AoeAiTacticalOpsRor *ops);
/* 0x004da830: start building `item` (NULL: the Build AI's next; none
   gives 0) at `site` (NULL: the ConAI's choice for it and `builder`; none
   gives 0) with `builder` (-1: the nearest list-0 unit to the site's
   truncated tile not doing 0x2be, 0x2ce or 0x2bc, 0x004d9760). A builder
   that exists with a unit AI and passes the path test to the site
   (radius 2.0) is used: the item becomes in progress and not built, the
   builder leaves list 1 and is marked tasked, a wonder (type 0x114) sets
   +0xe44, and the build order (type, x, y, the item's unique ID) goes
   out; 1. The site is deleted in every case that reaches it. */
int aoe_ai_tactical_start_build_ror(AoeAiTacticalRor *ai, int32_t builder,
                                    AoeAiBuildItemRor *item, AoeAiConItemRor *site,
                                    int32_t arg, const AoeAiTacticalOpsRor *ops);
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

/* 0x004e0d80(grid): the information AI's site grid (mode 0) avoids
   our forces: each group with a location (+0x19c x not -1) blocks a box
   of half the side of its +0x160's area (0x004c2dc0: 2 below 2, else the
   square) around it and lowers the full-sized box by 10; each unit of
   list 3 (+0x124) blocks its tile and lowers the box 2 around it by 10.
   Boxes are clamped to the map (the information AI's 0x004c2d60). */
struct AoeValueGridRor;
void aoe_ai_tactical_block_sites_ror(const AoeAiTacticalRor *ai, const AoeAiInfoRor *info,
                                     struct AoeValueGridRor *grid, const AoeAiInfoOpsRor *ops);

#endif
