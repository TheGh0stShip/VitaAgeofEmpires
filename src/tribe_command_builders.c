#include "tribe_command_builders.h"

#include <string.h>

#include "le_bytes.h"

static void put16(uint8_t *p, int16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)((uint16_t)v >> 8);
}

static void put32(uint8_t *p, uint32_t v) { aoe_le_put_u32(p, v); }

static void putf(uint8_t *p, float v) { aoe_le_put_f32(p, v); }

/* Zero `size` bytes of `out` when they fit. */
static int room(uint8_t *out, size_t out_size, size_t size) {
    if (!out || out_size < size) return 0;
    memset(out, 0, size);
    return 1;
}

static void put_units(uint8_t *p, const AoeTribeCommandUnitRor *const *units,
                      int16_t count) {
    int16_t i;
    for (i = 0; i < count; ++i)
        put32(p + i * 4, units[i] ? (uint32_t)units[i]->id : 0xffffffffu);
}

static uint8_t first_owner(const AoeTribeCommandUnitRor *const *units) {
    return units[0] ? (uint8_t)units[0]->owner : 0xff;
}

size_t aoe_tribe_build_66_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, int16_t value, float x,
                              float y) {
    size_t size = (size_t)count * 4 + 0x14;
    if (!units || count <= 0 || !room(out, out_size, size)) return 0;
    put_units(out + 0x14, units, count);
    out[0] = 0x66;
    out[1] = (uint8_t)count;
    out[2] = first_owner(units);
    put16(out + 0xc, value);
    putf(out + 4, x);
    putf(out + 8, y);
    put32(out + 0x10, 0xffffffffu);
    return size;
}

size_t aoe_tribe_build_66_ids_ror(uint8_t *out, size_t out_size,
                                  uint8_t player, const int32_t *ids,
                                  int32_t count, int16_t value, float x,
                                  float y, int32_t field_10) {
    size_t size = (size_t)count * 4 + 0x14;
    int32_t i;
    if (!ids || count <= 0 || !room(out, out_size, size)) return 0;
    for (i = 0; i < count; ++i) put32(out + 0x14 + i * 4, (uint32_t)ids[i]);
    put16(out + 0xc, value);
    putf(out + 8, y);
    putf(out + 4, x);
    out[2] = player;
    out[0] = 0x66;
    out[1] = (uint8_t)count;
    put32(out + 0x10, (uint32_t)field_10);
    return size;
}

static uint8_t clamp_byte(int32_t v) {
    return v < 0 ? 0 : v > 0xff ? 0xff : (uint8_t)v;
}

size_t aoe_tribe_build_69_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, int16_t value, int32_t a,
                              int32_t b, int32_t c, int32_t d) {
    size_t size = (size_t)count * 4 + 0x10;
    if (!units || count <= 0 || !room(out, out_size, size)) return 0;
    put_units(out + 0x10, units, count);
    out[0] = 0x69;
    out[1] = (uint8_t)count;
    out[2] = first_owner(units);
    put16(out + 8, value);
    out[3] = clamp_byte(a);
    out[6] = clamp_byte(d);
    out[4] = clamp_byte(b);
    out[5] = clamp_byte(c);
    put32(out + 0xc, 0xffffffffu);
    return size;
}

size_t aoe_tribe_build_67_value_ror(uint8_t *out, size_t out_size,
                                    int16_t a, int16_t b, int16_t value) {
    if (!room(out, out_size, 0x10)) return 0;
    put16(out + 2, a);
    put16(out + 4, b);
    put16(out + 0xc, value);
    out[0] = 0x67;
    out[1] = 0;
    putf(out + 8, (float)value);
    return 0x10;
}

size_t aoe_tribe_build_67_word_ror(uint8_t *out, size_t out_size,
                                   uint32_t word) {
    if (!room(out, out_size, 0x10)) return 0;
    put32(out + 8, word);
    out[0] = 0x67;
    out[1] = 1;
    return 0x10;
}

size_t aoe_tribe_build_67_byte_ror(uint8_t *out, size_t out_size, int16_t a,
                                   uint8_t b) {
    if (!room(out, out_size, 0x10)) return 0;
    put16(out + 2, a);
    put16(out + 4, (int16_t)(uint16_t)b);
    put32(out + 8, 0);
    out[0] = 0x67;
    out[1] = 5;
    return 0x10;
}

size_t aoe_tribe_build_67_pair_ror(uint8_t *out, size_t out_size, int16_t a,
                                   int16_t b) {
    if (!room(out, out_size, 0x10)) return 0;
    put16(out + 2, a);
    put16(out + 4, b);
    out[0] = 0x67;
    out[1] = 6;
    return 0x10;
}

size_t aoe_tribe_build_6a_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *unit) {
    /* The original reads the unit unconditionally; the port needs one. */
    if (!unit || !room(out, out_size, 8)) return 0;
    out[0] = 0x6a;
    put32(out + 4, (uint32_t)unit->id);
    return 8;
}

size_t aoe_tribe_build_6b_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, uint32_t a, uint32_t b) {
    size_t size = (size_t)count * 4 + 0xc;
    if (!units || count <= 0 || !room(out, out_size, size)) return 0;
    put_units(out + 0xc, units, count);
    put32(out + 4, a);
    out[0] = 0x6b;
    out[1] = (uint8_t)count;
    put32(out + 8, b);
    return size;
}

size_t aoe_tribe_build_6d_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, uint32_t value) {
    size_t size = (size_t)count * 4 + 8;
    if (!units || count <= 0 || !room(out, out_size, size)) return 0;
    put_units(out + 8, units, count);
    put32(out + 4, value);
    out[0] = 0x6d;
    out[1] = (uint8_t)count;
    return size;
}

size_t aoe_tribe_build_6d_one_ror(uint8_t *out, size_t out_size, int32_t id,
                                  uint32_t value) {
    if (!room(out, out_size, 0xc)) return 0;
    put32(out + 8, (uint32_t)id);
    put32(out + 4, value);
    out[0] = 0x6d;
    out[1] = 1;
    return 0xc;
}

size_t aoe_tribe_build_repair_ror(uint8_t *out, size_t out_size,
                                  const AoeTribeCommandUnitRor *const *units,
                                  int16_t count,
                                  const AoeTribeCommandUnitRor *source) {
    size_t size = (size_t)count * 4 + 8;
    /* The original reads the source unconditionally; the port needs one. */
    if (!units || count <= 0 || !source || !room(out, out_size, size))
        return 0;
    put_units(out + 8, units, count);
    put32(out + 4, (uint32_t)source->id);
    out[0] = 0x6e;
    out[1] = (uint8_t)count;
    return size;
}

size_t aoe_tribe_build_6f_ror(uint8_t *out, size_t out_size,
                              const AoeTribeCommandUnitRor *const *units,
                              int16_t count, float x, float y) {
    size_t size = (size_t)count * 4 + 0xc;
    if (!units || count <= 0 || !room(out, out_size, size)) return 0;
    put_units(out + 0xc, units, count);
    putf(out + 4, x);
    out[0] = 0x6f;
    out[1] = (uint8_t)count;
    putf(out + 8, y);
    return size;
}
