#ifndef VITA_AOE_COMMAND_BUILDERS_H
#define VITA_AOE_COMMAND_BUILDERS_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a command builders (0x0042b730..0x0042bf00).
   The original callocs each record and hands it to the command queue
   0x0042b6e0; here the record is written to `out` and its size returned
   (0 when the original builds nothing or `out` is too small). Objects
   are given as their command-relevant fields; a NULL object becomes ID
   -1 as in the original. Layouts match the executors in command_order.h
   and command_player_task.h. */

typedef struct {
    int32_t id;          /* object +0x04 */
    uint8_t template_65; /* object template (+8) byte +0x65 */
} AoeCommandObjectRor;

/* Session fields of opcode 0, read only when 0x0041b760 reports a
   session: the local player (world +0x7c), its +0x104 byte, its masks
   +0xd8 | +0xd4, and the map cell at row ftol(y), column ftol(x) of the
   global table 0x007d205c. */
typedef struct {
    int session;
    uint8_t local_player;
    uint8_t player_104;
    uint32_t mask;
    void *context;
    uint32_t (*cell)(void *context, int32_t row, int32_t column);
} AoeCommandSessionRor;

/* 0x0042b730, opcode 0: units (count > 0), target ID and its template
   byte +0x65 at +0x11 (-1 and 0 without a target), x +8, y +0xc; then the
   session fields +0x10 (player), +0x12, +0x14, +0x18. */
size_t aoe_command_build_order_ror(uint8_t *out, size_t out_size,
                                   const AoeCommandObjectRor *const *units,
                                   int16_t count,
                                   const AoeCommandObjectRor *target,
                                   float x, float y,
                                   const AoeCommandSessionRor *session);
/* 0x0042b8d0, opcode 2 from objects; 0x0042b970, opcode 2 from raw IDs
   and a target ID. */
size_t aoe_command_build_order_alternate_ror(
    uint8_t *out, size_t out_size, const AoeCommandObjectRor *const *units,
    int16_t count, const AoeCommandObjectRor *target, float x, float y);
size_t aoe_command_build_order_alternate_ids_ror(uint8_t *out,
                                                 size_t out_size,
                                                 const int32_t *ids,
                                                 int32_t count,
                                                 int32_t target_id, float x,
                                                 float y);
/* 0x0042b9f0, opcode 3: the same layout as opcode 2 from objects (units
   from +0x1c, target ID +4, x +8, y +0xc). */
size_t aoe_command_build_order_target_ror(
    uint8_t *out, size_t out_size, const AoeCommandObjectRor *const *units,
    int16_t count, const AoeCommandObjectRor *target, float x, float y);
/* 0x0042b850, opcode 1 (stop): count +1, unit IDs from +2. */
size_t aoe_command_build_stop_ror(uint8_t *out, size_t out_size,
                                  const AoeCommandObjectRor *const *units,
                                  int16_t count);
/* 0x0042bb60, opcode 6: count (+4 as count & 0xff), pattern +8. */
size_t aoe_command_build_arrange_ror(uint8_t *out, size_t out_size,
                                     const AoeCommandObjectRor *const *units,
                                     uint32_t count, int32_t pattern);

/* 0x0042bd70, opcode 0x11: the task fields of each listed unit. */
typedef struct {
    uint8_t player;    /* +0x01 */
    uint8_t mode;      /* +0x02 */
    int16_t value_16;  /* +0x04 */
    uint8_t extra_c;   /* +0x06 */
    int32_t value_32;  /* +0x08 */
    uint8_t flag;      /* +0x0c */
    uint32_t x_bits, y_bits, z_bits; /* +0x10..+0x18 */
    uint32_t extra_32; /* +0x1c */
    uint8_t extra_a, extra_b; /* +0x20, +0x21 */
} AoeCommandTaskFieldsRor;
size_t aoe_command_build_task_list_ror(
    uint8_t *out, size_t out_size, const AoeCommandObjectRor *const *units,
    int16_t count, const AoeCommandTaskFieldsRor *fields);
/* 0x0042be50, opcode 0x0b (4 bytes): player, slot, value. */
size_t aoe_command_build_resign_ror(uint8_t *out, size_t out_size,
                                    uint8_t player, uint8_t slot,
                                    uint8_t value);
/* 0x0042be90, opcode 0x0c (0x18 bytes): player, unit ID +4, count +8 and
   3-byte records from +9 (at most five fit). */
size_t aoe_command_build_unit_records_ror(uint8_t *out, size_t out_size,
                                          uint8_t player, int32_t unit_id,
                                          const uint8_t *records,
                                          int32_t count);
/* 0x0042bf00, opcode 0x10: count +1, record bytes +2/+3, IDs from +4. */
size_t aoe_command_build_record_list_ror(
    uint8_t *out, size_t out_size, const AoeCommandObjectRor *const *units,
    int16_t count, uint8_t a, uint8_t b);

/* The duplicate filter of the command sender: the last record sent, its
   size and send time (timeGetTime). */
typedef struct {
    const uint8_t *last;
    size_t last_size;
    uint32_t time_ms;
} AoeCommandRepeatRor;

/* 0x0042b550: two order records (opcodes 0/2/3) with the same count,
   (int16)ftol(x), (int16)ftol(y), target and IDs; 0x0042b5f0: two stop
   records (opcode 1) with the same count and IDs (+2); 0x0042b640 picks
   by opcode (others are never repeats). */
int aoe_command_same_order_ror(const uint8_t *a, const uint8_t *b);
int aoe_command_same_stop_ror(const uint8_t *a, const uint8_t *b);
int aoe_command_same_ror(const uint8_t *last, const uint8_t *command);
/* 0x0042b690: a repeat of the last record (same size and opcode) within
   500 ms. Records shorter than their headers are never repeats here. */
int aoe_command_is_repeat_ror(const AoeCommandRepeatRor *repeat,
                              const uint8_t *command, size_t size,
                              uint32_t now_ms);

#endif
