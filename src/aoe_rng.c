#include "aoe_rng.h"

void aoe_rng_seed(AoeRng *rng, uint32_t seed) {
    if (rng) rng->state = seed;
}

uint16_t aoe_rng_next(AoeRng *rng) {
    if (!rng) return 0;
    rng->state = rng->state * UINT32_C(0x343fd) + UINT32_C(0x269ec3);
    return (uint16_t)((rng->state >> 16) & UINT32_C(0x7fff));
}
