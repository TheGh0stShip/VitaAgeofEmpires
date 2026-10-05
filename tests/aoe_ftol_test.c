#include "aoe_ftol.h"

#include <assert.h>
#include <math.h>

int main(void) {
    /* Truncation toward zero. */
    assert(aoe_ftol32(2.75) == 2 && aoe_ftol32(-2.75) == -2);
    assert(aoe_ftol32(-0.5) == 0 && aoe_ftol32(0.0) == 0);
    /* Low 32 bits of the 64-bit fistp result. */
    assert(aoe_ftol32(2147483648.0) == INT32_MIN);
    assert(aoe_ftol32(4294967296.0) == 0);
    assert(aoe_ftol32(4294967297.0) == 1);
    assert(aoe_ftol32(-2147483649.0) == INT32_MAX);
    assert(aoe_ftol32(-9223372036854775808.0) == 0);
    /* Integer-indefinite for NaN, infinities and >= 2^63. */
    assert(aoe_ftol32(NAN) == 0 && aoe_ftol32(INFINITY) == 0 &&
           aoe_ftol32(-INFINITY) == 0);
    assert(aoe_ftol32(9223372036854775808.0) == 0);
    assert(aoe_ftol32((double)16777217.0f) == 16777216);
    return 0;
}
