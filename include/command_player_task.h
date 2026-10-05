#ifndef VITA_AOE_COMMAND_PLAYER_TASK_H
#define VITA_AOE_COMMAND_PLAYER_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "live_unit_factory.h"

/* Patched Rise of Rome 1.0a base command opcodes routed through player
   virtuals. Every listed player virtual is shared by the base (0x00544d18)
   and both derived (0x00549a44/0x00549b80) player vtables. */
enum {
    AOE_COMMAND_UNIT_TASK_ROR = 0x0a,        /* 0x0042a120 -> 0x0042b120 */
    AOE_COMMAND_UNIT_TASK_SIZE_ROR = 0x28,   /* builder 0x0042bcf0 */
    AOE_COMMAND_RESIGN_ROR = 0x0b,           /* 0x0042b290 */
    AOE_COMMAND_UNIT_RECORDS_ROR = 0x0c,     /* 0x0042b3b0 */
    AOE_COMMAND_UNIT_RECORD_LIST_ROR = 0x10, /* 0x0042b3f0 */
    AOE_COMMAND_UNIT_TASK_LIST_ROR = 0x11,   /* 0x0042b1a0 */
    AOE_COMMAND_UNIT_TASK_LIST_HEADER_ROR = 0x24
};

/* Arguments of player vtable +0x34 (0x0045beb0) after `this`, in call
   order. x/y/z keep their raw 32-bit float patterns. */
typedef struct {
    uint8_t mode;       /* record +0x02 */
    int32_t unit_id;
    int16_t value_16;
    uint32_t value_32;
    uint8_t flag;
    uint32_t x_bits, y_bits, z_bits;
    uint32_t extra_32;
    uint8_t extra_a, extra_b, extra_c;
} AoePlayerUnitTaskRor;

typedef struct {
    AoeLiveUnitDependencyOps world; /* 0x005206d0 */
    void *context;
    int16_t (*player_id)(void *context, uint8_t player_index); /* +0x4a */
    int16_t (*owner_id)(void *context, const AoeLiveUnit *unit);
    int32_t (*owner_kind)(void *context, const AoeLiveUnit *unit); /* +0x04 */
    /* Unit +0x74 group object (0x00407a60) and its vtable +0x10, which
       receives every task field except unit_id. */
    int (*has_group_object)(void *context, const AoeLiveUnit *unit);
    void (*group_task)(void *context, AoeLiveUnit *unit,
                       const AoePlayerUnitTaskRor *task);
    /* Unit vtable +0x1bc(record, 1) for one 3-byte record. */
    void (*unit_record)(void *context, AoeLiveUnit *unit,
                        const uint8_t record[3]);
    /* Opcode 0x0b. */
    int (*session_check)(void *context);                 /* 0x0041b760 */
    uint8_t *slot_left;           /* game +0xa15, indexed by slot 0..255 */
    uint32_t (*local_slot)(void *context);               /* 0x0041c2b0 */
    /* 0x0041bc10(1), 0x0041bc30(0), then game vtable +0x2c and +0x30
       objects (when present) +0x20(2). */
    void (*local_left)(void *context);
    uint32_t (*slot_player)(void *context, int slot);    /* 0x0041c080 */
    uint8_t *(*player_resigned)(void *context, uint8_t player); /* +0x81 */
    void (*player_state)(void *context, uint8_t player, int32_t state); /* +4 */
    void (*game_event)(void *context, uint32_t event, int32_t player,
                       uint32_t value);           /* game +0x40(..., 0, 0) */
} AoeCommandPlayerTaskOps;

/* Player vtable +0x34 = 0x0045beb0: resolve unit_id; when it exists, has a
   group object, and its owner ID equals `player_id`, forward the task to
   the group object. Returns 0 for missing dependencies. */
int aoe_player_unit_task_ror(int16_t player_id,
                             const AoePlayerUnitTaskRor *task,
                             const AoeCommandPlayerTaskOps *ops);

/* Player vtable +0x48 = 0x0045bf90: resolve unit_id, then feed `count`
   3-byte records to unit +0x1bc in order. */
int aoe_player_unit_records_ror(int32_t unit_id, const uint8_t *records,
                                int32_t count,
                                const AoeCommandPlayerTaskOps *ops);

/* Player vtable +0x08 = 0x0045bd00: set +0x81, call +4(2), and on the
   first resignation raise game event 5 (player ID, value). */
int aoe_player_resign_ror(uint8_t player_index, uint32_t value,
                          const AoeCommandPlayerTaskOps *ops);

size_t aoe_command_encode_unit_task_ror(uint8_t *out, size_t out_size,
                                        uint8_t player,
                                        const AoePlayerUnitTaskRor *task);

/* Each execute returns -1 for a malformed record or missing dependencies,
   0 when the original does nothing, and 1 otherwise.
   0x0a: the unit at +0x04 (direct lookup) runs 0x004ac320, then the
   issuing player (+0x01) receives the record's task through +0x34.
   0x11: for each of the count (+0x03) IDs from +0x24, a resolved unit's
   x/y/z are taken from +0x10/+0x14/+0x18 unless their bits equal -1.0f
   (0xbf800000), in which case its own position is used. The unit runs
   0x004ac320 and the issuing player receives +0x34(mode +0x02, the unit's
   ID, +0x04 int16, +0x08, byte +0x0c, x, y, z, +0x1c, bytes +0x20,
   +0x21, +0x06).
   0x0b: without a session, the player at +0x01 resigns with byte +0x03.
   In a session the slot at +0x02 is marked as left, a local slot runs
   local_left, and the player resigns only when every slot 1..8 mapped to
   it has left.
   0x0c: if the unit at +0x04 resolves, the issuing player runs +0x48 with
   the byte count at +0x08 and the records from +0x09.
   0x10: each resolved ID of the count (+0x01) from +0x04 gets the record
   {+0x02, +0x03, 0} through its owner's +0x48. */
int aoe_command_execute_unit_task_ror(const uint8_t *command, size_t size,
                                      const AoeCommandPlayerTaskOps *ops);
int aoe_command_execute_unit_task_list_ror(const uint8_t *command, size_t size,
                                           const AoeCommandPlayerTaskOps *ops);
int aoe_command_execute_resign_ror(const uint8_t *command, size_t size,
                                   const AoeCommandPlayerTaskOps *ops);
int aoe_command_execute_unit_records_ror(const uint8_t *command, size_t size,
                                         const AoeCommandPlayerTaskOps *ops);
int aoe_command_execute_unit_record_list_ror(
    const uint8_t *command, size_t size, const AoeCommandPlayerTaskOps *ops);

#endif
