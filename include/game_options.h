#ifndef VITA_AOE_GAME_OPTIONS_H
#define VITA_AOE_GAME_OPTIONS_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a game settings read during scenario load and
   game start (game object 0x00580e38). Names follow their offsets where
   the executable's own text does not identify them; defaults are those
   stored by the game object's constructor (around 0x004fe0d3). */
typedef struct {
    uint8_t option_ad6;      /* +0xad6, default 1; 0 applies effect 105 */
    uint8_t option_ad7;      /* +0xad7, default 0; 1 applies effect 100 */
    uint8_t option_ad9;      /* +0xad9, default 0; set: effect 210 replaces
                                the civ tech trees */
    int32_t resource_level;  /* +0xadc, default 0; 1..3 starting stocks */
    int32_t starting_age;    /* +0xae0, default 0 ("Default"); strings
                                0x1067 + n: 1 Nomad, 2 Stone, 3 Tool,
                                4 Bronze, 5 Iron, 6 Post-Iron */
    uint8_t option_ae5;      /* +0xae5, default 0; set: 20000/20000/5000/
                                10000 food/wood/stone/gold */
    uint8_t population_limit; /* 0x005054f0: 50 unless +0x97e selects
                                 +0xae6 (default 50) */
    uint8_t option_985;      /* +0x985 (0x0041b7c0), default 0 */
    int32_t mode_1b0;        /* +0x1b0; 7 is the scenario editor */
    int32_t mode_1b4;        /* +0x1b4; 7 is the scenario editor */
} AoeGameOptionsRor;

static inline AoeGameOptionsRor aoe_game_options_default_ror(void) {
    AoeGameOptionsRor o = {1, 0, 0, 0, 0, 0, 50, 0, 0, 0};
    return o;
}

#endif
