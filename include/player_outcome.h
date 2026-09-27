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
};

/* Original manager: condition list +4, outcome byte +0x10,
   event list +0x14. This is not the packed 32-bit PC ABI. */
struct AoeOutcomeManager {
    AoeOutcomeCondition *conditions;
    AoeOutcomeEvent *events;
    uint8_t outcome;
    AoeOutcomePlayerRef *owner; /* Original manager +0xc. */
    uint32_t score_sum_1c; /* Original manager +0x1c, wraps at 32 bits. */
};

typedef struct {
    void *user;
    /* Manager virtual +0 and +4, supplied by the translated dispatcher. */
    int (*update_event)(void *user, AoeOutcomeEvent *event);
    int (*update_condition)(void *user, AoeOutcomeCondition *condition);
} AoeOutcomeOps;

/* RoR 0x005100e0 / base 0x004337d0: group conditions after their updates.
   If no conditions exist, original code leaves the previous outcome alone. */
int aoe_outcome_recompute(AoeOutcomeManager *manager);

/* RoR 0x005101c0 / base 0x004338c0: update status-non-1/non-3 entries of both
   lists, then recompute and return the outcome byte. Returns zero on invalid
   state or callback failure; result receives the game outcome on success. */
int aoe_outcome_manager_update(AoeOutcomeManager *manager,
                               const AoeOutcomeOps *ops, uint8_t *result);

#endif
