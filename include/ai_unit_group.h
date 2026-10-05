#ifndef VITA_AOE_AI_UNIT_GROUP_H
#define VITA_AOE_AI_UNIT_GROUP_H

#include <stdint.h>

#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a tactical-AI unit group (vtable 0x00548ce0,
   0x004cc630..0x004cd2c0): up to 40 member unit IDs (-1 free) with a value
   each, an optional commander that the others follow, three-float
   locations and the group's state. Semantic storage; the comments give
   the original offsets. */
enum { AOE_AI_GROUP_SLOTS = 40 };

typedef struct {
    int32_t id;                                /* +0x0c, -1 */
    int32_t type;                              /* +0x10 */
    int32_t task;                              /* +0x14, -1 */
    int32_t field_18;                          /* +0x18, -1 */
    int32_t members[AOE_AI_GROUP_SLOTS];       /* +0x1c */
    int32_t values[AOE_AI_GROUP_SLOTS];        /* +0xbc */
    int32_t count;                             /* +0x15c */
    int32_t field_160;                         /* +0x160 */
    int32_t commander;                         /* +0x164, -1 */
    int32_t field_168, field_16c;              /* -1, -1 */
    float location[4];                         /* +0x170 (x, y, z, pad) */
    int32_t field_180, field_184, field_188;   /* 0, -1, -1 */
    float loc_18c[4], loc_19c[4], loc_1ac[4];
    int32_t field_1bc, field_1c0, field_1c4, field_1c8; /* 0, -1, -1, -1 */
    int32_t field_1cc, field_1d0;              /* 0, -1 */
    uint8_t flag_1d4, flag_1d5;
    uint8_t block_1d8[0xf0];
    uint8_t flag_2c8;                          /* version 6.94+ */
    int32_t field_2cc;                         /* 7.13+ */
    int32_t list_2d0[20];                      /* 7.13+, -1 */
    int32_t field_320, field_324;              /* 7.13+, -1 */
    int32_t field_328;                         /* 7.14+ */
    int32_t added_time;                        /* +0x32c, 7.20+ */
} AoeAiUnitGroupRor;

/* The tactical AI's world and player hooks. The player calls are its
   vtable +0xc4 (lead: the commander takes the members, range 4.0), +0xc8
   (follow: one more unit joins the commander, range 4.0), +0xcc (unfollow)
   and +0xd0 (release the commander). */
typedef struct {
    void *context;
    void (*lead)(void *context, int32_t commander, const int32_t *ids,
                 int32_t count, float range);
    void (*follow)(void *context, int32_t commander, int32_t id, float range);
    void (*unfollow)(void *context, int32_t commander, int32_t id);
    void (*release)(void *context, int32_t commander);
    int32_t (*game_time)(void *context); /* world +0x04 */
    /* 0x0040bab0: the world object exists and its +0x20 is set. */
    int (*present)(void *context, int32_t id);
    /* The object's hit points (+0x30); 0 when there is no object. */
    int (*hit_points)(void *context, int32_t id, float *out);
    /* Order the unit to stop (the player's +0x30 task with
       action 0x2c2 and the given value). */
    void (*dismiss)(void *context, int32_t id, int32_t value);
    /* The unit's distance to a point (0x004aaf50 on the world object:
       |dx| and |dy| less the type's +0x30 / +0x34 sizes, not below 0,
       then the root of the squares). */
    double (*distance)(void *context, int32_t id, float x, float y, float z);
    /* The world object and its unit AI (+0x74): the type ID (type +0x10),
       the AI's task (+0x30) and action (+0x28). 0 when there is no object
       (0x0040bab0) or it has no unit AI. */
    int (*unit_ai)(void *context, int32_t id, int32_t *type_id,
                   int32_t *task, int32_t *action);
    /* The tactical AI's strategic number (0x004d77d0). */
    int32_t (*sn)(void *context, int32_t index);
    /* Hooks of the group update (0x004cd2d0). The object's position
       (+0x38..+0x40); its type's float at +0x28 (0 when there is no
       object). */
    int (*position)(void *context, int32_t id, float out[3]);
    float (*type_28)(void *context, int32_t id);
    /* The unit AI's +0x38 (read when its task is 600). */
    int32_t (*unit_ai_38)(void *context, int32_t id);
    /* The object's cached map zone byte (+0x7c, filled on first use), and
       the zone byte at a tile as seen by that object (0x004ab9e0). */
    int32_t (*zone)(void *context, int32_t id);
    int32_t (*zone_at)(void *context, int32_t id, int32_t x, int32_t y);
    /* The tactical AI's orders: 0x004da160 move, 0x004db9f0 action 0x2bd
       on a target, 0x004da1b0 action 0x2d4, 0x004dc650 effort. */
    void (*order_move)(void *context, int32_t id, float x, float y,
                       int32_t value);
    void (*order_2bd)(void *context, int32_t id, int32_t target, float d,
                      int32_t value);
    void (*order_2d4)(void *context, int32_t id, int32_t target, float x,
                      float y, float z, float d, int32_t value);
    int32_t (*effort)(void *context, float f);
    /* Not yet translated tactical orders: attack a target at (x, y) with
       the target owner, the group's waypoint block and count, the
       commander and a round-up flag; the "play" attack of every member;
       a unit order taking 0 (task 8, commanders of task 7); commander
       orders of tasks 10 (target, x, y) and 0xb (target). */
    void (*order_attack)(void *context, int32_t id, float x, float y,
                         int32_t target, int32_t owner,
                         const uint8_t *waypoints, uint8_t count,
                         int32_t commander, int32_t roundup);
    void (*order_play)(void *context, int32_t commander, const int32_t *ids,
                       int32_t count, int32_t target, int32_t play,
                       const uint8_t *waypoints, uint8_t count_wp);
    void (*order_zero)(void *context, int32_t id, int32_t zero);
    void (*order_10)(void *context, int32_t id, int32_t target, float x,
                     float y);
    void (*order_11)(void *context, int32_t id, int32_t target);
    int32_t (*rand)(void *context); /* 0x0052605d */
    /* 0x004e15b0 on the commander. */
    int (*priest_or_siege)(void *context, int32_t id);
    /* 0x004c2d60 on the information AI (+0xcec), 0x004c3660 recording
       (x0, y0, x1, y1) for our player byte and the target owner's byte at
       time `t`, then 0x004e0c00. */
    void (*clamp_tile)(void *context, int32_t tile[2]);
    void (*record_area)(void *context, int32_t x0, int32_t y0, int32_t x1,
                        int32_t y1, int32_t owner, int32_t t);
    void (*roll_d24)(void *context);
    /* The object's owner (its player's +0x4a, a short). */
    int32_t (*owner)(void *context, int32_t id);
} AoeAiUnitGroupOpsRor;

/* 0x004cc630 constructor (0x004cc780 / 0x004cc760 destructors). */
void aoe_ai_unit_group_init_ror(AoeAiUnitGroupRor *g);
/* 0x004ccc90: the first free slot takes the unit (value -1); a commander
   gets a follower; the time is recorded. 0 when the group is full. */
int aoe_ai_unit_group_add_ror(AoeAiUnitGroupRor *g, int32_t id,
                              const AoeAiUnitGroupOpsRor *ops);
/* 0x004ccd10 by ID, 0x004ccd90 by slot: removing the commander releases
   it; anyone else stops following. */
int aoe_ai_unit_group_remove_ror(AoeAiUnitGroupRor *g, int32_t id,
                                 const AoeAiUnitGroupOpsRor *ops);
int aoe_ai_unit_group_remove_slot_ror(AoeAiUnitGroupRor *g, int32_t slot,
                                      const AoeAiUnitGroupOpsRor *ops);
/* 0x004cce10: release the commander and empty every slot. */
void aoe_ai_unit_group_clear_ror(AoeAiUnitGroupRor *g,
                                 const AoeAiUnitGroupOpsRor *ops);
/* 0x004cce50: drop (and dismiss with 100) members that are gone. */
int aoe_ai_unit_group_prune_ror(AoeAiUnitGroupRor *g,
                                const AoeAiUnitGroupOpsRor *ops);
/* 0x004ccef0 contains, 0x004ccf20 count, 0x004ccf30 / 0x004ccf70 the
   n-th occupied member and its value (-1 past the end). */
int aoe_ai_unit_group_contains_ror(const AoeAiUnitGroupRor *g, int32_t id);
int32_t aoe_ai_unit_group_count_ror(const AoeAiUnitGroupRor *g);
int32_t aoe_ai_unit_group_member_ror(const AoeAiUnitGroupRor *g, int32_t n);
int32_t aoe_ai_unit_group_value_ror(const AoeAiUnitGroupRor *g, int32_t n);
/* 0x004ccfe0: a new commander (the old one is released; the new one
   leads the occupied members in slot order). 0x004ccfd0 reads it. */
void aoe_ai_unit_group_set_commander_ror(AoeAiUnitGroupRor *g, int32_t id,
                                         const AoeAiUnitGroupOpsRor *ops);
/* 0x004cd070: the members' hit points, each truncated (__ftol), summed. */
int32_t aoe_ai_unit_group_hit_points_ror(const AoeAiUnitGroupRor *g,
                                         const AoeAiUnitGroupOpsRor *ops);
/* 0x004cd170: the location, and the +0x19c and +0x1ac ones too. */
void aoe_ai_unit_group_set_all_locations_ror(AoeAiUnitGroupRor *g, float x,
                                             float y, float z);
/* Field accessors: 0x004ccc50 id, 0x004ccc60 type, 0x004ccc70 task,
   0x004ccc80 +0x18, 0x004ccfb0 / 0x004ccfc0 +0x160, 0x004cd0b0 +0x168,
   0x004cd0c0 +0x16c, 0x004cd0d0 location, 0x004cd100 / 0x004cd110 +0x180,
   0x004cd120 / 0x004cd130 +0x184, 0x004cd140 / 0x004cd150 +0x188,
   0x004cd160 / 0x004cd1d0 +0x18c, 0x004cd200 / 0x004cd210 +0x19c,
   0x004cd240 / 0x004cd250 +0x1ac, 0x004cd280 / 0x004cd290 +0x1c0,
   0x004cd2a0 / 0x004cd2b0 +0x1c4, 0x004cd2c0 +0x1c8 (plain fields). */
void aoe_ai_unit_group_set_location_ror(float out[4], float x, float y,
                                        float z);

/* 0x004ce950: every member near the +0x19c location: a missing member
   (or one without a unit AI) is dropped by slot (0x004ccd90); otherwise
   its distance less 1.0 (not below 0, kept as a float) must not exceed
   strategic number 23 for task 0x6d, else 41. 0 at the first one too
   far, else 1. */
int aoe_ai_unit_group_in_range_ror(AoeAiUnitGroupRor *g,
                                   const AoeAiUnitGroupOpsRor *ops);
/* 0x004cea40: every member settled: missing members are dropped; type
   0x7d is skipped; with `allow_600` set, a unit AI task of 600 is skipped
   for group tasks 100 and 0x67; any other member with a task other than
   -1 must have action 0x2bd. 0 at the first that is not, else 1. */
int aoe_ai_unit_group_settled_ror(AoeAiUnitGroupRor *g,
                                  const AoeAiUnitGroupOpsRor *ops,
                                  int32_t allow_600);
/* 0x004ceb10 / 0x004ceb20: +0x1cc. */
int32_t aoe_ai_unit_group_field_1cc_ror(const AoeAiUnitGroupRor *g);
void aoe_ai_unit_group_set_field_1cc_ror(AoeAiUnitGroupRor *g, int32_t v);
/* 0x004ceb30: append a waypoint (x, y, 1.0) to the 16-byte entries at
   +0x1d8 while fewer than 14 are stored (+0x1d4 counts them). The entries
   stay little-endian bytes so the save writes them unchanged. */
void aoe_ai_unit_group_add_waypoint_ror(AoeAiUnitGroupRor *g, float x,
                                        float y);
/* Waypoint `i` as written above (the port's reader; 0 outside). */
int aoe_ai_unit_group_waypoint_ror(const AoeAiUnitGroupRor *g, int i,
                                   float out[3]);
/* 0x004ceb90: entry `i` (0..19) of the +0x2d0 list, else -1; 0x004cebb0
   appends while +0x2cc is below 19 (1), else 0. */
int32_t aoe_ai_unit_group_list_ror(const AoeAiUnitGroupRor *g, int32_t i);
int aoe_ai_unit_group_list_add_ror(AoeAiUnitGroupRor *g, int32_t value);

/* 0x004cca20 save / 0x004cc790 load: i32 id, type, 40 pairs (member,
   value), +0x15c..+0x16c, 16-byte location, +0x180..+0x188, three 16-byte
   locations, +0x1bc..+0x1c8, task, +0x18, +0x1cc, two flag bytes, 0xf0
   bytes, +0x1d0; then by save version: u8 +0x2c8 (6.94; zero before), i32
   +0x2cc, 20 i32, +0x320, +0x324 (7.13), +0x328 (7.14), the time (7.20).
   The save always writes the newest layout. */
int aoe_ai_unit_group_save_ror(const AoeAiUnitGroupRor *g,
                               AoeOutcomeStream *out);
int aoe_ai_unit_group_load_ror(AoeAiUnitGroupRor *g, float version,
                               AoeOutcomeStream *in);

/* 0x004cd2d0: carry out group task `task` for every member (40 slots).
   Returns 0 at once without a commander. Every task but 9 and 0xe resets
   +0x2c8. A member is "gone" when its object or unit AI is missing; it is
   then dropped by slot (0x004ccd90). Live members store their truncated
   hit points as values; "busy" (only checked when `force` is 0) means a
   unit AI task other than -1. With `record` 1 the tasks that set the type
   copy the summed values and the count to +0x168 / +0x16c.
   0, 1: stop every member (100); type 0. 2, 0x15: attack (see the
   source); 3: retreat members farther than a third of their type's +0x28
   to +0x1ac; 4: defend; 5, 6, 0xc, 0x13: only values, type 1, returns 0;
   7, 10, 0xb: the commander is ordered, the others follow it; 8: the
   zero order; 9, 0xe: gather at +0x19c; 0x14: attack round-up at most
   every 10 seconds. Unknown tasks return 0. */
int aoe_ai_unit_group_update_ror(AoeAiUnitGroupRor *g,
                                 const AoeAiUnitGroupOpsRor *ops,
                                 int32_t task, int32_t record,
                                 int32_t force);

#endif
