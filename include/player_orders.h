#ifndef VITA_AOE_PLAYER_ORDERS_H
#define VITA_AOE_PLAYER_ORDERS_H

#include <stddef.h>
#include <stdint.h>

#include "command_builders.h"
#include "command_player_task.h"
#include "player_selection.h"

/* Patched Rise of Rome 1.0a player virtuals that turn the player's
   selection (or explicit arguments) into command records. Each slot listed
   here holds the same function in the base (0x00544d18) and both derived
   (0x00549a44 / 0x00549b80) player vtables. Records are built with the
   portable builders and handed to `send`, which stands for the command
   sender at world +0x58 queueing the record. Booleans are the original
   `al` results. */

/* The live-unit fields of an order target. */
typedef struct {
    AoeCommandObjectRor object; /* +0x04 ID, template byte +0x65 */
    int16_t owner_id;           /* +0x0c -> +0x4a */
    float x, y;                 /* +0x38, +0x3c */
    float template_28;          /* template +0x28 */
} AoePlayerOrderTargetRor;

typedef struct {
    AoePlayerSelectionRor *selection; /* player +0x4a and +0x134..+0x1a0 */
    uint8_t game_state;               /* +0x80 */
    int16_t resource_count;           /* +0x4c */
    const float *resources;           /* +0x50 */
} AoePlayerOrdersRor;

typedef struct {
    void *context;
    void (*send)(void *context, const uint8_t *record, size_t size);
    /* The first ordered unit's vtable +0xac / +0xb0 (acknowledgements). */
    void (*acknowledge)(void *context, AoeSelectableUnitRor *unit);
    void (*acknowledge_move)(void *context, AoeSelectableUnitRor *unit);
    /* The first ordered unit's vtable +0xbc(target, x, y, 0) and, when it
       returns a template, that template's +0x2c. */
    void (*acknowledge_target)(void *context, AoeSelectableUnitRor *unit,
                               const AoePlayerOrderTargetRor *target,
                               float x, float y);
    /* Whether player +0x24[index] (the template table) is non-null. */
    int (*has_template)(void *context, int16_t index);
    /* Whether the world +0xa0 table has an entry for `index` (a non-null
       table and 0 <= index < its count). */
    int (*has_world_entry)(void *context, int32_t index);
    /* Session fields of opcode-0 orders (may be NULL: no session). */
    const AoeCommandSessionRor *session;
} AoePlayerOrdersOps;

/* 0x0045be60 (+0x30): queue a unit task record (opcode 0x0a) for
   `player`; record +0x02 also receives `player` (task->mode is ignored). */
int aoe_player_send_unit_task_ror(uint8_t player,
                                  const AoePlayerUnitTaskRor *task,
                                  const AoePlayerOrdersOps *ops);

/* 0x0045bf30 (+0x40): an opcode-0 order for the single `unit`. */
int aoe_player_order_unit_ror(const AoeCommandObjectRor *unit,
                              const AoeCommandObjectRor *target, float x,
                              float y, const AoePlayerOrdersOps *ops);

/* 0x0045bf60 (+0x44): unit records (opcode 0x0c) from this player. */
int aoe_player_send_unit_records_ror(const AoePlayerOrdersRor *player,
                                     int32_t unit_id, const uint8_t *records,
                                     int32_t count,
                                     const AoePlayerOrdersOps *ops);

/* 0x0045bfd0 (+0x50): clear `unit_id`'s dependencies (+0xd0). Without a
   world entry for `entry` it queues task 0x2d7 for the unit (value -1,
   flag 0xff); otherwise, when any of the 25 selection slots is occupied
   (regardless of the count), it sets their IDs as the unit's dependencies
   (+0xc4 with 2.0f) and queues task 0x2d6 with value `value` and flag
   `entry`. Every task here has x/y/z/+0x20 = -1.0f and bytes 1, 0, 100. */
int aoe_player_assign_selection_ror(const AoePlayerOrdersRor *player,
                                    int32_t unit_id, int32_t value,
                                    int32_t entry,
                                    const AoePlayerOrdersOps *ops);

/* 0x0045c0c0 (+0x4c): as above with an explicit ID list: without a world
   entry for `entry`, task 0x2d7; otherwise, for a non-zero count, the
   dependency set and task 0x2d6 (value, flag = entry). */
int aoe_player_assign_units_ror(const AoePlayerOrdersRor *player,
                                int32_t unit_id, const int32_t *ids,
                                int32_t count, int32_t value, int32_t entry,
                                const AoePlayerOrdersOps *ops);

/* 0x0045e2a0 (+0x9c): no result: *a = *b = 0, returns 0. */
int aoe_player_query_none_ror(int32_t *a, int32_t *b);

/* 0x0045e2c0 (+0xa0): order the eligible selection (see
   aoe_player_selection_collect_ror, default filters) with opcode 3; the
   first unit acknowledges through +0xb0. 0 when nothing is eligible. */
int aoe_player_order_selection_ror(const AoePlayerOrdersRor *player,
                                   const AoePlayerOrderTargetRor *target,
                                   float x, float y,
                                   const AoePlayerOrdersOps *ops);

/* 0x0045e340 (+0xa4): the target acknowledgement, an opcode-2 order, then
   with a target, for each ordered unit, task 0x2bd toward it (value =
   target ID, flag = target owner, x/y = target position, z = 1.0f, +0x20
   = template +0x28 * 0.5f, bytes 0, 0, 100). */
int aoe_player_order_selection_follow_ror(
    const AoePlayerOrdersRor *player, const AoePlayerOrderTargetRor *target,
    float x, float y, const AoePlayerOrdersOps *ops);

/* 0x0045e430 (+0xa8): the target acknowledgement and an opcode-0 order. */
int aoe_player_order_selection_primary_ror(
    const AoePlayerOrdersRor *player, const AoePlayerOrderTargetRor *target,
    float x, float y, const AoePlayerOrdersOps *ops);

/* 0x0045e4c0 (+0xac): stop (opcode 1) the eligible selection, with a
   minimum template +0x8d of 3; +0xac acknowledges. */
int aoe_player_stop_selection_ror(const AoePlayerOrdersRor *player,
                                  const AoePlayerOrdersOps *ops);

/* 0x0045e530 (+0xb0): create template `index` (opcode 4) at x/y/z when
   index is not -1 and the player has that template. */
int aoe_player_create_object_ror(const AoePlayerOrdersRor *player,
                                 int16_t index, float x, float y, float z,
                                 const AoePlayerOrdersOps *ops);

/* 0x0045e5c0 (+0xb8): tribute (opcode 7) `amount` of `resource` to
   `receiver` when resource is not -1, amount is neither 0 nor NaN, and
   amount is not above the player's stock (a NaN stock passes). Port: a
   resource outside 0..count-1 (read unchecked by the original) fails. */
int aoe_player_tribute_ror(const AoePlayerOrdersRor *player,
                           uint8_t receiver, int32_t resource, float amount,
                           const AoePlayerOrdersOps *ops);

/* 0x0045e620 (+0xbc): arrange the eligible selection (opcode 6) in
   pattern 0..5; +0xac acknowledges. */
int aoe_player_arrange_selection_ror(const AoePlayerOrdersRor *player,
                                     int32_t pattern,
                                     const AoePlayerOrdersOps *ops);

/* 0x0045e6a0 (+0xc0): task 0x2d5 for the eligible selection (opcode
   0x11: mode = player, value -1, flag 0xff, x/y/z -1.0f, +0x1c 1.0f,
   bytes 1, 0, 100); +0xac acknowledges. */
int aoe_player_task_selection_ror(const AoePlayerOrdersRor *player,
                                  const AoePlayerOrdersOps *ops);

/* 0x0045e730 (+0xc4), 0x0045e760 (+0xc8), 0x0045e790 (+0xcc) and
   0x0045e7b0 (+0xd0): the dependency set / propagate / remove / clear
   records (opcodes 8, 0x0e, 0x0f, 9) from this player. The set record
   holds at most 25 IDs; a larger count fails (the original overruns). */
int aoe_player_dependency_set_ror(const AoePlayerOrdersRor *player,
                                  int32_t unit_id, const int32_t *ids,
                                  int32_t count, uint32_t context_bits,
                                  const AoePlayerOrdersOps *ops);
int aoe_player_dependency_propagate_ror(const AoePlayerOrdersRor *player,
                                        int32_t unit_id, int32_t target_id,
                                        uint32_t context_bits,
                                        const AoePlayerOrdersOps *ops);
int aoe_player_dependency_remove_ror(const AoePlayerOrdersRor *player,
                                     int32_t unit_id, int32_t removed_id,
                                     const AoePlayerOrdersOps *ops);
int aoe_player_dependency_clear_ror(const AoePlayerOrdersRor *player,
                                    int32_t unit_id,
                                    const AoePlayerOrdersOps *ops);

/* 0x0045e7d0 (+0xd4): resign (opcode 0x0b, slot, value) while the game
   state byte +0x80 is 0. */
int aoe_player_send_resign_ror(const AoePlayerOrdersRor *player,
                               uint8_t slot, uint8_t value,
                               const AoePlayerOrdersOps *ops);

/* 0x0045e800 (+0xd8): a record list (opcode 0x10) for the eligible
   selection with bytes (int8)__ftol(a) and (int8)__ftol(b); the third
   argument is unused. +0xac acknowledges. */
int aoe_player_record_selection_ror(const AoePlayerOrdersRor *player,
                                    float a, float b,
                                    const AoePlayerOrdersOps *ops);

#endif
