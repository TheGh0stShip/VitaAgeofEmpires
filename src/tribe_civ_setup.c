#include "tribe_civ_setup.h"

#include <stddef.h>

typedef struct {
    int16_t index;
    int16_t value;
} Entry;

#define END {-1, 0}

typedef struct {
    const char *text;
    int16_t gate;           /* entry set to 100 when +0x993 < 1, or -1 */
    Entry base[16];
    Entry open[3];          /* map types 0 and 1 */
    Entry closed[3];        /* map types 2, 5, 6 and 8 */
    int16_t mode_a, mode_b; /* option +0xae5 == 1 */
} Spec;

static void put(int32_t *values, const Entry *e) {
    for (; e->index >= 0; ++e)
        if (e->index < AOE_TRIBE_SETUP_VALUES) values[e->index] = e->value;
}

static void run(const Spec *spec, int32_t *values, int32_t map,
                const AoeTribeSetupEnvRor *env) {
    if (!values || !env) return;
    if (env->log) env->log(env->context, spec->text);
    if (env->option_ae5 == 1) {
        if (map == 0 || map == 1) {
            values[spec->mode_a] = 100;
        } else if (map == 2 || map == 6) {
            values[spec->mode_b] = 60;
            values[spec->mode_a] = 40;
        } else if (map == 5 || map == 8) {
            values[spec->mode_b] = 80;
            values[spec->mode_a] = 20;
        } else {
            values[spec->mode_b] = 100;
        }
        return;
    }
    if (spec->gate >= 0 && (int32_t)env->byte_993 < 1) {
        values[spec->gate] = 100;
        return;
    }
    put(values, spec->base);
    if (map == 0 || map == 1)
        put(values, spec->open);
    else if (map == 2 || map == 6 || map == 8 || map == 5)
        put(values, spec->closed);
}

static const Spec egyptians = {
    "Setting up as Egyptians.", 35,
    {{3, 10}, {9, 10}, {10, 10}, {19, 50}, {20, 50}, {18, 40}, {46, 120},
     {47, 100}, {80, 100}, {81, 30}, {82, 20}, END},
    {{83, 200}, END}, {{83, 100}, END}, 63, 25};
static const Spec greeks = {
    "Setting up as Greeks.", 36,
    {{9, 10}, {10, 10}, {6, 30}, {11, 30}, {3, 70}, {4, 50}, {7, 20},
     {48, 100}, {84, 100}, {129, 40}, END},
    {{85, 200}, {86, 200}, END}, {{85, 100}, {86, 100}, END}, 64, 26};
static const Spec babylonians = {
    "Setting up as Babylonians.", -1,
    {{1, 10}, {6, 10}, {9, 10}, {11, 10}, {0, 20}, {19, 40}, {20, 40},
     {16, 30}, {42, 100}, {43, 100}, {3, 20}, {10, 20}, END},
    {{77, 60}, END}, {{77, 30}, END}, 61, 23};
static const Spec assyrians = {
    "Setting up as Assyrians.", 34,
    {{3, 10}, {7, 10}, {9, 10}, {10, 10}, {16, 40}, {72, 80}, {73, 80},
     {74, 80}, {11, 40}, {6, 20}, {19, 30}, {40, 100}, {41, 100}, END},
    {{75, 100}, {76, 100}, END}, {{75, 50}, {76, 50}, END}, 60, 22};
static const Spec minoans = {
    "Setting up as Minoans.", 37,
    {{4, 10}, {6, 10}, {9, 10}, {10, 10}, {11, 10}, {0, 100}, {3, 20},
     {51, 100}, END},
    {{88, 200}, {89, 200}, END}, {{88, 100}, {89, 100}, END}, 66, 28};
static const Spec hittites = {
    "Setting up as Hittites.", -1,
    {{9, 10}, {10, 10}, {11, 10}, {49, 100}, {50, 100}, {16, 80}, {3, 30},
     {4, 40}, {6, 20}, {125, 60}, {18, 40}, END},
    {{87, 160}, END}, {{87, 80}, END}, 65, 27};
static const Spec phoenicians = {
    "Setting up as Phoenicians.", -1,
    {{3, 10}, {4, 10}, {9, 10}, {10, 10}, {11, 10}, {0, 40}, {6, 20},
     {18, 70}, {19, 20}, {20, 30}, {53, 140}, END},
    {{92, 200}, END}, {{92, 100}, END}, 68, 30};
static const Spec sumerians = {
    "Setting up as Sumerians.", 38,
    {{3, 10}, {9, 10}, {10, 10}, {11, 10}, {16, 30}, {4, 20}, {57, 100},
     {58, 100}, {127, 100}, END},
    {{94, 200}, {95, 200}, END}, {{94, 100}, {95, 100}, END}, 70, 32};
static const Spec persians = {
    "Setting up as Persians.", -1,
    {{6, 10}, {9, 10}, {10, 10}, {126, 60}, {18, 60}, {1, 40}, {11, 40},
     {0, 20}, {16, 20}, {7, 50}, {52, 100}, {130, 50}, END},
    {{90, 200}, {91, 200}, END}, {{90, 100}, {91, 100}, END}, 67, 29};
static const Spec shang = {
    "Setting up as Shang.", -1,
    {{0, 10}, {3, 10}, {9, 10}, {10, 10}, {6, 50}, {7, 50}, {56, 50},
     {11, 30}, {20, 30}, {1, 20}, {16, 20}, {19, 20}, {54, 100}, {55, 100},
     END},
    {{93, 160}, END}, {{93, 80}, END}, 69, 31};
static const Spec yamato = {
    "Setting up as Yamato.", 39,
    {{3, 10}, {4, 10}, {9, 10}, {10, 10}, {6, 50}, {7, 50}, {0, 20},
     {1, 20}, {16, 40}, {11, 20}, {59, 100}, END},
    {{96, 200}, {97, 200}, END}, {{96, 100}, {97, 100}, END}, 71, 33};
static const Spec choson = {
    "Setting up as Choson.", -1,
    {{1, 10}, {16, 10}, {3, 10}, {7, 30}, {9, 30}, {10, 30}, {11, 70},
     {128, 70}, {0, 20}, {6, 20}, {44, 100}, {45, 100}, END},
    {{78, 200}, {79, 200}, END}, {{78, 100}, {79, 100}, END}, 62, 24};
static const Spec roman = {
    "Setting up as Roman.", -1,
    {{0, 10}, {6, 10}, {9, 10}, {10, 10}, {3, 20}, {11, 80}, {114, 80},
     {4, 20}, {113, 60}, {112, 100}, END},
    {{115, 200}, {116, 200}, END}, {{115, 100}, {116, 100}, END}, 124,
    120};
static const Spec carthaginian = {
    "Setting up as Carthaginian.", -1,
    {{0, 10}, {1, 10}, {16, 10}, {7, 10}, {9, 10}, {10, 10}, {11, 10},
     {3, 70}, {4, 50}, {6, 30}, {18, 60}, {98, 80}, {99, 100}, END},
    {{100, 200}, {101, 200}, END}, {{100, 100}, {101, 100}, END}, 121,
    117};
static const Spec palmyran = {
    "Setting up as Palmyran.", -1,
    {{3, 40}, {6, 40}, {0, 30}, {7, 10}, {9, 10}, {10, 10}, {11, 10},
     {1, 30}, {107, 80}, {108, 80}, {16, 60}, {4, 20}, {109, 100}, END},
    {{110, 200}, {111, 200}, END}, {{110, 100}, {111, 100}, END}, 123,
    119};
static const Spec macedonian = {
    "Setting up as Macedonian.", -1,
    {{3, 40}, {4, 40}, {0, 10}, {6, 30}, {7, 30}, {1, 10}, {104, 80},
     {103, 80}, {16, 20}, {102, 100}, END},
    {{105, 200}, {106, 200}, END}, {{105, 100}, {106, 100}, END}, 122,
    118};

#define SETUP(name, spec)                                                 \
    void aoe_tribe_setup_##name##_ror(int32_t *values, int32_t map_type,  \
                                      const AoeTribeSetupEnvRor *env) {  \
        run(&spec, values, map_type, env);                                \
    }

SETUP(egyptians, egyptians)
SETUP(greeks, greeks)
SETUP(babylonians, babylonians)
SETUP(assyrians, assyrians)
SETUP(minoans, minoans)
SETUP(hittites, hittites)
SETUP(phoenicians, phoenicians)
SETUP(sumerians, sumerians)
SETUP(persians, persians)
SETUP(shang, shang)
SETUP(yamato, yamato)
SETUP(choson, choson)
SETUP(roman, roman)
SETUP(carthaginian, carthaginian)
SETUP(palmyran, palmyran)
SETUP(macedonian, macedonian)

int aoe_tribe_setup_civ_ror(uint8_t civ, int32_t *values, int32_t map_type,
                            const AoeTribeSetupEnvRor *env) {
    static const Spec *const specs[16] = {
        &egyptians, &greeks,   &babylonians, &assyrians,
        &minoans,   &hittites, &phoenicians, &sumerians,
        &persians,  &shang,    &yamato,      &choson,
        &roman,     &carthaginian, &palmyran, &macedonian};
    if (civ < 1 || civ > 16) return 0;
    run(specs[civ - 1], values, map_type, env);
    return 1;
}
