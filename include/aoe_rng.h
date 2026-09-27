#ifndef VITA_AOE_RNG_H
#define VITA_AOE_RNG_H

#include <stdint.h>

/* Rise of Rome 0x00528410/0x00528420 and base 0x00532a70/0x00532a80.
   Each original executable keeps a single 32-bit global seed, initialized
   to one. The portable state can be owned by a world instance. */
typedef struct {
    uint32_t state;
} AoeRng;

void aoe_rng_seed(AoeRng *rng, uint32_t seed);
uint16_t aoe_rng_next(AoeRng *rng);

#endif
