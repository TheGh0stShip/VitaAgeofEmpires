#ifndef VITA_AOE_SCENARIO_LOAD_H
#define VITA_AOE_SCENARIO_LOAD_H

#include <stdint.h>

#include "aoe_stream.h"

/* Legacy-scenario world load family (EMPIRESX.EXE, RoR 1.0a). The world
   object's own virtuals, the per-player calls and the scenario info object
   are reached through this ops table. Player indices are world slots. */
typedef struct AoeScenarioLoadOps {
    void *ctx;
    void (*read_header)(void *ctx, AoeOutcomeStream *s);   /* world +0xf4 */
    void (*read_map)(void *ctx, AoeOutcomeStream *s);      /* world +0x70 */
    int (*player_present)(void *ctx, int player);   /* players[i] != 0 */
    void (*player_new_view)(void *ctx, int player);        /* 0x0045e950 */
    /* player +0x60: create an object at (x, y, z) of `type`. */
    void (*player_place)(void *ctx, int player, int32_t x, int32_t y,
                         int32_t z, int16_t type, uint8_t state,
                         int32_t extra);
    /* player +0x68 (stream, slot map, version scale). */
    void (*player_load)(void *ctx, int player, AoeOutcomeStream *s,
                        const int32_t map[9], float scale);
    void (*close)(void *ctx, AoeOutcomeStream *s);         /* 0x0046b6b0 */
    /* scenario info getters (info, player p, 1) */
    int32_t (*info_ai)(void *ctx, int p);          /* 0x00475b90 */
    int32_t (*info_city)(void *ctx, int p);        /* 0x00475d80 */
    int32_t (*info_personality)(void *ctx, int p); /* 0x00475f90 */
    int32_t (*info_kind)(void *ctx, int p);        /* 0x00476190 */
    /* player +0x2c (ai, city, personality, kind, -1). */
    void (*player_set_ai)(void *ctx, int player, int32_t ai, int32_t city,
                          int32_t per, int32_t kind, int32_t extra);
    /* player +0x70 (stream, slot map, version scale): variants 4..6 only
       (may be NULL for the others). */
    void (*player_load2)(void *ctx, int player, AoeOutcomeStream *s,
                         const int32_t map[9], float scale);
} AoeScenarioLoadOps;

typedef struct AoeScenarioLoadWorld {
    int16_t player_count;   /* +0x3c */
    int32_t header_value;   /* +0x68: the stream's first i32 */
    int32_t placing_id;     /* +0x70: set while a placement is in flight */
    uint8_t placing;        /* +0x74 */
    int32_t load_counter;   /* 0x007d2460 */
} AoeScenarioLoadWorld;

/* 0x0051ec20 (variant 0) and 0x0051eee0 (variant 1). `slot_order` is the
   scenario info's +0x270 i32 array (player_count entries, each 0..8).
   Variants 2..6 are 0x0051f1b0, 0x0051f4d0, 0x0051f7b0, 0x0051faf0 and
   0x0051fe30: each first skips a length-prefixed block (an i32 length,
   that many bytes read and discarded; a failed allocation closes the
   stream and stops) and clears 0x007d2460. Variant 2 then follows
   variant 1 with the player loads at scale 1.04. Variants 3..6 read the
   group count right after the views and run the player loads (+0x68,
   scale 1.04 / 1.06 / 1.07 / 1.08) before the object groups (no per-group
   byte pairs, no 13-byte skip); variants 4..6 then read the count again
   and run the players' +0x70 loads at the same scale before closing. */
void aoe_scenario_load_legacy(AoeScenarioLoadWorld *w,
                              const AoeScenarioLoadOps *ops,
                              AoeOutcomeStream *s, const int32_t *slot_order,
                              int variant);

#endif
