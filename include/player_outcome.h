#ifndef VITA_AOE_PLAYER_OUTCOME_H
#define VITA_AOE_PLAYER_OUTCOME_H

#include <stddef.h>
#include <stdint.h>

typedef struct AoeOutcomeUnitRef AoeOutcomeUnitRef;
typedef struct AoeOutcomePlayerRef AoeOutcomePlayerRef;
typedef struct AoeOutcomeManager AoeOutcomeManager;

typedef struct AoeOutcomeProgressRef {
    int32_t numerator_1c;
    int32_t denominator_20;
} AoeOutcomeProgressRef;

typedef struct AoeOutcomeUnitNode {
    AoeOutcomeUnitRef *unit;
    struct AoeOutcomeUnitNode *next;
} AoeOutcomeUnitNode;

typedef struct AoeOutcomeWorldRef {
    int16_t player_count;
    AoeOutcomePlayerRef *const *players;
    size_t player_capacity;
} AoeOutcomeWorldRef;

/* Semantic event fields from original +0 (kind), +1 (status),
   +4/+8/+0xc/+0x10/+0x14 (resource events 0/4), and +0x18 (next). */
typedef struct AoeOutcomeEvent {
    uint8_t kind;
    uint8_t status;
    uint8_t index_02;    /* +0x02: 0x0050ea40 first argument */
    uint8_t category_03; /* +0x03: 0x0050ea40 second argument */
    int32_t resource_index_04;
    union {
        int32_t divisor_08; /* Kinds 0/4. */
        int32_t target_08; /* Kinds 1/2. */
        int32_t rival_threshold_08; /* Kind 3, raised during updates. */
    };
    int32_t weight_0c;
    uint32_t contribution_10;
    float sample_14;
    struct AoeOutcomeEvent *next;
} AoeOutcomeEvent;

/* Semantic condition fields from +0 (kind), +0x2c (group),
   +0x2e (status), +0x2f (visited), and +0x30 (next). */
typedef struct AoeOutcomeCondition {
    uint8_t kind;
    uint8_t group;
    uint8_t status;
    uint8_t visited;
    int32_t unit_class_04;
    union {
        int32_t threshold_1c; /* Kind 3: signed target count. */
        int32_t resource_index_1c; /* Kind 6: resource array index. */
        int32_t unit_target_count_1c; /* Kinds 1/8: signed unit count. */
    };
    union {
        uint32_t counter_20; /* Kind 3: accumulating 32-bit count. */
        int32_t resource_target_20; /* Kind 6: signed target amount. */
        int32_t score_target_20; /* Kind 12: team-average score target. */
        int32_t progress_target_20; /* Kind 7: percentage target. */
    };
    uint8_t include_allies_2d; /* Original condition +0x2d. */
    /* Selected original references at +0x28, +0x24, and +8. */
    struct AoeOutcomeUnitRef *unit_28;
    struct AoeOutcomeUnitRef *unit_24;
    struct AoeOutcomePlayerRef *player_08;
    float min_x, max_x, min_y, max_y; /* Original +0xc/+0x14/+0x10/+0x18. */
    struct AoeOutcomeCondition *next;
} AoeOutcomeCondition;

struct AoeOutcomeUnitRef {
    int32_t id04;           /* Original unit +4, the saved reference. */
    const char *type_name;  /* Unit type +8, then its name pointer +8. */
    uint8_t code48;
    int16_t class12; /* Original unit type at unit +8, then +0x12. */
    AoeOutcomePlayerRef *owner0c; /* Original unit +0xc. */
    float x38, y3c;
    /* Original unit-type pointer +8 then float extents +0x30/+0x34. */
    float extent_x30, extent_y34;
};

struct AoeOutcomePlayerRef {
    uint8_t state80;
    uint8_t state_lock81;
    int16_t index4a;
    AoeOutcomeWorldRef *world3c;
    const uint8_t *eligible84;
    size_t eligible_capacity;
    AoeOutcomeUnitNode *units28;
    const float *resources50;
    size_t resource_capacity;
    uint8_t team_mode; /* Original +0x108 RoR / +0x104 base. */
    AoeOutcomeManager *outcome34; /* Original player +0x34. */
    AoeOutcomeProgressRef *progress38; /* Original player +0x38. */
    /* Player +0x24 unit types, each type's name (+8); NULL entries allowed. */
    const char *const *type_names;
    size_t type_count;
};

/* Original manager: condition list +4, outcome byte +0x10,
   event list +0x14. This is not the packed 32-bit PC ABI. */
struct AoeOutcomeManager {
    AoeOutcomeCondition *conditions;
    AoeOutcomeEvent *events;
    uint8_t outcome;
    AoeOutcomePlayerRef *owner; /* Original manager +0xc. */
    uint32_t score_sum_1c; /* Original manager +0x1c, wraps at 32 bits. */
    uint32_t event_count;  /* Original manager +0x18. */
    uint32_t condition_count; /* Original manager +8. */
};

/* Event adders 0x0050ead0/0x0050eb10/0x0050eb50/0x0050eb90/0x0050ebd0
   (kinds 0..4) over the allocator 0x0050ea40: a zeroed 0x1c-byte event
   (resource -1, kind 0xff) gets +2 = index and +3 = category, is pushed
   to the front of the event list (+0x14) and counted (+0x18); the adder
   then sets the kind, +4 resource, +8 and +0xc. Returns NULL on allocation
   failure (the original's AL 0). */
AoeOutcomeEvent *aoe_outcome_event_add_ror(AoeOutcomeManager *manager,
                                           uint8_t kind, uint8_t category,
                                           uint8_t index, int32_t resource,
                                           int32_t field_08,
                                           int32_t weight_0c);
void aoe_outcome_events_free_ror(AoeOutcomeManager *manager);

typedef struct {
    void *user;
    /* Manager virtual +0 and +4, supplied by the translated dispatcher. */
    int (*update_event)(void *user, AoeOutcomeEvent *event);
    int (*update_condition)(void *user, AoeOutcomeCondition *condition);
} AoeOutcomeOps;

/* RoR 0x005100e0 (patched 1.0a 0x0050d950) / base 0x004337d0: group
   conditions after their updates.
   If no conditions exist, original code leaves the previous outcome alone. */
int aoe_outcome_recompute(AoeOutcomeManager *manager);

/* RoR 0x005101c0 (patched 1.0a 0x0050da30) / base 0x004338c0: update
   status-non-1/non-3 entries of both
   lists, then recompute and return the outcome byte. Returns zero on invalid
   state or callback failure; result receives the game outcome on success. */
int aoe_outcome_manager_update(AoeOutcomeManager *manager,
                               const AoeOutcomeOps *ops, uint8_t *result);

/* Condition allocator 0x0050e4b0: a zeroed 0x34-byte condition with
   status 0, group +0x2c, allies flag +0x2d and +0x20 = 0 is pushed to the
   front of the list (+4) and counted (+8). NULL on allocation failure. */
AoeOutcomeCondition *aoe_outcome_condition_new_ror(AoeOutcomeManager *manager,
                                                   uint8_t group,
                                                   uint8_t include_allies);
/* Typed adders; each returns 1, or 0 when the allocation fails:
   - kind 0 0x0050e4f0 (unit +0x28, its owner +8; allies flag);
   - kind 1 0x0050e520 (unit class +4, count +0x1c; allies flag);
   - kind 8 0x0050e550 (kind 1 in the area +0xc/+0x10 - +0x14/+0x18);
   - kind 2 0x0050e5a0 (unit +0x28, its owner +8);
   - kind 3 0x0050e5d0 (class +4, count +0x1c, player +8);
   - kind 9 0x0050e610 (class +4, player +8);
   - kind 10 0x0050e640 (player +8);
   - kind 5 0x0050e670 (unit +0x24 to unit +0x28; owner of +0x28 at +8);
   - kind 4 0x0050e6b0 (unit +0x24 to the area);
   - kind 6 0x0050e6f0 (attribute +0x1c reaches +0x20; allies flag);
   - kind 7 0x0050e720 (explore +0x20 percent, clamped 1..100; allies);
   - kind 12 0x0050e770 (score +0x20; allies flag).
   Kinds 2, 3, 4, 5, 9 and 10 clear the allies flag. */
int aoe_outcome_add_destroy_unit_ror(AoeOutcomeManager *manager, uint8_t group,
                                     AoeOutcomeUnitRef *unit, uint8_t allies);
int aoe_outcome_add_create_ror(AoeOutcomeManager *manager, uint8_t group,
                               int32_t unit_class, int32_t count,
                               uint8_t allies);
int aoe_outcome_add_create_in_area_ror(AoeOutcomeManager *manager,
                                       uint8_t group, int32_t unit_class,
                                       int32_t count, float x0, float y0,
                                       float x1, float y1, uint8_t allies);
int aoe_outcome_add_capture_ror(AoeOutcomeManager *manager, uint8_t group,
                                AoeOutcomeUnitRef *unit);
int aoe_outcome_add_destroy_count_ror(AoeOutcomeManager *manager,
                                      uint8_t group, int32_t unit_class,
                                      int32_t count,
                                      AoeOutcomePlayerRef *player);
int aoe_outcome_add_destroy_all_ror(AoeOutcomeManager *manager, uint8_t group,
                                    int32_t unit_class,
                                    AoeOutcomePlayerRef *player);
int aoe_outcome_add_defeat_player_ror(AoeOutcomeManager *manager,
                                      uint8_t group,
                                      AoeOutcomePlayerRef *player);
int aoe_outcome_add_bring_to_unit_ror(AoeOutcomeManager *manager,
                                      uint8_t group, AoeOutcomeUnitRef *unit,
                                      AoeOutcomeUnitRef *destination);
int aoe_outcome_add_bring_to_area_ror(AoeOutcomeManager *manager,
                                      uint8_t group, AoeOutcomeUnitRef *unit,
                                      float x0, float y0, float x1, float y1);
int aoe_outcome_add_attribute_ror(AoeOutcomeManager *manager, uint8_t group,
                                  int32_t attribute, int32_t amount,
                                  uint8_t allies);
int aoe_outcome_add_explore_ror(AoeOutcomeManager *manager, uint8_t group,
                                int32_t percent, uint8_t allies);
int aoe_outcome_add_score_ror(AoeOutcomeManager *manager, uint8_t group,
                              int32_t score, uint8_t allies);
/* 0x00508570: kind 100 with +0x20 = value, allies flag 0 (scenario
   condition type 7). The dispatcher treats kind 100 as a no-op. */
int aoe_outcome_add_kind100_ror(AoeOutcomeManager *manager, uint8_t group,
                                int32_t value);

/* 0x0050e7a0: unlink, free and uncount one condition (absent: no-op);
   0x0050e7e0: remove them all; 0x0050da90: remove every condition whose
   +0x24 or +0x28 unit is `unit`. */
void aoe_outcome_condition_remove_ror(AoeOutcomeManager *manager,
                                      AoeOutcomeCondition *condition);
void aoe_outcome_conditions_clear_ror(AoeOutcomeManager *manager);
void aoe_outcome_conditions_drop_unit_ror(AoeOutcomeManager *manager,
                                          const AoeOutcomeUnitRef *unit);
/* 0x0050ea90: unlink, free and uncount one event. */
void aoe_outcome_event_remove_ror(AoeOutcomeManager *manager,
                                  AoeOutcomeEvent *event);
/* 0x0050d700: the destructor's list release; also zeroes the counts, the
   outcome byte and the owner. */
void aoe_outcome_manager_release_ror(AoeOutcomeManager *manager);

/* 0x0050ea10: the condition at list position `index` (NULL past the end). */
AoeOutcomeCondition *aoe_outcome_condition_at_ror(
    const AoeOutcomeManager *manager, int32_t index);
/* 0x0050ea30: outcome byte +0x10 equals 2 (won). */
int aoe_outcome_won_ror(const AoeOutcomeManager *manager);
/* 0x0050ef70: the sum of event contributions (+0x10) whose category (+3)
   is `category`, or all of them for -1. */
int32_t aoe_outcome_points_ror(const AoeOutcomeManager *manager,
                               int8_t category);
/* 0x0050efa0 / 0x0050efc0: the contribution (+0x10) / truncated sample
   (+0x14) of the last event in the list whose index byte (+2) matches;
   0 when none does. */
int32_t aoe_outcome_event_points_ror(const AoeOutcomeManager *manager,
                                     int8_t index);
int32_t aoe_outcome_event_sample_ror(const AoeOutcomeManager *manager,
                                     int8_t index);

/* 0x0050e800: the debug text of a condition, written to `out`:
   kind 0 "Capture %s" (unit +0x24 type name), 1 "Create %d %s",
   2 "Destroy %s" (+0x24), 3 "Destroy %d %s of player %d", 4 "Bring %s to
   area (%f, %f) - (%f, %f)", 5 "Bring %s to %s", 6 "aqquire %d of
   attribute %d" (+0x20, +0x1c), 7 "explore %d percent of the map",
   8 "Create %d %s in area (%f, %f) - (%f, %f)", 9 "Destroy all %s of
   player %d". Class names come from player +8's type table (+0x24[+4]).
   Other kinds leave the original's buffer unwritten; here they give "".
   A missing unit, player or name (the original dereferences it; kind 0
   reads +0x24 although its adder fills +0x28) gives "". */
int aoe_outcome_condition_text_ror(const AoeOutcomeCondition *condition,
                                   char *out, size_t size);

/* Decompressed save bytes, little-endian and unaligned as the original
   stream read/write calls 0x0046b800 / 0x0046ba40 see them. */
typedef struct {
    uint8_t *data;
    size_t size;
    size_t pos;
    int failed; /* Set by a short read or a full write buffer. */
} AoeOutcomeStream;

/* Save 0x0050d750: f32 version 1.0, u32 condition count, u8 outcome; per
   condition (list order) u8 kind, i32 class, i32 player index (+8's +0x4a
   or -1; replaced by the owner of +0x28 when that unit is set), f32 x0, y0,
   x1, y1, i32 +0x1c, +0x20, i32 unit +0x24 ID and +0x28 ID (-1 for none),
   u8 group, allies, status; then i32 points sum (+0x1c), u32 event count
   and per event u8 kind, status, i32 +4, +8, +0xc, +0x10, u8 index,
   category, f32 sample. Returns 0 if the stream fills up. */
int aoe_outcome_manager_save_ror(const AoeOutcomeManager *manager,
                                 AoeOutcomeStream *out);

/* Constructor 0x0050d6d0 (owner +0xc, every other field 0) and load
   constructor 0x0050d380 (the derived vtable 0x0054a3e4 one, 0x005084c0,
   wraps it). `owner` is +0xc; `has_version` reads the
   leading f32 (otherwise 0.0). Conditions keep their saved order. Unit
   +0x24 is looked up in each world player's list until found, unit +0x28
   in the owner's list, and when that misses, in the saved player's list;
   the player index passes through `player_map` when given. A missing
   unit or a negative mapped player sets status 3. Events are read when
   the version is at least 0.0; a negative points sum zeroes each event's
   weight (+0xc) and is itself stored as 0. Returns 0 for a short stream
   or allocation failure (the manager is then released). */
typedef AoeOutcomeUnitRef *(*AoeOutcomeFindUnitFn)(void *context,
                                                   AoeOutcomePlayerRef *player,
                                                   int32_t id); /* 0x00451340 */
int aoe_outcome_manager_load_ror(AoeOutcomeManager *manager,
                                 AoeOutcomePlayerRef *owner,
                                 AoeOutcomeStream *in, int has_version,
                                 const int32_t *player_map,
                                 size_t player_map_count,
                                 AoeOutcomeFindUnitFn find_unit,
                                 void *context);

#endif
