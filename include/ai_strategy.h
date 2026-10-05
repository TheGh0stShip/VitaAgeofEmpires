#ifndef VITA_AOE_AI_STRATEGY_H
#define VITA_AOE_AI_STRATEGY_H

#include <stdint.h>

#include "ai_module.h"
#include "ai_tactics.h"     /* AoeAiCountersRor, AoeAiPlayListRor */
#include "player_outcome.h" /* AoeOutcomeStream */

/* Patched Rise of Rome 1.0a computer-player strategy pieces
   (0x004c6880..0x004cc5c0): the growable ID list the AI modules share,
   the RoR play list overrides, the Resource AI (module 0x3ef), the
   strategic-number table and the Strategy AI (module 0x3f0) that loads it
   from a file and pushes it into the tactical AI by category. Semantic
   storage; comments give the original offsets. */

/* A growable list of 32-bit values: data +0, count +4, an unused +8 and
   the allocated capacity +0xc. */
typedef struct {
    int32_t *data;
    int32_t count;
    int32_t field_8;
    int32_t capacity;
} AoeAiIntListRor;

/* 0x004c6880: nonzero when the value is among the first `count` entries
   (the scan also stops at the capacity). */
int aoe_ai_int_list_contains_ror(const AoeAiIntListRor *list, int32_t value);
/* 0x004c68c0: append the value unless present (1 when appended); a full
   list grows to exactly count + 1. The original writes past a list whose
   allocation failed; the port returns 0 instead. */
int aoe_ai_int_list_add_unique_ror(AoeAiIntListRor *list, int32_t value);
/* 0x004cc5c0: the address of entry `i`, first growing the capacity to
   i + 1 when it is smaller (the count is unchanged). NULL when that
   allocation fails (the original then indexes the old array). */
int32_t *aoe_ai_int_list_at_ror(AoeAiIntListRor *list, int32_t i);
/* 0x004e3af0: remove the first match, searching the whole capacity as the
   original does, shifting the rest down; the count drops (not below 0).
   1 when found. */
int aoe_ai_int_list_remove_ror(AoeAiIntListRor *list, int32_t value);
/* Free the storage and zero the list (the destructors' inline form). */
void aoe_ai_int_list_release_ror(AoeAiIntListRor *list);

/* RoR play list (vtable 0x00548c48 over the base play list): 0x004c88c0
   constructs, 0x004c8900 releases (deleting 0x004c88e0). Its name parsers
   0x004c8910 and 0x004c8920 forward to the base "Any" and speed parsers
   (aoe_ai_any_kind_ror / aoe_ai_speed_kind_ror). */
void aoe_ai_ror_play_list_init_ror(AoeAiPlayListRor *list);
void aoe_ai_ror_play_list_release_ror(AoeAiPlayListRor *list);
int32_t aoe_ai_ror_play_any_kind_ror(const char *name);
int32_t aoe_ai_ror_play_speed_kind_ror(const char *name);
/* 0x004c8930: the unit category named in a play: Hand 0, Ranged 1,
   Cavalry 2, Legion 3, Priest 4, Siege 5, else -1 (the base default). */
int32_t aoe_ai_play_category_ror(const char *name);
/* 0x004c8ae0: a unit's play category from its type class (+8, +0x14):
   class 0x12 Priest 4, 0x1c Legion 3, 0x0d Siege 5, 0x0c or 0x23 Cavalry
   2, otherwise Ranged 1 when its range (the unit's vtable +0x10c) is above
   1.0, else Hand 0. The range is only asked for in that last case. */
typedef double (*AoeAiUnitRangeFn)(void *context);
int32_t aoe_ai_play_unit_category_ror(int16_t unit_class,
                                      AoeAiUnitRangeFn range, void *context);

/* Resource AI (vtable 0x00548c5c): the module, +0xf0 the owning player
   (unused here) and +0xf4 the resource count. */
typedef struct {
    AoeAiModuleRor module;
    int32_t field_f0;
    int32_t resource_count;
} AoeAiResourceRor;

/* 0x004c8b50: "Resource AI" 0x3ef for the player, with the resource count
   (the computer player passes 4). Load constructor 0x004c8bb0: "ResAI"
   and the count read from the stream. 0x004c8c30 releases (deleting
   0x004c8b90); 0x004c8c40 saves the count (i32). */
void aoe_ai_resource_init_ror(AoeAiResourceRor *ai, int32_t player,
                              int32_t resource_count, AoeAiLogRor *log);
int aoe_ai_resource_load_ror(AoeAiResourceRor *ai, int32_t player,
                             AoeOutcomeStream *in, AoeAiLogRor *log);
void aoe_ai_resource_release_ror(AoeAiResourceRor *ai);
int aoe_ai_resource_save_ror(const AoeAiResourceRor *ai,
                             AoeOutcomeStream *out);
/* 0x004c8c60: the player's resource `i` (player +0x50 floats) truncated
   with __ftol, or -1 outside 0..count-1. */
int32_t aoe_ai_resource_amount_ror(const AoeAiResourceRor *ai,
                                   const float *resources, int32_t i);
/* 0x004c8c90: 1 when every cost in `costs` is at most the matching stored
   resource (the cost converted to float); 0 for no costs or a count other
   than the module's. */
int aoe_ai_resource_can_afford_ror(const AoeAiResourceRor *ai,
                                   const float *resources,
                                   const AoeAiCountersRor *costs);
/* 0x004c8d10: Food, Wood, Stone, Gold for 0..3, else "Unknown". */
const char *aoe_ai_resource_name_ror(int32_t i);

/* Strategic numbers: 226 values. The table (vtable 0x00548c9c, values at
   +4) is built by 0x004c8d50 with the defaults below; 0x004c9370 /
   0x004c9350 are its destructors (no state). */
enum { AOE_AI_SN_COUNT = 226 };
typedef struct {
    int32_t values[AOE_AI_SN_COUNT];
} AoeAiStrategicNumbersRor;

extern const int32_t aoe_ai_sn_defaults_ror[AOE_AI_SN_COUNT];
void aoe_ai_sn_init_ror(AoeAiStrategicNumbersRor *sn);
/* 0x004c9520 set / 0x004c9540 get (-1 outside 0..225; a set there is
   ignored). */
void aoe_ai_sn_set_ror(AoeAiStrategicNumbersRor *sn, int32_t index,
                       int32_t value);
int32_t aoe_ai_sn_get_ror(const AoeAiStrategicNumbersRor *sn, int32_t index);

/* The host's text files for 0x004c9380. `open` returns a handle or NULL,
   `gets` reads one line of at most `size` - 1 bytes like fgets (0 at the
   end), `remove` deletes a file. */
typedef struct {
    void *context;
    void *(*open)(void *context, const char *path);
    int (*gets)(void *context, void *file, char *line, int size);
    void (*close)(void *context, void *file);
    void (*remove)(void *context, const char *path);
} AoeAiTextFileOpsRor;

/* 0x004c9380: read "<index> <value>" lines into the table. The name
   "NONE" or "" reads nothing (0). Lines whose first two non-blank
   characters are "//", "DE" or "VC" are skipped and "EN" ends the file;
   any other line is scanned with "%d %d" and an index in 0..225 is set.
   As in the original, a line that does not scan reuses the previous
   line's index and value (the port starts them at -1, so nothing is set
   before the first good line). After reading, a name containing ".tmp"
   is deleted. 1 when the file opened. A NULL name gives 0 (the original
   compares it before checking). */
int aoe_ai_sn_load_file_ror(AoeAiStrategicNumbersRor *sn, const char *name,
                            const AoeAiTextFileOpsRor *ops);

/* 0x004c9560: -2 for the strategy categories 1, 2, 12..14, 16..18 and
   21..49 that 0x004c95d0 knows, else 0. */
int32_t aoe_ai_sn_category_kind_ror(int32_t category);

/* The computer player's tactical-AI strategic-number setter
   (aoe_ai_tactical_sn_set_ror for the player's tactical AI). */
typedef void (*AoeAiSnSetFn)(void *context, int32_t index, int32_t value);
/* 0x004c95d0: copy the category's numbers from the table into the
   player's tactical AI, in the original's order. `victory_kind` must be
   0..12. 1 when the category is known, else 0 (nothing copied). */
int aoe_ai_sn_apply_category_ror(const AoeAiStrategicNumbersRor *sn,
                                 int32_t victory_kind, int32_t category,
                                 AoeAiSnSetFn set, void *context);
/* The numbers a category copies (for tests and tools). */
int32_t aoe_ai_sn_category_numbers_ror(int32_t category,
                                       const int16_t **numbers);

/* 0x004cac50: scale the table for difficulty 1..4 (0 and others do
   nothing). Each positive number among 16, 19, 20, 25, 26, 28, 36, 38,
   58, 59, 60, 67, 68, 69 is multiplied by the difficulty's factor and
   truncated (__ftol; 0 becomes 1); 46 and 48 gain an amount, 104 gains
   another, 88 gains the difficulty, and 103 becomes 1 below difficulty 4,
   else 0. Difficulty 2: 0.6, 60, 240. 3: 0.4, 90, 360 when
   `settings_flag` (the game setting read by 0x0041b700) is 1, else 900.
   4: 0.2, 120, 480 or 1260. Difficulty 1 is 0.8, 30, 120 (and 88 gains
   1) only when the flag is 1; otherwise the factor stays 1.0 and nothing
   is added. */
void aoe_ai_sn_scale_difficulty_ror(AoeAiStrategicNumbersRor *sn,
                                    int32_t difficulty,
                                    int32_t settings_flag);

/* The scenario's first victory condition as the Strategy AI reads it
   (0x0050ea10(0) on the player's outcome list): the kind (+0), the unit
   at +0x28 and at +0x24 (their IDs, +4, and type IDs, type +0x10), the
   area (+0xc, +0x10, +0x14, +0x18) and +0x1c, +0x20. */
typedef struct {
    uint8_t kind;
    int has_unit_28, has_unit_24;
    int32_t unit_28_id, unit_28_type;
    int32_t unit_24_id, unit_24_type;
    float area[4];
    int32_t field_1c, field_20;
} AoeAiVictoryTargetRor;

/* Strategy AI (vtable 0x00548ca0). */
typedef struct {
    AoeAiModuleRor module;
    int32_t field_f0;                 /* +0xf0, the computer player */
    int32_t victory_kind;             /* +0xf4, -1 */
    char sn_file[256];                /* +0xf8 */
    int32_t target_id, target_type;   /* +0x1fc, +0x200 (-1) */
    int32_t second_id, second_type;   /* +0x204, +0x208 (-1) */
    float target_point[4];            /* +0x20c (-1, -1, -1) */
    float second_point[4];            /* +0x21c (-1, -1, -1) */
    int32_t field_22c, field_230;     /* -1 */
    int32_t update_count;             /* +0x234 */
    AoeAiIntListRor categories;       /* +0x238 */
    AoeAiIntListRor applied;          /* +0x248 */
    AoeAiIntListRor rejected;         /* +0x258 */
    AoeAiStrategicNumbersRor numbers; /* +0x268 (table +4 at +0x26c) */
    int32_t difficulty;               /* +0x5f4 */
} AoeAiStrategyRor;

typedef struct {
    void *context;
    /* Fills the first victory condition; 0 when there is none. */
    int (*first_condition)(void *context, AoeAiVictoryTargetRor *out);
    AoeAiSnSetFn set_tactical_sn;
    const AoeAiTextFileOpsRor *files;
} AoeAiStrategyOpsRor;

/* 0x004cb030: the module, the fields above and the default table; the
   SN file name is "". */
void aoe_ai_strategy_ai_init_ror(AoeAiStrategyRor *ai, int32_t player,
                                 AoeAiLogRor *log);
/* 0x004cb640 (deleting 0x004cb160): free the three lists. */
void aoe_ai_strategy_ai_release_ror(AoeAiStrategyRor *ai);
/* 0x004cbcc0: the owning computer player (+0xf0). */
void aoe_ai_strategy_ai_set_owner_ror(AoeAiStrategyRor *ai, int32_t owner);
/* 0x004cbfe0 the SN file name; 0x004cbff0 copies a new name (at most 255
   characters; NULL is ignored) and loads it into the table. */
const char *aoe_ai_strategy_ai_sn_file_ror(const AoeAiStrategyRor *ai);
void aoe_ai_strategy_ai_load_sn_file_ror(AoeAiStrategyRor *ai,
                                         const char *name,
                                         const AoeAiTextFileOpsRor *files);
/* 0x004cc030: log "Setting SN %d to %d" and set; 0x004cc060 get;
   0x004cc080 scale for difficulty (0x004cac50). */
void aoe_ai_strategy_ai_set_sn_ror(AoeAiStrategyRor *ai, int32_t index,
                                   int32_t value);
int32_t aoe_ai_strategy_ai_get_sn_ror(const AoeAiStrategyRor *ai,
                                      int32_t index);
void aoe_ai_strategy_ai_scale_ror(AoeAiStrategyRor *ai, int32_t difficulty,
                                  int32_t settings_flag);
/* Getters: 0x004cc0a0 victory kind, 0x004cc0b0 target ID, 0x004cc0c0
   second ID, 0x004cc0d0 target point, 0x004cc0e0 second point. */
int32_t aoe_ai_strategy_ai_victory_kind_ror(const AoeAiStrategyRor *ai);
int32_t aoe_ai_strategy_ai_target_id_ror(const AoeAiStrategyRor *ai);
int32_t aoe_ai_strategy_ai_second_id_ror(const AoeAiStrategyRor *ai);
const float *aoe_ai_strategy_ai_target_point_ror(const AoeAiStrategyRor *ai);
const float *aoe_ai_strategy_ai_second_point_ror(const AoeAiStrategyRor *ai);

/* 0x004cc0f0: a new victory kind. The category list restarts as 1, 2,
   12, 13, 14, 16, 17, 18, 21..25 (each once) and gains 26..49; every
   category the table knows is applied to the tactical AI and recorded in
   the applied list (or, when the kind is outside 0..12, the rejected
   list). */
void aoe_ai_strategy_ai_set_victory_kind_ror(AoeAiStrategyRor *ai,
                                             int32_t kind,
                                             const AoeAiStrategyOpsRor *ops);
/* 0x004cb730 (vtable +0x30), the update. The counter is raised and, when
   not negative, reset; the first victory condition (kind 11 when there
   is none) refreshes the targets and a changed kind goes through
   0x004cc0f0. The original then rescans the three lists: a rejected
   category with a better kind than 0 is re-applied, the applied category
   with the lowest priority (a shared stub that always returns 0) moves
   back to the rejected list when that priority is 1, and categories of
   kind -1 are applied again. With the fixed kinds above none of those
   fire; the port keeps the scans. Returns 1. */
int aoe_ai_strategy_ai_update_ror(AoeAiStrategyRor *ai,
                                  const AoeAiStrategyOpsRor *ops);

/* 0x004cbcd0 save (vtable +0x3c) / 0x004cb180 load constructor: i32
   victory kind, target ID, second ID, second type (the target type is
   not saved), 16 bytes each point, +0x22c, +0x230, the counter, the
   difficulty (loaded only from save version 1.5), i32 name length and
   the name bytes, then each list as an i32 count and its values. Loading
   reads the SN file by name, scales it for the difficulty and adds the
   saved values to each list once. The target type is left at -1 (the
   original leaves it unset). */
int aoe_ai_strategy_ai_save_ror(const AoeAiStrategyRor *ai,
                                AoeOutcomeStream *out);
int aoe_ai_strategy_ai_load_ror(AoeAiStrategyRor *ai, int32_t player,
                                float version, AoeOutcomeStream *in,
                                int32_t settings_flag,
                                const AoeAiTextFileOpsRor *files,
                                AoeAiLogRor *log);

#endif
