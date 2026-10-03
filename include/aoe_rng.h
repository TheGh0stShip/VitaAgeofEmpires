#ifndef VITA_AOE_RNG_H
#define VITA_AOE_RNG_H

#include <stdint.h>

/* MSVC CRT srand/rand. Patched RoR 1.0a: 0x00526053/0x0052605d, seed
   0x00560664. Patched AoE 1.0c: 0x0051fc8f/0x0051fc99, seed 0x005591ec.
   (Earlier notes cited unpatched-release 0x00528410/0x00528420 and
   0x00532a70/0x00532a80.) Each executable keeps a single 32-bit global
   seed, initialized to one; the portable state can be owned by a world
   instance. */
typedef struct {
    uint32_t state;
} AoeRng;

void aoe_rng_seed(AoeRng *rng, uint32_t seed);
uint16_t aoe_rng_next(AoeRng *rng);

#endif
