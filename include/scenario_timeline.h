#ifndef VITA_AOE_SCENARIO_TIMELINE_H
#define VITA_AOE_SCENARIO_TIMELINE_H

#include <stdint.h>

/* Semantic form of the 48-byte timed-action node used by RoR 0x004ee2b0
   and base 0x00512a90. This is not the original 32-bit object ABI. */
typedef struct AoeScenarioAction {
    float time_s;             /* Original +0x00. */
    uint8_t kind;             /* +0x04: 0, 1, 2, or 4 have actions. */
    uint8_t reserved_05;
    int16_t template_index;   /* +0x06, used by kind 1. */
    uint8_t player_index;     /* +0x08, used by kind 1. */
    uint8_t reserved_09[3];
    float arg_a;              /* +0x0c. */
    float arg_b;              /* +0x10. */
    float arg_c;              /* +0x14. */
    uint32_t reserved_18;
    void *target_a;           /* +0x1c. */
    int16_t target_a_id;      /* +0x20. */
    uint16_t reserved_22;
    void *target_b;           /* +0x24. */
    int16_t target_b_id;      /* +0x28. */
    uint16_t reserved_2a;
    struct AoeScenarioAction *next; /* +0x2c. */
} AoeScenarioAction;

typedef struct {
    int attached;             /* Original controller +0x04 world pointer. */
    AoeScenarioAction *head;  /* Original controller +0x08. */
    float last_time_s;        /* Original controller +0x10. */
} AoeScenarioTimeline;

typedef struct {
    void *user;
    /* Byte at live object +0x48. Values >= 8 invalidate a reference. */
    uint8_t (*object_class)(void *user, void *object);
    /* Original kind 0: object virtual +0xa8 (RoR), +0xa0 (base). */
    void (*action_pair)(void *user, void *target_a, void *target_b);
    /* Original kind 1: resolve player/template then virtual +0x18.
       Return zero when the template slot is absent. A present slot may
       produce a null object, which still updates matching references. */
    int (*spawn)(void *user, uint8_t player_index, int16_t template_index,
                 float arg_a, float arg_b, void **created);
    /* Original kind 2: object virtual +0x9c (RoR), +0x94 (base). */
    void (*action_position)(void *user, void *target_a,
                            float arg_a, float arg_b, float arg_c);
    /* Original kind 4: object virtual +0x68 (RoR), +0x64 (base). */
    void (*action_simple)(void *user, void *target_a);
} AoeScenarioTimelineOps;

/* On an attached timeline, convert game_time_ms to float seconds with the
   original 0.001f multiplier without prematurely rounding milliseconds,
   clean
   references on every node, process due nodes after last_time_s, then set
   last_time_s to the new time. Returns zero for missing dependencies. */
int aoe_scenario_timeline_step(AoeScenarioTimeline *timeline,
                               uint32_t game_time_ms,
                               const AoeScenarioTimelineOps *ops);

#endif
