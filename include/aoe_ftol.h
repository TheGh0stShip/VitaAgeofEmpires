#ifndef VITA_AOE_FTOL_H
#define VITA_AOE_FTOL_H

#include <stdint.h>

/* MSVC __ftol (patched RoR 1.0a 0x0052602c): switch the x87 control word
   to truncation, `fistp` a 64-bit integer, and return its low 32 bits in
   EAX. NaN, infinities and values outside the signed 64-bit range store
   the integer-indefinite 0x8000000000000000, whose low half is 0. Every
   float/double inside that range converts exactly, so this is bit-exact on
   hosts and on ARMv7 VFP, unlike a plain C cast. */
static inline int32_t aoe_ftol32(double value) {
    uint32_t low;
    if (!(value >= -9223372036854775808.0 && value < 9223372036854775808.0))
        return 0;
    low = (uint32_t)(uint64_t)(int64_t)value;
    return low <= INT32_MAX ? (int32_t)low : -(int32_t)(UINT32_MAX - low) - 1;
}

#endif
