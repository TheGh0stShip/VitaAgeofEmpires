#include "aoe_rng.h"

#include <assert.h>
#include <stdint.h>

int main(void) {
    AoeRng rng = {1}; /* Both original PE images initialize the seed to 1. */
    const uint16_t expected[] = {41, 18467, 6334, 26500,
                                 19169, 15724, 11478, 29358};
    const uint32_t states[] = {0x0029e2c0, 0xc823f683, 0x18be873a,
                               0xe7847115, 0xcae1df84, 0x3d6c1037,
                               0x2cd60a1e, 0x72ae7869};
    for (unsigned i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        assert(aoe_rng_next(&rng) == expected[i]);
        assert(rng.state == states[i]);
    }
    uint32_t saved = rng.state;
    uint16_t next = aoe_rng_next(&rng);
    aoe_rng_seed(&rng, saved);
    assert(aoe_rng_next(&rng) == next);
    assert(aoe_rng_next(0) == 0);
    aoe_rng_seed(0, 1);
    return 0;
}
