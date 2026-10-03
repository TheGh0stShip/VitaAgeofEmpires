#ifndef VITA_AOE_COMMAND_ORDER_H
#define VITA_AOE_COMMAND_ORDER_H

#include <stddef.h>
#include <stdint.h>

#include "aoe_rng.h"
#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a command opcode 0, unit order (0x0042a280).
   Record: count byte +0x01, target ID +0x04, floats x/y +0x08/+0x0c,
   issuing player byte +0x10, flag byte +0x12, masks +0x14/+0x18, and
   `count` unit IDs from +0x1c. */
enum {
    AOE_COMMAND_ORDER_ROR = 0x00,
    AOE_COMMAND_ORDER_HEADER_SIZE_ROR = 0x1c,
    /* Opcode 1 (0x0042a730): count +0x01, unit IDs from the unaligned
       offset +0x02. Opcode 2 (0x0042a780): opcode 0's layout. */
    AOE_COMMAND_STOP_ROR = 0x01,
    AOE_COMMAND_ORDER_ALTERNATE_ROR = 0x02,
    /* Opcode 3 (0x0042a850): opcode 0's layout. */
    AOE_COMMAND_ORDER_FORMATION_ROR = 0x03,
    /* Opcode 6 (0x0042ae20): int32 count +0x04, int32 pattern +0x08, unit
       IDs from +0x0c. */
    AOE_COMMAND_ARRANGE_ROR = 0x06,
    AOE_COMMAND_ARRANGE_HEADER_SIZE_ROR = 0x0c
};

typedef struct {
    AoeLiveUnitDependencyOps world; /* 0x0042a120: -1 never resolves */
    void *context;
    /* Validity gate inputs. */
    int (*session_check)(void *context);                 /* game 0x0041b760 */
    int32_t (*owner_kind)(void *context, const AoeLiveUnit *unit); /* +0x04 */
    int16_t local_player;                               /* world +0x7c */
    int32_t (*player_104)(void *context, uint8_t player); /* player +0x104 */
    /* Global row table 0x007d205c[row][column]; return 0 when outside the
       host map (the original reads unchecked). */
    int (*map_cell)(void *context, int32_t row, int32_t column,
                    uint32_t *cell);
    AoeRng *rng;          /* CRT rand() for dropped commands */
    int32_t *rand_result; /* global 0x00581ab0 */
    /* Per unit. */
    void (*set_order_flag)(void *context, AoeLiveUnit *unit,
                           int32_t value);              /* vtable +0x12c */
    void (*order)(void *context, AoeLiveUnit *unit, AoeLiveUnit *target,
                  float x, float y);              /* vtable +0x98(..., 0) */
    /* Owner +0x3c -> world +0x28 -> map +0x08 (axis 0) / +0x0c (axis 1). */
    int32_t (*map_size)(void *context, const AoeLiveUnit *unit, int axis);
    /* Opcode 1: vtable +0xa4 (see aoe_live_unit_stop_ror). */
    int (*stop)(void *context, AoeLiveUnit *unit);
    /* Opcode 2: vtable +0xa0(target, x, y, 0), which for action classes
       (0x00405ee0) is +0x98's +0x1f8 call with final flag 1. */
    void (*order_alternate)(void *context, AoeLiveUnit *unit,
                            AoeLiveUnit *target, float x, float y);
    /* Opcodes 3 and 6: vtable +0x9c(target, x, y, last). `last` is the
       raw 32-bit fourth argument: 0 for opcode 3, 0x3f800000 (1.0f) for
       opcode 6. */
    void (*order_formation)(void *context, AoeLiveUnit *unit,
                            AoeLiveUnit *target, float x, float y,
                            uint32_t last);
    /* Opcode 3: when the unit's +0x74 group object exists (0x00407a60),
       it runs 0x0040f6c0(0x2c6), 0x0040f6d0(0x262) and then
       0x00412f50(target_id, target_class, x, y, 0). */
    int (*has_group_object)(void *context, const AoeLiveUnit *unit);
    void (*group_order)(void *context, AoeLiveUnit *unit, int32_t target_id,
                        int32_t target_class, float x, float y);
} AoeCommandOrderOps;

size_t aoe_command_encode_order_ror(
    uint8_t *out, size_t out_size, uint8_t player, int32_t target_id,
    float x, float y, uint8_t flag, uint32_t mask_14, uint32_t mask_18,
    const int32_t *unit_ids, uint8_t count);

/* 0x0042a280.
   Gate: applies only when the target resolves, session_check() is true,
   the target ID is not -1, count > 0, the first unit resolves with owner
   +0x04 == 0, the issuing player is not the local player, and the target
   template type is 80, 90, 10 or 20. It then drops the command, storing
   rand() in *rand_result, if the issuing player's +0x104 is 0 while the
   flag byte is set, if the masks do not intersect, or if the target's map
   cell (row __ftol(y), column __ftol(x)) lacks any of mask +0x18's high 16
   bits.
   Orders: with more than one unit whose positions all lie within an 8.0
   spread on both axes (NaN counts as within), each unit keeps its offset
   from the group centre, clamped to [1.0 if negative, size - 1 if at or
   beyond the map edge], and the target is passed only when its template
   size[0] or size[1] is positive. Otherwise every unit gets the raw x/y and
   target. Each resolved unit first runs 0x004ac320, vtable +0x12c(1), and
   adds every other listed ID (raw, unresolved included) to its group
   through 0x004ac280.
   Returns 0 for a dropped command, 1 when processed, and -1 for a
   malformed record, missing dependencies, an unreadable map cell, or a
   failed group update. */
int aoe_command_execute_order_ror(const uint8_t *command, size_t size,
                                  const AoeCommandOrderOps *ops);

/* 0x0042a850: opcode 0's formation/simple paths without the validity
   gate, ordering through +0x9c. After each order a unit with a group
   object reports the target ID/class, or -1/-1 when the target is missing
   or has no positive size, and the x/y the unit was ordered to. In the
   simple path the order itself still receives the unfiltered target. */
int aoe_command_execute_order_formation_ror(const uint8_t *command,
                                            size_t size,
                                            const AoeCommandOrderOps *ops);

size_t aoe_command_encode_arrange_ror(uint8_t *out, size_t out_size,
                                      int32_t pattern,
                                      const int32_t *unit_ids, uint8_t count);

/* 0x0042ae20. Nothing happens unless count > 1. The leader is the unit
   with the smallest sqrt(x*x + y*y) (computed unrounded, kept as a float;
   a later unit replaces it when strictly closer or NaN). Every resolved
   unit runs 0x004ac320, +0x12c(1) and the group update; each non-leader is
   then ordered through +0x9c(0, x, y, 1.0f) at a point stepped from the
   leader's own position:
     0: (+sx, +sy), steps start 1.0 and grow by 1.0 (diagonal);
     1: (+sx, -sy), start 1.5, grow 1.5;
     2: (+sx, 0), start 2.0, grow 2.0;   3: (0, +sy), likewise;
     4/5: x = +sx (4) or -sx (5) with steps starting 1.5; a toggle shared
          by all units alternates y = -sy (set toggle) and y = +sy (clear
          toggle, then both steps grow by 1.5);
     other: the leader's own point.
   Host deviation: if the unit at index 0 does not resolve, the original
   compares the next distance against an uninitialised local; here the first
   resolved unit becomes the leader. Returns -1 for a malformed record or
   missing dependencies. */
int aoe_command_execute_arrange_ror(const uint8_t *command, size_t size,
                                    const AoeCommandOrderOps *ops);

size_t aoe_command_encode_stop_ror(uint8_t *out, size_t out_size,
                                   const int32_t *unit_ids, uint8_t count);

/* 0x0042a730: each resolved unit whose template ID is not 50 runs
   0x004ac320 and then stop. Returns -1 for a malformed record, missing
   dependencies, or a failed stop; otherwise 1. */
int aoe_command_execute_stop_ror(const uint8_t *command, size_t size,
                                 const AoeCommandOrderOps *ops);

/* 0x0042a780: the target is looked up once, then every resolved unit runs
   0x004ac320, +0x12c(1), the 0x004ac280 group update, and order_alternate
   with the raw x/y. No validity gate or formation logic. */
int aoe_command_execute_order_alternate_ror(const uint8_t *command,
                                            size_t size,
                                            const AoeCommandOrderOps *ops);

#endif
