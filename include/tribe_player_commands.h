#ifndef VITA_AOE_TRIBE_PLAYER_COMMANDS_H
#define VITA_AOE_TRIBE_PLAYER_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

#include "player_outcome.h" /* AoeOutcomeStream */
#include "tribe_command_builders.h"

/* Patched Rise of Rome 1.0a tribe player command methods
   (0x004f10e0..0x004f2d60): the player turns an interface request into a
   command for its selection (0x0045e0c0 with the filters noted) and
   queues it (the builders' calloc + queue); the AI module relays. The
   selection, the player's templates and the queue are ops. */

typedef struct {
    void *context;
    int16_t player;                     /* +0x4a */
    /* 0x0045e0c0(min +0x8d, class, template, +0x8f): up to 25 units. */
    int16_t (*collect)(void *context, int16_t min_8d, int16_t cls, int16_t tmpl,
                       int16_t f8f, const AoeTribeCommandUnitRor **out);
    /* A template (+0x24[id]): 0 absent, 1 present, 2 available (+0x52). */
    int (*template_state)(void *context, int16_t id);
    /* The acknowledgements: template 118's +0x2c, a unit's +0xac. */
    void (*ack_builder)(void *context);
    void (*ack_unit)(void *context, const AoeTribeCommandUnitRor *unit);
    /* The selected object (+0x134). */
    const AoeTribeCommandUnitRor *(*selected)(void *context);
    uint8_t (*object_type)(void *context, const AoeTribeCommandUnitRor *o);  /* template +4 */
    int16_t (*object_class)(void *context, const AoeTribeCommandUnitRor *o); /* template +0x14 */
    int (*queue)(void *context, const uint8_t *command, size_t size);
} AoeTribePlayerOpsRor;

/* 0x004f10e0 (+?): build `id` at (x, y) with the selected villagers
   (+0x8d >= 4, class 4), when the template is available or is 0x6d:
   template 118 acknowledges, then opcode 0x66. 0x004f1170: the same for
   opcode 0x69 (a..d), the template must be available. */
int aoe_tribe_player_build_ror(const AoeTribePlayerOpsRor *ops, int16_t id, float x, float y);
int aoe_tribe_player_build_69_ror(const AoeTribePlayerOpsRor *ops, int16_t id, int32_t a,
                                  int32_t b, int32_t c, int32_t d);
/* 0x004f1410 / 0x004f1200: train / research `item` at the selected
   building (type 'P' = 80). */
int aoe_tribe_player_train_ror(const AoeTribePlayerOpsRor *ops, int16_t item);
int aoe_tribe_player_research_ror(const AoeTribePlayerOpsRor *ops, int16_t item);
/* 0x004f1240: delete the selected object (ours, type 80 or 70, class not
   1); 0x004f1280 the given one (ours, type 80 or 70): opcode 0x6a. */
int aoe_tribe_player_delete_selected_ror(const AoeTribePlayerOpsRor *ops);
int aoe_tribe_player_delete_ror(const AoeTribePlayerOpsRor *ops,
                                const AoeTribeCommandUnitRor *object, int ours);
/* 0x004e1ae0 (the computer player's): the player's object with this
   ID (0x0040bab0), deleted as above; 1 when that queued. */
int aoe_tribe_player_delete_id_ror(const AoeTribePlayerOpsRor *ops,
                                   const AoeTribeCommandUnitRor *(*lookup)(void *context,
                                                                         int32_t id),
                                   int32_t id);
/* 0x004f12c0 (+0x8f 5) opcode 0x6d value; 0x004f1350 (+0x8f 4) opcode
   0x6b (a, b); 0x004f1580 (+0x8f 7) opcode 0x6f (x, y): the first unit
   acknowledges. 0x004f1330: opcode 0x6d for one raw ID. */
int aoe_tribe_player_6d_ror(const AoeTribePlayerOpsRor *ops, uint32_t value);
int aoe_tribe_player_6d_one_ror(const AoeTribePlayerOpsRor *ops, int32_t id, uint32_t value);
int aoe_tribe_player_6b_ror(const AoeTribePlayerOpsRor *ops, uint32_t a, uint32_t b);
int aoe_tribe_player_6f_ror(const AoeTribePlayerOpsRor *ops, float x, float y);
/* 0x004f2b60: opcode 0x66 for one raw ID (value, x, y, +0x10);
   0x004f2d60: opcode 0x67 sub 0 (the player, b, value). */
int aoe_tribe_player_66_one_ror(const AoeTribePlayerOpsRor *ops, int32_t id, int16_t value,
                                float x, float y, int32_t field_10);
int aoe_tribe_player_67_ror(const AoeTribePlayerOpsRor *ops, int16_t b, int16_t value);
/* 0x004f20c0: a template is usable (available, or ID 0x6d present... as
   the original: absent or unavailable fail unless the ID is 0x6d). */
int aoe_tribe_player_template_usable_ror(const AoeTribePlayerOpsRor *ops, int16_t id);
/* 0x004f20f0: tech `i`'s state (tech tree +0x224, 8-byte entries, +4) is
   1. 0x004f2110: a research state (0x004ea9ac) other than 3 and -1. */
int aoe_tribe_tech_state_is_one_ror(const uint8_t *entries, int32_t i);
int aoe_tribe_research_open_ror(int16_t state);

/* The RoR tribe player's own slots (vtable 0x00549a44, also
   0x00549b80) that forward to its computer player (+0x23c), the
   production queue, the command builder or the score history. */
typedef struct {
    void *context;
    int has_ai;        /* +0x23c set */
    int16_t player_id; /* +0x4a */
    void (*ai_kick)(void *context, int32_t kind);                  /* 0x004c7ff0 */
    int32_t (*ai_strategic)(void *context, int32_t i);             /* 0x004d77d0 */
    void (*ai_log)(void *context, const char *line);               /* 0x0040c240 */
    void (*ai_unit_added)(void *context, int32_t id);              /* AI +0x44 */
    void (*add_unit)(void *context, void *unit, int32_t a, int32_t b, int32_t c); /* 0x0045e8d0 */
    int (*unit_info)(void *context, void *unit, int16_t *unit_class, int32_t *id); /* +0x08 +0x14, +0x04 */
    void (*queue_train)(void *context, int16_t player, int32_t a, int32_t b, int32_t c);    /* 0x004e94e0 */
    void (*queue_research)(void *context, int16_t player, int32_t a, int32_t b, int32_t c); /* 0x004e9580 */
    void (*order_ids)(void *context, int16_t player, const int32_t *ids, int32_t count, int32_t a,
                      int32_t b, int32_t c, int32_t d);              /* 0x0042b970 */
    void (*tribute)(void *context, int16_t player, int32_t a, int32_t resource, float amount,
                    int32_t b);                                      /* world +0x58 +0x14 */
    void (*sample)(void *context, int32_t skip, int32_t value);    /* 0x004b0c00 */
} AoeTribeForwardOpsRor;

/* +0x38 0x004f29d0: the AI's kick; +0x3c 0x004f29f0: its strategic
   number (-1 without an AI); +0xe4 0x004f2a20: a formatted AI log line
   (1024 bytes). +0xe0 0x004f2b10: the base add (0x0045e8d0), then the AI
   hears of a unit with an ID that is not class 11. +0x108 0x004f2c20 /
   +0x114 0x004f2c80: train / research with the first two arguments
   swapped. +0x120 / +0x124 0x004f2d10: an order for one raw ID.
   +0xf8 0x004f13c0: a tribute when the resource is set and the amount
   nonzero (1), else 0. 0x004f1aa0: a score history sample of the
   +0x50 record's +0x2c (truncated; 0 when negative). */
void aoe_tribe_ai_kick_ror(const AoeTribeForwardOpsRor *ops, int32_t kind);
int32_t aoe_tribe_ai_strategic_ror(const AoeTribeForwardOpsRor *ops, int32_t i);
void aoe_tribe_ai_log_ror(const AoeTribeForwardOpsRor *ops, const char *format, ...);
void aoe_tribe_add_unit_ror(const AoeTribeForwardOpsRor *ops, void *unit, int32_t a, int32_t b,
                            int32_t c);
void aoe_tribe_queue_train_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t b, int32_t c);
void aoe_tribe_queue_research_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t b, int32_t c);
void aoe_tribe_order_one_ror(const AoeTribeForwardOpsRor *ops, int32_t id, int32_t a, int32_t b,
                             int32_t c, int32_t d);
int aoe_tribe_tribute_ror(const AoeTribeForwardOpsRor *ops, int32_t a, int32_t resource,
                          float amount, int32_t b);
void aoe_tribe_sample_score_ror(const AoeTribeForwardOpsRor *ops, float score);

/* +0x64 0x004f0280: the +0x50 record's dwords +0x00, +0x04, +0x0c,
   +0x08 (in that order) after the empty base save (0x004ed890). */
void aoe_tribe_save_record_ror(const int32_t record[4], AoeOutcomeStream *s);

/* 0x004f0ac0: after unit `after` (when given) the next unit of
   template `id` (+0x10) in the player's list that is below state 3;
   otherwise the first one in state 2; NULL when none. */
typedef struct {
    void *unit;
    int16_t template_id;
    uint8_t state;
} AoeTribeListUnitRor;
void *aoe_tribe_next_of_template_ror(const AoeTribeListUnitRor *units, int32_t count, int16_t id,
                                     const void *after);
/* Vtable 0x00549b80 +0x7c 0x004f7360: the 30-step cycle counter (+0x850)
   advances once per whole second of world time accumulated at +0x84c,
   then the base list update (0x0045d040). */
void aoe_tribe_cycle_tick_ror(float *seconds, int32_t *counter, float tick);
/* +0x70 0x004f73d0: after the base block load (0x0045c6a0), an
   attribute 50 above 0 sets +0x104 and refreshes the world's shared
   vision (0x00520c60). Returns whether it did. */
int aoe_tribe_shared_vision_check_ror(float attribute_50, int32_t *flag_104,
                                      void (*refresh)(void *context), void *context);

#endif
