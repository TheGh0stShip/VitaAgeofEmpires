#ifndef VITA_AOE_TRIBE_COMMAND_BUILDERS_H
#define VITA_AOE_TRIBE_COMMAND_BUILDERS_H

#include <stddef.h>
#include <stdint.h>

/* Patched Rise of Rome 1.0a tribe command builders (0x004e95d0..
   0x004e9ba0). As in command_builders.h, the original callocs each
   record and queues it; here it is written to `out` (zero-filled first)
   and its size returned, 0 when the original builds nothing or `out` is
   too small. Units are given by ID and owning player; a NULL unit becomes
   ID -1. Layouts match the executors in command_tribe_unit.h. */

typedef struct {
    int32_t id;     /* object +0x04 */
    int16_t owner;  /* object +0x0c player's +0x4a */
} AoeTribeCommandUnitRor;

/* 0x004e95d0, opcode 0x66: units (count > 0) from +0x14, count +1, the
   first unit's owner +2 (0xff without one), int16 +0xc, x +4, y +8,
   -1 at +0x10. 0x004e9680: the same from raw IDs with the player, int16,
   x, y and +0x10 given. */
size_t aoe_tribe_build_66_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, int16_t value, float x, float y);
size_t aoe_tribe_build_66_ids_ror(uint8_t *out, size_t out_size,
                                  uint8_t player, const int32_t *ids,
                                  int32_t count, int16_t value, float x,
                                  float y, int32_t field_10);
/* 0x004e9710, opcode 0x69: units from +0x10, count +1, owner +2, int16
   +8, four bytes +3..+6 clamped to 0..255, -1 at +0xc. */
size_t aoe_tribe_build_69_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, int16_t value, int32_t a,
                              int32_t b, int32_t c, int32_t d);
/* Opcode 0x67 (16 bytes) by sub-command +1: 0x004e9890 sub 0 (int16 +2,
   +4, float +8 and int16 +0xc of one value), 0x004e98f0 sub 1 (+8),
   0x004e9840 sub 5 (int16 +2, a byte widened at +4, 0 at +8),
   0x004e99d0 sub 6 (int16 +2, +4). */
size_t aoe_tribe_build_67_value_ror(uint8_t *out, size_t out_size,
                                    int16_t a, int16_t b, int16_t value);
size_t aoe_tribe_build_67_word_ror(uint8_t *out, size_t out_size,
                                   uint32_t word);
size_t aoe_tribe_build_67_byte_ror(uint8_t *out, size_t out_size, int16_t a,
                                   uint8_t b);
size_t aoe_tribe_build_67_pair_ror(uint8_t *out, size_t out_size, int16_t a,
                                   int16_t b);
/* 0x004e9920, opcode 0x6a (8 bytes): the unit's ID at +4. */
size_t aoe_tribe_build_6a_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *unit);
/* 0x004e9950, opcode 0x6b: units from +0xc, count +1, +4 and +8. */
size_t aoe_tribe_build_6b_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, uint32_t a, uint32_t b);
/* 0x004e9a60, opcode 0x6d: units from +8, count +1, value +4.
   0x004e9ae0: one raw ID at +8, count 1, value +4. */
size_t aoe_tribe_build_6d_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, uint32_t value);
size_t aoe_tribe_build_6d_one_ror(uint8_t *out, size_t out_size, int32_t id,
                                  uint32_t value);
/* 0x004e9b20, opcode 0x6e (repair): units from +8, count +1, the
   source's ID +4. 0x004e9ba0, opcode 0x6f: units from +0xc, count +1,
   x +4, y +8. */
size_t aoe_tribe_build_repair_ror(uint8_t *out, size_t out_size,
                                  const AoeTribeCommandUnitRor *const *units,
                                  int16_t count,
                                  const AoeTribeCommandUnitRor *source);
size_t aoe_tribe_build_6f_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, float x, float y);

#endif
