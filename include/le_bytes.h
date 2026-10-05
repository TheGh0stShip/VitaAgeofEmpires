#ifndef VITA_AOE_LE_BYTES_H
#define VITA_AOE_LE_BYTES_H

#include <stdint.h>
#include <string.h>

/* Byte-wise little-endian access for original 32-bit records. These never
   depend on host endianness, alignment, or structure layout, so the same
   bytes decode identically on LP64/LLP64 hosts and on ARMv7 Vita. */
static inline uint32_t aoe_le_u32(const uint8_t *in) {
    return (uint32_t)in[0] | (uint32_t)in[1] << 8 |
           (uint32_t)in[2] << 16 | (uint32_t)in[3] << 24;
}

static inline int32_t aoe_le_i32(const uint8_t *in) {
    uint32_t value = aoe_le_u32(in);
    return value <= INT32_MAX ? (int32_t)value
                              : -(int32_t)(UINT32_MAX - value) - 1;
}

/* Exact IEEE-754 single bit pattern; no numeric conversion. */
static inline float aoe_le_f32(const uint8_t *in) {
    uint32_t bits = aoe_le_u32(in);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static inline uint16_t aoe_le_u16(const uint8_t *in) {
    return (uint16_t)(in[0] | in[1] << 8);
}

static inline void aoe_le_put_u16(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
}

static inline void aoe_le_put_u32(uint8_t *out, uint32_t value) {
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
}

static inline void aoe_le_put_f32(uint8_t *out, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    aoe_le_put_u32(out, bits);
}

#endif
