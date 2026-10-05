#ifndef VITA_AOE_TRIBE_CIV_SETUP_H
#define VITA_AOE_TRIBE_CIV_SETUP_H

#include <stdint.h>

/* Patched Rise of Rome 1.0a computer-player civilisation set-up
   (tribe player, vtables 0x00549a44 / 0x00549b80). Each routine logs
   "Setting up as <civ>." through the player's computer-player object
   (+0x23c) and writes weights into the 131-entry table that the
   build-list chooser zeroes and passes in. Indices are table entries
   (byte offset / 4).

   Every routine has the same shape:
   - when game option +0xae5 is 1, only a map-type pair (A, B) is set:
     map types 0/1: A = 100; 2/6: B = 60, A = 40; 5/8: B = 80, A = 20;
     anything else: B = 100;
   - otherwise some civilisations first test game byte +0x993: below 1
     only one entry becomes 100; then the civilisation's entries are
     written, followed by map-type entries for 0/1 and for 2/5/6/8 (other
     map types add nothing). */

enum { AOE_TRIBE_SETUP_VALUES = 131 };

typedef struct {
    uint8_t option_ae5;  /* global game +0xae5 */
    uint8_t byte_993;    /* global game +0x993 (zero-extended) */
    void *context;
    /* The computer-player object's log line. */
    void (*log)(void *context, const char *text);
} AoeTribeSetupEnvRor;

void aoe_tribe_setup_egyptians_ror(int32_t *values, int32_t map_type,
                                   const AoeTribeSetupEnvRor *env); /* 0x004f5430 */
void aoe_tribe_setup_greeks_ror(int32_t *values, int32_t map_type,
                                const AoeTribeSetupEnvRor *env);    /* 0x004f5570 */
void aoe_tribe_setup_babylonians_ror(int32_t *values, int32_t map_type,
                                     const AoeTribeSetupEnvRor *env); /* 0x004f56b0 */
void aoe_tribe_setup_assyrians_ror(int32_t *values, int32_t map_type,
                                   const AoeTribeSetupEnvRor *env); /* 0x004f57d0 */
void aoe_tribe_setup_minoans_ror(int32_t *values, int32_t map_type,
                                 const AoeTribeSetupEnvRor *env);   /* 0x004f5920 */
void aoe_tribe_setup_hittites_ror(int32_t *values, int32_t map_type,
                                  const AoeTribeSetupEnvRor *env);  /* 0x004f5a50 */
void aoe_tribe_setup_phoenicians_ror(int32_t *values, int32_t map_type,
                                     const AoeTribeSetupEnvRor *env); /* 0x004f5b70 */
void aoe_tribe_setup_sumerians_ror(int32_t *values, int32_t map_type,
                                   const AoeTribeSetupEnvRor *env); /* 0x004f5c80 */
void aoe_tribe_setup_persians_ror(int32_t *values, int32_t map_type,
                                  const AoeTribeSetupEnvRor *env);  /* 0x004f5dc0 */
void aoe_tribe_setup_shang_ror(int32_t *values, int32_t map_type,
                               const AoeTribeSetupEnvRor *env);     /* 0x004f5ef0 */
void aoe_tribe_setup_yamato_ror(int32_t *values, int32_t map_type,
                                const AoeTribeSetupEnvRor *env);    /* 0x004f6020 */
void aoe_tribe_setup_choson_ror(int32_t *values, int32_t map_type,
                                const AoeTribeSetupEnvRor *env);    /* 0x004f6170 */
void aoe_tribe_setup_roman_ror(int32_t *values, int32_t map_type,
                               const AoeTribeSetupEnvRor *env);     /* 0x004f62a0 */
void aoe_tribe_setup_carthaginian_ror(int32_t *values, int32_t map_type,
                                      const AoeTribeSetupEnvRor *env); /* 0x004f63d0 */
void aoe_tribe_setup_palmyran_ror(int32_t *values, int32_t map_type,
                                  const AoeTribeSetupEnvRor *env);  /* 0x004f6500 */
void aoe_tribe_setup_macedonian_ror(int32_t *values, int32_t map_type,
                                    const AoeTribeSetupEnvRor *env); /* 0x004f6640 */

/* The build-list chooser's civilisation switch: civilisation byte
   (+0x109) 1..16 in the order above (Egyptians = 1 … Macedonian = 16);
   other values set nothing. Returns 0 for those. */
int aoe_tribe_setup_civ_ror(uint8_t civ, int32_t *values, int32_t map_type,
                            const AoeTribeSetupEnvRor *env);

#endif
