#include <assert.h>
#include <string.h>

#include "tribe_civ_setup.h"

static char logged[64];
static void log_line(void *c, const char *t) {
    (void)c;
    strncpy(logged, t, sizeof(logged) - 1);
}

static int count_set(const int32_t *v) {
    int i, n = 0;
    for (i = 0; i < AOE_TRIBE_SETUP_VALUES; ++i) n += v[i] != 0;
    return n;
}

int main(void) {
    int32_t v[AOE_TRIBE_SETUP_VALUES];
    AoeTribeSetupEnvRor env = {0, 2, NULL, log_line};

    /* Egyptians, map type 1: eleven entries plus the 0/1 entry. */
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_egyptians_ror(v, 1, &env);
    assert(strcmp(logged, "Setting up as Egyptians.") == 0);
    assert(v[3] == 10 && v[19] == 50 && v[46] == 120 && v[82] == 20 &&
           v[83] == 200 && count_set(v) == 12);
    /* Map type 3 adds nothing beyond the civilisation's entries. */
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_egyptians_ror(v, 3, &env);
    assert(v[83] == 0 && count_set(v) == 11);
    /* +0x993 below 1: only the gate entry. */
    env.byte_993 = 0;
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_egyptians_ror(v, 1, &env);
    assert(v[35] == 100 && count_set(v) == 1);
    /* Babylonians have no gate. */
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_babylonians_ror(v, 8, &env);
    assert(v[77] == 30 && v[0] == 20 && count_set(v) == 13);

    /* Option +0xae5 == 1: only the map-type pair. */
    env.option_ae5 = 1;
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_roman_ror(v, 0, &env);
    assert(v[124] == 100 && count_set(v) == 1);
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_roman_ror(v, 6, &env);
    assert(v[120] == 60 && v[124] == 40 && count_set(v) == 2);
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_roman_ror(v, 5, &env);
    assert(v[120] == 80 && v[124] == 20);
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_roman_ror(v, 4, &env);
    assert(v[120] == 100 && count_set(v) == 1);

    /* Shang (14 entries), map type 2. */
    env.option_ae5 = 0;
    env.byte_993 = 1;
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_shang_ror(v, 2, &env);
    assert(v[55] == 100 && v[56] == 50 && v[93] == 80 && count_set(v) == 15);
    /* Persians reach the last entry (130). */
    memset(v, 0, sizeof(v));
    aoe_tribe_setup_persians_ror(v, 0, &env);
    assert(v[130] == 50 && v[126] == 60 && v[90] == 200 && v[91] == 200);

    /* The civilisation switch. */
    memset(v, 0, sizeof(v));
    assert(aoe_tribe_setup_civ_ror(16, v, 1, &env) == 1);
    assert(strcmp(logged, "Setting up as Macedonian.") == 0 && v[105] == 200);
    assert(aoe_tribe_setup_civ_ror(0, v, 1, &env) == 0);
    assert(aoe_tribe_setup_civ_ror(17, v, 1, &env) == 0);
    {
        static const char *names[16] = {
            "Egyptians", "Greeks", "Babylonians", "Assyrians", "Minoans",
            "Hittites", "Phoenicians", "Sumerians", "Persians", "Shang",
            "Yamato", "Choson", "Roman", "Carthaginian", "Palmyran",
            "Macedonian"};
        int c;
        for (c = 1; c <= 16; ++c) {
            char expect[64] = "Setting up as ";
            strcat(expect, names[c - 1]);
            strcat(expect, ".");
            aoe_tribe_setup_civ_ror((uint8_t)c, v, 1, &env);
            assert(strcmp(logged, expect) == 0);
        }
    }
    return 0;
}
